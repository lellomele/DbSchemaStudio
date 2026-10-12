#pragma once
#include <QtWidgets>

class RoleBadge:public QGraphicsItem {
public:
 QString role;
 explicit RoleBadge(QString value,QGraphicsItem *parent):QGraphicsItem(parent),role(std::move(value)){setZValue(100);setAcceptedMouseButtons(Qt::NoButton);}
 QRectF boundingRect()const override{return QRectF(-1,-1,26,26);}
 void paint(QPainter *p,const QStyleOptionGraphicsItem*,QWidget*)override{
  p->setPen(QPen(QColor("#9b111e"),2));p->setBrush(QColor("#ffeb3b"));p->drawRect(QRectF(0,0,24,24));QFont font("Segoe UI");font.setPixelSize(16);font.setBold(true);p->setFont(font);p->setPen(Qt::black);p->drawText(QRectF(0,0,24,24),Qt::AlignCenter,role);
 }
};
