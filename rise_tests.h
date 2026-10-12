#pragma once
static void riseTests(Window &w,const QString &output,std::function<void(QString,bool)> check){
 QString error;RiseImport sample;
 auto fixture=QCoreApplication::applicationDirPath()+"/fixtures/iscrizioni.rise";
 bool loaded=loadRise(fixture,sample,error);check("RISE real sample loads without RISE runtime",loaded);if(!loaded)return;
 auto count=[](const Model &m,QString kind){int n=0;for(const auto &o:m.objects)n+=o.kind==kind;return n;};
 check("RISE replays 48 operations into 4 entities 15 attributes 3 relationships",sample.operations==48&&count(sample.model,"entity")==4&&count(sample.model,"attribute")==15&&count(sample.model,"relation")==3);
 bool direction=false,spelling=false,keys=false;for(const auto &o:sample.model.objects){direction|=o.name=="<Effetuare"&&sample.model.get(o.a)->name=="Iscritto"&&o.cardA=="0,N"&&o.cardB=="1,1";spelling|=o.name=="idIscirtto";keys|=o.key;}
 check("RISE endpoint multiplicities convert to role participation",direction);check("RISE preserves source spelling and never invents PKs",spelling&&!keys&&sample.keyCandidates.size()==6);
 check("RISE reports unsupported SQL constraints and textual FK ambiguity",sample.report().contains("{FK}")&&sample.report().contains("SQL")&&sample.report(true).contains("No identifier"));
 auto positioned=sample.positioned(0);bool layout=false;for(const auto&o:positioned.objects)if(o.name=="Pagamento"&&o.kind=="entity")layout=QLineF(o.uml,QPointF(657,273.6)).length()<0.001;check("RISE recovers entity centers and scales layout",layout);
 auto document=[](QString ops,QString extra=QString()){return ("<r:model xmlns:r=\"http://www.r2bsoftware/ns/rise/\"><r:modelInfo><r:name>Test</r:name>"+extra+"</r:modelInfo><r:evolution>"+ops+"</r:evolution></r:model>").toUtf8();};
 auto entity=[](QString name){return "<r:entity><r:name>"+name+"</r:name></r:entity>";};
 auto operation=[](int seq,QString tag,QString content){return "<r:"+tag+"><r:sequenceNumber>"+QString::number(seq)+"</r:sequenceNumber>"+content+"</r:"+tag+">";};
 auto node=[](QString name,QString role,QString card){return "<r:node><r:name>"+role+"</r:name><r:entityName>"+name+"</r:entityName><r:cardinality>"+card+"</r:cardinality></r:node>";};
 auto relation=[&](QString a,QString b,QString ca,QString cb){return "<r:relation><r:name>R</r:name>"+node(a,"parent",ca)+node(b,"child",cb)+"</r:relation>";};
 QString initial=operation(1,"newEntity",entity("A"))+operation(2,"newEntity",entity("B"));RiseImport out;
 bool allCards=true;QMap<QString,QString> cards{{"1","1,1"},{"0or1","0,1"},{"0toN","0,N"},{"1toN","1,N"}};for(auto a:cards.keys())for(auto b:cards.keys()){bool ok=readRise(document(initial+operation(3,"newRelation",relation("A","B",a,b))),out,error);allCards&=ok;if(ok){auto o=out.model.objects.last();allCards&=o.cardA==cards[b]&&o.cardB==cards[a];}}check("RISE all 16 endpoint cardinality combinations",allCards);
 check("RISE recursive roles preserved",readRise(document(initial+operation(3,"newRelation",relation("A","A","0toN","1"))),out,error)&&out.model.objects.last().a==out.model.objects.last().b&&out.model.objects.last().roleA!=out.model.objects.last().roleB);
 auto rename=operation(4,"renameEntity","<r:entityName>A</r:entityName><r:newEntityName>C</r:newEntityName>");
 check("RISE entity rename keeps relationship identity references",readRise(document(initial+operation(3,"newRelation",relation("A","B","1","0toN"))+rename),out,error)&&out.model.get(out.model.objects.last().a)->name=="C");
 auto recreate=initial+operation(3,"newRelation",relation("A","B","1","0toN"))+operation(4,"deleteEntity","<r:entityName>A</r:entityName>")+operation(5,"newEntity",entity("A"));
 check("RISE delete and recreate do not resurrect old links",readRise(document(recreate),out,error)&&count(out.model,"relation")==0&&count(out.model,"entity")==2);
 auto isa=[](QString name,QString sub,QString super){return "<r:specialization><r:name>"+name+"</r:name><r:subEntity>"+sub+"</r:subEntity><r:superEntity>"+super+"</r:superEntity></r:specialization>";};
 check("RISE simple ISA imported",readRise(document(initial+operation(3,"newSpecialization",isa("ISA","B","A"))),out,error)&&count(out.model,"isa")==1);
 check("RISE ISA cycles rejected",!readRise(document(initial+operation(3,"newSpecialization",isa("ISA","B","A"))+operation(4,"newSpecialization",isa("Cycle","A","B"))),out,error));
 check("RISE multiple inheritance rejected",!readRise(document(initial+operation(3,"newEntity",entity("C"))+operation(4,"newSpecialization",isa("ISA","B","A"))+operation(5,"newSpecialization",isa("Other","B","C"))),out,error));
 QString snapshot=initial+operation(3,"snapshot",entity("C"))+operation(4,"newEntity",entity("D"));check("RISE snapshot replaces earlier state then applies later operations",readRise(document(snapshot),out,error)&&count(out.model,"entity")==2&&out.model.objects[0].name=="C");
 auto attribute=[](QString name){return "<r:entity><r:name>A</r:name><r:attribute><r:name>"+name+"</r:name></r:attribute></r:entity>";};
 QString attrOps=initial+operation(3,"newAttribute",attribute("old"))+operation(4,"renameAttribute","<r:entityName>A</r:entityName><r:attributeName>old</r:attributeName><r:newAttributeName>new</r:newAttributeName>")+operation(5,"editAttribute",attribute("new"));
 check("RISE attribute creation rename and edit",readRise(document(attrOps),out,error)&&out.model.objects.last().name=="new");
 check("RISE attribute deletion",readRise(document(attrOps+operation(6,"deleteAttribute","<r:entityName>A</r:entityName><r:attributeName>new</r:attributeName>")),out,error)&&count(out.model,"attribute")==0);
 out=sample;auto before=out.model.json();check("RISE malformed XML rejected atomically",!readRise("<bad>",out,error)&&out.model.json()==before);
 check("RISE unknown structural operation rejected",!readRise(document(initial+operation(3,"inventedOperation","")),out,error));
 check("RISE missing endpoints rejected",!readRise(document(initial+operation(3,"newRelation",relation("Missing","B","1","0toN"))),out,error));
 check("RISE unknown cardinality rejected",!readRise(document(initial+operation(3,"newRelation",relation("A","B","2to5","1"))),out,error));
 check("RISE duplicate sequence rejected",!readRise(document(initial+operation(2,"release","")),out,error));
 check("RISE external base rejected",!readRise(document(initial,"<r:base><r:name>Missing</r:name></r:base>"),out,error));
 check("RISE foreign namespace rejected",!readRise(QByteArray(document(initial)).replace("http://www.r2bsoftware/ns/rise/","urn:other"),out,error));
 check("RISE external DTD never resolved",!readRise("<!DOCTYPE r:model SYSTEM \"file:///not-read\">"+document(initial),out,error));
 check("RISE internal entity declarations rejected",!readRise("<!DOCTYPE r:model [<!ENTITY x 'text'>]>"+document(initial),out,error));
 check("RISE unsupported declared format rejected",!readRise("<!DOCTYPE r:model [<!--DTD for RISE XML 9.0 -->]>"+document(initial),out,error));
 check("RISE sample data ignored with explicit report",readRise(document(initial+operation(3,"newInstance",entity("A"))),out,error)&&out.report(true).contains("sample data"));
 auto encoded=document(operation(1,"newEntity",entity(QString::fromUtf8("Città"))));QString unicode=QString::fromUtf8(encoded);QByteArray utf16("\xff\xfe",2);for(QChar c:unicode){utf16.append(char(c.unicode()&255));utf16.append(char(c.unicode()>>8));}check("RISE UTF16 and accented names",readRise(utf16,out,error)&&out.model.objects[0].name==QString::fromUtf8("Città"));
 w.logicalView=false;w.model=Model::example();w.history.clear();w.filePath="untouched.schema";w.en=false;w.buildMenus();w.refresh();auto old=w.model.json();
 QTimer::singleShot(0,&w,[&]{if(auto d=w.findChild<QDialog*>("risePreview"))d->reject();});check("RISE cancel preview preserves current document and path",!w.previewRise(sample)&&w.model.json()==old&&w.filePath=="untouched.schema"&&w.history.isClean());
 bool checked=false;QTimer::singleShot(0,&w,[&]{auto d=w.findChild<QDialog*>("risePreview");if(!d)return;auto tree=d->findChild<QTreeWidget*>("riseKeys");for(int i=0;i<tree->topLevelItemCount();++i){auto e=tree->topLevelItem(i);for(int j=0;j<e->childCount();++j){auto a=e->child(j);if(QStringList{"idNumero","idIscirtto","idNazione","idTipo"}.contains(a->text(0)))a->setCheckState(0,Qt::Checked);}}checked=true;d->grab().save(output+"/rise-preview-it.png");d->findChild<QPushButton*>("applyRiseImport")->click();});
 check("RISE preview imports selected identifiers as a new unsaved UML project",w.previewRise(sample)&&checked&&w.uml&&!w.logicalView&&w.filePath.isEmpty()&&!w.history.isClean()&&w.model.issues().isEmpty());
 check("RISE imported draft saves as native schema",saveModel(output+"/Iscrizioni-importato.schema",w.model,error));
 QStringList logicalErrors;auto logical=generateLogical(w.model,{},logicalErrors);check("RISE confirmed identifiers permit existing logical translation",logicalErrors.isEmpty()&&logical.tables.size()==4);
 qApp->processEvents();bool labelsClear=true;for(auto item:w.connectors)if(auto label=dynamic_cast<QGraphicsSimpleTextItem*>(item))for(const auto &o:w.model.objects)if(o.kind=="entity")labelsClear&=!label->sceneBoundingRect().intersects(w.nodes[o.id]->sceneBoundingRect());check("RISE sample cardinality and role labels remain outside entities",labelsClear);bool labelsAvoidLines=true;for(auto item:w.connectors)if(auto label=dynamic_cast<QGraphicsSimpleTextItem*>(item))for(auto other:w.connectors)if(auto line=dynamic_cast<LinkItem*>(other)){QPainterPath bounds;bounds.addRect(label->sceneBoundingRect());labelsAvoidLines&=!line->path().intersects(bounds);}check("RISE vertical role and cardinality labels do not cross connector lines",labelsAvoidLines);w.grab().save(output+"/rise-imported-it.png");check("RISE imported diagram exports PDF",w.exportPdf(output+"/Iscrizioni-importato.pdf",QPageSize::A3,true,true,error));
 w.history.undo();check("RISE import is undoable",w.model.objects.isEmpty());w.history.redo();check("RISE import redo",w.model.objects.size()==22);
 w.en=true;w.buildMenus();w.refresh();QTimer::singleShot(0,&w,[&]{auto d=w.findChild<QDialog*>("risePreview");if(!d)return;check("RISE import interface is English",d->findChild<QPushButton*>("applyRiseImport")->text()=="Import as new project"&&d->findChild<QTextBrowser*>("riseReport")->toPlainText().contains("Entities:"));d->grab().save(output+"/rise-preview-en.png");d->reject();});w.previewRise(sample);
 // Rejecting the existing-document save prompt must preserve that document too.
 auto prior=w.model.json();w.filePath="existing.schema";QTimer::singleShot(0,&w,[&]{auto d=w.findChild<QDialog*>("risePreview");if(!d)return;QTimer::singleShot(20,&w,[&]{for(auto widget:QApplication::topLevelWidgets())if(auto box=qobject_cast<QMessageBox*>(widget))box->button(QMessageBox::Cancel)->click();});d->findChild<QPushButton*>("applyRiseImport")->click();});check("RISE cancel at save prompt preserves unsaved current project",!w.previewRise(sample)&&w.model.json()==prior&&w.filePath=="existing.schema");
 w.en=false;w.model=Model::example();w.filePath.clear();w.selection.clear();w.selectionIds.clear();w.history.clear();w.buildMenus();w.refresh();
}
