#pragma once
static int guideAssets(Window &w,const QString &output){
 if(!QDir().mkpath(output)){return 2;}
 QString error;int failures=0;
 w.testMode=true;w.en=false;w.mode=1;w.showTypes=true;w.showNotes=false;w.uml=false;w.logicalView=false;w.model={};w.history.clear();w.selection.clear();w.selectionIds.clear();w.applyTheme();w.buildMenus();w.resize(1400,880);w.show();qApp->processEvents();w.resizeDocks({w.outline,w.properties},{205,350},Qt::Horizontal);
 auto settled=[&]{w.refresh();w.fit();qApp->processEvents();};
 auto scene=[&](QString name){auto bounds=w.scene->itemsBoundingRect().adjusted(-25,-25,25,25);QImage image((bounds.size()*1.7).toSize(),QImage::Format_ARGB32_Premultiplied);image.fill(Qt::white);QPainter painter(&image);painter.setRenderHint(QPainter::Antialiasing);w.scene->render(&painter,QRectF(QPointF(0,0),image.size()),bounds);painter.end();if(!image.save(output+"/"+name+".png"))++failures;};
 auto properties=[&](QString name){qApp->processEvents();auto scroll=qobject_cast<QScrollArea*>(w.properties->widget());if(scroll)scroll->verticalScrollBar()->setValue(0);qApp->processEvents();auto height=qMin(w.formWidget->height(),w.form->sizeHint().height()+12);if(!w.formWidget->grab(QRect(0,0,w.formWidget->width(),height)).save(output+"/"+name+".png"))++failures;};
 auto model=ordersExample();model.get("numordine")->dataType="integer";model.get("data")->dataType="date";model.get("numriga")->dataType="integer";model.get("quantita")->dataType="integer";
 for(auto object:model.objects){if(object.kind=="entity")w.model.objects<<object;}
 settled();scene("01-entita");w.grab().save(output+"/01-interfaccia.png");
 w.model=model;w.model.remove("contiene");settled();w.selectOnly("numordine");properties("02-parte-della-chiave");
 w.model=model;settled();w.selectOnly("contiene");properties("03-associazione");scene("03-ruoli");w.selectOnly("numriga");properties("04-chiave-parziale");w.pick("",0);w.selection.clear();w.selectionIds.clear();settled();scene("05-concettuale-Chen");
 if(!saveModel(output+"/Ordini-01-concettuale.schema",w.model,error)||!w.model.issues().isEmpty())++failures;
 w.notationBox->setCurrentIndex(1);settled();scene("06-concettuale-UML");
 bool captured=false;QTimer::singleShot(60,&w,[&]{auto d=w.findChild<QDialog*>("mappingDialog");if(!d)return;d->resize(860,420);qApp->processEvents();captured=d->grab().save(output+"/07-traduzione.png");auto button=d->findChild<QPushButton*>("applyMapping");if(button&&button->isEnabled())button->click();else d->reject();});w.generateAction->trigger();if(!captured||w.model.logical.isEmpty())++failures;settled();scene("08-logico");
 if(!saveModel(output+"/Ordini-02-logico.schema",w.model,error))++failures;
 for(const auto &table:w.logicalCache.tables)if(table.source=="riga"&&!table.foreignKeys.isEmpty()){w.selectOnly(table.foreignKeys.first().columns.first());break;}
 qApp->processEvents();properties("09-proprieta-FK");
 for(auto label:w.formWidget->findChildren<QLabel*>())if(label->text()=="PROVENIENZA DELLA COLONNA"){int y=label->y();w.formWidget->grab(QRect(0,y,w.formWidget->width(),w.form->sizeHint().height()-y+10)).save(output+"/09-provenienza.png");break;}
 auto finalModel=w.model;w.switchLevel(false);w.change("Rinomina",[](Model &m){m.get("data")->name="DataOrdine";});bool compared=false;QTimer::singleShot(60,&w,[&]{auto d=w.findChild<QDialog*>("mappingDialog");if(!d)return;qApp->processEvents();compared=d->grab().save(output+"/10-confronto.png");d->reject();});w.generateDialog();if(!compared)++failures;
 w.model=finalModel;w.history.clear();w.selection.clear();w.selectionIds.clear();w.uml=false;w.logicalView=false;w.showNotes=true;settled();scene("11-nota");
 w.grab().save(output+"/11-interfaccia-note.png");
 {SqlDialog dialog(w.model,false,false,nullptr,&w);dialog.show();qApp->processEvents();if(!dialog.regenerate(false))++failures;qApp->processEvents();if(!dialog.grab().save(output+"/13-sql.png"))++failures;auto tree=dialog.findChild<QTreeWidget*>("physicalTree");if(tree&&tree->topLevelItemCount())tree->setCurrentItem(tree->topLevelItem(0)->child(0));dialog.findChild<QTabWidget*>("sqlTabs")->setCurrentIndex(0);qApp->processEvents();if(!dialog.grab().save(output+"/12-fisico.png"))++failures;dialog.accept();}
 w.history.clear();
 QFile report(output+"/verifica-guida.json");if(!report.open(QIODevice::WriteOnly))return 2;report.write(QJsonDocument(QJsonObject{{"failures",failures},{"version",QCoreApplication::applicationVersion()},{"qt",qVersion()}}).toJson());return failures?1:0;
}
