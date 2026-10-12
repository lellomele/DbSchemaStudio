// Included within Window. Recovery is independent from the user's project file.
 bool writeRecovery(QString *failure=nullptr){
  if(testMode||history.isClean())return true;
  QString error;
  // Preserve a recovery declined at startup before this session creates its own.
  if(!ownsRecovery)for(const auto &source:{recoveryPath,recoveryPath+".previous"})if(QFile::exists(source)){
   const auto archive=source+".retained-"+QDateTime::currentDateTimeUtc().toString("yyyyMMdd-HHmmsszzz");
   if(!atomicCopy(source,archive,error)){if(failure)*failure=error;status->setText(t("Copia di recupero non aggiornata: ","Recovery copy not updated: ")+localizedError(error,en));return false;}
  }
  Model previous;QString ignored;
  // Never replace a valid fallback with a corrupt primary recovery file.
  if(QFile::exists(recoveryPath)&&loadModel(recoveryPath,previous,ignored)&&!atomicCopy(recoveryPath,recoveryPath+".previous",error)){if(failure)*failure=error;status->setText(t("Recupero automatico non riuscito: ","Automatic recovery failed: ")+localizedError(error,en));return false;}
  if(!saveModel(recoveryPath,model,error)){if(failure)*failure=error;status->setText(t("Recupero automatico non riuscito: ","Automatic recovery failed: ")+localizedError(error,en));return false;}
  ownsRecovery=true;return true;
 }
 void clearRecovery(){
  if(recoveryTimer)recoveryTimer->stop();
  if(!ownsRecovery)return;
  QFile::remove(recoveryPath);QFile::remove(recoveryPath+".previous");ownsRecovery=false;
 }
 void recover(){
  if(!QFile::exists(recoveryPath)&&!QFile::exists(recoveryPath+".previous"))return;
  Model candidate;QString error;bool fallback=false;
  if(!loadModel(recoveryPath,candidate,error)){
   if(!loadModel(recoveryPath+".previous",candidate,error)){QMessageBox::warning(this,t("Recupero non disponibile","Recovery unavailable"),t("Le copie di recupero non sono leggibili. Sono state conservate per la verifica in:\n","Recovery copies cannot be read. They have been retained for inspection in:\n")+QFileInfo(recoveryPath).absolutePath());return;}fallback=true;
  }
  const auto prompt=fallback?t("La copia più recente non è leggibile. Aprire la copia precedente valida?\nSe scegli No, i file saranno conservati.","The latest recovery is unreadable. Open the previous valid copy?\nIf you choose No, the files will be retained."):t("È disponibile un progetto non salvato dalla sessione precedente. Recuperarlo?\nSe scegli No, la copia sarà conservata.","An unsaved project from the previous session is available. Recover it?\nIf you choose No, the copy will be retained.");
  if(QMessageBox::question(this,t("Recupero progetto","Recover project"),prompt,QMessageBox::Yes|QMessageBox::No,QMessageBox::Yes)!=QMessageBox::Yes)return;
  ++documentEpoch;model={};filePath.clear();selection.clear();selectionIds.clear();history.clear();ownsRecovery=true;change(t("Recupera progetto","Recover project"),[=](Model &target){target=candidate;});fit();
 }
