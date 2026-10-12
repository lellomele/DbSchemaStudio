#pragma once
#include <QtCore>

inline constexpr int MoneyPrecision=19;
inline constexpr int MoneyScale=4;

// Stable language-independent values in .schema files. Empty means unspecified.
inline QStringList genericTypeIds(){return {"","integer","real","money","string","boolean","date","time","datetime"};}
inline bool validGenericType(const QString &id){return genericTypeIds().contains(id);}
inline QString genericTypeName(const QString &id,bool en=false){
 const QStringList it={"Non specificato","Intero","Reale","Valuta","Stringa","Booleano","Data","Ora","Data e ora"};
 const QStringList english={"Unspecified","Integer","Real","Money","String","Boolean","Date","Time","Date and time"};
 int i=genericTypeIds().indexOf(id);return (en?english:it).value(i);
}
inline bool readGenericType(const QJsonObject &j,const QString &key,QString &out){
 if(!j.contains(key)){out.clear();return true;}
 if(!j[key].isString()||!validGenericType(j[key].toString()))return false;
 out=j[key].toString();return true;
}
