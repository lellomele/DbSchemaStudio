#pragma once
#include "model.h"

struct LogicalColumn {
 QString id,name,autoName,source;bool pk=false,nullable=true,autoNullable=true,generated=false;
 QString dataType={},autoDataType={};bool typeOverride=false;
};
struct LogicalForeignKey {QString id,source,target;QStringList columns,targetColumns;};
struct LogicalTable {
 QString id,name,autoName,source;QPointF position;QVector<LogicalColumn> columns;QVector<LogicalForeignKey> foreignKeys;QVector<QStringList> uniqueKeys;
 QStringList primaryKey()const{QStringList ids;for(const auto &c:columns)if(c.pk)ids<<c.id;return ids;}
 LogicalColumn *column(QString id){for(auto &c:columns)if(c.id==id)return &c;return nullptr;}
 const LogicalColumn *column(QString id)const{for(const auto &c:columns)if(c.id==id)return &c;return nullptr;}
};
struct LogicalMessage {QString source,it,en;bool warning=false;};
struct LogicalSchema {
 QString sourceDigest;QMap<QString,QString> oneToOne;QVector<LogicalTable> tables;QVector<LogicalMessage> messages;
 LogicalTable *table(QString id){for(auto &t:tables)if(t.id==id)return &t;return nullptr;}
 const LogicalTable *table(QString id)const{for(const auto &t:tables)if(t.id==id)return &t;return nullptr;}
 LogicalTable *owner(QString id){for(auto &t:tables)if(t.id==id||t.column(id))return &t;return nullptr;}
 bool has(QString id)const{for(const auto &t:tables)if(t.id==id||t.column(id))return true;return false;}
 QJsonObject json()const;
 QStringList issues(bool english=false)const;
 static bool parse(const QJsonObject&,LogicalSchema&,QString&);
};
QString conceptualDigest(const Model&);
bool logicalStale(const Model&);
LogicalSchema generateLogical(const Model&,const QMap<QString,QString>& choices,QStringList &errors,bool english=false);
void synchronizeForeignKeyTypes(LogicalSchema&);
void preserveLogicalEdits(LogicalSchema &next,const LogicalSchema &previous);
