#define main application_main
#include "../main.cpp"
#undef main
#include <iostream>

int main(int argc,char **argv){
 QApplication app(argc,argv);app.setStyle("Fusion");QCoreApplication::setApplicationVersion("0.9.2");
 if(app.arguments().size()<2)return 2;
 const QString dir=app.arguments().at(1);const bool final=app.arguments().contains("--final");QDir().mkpath(dir+"/immagini");
 QJsonArray tests;int failures=0;auto check=[&](QString name,bool ok){tests<<QJsonObject{{"check",name},{"ok",ok}};if(!ok){++failures;std::cerr<<name.toStdString()<<" FAILED\n";}};
 QString error;Model conceptual;QElapsedTimer timer;timer.start();check("Apertura concettuale",loadModel(dir+"/01-Test-limite-Azienda-concettuale.schema",conceptual,error));auto loadMs=timer.nsecsElapsed()/1e6;
 check("Concettuale privo di errori",conceptual.issues().isEmpty());check("File concettuale senza logico",conceptual.logical.isEmpty());
 if(failures)return 3;
 Window w(true);w.en=false;w.mode=1;w.showTypes=true;w.uml=true;w.model=conceptual;w.applyTheme();w.buildMenus();w.resize(1500,940);w.show();qApp->processEvents();w.resizeDocks({w.outline,w.properties},{210,350},Qt::Horizontal);
 timer.restart();w.refresh();w.fit();qApp->processEvents();double conceptualRenderMs=timer.nsecsElapsed()/1e6;
 if(final){check("Apertura file con logico",loadModel(dir+"/02-Test-limite-Azienda-logico.schema",w.model,error));}
 else{
  bool clicked=false;QTimer::singleShot(40,&w,[&]{auto dialog=w.findChild<QDialog*>("mappingDialog");if(!dialog)return;auto apply=dialog->findChild<QPushButton*>("applyMapping");clicked=apply&&apply->isEnabled();if(clicked)apply->click();else dialog->reject();});
  check("Traduzione dalla finestra dell'app",w.generateDialog()&&clicked);
 }
 LogicalSchema schema;check("Parsing logico",LogicalSchema::parse(w.model.logical,schema,error));if(failures)return 4;
 if(!final){for(auto &t:schema.tables){auto o=conceptual.get(t.source);t.position=o?o->uml*1.3:QPointF();}w.model.logical=schema.json();}
 auto table=[&](QString source)->const LogicalTable*{for(const auto &t:schema.tables)if(t.source==source)return &t;return nullptr;};
 check("27 tabelle: 23 entita e 4 N:M",schema.tables.size()==27);
 check("Concettuale conservato identico",w.model.json()["objects"]==conceptual.json()["objects"]);
 check("Logico aggiornato",!logicalStale(w.model));check("Nessun errore logico",schema.issues().isEmpty());
 QMap<QString,int> keySizes{{"soggetto",2},{"cliente",2},{"azienda",2},{"dipendente",2},{"fornitore",2},{"ordine",2},{"riga",3},{"tranche",4},{"ubicazione",2},{"lotto",2},{"scorta",4},{"offerta",4},{"incarico",4},{"pagamento",3},{"classifica",2},{"composizione",2},{"finanzia",3},{"soddisfa",4}};
 for(auto it=keySizes.begin();it!=keySizes.end();++it)check("Componenti PK: "+it.key(),table(it.key())&&table(it.key())->primaryKey().size()==it.value());
 for(auto id:{"cliente","azienda","dipendente","fornitore"})check("Chiave locale UNIQUE: "+QString(id),table(id)&&table(id)->uniqueKeys.size()==1);
 check("ISA su due livelli",conceptual.ancestors("azienda").size()==2&&conceptual.inheritedAttributes("azienda").size()==6);
 check("1:1 badge: FK lato B",schema.oneToOne.value("assegna")=="B"&&table("badge")->uniqueKeys.size()==1);
 check("1:1 fattura: FK lato B",schema.oneToOne.value("fatturazione")=="B"&&table("fattura")->uniqueKeys.size()==1);
 int columns=0,fks=0,compositeFks=0,nullableComposite=0,generated=0,origins=0,maxDepth=0,weak=0,warnings=0,uniqueCount=0;
 QJsonArray provenance;QJsonArray remaining;QJsonArray tableSummary;
 for(const auto &o:conceptual.objects)if(o.kind=="entity"&&conceptual.weak(o.id))++weak;
 for(const auto &t:schema.tables){
  columns+=t.columns.size();fks+=t.foreignKeys.size();uniqueCount+=t.uniqueKeys.size();QStringList pk;
  for(auto id:t.primaryKey())pk<<t.column(id)->name;
  for(const auto &f:t.foreignKeys){if(f.columns.size()>1){++compositeFks;if(t.column(f.columns.first())->nullable)++nullableComposite;}}
  for(const auto &c:t.columns){const auto trace=columnOrigin(schema,c.id);const auto attr=conceptual.get(trace.attribute);bool ok=trace.complete&&attr&&attr->kind=="attribute"&&attr->dataType==c.dataType;origins+=ok;generated+=c.generated;maxDepth=qMax(maxDepth,int(trace.relations.size()));check("Origine e dominio: "+t.name+"."+c.name,ok);if(c.generated)provenance<<QJsonObject{{"table",t.name},{"column",c.name},{"attribute",trace.attribute},{"chain",QJsonArray::fromStringList(trace.columns)}};}
  tableSummary<<QJsonObject{{"table",t.name},{"pk",QJsonArray::fromStringList(pk)},{"columns",int(t.columns.size())},{"fks",int(t.foreignKeys.size())},{"unique",int(t.uniqueKeys.size())}};
 }
 for(const auto &msg:schema.messages)if(msg.warning){++warnings;remaining<<msg.it;}
 check("FK composte facoltative presenti",nullableComposite>=2);check("Provenienza attraversa almeno tre legami",maxDepth>=3);check("Vincoli residui dichiarati",warnings>=4);
 QStringList generationErrors;timer.restart();auto again=generateLogical(conceptual,schema.oneToOne,generationErrors);auto generationMs=timer.nsecsElapsed()/1e6;preserveLogicalEdits(again,schema);
 check("Rigenerazione senza errori",generationErrors.isEmpty());check("Rigenerazione idempotente con layout",again.json()==schema.json());
 check("Confronto rigenerazione vuoto",logicalDifferences(schema,again).isEmpty());
 // A transient mutation verifies that types propagate through composed keys and ISA.
 Model changed=conceptual;changed.get("soggetto_Numero")->dataType="string";QStringList errors;auto mutation=generateLogical(changed,schema.oneToOne,errors);int propagated=0;bool propagationOk=errors.isEmpty();
 for(const auto &t:mutation.tables)for(const auto &c:t.columns)if(columnOrigin(mutation,c.id).attribute=="soggetto_Numero"){++propagated;propagationOk&=c.dataType=="string";}
 check("Propagazione transitoria tipo attraverso ISA/FK",propagationOk&&propagated>=10);
 check("Salvataggio concettuale",saveModel(dir+"/01-Test-limite-Azienda-concettuale.schema",conceptual,error));check("Salvataggio logico",saveModel(dir+"/02-Test-limite-Azienda-logico.schema",w.model,error));
 for(auto name:{"01-Test-limite-Azienda-concettuale.schema","02-Test-limite-Azienda-logico.schema"}){Model reopened;check("Riapertura "+QString(name),loadModel(dir+"/"+name,reopened,error)&&reopened.json()==(QString(name).startsWith("01")?conceptual.json():w.model.json()));}
 QJsonObject dimensions,imageBounds;
 auto measure=[&](QString view){QJsonArray list;QVector<Node*> top;for(const auto &id:w.nodes.keys()){auto n=w.nodes.value(id);if(n->parentItem())continue;top<<n;list<<QJsonObject{{"id",id},{"width",n->size.width()},{"height",n->size.height()},{"x",n->pos().x()},{"y",n->pos().y()}};}dimensions[view]=list;bool clear=true;for(int i=0;i<top.size();++i)for(int j=i+1;j<top.size();++j)clear&=!top[i]->sceneBoundingRect().intersects(top[j]->sceneBoundingRect());check("Nodi senza sovrapposizioni: "+view,clear);};
 auto render=[&](QString filename){QRectF rect=w.scene->itemsBoundingRect().adjusted(-50,-50,50,50);const double scale=qMin(1.4,12000./qMax(rect.width(),rect.height()));QImage image((rect.size()*scale).toSize(),QImage::Format_ARGB32_Premultiplied);image.fill(Qt::white);QPainter painter(&image);painter.setRenderHint(QPainter::Antialiasing);w.scene->render(&painter,QRectF(QPointF(0,0),image.size()),rect);painter.end();check("Immagine "+filename,image.save(dir+"/immagini/"+filename+".png"));imageBounds[filename]=QJsonObject{{"x",rect.x()},{"y",rect.y()},{"width",rect.width()},{"height",rect.height()},{"pixelWidth",image.width()},{"pixelHeight",image.height()}};};
 w.logicalView=false;w.uml=false;w.refresh();measure("chen");if(final)render("01-concettuale-Chen");
 w.uml=true;w.refresh();measure("uml");if(final)render("02-concettuale-UML");
 w.logicalView=true;w.buildMenus();timer.restart();w.refresh();w.fit();qApp->processEvents();auto logicalRenderMs=timer.nsecsElapsed()/1e6;measure("logical");if(final)render("03-logico");
 QFile dims(dir+"/dimensioni.json");if(!dims.open(QIODevice::WriteOnly))return 5;dims.write(QJsonDocument(dimensions).toJson());
 if(final){QFile bounds(dir+"/immagini/geometria.json");if(!bounds.open(QIODevice::WriteOnly))return 5;bounds.write(QJsonDocument(imageBounds).toJson());}
 if(final){
  QFile mapping(dir+"/rapporto-traduzione.html");if(!mapping.open(QIODevice::WriteOnly))return 5;mapping.write(("<!doctype html><html lang=it><meta charset=utf-8><title>Azienda - rapporto di traduzione</title><style>body{font:16px/1.6 system-ui;max-width:1100px;margin:3rem auto;padding:0 2rem;color:#172334}h3{color:#234fa2}li{margin:.65rem 0}</style>"+w.schemaReport(schema)).toUtf8());
 }
 QJsonObject counts{{"entities",23},{"attributes",53},{"relationships",24},{"isa",6},{"weakEntities",weak},{"tables",int(schema.tables.size())},{"columns",columns},{"foreignKeys",fks},{"compositeForeignKeys",compositeFks},{"optionalCompositeForeignKeys",nullableComposite},{"uniqueGroups",uniqueCount},{"generatedColumns",generated},{"provenanceVerified",origins},{"maxOriginHops",maxDepth},{"remainingConstraints",warnings}};
 int attrs=0;for(const auto&o:conceptual.objects)attrs+=o.kind=="attribute";counts["attributes"]=attrs;counts["objects"]=int(conceptual.objects.size());
 QJsonObject report{{"applicationVersion","0.9.2"},{"qt",qVersion()},{"failures",failures},{"checks",tests},{"counts",counts},{"remainingConstraints",remaining},{"tables",tableSummary},{"provenance",provenance},{"timingsMs",QJsonObject{{"loadConceptual",loadMs},{"generateLogical",generationMs},{"refreshConceptualAndFit",conceptualRenderMs},{"refreshLogicalAndFit",logicalRenderMs}}}};
 QFile output(dir+"/verifica.json");if(!output.open(QIODevice::WriteOnly))return 5;output.write(QJsonDocument(report).toJson());std::cout<<QJsonDocument(counts).toJson().toStdString()<<"Checks: "<<tests.size()<<" Failures: "<<failures<<"\n";
 w.hide();return failures?1:0;
}
