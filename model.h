#pragma once
#include <QtCore>
#include "generic_types.h"
#include <functional>
#include <cmath>

struct Object {
 QString id, kind, name, owner, a, b, roleA, roleB;
 QString cardA="0,N", cardB="1,1";
 bool key=false, identifying=false;
 QPointF chen, uml;
 QString dataType;
};
struct DiagramNote {
 QString id,text;QPointF chen,uml,logical;QSizeF size{340,180};
};
struct Model {
 QVector<DiagramNote> notes;
 DiagramNote *note(const QString &id);
 const DiagramNote *note(const QString &id)const;
 QVector<Object> objects;
 QJsonObject physical; // Persisted SQL settings and editable draft (project format v4).
 QJsonObject logical; // A separate, persisted relational model; conceptual objects stay unchanged.
 Object *get(const QString &id);
 const Object *get(const QString &id) const;
 QVector<Object> attributes(const QString &entity) const;
 bool weak(const QString &entity) const;
 QStringList ancestors(const QString &entity) const;
 QVector<Object> inheritedAttributes(const QString &entity) const;
 QJsonObject json() const;
 static bool parse(const QByteArray &bytes,Model &out,QString &error);
 QStringList issues(bool en=false) const;
 void remove(const QString &id);
 static Model example();
};
QString newId();
bool saveModel(const QString &path,const Model &model,QString &error);
bool loadModel(const QString &path,Model &model,QString &error);
