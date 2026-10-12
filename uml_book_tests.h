#pragma once
static void umlBookTests(Window &w,const QString &output,std::function<void(QString,bool)> check){
 w.logicalView=false;w.uml=true;w.en=false;w.mode=1;w.model=Model::example();w.model.get("contiene")->identifying=false;w.selection.clear();w.selectionIds.clear();w.history.clear();w.applyTheme();
 bool matrix=true;for(QString a:{"0,1","1,1","0,N","1,N"})for(QString b:{"0,1","1,1","0,N","1,N"}){w.model.get("contiene")->cardA=a;w.model.get("contiene")->cardB=b;auto original=w.model.json();w.refresh();int count=0;for(auto item:w.connectors)if(item->data(1)=="umlCrowFoot"){++count;matrix&=(item->data(2)=="A"?b:a)==item->data(3).toString();matrix&=item->data(0)=="contiene";}matrix&=count==int(a.endsWith('N'))+int(b.endsWith('N'))&&original==w.model.json();}
 check("Book-style UML crow feet preserve all multiplicities at opposite endpoints",matrix);
 bool participation=true;
 for(bool uml:{false,true})for(bool recursive:{false,true})for(QString a:{"0,1","1,1","0,N","1,N"})for(QString b:{"0,1","1,1","0,N","1,N"}){
  w.uml=uml;auto rel=w.model.get("contiene");rel->a="ordine";rel->b=recursive?"ordine":"riga";rel->cardA=a;rel->cardB=b;auto before=w.model.json();w.refresh();int count=0;
  for(auto item:w.connectors)if(item->data(1)=="participation"){++count;QString card=item->data(2)=="A"?a:b;auto path=dynamic_cast<QGraphicsPathItem*>(item);participation&=path&&path->pen().style()==(card.startsWith("0")?Qt::DashLine:Qt::SolidLine)&&item->data(3)==card;}
  participation&=count==2&&before==w.model.json();
 }
 check("Participation line style follows local role in both notations and recursive associations",participation);w.uml=true;
 check("Book-style UML notation retains optionality and uses N",w.cardinal("0,N")=="0..N"&&w.cardinal("1,N")=="1..N"&&w.cardinal("1,1")=="1"&&w.cardinal("0,1")=="0..1");
 w.model=Model::example();w.refresh();check("Book-style UML distinguishes global and local identifiers",w.nodes["numordine"]->label.contains("{ID}")&&w.nodes["numriga"]->label.contains("{ID locale}"));
 for(bool dark:{false,true})for(int preset=0;preset<4;++preset){auto theme=Theme::make(dark,preset);check("Book-style UML title contrast",Theme::contrast(theme.accent,Theme::textOn(theme.accent))>=4.5);}
 Model book;auto entity=[&](QString id,QString name,QPointF pos){Object o;o.id=id;o.kind="entity";o.name=name;o.chen=o.uml=pos;book.objects<<o;};auto attr=[&](QString id,QString name,QString owner,bool key){Object o;o.id=id;o.kind="attribute";o.name=name;o.owner=owner;o.key=key;book.objects<<o;};
 entity("fornitore","Fornitore",{170,200});entity("prodotto","Prodotto",{740,200});attr("codfornitore","CodFornitore","fornitore",true);attr("nominativo","Nominativo","fornitore",false);attr("via","Via","fornitore",false);attr("citta","Citta","fornitore",false);attr("cap","CAP","fornitore",false);attr("codprodotto","CodProdotto","prodotto",true);attr("descrizione","Descrizione","prodotto",false);attr("prezzo","Prezzo","prodotto",false);Object rel;rel.id="fornire";rel.kind="relation";rel.name="Fornire";rel.a="fornitore";rel.b="prodotto";rel.cardA="0,N";rel.cardB="1,1";rel.chen=rel.uml={450,90};book.objects<<rel;book.get("prezzo")->dataType="money";
 book.get("fornitore")->chen={150,300};book.get("prodotto")->chen={850,300};book.get("fornire")->chen={500,300};
 const QVector<QPointF> positions={{-90,110},{150,100},{380,100},{-80,490},{160,500},{670,100},{900,100},{1120,260}};int k=0;for(auto &o:book.objects)if(o.kind=="attribute")o.chen=positions[k++];
 w.model=book;w.refresh();
 for(auto id:{"fornitore","prodotto"}){w.model.get(id)->uml.setY(90+w.nodes[id]->size.height()/2-21);}
 w.refresh();QString error;check("Book-style sample saved",saveModel(output+"/Fornitori-Prodotti-UML.schema",w.model,error));
 auto render=[&](QString filename){auto rect=w.scene->itemsBoundingRect().adjusted(-30,-30,30,30);QImage image((rect.size()*1.7).toSize(),QImage::Format_ARGB32_Premultiplied);image.fill(w.theme.paper);QPainter painter(&image);painter.setRenderHint(QPainter::Antialiasing);w.scene->render(&painter,QRectF(QPointF(0,0),image.size()),rect);painter.end();return image.save(output+"/"+filename+".png");};
 check("Book-style sample rendered",render("UML-libro"));check("Book-style vector PDF",w.exportPdf(output+"/UML-libro.pdf",QPageSize::A4,true,false,error));w.mode=2;w.applyTheme();w.refresh();check("Book-style night rendered",render("UML-libro-notte"));
 w.model.get("fornire")->b="fornitore";w.model.get("fornire")->cardB="0,N";w.refresh();int feet=0;for(auto item:w.connectors)feet+=item->data(1)=="umlCrowFoot";check("Book-style recursive relationship keeps both role ends",feet==2);check("Book-style recursive example rendered",render("UML-libro-ricorsiva"));
 w.model=Model::example();w.uml=true;w.mode=1;w.applyTheme();w.buildMenus();w.refresh();auto original=w.model.json();int undoCount=w.history.count();
 auto pkAction=w.findChild<QAction*>("umlIdentifierPK");auto idAction=w.findChild<QAction*>("umlIdentifierID");
 check("Identifier menu defaults to ID",pkAction&&idAction&&idAction->isChecked()&&!pkAction->isChecked());
 if(pkAction)pkAction->trigger();
 check("PK preference changes only the UML presentation",w.umlPkLabels&&w.nodes["numordine"]->badgeText=="[PK]"&&w.nodes["numriga"]->badgeText=="[PK locale]"&&w.model.json()==original&&w.history.count()==undoCount);
 check("PK and ID choices are exclusive",pkAction&&idAction&&pkAction->isChecked()&&!idAction->isChecked());
 check("PK badge rendered",render("UML-PK-giorno"));
 for(bool dark:{false,true})for(int palette=0;palette<4;++palette){auto row=w.nodes["numordine"];row->theme=Theme::make(dark,palette);check("Identifier badge contrast in every palette",Theme::contrast(row->badgeForeground(),row->badgeBackground())>=4.5);}
 w.mode=2;w.applyTheme();w.refresh();check("PK badge night rendered",render("UML-PK-notte"));
 check("PK badge PDF export",w.exportPdf(output+"/UML-PK.pdf",QPageSize::A4,true,false,error));
 w.uml=false;w.refresh();check("PK display preference does not affect Chen identifiers",w.attributeKeySuffix(*w.model.get("numordine"))==" {ID}"&&w.nodes["numordine"]->badgeText.isEmpty());
 w.uml=true;Object sub;sub.id="sub-order";sub.kind="entity";sub.name="OrdineSpeciale";sub.uml={1200,200};w.model.objects<<sub;
 Object isa;isa.id="isa-order";isa.kind="isa";isa.a=sub.id;isa.b="ordine";w.model.objects<<isa;
 Object alt;alt.id="alternate-code";alt.kind="attribute";alt.owner=sub.id;alt.name="CodiceAlternativo";alt.key=true;w.model.objects<<alt;
 for(bool english:{false,true}){w.en=english;w.refresh();check("Alternate and weak identifiers remain distinct in both languages",w.nodes[alt.id]->badgeText==(english?"{alternate}":"{alternativa}")&&w.nodes["numriga"]->badgeText==(english?"[local PK]":"[PK locale]"));bool inherited=false;for(auto item:w.scene->items())if(auto row=dynamic_cast<Node*>(item))if(row->inherited&&row->badgeText=="[PK]")inherited=true;check("Inherited primary identifier keeps the PK label",inherited);}
 w.model.get("numordine")->name=QString(100,'W');w.refresh();bool fits=true;for(auto item:w.scene->items())if(auto row=dynamic_cast<Node*>(item))if(!row->badgeText.isEmpty())fits&=Node::attributeWidth(row->badgePrefix,row->badgeText,row->badgeTail,row->inherited)<=row->size.width()-20;check("Long identifiers and inherited origins fit without clipping",fits);
 w.model=Model::example();QStringList logicalErrors;w.model.logical=generateLogical(w.model,{},logicalErrors).json();w.logicalView=true;w.refresh();bool logicalBadges=false;for(auto item:w.scene->items())if(auto row=dynamic_cast<Node*>(item))logicalBadges|=!row->badgeText.isEmpty();check("Conceptual badge option leaves logical table rendering unchanged",logicalErrors.isEmpty()&&!logicalBadges);
 w.logicalView=false;w.en=false;w.buildMenus();check("Identifier preference survives menu rebuild",w.findChild<QAction*>("umlIdentifierPK")->isChecked());w.findChild<QAction*>("umlIdentifierID")->trigger();check("ID style can be restored",!w.umlPkLabels&&w.nodes["numordine"]->badgeText=="{ID}");
 w.model=Model::example();w.uml=false;w.en=false;w.mode=1;w.applyTheme();w.buildMenus();w.refresh();
}
