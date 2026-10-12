#include <QtWidgets>
#include <QtPrintSupport>
#include "model.h"
#include "runtime_support.h"
#include <exception>
#include "logical.h"
#include "logical_audit.h"
#include "sql_dialog.h"
#include "rise_import.h"
#include "interaction.h"
#include "teaching_examples.h"
#include <algorithm>
#include "language.h"
#include "title_bar.h"
#include <functional>

static QString storageDirectory(){return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)+"/SchemaStudio/DbSchemaStudio-Qt6";}

struct Theme {
 QColor paper,surface,ink,muted,line,accent,selected;
 static double luminance(QColor c){auto f=[](double v){return v<=.04045?v/12.92:std::pow((v+.055)/1.055,2.4);};return .2126*f(c.redF())+.7152*f(c.greenF())+.0722*f(c.blueF());}
 static double contrast(QColor a,QColor b){double x=luminance(a),y=luminance(b);return (qMax(x,y)+.05)/(qMin(x,y)+.05);}
 static QColor textOn(QColor bg){return contrast(Qt::black,bg)>=contrast(Qt::white,bg)?QColor(Qt::black):QColor(Qt::white);}
 static Theme make(bool dark,int preset,QColor custom={}){
  Theme t;if(dark){t.paper="#171c24";t.surface="#242c38";t.ink="#f0f3f8";t.muted="#bac6d6";t.line="#b4c1d2";t.selected="#34445c";t.accent=preset==2?"#76dfc1":preset==0?"#e0e6ef":"#a9cbff";}
  else{t.paper="#f5f7fb";t.surface="#ffffff";t.ink="#172334";t.muted="#47576c";t.line="#50647a";t.selected="#dce8fc";t.accent=preset==2?"#08674e":preset==0?"#34465d":"#234fa2";}
  if(preset==3){t.paper=dark?Qt::black:Qt::white;t.surface=t.paper;t.ink=dark?Qt::white:Qt::black;t.muted=t.ink;t.line=t.ink;t.accent=dark?QColor("#ffff00"):QColor("#002080");t.selected=dark?QColor("#333333"):QColor("#dddddd");}
  if(custom.isValid()&&contrast(custom,t.paper)>=3&&contrast(custom,t.surface)>=3)t.accent=custom;
  return t;
 }
};
class Node:public QGraphicsItem {
public:
 bool layoutReference=false; qreal readingAngle=0;QString id,kind,label,subtitle,badgePrefix,badgeText,badgeTail;bool inherited=false;bool key=false,weak=false,identifying=false,uml=false;QSizeF size;Theme theme;
 std::function<void(Node*)> moving,begin,end;
 static QFont attributeFont(bool inherited=false){QFont f("Segoe UI");f.setPixelSize(14);f.setItalic(inherited);return f;}
 static QFont badgeFont(){auto f=attributeFont();f.setPixelSize(12);f.setBold(true);return f;}
 static qreal attributeWidth(const QString &prefix,const QString &badge,const QString &tail,bool inherited=false){QFontMetricsF fm(attributeFont(inherited));return fm.horizontalAdvance(prefix)+fm.horizontalAdvance(tail)+(badge.isEmpty()?0:QFontMetricsF(badgeFont()).horizontalAdvance(badge)+20);}
 QColor badgeBackground()const{return theme.selected;}
 QColor badgeForeground()const{return Theme::contrast(theme.muted,badgeBackground())>=4.5?theme.muted:Theme::textOn(badgeBackground());}
 void setNoteSize(QSizeF value){prepareGeometryChange();size=value;}
 Node(){setFlags(ItemIsSelectable|ItemIsMovable|ItemSendsGeometryChanges);setZValue(2);}
 QRectF boundingRect()const override{if(uml&&kind=="relation")return QRectF(-size.width()/2-4,-size.height()/2-28,size.width()+8,size.height()+32);return QRectF(-size.width()/2-4,-size.height()/2-4,size.width()+8,size.height()+8);}
 QPainterPath shape()const override{QPainterPath p;QRectF r(-size.width()/2,-size.height()/2,size.width(),size.height());if(kind=="attribute"&&!uml)p.addEllipse(r);else if(kind=="relation"&&!uml){QPolygonF q;q<<QPointF(0,r.top())<<QPointF(r.right(),0)<<QPointF(0,r.bottom())<<QPointF(r.left(),0);p.addPolygon(q);}else{p.addRect(r);if(uml&&kind=="relation")p.addRect(r.translated(0,-24));}return p;}
 QVariant itemChange(GraphicsItemChange c,const QVariant &v) override{auto r=QGraphicsItem::itemChange(c,v);if(c==ItemPositionHasChanged&&moving)moving(this);return r;}
 void mousePressEvent(QGraphicsSceneMouseEvent *e)override{if(begin)begin(this);QGraphicsItem::mousePressEvent(e);}
 void mouseReleaseEvent(QGraphicsSceneMouseEvent *e)override{QGraphicsItem::mouseReleaseEvent(e);if(end)end(this);}
 void paint(QPainter *p,const QStyleOptionGraphicsItem *,QWidget *)override{
  p->setRenderHint(QPainter::Antialiasing);QRectF r(-size.width()/2,-size.height()/2,size.width(),size.height());
  p->setPen(QPen(isSelected()?theme.accent:theme.line,isSelected()?2.5:1.5));p->setBrush(isSelected()?theme.selected:theme.surface);
  if(kind=="note"){p->drawRect(r);p->save();p->setClipRect(r.adjusted(10,8,-10,-8));p->setPen(theme.ink);QFont f("Segoe UI");f.setPixelSize(14);p->setFont(f);p->drawText(r.adjusted(12,10,-12,-12),Qt::AlignLeft|Qt::AlignTop|Qt::TextWordWrap,label);p->restore();if(isSelected()){p->setPen(QPen(theme.accent,1.5));p->drawLine(r.bottomRight()-QPointF(12,2),r.bottomRight()-QPointF(2,12));p->drawLine(r.bottomRight()-QPointF(7,2),r.bottomRight()-QPointF(2,7));}return;}
  if(kind=="entity") {p->drawRect(r);if(weak)p->drawRect(r.adjusted(5,5,-5,-5));}
  else if(kind=="attribute"&&!uml)p->drawEllipse(r);
  else if(kind=="relation"&&!uml){QPolygonF q;q<<QPointF(0,r.top())<<QPointF(r.right(),0)<<QPointF(0,r.bottom())<<QPointF(r.left(),0);p->drawPolygon(q);if(identifying){p->save();p->scale(.88,.8);p->drawPolygon(q);p->restore();}}
  else if(kind=="relation"&&uml){
   QRectF labelRect=r.translated(0,-24);p->setPen(isSelected()?QPen(theme.accent,2):QPen(Qt::NoPen));p->setBrush(isSelected()?theme.selected:theme.paper);p->drawRoundedRect(labelRect,3,3);
   QFont f("Segoe UI");f.setPixelSize(14);f.setBold(true);p->setFont(f);p->setPen(theme.ink);p->drawText(labelRect.adjusted(8,0,-28,0),Qt::AlignCenter,label);
   p->save();p->translate(labelRect.right()-13,labelRect.center().y());p->rotate(readingAngle*180/3.141592653589793);p->setPen(Qt::NoPen);p->setBrush(theme.ink);QPolygonF arrow;arrow<<QPointF(6,0)<<QPointF(-5,-5)<<QPointF(-5,5);p->drawPolygon(arrow);p->restore();return;
  }
  else if(kind=="isa"){QPolygonF q;q<<QPointF(0,-20)<<QPointF(30,24)<<QPointF(-30,24);p->drawPolygon(q);}
  else {p->setPen(Qt::NoPen);p->drawRoundedRect(r,3,3);}
  if(layoutReference&&isSelected())p->fillRect(QRectF(r.left()-4,r.top()-4,9,9),theme.accent);
  p->setPen(inherited?theme.muted:theme.ink);QFont f("Segoe UI");f.setPixelSize(14);f.setItalic(inherited);p->setFont(f);
  if(uml&&kind=="attribute"&&!badgeText.isEmpty()){
   p->save();p->setClipRect(r);const auto body=attributeFont(inherited);p->setFont(body);QFontMetricsF fm(body);qreal x=r.left()+10;
   p->drawText(QRectF(x,r.top(),fm.horizontalAdvance(badgePrefix),r.height()),Qt::AlignLeft|Qt::AlignVCenter,badgePrefix);x+=fm.horizontalAdvance(badgePrefix)+6;
   QRectF badge(x,r.center().y()-10,QFontMetricsF(badgeFont()).horizontalAdvance(badgeText)+12,20);
   p->setPen(Qt::NoPen);p->setBrush(badgeBackground());p->drawRoundedRect(badge,3,3);p->setPen(badgeForeground());p->setFont(badgeFont());p->drawText(badge,Qt::AlignCenter,badgeText);
   p->setFont(body);p->setPen(inherited?theme.muted:theme.ink);p->drawText(QRectF(badge.right()+2,r.top(),fm.horizontalAdvance(badgeTail),r.height()),Qt::AlignLeft|Qt::AlignVCenter,badgeTail);p->restore();return;
  }
  if(kind=="entity"&&uml){p->fillRect(QRectF(r.left()+1,r.top()+1,r.width()-2,41),theme.accent);p->setPen(Theme::textOn(theme.accent));f.setBold(true);p->setFont(f);p->drawText(QRectF(r.left()+10,r.top()+7,r.width()-20,28),Qt::AlignCenter,label);p->setPen(theme.line);p->drawLine(QPointF(r.left(),r.top()+42),QPointF(r.right(),r.top()+42));}
  else{if(kind=="entity")f.setBold(true);f.setUnderline(key&&!weak);p->setFont(f);p->drawText(r.adjusted(10,2,-10,-2),(uml&&kind=="attribute"?Qt::AlignLeft|Qt::AlignVCenter:Qt::AlignCenter),label);if(key&&weak){auto width=QFontMetricsF(f).horizontalAdvance(label);p->setPen(QPen(theme.ink,1.2,Qt::DashLine));p->drawLine(QPointF(-width/2,11),QPointF(width/2,11));}}
 }
};
class IllustrationView:public QGraphicsView {
public:
 explicit IllustrationView(QGraphicsScene *scene):QGraphicsView(scene){setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);}
 void resizeEvent(QResizeEvent *event)override{QGraphicsView::resizeEvent(event);if(scene()&&!scene()->sceneRect().isEmpty())fitInView(scene()->sceneRect(),Qt::KeepAspectRatio);}
};
class Snapshot:public QUndoCommand {
 Model before,after;std::function<void(const Model&)> restore;
public:
 Snapshot(Model b,Model a,std::function<void(const Model&)> fn,QString title):QUndoCommand(title),before(std::move(b)),after(std::move(a)),restore(std::move(fn)){}
 void undo()override{restore(before);}void redo()override{restore(after);}
};
class Window:public QMainWindow {
public:
 LogicalSchema logicalCache;bool logicalView=false;QComboBox *levelBox=nullptr;QLabel *levelStatus=nullptr;QAction *generateAction=nullptr;
 Model model=ordersExample();bool showNotes=true;QString resizingNote;bool showTypes=true,umlPkLabels=false;bool en=false,uml=false,rebuilding=false,printing=false,testMode=false;
 QString selection,filePath;QSet<QString> selectionIds,dragIds;Theme theme;int mode=0,preset=1,fontSize=10;QColor custom;
 QSettings prefs;QUndoStack history;QGraphicsScene *scene;View *view;QTreeWidget *tree;QDockWidget *outline,*properties;QWidget *formWidget;QFormLayout *form;QLabel *status,*phase,*credit,*flag;
 QMenuBar *menus=nullptr;QLineEdit *outlineSearch=nullptr;QSpinBox *zoomBox=nullptr;QLabel *contextHint=nullptr;QTimer *recoveryTimer=nullptr;quint64 documentEpoch=0;bool ownsRecovery=false;QVector<RoleBadge*> roleBadges;
 std::function<bool(const QUrl&)> openExternal=[](const QUrl &url){return QDesktopServices::openUrl(url);};
 QHash<QString,Node*> nodes;QVector<QGraphicsItem*> connectors;Model dragBefore;QComboBox *notationBox;QString recoveryPath;
 explicit Window(bool testing=false):testMode(testing),prefs(storageDirectory()+"/preferences.ini",QSettings::IniFormat){
  if(!testing){umlPkLabels=prefs.value("umlPkLabels",false).toBool();showNotes=prefs.value("showNotes",true).toBool();showTypes=prefs.value("showTypes",true).toBool();en=prefs.value("language","it")=="en";mode=prefs.value("mode",0).toInt();preset=prefs.value("palette",1).toInt();fontSize=qBound(9,prefs.value("fontSize",10).toInt(),20);custom=QColor(prefs.value("accent").toString());}
  setMinimumSize(800,520);history.setUndoLimit(100);
  menus=menuBar();auto signature=new QWidget;auto signatureLayout=new QHBoxLayout(signature);signatureLayout->setContentsMargins(8,0,12,0);flag=new QLabel;credit=new QLabel;credit->setObjectName("copyrightLink");credit->setTextInteractionFlags(Qt::LinksAccessibleByMouse|Qt::LinksAccessibleByKeyboard);signatureLayout->addWidget(flag);signatureLayout->addWidget(credit);menus->setCornerWidget(signature,Qt::TopRightCorner);
  recoveryPath=storageDirectory()+"/recovery.schema";
  scene=new QGraphicsScene(this);view=new View(scene);if(!testing){view->gridVisible=prefs.value("gridVisible",false).toBool();view->snapEnabled=prefs.value("snapEnabled",false).toBool();}view->setObjectName("diagram");view->setAccessibleName(t("Diagramma concettuale","Conceptual diagram"));auto canvas=new QWidget;auto canvasLayout=new QVBoxLayout(canvas);canvasLayout->setContentsMargins(0,0,0,0);canvasLayout->setSpacing(0);contextHint=new QLabel;contextHint->setObjectName("contextHint");contextHint->setWordWrap(true);contextHint->setMargin(9);contextHint->setTextFormat(Qt::PlainText);canvasLayout->addWidget(contextHint);canvasLayout->addWidget(view,1);setCentralWidget(canvas);
  outline=new QDockWidget(this);outline->setObjectName("outline");tree=new QTreeWidget;tree->setHeaderHidden(true);tree->setSelectionMode(QAbstractItemView::ExtendedSelection);tree->setMinimumWidth(145);auto outlinePanel=new QWidget;auto outlineLayout=new QVBoxLayout(outlinePanel);outlineLayout->setContentsMargins(8,4,8,8);outlineSearch=new QLineEdit;outlineSearch->setObjectName("outlineSearch");outlineSearch->setClearButtonEnabled(true);outlineLayout->addWidget(outlineSearch);outlineLayout->addWidget(tree);outline->setWidget(outlinePanel);connect(outlineSearch,&QLineEdit::textChanged,this,[this]{filterOutline();});connect(outlineSearch,&QLineEdit::returnPressed,this,[this]{for(QTreeWidgetItemIterator it(tree);*it;++it)if(!(*it)->isHidden()&&(*it)->text(0).contains(outlineSearch->text(),Qt::CaseInsensitive)){selectOnly((*it)->data(0,Qt::UserRole).toString());fitSelection();view->setFocus();break;}});addDockWidget(Qt::LeftDockWidgetArea,outline);
  properties=new QDockWidget(this);properties->setObjectName("properties");auto scroll=new QScrollArea;scroll->setWidgetResizable(true);formWidget=new QWidget;form=new QFormLayout(formWidget);form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);form->setRowWrapPolicy(QFormLayout::WrapLongRows);scroll->setWidget(formWidget);properties->setWidget(scroll);properties->setMinimumWidth(240);addDockWidget(Qt::RightDockWidgetArea,properties);
  status=new QLabel;status->setSizePolicy(QSizePolicy::Ignored,QSizePolicy::Preferred);statusBar()->addWidget(status,1);phase=new QLabel;statusBar()->addPermanentWidget(phase);zoomBox=new QSpinBox;zoomBox->setObjectName("zoomPercent");zoomBox->setRange(10,600);zoomBox->setSingleStep(10);zoomBox->setSuffix(" %");zoomBox->setKeyboardTracking(false);zoomBox->setFixedWidth(100);statusBar()->addPermanentWidget(zoomBox);connect(zoomBox,QOverload<int>::of(&QSpinBox::valueChanged),this,[this](int value){view->zoom(value/100./view->transform().m11());});
  connect(credit,&QLabel::linkActivated,this,[this](const QString&){openExternal(QUrl(profileUrl()));});
  connect(scene,&QGraphicsScene::selectionChanged,this,[this]{if(rebuilding)return;selectionIds.clear();for(auto i:scene->selectedItems())if(auto n=dynamic_cast<Node*>(i))selectionIds.insert(n->id);normalizeSelection();inspector();syncTree();updateStatus();});
  connect(tree,&QTreeWidget::itemSelectionChanged,this,[this]{if(rebuilding)return;selectionIds.clear();for(auto i:tree->selectedItems())selectionIds.insert(i->data(0,Qt::UserRole).toString());if(tree->currentItem())selection=tree->currentItem()->data(0,Qt::UserRole).toString();applySelection();});
  connect(&history,&QUndoStack::cleanChanged,this,[this]{title();});
  recoveryTimer=new QTimer(this);recoveryTimer->setSingleShot(true);recoveryTimer->setInterval(1500);connect(recoveryTimer,&QTimer::timeout,this,[this]{writeRecovery();});connect(&history,&QUndoStack::indexChanged,this,[this]{if(!testMode&&!history.isClean())recoveryTimer->start();});
  view->zoomChanged=[this]{updateStatus();};
  view->commandKey=[this](QKeyEvent *event){return diagramKey(event);};tree->installEventFilter(this);
  view->picked=[this](QString id,int mode){pick(id,mode);};
  view->dragStarted=[this]{dragBefore=model;dragIds=selectionIds;};
  view->dragMoved=[this](QPointF delta){moveDrag(delta);};
  view->dragFinished=[this]{finishGroupDrag();};
  view->contextRequested=[this](QString id,QPointF point,QPoint screen){QScopedPointer<QMenu> menu(contextMenu(id,point));menu->exec(screen);};
  view->doubleClicked=[this](QString id){if(model.note(id))editNote(id);};
  view->resizeHit=[this](QString id,QPointF pos){auto n=nodes.value(id);return n&&n->kind=="note"&&n->isSelected()&&QRectF(n->size.width()/2-16,n->size.height()/2-16,20,20).contains(n->mapFromScene(pos));};
  view->resizeStarted=[this](QString id){resizingNote=id;dragBefore=model;};
  view->resizeMoved=[this](QPointF delta){resizeNoteDrag(delta);};
  view->resizeFinished=[this]{resizingNote.clear();finishGroupDrag();};
  applyTheme();buildMenus();refresh();resize(1380,850);if(auto screen=QGuiApplication::primaryScreen())resize(size().boundedTo(screen->availableGeometry().size()-QSize(30,70)));
  if(!testing){auto geo=prefs.value("geometry").toByteArray();if(!geo.isEmpty())restoreGeometry(geo);restoreState(prefs.value("layout").toByteArray());bool visible=false;for(auto s:QGuiApplication::screens())if(s->availableGeometry().intersects(frameGeometry()))visible=true;if(!visible)move(QGuiApplication::primaryScreen()->availableGeometry().topLeft()+QPoint(30,30));}
  QTimer::singleShot(0,this,[this]{fit();});
  if(!testing){auto timer=new QTimer(this);connect(timer,&QTimer::timeout,this,[this]{if(!history.isClean()){QString error;if(!writeRecovery(&error))status->setText(t("Recupero automatico non riuscito: ","Recovery write failed: ")+localizedError(error,en));}});timer->start(30000);}
 }
 QString t(const char *it,const char *eng)const{return QString::fromUtf8(en?eng:it);}
 ~Window()override{
  // QGraphicsScene emits selectionChanged while removing selected items. Disconnect
  // before C++ members (model, selection, history) are destroyed ahead of QObject children.
  rebuilding=true;disconnect(scene,nullptr,this,nullptr);disconnect(tree,nullptr,this,nullptr);disconnect(&history,nullptr,this,nullptr);view->dragFinished={};view->commandKey={};scene->clear();
 }
 #include "notes_ui.h"
 #include "learning_ui.h"
 #include "examples_ui.h"
 #include "window_help.h"
 #include "workflow_help.h"
 #include "inheritance_ui.h"
 #include "arrangement_ui.h"
 #include "association_help.h"
 #include "logical_ui.h"
 #include "rise_ui.h"
 #include "modern_ui.h"
 #include "recovery_ui.h"
 void physicalDialog(){
  if(model.logical.isEmpty()||logicalStale(model)){if(QMessageBox::question(this,t("Modello logico","Logical model"),t("Prima del SQL occorre generare o aggiornare il modello logico. Procedere?","Generate or update the logical model before SQL. Continue?"),QMessageBox::Yes|QMessageBox::No)!=QMessageBox::Yes||!generateDialog())return;}
  SqlDialog dialog(model,en,isDark(),testMode?nullptr:&prefs,this);if(dialog.exec()==QDialog::Accepted){auto settings=dialog.resultConfig();change(t("Impostazioni fisiche e SQL","Physical settings and SQL"),[settings](Model &m){m.physical=settings;});}
 }
 bool systemDark()const {
#ifdef Q_OS_WIN
  return QGuiApplication::styleHints()->colorScheme()==Qt::ColorScheme::Dark;
#else
  return QApplication::palette().color(QPalette::Window).lightness()<128;
#endif
 }
 bool isDark()const {return mode==2||(mode==0&&systemDark());}
 void applyTheme(){
  theme=Theme::make(isDark(),preset,custom);QPalette p;p.setColor(QPalette::Window,theme.surface);p.setColor(QPalette::WindowText,theme.ink);p.setColor(QPalette::Base,theme.paper);p.setColor(QPalette::AlternateBase,theme.surface);p.setColor(QPalette::Text,theme.ink);p.setColor(QPalette::PlaceholderText,theme.muted);p.setColor(QPalette::Button,theme.surface);p.setColor(QPalette::ButtonText,theme.ink);p.setColor(QPalette::Highlight,theme.accent);p.setColor(QPalette::HighlightedText,Theme::textOn(theme.accent));p.setColor(QPalette::ToolTipBase,theme.surface);p.setColor(QPalette::ToolTipText,theme.ink);p.setColor(QPalette::Link,theme.accent);p.setColor(QPalette::Disabled,QPalette::Text,theme.muted);p.setColor(QPalette::Disabled,QPalette::ButtonText,theme.muted);qApp->setPalette(p);qApp->setFont(QFont("Segoe UI",fontSize));
  qApp->setStyleSheet(QString("QToolBar{spacing:6px;padding:7px;border-bottom:1px solid %1} QToolButton{padding:6px} QLineEdit,QComboBox,QSpinBox{padding:5px} QPushButton{padding:6px 12px} QDockWidget::title{padding:8px} QTreeWidget{border:0} QTreeView::item{padding:2px} QLineEdit,QSpinBox,QPushButton{border-radius:4px;border:1px solid %1;color:%4} QLineEdit,QSpinBox{background:%5} QPushButton{background:%6} QLineEdit:focus,QSpinBox:focus{border:2px solid %2} QPushButton:default{font-weight:600} QToolButton:hover{background:%3;border-radius:4px} QStatusBar{padding:4px} QGroupBox{margin-top:16px;padding:8px} QGroupBox::title{subcontrol-origin:margin;left:8px} ").arg(theme.line.name(),theme.accent.name(),theme.selected.name(),theme.ink.name(),theme.paper.name(),theme.surface.name()));
  view->setBackgroundBrush(theme.paper);view->gridColor=theme.muted;view->gridColor.setAlpha(100);updateCredit();
 }
 QString attributeLabel(const Object &o)const{return o.name+(showTypes&&!o.dataType.isEmpty()?": "+genericTypeName(o.dataType,en):QString());}
 QComboBox *typeCombo(QString value){auto combo=new QComboBox;combo->setObjectName("genericType");combo->setAccessibleName(t("Tipo di dato","Data type"));for(const auto &id:genericTypeIds())combo->addItem(genericTypeName(id,en),id);combo->setCurrentIndex(combo->findData(value));helpWidget(combo,t("Tipo generico facoltativo, indipendente da SQL. Reale include numeri con parte frazionaria: DECIMAL o DOUBLE saranno scelti nel fisico. Telefono e CAP sono normalmente stringhe. Conferma con Applica modifiche.","Optional generic type, independent of SQL. Real includes fractional numbers: DECIMAL or DOUBLE will be chosen in the physical model. Phone numbers and postal codes are normally strings. Confirm with Apply changes."));return combo;}
 void title(){QString text=QString("%1%2 — Database Schema Studio 0.9.7").arg(history.isClean()?"":"* ",filePath.isEmpty()?t("Senza titolo","Untitled"):QFileInfo(filePath).fileName());setWindowTitle(text);}

 QAction *action(QMenu *menu,QString name,QKeySequence shortcut,std::function<void()> fn){auto a=menu->addAction(name);if(!shortcut.isEmpty())a->setShortcut(shortcut);connect(a,&QAction::triggered,this,std::move(fn));return a;}
 void buildMenus(){
  setUiLanguage(en);localizeHistory();if(levelStatus){statusBar()->removeWidget(levelStatus);delete levelStatus;}levelStatus=nullptr;generateAction=nullptr;levelBox=nullptr;
  auto oldMenus=menus->findChildren<QMenu*>(QString(),Qt::FindDirectChildrenOnly);menus->clear();qDeleteAll(oldMenus);auto bars=findChildren<QToolBar*>(QString(),Qt::FindDirectChildrenOnly);for(auto bar:bars){removeToolBar(bar);delete bar;}
  auto file=menus->addMenu(t("&File","&File"));
  auto n=action(file,t("Nuovo","New"),QKeySequence::New,[this]{if(maybeSave()){++documentEpoch;model={};filePath.clear();selection.clear();selectionIds.clear();history.clear();refresh();}});
  auto o=action(file,t("Apri…","Open…"),QKeySequence::Open,[this]{openDialog();});
  action(file,t("Importa RISE…","Import RISE…"),{},[this]{importRiseDialog();})->setObjectName("importRiseAction");
  auto s=action(file,t("Salva","Save"),QKeySequence::Save,[this]{save(false);});
  action(file,t("Salva con nome…","Save as…"),QKeySequence::SaveAs,[this]{save(true);});file->addSeparator();
  auto pdf=action(file,t("Esporta PDF…","Export PDF…"),QKeySequence("Ctrl+P"),[this]{pdfDialog();});
  file->addSeparator();action(file,t("Esci","Exit"),QKeySequence::Quit,[this]{close();});
  auto edit=menus->addMenu(t("&Modifica","&Edit"));auto undo=history.createUndoAction(edit,t("Annulla","Undo"));undo->setShortcut(QKeySequence::Undo);edit->addAction(undo);auto redo=history.createRedoAction(edit,t("Ripristina","Redo"));redo->setShortcuts({QKeySequence::Redo,QKeySequence("Ctrl+Shift+Z")});edit->addAction(redo);
  auto del=action(edit,t("Elimina selezione","Delete selection"),QKeySequence::Delete,[this]{deleteSelected();});
  auto ins=menus->addMenu(t("&Inserisci","&Insert"));auto entity=action(ins,t("Entità","Entity"),QKeySequence("Ctrl+E"),[this]{addEntity();});auto attribute=action(ins,t("Attributo","Attribute"),QKeySequence("Ctrl+T"),[this]{addAttribute();});auto rel=action(ins,t("Associazione","Relationship"),QKeySequence("Ctrl+R"),[this]{addLink(false);});auto isa=action(ins,"ISA",QKeySequence("Ctrl+I"),[this]{addLink(true);});
  auto noteAction=action(ins,t("Nota","Note"),{},[this]{addNoteAt(center());});noteAction->setObjectName("addNote");noteAction->setIcon(noteIcon());noteAction->setToolTip(richTip(t("Aggiunge un riquadro di testo libero. Doppio clic per modificare; trascina l’angolo inferiore destro per ridimensionare. Le note non cambiano le tabelle.","Add a plain text box. Double-click to edit; drag the lower-right corner to resize. Notes do not change tables.")));
  auto v=menus->addMenu(t("&Vista","&View"));action(v,t("Adatta diagramma","Fit diagram"),QKeySequence("Ctrl+0"),[this]{fit();});action(v,t("Ingrandisci","Zoom in"),QKeySequence("Ctrl++"),[this]{view->zoom(1.2);});action(v,t("Riduci","Zoom out"),QKeySequence("Ctrl+-"),[this]{view->zoom(1/1.2);});v->addAction(outline->toggleViewAction());v->addAction(properties->toggleViewAction());action(v,t("Aspetto e lingua…","Appearance and language…"),{},[this]{appearance();});
  auto identifiersMenu=v->addMenu(t("Identificatori UML-like","UML-like identifiers"));identifiersMenu->setToolTipsVisible(true);auto identifiersGroup=new QActionGroup(identifiersMenu);identifiersGroup->setExclusive(true);
  for(bool pk:{false,true}){auto choice=identifiersMenu->addAction(pk?t("[PK] · convenzione didattica","[PK] · teaching convention"):t("{ID} · identificatore","{ID} · identifier"));choice->setObjectName(pk?"umlIdentifierPK":"umlIdentifierID");choice->setCheckable(true);choice->setChecked(umlPkLabels==pk);identifiersGroup->addAction(choice);choice->setToolTip(t("Cambia solo le etichette del concettuale UML-like. [PK] indica l’identificatore completo; gli identificatori locali e le chiavi alternative restano distinti. Il modello e il SQL non cambiano.","Changes only conceptual UML-like labels. [PK] denotes the complete identifier; local identifiers and alternate keys remain distinct. The model and SQL are unchanged."));connect(choice,&QAction::triggered,this,[this,pk]{if(umlPkLabels==pk)return;umlPkLabels=pk;storePreferences();refresh();});}
  auto typesAction=action(v,t("Mostra tipi di dato","Show data types"),{},[]{});typesAction->setObjectName("showDataTypes");typesAction->setCheckable(true);typesAction->setChecked(showTypes);connect(typesAction,&QAction::toggled,this,[this](bool value){showTypes=value;refresh();});
  auto notesAction=action(v,t("Mostra note","Show notes"),{},[]{});notesAction->setObjectName("showNotes");notesAction->setCheckable(true);notesAction->setChecked(showNotes);connect(notesAction,&QAction::toggled,this,[this](bool value){showNotes=value;refresh();});
  gridMenus(v);
  auto tools=menus->addMenu(t("&Modello","&Model"));auto check=action(tools,t("Verifica modello","Validate model"),QKeySequence("F7"),[this]{validate();});
  examplesMenu();
  auto help=menus->addMenu(t("&Aiuto","&Help"));action(help,t("Guida illustrata: Ordini…","Illustrated guide: Orders (Italian)…"),{},[this]{auto path=QCoreApplication::applicationDirPath()+"/guide/Guida-rapida-Ordini.pdf";if(QFile::exists(path))openExternal(QUrl::fromLocalFile(path));else QMessageBox::information(this,t("Guida illustrata","Illustrated guide"),t("La guida non è presente nella cartella dell’applicazione. Estrai il pacchetto completo.","The guide is missing from the application folder. Extract the complete package."));})->setObjectName("ordersGuide");action(help,t("Guida rapida","Quick guide"),QKeySequence("F1"),[this]{QMessageBox::information(this,t("Guida rapida","Quick guide"),t("1. Crea un’entità, selezionala e aggiungi attributi.\n2. Seleziona un attributo e marca Parte della chiave.\n3. Crea associazioni scegliendo i due ruoli e le cardinalità.\n4. Trascina gli oggetti per sistemare il disegno.\n\nIns: aggiunge un attributo all’entità selezionata; senza selezione crea un’entità; su associazioni e ISA apre le proprietà. F2: rinomina.\nCtrl + rotella: zoom sul puntatore (10–600%).\nCtrl + clic sinistro: aggiungi/rimuovi dalla selezione.\nCtrl + clic destro: aggiungi e apri le azioni.\nTasto destro: azioni per oggetto o gruppo.\nTrascina una selezione per spostarla insieme.\nSpazio + trascinamento: sposta il foglio.\nLe viste Chen e UML-like condividono il modello e conservano posizioni separate.\n\nSeleziona il livello Logico per tradurre il concettuale in tabelle, PK e FK. Usa Genera/Aggiorna logico e controlla il rapporto. File → Importa RISE recupera un modello concettuale con riepilogo e scelta degli identificatori. Scegli 3 · Fisico/SQL (F8) per scegliere HSQLDB, MySQL o MariaDB, configurare i tipi e generare, copiare o salvare il codice.","1. Create an entity, select it and add attributes.\n2. Select an attribute and mark Part of key.\n3. Add relationships with two roles and multiplicities.\n4. Drag objects to arrange the diagram.\n\nInsert: add an attribute to the selected entity; without selection create an entity; on relationships and ISA open Properties. F2: rename.\nCtrl + wheel: zoom at pointer (10–600%).\nCtrl + left-click: toggle selection.\nCtrl + right-click: add and open actions.\nRight-click: object or group actions.\nDrag a selection to move it together.\nSpace + drag: pan.\nChen and UML-like share the model and retain separate positions.\n\nSelect the Logical level to map the conceptual model to tables, PKs and FKs. Use Generate/Update logical and review the report. File → Import RISE recovers a conceptual model with a summary and identifier selection. Choose 3 · Physical/SQL (F8) to choose HSQLDB, MySQL or MariaDB, configure types and generate, copy or save code."));});
  auto bar=addToolBar(t("Strumenti","Tools"));bar->setObjectName("tools");bar->setMovable(false);bar->addAction(n);bar->addAction(o);bar->addAction(s);bar->addSeparator();bar->addAction(undo);bar->addAction(redo);bar->addSeparator();bar->addAction(entity);bar->addAction(attribute);bar->addAction(rel);bar->addAction(isa);bar->addAction(del);bar->addSeparator();bar->addAction(check);bar->addAction(noteAction);
  addToolBarBreak();auto display=addToolBar(t("Diagramma","Diagram"));display->setObjectName("display");display->setMovable(false);
  levelBox=new QComboBox;levelBox->setObjectName("modelLevel");levelBox->addItems({t("1 · Concettuale","1 · Conceptual"),t("2 · Logico","2 · Logical"),t("3 · Fisico/SQL","3 · Physical/SQL")});levelBox->setCurrentIndex(logicalView?1:0);display->addWidget(levelBox);connect(levelBox,QOverload<int>::of(&QComboBox::currentIndexChanged),this,[this](int index){QTimer::singleShot(0,this,[this,index]{if(index<2)switchLevel(index==1);else {if(levelBox){QSignalBlocker block(levelBox);levelBox->setCurrentIndex(logicalView?1:0);}physicalDialog();}});});notationBox=new QComboBox;notationBox->addItems({"Chen · min–max","UML-like"});notationBox->setToolTip(richTip(t("Chen mostra gli attributi separati dalle entità; UML-like li elenca nel rettangolo. Le due viste descrivono lo stesso modello. Una linea tratteggiata indica una partecipazione facoltativa, una continua una partecipazione obbligatoria. La zampa di gallina indica molti. In UML-like puoi scegliere le etichette {ID} oppure [PK] dal menu Vista. Per le chiavi parziali compare anche la parola locale.","Chen shows attributes separately from entities; UML-like lists them inside the rectangle. Both views describe the same model. A dashed line indicates optional participation; a solid line indicates mandatory participation. A crow’s foot means many. In UML-like, choose {ID} or [PK] labels from the View menu. Partial keys also carry the word local.")));notationBox->setVisible(!logicalView);notationBox->setCurrentIndex(uml?1:0);notationBox->setAccessibleName(t("Notazione","Notation"));display->addWidget(notationBox);connect(notationBox,QOverload<int>::of(&QComboBox::currentIndexChanged),this,[this](int i){uml=i==1;refresh();fit();});
  auto fitAction=display->addAction(t("Adatta","Fit"));connect(fitAction,&QAction::triggered,this,[this]{fit();});auto less=display->addAction("−");connect(less,&QAction::triggered,this,[this]{view->zoom(1/1.2);});auto more=display->addAction("+");connect(more,&QAction::triggered,this,[this]{view->zoom(1.2);});auto settings=display->addAction(t("Aspetto e lingua","Appearance and language"));connect(settings,&QAction::triggered,this,[this]{appearance();});
  action(help,t("Come procedere: dal concettuale al database…","How to proceed: from conceptual model to database…"),{},[this]{workflow();})->setObjectName("workflowAction");
  auto journey=display->addAction(t("Come procedere","How to proceed"));journey->setObjectName("workflowToolbar");journey->setToolTip(t("Leggi come descrivere una situazione, trasformarla in tabelle e preparare il codice SQL.","Learn how to describe a situation, turn it into tables and prepare SQL code."));connect(journey,&QAction::triggered,this,[this]{workflow();});
  auto sqlAction=action(tools,t("Fisico / SQL…","Physical / SQL…"),QKeySequence("F8"),[this]{physicalDialog();});sqlAction->setObjectName("physicalSqlAction");
  levelControls(display,tools);for(auto a:{entity,attribute,rel,isa})a->setVisible(!logicalView);
  outlineSearch->setPlaceholderText(t("Cerca elementi…","Find elements…"));outlineSearch->setAccessibleName(t("Cerca nel modello","Search model"));outlineSearch->setToolTip(t("Filtra entità, attributi e tabelle per nome. Invio seleziona e inquadra il primo risultato. Ctrl+F apre la ricerca.","Filter entities, attributes and tables by name. Enter selects and centers the first match. Ctrl+F opens search."));zoomBox->setAccessibleName(t("Zoom del diagramma","Diagram zoom"));zoomBox->setToolTip(t("Da 10% a 600%. Puoi anche usare Ctrl + rotella sul diagramma.","From 10% to 600%. You can also use Ctrl + wheel over the diagram."));
  action(v,t("Cerca nel modello","Search model"),QKeySequence::Find,[this]{outline->show();outlineSearch->setFocus();outlineSearch->selectAll();});
  action(v,t("Ripristina disposizione pannelli","Reset panel layout"),{},[this]{outline->setFloating(false);properties->setFloating(false);addDockWidget(Qt::LeftDockWidgetArea,outline);addDockWidget(Qt::RightDockWidgetArea,properties);outline->show();properties->show();resizeDocks({outline,properties},{220,310},Qt::Horizontal);fit();});
  decorateActions(n,o,s,undo,redo,entity,attribute,rel,isa,del,check,pdf);
  action(help,t("Esempio Negozio: cosa osservare…","Shop example: what to look for…"),{},[this]{shopGuide();});
  action(help,t("Attributi dell’associazione…","Relationship attributes…"),{},[this]{associationAttributesHelp();});
  action(help,t("Informazioni su Database Schema Studio…","About Database Schema Studio…"),{},[this]{about();})->setObjectName("aboutAction");
  outline->setWindowTitle(logicalView?t("Tabelle e colonne","Tables and columns"):t("Modello","Model"));properties->setWindowTitle(t("Proprietà","Properties"));phase->setText(logicalView?t("Logico · nessun tipo SQL","Logical · no SQL types"):t("Concettuale","Conceptual"));decorateInterface();
 }
 void change(const QString &name,std::function<void(Model&)> fn){Model next=model;try{fn(next);}catch(const std::exception &){status->setText(t("Operazione non completata. Il modello precedente è stato conservato.","Operation failed. The previous model has been preserved."));return;}if(next.json()==model.json())return;Model verified;QString error;if(!Model::parse(QJsonDocument(next.json()).toJson(QJsonDocument::Compact),verified,error)){status->setText(t("Modifica non applicata: ","Edit not applied: ")+localizedError(error,en));return;}history.push(new Snapshot(model,next,[this](const Model&m){model=m;refresh();},name));}
 void normalizeSelection(){
  if(!showNotes)for(const auto &n:model.notes)selectionIds.remove(n.id);
  if(!selectionIds.contains(selection))for(const auto &n:model.notes)if(selectionIds.contains(n.id)){selection=n.id;break;}
  if(logicalView){for(auto it=selectionIds.begin();it!=selectionIds.end();)if(!logicalCache.has(*it)&&!model.note(*it))it=selectionIds.erase(it);else ++it;if(!selectionIds.contains(selection)){selection.clear();for(const auto&t:logicalCache.tables){if(selectionIds.contains(t.id)){selection=t.id;break;}for(const auto&c:t.columns)if(selectionIds.contains(c.id)){selection=c.id;break;}if(!selection.isEmpty())break;}}return;}
  for(auto it=selectionIds.begin();it!=selectionIds.end();)if(!model.get(*it)&&!model.note(*it))it=selectionIds.erase(it);else ++it;
  if(!selectionIds.contains(selection)){selection.clear();for(const auto&o:model.objects)if(selectionIds.contains(o.id)){selection=o.id;break;}}
 }
 void applySelection(){normalizeSelection();rebuilding=true;scene->clearSelection();for(auto id:selectionIds)if(nodes.contains(id))nodes[id]->setSelected(true);rebuilding=false;syncTree();inspector();updateStatus();}
 void pick(QString id,int mode){if(!model.note(id)&&(logicalView?!logicalCache.has(id):!model.get(id)))return;if(mode==1){if(selectionIds.contains(id))selectionIds.remove(id);else selectionIds.insert(id);}else if(mode==2)selectionIds.insert(id);else if(!selectionIds.contains(id))selectionIds={id};if(selectionIds.contains(id))selection=id;applySelection();}
 void selectOnly(QString id){selection=id;selectionIds={id};applySelection();}
 void selectAll(){if(logicalView){selectionIds.clear();for(const auto&t:logicalCache.tables)selectionIds.insert(t.id);if(showNotes)for(const auto &n:model.notes)selectionIds.insert(n.id);applySelection();return;}selectionIds.clear();for(const auto&o:model.objects)selectionIds.insert(o.id);if(showNotes)for(const auto &n:model.notes)selectionIds.insert(n.id);applySelection();}
 void moveDrag(QPointF delta){delta=snappedDrag(delta);if(logicalView){moveLogicalDrag(delta);return;}
  model=dragBefore;rebuilding=true;moveNotesDrag(delta);
  for(auto&o:model.objects){bool selected=dragIds.contains(o.id);if(o.kind=="attribute"){if(uml)continue;selected=selected||dragIds.contains(o.owner);}if(selected)(uml?o.uml:o.chen)+=delta;}
  for(const auto&o:model.objects)if(nodes.contains(o.id)&&!(uml&&o.kind=="attribute"))nodes[o.id]->setPos(uml?o.uml:o.chen);
  links();rebuilding=false;
 }
 void finishGroupDrag(){if(model.json()==dragBefore.json())return;Model verified;QString error;if(!Model::parse(QJsonDocument(model.json()).toJson(QJsonDocument::Compact),verified,error)){model=dragBefore;refresh();status->setText(t("Spostamento annullato: posizione fuori dai limiti del diagramma.","Move cancelled: position outside diagram bounds."));return;}Model before=dragBefore,after=model;history.push(new Snapshot(before,after,[this](const Model&m){model=m;refresh();},t("Sposta selezione","Move selection")));}
 QStringList movableSelection()const{QStringList ids;for(const auto &n:model.notes)if(selectionIds.contains(n.id)&&nodes.contains(n.id))ids<<n.id;if(logicalView){for(const auto&t:logicalCache.tables)if(selectionIds.contains(t.id))ids<<t.id;return ids;}for(const auto&o:model.objects)if(selectionIds.contains(o.id)&&!(o.kind=="attribute"&&(uml||selectionIds.contains(o.owner))))ids<<o.id;return ids;}
 void alignSelection(bool horizontal){arrangeSelection(horizontal?0:1);}

 void fitSelection(){QRectF bounds;for(auto id:selectionIds)if(nodes.contains(id))bounds=bounds.united(nodes[id]->sceneBoundingRect());if(bounds.isEmpty()){fit();return;}view->fitInView(bounds.adjusted(-50,-50,50,50),Qt::KeepAspectRatio);if(view->transform().m11()>2){view->resetTransform();view->scale(2,2);view->centerOn(bounds.center());}updateStatus();}
 void renameObject(QString id){if(model.note(id)){editNote(id);return;}if(logicalView){renameLogical(id);return;}auto o=model.get(id);if(!o)return;bool ok=false;QString value=QInputDialog::getText(this,t("Rinomina","Rename"),t("Nome","Name"),QLineEdit::Normal,o->name,&ok).trimmed().left(120);if(ok&&!value.isEmpty())change(t("Rinomina","Rename"),[=](Model&m){m.get(id)->name=value;});}
 QMenu *contextMenu(QString hit,QPointF point){if(!hit.isEmpty()&&(model.note(hit)||(!selectionIds.isEmpty()&&model.note(selection))))return noteContext(point);if(logicalView){auto menu=logicalContextMenu(hit);if(hit.isEmpty()){action(menu,t("Nuova nota qui","New note here"),{},[this,point]{addNoteAt(point);});pasteNoteAction(menu,point);}return menu;}
  auto menu=new QMenu(this);menu->setToolTipsVisible(true);
  auto add=[&](QMenu*parent,QString code,QString label,std::function<void()> fn){auto a=parent->addAction(label);a->setObjectName(code);connect(a,&QAction::triggered,menu,std::move(fn));return a;};
  if(hit.isEmpty()){pasteNoteAction(menu,point);add(menu,"ctx.note",t("Nuova nota qui","New note here"),[this,point]{addNoteAt(point);});add(menu,"ctx.new",t("Nuova entità qui","New entity here"),[=]{addEntityAt(point);});menu->addSeparator();}
  if(selectionIds.size()==1&&model.get(selection)){QString id=selection;auto o=model.get(id);
   add(menu,"ctx.properties",t("Modifica proprietà…","Edit properties…"),[=]{selectOnly(id);properties->show();properties->raise();if(auto field=formWidget->findChild<QLineEdit*>()){field->setFocus();field->selectAll();}});
   if(o->kind!="isa")add(menu,"ctx.rename",t("Rinomina…","Rename…"),[=]{renameObject(id);});
   if(o->kind=="entity"){
    add(menu,"ctx.addAttribute",t("Aggiungi attributo","Add attribute"),[=]{selectOnly(id);addAttribute();});
    add(menu,"ctx.relation",t("Collega con un’associazione…","Connect with a relationship…"),[=]{addLink(false,id);});
    add(menu,"ctx.isa",t("Imposta superclasse…","Set superclass…"),[=]{addLink(true,id);});
   }else if(o->kind=="attribute"){
    auto a=add(menu,"ctx.identifier",model.weak(o->owner)?t("Chiave parziale","Partial key"):t("Parte della chiave","Part of key"),[=]{change(t("Modifica identificatore","Change identifier"),[=](Model&m){m.get(id)->key=!m.get(id)->key;});});a->setCheckable(true);a->setChecked(o->key);
    QString owner=o->owner;add(menu,"ctx.owner",t("Seleziona entità","Select entity"),[=]{selectOnly(owner);});
   }else if(o->kind=="relation"){
    add(menu,"ctx.associationAttributes",t("Attributi dell’associazione…","Relationship attributes…"),[this]{associationAttributesHelp();});
    for(bool sideA:{true,false}){auto entity=model.get(sideA?o->a:o->b);auto sub=menu->addMenu(t("Partecipazione ","Participation ")+(sideA?"A · ":"B · ")+entity->name);if(!sideA&&o->identifying)sub->setEnabled(false);for(QString card:{"0,1","1,1","0,N","1,N"}){auto a=add(sub,QString("ctx.card%1.%2").arg(sideA?"A":"B",card),"("+card+")",[=]{change(t("Modifica cardinalità","Change multiplicity"),[=](Model&m){auto r=m.get(id);(sideA?r->cardA:r->cardB)=card;});});a->setCheckable(true);a->setChecked(card==(sideA?o->cardA:o->cardB));a->setEnabled(sideA||!o->identifying);}}
    auto a=add(menu,"ctx.identifying",t("Identificante · A identifica B","Identifying · A identifies B"),[=]{change(t("Relazione identificante","Identifying relationship"),[=](Model&m){auto r=m.get(id);r->identifying=!r->identifying;if(r->identifying)r->cardB="1,1";});});a->setCheckable(true);a->setChecked(o->identifying);a->setEnabled(o->a!=o->b);
   }else if(o->kind=="isa"){QString a=o->a,b=o->b;add(menu,"ctx.subclass",t("Seleziona sottoclasse","Select subclass"),[=]{selectOnly(a);});add(menu,"ctx.superclass",t("Seleziona superclasse","Select superclass"),[=]{selectOnly(b);});}
  }else if(selectionIds.size()>1){
   auto title=menu->addAction(t("Selezione: ","Selection: ")+QString::number(selectionIds.size()));title->setEnabled(false);
   QStringList entities;for(const auto&o:model.objects)if(selectionIds.contains(o.id)&&o.kind=="entity")entities<<o.id;
   if(selectionIds.size()==2&&entities.size()==2){QString a=entities.contains(selection)?selection:entities.first();QString b=entities.first()==a?entities.last():entities.first();add(menu,"ctx.relation",t("Associa le due entità…","Relate these entities…"),[=]{addLink(false,a,b);});add(menu,"ctx.isa",t("Crea ISA tra le due entità…","Create ISA between entities…"),[=]{addLink(true,a,b);});}
  }
  if(!selectionIds.isEmpty()){auto arrange=menu->addMenu(t("Disponi","Arrange"));addArrangementActions(arrange);menu->addSeparator();add(menu,"ctx.fitSelection",t("Inquadra selezione","Fit selection"),[=]{fitSelection();});add(menu,"ctx.delete",t("Elimina selezione","Delete selection"),[=]{deleteSelected(true);});}
  menu->addSeparator();add(menu,"ctx.selectAll",t("Seleziona tutto","Select all"),[=]{selectAll();});add(menu,"ctx.fit",t("Adatta diagramma","Fit diagram"),[=]{fit();});add(menu,"ctx.zoom100",t("Zoom 100%","Zoom 100%"),[=]{view->resetZoom();});return menu;
 }
 void syncTree(){filterOutline();rebuilding=true;for(QTreeWidgetItemIterator it(tree);*it;++it)(*it)->setSelected(selectionIds.contains((*it)->data(0,Qt::UserRole).toString()));rebuilding=false;}
 void updateStatus(){updateModernUi();updateLayoutReference();updateLevelStatus();if(logicalView){status->setText(t("Tabelle: ","Tables: ")+QString::number(logicalCache.tables.size())+t(" · Selezionati: "," · Selected: ")+QString::number(selectionIds.size())+QString(" · Zoom %1%").arg(qRound(view->transform().m11()*100)));return;}int count=0;for(const auto &o:model.objects)if(o.kind=="entity")++count;status->setText(t("Entità: ","Entities: ")+QString::number(count)+t("   ·   Selezionati: ","   ·   Selected: ")+QString::number(selectionIds.size())+QString("   ·   Zoom %1%").arg(qRound(view->transform().m11()*100)));}
 void refresh(){if(logicalView&&model.logical.isEmpty()){logicalView=false;buildMenus();}if(logicalView){refreshLogical();return;}
  rebuilding=true;roleBadges.clear();nodes.clear();connectors.clear();scene->clear();tree->clear();QHash<QString,QTreeWidgetItem*> parents;
  auto makeTree=[&](const Object &o,QTreeWidgetItem *parent){auto item=parent?new QTreeWidgetItem(parent):new QTreeWidgetItem(tree);item->setText(0,o.name+attributeKeySuffix(o));item->setData(0,Qt::UserRole,o.id);item->setToolTip(0,objectHelp(o));return item;};
  QFont diagramFont("Segoe UI");diagramFont.setPixelSize(14);
  for(const auto &o:model.objects)if(o.kind=="entity")parents[o.id]=makeTree(o,nullptr);
  for(const auto &o:model.objects)if(o.kind!="entity")makeTree(o,o.kind=="attribute"?parents.value(o.owner):nullptr);
  for(const auto &entity:model.objects)if(entity.kind=="entity")for(const auto &a:model.inheritedAttributes(entity.id)){auto item=new QTreeWidgetItem(parents.value(entity.id));item->setText(0,t("Ereditato: ","Inherited: ")+inheritedLabel(a));item->setData(0,Qt::UserRole,a.id);item->setToolTip(0,inheritedTip(a));QFont font=item->font(0);font.setItalic(true);item->setFont(0,font);}
  for(const auto &o:model.objects){
   if(o.kind=="attribute"&&uml)continue;
   auto node=new Node;node->setToolTip(objectHelp(o));node->id=o.id;node->setData(0,o.id);node->kind=o.kind;node->label=o.kind=="isa"?"ISA":o.kind=="attribute"?attributeLabel(o):o.name;node->theme=theme;node->uml=uml;node->key=o.key;node->weak=o.kind=="entity"?model.weak(o.id):model.weak(o.owner);node->identifying=o.identifying;
   QFont objectFont=diagramFont;objectFont.setBold(o.kind=="entity");double width=qMax(130.,QFontMetricsF(objectFont).horizontalAdvance(node->label)+40);
   if(o.kind=="entity"&&uml){for(auto a:model.inheritedAttributes(o.id))width=qMax(width,conceptualAttributeWidth(a,true)+56);for(auto a:model.attributes(o.id))width=qMax(width,conceptualAttributeWidth(a,false)+56);node->size={width,qMax(78.,48.+(model.attributes(o.id).size()+model.inheritedAttributes(o.id).size())*30.)};}
   else if(o.kind=="entity")node->size={qMax(150.,width),64};else if(o.kind=="attribute")node->size={width,44};else if(o.kind=="isa")node->size={64,50};else node->size={uml?width:width+30,uml?34.:86.};
   scene->addItem(node);node->setPos(uml?o.uml:o.chen);nodes[o.id]=node;
   node->begin=[this](Node*){dragBefore=model;};
   node->moving=[this](Node*n){if(rebuilding)return;if(auto o=model.get(n->id)){QPointF old=uml?o->uml:o->chen;QPointF delta=n->pos()-old;(uml?o->uml:o->chen)=n->pos();if(!uml&&o->kind=="entity"){for(auto &a:model.objects)if(a.kind=="attribute"&&a.owner==o->id){a.chen+=delta;if(auto an=nodes.value(a.id)){rebuilding=true;an->setPos(a.chen);rebuilding=false;}}}links();}};
   node->end=[this](Node*){if(model.json()==dragBefore.json())return;Model before=dragBefore,after=model;QTimer::singleShot(0,this,[this,before,after]{history.push(new Snapshot(before,after,[this](const Model&m){model=m;refresh();},t("Sposta oggetti","Move objects")));});};
   if(o.kind=="entity"&&uml){int i=0;for(const auto &a:model.attributes(o.id)){auto row=new Node;row->setToolTip(objectHelp(a));row->kind="attribute";row->id=a.id;row->setData(0,a.id);row->uml=true;row->theme=theme;configureAttributeLabel(row,a,false);row->setParentItem(node);row->setFlag(QGraphicsItem::ItemIsMovable,false);row->size={width-16,28};row->setPos(0,-node->size.height()/2+57+i*30);nodes[a.id]=row;++i;}for(const auto &a:model.inheritedAttributes(o.id)){auto row=new Node;row->setFlags(QGraphicsItem::GraphicsItemFlags());row->inherited=true;row->uml=true;row->kind="attribute";row->theme=theme;configureAttributeLabel(row,a,true);row->setToolTip(inheritedTip(a));row->setParentItem(node);row->size={width-16,28};row->setPos(0,-node->size.height()/2+57+i++*30);}}
  }
  renderNotes();links();scene->setSceneRect(-200000,-200000,400000,400000);normalizeSelection();for(auto id:selectionIds)if(nodes.contains(id))nodes[id]->setSelected(true);tree->expandAll();rebuilding=false;syncTree();inspector();updateStatus();title();
 }
#include "conceptual_links.h"
 void fit(){auto r=scene->itemsBoundingRect().adjusted(-50,-50,50,50);if(!r.isEmpty()){view->fitInView(r,Qt::KeepAspectRatio);if(view->transform().m11()>1.5){view->resetTransform();view->scale(1.5,1.5);view->centerOn(r.center());}}updateStatus();}
 void note(QString text){auto l=new QLabel(text);l->setWordWrap(true);l->setTextFormat(Qt::PlainText);form->addRow(l);}
 QLineEdit *field(QString label,QString value){auto e=new QLineEdit(value);e->setMaxLength(120);e->setAccessibleName(label);helpWidget(e,helpText(label));form->addRow(label,e);return e;}
 void inspector(){
  updateRoleBadges();
  while(form->rowCount())form->removeRow(0);
  if(selectionIds.size()==1&&model.note(selection)){noteInspector();return;}
  if(logicalView){logicalInspector();return;}
  if(selectionIds.size()>1){note(t("SELEZIONE MULTIPLA","MULTIPLE SELECTION"));note(QString::number(selectionIds.size())+t(" oggetti selezionati. Trascina un elemento del gruppo per spostarli insieme.\n\nCtrl+clic sinistro aggiunge o rimuove. Ctrl+clic destro aggiunge e mostra le azioni.\n\nL’allineamento usa come riferimento l’ultimo elemento cliccato."," selected objects. Drag a group member to move them together.\n\nCtrl+left-click adds or removes. Ctrl+right-click adds and shows actions.\n\nAlignment uses the last clicked element as its reference."));return;}
  auto o=model.get(selection);
  if(!o){note(t("Seleziona un oggetto nel disegno o nell’elenco.\n\nCrea un’entità e aggiungi i suoi attributi. Nessun tipo SQL è richiesto in questa fase.","Select an object in the diagram or outline.\n\nCreate an entity and add its attributes. No SQL type is required at this stage."));return;}
  const quint64 epoch=documentEpoch;const QString id=o->id;QString kind=o->kind;note(kind=="entity"?t("ENTITÀ","ENTITY"):kind=="attribute"?t("ATTRIBUTO","ATTRIBUTE"):kind=="isa"?"ISA":t("ASSOCIAZIONE BINARIA","BINARY RELATIONSHIP"));
  auto name=field(t("Nome","Name"),o->name);QCheckBox *key=nullptr,*identifying=nullptr;QComboBox *dataType=nullptr,*ca=nullptr,*cb=nullptr;QLineEdit *ra=nullptr,*rb=nullptr;
  if(kind=="entity"){
   QStringList ids;for(auto a:model.attributes(id))if(a.key)ids<<a.name;for(const auto &r:model.objects)if(r.kind=="relation"&&r.identifying&&r.b==id)if(auto parent=model.get(r.a))ids.prepend(parent->name);
   note((model.weak(id)?t("Entità debole\nChiave completa: ","Weak entity\nComplete key: "):t("Chiave completa: ","Complete key: "))+keySummary(id));
   if(!model.ancestors(id).isEmpty()){note(t("Chiave ereditata dalla superclasse. Gli eventuali attributi locali marcati come chiave diventano una chiave alternativa nel logico.","Key inherited from the superclass. Any local attributes marked as key become an alternate key in the logical model."));inheritanceInspector(id);}
   for(const auto &r:model.objects)if(r.kind=="isa"&&r.a==id)if(auto parent=model.get(r.b))note(t("Sottoclasse di: ","Subclass of: ")+parent->name);
  }else if(kind=="attribute"){
   dataType=typeCombo(o->dataType);form->addRow(t("Tipo di dato","Data type"),dataType);
   auto owner=model.get(o->owner);note(t("Entità: ","Entity: ")+(owner?owner->name:QString()));
   key=new QCheckBox(model.weak(o->owner)?t("Chiave parziale","Partial key"):t("Parte della chiave","Part of key"));helpWidget(key,t("Marca gli attributi che insieme distinguono ogni istanza dell’entità. Per un’entità debole sono i componenti locali dell’identificatore esterno. Conferma con Applica modifiche.","Mark attributes that together distinguish every entity instance. For a weak entity these are the local components of the external identifier. Confirm with Apply changes."));key->setChecked(o->key);form->addRow(key);note(t("Più attributi marcati formano un unico identificatore composto.","Multiple marked attributes form one composite identifier."));
  }else{
   auto a=model.get(o->a),b=model.get(o->b);note((kind=="isa"?t("Sottoclasse: ","Subclass: "):"A: ")+(a?a->name:QString())+"\n"+(kind=="isa"?t("Superclasse: ","Superclass: "):"B: ")+(b?b->name:QString()));
   if(kind=="relation"){
    ra=field(t("Nome ruolo A","Role A name"),o->roleA);rb=field(t("Nome ruolo B","Role B name"),o->roleB);
    ca=new QComboBox;cb=new QComboBox;QStringList cards={"0,1","1,1","0,N","1,N"};for(const auto &c:cards){ca->addItem("("+c+")",c);cb->addItem("("+c+")",c);}helpWidget(ca,t("Per ogni istanza di A: numero minimo e massimo di istanze B collegate. (0,1): al massimo una; (1,1): esattamente una; (0,N): zero o più; (1,N): una o più.","For each A instance: minimum and maximum linked B instances. (0,1): at most one; (1,1): exactly one; (0,N): zero or more; (1,N): one or more."));helpWidget(cb,t("Per ogni istanza di B: numero minimo e massimo di istanze A collegate. Se A identifica B, questa partecipazione è obbligatoriamente (1,1).","For each B instance: minimum and maximum linked A instances. When A identifies B, this participation must be (1,1)."));ca->setCurrentIndex(cards.indexOf(o->cardA));cb->setCurrentIndex(cards.indexOf(o->cardB));form->addRow(t("Per ogni A, quante B?","For each A, how many B?"),ca);form->addRow(t("Per ogni B, quante A?","For each B, how many A?"),cb);
    identifying=new QCheckBox(t("A identifica B (entità debole)","A identifies B (weak entity)"));helpWidget(identifying,t("L’identificatore di B include quello di A insieme ai componenti della chiave parziale di B. Ogni B deve appartenere a un solo A. Conferma con Applica modifiche.","B’s identifier includes A’s identifier together with B’s partial keys. Each B must belong to exactly one A. Confirm with Apply changes."));identifying->setChecked(o->identifying);form->addRow(identifying);if(o->identifying)cb->setEnabled(false);connect(identifying,&QCheckBox::toggled,this,[cb](bool value){cb->setEnabled(!value);if(value)cb->setCurrentIndex(1);});
    note(t("Le cardinalità descrivono la partecipazione. Nella vista UML-like sono collocate all’estremità opposta.","Multiplicities describe participation. UML-like displays them at the opposite endpoint."));
   }else note(t("ISA semplice: identità e attributi della superclasse sono ereditati. Non vengono duplicati nel modello.","Simple ISA: superclass identity and attributes are inherited. They are not duplicated in the model."));
  }
  auto apply=new QPushButton(t("Applica modifiche","Apply changes"));apply->setObjectName("applyProperties");helpWidget(apply,helpText(apply->text()));form->addRow(apply);
  connect(apply,&QPushButton::clicked,this,[=]{
   QString text=name->text().trimmed();if(text.isEmpty()){QMessageBox::warning(this,t("Nome richiesto","Name required"),t("Inserisci un nome.","Enter a name."));return;}
   QString chosenType=dataType?dataType->currentData().toString():QString();bool k=key&&key->isChecked(),ident=identifying&&identifying->isChecked();QString aCard=ca?ca->currentData().toString():"",bCard=cb?cb->currentData().toString():"",rA=ra?ra->text().trimmed():"",rB=rb?rb->text().trimmed():"";
   if(ident){auto r=model.get(id);if(r&&r->a==r->b){QMessageBox::warning(this,t("Identificazione","Identification"),t("Una relazione ricorsiva non può identificare la stessa entità.","A recursive relationship cannot identify the same entity."));return;}}
   QTimer::singleShot(0,this,[=]{if(epoch!=documentEpoch)return;change(t("Modifica proprietà","Edit properties"),[=](Model &m){auto r=m.get(id);if(!r)return;r->name=text;if(kind=="attribute"){r->key=k;r->dataType=chosenType;}if(kind=="relation"){r->cardA=aCard;r->cardB=ident?"1,1":bCard;r->roleA=rA;r->roleB=rB;r->identifying=ident;}});});
  });
  if(kind=="entity"){auto add=new QPushButton(t("Aggiungi attributo","Add attribute"));helpWidget(add,helpText(t("Attributo","Attribute")));form->addRow(add);connect(add,&QPushButton::clicked,this,[this]{addAttribute();});}
 }
 QPointF center()const{return view->mapToScene(view->viewport()->rect().center());}
 QString uniqueName(QString prefix,QString kind,QString owner={})const{int i=1;while(true){QString name=prefix+QString::number(i);bool exists=false;for(auto o:model.objects)if(o.kind==kind&&o.owner==owner&&o.name==name)exists=true;if(!exists)return name;++i;}}
 void addEntity(){if(logicalView)return;addEntityAt(center()+QPointF((model.objects.size()%4)*25,0));}
 void addEntityAt(QPointF point){if(!std::isfinite(point.x())||!std::isfinite(point.y()))return;point={qBound(-99000.,point.x(),99000.),qBound(-99000.,point.y(),99000.)};Object o;o.id=newId();o.kind="entity";o.name=uniqueName(t("Entita","Entity"),"entity");o.chen=o.uml=point;selection=o.id;selectionIds={o.id};change(t("Crea entità","Create entity"),[=](Model&m){m.objects<<o;});}
 void addAttribute(){if(logicalView)return;auto parent=model.get(selection);if(parent&&parent->kind=="attribute")parent=model.get(parent->owner);if(!parent||parent->kind!="entity"){QMessageBox::information(this,t("Attributo","Attribute"),t("Seleziona prima un’entità.","Select an entity first."));return;}
  Object a;a.id=newId();a.kind="attribute";a.owner=parent->id;a.name=uniqueName(t("Attributo","Attribute"),"attribute",a.owner);int n=model.attributes(a.owner).size();a.chen=parent->chen+QPointF((n%3-1)*170,-130-(n/3)*85);a.uml=a.chen;selection=a.id;selectionIds={a.id};change(t("Crea attributo","Create attribute"),[=](Model&m){m.objects<<a;});}
 void addLink(bool isa,QString first={},QString second={}){if(logicalView)return;
  QVector<Object> entities;for(const auto &o:model.objects)if(o.kind=="entity")entities<<o;if(entities.isEmpty()||(isa&&entities.size()<2)){QMessageBox::information(this,"Database Schema Studio",t("Crea prima le entità da collegare.","Create the entities first."));return;}
  QDialog d(this);d.setWindowTitle(isa?t("Nuova ISA","New ISA"):t("Nuova associazione","New relationship"));QFormLayout f(&d);QLineEdit name(isa?"ISA":t("Associazione","Relationship"));name.setMaxLength(120);QComboBox a,b;for(auto o:entities){a.addItem(o.name,o.id);b.addItem(o.name,o.id);}if(entities.size()>1)b.setCurrentIndex(1);int index=a.findData(first.isEmpty()?selection:first);if(index>=0)a.setCurrentIndex(index);if(!second.isEmpty())b.setCurrentIndex(b.findData(second));else if(entities.size()>1&&a.currentIndex()==b.currentIndex())b.setCurrentIndex((a.currentIndex()+1)%entities.size());f.addRow(t("Nome","Name"),&name);f.addRow(isa?t("Sottoclasse","Subclass"):t("Entità A","Entity A"),&a);f.addRow(isa?t("Superclasse","Superclass"):t("Entità B","Entity B"),&b);QDialogButtonBox buttons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);f.addRow(&buttons);connect(&buttons,&QDialogButtonBox::accepted,&d,&QDialog::accept);connect(&buttons,&QDialogButtonBox::rejected,&d,&QDialog::reject);if(d.exec()!=QDialog::Accepted)return;
  Object r;r.id=newId();r.kind=isa?"isa":"relation";r.name=name.text().trimmed();if(r.name.isEmpty())r.name=isa?"ISA":t("Associazione","Relationship");r.a=a.currentData().toString();r.b=b.currentData().toString();auto ea=model.get(r.a),eb=model.get(r.b);r.chen=(ea->chen+eb->chen)/2;r.uml=(ea->uml+eb->uml)/2;if(r.a==r.b){r.chen+=QPointF(250,0);r.uml+=QPointF(250,0);r.roleA=t("ruolo1","role1");r.roleB=t("ruolo2","role2");}
  if(isa){Model trial=model;trial.objects<<r;auto errors=trial.issues(en);bool fail=r.a==r.b;for(const auto &s:errors)fail|=s.contains("ISA",Qt::CaseInsensitive)||s.contains("isa",Qt::CaseSensitive);if(fail){QMessageBox::warning(this,"ISA",t("ISA non valida: evita cicli e più superclassi per la stessa entità.","Invalid ISA: avoid cycles and multiple superclasses."));return;}}
  selection=r.id;selectionIds={r.id};change(t("Crea collegamento","Create link"),[=](Model&m){m.objects<<r;});
 }
 void deleteSelected(bool fromContext=false){if(auto f=QApplication::focusWidget();!fromContext&&(qobject_cast<QPlainTextEdit*>(f)||qobject_cast<QTextEdit*>(f)||qobject_cast<QAbstractSpinBox*>(f)))return;
  if(!fromContext&&qobject_cast<QLineEdit*>(QApplication::focusWidget()))return;
  auto ids=selectionIds;if(ids.isEmpty())return;selection.clear();selectionIds.clear();change(t("Elimina oggetti","Delete objects"),[=](Model&m){for(auto id:ids){m.notes.erase(std::remove_if(m.notes.begin(),m.notes.end(),[&](const DiagramNote &n){return n.id==id;}),m.notes.end());if(!logicalView)m.remove(id);}});
 }
 void validate(){if(logicalView){auto issues=logicalCache.issues(en);QMessageBox::information(this,t("Verifica logica","Logical validation"),issues.isEmpty()?(logicalStale(model)?t("Schema logico coerente, ma il concettuale è cambiato: aggiorna la traduzione.","Logical schema is consistent, but the conceptual model has changed: update the mapping."):t("Tabelle, chiavi e riferimenti FK sono coerenti. Consulta il rapporto per i vincoli concettuali che richiedono controlli aggiuntivi.","Tables, keys and FK references are consistent. Consult the report for conceptual constraints requiring additional checks.")):issues.join("\n"));return;}auto issues=model.issues(en);QMessageBox::information(this,t("Verifica concettuale","Conceptual validation"),issues.isEmpty()?t("Nessun problema rilevato.\n\nPuoi salvare il modello, esportare il diagramma o generare il modello logico.","No issues found.\n\nYou can save the model, export the diagram or generate the logical model."):issues.join("\n"));}
 bool maybeSave(){if(history.isClean())return true;auto answer=QMessageBox::question(this,t("Modifiche non salvate","Unsaved changes"),t("Salvare il progetto prima di continuare?","Save the project before continuing?"),QMessageBox::Save|QMessageBox::Discard|QMessageBox::Cancel,QMessageBox::Save);if(answer!=QMessageBox::Save&&answer!=QMessageBox::Discard)return false;if(answer==QMessageBox::Save)return save(false);return true;}
 bool save(bool as){QString path=filePath;if(as||path.isEmpty())path=QFileDialog::getSaveFileName(this,t("Salva progetto","Save project"),path.isEmpty()?storageDirectory()+"/Progetti/"+t("Progetto.schema","Project.schema"):path,"Database Schema Studio (*.schema)");if(path.isEmpty())return false;if(!path.endsWith(".schema",Qt::CaseInsensitive))path+=".schema";QString error;if(!saveWithBackup(path,model,error)){QMessageBox::critical(this,t("Salvataggio non riuscito","Save failed"),error+"\n\n"+t("Il progetto rimane aperto. Prova una cartella in cui l’applicazione può scrivere, ad esempio la cartella Progetti predefinita.","The project remains open. Try a folder this application can write to, such as the default Projects folder."));return false;}filePath=path;history.setClean();clearRecovery();title();return true;}
 bool openPath(QString path){Model m;QString error;if(!loadModel(path,m,error)){QMessageBox::critical(this,t("Apertura non riuscita","Open failed"),localizedError(error,en));return false;}++documentEpoch;model=m;filePath=path;selection.clear();selectionIds.clear();history.clear();refresh();fit();return true;}
 void openDialog(){if(!maybeSave())return;auto path=QFileDialog::getOpenFileName(this,t("Apri progetto","Open project"),filePath,"Database Schema Studio (*.schema)");if(!path.isEmpty())openPath(path);}

 void appearance(){
  QDialog d(this);d.setWindowTitle(t("Aspetto e lingua","Appearance and language"));QFormLayout f(&d);QComboBox modes,palettes,languages;QSpinBox fonts;modes.addItems({t("Sistema","System"),t("Giorno","Day"),t("Notte","Night")});modes.setCurrentIndex(mode);palettes.addItems({t("Neutra","Neutral"),t("Blu","Blue"),t("Verde petrolio","Teal"),t("Contrasto elevato","High contrast")});palettes.setCurrentIndex(preset);languages.addItems({"Italiano","English"});languages.setCurrentIndex(en?1:0);fonts.setRange(9,20);fonts.setValue(fontSize);fonts.setSuffix(" pt");f.addRow(t("Modalità","Mode"),&modes);f.addRow(t("Palette","Palette"),&palettes);f.addRow(t("Lingua","Language"),&languages);f.addRow(t("Testo interfaccia","Interface text"),&fonts);
  QColor selected=custom;QPushButton color(t("Scegli colore accento…","Choose accent color…")),reset(t("Ripristina accento","Reset accent"));f.addRow(&color);f.addRow(&reset);QLabel preview;preview.setWordWrap(true);preview.setMinimumHeight(80);f.addRow(&preview);
  auto update=[&]{bool dark=modes.currentIndex()==2||(modes.currentIndex()==0&&systemDark());auto base=Theme::make(dark,palettes.currentIndex());bool safe=!selected.isValid()||(Theme::contrast(selected,base.paper)>=3&&Theme::contrast(selected,base.surface)>=3);auto th=Theme::make(dark,palettes.currentIndex(),selected);preview.setText((safe?t("Colori · Entità, attributi, cardinalità","Colors · Entities, attributes, multiplicities"):t("Contrasto insufficiente: scegli un altro accento o ripristina.","Insufficient contrast: choose another accent or reset."))+QString("\n%1:1").arg(Theme::contrast(th.ink,th.surface),0,'f',1));preview.setStyleSheet(QString("padding:12px;background:%1;color:%2;border:2px solid %3").arg(th.surface.name(),th.ink.name(),th.accent.name()));};
  connect(&color,&QPushButton::clicked,&d,[&]{auto c=QColorDialog::getColor(selected.isValid()?selected:theme.accent,&d);if(c.isValid())selected=c;update();});connect(&reset,&QPushButton::clicked,&d,[&]{selected=QColor();update();});connect(&modes,QOverload<int>::of(&QComboBox::currentIndexChanged),&d,[&]{update();});connect(&palettes,QOverload<int>::of(&QComboBox::currentIndexChanged),&d,[&]{update();});QDialogButtonBox buttons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);f.addRow(&buttons);connect(&buttons,&QDialogButtonBox::rejected,&d,&QDialog::reject);connect(&buttons,&QDialogButtonBox::accepted,&d,[&]{auto th=Theme::make(modes.currentIndex()==2||(modes.currentIndex()==0&&systemDark()),palettes.currentIndex());if(selected.isValid()&&(Theme::contrast(selected,th.paper)<3||Theme::contrast(selected,th.surface)<3)){QMessageBox::warning(&d,t("Contrasto","Contrast"),t("Scegli un colore con contrasto sufficiente oppure ripristina l’accento.","Choose a color with sufficient contrast or reset the accent."));return;}d.accept();});update();if(d.exec()!=QDialog::Accepted)return;
  mode=modes.currentIndex();preset=palettes.currentIndex();en=languages.currentIndex()==1;fontSize=fonts.value();custom=selected;storePreferences();applyTheme();buildMenus();refresh();
 }
 void storePreferences(){if(testMode)return;prefs.setValue("mode",mode);prefs.setValue("palette",preset);prefs.setValue("language",en?"en":"it");prefs.setValue("fontSize",fontSize);prefs.setValue("accent",custom.isValid()?custom.name():"");prefs.setValue("showNotes",showNotes);prefs.setValue("showTypes",showTypes);prefs.setValue("umlPkLabels",umlPkLabels);prefs.setValue("gridVisible",view->gridVisible);prefs.setValue("snapEnabled",view->snapEnabled);prefs.setValue("geometry",saveGeometry());prefs.setValue("layout",saveState());prefs.sync();}
 bool exportPdf(QString path,QPageSize::PageSizeId paper,bool landscape,bool mono,QString &error){
  if(scene->items().isEmpty()){error=t("Il diagramma è vuoto.","The diagram is empty.");return false;}
  QSaveFile file(path);if(!file.open(QIODevice::WriteOnly)){error=file.errorString();return false;}
  bool painted=false;
  {QPdfWriter pdf(&file);pdf.setTitle("Database Schema Studio — "+(filePath.isEmpty()?t("Modello concettuale","Conceptual model"):QFileInfo(filePath).baseName()));pdf.setCreator("Database Schema Studio 0.9.7");pdf.setPageSize(QPageSize(paper));pdf.setPageOrientation(landscape?QPageLayout::Landscape:QPageLayout::Portrait);pdf.setPageMargins(QMarginsF(12,12,12,12),QPageLayout::Millimeter);pdf.setResolution(144);
   printing=true;updateRoleBadges();Theme original=theme;theme=Theme::make(false,mono?3:preset);rebuilding=true;scene->clearSelection();setSceneNodeTheme();links();rebuilding=false;QPainter painter;if(painter.begin(&pdf)){painter.setRenderHints(QPainter::Antialiasing|QPainter::TextAntialiasing);auto source=scene->itemsBoundingRect().adjusted(-15,-15,15,15);scene->render(&painter,QRectF(0,0,pdf.width(),pdf.height()),source,Qt::KeepAspectRatio);painted=painter.end();}theme=original;setSceneNodeTheme();links();rebuilding=true;for(auto id:selectionIds)if(nodes.contains(id))nodes[id]->setSelected(true);rebuilding=false;printing=false;updateRoleBadges();scene->update();
  }
  if(!painted){file.cancelWriting();error=t("Creazione PDF non riuscita.","PDF rendering failed.");return false;}if(!file.commit()){error=file.errorString();return false;}return true;
 }
 void pdfDialog(){
  if(model.objects.isEmpty()&&model.notes.isEmpty()){QMessageBox::information(this,"PDF",t("Il diagramma è vuoto.","The diagram is empty."));return;}
  QDialog d(this);d.setWindowTitle(t("Esporta PDF","Export PDF"));QFormLayout f(&d);QComboBox paper,orientation;paper.addItems({"A4","A3"});orientation.addItems({t("Orizzontale","Landscape"),t("Verticale","Portrait")});QCheckBox mono(t("Contrasto elevato per stampa","High contrast for printing"));mono.setChecked(true);f.addRow(t("Formato","Page size"),&paper);f.addRow(t("Orientamento","Orientation"),&orientation);f.addRow(&mono);QCheckBox includeNotes(t("Includi note","Include notes"));includeNotes.setChecked(showNotes);includeNotes.setEnabled(!model.notes.isEmpty());f.addRow(&includeNotes);QLabel note(t("Intero diagramma su una pagina, con profilo chiaro.\nPer modelli ampi scegli A3.","Entire diagram on one page, with a light print profile.\nChoose A3 for large models."));note.setWordWrap(true);f.addRow(&note);QDialogButtonBox buttons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);f.addRow(&buttons);connect(&buttons,&QDialogButtonBox::accepted,&d,&QDialog::accept);connect(&buttons,&QDialogButtonBox::rejected,&d,&QDialog::reject);if(d.exec()!=QDialog::Accepted)return;
  auto path=QFileDialog::getSaveFileName(this,t("Esporta PDF","Export PDF"),storageDirectory()+"/Progetti/"+t("Diagramma.pdf","Diagram.pdf"),"PDF (*.pdf)");if(path.isEmpty())return;if(!path.endsWith(".pdf",Qt::CaseInsensitive))path+=".pdf";bool previousNotes=showNotes;showNotes=includeNotes.isChecked();refresh();QString error;bool exported=exportPdf(path,paper.currentIndex()==0?QPageSize::A4:QPageSize::A3,orientation.currentIndex()==0,mono.isChecked(),error);showNotes=previousNotes;refresh();if(!exported)QMessageBox::critical(this,"PDF",error);else status->setText(t("PDF esportato: ","PDF exported: ")+path);
 }
 void closeEvent(QCloseEvent *e)override{if(!testMode&&!maybeSave()){e->ignore();return;}storePreferences();if(!testMode)clearRecovery();e->accept();}
};

#include "../tests/ui/interaction_tests.h"
#include "../tests/ui/branding_tests.h"
#include "../tests/ui/guidance_tests.h"
#include "../tests/ui/logical_tests.h"
#include "../tests/ui/rise_tests.h"
#include "../tests/ui/modern_tests.h"
#include "../tests/ui/type_tests.h"
#include "../tests/ui/completion_tests.h"
#include "../tests/ui/crowfoot_tests.h"
#include "../tests/ui/uml_book_tests.h"
#include "../tests/ui/physical_tests.h"
#include "../tests/ui/guide_assets.h"
#include "../tests/ui/arrangement_tests.h"
#include "../tests/ui/learning_tests.h"
#include "../tests/ui/notes_tests.h"
#include "../tests/ui/example_documentation_tests.h"
int selfTest(Window &w,QString output){
 fprintf(stdout,"Output: %s, current: %s\n",qPrintable(output),qPrintable(QDir::currentPath()));
 QDir().mkpath(output);QJsonArray tests;int failures=0;
 auto check=[&](QString name,bool ok){tests.append(QJsonObject{{"test",name},{"passed",ok}});fprintf(stdout,"%s: %s\n",ok?"PASS":"FAIL",qPrintable(name));fflush(stdout);if(!ok)++failures;};
 QString error;Model original=Model::example(),parsed;w.model=original;w.refresh();w.history.clear();auto bytes=QJsonDocument(original.json()).toJson();check("JSON roundtrip",Model::parse(bytes,parsed,error)&&parsed.json()==original.json());check("Example validation",original.issues().isEmpty());
 Model kept=original;check("Malformed JSON rejected atomically",!Model::parse("{broken",kept,error)&&kept.json()==original.json());auto invalid=original.json();auto arr=invalid["objects"].toArray();arr.append(arr.first());invalid["objects"]=arr;check("Duplicate IDs rejected",!Model::parse(QJsonDocument(invalid).toJson(),kept,error));
 auto bad=original;bad.get("numordine")->owner="missing";check("Dangling owner rejected",!Model::parse(QJsonDocument(bad.json()).toJson(),kept,error));
 check("Atomic save",saveModel(output+"/Ordini.schema",original,error));check("Read saved model",loadModel(output+"/Ordini.schema",parsed,error)&&parsed.json()==original.json());
 auto deleted=original;deleted.remove("ordine");check("Cascade removes attributes and relationships",!deleted.get("numordine")&&!deleted.get("data")&&!deleted.get("contiene")&&deleted.get("riga"));
 auto cycle=original;Object isa;isa.id="isa1";isa.kind="isa";isa.name="ISA";isa.a="ordine";isa.b="riga";cycle.objects<<isa;isa.id="isa2";std::swap(isa.a,isa.b);cycle.objects<<isa;check("ISA cycle detected",cycle.issues().join(" ").contains("Ciclo"));
 auto noKey=original;noKey.get("numordine")->key=false;check("Missing identifier detected",!noKey.issues().isEmpty());
 w.change("Rename",[](Model&m){m.get("ordine")->name="OrdineTest";});check("Edit model",w.model.get("ordine")->name=="OrdineTest");w.history.undo();check("Undo",w.model.json()==original.json());w.history.redo();check("Redo",w.model.get("ordine")->name=="OrdineTest");w.history.undo();
 for(bool dark:{false,true})for(int p=0;p<4;++p){auto t=Theme::make(dark,p);check(QString("Theme contrast %1 %2").arg(dark).arg(p),Theme::contrast(t.ink,t.paper)>=4.5&&Theme::contrast(t.ink,t.surface)>=4.5&&Theme::contrast(t.ink,t.selected)>=4.5&&Theme::contrast(t.muted,t.surface)>=4.5&&Theme::contrast(t.line,t.paper)>=3&&Theme::contrast(t.accent,t.surface)>=3&&Theme::contrast(Theme::textOn(t.accent),t.accent)>=4.5);}
 w.mode=1;w.applyTheme();w.refresh();w.fit();w.show();qApp->processEvents();w.grab().save(output+"/chen.png");check("Chen PDF",w.exportPdf(output+"/Ordini-Chen.pdf",QPageSize::A4,true,true,error));
 w.notationBox->setCurrentIndex(1);check("View preserves conceptual data",w.model.json()==original.json());check("UML attributes are independently selectable",w.nodes.contains("numordine")&&w.nodes["numordine"]->flags().testFlag(QGraphicsItem::ItemIsSelectable));w.fit();qApp->processEvents();w.grab().save(output+"/uml.png");check("UML PDF",w.exportPdf(output+"/Ordini-UML.pdf",QPageSize::A3,true,false,error));
 w.en=true;w.mode=2;w.applyTheme();w.buildMenus();w.refresh();w.fit();qApp->processEvents();w.grab().save(output+"/night.png");auto priorTheme=w.theme;check("Night to light PDF",w.exportPdf(output+"/Ordini-night-export.pdf",QPageSize::A4,true,true,error));check("PDF keeps live theme and model",w.theme.paper==priorTheme.paper&&w.model.json()==original.json());
 w.uml=false;w.refresh();auto entityNode=w.nodes["ordine"];entityNode->begin(entityNode);auto oldAttribute=w.model.get("numordine")->chen;entityNode->setPos(entityNode->pos()+QPointF(90,50));check("Moving entity moves Chen attributes",w.model.get("numordine")->chen==oldAttribute+QPointF(90,50));check("Layouts are independent",w.model.get("ordine")->uml==original.get("ordine")->uml);entityNode->end(entityNode);qApp->processEvents();w.history.undo();check("Drag undo restores positions",w.model.json()==original.json());
 w.selection="ordine";w.addAttribute();auto added=w.selection;check("New attribute has separate identity",w.model.get(added)&&w.model.get(added)->owner=="ordine"&&!w.model.get(added)->key);w.history.undo();check("Undo attribute creation",!w.model.get(added)&&w.model.json()==original.json());
 Model draft=original;draft.get("contiene")->identifying=false;draft.get("contiene")->a="riga";draft.get("contiene")->b="riga";draft.get("contiene")->roleA="parent";draft.get("contiene")->roleB="child";w.model=draft;w.refresh();check("Recursive relationship renders",w.connectors.size()>4);w.model=original;w.history.clear();w.refresh();
 check("Generated PDF signature",[&]{QFile f(output+"/Ordini-Chen.pdf");return f.open(QIODevice::ReadOnly)&&f.read(5)=="%PDF-";}());
 interactionTests(w,output,check);
 brandingTests(w,output,check);
 guidanceTests(w,output,check);
 logicalTests(w,output,check);
 riseTests(w,output,check);
 modernTests(w,output,check);
 typeTests(w,output,check);
 completionTests(w,output,check);
 crowfootTests(w,output,check);
 umlBookTests(w,output,check);
 physicalTests(w,output,check);
 arrangementTests(w,output,check);
 learningTests(w,output,check);
 notesTests(w,output,check);
 exampleDocumentationTests(w,output,check);
 QFile report(output+"/test-results.json");if(!report.open(QIODevice::WriteOnly)){fprintf(stderr,"Report: %s\n",qPrintable(report.errorString()));return 2;}report.write(QJsonDocument(QJsonObject{{"failures",failures},{"tests",tests},{"qt",qVersion()}}).toJson());return failures?1:0;
}
int main(int argc,char **argv){
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
 QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif
 QGuiApplication::setHighDpiScaleFactorRoundingPolicy(Qt::HighDpiScaleFactorRoundingPolicy::PassThrough);
 QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
 QApplication app(argc,argv);app.setApplicationName("DbSchemaStudio");app.setApplicationDisplayName("Database Schema Studio");app.setApplicationVersion("0.9.7");QIcon applicationIcon;for(int size:{16,20,24,32,40,48,64,128,256})applicationIcon.addFile(QString(":/icons/app-%1.png").arg(size),QSize(size,size));app.setWindowIcon(applicationIcon);app.setOrganizationName("SchemaStudio");app.setStyle(new EducationalStyle);app.installEventFilter(new DialogSizer(&app));QSettings initial(storageDirectory()+"/preferences.ini",QSettings::IniFormat);bool startupEnglish=initial.value("language","it")=="en";setUiLanguage(startupEnglish);
 bool sqlTesting=app.arguments().contains("--sql-display-test");bool guideTesting=app.arguments().contains("--guide-assets");bool visualTesting=app.arguments().contains("--display-test");bool testing=sqlTesting||guideTesting||app.arguments().contains("--self-test")||visualTesting;if(!testing&&!QDir().mkpath(storageDirectory()+"/Progetti")){QMessageBox::critical(nullptr,"Database Schema Studio",(startupEnglish?"Cannot create data folder:\n":"Impossibile creare la cartella dati:\n")+storageDirectory());return 2;}
 QLockFile lock(storageDirectory()+"/session.lock");if(!testing&&!lock.tryLock(100)){QMessageBox::information(nullptr,"Database Schema Studio",lock.error()==QLockFile::LockFailedError?(startupEnglish?"Database Schema Studio is already running.":"Database Schema Studio è già aperto."):(startupEnglish?"Cannot write to the data folder.":"La cartella dati non è scrivibile."));return 2;}
 if(!testing)initializeDiagnostics(storageDirectory());
 Window w(testing);if(sqlTesting)return sqlDisplayTest(w,app.arguments().value(app.arguments().indexOf("--sql-display-test")+1));if(guideTesting)return guideAssets(w,app.arguments().value(app.arguments().indexOf("--guide-assets")+1));if(visualTesting)return displayTest(w,app.arguments().value(app.arguments().indexOf("--display-test")+1));if(testing){QString out=app.arguments().value(app.arguments().indexOf("--self-test")+1,QDir::currentPath()+"/test-results");return selfTest(w,out);}
 w.show();if(app.arguments().contains("--smoke-test"))QTimer::singleShot(1800,&w,[&]{w.close();});else if(app.arguments().size()>1&&app.arguments()[1].endsWith(".schema",Qt::CaseInsensitive))w.openPath(app.arguments()[1]);else QTimer::singleShot(100,&w,[&]{w.recover();});
 QObject::connect(app.styleHints(),&QStyleHints::colorSchemeChanged,&w,[&]{if(w.mode==0){w.applyTheme();w.setSceneNodeTheme();w.buildMenus();w.links();w.updateStatus();w.scene->update();}});
 return app.exec();
}
