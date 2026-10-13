#pragma once
#include <QtWidgets>
#include <functional>
#include <cmath>

// Semantic IDs are attached to nodes, labels and connector segments alike.
class LinkItem:public QGraphicsPathItem {
public:
 explicit LinkItem(const QPainterPath &path):QGraphicsPathItem(path){}
 QPainterPath shape()const override{QPainterPathStroker s;s.setWidth(12);return s.createStroke(path());}
 QRectF boundingRect()const override{return QGraphicsPathItem::boundingRect().adjusted(-6,-6,6,6);}
};

class View:public QGraphicsView {
 bool resizing=false;bool pressedObject=false,dragging=false,spacePan=false;
 QPoint pressPoint;
 QPointF pressScene;
public:
 bool gridVisible=false,snapEnabled=false,dragAlt=false;
 QColor gridColor=QColor("#8c98a6");
 static constexpr double gridStep=20.;
 double visibleGridStep()const{double step=gridStep;while(step*transform().m11()<12)step*=2;return step;}
 static QPointF nearestGrid(QPointF p){return {std::round(p.x()/gridStep)*gridStep,std::round(p.y()/gridStep)*gridStep};}
 void drawBackground(QPainter *p,const QRectF &rect)override{
  QGraphicsView::drawBackground(p,rect);if(!gridVisible)return;
  const double step=visibleGridStep();p->save();QPen pen(gridColor,2.,Qt::SolidLine,Qt::RoundCap);pen.setCosmetic(true);p->setPen(pen);
  QVector<QPointF> points;for(double x=std::ceil(rect.left()/step)*step;x<=rect.right();x+=step)for(double y=std::ceil(rect.top()/step)*step;y<=rect.bottom();y+=step)points<<QPointF(x,y);
  if(!points.isEmpty()){p->drawPoints(points.constData(),points.size());}
  p->restore();
 }
 std::function<void(QString)> doubleClicked,resizeStarted;
 std::function<bool(QString,QPointF)> resizeHit;
 std::function<void(QPointF)> resizeMoved;
 std::function<void()> resizeFinished;
 std::function<void()> zoomChanged,dragStarted,dragFinished;
 std::function<void(QPointF)> dragMoved;
 // mode 0: select unless already in the group; 1: toggle; 2: add.
 std::function<void(QString,int)> picked;
 std::function<void(QString,QPointF,QPoint)> contextRequested;
 std::function<bool(QKeyEvent*)> commandKey;
 explicit View(QGraphicsScene *s):QGraphicsView(s){
  setRenderHints(QPainter::Antialiasing|QPainter::TextAntialiasing);
  setDragMode(RubberBandDrag);setTransformationAnchor(NoAnchor);
  setResizeAnchor(AnchorViewCenter);setBackgroundBrush(Qt::white);
  setViewportUpdateMode(BoundingRectViewportUpdate);
 }
 QString objectAt(QPoint pos)const{
  for(auto item:items(pos)){QString id=item->data(0).toString();if(!id.isEmpty())return id;}
  return {};
 }
 void zoomAt(double factor,QPoint anchor){
  if(!std::isfinite(factor)||factor<=0)return;
  const double current=transform().m11();
  const double target=qBound(.1,current*factor,6.);
  if(qFuzzyCompare(current,target))return;
  QPointF before=mapToScene(anchor);
  scale(target/current,target/current);
  QPointF after=mapToScene(anchor);
  // A large scene extent prevents scroll-bar clamping near a small diagram.
  translate(after.x()-before.x(),after.y()-before.y());
  if(zoomChanged)zoomChanged();
 }
 void zoom(double factor){zoomAt(factor,viewport()->rect().center());}
 void resetZoom(){zoom(1./transform().m11());}
 void wheelEvent(QWheelEvent *e)override{
  if(e->modifiers()&Qt::ControlModifier){
   double steps=e->angleDelta().y()/120.;
   if(e->angleDelta().y()==0)steps=e->pixelDelta().y()/80.;
   if(steps!=0)zoomAt(std::pow(1.2,qBound(-12.,steps,12.)),e->position().toPoint());
   e->accept();
  }else QGraphicsView::wheelEvent(e);
 }
 void mouseDoubleClickEvent(QMouseEvent *e)override{auto id=objectAt(e->pos());if(e->button()==Qt::LeftButton&&doubleClicked){finishDrag();doubleClicked(id);e->accept();return;}QGraphicsView::mouseDoubleClickEvent(e);}
 void mousePressEvent(QMouseEvent *e)override{
  if(spacePan){QGraphicsView::mousePressEvent(e);return;}
  auto id=objectAt(e->pos());
  if(e->button()==Qt::RightButton){
   if(!id.isEmpty()&&picked)picked(id,e->modifiers()&Qt::ControlModifier?2:0);
   e->accept();return;
  }
  if(e->button()==Qt::LeftButton&&!id.isEmpty()){
   if(!(e->modifiers()&Qt::ControlModifier)&&resizeHit&&resizeHit(id,mapToScene(e->pos()))){resizing=true;pressScene=mapToScene(e->pos());if(resizeStarted)resizeStarted(id);e->accept();return;}
   setFocus(Qt::MouseFocusReason);
   bool ctrl=e->modifiers()&Qt::ControlModifier;
   if(picked)picked(id,ctrl?1:0);
   // Ctrl-click changes membership only; an ordinary drag moves the group.
   pressedObject=!ctrl;dragging=false;pressPoint=e->pos();pressScene=mapToScene(pressPoint);
   e->accept();return;
  }
  QGraphicsView::mousePressEvent(e);
 }
 void mouseMoveEvent(QMouseEvent *e)override{
  if(resizing){if(resizeMoved)resizeMoved(mapToScene(e->pos())-pressScene);e->accept();return;}
  if(pressedObject&&(e->buttons()&Qt::LeftButton)){
   if(!dragging&&(e->pos()-pressPoint).manhattanLength()>=QApplication::startDragDistance()){
    dragging=true;if(dragStarted)dragStarted();
   }
   dragAlt=e->modifiers()&Qt::AltModifier;
   if(dragging&&dragMoved)dragMoved(mapToScene(e->pos())-pressScene);
   e->accept();return;
  }
  QGraphicsView::mouseMoveEvent(e);
 }
 void finishDrag(){if(resizing){resizing=false;if(resizeFinished)resizeFinished();}bool commit=dragging;pressedObject=false;dragging=false;dragAlt=false;if(commit&&dragFinished)dragFinished();}
 void cancelInteraction(){resizing=false;pressedObject=false;dragging=false;dragAlt=false;spacePan=false;setInteractive(true);setDragMode(RubberBandDrag);}
 void mouseReleaseEvent(QMouseEvent *e)override{
  if(e->button()==Qt::LeftButton&&(pressedObject||resizing)){finishDrag();e->accept();return;}
  if(e->button()==Qt::RightButton){e->accept();return;}
  QGraphicsView::mouseReleaseEvent(e);
 }
 void contextMenuEvent(QContextMenuEvent *e)override{
  if(!contextRequested){e->ignore();return;}
  QPoint pos=e->reason()==QContextMenuEvent::Keyboard?viewport()->rect().center():e->pos();
  auto id=e->reason()==QContextMenuEvent::Keyboard?QString():objectAt(pos);
  if(!id.isEmpty()&&picked)picked(id,e->modifiers()&Qt::ControlModifier?2:0);
  contextRequested(id,mapToScene(pos),e->reason()==QContextMenuEvent::Keyboard?viewport()->mapToGlobal(pos):e->globalPos());
  e->accept();
 }
 void keyPressEvent(QKeyEvent *e)override{
  if(commandKey&&commandKey(e)){e->accept();return;}
  if(e->key()==Qt::Key_Space&&!e->isAutoRepeat()){
   finishDrag();spacePan=true;setInteractive(false);setDragMode(ScrollHandDrag);e->accept();
  }else QGraphicsView::keyPressEvent(e);
 }
 void keyReleaseEvent(QKeyEvent *e)override{
  if(e->key()==Qt::Key_Space){spacePan=false;setInteractive(true);setDragMode(RubberBandDrag);e->accept();}
  else QGraphicsView::keyReleaseEvent(e);
 }
 void focusOutEvent(QFocusEvent *e)override{
  finishDrag();spacePan=false;setInteractive(true);setDragMode(RubberBandDrag);QGraphicsView::focusOutEvent(e);
 }
};
