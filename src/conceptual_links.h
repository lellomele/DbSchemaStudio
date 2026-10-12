// Included within Window. Conceptual diagram connectors only.
 QPointF boundary(Node*n,QPointF toward){QPointF c=n->scenePos(),d=toward-c;double w=n->size.width()/2,h=n->size.height()/2;if(qFuzzyIsNull(d.x())&&qFuzzyIsNull(d.y()))return c;double scale=1/qMax(std::abs(d.x())/w,std::abs(d.y())/h);return c+d*scale;}
 QString currentLinkId;
 void linkPath(const QPainterPath &p){auto item=new LinkItem(p);item->setPen(QPen(theme.line,1.5));item->setData(0,currentLinkId);if(auto o=model.get(currentLinkId))item->setToolTip(objectHelp(*o));scene->addItem(item);item->setZValue(-2);connectors<<item;}
 void linkText(QString text,QPointF pos){auto item=new QGraphicsSimpleTextItem(text);item->setData(0,currentLinkId);if(auto o=model.get(currentLinkId))item->setToolTip(objectHelp(*o));QFont f("Segoe UI");f.setPixelSize(12);item->setFont(f);item->setBrush(theme.ink);item->setPos(pos-QPointF(item->boundingRect().width()/2,item->boundingRect().height()/2));item->setZValue(1);scene->addItem(item);connectors<<item;}
 QString cardinal(QString s)const{return uml?(s=="1,1"?QString("1"):s.replace(",","..")):"("+s+")";}
 // The caller supplies the displayed multiplicity: local in Chen, opposite in UML.
 void drawCrowFoot(Node *entity,QPointF tip,QPointF toward,const QString &card,const QString &role){
  if(logicalView||!card.endsWith("N"))return;
  QPointF direction=toward-tip;const double length=std::hypot(direction.x(),direction.y());if(length<1.)return;
  direction/=length;const double depth=qMin(22.,length*.4),spread=qMin(9.,depth*.45);const QPointF stem=tip+direction*depth;
  const QRectF rect(entity->scenePos()-QPointF(entity->size.width()/2,entity->size.height()/2),entity->size);
  QPointF first=tip,last=tip;
  if(qAbs(tip.x()-rect.left())<.01||qAbs(tip.x()-rect.right())<.01){first.setY(qMax(rect.top()+1.,tip.y()-spread));last.setY(qMin(rect.bottom()-1.,tip.y()+spread));}
  else{first.setX(qMax(rect.left()+1.,tip.x()-spread));last.setX(qMin(rect.right()-1.,tip.x()+spread));}
  QPainterPath fork;for(const auto &end:{first,tip,last}){fork.moveTo(stem);fork.lineTo(end);}linkPath(fork);
  auto item=connectors.last();item->setData(1,uml?"umlCrowFoot":"chenCrowFoot");item->setData(2,role);item->setData(3,card);
  if(uml){item->setToolTip(richTip(t("N: molte istanze di questa entità per una istanza all’altro estremo. Il minimo è indicato nella molteplicità.","N: many instances of this entity for one instance at the opposite endpoint. The multiplicity label shows the minimum.")));return;}
  item->setToolTip(richTip(t("Chen estesa: la zampa di gallina evidenzia N nella partecipazione di questo ruolo. Il minimo resta indicato da (0,N) o (1,N).","Extended Chen: the crow's foot emphasizes N in this role's participation. The minimum remains shown as (0,N) or (1,N).")));
 }
 // A segment belongs to its adjacent entity's participation, even in UML:
 // the multiplicity text at that endpoint describes the opposite role instead.
 void participationPath(const QPainterPath &path,const Object &relation,bool sideA){
  linkPath(path);if(relation.kind!="relation")return;
  const QString card=sideA?relation.cardA:relation.cardB;
  auto item=static_cast<QGraphicsPathItem*>(connectors.last());auto pen=item->pen();pen.setStyle(card.startsWith("0")?Qt::DashLine:Qt::SolidLine);item->setPen(pen);
  item->setData(1,"participation");item->setData(2,sideA?"A":"B");item->setData(3,card);
  item->setToolTip(objectHelp(relation)+"<br>"+t("Tratto vicino a ","Segment next to ")+model.get(sideA?relation.a:relation.b)->name.toHtmlEscaped()+": "+(card.startsWith("0")?t("partecipazione facoltativa (tratteggio).","optional participation (dashed)."):t("partecipazione obbligatoria (linea continua).","mandatory participation (solid).")));
 }
 void links(){if(logicalView){logicalLinks();return;}
  for(auto i:connectors)delete i;
  connectors.clear();
  for(const auto &o:model.objects){
   currentLinkId=o.id;
   if(o.kind=="attribute"&&!uml){auto a=nodes.value(o.owner),b=nodes.value(o.id);if(!a||!b)continue;QPainterPath p(boundary(a,b->pos()));p.lineTo(boundary(b,a->pos()));linkPath(p);}
   if(o.kind!="relation"&&o.kind!="isa")continue;
   auto a=nodes.value(o.a),b=nodes.value(o.b),mid=nodes.value(o.id);if(!a||!b||!mid)continue;
   QPointF ac=a->scenePos(),bc=b->scenePos(),mc=mid->scenePos();if(uml&&o.kind=="relation"){mid->readingAngle=o.a==o.b?3.141592653589793/2:std::atan2(bc.y()-ac.y(),bc.x()-ac.x());mid->update();}
   if(o.a==o.b){
    QPointF top=ac+QPointF(0,-a->size.height()/2),bottom=ac+QPointF(0,a->size.height()/2),cornerA(mc.x(),top.y()-60),cornerB(mc.x(),bottom.y()+60);
    QPainterPath p(top);p.lineTo(QPointF(top.x(),cornerA.y()));p.lineTo(cornerA);p.lineTo(uml?mc:boundary(mid,cornerA));participationPath(p,o,true);QPainterPath q(bottom);q.lineTo(QPointF(bottom.x(),cornerB.y()));q.lineTo(cornerB);q.lineTo(uml?mc:boundary(mid,cornerB));participationPath(q,o,false);
    if(o.kind=="relation"){drawCrowFoot(a,top,top+QPointF(0,-60),uml?o.cardB:o.cardA,"A");drawCrowFoot(b,bottom,bottom+QPointF(0,60),uml?o.cardA:o.cardB,"B");}
    linkText(cardinal(uml?o.cardB:o.cardA)+" "+o.roleA,top+QPointF(65,-25));linkText(cardinal(uml?o.cardA:o.cardB)+" "+o.roleB,bottom+QPointF(65,25));continue;
   }
   QPointF ap=boundary(a,mc),bp=boundary(b,mc),ma=boundary(mid,ac),mb=boundary(mid,bc);if(uml&&o.kind=="relation"){
    auto endpoint=[&](Node *node){auto c=node->scenePos();if(qAbs(mc.x()-c.x())>node->size.width()/2)return QPointF(c.x()+(mc.x()>c.x()?1:-1)*node->size.width()/2,qBound(c.y()-node->size.height()/2+1,mc.y(),c.y()+node->size.height()/2-1));return boundary(node,mc);};ap=endpoint(a);bp=endpoint(b);ma=mb=mc;
   }
   if(uml&&o.kind=="relation"){mid->readingAngle=std::atan2(bp.y()-ap.y(),bp.x()-ap.x());mid->update();}
   QPainterPath p(ap);p.lineTo(ma);participationPath(p,o,true);QPainterPath q(mb);q.lineTo(bp);participationPath(q,o,false);
   if(o.kind=="isa"){
    QLineF line(mc,bp);double angle=std::atan2(line.dy(),line.dx());QPointF back=bp-QPointF(std::cos(angle)*16,std::sin(angle)*16),perp(-std::sin(angle)*8,std::cos(angle)*8);QPolygonF tri;tri<<bp<<back+perp<<back-perp;auto arrow=scene->addPolygon(tri,QPen(theme.line,1.5),theme.surface);arrow->setData(0,o.id);connectors<<arrow;
   }else{
    drawCrowFoot(a,ap,ma,uml?o.cardB:o.cardA,"A");drawCrowFoot(b,bp,mb,uml?o.cardA:o.cardB,"B");
    auto nearEnd=[](QPointF a,QPointF b){QLineF l(a,b);return l.length()>0?a+(b-a)*(qMin(35.,l.length()*.25)/l.length()):a;};QPointF pa=uml?nearEnd(ap,ma):ap*.6+ma*.4,pb=uml?nearEnd(bp,mb):bp*.6+mb*.4;
    auto roleLabels=[&](QString card,QString role,QPointF point,QPointF direction){bool vertical=qAbs(direction.y())>qAbs(direction.x());QFont font("Segoe UI");font.setPixelSize(12);QFontMetricsF metrics(font);linkText(card,point+(vertical?QPointF(metrics.horizontalAdvance(card)/2+10,0):QPointF(0,-17)));if(!role.isEmpty())linkText(role,point+(vertical?QPointF(-metrics.horizontalAdvance(role)/2-10,0):QPointF(0,18)));};
    roleLabels(cardinal(uml?o.cardB:o.cardA),o.roleA,pa,ma-ap);roleLabels(cardinal(uml?o.cardA:o.cardB),o.roleB,pb,mb-bp);
    if(uml&&o.identifying)linkText(t("«identificante»","«identifying»"),mc+QPointF(0,31));
   }
  }
 }
