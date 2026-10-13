#pragma once

// Launched only with an explicit isolated --data-dir. The driver terminates the
// seed process after the marker, then starts a fresh process for recovery.
static int sessionProbe(Window &w,const QString &phase){
 if(storageOverride.isEmpty()||!QFileInfo(storageOverride).fileName().startsWith("DbSchemaStudio-session-"))return 2;
 QJsonArray checks;int failures=0;auto check=[&](QString name,bool ok){checks<<QJsonObject{{"test",name},{"passed",ok}};if(!ok)++failures;};
 auto report=[&](QString name){QSaveFile f(storageDirectory()+"/"+name+".json");if(!f.open(QIODevice::WriteOnly))return false;f.write(QJsonDocument(QJsonObject{{"checks",checks},{"failures",failures},{"version",QCoreApplication::applicationVersion()}}).toJson());return f.commit();};
 if(phase=="seed"){
  w.installExample(ordersExample(),true);w.filePath=storageDirectory()+"/Progetti/saved.schema";QString error;
  check("Initial document saved",w.save(false));
  w.change("Session edit",[](Model &m){m.get("ordine")->name="Ordine recuperato — 名";});
  w.en=true;w.mode=2;w.fontSize=12;w.view->gridVisible=true;w.view->snapEnabled=true;w.umlPkLabels=true;w.storePreferences();
  check("Dirty recovery persisted",w.writeRecovery(&error));check("Preferences persisted",w.prefs.status()==QSettings::NoError);
  if(!report("seed")||failures)return 1;
  return qApp->exec();
 }
 if(phase=="recover"){
  check("Language and appearance restored",w.en&&w.mode==2&&w.fontSize==12&&w.umlPkLabels);
  check("Grid preferences restored",w.view->gridVisible&&w.view->snapEnabled);
  QTimer::singleShot(0,&w,[&]{for(auto widget:qApp->topLevelWidgets())if(auto box=qobject_cast<QMessageBox*>(widget))if(auto b=box->button(QMessageBox::Yes))b->click();});w.recover();
  check("Recovery contains the latest unsaved document",w.model.get("ordine")&&w.model.get("ordine")->name=="Ordine recuperato — 名"&&!w.history.isClean()&&w.filePath.isEmpty());
  Model saved;QString error;check("Crash does not overwrite the saved file",loadModel(storageDirectory()+"/Progetti/saved.schema",saved,error)&&saved.get("ordine")&&saved.get("ordine")->name=="Ordine");
  w.filePath=storageDirectory()+"/Progetti/recovered.schema";check("Recovered project saves successfully",w.save(false));
  check("Saving clears the claimed recovery",!QFile::exists(w.recoveryPath)&&w.history.isClean());
  return report("recover")&&!failures?0:1;
 }
 return 2;
}
