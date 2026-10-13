#pragma once

static int benchmarkTests(Window &w,const QString &output){
 QDir().mkpath(output);QJsonArray records;int failures=0;QString error;
 QVector<QPair<QString,Model>> cases{{"orders",ordersExample()}};Model limit;
 const auto fixture=QCoreApplication::applicationDirPath()+"/test-fixtures/01-Test-limite-Azienda-concettuale.schema";
 if(loadModel(fixture,limit,error))cases<<qMakePair(QString("test-limit"),limit);else return 2;
 Model large;
 for(int i=0;i<150;++i){Object e;e.kind="entity";e.id=QString("entity%1").arg(i);e.name=e.id;e.chen=e.uml={double(i%15)*450,double(i/15)*450};large.objects<<e;
  for(int n=0;n<4;++n){Object a;a.kind="attribute";a.id=e.id+QString("-attr%1").arg(n);a.name=QString("Campo%1").arg(n);a.owner=e.id;a.dataType=n==1?"string":"integer";a.key=n==0;a.chen=a.uml=e.chen+QPointF((n-2)*120,-140);large.objects<<a;}
  if(i){Object r;r.kind="relation";r.id=QString("rel%1").arg(i);r.name=r.id;r.a=QString("entity%1").arg(i-1);r.b=e.id;r.chen=r.uml=e.chen-QPointF(200,0);large.objects<<r;}
 }
 cases<<qMakePair(QString("150-entities"),large);
 for(const auto &entry:cases)for(int run=0;run<3;++run){
  w.history.clear();w.model=entry.second;w.logicalView=false;w.uml=false;w.selection.clear();w.selectionIds.clear();QElapsedTimer timer;QJsonObject ms;timer.start();
  Model parsed;bool ok=Model::parse(QJsonDocument(w.model.json()).toJson(),parsed,error);ms["roundtrip"]=timer.nsecsElapsed()/1e6;
  timer.restart();w.refresh();w.fit();QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);ms["chenRefresh"]=timer.nsecsElapsed()/1e6;
  timer.restart();w.uml=true;w.refresh();w.fit();QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);ms["umlRefresh"]=timer.nsecsElapsed()/1e6;
  QString selected;for(auto o:w.model.objects)if(o.kind=="entity"){selected=o.id;break;}
  timer.restart();for(int i=0;i<20;++i)w.selectOnly(selected);ms["selectionMean"]=timer.nsecsElapsed()/20e6;
  timer.restart();w.change("Benchmark",[&](Model&m){m.get(selected)->name+=" edit";});ms["edit"]=timer.nsecsElapsed()/1e6;w.history.undo();
  QStringList errors;timer.restart();auto schema=generateLogical(w.model,{},errors,true);ms["mapping"]=timer.nsecsElapsed()/1e6;ok&=errors.isEmpty();
  w.model.logical=schema.json();timer.restart();w.logicalView=true;w.refresh();w.fit();QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);ms["logicalRefresh"]=timer.nsecsElapsed()/1e6;
  for(auto profile:physicalProfiles()){auto settings=physicalSwitchProfile(schema,{},profile);auto columns=settings["columns"].toObject();for(auto it=columns.begin();it!=columns.end();++it){auto spec=it.value().toObject();if(spec["type"].toString().isEmpty())spec["type"]=spec["sourceType"]=="real"?"DECIMAL":"INTEGER";it.value()=spec;}settings["columns"]=columns;timer.restart();auto sql=generateSql(w.model,settings,true);ms["sql-"+profile]=timer.nsecsElapsed()/1e6;ok&=sql.errors.isEmpty();}
  if(!ok)++failures;
  records<<QJsonObject{{"model",entry.first},{"run",run},{"objects",int(entry.second.objects.size())},{"passed",ok},{"milliseconds",ms}};
 }
 QFile file(output+"/benchmarks.json");if(!file.open(QIODevice::WriteOnly))return 2;file.write(QJsonDocument(QJsonObject{{"failures",failures},{"qt",qVersion()},{"runs",records}}).toJson());return failures?1:0;
}
