#pragma once
#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif

// Regression cases from the beta audit. Every file/profile belongs to this run.
static void auditTests(Window &w,const QString &output,std::function<void(QString,bool)> check){
 auto reset=[&]{w.testMode=true;w.logicalView=false;w.uml=false;w.en=false;w.history.clear();w.model=Model::example();w.selection.clear();w.selectionIds.clear();w.buildMenus();w.refresh();};reset();
 QString error;Model kept=Model::example();const auto original=kept.json();
 auto rejected=[&](QJsonObject data){return !Model::parse(QJsonDocument(data).toJson(),kept,error)&&kept.json()==original;};
 auto data=original;data["version"]=1.5;check("Fractional project versions are rejected atomically",rejected(data));
 QStringList errors;auto logical=generateLogical(kept,{},errors);
 auto invalidLogical=logical.json();invalidLogical["version"]=1.5;LogicalSchema rejectedLogical;check("Fractional logical versions are rejected",!LogicalSchema::parse(invalidLogical,rejectedLogical,error));
 data=original;data["logical"]=logical.json();check("Older format cannot silently discard a logical model",rejected(data));
 for(QString field:{"owner","a","b","key","identifying"}){data=original;auto objects=data["objects"].toArray();auto entity=objects[0].toObject();entity[field]=(field=="key"||field=="identifying")?QJsonValue(true):QJsonValue("numordine");objects[0]=entity;data["objects"]=objects;check("Invalid entity field rejected: "+field,rejected(data));}
 w.selectOnly("contiene");int boundedRoles=0;for(auto field:w.formWidget->findChildren<QLineEdit*>())if(field->accessibleName().contains("ruolo"))boundedRoles+=field->maxLength()==100;check("Role editors agree with persisted length limits",boundedRoles==2);
 auto before=w.model.json();const auto index=w.history.index();bool accepted=w.change("invalid",[](Model &m){m.get("data")->owner="missing";});check("Failed model mutation reports failure without changing history",!accepted&&w.model.json()==before&&w.history.index()==index);

 // A pending native item release must never restore the previous document.
 w.selectOnly("ordine");auto node=w.nodes["ordine"];node->begin(node);node->setPos(node->pos()+QPointF(10,10));node->end(node);w.installExample(purchaseExample(),true);const auto replacement=w.model.json();qApp->processEvents();check("Deferred item release cannot overwrite a replacement document",w.model.json()==replacement&&w.history.isClean());
 reset();node=w.nodes["ordine"];node->begin(node);node->setPos({200001,0});node->end(node);qApp->processEvents();check("Native item drag enforces the same position bounds as group drag",w.model.json()==original&&w.history.isClean());
 reset();w.selectOnly("ordine");QPointer<QPushButton> add;for(auto b:w.formWidget->findChildren<QPushButton*>())if(b->text()=="Aggiungi attributo")add=b;
 if(add){add->click();}check("Clicked property button survives signal delivery",add&&w.model.objects.size()==8);QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);check("Replaced property widgets are released after signal delivery",add.isNull());
 QPointer<QAction> example=w.findChild<QAction*>("examplePurchase");w.history.clear();if(example)example->trigger();check("Triggered menu survives synchronous menu replacement",example&&w.model.json()==purchaseExample().json());QCoreApplication::sendPostedEvents(nullptr,QEvent::DeferredDelete);check("Retired menus do not accumulate after rebuilding",example.isNull());

 QTemporaryDir temporary(output+"/audit-storage-XXXXXX");check("Audit storage is isolated",temporary.isValid());
 const auto savedRecovery=w.recoveryPath;w.recoveryPath=temporary.path()+"/recovery.schema";
 reset();w.change("edit",[](Model &m){m.get("ordine")->name="Da scartare";});w.testMode=false;bool recovery=w.writeRecovery();w.testMode=true;
 auto next=Model::example();next.get("ordine")->name="Altro progetto";const auto path=temporary.path()+"/altro.schema";saveModel(path,next,error);bool opened=w.openPath(path);
 check("Successful document replacement clears only owned recovery",recovery&&opened&&!QFile::exists(w.recoveryPath)&&!QFile::exists(w.recoveryPath+".previous")&&w.model.json()==next.json());
 saveModel(w.recoveryPath,kept,error);w.ownsRecovery=false;w.installExample(purchaseExample(),true);check("Replacing a document preserves an unclaimed recovery",QFile::exists(w.recoveryPath));
 QFile::remove(w.recoveryPath);reset();w.change("edit",[](Model &m){m.get("ordine")->name="Da conservare";});w.testMode=false;w.writeRecovery();w.testMode=true;const auto dirty=w.model.json();
 QTimer::singleShot(0,&w,[&]{for(auto widget:qApp->topLevelWidgets())if(auto box=qobject_cast<QMessageBox*>(widget))box->accept();});bool failed=w.openPath(temporary.path()+"/missing.schema");check("Failed open retains document history and recovery",!failed&&w.model.json()==dirty&&!w.history.isClean()&&QFile::exists(w.recoveryPath));
 QTimer::singleShot(0,&w,[&]{for(auto widget:qApp->topLevelWidgets())if(auto box=qobject_cast<QMessageBox*>(widget))if(auto b=box->button(QMessageBox::Cancel))b->click();});check("Cancel before replacing a dirty document preserves all work",!w.maybeSave()&&w.model.json()==dirty&&QFile::exists(w.recoveryPath));
 w.clearRecovery();w.recoveryPath=savedRecovery;reset();
#ifdef Q_OS_WIN
 const auto lockedPath=temporary.path()+"/locked.schema";saveModel(lockedPath,kept,error);
 HANDLE handle=CreateFileW(reinterpret_cast<LPCWSTR>(lockedPath.utf16()),GENERIC_READ,FILE_SHARE_READ,nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
 bool lockedFailure=handle!=INVALID_HANDLE_VALUE&&!saveWithBackup(lockedPath,next,error);
 if(handle!=INVALID_HANDLE_VALUE)CloseHandle(handle);
 Model unchanged;check("Windows locked project is never replaced on failed save",lockedFailure&&loadModel(lockedPath,unchanged,error)&&unchanged.json()==kept.json());
#endif
 w.model.get("ordine")->name=QString(200,'L');w.refresh();w.selectOnly("ordine");check("Long imported names are not truncated by the inspector",w.formWidget->findChild<QLineEdit*>()->text()==QString(200,'L'));reset();

 // SQLite-like text is not assumed: exercise date/time defaults for our engines.
 auto dated=Model::example();dated.get("numordine")->dataType="integer";dated.get("numriga")->dataType="integer";dated.get("data")->dataType="datetime";dated.get("quantita")->dataType="integer";
 auto schema=generateLogical(dated,{},errors);dated.logical=schema.json();QString column;for(const auto &t:schema.tables)for(const auto &c:t.columns)if(c.source=="data")column=c.id;
 for(const auto &profile:physicalProfiles()){auto config=physicalSwitchProfile(schema,{},profile);auto columns=config["columns"].toObject(),spec=columns[column].toObject();spec["defaultMode"]="literal";spec["defaultValue"]="2026-03-29 02:30:00";columns[column]=spec;config["columns"]=columns;check("SQL civil date/time is independent of local DST: "+profile,generateSql(dated,config,true).errors.isEmpty());}
 auto physical=physicalDefaults(schema);auto renamed=schema;renamed.tables[0].name="NuovoNome";auto tracked=physicalDefaults(renamed,physical);check("Physical table name follows logical rename",tracked["tables"].toObject()[schema.tables[0].id]=="NuovoNome");auto names=physical["tables"].toObject();names[schema.tables[0].id]="CustomSql";physical["tables"]=names;check("Physical table override survives logical rename",physicalDefaults(renamed,physical)["tables"].toObject()[schema.tables[0].id]=="CustomSql");
 {auto longModel=dated;auto longSchema=schema;longSchema.tables[0].name=QString(200,'L');longModel.logical=longSchema.json();SqlDialog dialog(longModel,false,false,nullptr,&w);check("Physical names stay editable without silent truncation",dialog.findChild<QLineEdit*>("physicalName")->text()==QString(200,'L'));check("Overlong SQL names report a generation error",!dialog.regenerate(false));}
 {SqlEditor editor;editor.resize(700,400);editor.show();qApp->processEvents();editor.setPlainText(QString("SELECT 1;\n").repeated(1000));auto gutter=editor.findChild<QWidget*>("sqlLineNumbers");check("Line-number gutter grows when line count gains digits",gutter&&gutter->width()==editor.numberWidth());}
 {WrappedLabel label(QString(200,'W'));label.setFont(QFont("Segoe UI",20));label.resize(180,label.heightForWidth(180));check("Inspector wraps long unbroken identifiers without altering text",label.minimumSizeHint().width()==0&&label.heightForWidth(180)>label.heightForWidth(360)&&label.text()==QString(200,'W'));}
 reset();
}
