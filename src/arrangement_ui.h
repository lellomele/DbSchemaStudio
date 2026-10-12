// Included within Window. Layout edits never change conceptual semantics.
 QString arrangementAnchor()const{auto ids=movableSelection();return ids.contains(selection)?selection:ids.value(0);}
 QPointF gridOffset(QString id)const{auto n=nodes.value(id);return n&&(n->kind=="entity"||n->kind=="note")?-QPointF(n->size.width()/2,n->size.height()/2):QPointF();}
 QPointF snappedDrag(QPointF delta)const{
  if(!view->snapEnabled||view->dragAlt)return delta;
  auto id=arrangementAnchor();if(id.isEmpty())return delta;
  QPointF base;
  if(auto note=dragBefore.note(id))base=notePosition(*note);
  else if(logicalView){LogicalSchema schema;QString error;if(!LogicalSchema::parse(dragBefore.logical,schema,error)||!schema.table(id))return delta;base=schema.table(id)->position;}
  else{auto o=dragBefore.get(id);if(!o)return delta;base=uml?o->uml:o->chen;}
  auto anchor=base+gridOffset(id);return View::nearestGrid(anchor+delta)-anchor;
 }
 void applyLayout(const QMap<QString,QPointF> &deltas,const QString &label){
  if(deltas.isEmpty())return;
  const bool logical=logicalView,notation=uml;
  change(label,[=](Model &m){
   for(auto &n:m.notes)(logical?n.logical:notation?n.uml:n.chen)+=deltas.value(n.id);
   if(logical){LogicalSchema schema;QString error;if(!LogicalSchema::parse(m.logical,schema,error))return;for(auto &table:schema.tables)table.position+=deltas.value(table.id);m.logical=schema.json();}
   else for(auto &o:m.objects){auto delta=deltas.value(o.id);if(o.kind=="attribute"&&!notation&&deltas.contains(o.owner))delta=deltas.value(o.owner);(notation?o.uml:o.chen)+=delta;}
  });
 }
 // 0=row, 1=column, 2=left edges, 3=top edges, 4/5=equal free gaps, 6=grid.
 void arrangeSelection(int operation){
  auto ids=movableSelection();if(ids.size()<(operation==6?1:operation>=4?3:2))return;
  const auto anchor=arrangementAnchor();if(!nodes.contains(anchor))return;
  QMap<QString,QPointF> deltas;
  auto bounds=[&](QString id){auto n=nodes.value(id);return QRectF(n->scenePos()-QPointF(n->size.width()/2,n->size.height()/2),n->size);};
  if(operation==6){for(auto id:ids){auto p=nodes[id]->scenePos()+gridOffset(id);deltas[id]=View::nearestGrid(p)-p;}}
  else if(operation<4){auto ref=nodes[anchor]->scenePos();auto edge=bounds(anchor).topLeft();for(auto id:ids){auto p=nodes[id]->scenePos();auto b=bounds(id);deltas[id]=operation==0?QPointF(0,ref.y()-p.y()):operation==1?QPointF(ref.x()-p.x(),0):operation==2?QPointF(edge.x()-b.left(),0):QPointF(0,edge.y()-b.top());}}
  else{
   const bool horizontal=operation==4;
   std::stable_sort(ids.begin(),ids.end(),[&](QString a,QString b){return horizontal?bounds(a).center().x()<bounds(b).center().x():bounds(a).center().y()<bounds(b).center().y();});
   const auto first=bounds(ids.first()),last=bounds(ids.last());double start=horizontal?first.left():first.top(),end=horizontal?last.right():last.bottom(),total=0;
   for(auto id:ids)total+=horizontal?bounds(id).width():bounds(id).height();
   double gap=(end-start-total)/(ids.size()-1);
   if(gap<0){status->setText(t("Spazio insufficiente: allontana le due figure estreme prima di distribuire.","Not enough room: move the two outer figures farther apart before distributing."));return;}
   double position=start;for(auto id:ids){auto b=bounds(id);deltas[id]=horizontal?QPointF(position-b.left(),0):QPointF(0,position-b.top());position+=(horizontal?b.width():b.height())+gap;}
  }
  applyLayout(deltas,t("Disponi figure","Arrange shapes"));
 }
 QIcon arrangementIcon(int op){QPixmap pix(48,48);pix.setDevicePixelRatio(2);pix.fill(Qt::transparent);QPainter p(&pix);p.setRenderHint(QPainter::Antialiasing);p.setPen(QPen(theme.accent,1.5));p.setBrush(theme.selected);if(op==0||op==3){p.drawLine(2,op==0?12:5,22,op==0?12:5);p.drawRect(QRectF(4,5,5,14));p.drawRect(QRectF(14,8,5,8));}else if(op==1||op==2){p.drawLine(op==1?12:5,2,op==1?12:5,22);p.drawRect(QRectF(5,4,14,5));p.drawRect(QRectF(8,14,8,5));}else if(op==4){for(int x:{3,10,17})p.drawRect(QRectF(x,7,4,10));}else if(op==5){for(int y:{3,10,17})p.drawRect(QRectF(7,y,10,4));}else{for(int x:{5,12,19})for(int y:{5,12,19})p.drawPoint(x,y);}return QIcon(pix);}
 void addArrangementActions(QMenu *menu,bool legacyAlignment=false){
  const QStringList labels={t("Allinea sulla stessa riga","Align on the same row"),t("Allinea sulla stessa colonna","Align on the same column"),t("Allinea a sinistra","Align left edges"),t("Allinea in alto","Align top edges"),t("Distribuisci orizzontalmente","Distribute horizontally"),t("Distribuisci verticalmente","Distribute vertically"),t("Aggancia selezione alla griglia","Snap selection to grid")};
  for(int op=legacyAlignment?2:0;op<labels.size();++op){auto a=action(menu,labels[op],{},[this,op]{arrangeSelection(op);});a->setObjectName(op==0?QString("ctx.alignH"):op==1?QString("ctx.alignV"):QString("arrange.%1").arg(op));a->setIcon(arrangementIcon(op));a->setEnabled(movableSelection().size()>=(op==6?1:op>=4?3:2));a->setToolTip(op==6?t("Posiziona ogni figura sulla griglia. Annullabile con Ctrl+Z.","Place each shape on the grid. Undo with Ctrl+Z."):op>=4?t("Mantiene ferme le figure estreme e uniforma gli spazi liberi.","Keep the outer shapes fixed and equalize the free gaps."):t("Il quadratino colorato indica la figura di riferimento. L’allineamento prevale sull’aggancio alla griglia.","The colored square marks the reference shape. Alignment takes precedence over grid snapping."));}
 }
 void gridMenus(QMenu *viewMenu){
  for(bool snap:{false,true}){auto a=action(viewMenu,snap?t("Aggancia alla griglia","Snap to grid"):t("Mostra griglia","Show grid"),{},[]{});a->setObjectName(snap?"snapGrid":"showGrid");a->setCheckable(true);a->setChecked(snap?view->snapEnabled:view->gridVisible);a->setToolTip(snap?t("Passo 20. Alt durante il trascinamento sospende l’aggancio; i gruppi mantengono le distanze interne.","Step 20. Hold Alt while dragging to bypass snapping; groups preserve their internal spacing."):t("Griglia a punti, solo sullo schermo: non compare nel PDF.","Dotted grid, on screen only: it is excluded from PDF export."));connect(a,&QAction::toggled,this,[this,snap](bool checked){if(snap)view->snapEnabled=checked;else view->gridVisible=checked;view->viewport()->update();});}
  auto menu=menus->addMenu(t("&Disponi","&Arrange"));menu->setObjectName("arrangeMenu");connect(menu,&QMenu::aboutToShow,this,[this,menu]{menu->clear();addArrangementActions(menu);});addArrangementActions(menu);
 }
 void updateLayoutReference(){
  const auto anchor=movableSelection().size()>1?arrangementAnchor():QString();
  for(auto node:nodes){bool mark=node->id==anchor&&!printing;if(node->layoutReference!=mark){node->layoutReference=mark;node->update();}}
  if(!anchor.isEmpty()&&contextHint){contextHint->setText(contextHint->text()+t("   ·   Riferimento: ","   ·   Reference: ")+nodes[anchor]->label);}
 }
