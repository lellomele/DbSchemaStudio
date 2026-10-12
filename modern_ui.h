// Included inside Window. Native window frame, readable controls and model search.
 void filterOutline(){
  if(!outlineSearch)return;
  const auto query=outlineSearch->text().trimmed();
  for(int i=0;i<tree->topLevelItemCount();++i){auto parent=tree->topLevelItem(i);bool match=parent->text(0).contains(query,Qt::CaseInsensitive),childMatch=false;for(int j=0;j<parent->childCount();++j){auto child=parent->child(j);bool visible=match||child->text(0).contains(query,Qt::CaseInsensitive);child->setHidden(!visible);childMatch|=visible;}parent->setHidden(!match&&!childMatch);if(!query.isEmpty()&&childMatch)parent->setExpanded(true);}
 }
 void updateModernUi(){
  if(zoomBox){QSignalBlocker blocker(zoomBox);zoomBox->setValue(qRound(view->transform().m11()*100));}
  if(!contextHint)return;
  QString hint;
  if(logicalView)hint=t("LOGICO   ·   Seleziona tabelle e colonne per modificarne il nome. «Mostra origine» torna al concettuale.","LOGICAL   ·   Select tables and columns to edit their names. ‘Show source’ returns to conceptual modeling.");
  else if(model.objects.isEmpty())hint=t("INIZIA QUI   ·   Premi Ins o «Entità», poi aggiungi gli attributi. Ctrl+rotella ingrandisce il diagramma.","START HERE   ·   Press Insert or ‘Entity’, then add attributes. Ctrl+wheel zooms the diagram.");
  else if(selectionIds.size()>1)hint=t("SELEZIONE MULTIPLA   ·   Trascina per spostare il gruppo. Tasto destro per allineare. Ctrl+clic modifica la selezione.","MULTIPLE SELECTION   ·   Drag to move the group. Right-click to align. Ctrl+click changes selection.");
  else {auto object=model.get(selection);hint=object&&object->kind=="relation"?t("ASSOCIAZIONE   ·   A e B identificano i ruoli. Imposta le cardinalità nelle proprietà, poi premi «Applica modifiche».","RELATIONSHIP   ·   A and B identify roles. Set multiplicities in Properties, then click ‘Apply changes’."):t("CONCETTUALE   ·   Ins aggiunge un attributo all’entità selezionata. F2 rinomina. Tasto destro per le azioni. F7 verifica.","CONCEPTUAL   ·   Insert adds an attribute to the selected entity. F2 renames. Right-click for actions. F7 validates.");}
  contextHint->setText(hint);contextHint->setStyleSheet(QString("background:%1;color:%2;border-bottom:1px solid %3;").arg(theme.surface.name(),theme.muted.name(),theme.line.name()));
 }
 QIcon modelIcon(int kind){
  QIcon icon;for(int scale:{1,2,3}){QPixmap image(24*scale,24*scale);image.setDevicePixelRatio(scale);image.fill(Qt::transparent);QPainter painter(&image);painter.setRenderHint(QPainter::Antialiasing);painter.setPen(QPen(theme.accent,1.8));painter.setBrush(Qt::NoBrush);if(kind==0)painter.drawRoundedRect(QRectF(3,5,18,14),2,2);else if(kind==1)painter.drawEllipse(QRectF(3,6,18,12));else if(kind==2){QPolygonF polygon;polygon<<QPointF(12,3)<<QPointF(22,12)<<QPointF(12,21)<<QPointF(2,12);painter.drawPolygon(polygon);}else{QPolygonF polygon;polygon<<QPointF(12,3)<<QPointF(22,20)<<QPointF(2,20);painter.drawPolygon(polygon);}painter.end();icon.addPixmap(image);}return icon;
 }
 void decorateActions(QAction *create,QAction *open,QAction *save,QAction *undo,QAction *redo,QAction *entity,QAction *attribute,QAction *relation,QAction *isa,QAction *remove,QAction *validate,QAction *pdf){
  const QList<QPair<QAction*,QStyle::StandardPixmap>> standard={{create,QStyle::SP_FileIcon},{open,QStyle::SP_DialogOpenButton},{save,QStyle::SP_DialogSaveButton},{undo,QStyle::SP_ArrowBack},{redo,QStyle::SP_ArrowForward},{remove,QStyle::SP_TrashIcon},{validate,QStyle::SP_DialogApplyButton},{pdf,QStyle::SP_FileDialogDetailedView}};
  for(const auto &entry:standard)entry.first->setIcon(style()->standardIcon(entry.second));
  entity->setIcon(modelIcon(0));attribute->setIcon(modelIcon(1));relation->setIcon(modelIcon(2));isa->setIcon(modelIcon(3));
  for(auto toolbar:findChildren<QToolBar*>(QString(),Qt::FindDirectChildrenOnly)){toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);toolbar->setIconSize(QSize(20,20));for(auto action:{create,open,save,undo,redo,remove,pdf})if(auto button=qobject_cast<QToolButton*>(toolbar->widgetForAction(action)))button->setToolButtonStyle(Qt::ToolButtonIconOnly);}
 }
