#include "model.h"
#include "logical.h"
#include "physical.h"

DiagramNote *Model::note(const QString &id){for(auto &n:notes)if(n.id==id)return &n;return nullptr;}
const DiagramNote *Model::note(const QString &id)const{for(const auto &n:notes)if(n.id==id)return &n;return nullptr;}
QString newId(){return QUuid::createUuid().toString(QUuid::WithoutBraces);}
Object *Model::get(const QString &id){for(auto &o:objects)if(o.id==id)return &o;return nullptr;}
const Object *Model::get(const QString &id)const{for(const auto &o:objects)if(o.id==id)return &o;return nullptr;}
QVector<Object> Model::attributes(const QString &e)const{QVector<Object> r;for(const auto &o:objects)if(o.kind=="attribute"&&o.owner==e)r<<o;return r;}
bool Model::weak(const QString &id)const{for(const auto &o:objects)if(o.kind=="relation"&&o.identifying&&o.b==id)return true;return false;}
QStringList Model::ancestors(const QString &entity)const{
 QHash<QString,QString> parent;for(const auto &o:objects)if(o.kind=="isa"&&!parent.contains(o.a))parent[o.a]=o.b;
 QStringList result;QSet<QString> seen{entity};QString id=parent.value(entity);while(!id.isEmpty()&&!seen.contains(id)){seen.insert(id);result<<id;id=parent.value(id);}return result;
}
QVector<Object> Model::inheritedAttributes(const QString &entity)const{
 QHash<QString,QVector<Object>> attrs;for(const auto &o:objects)if(o.kind=="attribute")attrs[o.owner]<<o;
 QVector<Object> result;auto parents=ancestors(entity);for(auto i=parents.crbegin();i!=parents.crend();++i)result+=attrs.value(*i);return result;
}
QJsonObject Model::json()const{
 QJsonArray list;
 for(const auto &o:objects){QJsonObject j=QJsonObject{{"id",o.id},{"kind",o.kind},{"name",o.name},{"owner",o.owner},{"a",o.a},{"b",o.b},{"roleA",o.roleA},{"roleB",o.roleB},{"cardA",o.cardA},{"cardB",o.cardB},{"key",o.key},{"identifying",o.identifying},{"chen",QJsonArray{o.chen.x(),o.chen.y()}},{"uml",QJsonArray{o.uml.x(),o.uml.y()}}};if(!o.dataType.isEmpty())j["dataType"]=o.dataType;list.append(j);}
 bool typed=logical.value("version").toInt()==2;for(const auto &o:objects)typed|=!o.dataType.isEmpty();
 QJsonObject root{{"format","SchemaStudio"},{"version",!notes.isEmpty()?5:!physical.isEmpty()?4:typed?3:logical.isEmpty()?1:2},{"objects",list}};if(!logical.isEmpty())root["logical"]=logical;if(!physical.isEmpty())root["physical"]=physical;if(!notes.isEmpty()){QJsonArray a;for(const auto &n:notes)a<<QJsonObject{{"id",n.id},{"text",n.text},{"chen",QJsonArray{n.chen.x(),n.chen.y()}},{"uml",QJsonArray{n.uml.x(),n.uml.y()}},{"logical",QJsonArray{n.logical.x(),n.logical.y()}},{"size",QJsonArray{n.size.width(),n.size.height()}}};root["notes"]=a;}return root;
}
bool Model::parse(const QByteArray &bytes,Model &out,QString &error){
 if(bytes.size()>20*1024*1024){error="File too large / File troppo grande (20 MB)";return false;}
 QJsonParseError pe;auto doc=QJsonDocument::fromJson(bytes,&pe);
 auto fail=[&](const QString &s){error=s;return false;};
 if(pe.error!=QJsonParseError::NoError||!doc.isObject())return fail("Invalid JSON / JSON non valido");
 auto root=doc.object();
 int version=root.value("version").toInt();if(root.value("format")!="SchemaStudio"||root.value("version")!=version||(version!=1&&version!=2&&version!=3&&version!=4&&version!=5)||!root.value("objects").isArray()||(version==1&&root.contains("logical")))return fail("Unsupported format / Formato non supportato");
 auto array=root.value("objects").toArray();if(array.size()>20000)return fail("Too many objects / Troppi oggetti");
 Model candidate;QSet<QString> ids;const QStringList kinds={"entity","attribute","relation","isa"},cards={"0,1","1,1","0,N","1,N"};
 for(const auto &v:array){
  if(!v.isObject())return fail("Invalid object");
  auto j=v.toObject();Object o;
  for(const QString k:{"id","kind","name","owner","a","b","roleA","roleB","cardA","cardB"})if(!j.value(k).isString())return fail("Missing or invalid field: "+k);
  if(!j.value("key").isBool()||!j.value("identifying").isBool())return fail("Invalid flags");
  if(!readGenericType(j,"dataType",o.dataType))return fail("Invalid generic data type");
  o.id=j["id"].toString();o.kind=j["kind"].toString();o.name=j["name"].toString();o.owner=j["owner"].toString();o.a=j["a"].toString();o.b=j["b"].toString();o.roleA=j["roleA"].toString();o.roleB=j["roleB"].toString();o.cardA=j["cardA"].toString();o.cardB=j["cardB"].toString();o.key=j["key"].toBool();o.identifying=j["identifying"].toBool();
  if((o.kind!="attribute"&&!o.dataType.isEmpty()))return fail("Only attributes have a generic data type");
  if((o.kind!="attribute"&&(!o.owner.isEmpty()||o.key))||
     (o.kind!="relation"&&o.identifying)||
     ((o.kind=="entity"||o.kind=="attribute")&&(!o.a.isEmpty()||!o.b.isEmpty())))return fail("Invalid object references or flags / Riferimenti o opzioni non validi");
  if(o.id.isEmpty()||o.id.size()>100||ids.contains(o.id)||!kinds.contains(o.kind)||o.name.size()>200||o.roleA.size()>100||o.roleB.size()>100||!cards.contains(o.cardA)||!cards.contains(o.cardB))return fail("Invalid object identity or values");
  for(const QString k:{"chen","uml"}){
   auto a=j[k].toArray();if(a.size()!=2||!a[0].isDouble()||!a[1].isDouble())return fail("Invalid position");
   double x=a[0].toDouble(),y=a[1].toDouble();if(!std::isfinite(x)||!std::isfinite(y)||std::abs(x)>100000||std::abs(y)>100000)return fail("Position out of range");
   (k=="chen"?o.chen:o.uml)=QPointF(x,y);
  }
  ids.insert(o.id);candidate.objects<<o;
 }
 for(const auto &o:candidate.objects){
  auto entity=[&](const QString &id){auto e=candidate.get(id);return e&&e->kind=="entity";};
  if(o.kind=="attribute"&&!entity(o.owner))return fail("Attribute owner missing");
  if((o.kind=="relation"||o.kind=="isa")&&(!entity(o.a)||!entity(o.b)))return fail("Relationship endpoint missing");
 }
 if(version==2||(version>=3&&root.contains("logical"))){LogicalSchema schema;if(!root["logical"].isObject()||!LogicalSchema::parse(root["logical"].toObject(),schema,error))return fail("Invalid logical model");candidate.logical=schema.json();}
 if(version==4||(version==5&&root.contains("physical"))){if(candidate.logical.isEmpty()||!root["physical"].isObject()||!validPhysicalStorage(root["physical"].toObject(),error))return fail("Invalid physical configuration / Configurazione fisica non valida");candidate.physical=root["physical"].toObject();
  // Drop retired 0.9.4 options once on load; preserve the user's SQL draft.
  if(candidate.physical.contains("includeNotes")||candidate.physical.contains("generatedNotesDigest")){candidate.physical.remove("includeNotes");candidate.physical.remove("generatedNotesDigest");candidate.physical["generatedDigest"]="";}}
 else if(root.contains("physical"))return fail("Physical settings require format v4 / Il fisico richiede il formato v4");
 if(root.contains("notes")){
  if(version!=5||!root["notes"].isArray()||root["notes"].toArray().size()>500)return fail("Invalid notes / Note non valide");
  for(const auto &value:root["notes"].toArray()){
   if(!value.isObject())return fail("Invalid note / Nota non valida");
   auto j=value.toObject();DiagramNote n;
   if(!j["id"].isString()||!j["text"].isString())return fail("Invalid note text / Testo nota non valido");
   n.id=j["id"].toString();n.text=j["text"].toString();
   if(!n.id.startsWith("note-")||n.id.size()>100||ids.contains(n.id)||n.text.size()>10000)return fail("Invalid note identity or length / Identificatore o lunghezza nota non validi");
   for(const auto &key:{"chen","uml","logical","size"}){auto a=j[key].toArray();if(a.size()!=2||!a[0].isDouble()||!a[1].isDouble())return fail("Invalid note geometry / Geometria nota non valida");double x=a[0].toDouble(),y=a[1].toDouble();if(!std::isfinite(x)||!std::isfinite(y)||std::abs(x)>100000||std::abs(y)>100000)return fail("Invalid note position / Posizione nota non valida");if(QString(key)=="size"){if(x<160||x>2000||y<80||y>2000)return fail("Invalid note size / Dimensione nota non valida");n.size={x,y};}else (QString(key)=="chen"?n.chen:QString(key)=="uml"?n.uml:n.logical)={x,y};}
   ids.insert(n.id);candidate.notes<<n;
  }
 }
 out=candidate;return true;
}
QStringList Model::issues(bool en)const{
 QStringList r;auto t=[&](const char *it,const char *eng){return QString::fromUtf8(en?eng:it);};
 QSet<QString> names;
 for(const auto &o:objects){
  if(!validGenericType(o.dataType)||(o.kind!="attribute"&&!o.dataType.isEmpty()))r<<t("Tipo generico non valido.","Invalid generic data type.");
  if(o.name.trimmed().isEmpty())r<<t("Nome vuoto: ","Empty name: ")+(o.kind=="entity"?t("entità","entity"):o.kind=="attribute"?t("attributo","attribute"):o.kind=="relation"?t("associazione","relationship"):QString("ISA"));
  if(o.kind=="entity"||o.kind=="attribute"){
   QString scope=(o.kind=="entity"?"entity":o.owner)+":"+o.name.trimmed().toCaseFolded();
   if(names.contains(scope))r<<t("Nome duplicato: ","Duplicate name: ")+o.name;
   names.insert(scope);
  }
  if(o.kind=="entity"){
   bool keyFound=false,inherited=false;for(const auto &a:attributes(o.id))keyFound|=a.key;
   for(const auto &i:objects)if(i.kind=="isa"&&i.a==o.id)inherited=true;
   auto local=attributes(o.id);if(!local.isEmpty())for(const auto &base:inheritedAttributes(o.id))for(const auto &attr:local)if(attr.name.trimmed().compare(base.name.trimmed(),Qt::CaseInsensitive)==0){auto owner=get(base.owner);QString source=owner?owner->name:base.owner;r<<o.name+"."+attr.name+t(": attributo già ereditato da ",": attribute already inherited from ")+source+t(". Modifica l’origine invece di duplicarlo.",". Edit the source instead of duplicating it.");if(!attr.dataType.isEmpty()&&!base.dataType.isEmpty()&&attr.dataType!=base.dataType)r<<o.name+"."+attr.name+t(": tipo incompatibile con l’attributo ereditato.",": type conflicts with the inherited attribute.");}
   QVector<Object> owners;for(const auto &rel:objects)if(rel.kind=="relation"&&rel.identifying&&rel.b==o.id)owners<<rel;
   if(inherited&&!owners.isEmpty())r<<o.name+t(": combina ISA e identificazione esterna; scegli un solo meccanismo di identificazione.",": combines ISA and external identification; choose one identification mechanism.");
   if(!inherited&&!keyFound&&owners.size()==1&&owners.first().cardA.endsWith("N"))r<<o.name+t(": manca una chiave parziale per distinguere le istanze dello stesso proprietario.",": a partial key is needed to distinguish instances of the same owner.");
   if(!keyFound&&!weak(o.id)&&!inherited)r<<o.name+t(": identificatore da definire.",": identifier missing.");
  }
  if(o.kind=="relation"){
   if(o.a==o.b&&o.roleA.trimmed()==o.roleB.trimmed())r<<o.name+t(": distinguere i ruoli della ricorsiva.",": distinguish recursive roles.");
   if(o.identifying&&(o.cardB!="1,1"||o.a==o.b))r<<o.name+t(": identificazione esterna non valida.",": invalid external identification.");
  }
  if(o.kind=="isa"){
   int parents=0;for(const auto &i:objects)if(i.kind=="isa"&&i.a==o.a)++parents;
   if(parents>1)r<<t("ISA: è supportata una sola superclasse.","ISA: only one superclass supported.");
  }
 }
 for(const auto &kind:{QString("isa"),QString("identifying")}){
  QHash<QString,int> marks;QHash<QString,QStringList> edges;
  for(const auto &o:objects){if(kind=="isa"&&o.kind=="isa")edges[o.a]<<o.b;else if(kind=="identifying"&&o.kind=="relation"&&o.identifying)edges[o.b]<<o.a;}
  bool cycle=false;
  for(const auto &entity:objects){if(entity.kind!="entity"||marks.value(entity.id)==2)continue;QVector<QPair<QString,qsizetype>> stack;stack.append({entity.id,0});marks[entity.id]=1;
   while(!stack.isEmpty()&&!cycle){auto &frame=stack.last();const auto neighbors=edges.value(frame.first);if(frame.second>=neighbors.size()){marks[frame.first]=2;stack.removeLast();continue;}auto next=neighbors[frame.second++];if(marks.value(next)==1){cycle=true;break;}if(marks.value(next)==0){marks[next]=1;stack.append({next,0});}}
   if(cycle){r<<t("Ciclo nelle dipendenze: ","Dependency cycle: ")+(kind=="isa"?QString("ISA"):t("identificazione esterna","external identification"));break;}
  }
 }
 r.removeDuplicates();return r;
}
void Model::remove(const QString &id){
 QSet<QString> removed{id};bool changed=true;
 while(changed){changed=false;for(const auto &o:objects)if(!removed.contains(o.id)&&(removed.contains(o.owner)||removed.contains(o.a)||removed.contains(o.b))){removed.insert(o.id);changed=true;}}
 for(int i=objects.size()-1;i>=0;--i)if(removed.contains(objects[i].id))objects.removeAt(i);
}
Model Model::example(){
 Model m;Object a;a.id="ordine";a.kind="entity";a.name="Ordine";a.chen={180,220};a.uml={150,200};m.objects<<a;
 Object b=a;b.id="riga";b.name="RigaOrdine";b.chen={720,220};b.uml={690,200};m.objects<<b;
 auto attr=[&](QString id,QString name,QString owner,bool key,QPointF pos){Object o;o.id=id;o.kind="attribute";o.name=name;o.owner=owner;o.key=key;o.chen=pos;o.uml=pos;m.objects<<o;};
 attr("numordine","NumeroOrdine","ordine",true,{120,70});attr("data","Data","ordine",false,{300,80});
 attr("numriga","NumeroRiga","riga",true,{650,70});attr("quantita","Quantita","riga",false,{860,80});
 Object rel;rel.id="contiene";rel.kind="relation";rel.name="Contiene";rel.a="ordine";rel.b="riga";rel.chen={450,220};rel.uml={420,210};rel.identifying=true;rel.roleA="ordine";rel.roleB="riga";m.objects<<rel;
 return m;
}
bool saveModel(const QString &path,const Model &m,QString &error){QSaveFile file(path);if(!file.open(QIODevice::WriteOnly)){error=file.errorString();return false;}auto bytes=QJsonDocument(m.json()).toJson();if(file.write(bytes)!=bytes.size()){error=file.errorString();return false;}if(!file.commit()){error=file.errorString();return false;}return true;}
bool loadModel(const QString &path,Model &m,QString &error){QFile f(path);if(!f.open(QIODevice::ReadOnly)){error=f.errorString();return false;}if(f.size()>20*1024*1024){error="File too large / File troppo grande (20 MB)";return false;}auto bytes=f.read(20*1024*1024+1);if(f.error()!=QFile::NoError){error=f.errorString();return false;}return Model::parse(bytes,m,error);}
