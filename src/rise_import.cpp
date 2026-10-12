#include "rise_import.h"

namespace {
const QString ns="http://www.r2bsoftware/ns/rise/";
struct X {
 QString tag,text;
 QVector<X> children;
 const X *find(const QString &name) const {for(const auto &c:children)if(c.tag==name)return &c;return nullptr;}
 QString value(const QString &name) const {auto c=find(name);return c?c->text:QString();}
};
struct Reader {
 bool en;
 QString error;
 int nodes=0;
 QString tr(const char *it,const char *eng)const{return QString::fromUtf8(en?eng:it);}
 bool fail(QString detail){error=tr("Importazione RISE: ","RISE import: ")+detail;return false;}
 bool element(QXmlStreamReader &xml,X &out,int depth=0){
  if(depth>48||++nodes>150000)return fail(tr("documento troppo complesso.","document too complex."));
  if(xml.namespaceUri()!=ns)return fail(tr("namespace non supportato.","unsupported namespace."));
  out.tag=xml.name().toString();
  while(!xml.atEnd()){
   auto token=xml.readNext();
   if(token==QXmlStreamReader::StartElement){X c;if(!element(xml,c,depth+1))return false;out.children<<c;}
   else if(token==QXmlStreamReader::Characters){out.text+=xml.text();if(out.text.size()>1000000)return fail(tr("testo troppo lungo.","text too long."));}
   else if(token==QXmlStreamReader::EndElement)return true;
   else if(token==QXmlStreamReader::EntityReference)return fail(tr("entità XML non supportata.","unsupported XML entity."));
  }
  return false;
 }
};
}

Model RiseImport::positioned(int diagram)const{
 Model result=model;
 for(auto &e:result.objects)if(e.kind=="entity"){
  if(diagram>=0&&diagram<diagrams.size()&&diagrams[diagram].centers.contains(e.name))e.chen=e.uml=diagrams[diagram].centers[e.name];
 }
 for(auto &e:result.objects)if(e.kind=="attribute"){
  auto owner=result.get(e.owner);auto attrs=result.attributes(e.owner);int index=0;for(;index<attrs.size()&&attrs[index].id!=e.id;++index){}
  e.chen=owner->chen+QPointF((index%4-1.5)*160,-150-(index/4)*80);e.uml=owner->uml;
 }else if(e.kind=="relation"||e.kind=="isa"){
  auto a=result.get(e.a),b=result.get(e.b);e.chen=e.uml=(a->uml+b->uml)/2+(e.a==e.b?QPointF(200,150):QPointF());
 }
 return result;
}
QString RiseImport::report(bool english)const{
 int entities=0,attributes=0,relations=0,isa=0;for(const auto &o:model.objects){entities+=o.kind=="entity";attributes+=o.kind=="attribute";relations+=o.kind=="relation";isa+=o.kind=="isa";}
 auto tr=[&](const char *it,const char *en){return QString::fromUtf8(english?en:it);};
 QString text=tr("Modello: ","Model: ")+name+"\n"+tr("Motore dichiarato: ","Declared engine: ")+engine+"\n"+tr("Operazioni interpretate: ","Operations interpreted: ")+QString::number(operations)+"\n";
 text+=QString(tr("Entità: %1 · Attributi: %2 · Associazioni: %3 · ISA: %4","Entities: %1 · Attributes: %2 · Relationships: %3 · ISA: %4")).arg(entities).arg(attributes).arg(relations).arg(isa)+"\n\n";
 for(const auto &n:notes)text+="• "+(english?n.en:n.it)+"\n";
 return text;
}
bool loadRise(const QString &path,RiseImport &out,QString &error,bool english){QFile file(path);if(!file.open(QIODevice::ReadOnly)){error=file.errorString();return false;}if(file.size()>20*1024*1024){error=english?"RISE file exceeds 20 MB.":"Il file RISE supera 20 MB.";return false;}auto bytes=file.read(20*1024*1024+1);if(file.error()!=QFile::NoError){error=file.errorString();return false;}return readRise(bytes,out,error,english);}

bool readRise(const QByteArray &bytes,RiseImport &output,QString &error,bool english){
 Reader reader{english,{},0};auto tr=[&](const char *it,const char *en){return reader.tr(it,en);};
 auto fail=[&](QString detail){reader.fail(detail);error=reader.error;return false;};
 if(bytes.size()>20*1024*1024)return fail(tr("file oltre 20 MB.","file exceeds 20 MB."));
 QXmlStreamReader xml(bytes);X root;bool found=false;
 while(!xml.atEnd()){
  auto token=xml.readNext();
  if(token==QXmlStreamReader::DTD){
   if(!xml.dtdSystemId().isEmpty()||!xml.dtdPublicId().isEmpty()||!xml.entityDeclarations().isEmpty()||xml.text().contains(u"<!ENTITY"))return fail(tr("DTD esterne o dichiarazioni di entità non ammesse.","external DTDs or entity declarations are not allowed."));
   auto match=QRegularExpression("DTD for RISE XML ([0-9.]+)").match(xml.text().toString());if(match.hasMatch()&&match.captured(1)!="1.5")return fail(tr("versione RISE XML non supportata: ","unsupported RISE XML version: ")+match.captured(1));
  }
  if(token==QXmlStreamReader::StartElement){if(found)return fail(tr("più radici XML.","multiple XML roots."));found=true;if(!reader.element(xml,root)){error=reader.error.isEmpty()?tr("XML non valido: ","Invalid XML: ")+xml.errorString():reader.error;return false;}}
 }
 if(xml.hasError()||!found||root.tag!="model")return fail(tr("XML non valido o radice non riconosciuta.","invalid XML or unrecognized root."));
 auto info=root.find("modelInfo"),evolution=root.find("evolution");
 if(!info||!evolution)return fail(tr("modelInfo/evolution mancanti.","missing modelInfo/evolution."));
 if(info->find("base"))return fail(tr("il modello riferisce una base esterna: importazione non supportata.","model references an external base: import is not supported."));
 RiseImport result;result.name=info->value("name");result.engine=info->value("engineVersion");
 auto note=[&](QString it,QString en){for(const auto &n:result.notes)if(n.it==it)return;result.notes<<RiseNote{it,en};};
 QMap<QString,QString> entities,relations,specializations;QMap<QString,QString> parents;
 auto nameOk=[](QString name){return !name.trimmed().isEmpty()&&name.size()<=200;};
 bool bad=false;
 auto reject=[&](QString text){if(!bad){error=tr("Importazione RISE: ","RISE import: ")+text;bad=true;}};
 auto entityId=[&](QString name){auto id=entities.value(name);if(id.isEmpty())reject(tr("entità non trovata: ","entity not found: ")+name);return id;};
 auto unsupportedFields=[&](const X &node,const QSet<QString>&known){for(const auto &c:node.children)if(!known.contains(c.tag))note("Informazione non importata: "+node.tag+"/"+c.tag,"Information not imported: "+node.tag+"/"+c.tag);};
 auto deleted=[](const X &x){return x.value("deleted").compare("true",Qt::CaseInsensitive)==0;};
 auto readAttribute=[&](const X &x,QString owner,bool edit){
  QString name=x.value("name");if(!nameOk(name)){reject(tr("nome attributo non valido.","invalid attribute name."));return;}
  QString id;for(const auto &o:result.model.objects)if(o.kind=="attribute"&&o.owner==owner&&o.name==name){id=o.id;break;}
  if(edit&&id.isEmpty()){reject(tr("attributo da modificare non trovato: ","attribute to edit not found: ")+name);return;}
  if(!edit&&!id.isEmpty()){reject(tr("attributo duplicato: ","duplicate attribute: ")+name);return;}
  if(deleted(x)){if(!id.isEmpty()){result.model.remove(id);result.keyCandidates.remove(id);}return;}
  if(id.isEmpty()){Object a;a.id=id=newId();a.kind="attribute";a.name=name;a.owner=owner;result.model.objects<<a;}
  if(x.value("mustBeUnique").compare("true",Qt::CaseInsensitive)==0&&x.value("mustExist").compare("true",Qt::CaseInsensitive)==0)result.keyCandidates.insert(id);else result.keyCandidates.remove(id);
  unsupportedFields(x,{"name","deleted","dataTypeAlias","dataType","dataSize","mustBeUnique","mustExist","default","check","fkAttribute","description","instance"});
  if(x.find("dataType")||x.find("mustExist")||x.find("mustBeUnique"))note("Tipi, dimensioni e vincoli SQL non sono importati nel concettuale. UNIQUE e obbligatorietà sono solo indizi per scegliere un identificatore.","SQL types, sizes and constraints are not imported into the conceptual model. UNIQUE and required values are only hints for choosing an identifier.");
  for(QString field:{"default","check","description","instance"})if(x.find(field))note("Contenuto escluso: attributo/"+field,"Excluded content: attribute/"+field);
  if(x.find("fkAttribute")||name.contains("{FK}",Qt::CaseInsensitive))note("I riferimenti FK degli attributi e il testo {FK} non diventano associazioni. Gli attributi restano ordinari: verifica eventuali duplicati prima di generare il logico.","Attribute FK references and {FK} text do not become relationships. Attributes remain ordinary: review possible duplicates before generating the logical model.");
 };
 auto addEntity=[&](const X &x){
  if(deleted(x))return;
  QString name=x.value("name");if(!nameOk(name)||entities.contains(name)){reject(tr("entità duplicata o non valida: ","duplicate or invalid entity: ")+name);return;}
  if(x.value("isDataType").compare("true",Qt::CaseInsensitive)==0){reject(tr("entità-tipo personalizzata non supportata: ","custom datatype entity is not supported: ")+name);return;}
  Object o;o.id=newId();o.name=name;o.kind="entity";int i=entities.size();o.chen=o.uml={double(i%4)*650,double(i/4)*600};entities[name]=o.id;result.model.objects<<o;
  for(const auto &c:x.children)if(c.tag=="attribute")readAttribute(c,o.id,false);
  for(const auto &c:x.children)if(c.tag=="index")note("Gli indici RISE non sono importati e non vengono convertiti automaticamente in identificatori.","RISE indexes are not imported or automatically converted to identifiers.");
  unsupportedFields(x,{"name","isDataType","attribute","index","deleted","maxID","ID"});
 };
 const QMap<QString,QString> cards{{"1","1,1"},{"0or1","0,1"},{"0toN","0,N"},{"1toN","1,N"}};
 auto addRelation=[&](const X &x,bool edit){
  QString name=x.value("name");if(deleted(x)){if(relations.contains(name)){result.model.remove(relations.take(name));}return;}
  if(!nameOk(name)||(edit?!relations.contains(name):relations.contains(name))){reject(tr("associazione duplicata o non trovata: ","duplicate or missing relationship: ")+name);return;}
  QVector<X> nodes;for(const auto &c:x.children)if(c.tag=="node")nodes<<c;
  if(nodes.size()!=2){reject(tr("è richiesta un’associazione binaria: ","a binary relationship is required: ")+name);return;}
  for(const auto &n:nodes)if(!cards.contains(n.value("cardinality"))||n.value("name").size()>100){reject(tr("cardinalità o ruolo non supportato: ","unsupported cardinality or role: ")+name);return;}
  Object o;o.id=edit?relations[name]:newId();o.kind="relation";o.name=name;o.a=entityId(nodes[0].value("entityName"));o.b=entityId(nodes[1].value("entityName"));o.roleA=nodes[0].value("name");o.roleB=nodes[1].value("name");
  // RISE stores multiplicity at an endpoint; Model stores participation of its role.
  o.cardA=cards[nodes[1].value("cardinality")];o.cardB=cards[nodes[0].value("cardinality")];if(bad)return;
  if(edit)*result.model.get(o.id)=o;else{relations[name]=o.id;result.model.objects<<o;}
  for(const auto &n:nodes){unsupportedFields(n,{"name","entityName","cardinality","attributeName","reference"});if(n.find("attributeName")||n.find("reference"))note("Dettagli colonna/istanze delle associazioni esclusi; importati solo estremi, ruoli e cardinalità.","Relationship column/instance details excluded; only endpoints, roles and cardinalities are imported.");}
  unsupportedFields(x,{"name","node","deleted","maxID","ID","relationHint"});if(x.find("relationHint"))note("Suggerimenti di traduzione delle associazioni esclusi.","Relationship mapping hints excluded.");
 };
 auto addIsa=[&](const X &x){
  if(deleted(x))return;
  QString name=x.value("name"),sub=entityId(x.value("subEntity")),super=entityId(x.value("superEntity"));if(bad)return;
  if(!nameOk(name)||specializations.contains(name)||parents.contains(sub)){reject(tr("ISA duplicata o ereditarietà multipla non supportata.","duplicate ISA or unsupported multiple inheritance."));return;}
  QString p=super;QSet<QString> seen{sub};while(!p.isEmpty()){if(seen.contains(p)){reject(tr("ciclo ISA.","ISA cycle."));return;}seen.insert(p);p=parents.value(p);}
  Object o;o.id=newId();o.kind="isa";o.name=name;o.a=sub;o.b=super;parents[sub]=super;specializations[name]=o.id;result.model.objects<<o;unsupportedFields(x,{"name","subEntity","superEntity","deleted","specialized"});
 };
 auto rename=[&](QMap<QString,QString>&map,QString from,QString to){if(!map.contains(from)||!nameOk(to)||(from!=to&&map.contains(to))){reject(tr("rinomina non valida: ","invalid rename: ")+from);return;}QString id=map.take(from);map[to]=id;result.model.get(id)->name=to;};
 qint64 previous=-1;
 for(const auto &op:evolution->children){
  if(++result.operations>30000)return fail(tr("troppe operazioni (limite 30000).","too many operations (limit 30000)."));
  bool ok=false;auto seq=op.value("sequenceNumber").toLongLong(&ok);if(!ok||seq<0||seq<=previous)return fail(tr("sequenza della cronologia non valida.","invalid history sequence."));previous=seq;
  auto x=op.find("entity"),r=op.find("relation"),s=op.find("specialization");QString tag=op.tag;
  if(tag=="newEntity"&&x)addEntity(*x);
  else if(tag=="renameEntity")rename(entities,op.value("entityName"),op.value("newEntityName"));
  else if(tag=="deleteEntity"){
   QString name=op.value("entityName"),id=entityId(name);if(!bad){result.model.remove(id);entities.remove(name);for(auto map:{&relations,&specializations})for(auto it=map->begin();it!=map->end();)if(!result.model.get(it.value()))it=map->erase(it);else ++it;parents.clear();for(const auto &o:result.model.objects)if(o.kind=="isa")parents[o.a]=o.b;}
  }else if((tag=="newAttribute"||tag=="editAttribute")&&x){QString owner=entityId(x->value("name"));int n=0;if(!bad)for(const auto &a:x->children)if(a.tag=="attribute"){++n;readAttribute(a,owner,tag=="editAttribute");}if(!n)reject(tr("operazione attributo vuota.","empty attribute operation."));}
  else if(tag=="renameAttribute"||tag=="deleteAttribute"||tag=="sortAttribute"){
   QString owner=entityId(op.value("entityName"));int index=-1;for(int i=0;i<result.model.objects.size();++i){const auto &a=result.model.objects[i];if(a.kind=="attribute"&&a.owner==owner&&a.name==op.value("attributeName")){index=i;break;}}
   if(index<0)reject(tr("attributo non trovato.","attribute not found."));
   else if(tag=="deleteAttribute"){result.keyCandidates.remove(result.model.objects[index].id);result.model.objects.removeAt(index);}
   else if(tag=="renameAttribute"){QString to=op.value("newAttributeName");if(!nameOk(to))reject(tr("nome attributo non valido.","invalid attribute name."));for(const auto &a:result.model.attributes(owner))if(a.name==to&&a.id!=result.model.objects[index].id)reject(tr("attributo duplicato.","duplicate attribute."));if(!bad)result.model.objects[index].name=to;}
   else note("L’ordine personalizzato degli attributi non è importato.","Custom attribute ordering is not imported.");
  }else if((tag=="newRelation"||tag=="editRelation")&&r)addRelation(*r,tag=="editRelation");
  else if(tag=="renameRelation")rename(relations,op.value("relationName"),op.value("newRelationName"));
  else if(tag=="deleteRelation"){QString name=op.value("relationName");if(!relations.contains(name))reject(tr("associazione da eliminare non trovata.","relationship to delete not found."));else result.model.remove(relations.take(name));}
  else if(tag=="newSpecialization"&&s)addIsa(*s);
  else if(tag=="deleteSpecialization"){QString name=op.value("specializationName");if(!specializations.contains(name))reject(tr("ISA da eliminare non trovata.","ISA to delete not found."));else{auto id=specializations.take(name);parents.remove(result.model.get(id)->a);result.model.remove(id);}}
  else if(tag=="snapshot"){
   result.model={};entities.clear();relations.clear();specializations.clear();parents.clear();result.keyCandidates.clear();
   for(const auto &c:op.children)if(c.tag=="entity")addEntity(c);
   for(const auto &c:op.children)if(c.tag=="relation")addRelation(c,false);else if(c.tag=="specialization")addIsa(c);
   for(const auto &c:op.children)if(!QSet<QString>{"sequenceNumber","timeStamp","releases","entity","relation","specialization"}.contains(c.tag))reject(tr("elemento snapshot non supportato: ","unsupported snapshot element: ")+c.tag);
  }else if(tag=="release"||tag=="noop"){}
  else if(tag=="newIndex"||tag=="deleteIndex"||tag=="newInstance"||tag=="editInstance"||tag=="deleteInstance")note("Indici e dati di esempio/istanze non sono importati.","Indexes and sample data/instances are not imported.");
  else reject(tr("operazione non supportata o incompleta: ","unsupported or incomplete operation: ")+tag);
  if(bad)return false;
  if(result.model.objects.size()>2000)return fail(tr("più di 2000 oggetti: limite dell’importatore didattico.","more than 2000 objects: educational importer limit."));
 }
 if(entities.isEmpty())return fail(tr("nessuna entità recuperabile.","no recoverable entities."));
 for(const auto &block:root.children){
  if(block.tag=="presentation"){
   for(const auto &d:block.children)if(d.tag=="diagram"){
    RiseDiagram diagram;diagram.name=d.value("name");for(const auto &draw:d.children)if(draw.tag=="drawEntity"){
     auto b=draw.find("boundingBox");if(!b)continue;bool ok[4];double x=b->value("x").toDouble(&ok[0]),y=b->value("y").toDouble(&ok[1]),w=b->value("sizeX").toDouble(&ok[2]),h=b->value("sizeY").toDouble(&ok[3]);
     if(ok[0]&&ok[1]&&ok[2]&&ok[3]&&std::isfinite(x)&&std::isfinite(y)&&std::isfinite(w)&&std::isfinite(h)&&w>=0&&h>=0&&qAbs(x)+w<20000&&qAbs(y)+h<20000)diagram.centers[draw.value("entityName")]={(x+w/2)*1.8,(y+h/2)*1.8};
     else note("Coordinate non valide: utilizzata una disposizione iniziale a griglia.","Invalid coordinates: initial grid layout used.");
    }else if(draw.tag!="name"&&draw.tag!="drawingMode"&&draw.tag!="drawingModeX")note("Oggetti grafici accessori RISE esclusi.","RISE auxiliary drawing objects excluded.");
    result.diagrams<<diagram;
   }
   note("Recuperate le posizioni disponibili. Dimensioni, colori e notazione vengono adattati alla vista UML-like dell’app.","Available positions recovered. Sizes, colors and notation follow the app's UML-like view.");
  }else if(block.tag!="modelInfo"&&block.tag!="evolution"&&(!block.children.isEmpty()||!block.text.trimmed().isEmpty()))note("Sezione non importata: "+block.tag,"Section not imported: "+block.tag);
 }
 note("Nessun identificatore è assegnato automaticamente. Seleziona solo gli attributi che formano l’identificatore concettuale; puoi completarli dopo l’importazione.","No identifier is assigned automatically. Select only the attributes forming the conceptual identifier; you can complete them after import.");
 note("Le associazioni importate non sono identificanti: RISE non dichiara qui un’identificazione esterna equivalente. Impostala nel concettuale soltanto se prevista dal modello.","Imported relationships are not identifying: an equivalent external identification is not declared here. Set it in the conceptual model only when intended.");
 Model verified;QString detail;if(!Model::parse(QJsonDocument(result.model.json()).toJson(),verified,detail))return fail(tr("struttura non valida: ","invalid structure: ")+detail);
 output=result;error.clear();return true;
}
