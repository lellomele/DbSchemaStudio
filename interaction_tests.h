#pragma once

// Dispatch real Qt viewport events; assertions concern observable model/selection state.
static void interactionTests(Window &w,const QString &output,std::function<void(QString,bool)> check){
 const auto original=Model::example();
 auto reset=[&]{w.model=original;w.history.clear();w.selection.clear();w.selectionIds.clear();w.uml=false;w.en=false;w.mode=1;w.applyTheme();w.buildMenus();w.refresh();w.fit();qApp->processEvents();};
 auto point=[&](QString id){auto n=w.nodes[id];return w.view->mapFromScene(w.uml&&n->kind=="entity"?n->sceneBoundingRect().topLeft()+QPointF(30,20):n->sceneBoundingRect().center());};
 auto mouse=[&](QEvent::Type type,QPoint p,Qt::MouseButton button,Qt::MouseButtons buttons,Qt::KeyboardModifiers mods=Qt::NoModifier){QMouseEvent event(type,p,w.view->viewport()->mapToGlobal(p),button,buttons,mods);QApplication::sendEvent(w.view->viewport(),&event);};
 auto click=[&](QString id,Qt::MouseButton button,Qt::KeyboardModifiers mods=Qt::NoModifier){auto p=point(id);mouse(QEvent::MouseButtonPress,p,button,button,mods);mouse(QEvent::MouseButtonRelease,p,button,Qt::NoButton,mods);};
 auto menuAction=[](QMenu *menu,QString name)->QAction*{for(auto a:menu->findChildren<QAction*>())if(a->objectName()==name)return a;return nullptr;};
 auto command=[&](QString name){QScopedPointer<QMenu> menu(w.contextMenu(w.selection,{}));auto a=menuAction(menu.data(),name);if(a&&a->isEnabled()){a->trigger();return true;}return false;};
 reset();
 click("ordine",Qt::LeftButton);click("riga",Qt::LeftButton,Qt::ControlModifier);
 check("Ctrl-left adds to selection",w.selectionIds==QSet<QString>({"ordine","riga"}));
 click("ordine",Qt::LeftButton,Qt::ControlModifier);
 check("Ctrl-left toggles off",w.selectionIds==QSet<QString>({"riga"}));
 click("ordine",Qt::RightButton,Qt::ControlModifier);
 check("Ctrl-right adds without replacing",w.selectionIds==QSet<QString>({"ordine","riga"}));
 click("riga",Qt::RightButton);
 check("Right-click preserves an existing group",w.selectionIds.size()==2&&w.selection=="riga");
 click("contiene",Qt::RightButton);
 check("Right-click another object replaces selection",w.selectionIds==QSet<QString>({"contiene"}));
 auto realContext=w.view->contextRequested;QString contextId;int calls=0;
 w.view->contextRequested=[&](QString id,QPointF,QPoint){contextId=id;++calls;};
 QPoint ep=point("ordine");QContextMenuEvent ce(QContextMenuEvent::Mouse,ep,w.view->viewport()->mapToGlobal(ep),Qt::ControlModifier);QApplication::sendEvent(w.view->viewport(),&ce);
 check("Ctrl context event preserves modifier and semantic target",calls==1&&contextId=="ordine"&&w.selectionIds==QSet<QString>({"contiene","ordine"}));
 w.view->contextRequested=realContext;
 check("Outline mirrors multi-selection",w.tree->selectedItems().size()==2);
 QPoint blank(20,20);mouse(QEvent::MouseButtonPress,blank,Qt::LeftButton,Qt::LeftButton);mouse(QEvent::MouseButtonRelease,blank,Qt::LeftButton,Qt::NoButton);check("Blank click clears selection",w.selectionIds.isEmpty());
 auto rect=w.nodes["ordine"]->sceneBoundingRect().adjusted(-15,-15,15,15);auto start=w.view->mapFromScene(rect.topLeft()),stop=w.view->mapFromScene(rect.bottomRight());mouse(QEvent::MouseButtonPress,start,Qt::LeftButton,Qt::LeftButton);mouse(QEvent::MouseMove,stop,Qt::NoButton,Qt::LeftButton);mouse(QEvent::MouseButtonRelease,stop,Qt::LeftButton,Qt::NoButton);check("Rubber-band selection remains available",w.selectionIds.contains("ordine"));
 auto beforePan=w.view->mapToScene(blank);QKeyEvent spaceDown(QEvent::KeyPress,Qt::Key_Space,Qt::NoModifier);QApplication::sendEvent(w.view,&spaceDown);mouse(QEvent::MouseButtonPress,blank,Qt::LeftButton,Qt::LeftButton);mouse(QEvent::MouseMove,blank+QPoint(30,40),Qt::NoButton,Qt::LeftButton);mouse(QEvent::MouseButtonRelease,blank+QPoint(30,40),Qt::LeftButton,Qt::NoButton);QKeyEvent spaceUp(QEvent::KeyRelease,Qt::Key_Space,Qt::NoModifier);QApplication::sendEvent(w.view,&spaceUp);check("Space drag pans without editing the model",QLineF(beforePan,w.view->mapToScene(blank)).length()>5&&w.model.json()==original.json());
 reset();click("ordine",Qt::LeftButton);click("numordine",Qt::LeftButton,Qt::ControlModifier);click("riga",Qt::LeftButton,Qt::ControlModifier);
 auto p=point("ordine"),end=p+QPoint(48,32);QPointF delta=w.view->mapToScene(end)-w.view->mapToScene(p);
 mouse(QEvent::MouseButtonPress,p,Qt::LeftButton,Qt::LeftButton);mouse(QEvent::MouseMove,end,Qt::NoButton,Qt::LeftButton);mouse(QEvent::MouseButtonRelease,end,Qt::LeftButton,Qt::NoButton);
 check("Group drag moves both entities",QLineF(w.model.get("ordine")->chen,original.get("ordine")->chen+delta).length()<.01&&QLineF(w.model.get("riga")->chen,original.get("riga")->chen+delta).length()<.01);
 check("Group drag moves selected owned attributes only once",QLineF(w.model.get("numordine")->chen,original.get("numordine")->chen+delta).length()<.01);
 check("Group drag is one undoable edit",w.history.count()==1);w.history.undo();check("Group undo restores entire model",w.model.json()==original.json());w.history.redo();check("Group redo preserves selection",w.selectionIds.size()==3);w.history.undo();
 w.uml=true;w.refresh();w.fit();qApp->processEvents();w.selectOnly("contiene");click("numordine",Qt::LeftButton);check("UML attribute click targets row not entity",w.selection=="numordine"&&w.selectionIds.size()==1);
 click("ordine",Qt::LeftButton);click("riga",Qt::LeftButton,Qt::ControlModifier);p=point("ordine");p.setY(w.view->mapFromScene(w.nodes["ordine"]->sceneBoundingRect().topLeft()+QPointF(30,20)).y());end=p+QPoint(40,25);
 mouse(QEvent::MouseButtonPress,p,Qt::LeftButton,Qt::LeftButton);mouse(QEvent::MouseMove,end,Qt::NoButton,Qt::LeftButton);mouse(QEvent::MouseButtonRelease,end,Qt::LeftButton,Qt::NoButton);
 check("UML group drag leaves Chen layout intact",w.model.get("ordine")->chen==original.get("ordine")->chen&&w.model.get("ordine")->uml!=original.get("ordine")->uml);
 reset();w.selectOnly("ordine");check("Entity menu adds owned attribute",command("ctx.addAttribute")&&w.model.get(w.selection)->owner=="ordine");w.history.undo();
 w.selectOnly("data");check("Attribute menu changes identifier",command("ctx.identifier")&&w.model.get("data")->key);w.history.undo();check("Identifier context edit undo",!w.model.get("data")->key);
 w.selectOnly("contiene");check("Relationship menu changes multiplicity",command("ctx.cardA.1,N")&&w.model.get("contiene")->cardA=="1,N");w.history.undo();
 {QScopedPointer<QMenu> menu(w.contextMenu("contiene",{}));auto a=menuAction(menu.data(),"ctx.cardB.0,N");check("Identifying dependent multiplicity stays constrained",a&&!a->isEnabled()&&w.model.get("contiene")->cardB=="1,1");}
 check("Context identifying toggle",command("ctx.identifying")&&!w.model.weak("riga"));w.history.undo();
 bool connectorHit=false,labelHit=false;for(auto item:w.connectors)if(item->data(0).toString()=="contiene"){if(auto line=dynamic_cast<LinkItem*>(item)){auto p2=w.view->mapFromScene(line->path().pointAtPercent(.5));connectorHit|=w.view->objectAt(p2)=="contiene";}if(dynamic_cast<QGraphicsSimpleTextItem*>(item))labelHit|=w.view->objectAt(w.view->mapFromScene(item->sceneBoundingRect().center()))=="contiene";}
 check("Relationship lines are semantic click targets",connectorHit);check("Cardinality labels are semantic click targets",labelHit);
 w.change("Offset",[](Model&m){m.get("riga")->chen+=QPointF(0,100);});auto prior=w.model;w.selectOnly("ordine");w.pick("riga",2);check("Group context alignment",command("ctx.alignH")&&w.model.get("ordine")->chen.y()==w.model.get("riga")->chen.y());check("Alignment moves owned attributes",w.model.get("numordine")->chen.y()==prior.get("numordine")->chen.y()+100);w.history.undo();check("Alignment undo restores model",w.model.json()==prior.json());
 QString error;auto ids=w.selectionIds;check("PDF export preserves entire selection",w.exportPdf(output+"/group-selection.pdf",QPageSize::A4,true,true,error)&&w.selectionIds==ids&&w.scene->selectedItems().size()==ids.size());
 check("Group deletion removes dependent objects",command("ctx.delete")&&w.model.objects.isEmpty());w.history.undo();check("Group deletion undo restores dependencies",w.model.json()==prior.json());
 {QScopedPointer<QMenu> menu(w.contextMenu({},QPointF(900,500)));auto a=menuAction(menu.data(),"ctx.new");if(a)a->trigger();check("Canvas context creates at clicked position",a&&w.model.get(w.selection)->chen==QPointF(900,500));}w.history.undo();
 reset();QPoint anchor(w.view->viewport()->width()*3/4,w.view->viewport()->height()/3);auto before=w.view->mapToScene(anchor);double scale=w.view->transform().m11();
 auto wheel=[&](QPoint angle,QPoint pixel,Qt::KeyboardModifiers mods){QWheelEvent e(anchor,w.view->viewport()->mapToGlobal(anchor),pixel,angle,Qt::NoButton,mods,Qt::NoScrollPhase,false);QApplication::sendEvent(w.view->viewport(),&e);};
 wheel({0,120},{},Qt::ControlModifier);check("Ctrl-wheel scales proportionally",qAbs(w.view->transform().m11()/scale-1.2)<.0001);check("Ctrl-wheel keeps pointer anchored",QLineF(before,w.view->mapToScene(anchor)).length()<2/w.view->transform().m11());
 wheel({0,-120},{},Qt::ControlModifier);check("Reverse wheel restores zoom",qAbs(w.view->transform().m11()-scale)<.0001);wheel({0,60},{},Qt::ControlModifier);check("High resolution wheel uses partial steps",qAbs(w.view->transform().m11()/scale-std::sqrt(1.2))<.0001);
 scale=w.view->transform().m11();wheel({}, {0,40},Qt::ControlModifier);check("Pixel wheel deltas are supported",qAbs(w.view->transform().m11()/scale-std::sqrt(1.2))<.0001);scale=w.view->transform().m11();wheel({120,0},{},Qt::ControlModifier);check("Horizontal Ctrl-wheel does not change zoom",w.view->transform().m11()==scale);wheel({0,120},{},Qt::NoModifier);check("Wheel without Ctrl only scrolls",w.view->transform().m11()==scale);
 for(int i=0;i<50;++i){wheel({0,120},{},Qt::ControlModifier);}check("Zoom upper limit",qAbs(w.view->transform().m11()-6)<.0001);for(int i=0;i<100;++i){wheel({0,-120},{},Qt::ControlModifier);}check("Zoom lower limit",qAbs(w.view->transform().m11()-.1)<.0001);w.view->resetZoom();check("Zoom 100 percent",qAbs(w.view->transform().m11()-1)<.0001);
 reset();w.selectOnly("ordine");w.pick("riga",2);qApp->processEvents();w.grab().save(output+"/multi-selection.png");
 {QScopedPointer<QMenu> menu(w.contextMenu("riga",{}));menu->popup(w.view->viewport()->mapToGlobal(point("riga")));qApp->processEvents();menu->grab().save(output+"/group-menu.png");menu->close();}
 w.selectOnly("contiene");{QScopedPointer<QMenu> menu(w.contextMenu("contiene",{}));menu->popup(w.view->viewport()->mapToGlobal(point("contiene")));qApp->processEvents();menu->grab().save(output+"/relationship-menu.png");menu->close();}
 reset();
}
