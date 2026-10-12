#pragma once
#include "logical.h"

struct ColumnOrigin {QString attribute;QStringList columns,relations;bool complete=false;};
inline ColumnOrigin columnOrigin(const LogicalSchema &schema,QString id){
 ColumnOrigin out;QSet<QString> seen;
 while(!id.isEmpty()&&!seen.contains(id)){
  seen.insert(id);const LogicalTable *table=nullptr;for(const auto &t:schema.tables)if(t.column(id)){table=&t;break;}if(!table)return out;
  auto column=table->column(id);out.columns<<table->name+"."+column->name;
  bool followed=false;for(const auto &fk:table->foreignKeys){int i=fk.columns.indexOf(id);if(i<0)continue;if(i>=fk.targetColumns.size())return out;out.relations<<fk.source;id=fk.targetColumns[i];followed=true;break;}
  if(!followed){out.attribute=column->source;out.complete=!column->generated;return out;}
 }
 return out;
}

struct LogicalDifference {QString element,action,label;QJsonObject before,after;};
inline QVector<LogicalDifference> logicalDifferences(const LogicalSchema &before,const LogicalSchema &after){
 struct Entry {QString element,label;QJsonObject state;};
 auto entries=[](const LogicalSchema &s){QMap<QString,Entry> result;
  for(const auto &t:s.tables){
   auto names=[&](const QStringList &ids){QStringList labels;for(auto id:ids){auto c=t.column(id);labels<< (c?c->name:id);}return labels.join(" + ");};
   QStringList unique;for(auto group:t.uniqueKeys)unique<<names(group);unique.sort();
   result[t.id]={"table",t.name,{{"name",t.name},{"primaryKey",names(t.primaryKey())},{"unique",unique.join("; ")},{"position",QString::number(t.position.x())+", "+QString::number(t.position.y())}}};
   for(const auto &c:t.columns)result[c.id]={"column",t.name+"."+c.name,{{"name",c.name},{"type",c.dataType},{"sourceType",c.autoDataType},{"override",c.typeOverride},{"pk",c.pk},{"nullable",c.nullable}}};
   for(const auto &f:t.foreignKeys){auto target=s.table(f.target);QStringList targets;for(auto id:f.targetColumns){auto c=target?target->column(id):nullptr;targets<<(c?c->name:id);}result[f.id]={"fk",t.name+" ("+names(f.columns)+")",{{"target",target?target->name:f.target},{"columns",names(f.columns)},{"referenced",targets.join(" + ")}}};}
  }
  return result;
 };
 auto old=entries(before),next=entries(after);QVector<LogicalDifference> result;
 for(auto i=old.begin();i!=old.end();++i)if(!next.contains(i.key()))result<<LogicalDifference{i->element,"removed",i->label,i->state,{}};
 for(auto i=next.begin();i!=next.end();++i){auto prior=old.constFind(i.key());if(prior==old.cend())result<<LogicalDifference{i->element,"added",i->label,{},i->state};else if(prior->state!=i->state)result<<LogicalDifference{i->element,"changed",i->label,prior->state,i->state};}
 return result;
}
