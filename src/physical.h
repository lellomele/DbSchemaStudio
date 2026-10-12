#pragma once
#include "logical.h"

struct SqlResult { QString text; QStringList statements,errors,warnings; QJsonObject manifest; };
QStringList physicalProfiles();
QString physicalProfileName(QString profile);
QStringList physicalTypes(QString profile="hsqldb-1.8");
QString physicalDefaultType(QString generic,QString profile="hsqldb-1.8");
QJsonObject physicalSwitchProfile(const LogicalSchema&,const QJsonObject&,QString profile);
QJsonObject physicalDefaults(const LogicalSchema &schema,const QJsonObject &previous={});
bool validPhysicalStorage(const QJsonObject &data,QString &error);
QString physicalDigest(const LogicalSchema &schema,const QJsonObject &config);
QString physicalTypeOwner(const LogicalSchema &schema,QString column);
QString sqlIdentifier(QString name);
SqlResult generateHsql(const Model &model,const QJsonObject &config,bool english=false);
SqlResult generateSql(const Model &model,const QJsonObject &config,bool english=false);
// Spans include their terminating semicolon; quoted/commented semicolons are ignored.
QVector<QPair<int,int>> sqlStatementSpans(const QString &text,bool server=false);
