#pragma once
template<class Check>void exampleDocumentationTests(Window &w,const QString &output,Check check){
 const auto base=QCoreApplication::applicationDirPath()+"/esempi";QDirIterator it(base,{"*.schema"},QDir::Files,QDirIterator::Subdirectories);int count=0;
 w.en=false;w.mode=1;w.showNotes=true;w.selection.clear();w.selectionIds.clear();w.history.clear();w.applyTheme();
 while(it.hasNext()){
  auto path=it.next();auto label=QFileInfo(path).baseName();Model model;QString error;if(!loadModel(path,model,error)){check("Annotated example loads: "+label,false);continue;}++count;
  check("Example has a descriptive, persisted note: "+label,model.notes.size()==1&&model.notes[0].text.size()>400&&model.json()["version"]==5);
  auto plain=model;plain.notes.clear();check("Example note does not change conceptual meaning: "+label,conceptualDigest(model)==conceptualDigest(plain)&&model.issues()==plain.issues());
  for(int level=0;level<3;++level){
   w.model=model;w.uml=level==1;w.logicalView=level==2;
   if(w.logicalView&&model.logical.isEmpty()){QStringList errors;QMap<QString,QString> choices;for(const auto &o:model.objects)if(o.kind=="relation"&&o.cardA.endsWith("1")&&o.cardB.endsWith("1"))choices[o.id]="B";w.model.logical=generateLogical(w.model,choices,errors).json();if(!errors.isEmpty()){check("Annotated example translates: "+label,false);continue;}}
   w.refresh();auto node=w.nodes.value(model.notes[0].id);bool separated=node;
   if(node)for(auto other:w.nodes)if(other!=node&&other->kind!="note")separated&=!node->sceneBoundingRect().intersects(other->sceneBoundingRect());
   check(QString("Note stays outside figures (%1): ").arg(level)+label,separated);
  }
  QFont font("Segoe UI");font.setPixelSize(14);const auto &note=model.notes[0];auto textBounds=QFontMetricsF(font).boundingRect(QRectF(0,0,note.size.width()-24,10000),Qt::TextWordWrap,note.text);check("Example note text fits without clipping: "+label,textBounds.height()<=note.size.height()-22);
 }
 check("All 16 shipped examples checked",count==16);
 for(auto example:{ordersExample(),shopExample(),purchaseExample()})check("Embedded menu/startup examples carry a note",example.notes.size()==1&&!example.notes[0].text.isEmpty());
 // Editing even the largest allowed note does not affect generated SQL.
 auto model=purchaseExample();auto config=model.physical;auto baseline=generateSql(model,config);model.notes[0].text=QString(10000,'x');auto result=generateSql(model,config);check("Large notes are outside the SQL generation path",result.errors.isEmpty()&&result.text==baseline.text);
 w.model=ordersExample();w.logicalView=false;w.uml=false;w.buildMenus();w.refresh();w.fit();qApp->processEvents();w.grab().save(output+"/ordini-nota.png");
 w.model=Model::example();w.history.clear();w.refresh();
}
