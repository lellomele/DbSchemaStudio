#include "physical.h"

QStringList physicalProfiles(){return {"hsqldb-1.8","mysql-8.4","mariadb-10.11"};}
QString physicalProfileName(QString p){return QMap<QString,QString>{{"hsqldb-1.8","HSQLDB 1.8 / LibreOffice Base"},{"mysql-8.4","MySQL 8.4"},{"mariadb-10.11","MariaDB 10.11"}}.value(p,p);}
QStringList physicalTypes(QString profile){QStringList types={"","INTEGER","BIGINT","SMALLINT","DECIMAL","DOUBLE","VARCHAR","CHAR","LONGVARCHAR","BOOLEAN","DATE","TIME","TIMESTAMP"};if(profile!="hsqldb-1.8"){types.replace(types.indexOf("LONGVARCHAR"),"LONGTEXT");types<<"DATETIME";}return types;}
QJsonObject physicalSwitchProfile(const LogicalSchema &s,const QJsonObject &old,QString profile){
 auto p=physicalDefaults(s,old);bool wasServer=p["profile"]!="hsqldb-1.8",server=profile!="hsqldb-1.8";p["profile"]=profile;
 if(wasServer!=server){p["schema"]=server?"database_esempio":"PUBLIC";auto cols=p["columns"].toObject();for(auto it=cols.begin();it!=cols.end();++it){auto c=it.value().toObject();QString typ=c["type"].toString();if(server&&typ=="LONGVARCHAR")c["type"]="LONGTEXT";if(server&&typ=="TIMESTAMP")c["type"]="DATETIME";if(!server&&typ=="LONGTEXT")c["type"]="LONGVARCHAR";if(!server&&typ=="DATETIME")c["type"]="TIMESTAMP";it.value()=c;}p["columns"]=cols;}
 if(server){if(!p.value("createDatabase").isBool())p["createDatabase"]=false;if(!p.value("collation").isString())p["collation"]="utf8mb4_unicode_ci";}
 else{p.remove("createDatabase");p.remove("collation");}return p;
}
QString physicalDefaultType(QString generic,QString profile){if(generic=="datetime"&&profile!="hsqldb-1.8")return "DATETIME";return QMap<QString,QString>{{"integer","INTEGER"},{"money","DECIMAL"},{"string","VARCHAR"},{"boolean","BOOLEAN"},{"date","DATE"},{"time","TIME"},{"datetime","TIMESTAMP"}}.value(generic);}
QString sqlIdentifier(QString name){return '"'+name.replace("\"","\"\"")+'"';}
QString physicalTypeOwner(const LogicalSchema &s,QString id){
 QSet<QString> seen;while(!seen.contains(id)){seen.insert(id);QString next;for(const auto &t:s.tables)for(const auto &fk:t.foreignKeys){int n=fk.columns.indexOf(id);if(n>=0&&n<fk.targetColumns.size())next=fk.targetColumns[n];}if(next.isEmpty())return id;id=next;}return {};
}
QJsonObject physicalDefaults(const LogicalSchema &s,const QJsonObject &old){
 QJsonObject result{{"version",1},{"profile","hsqldb-1.8"},{"schema","PUBLIC"},{"strictSize",true},{"draft",""},{"generatedDigest",""},{"generatedText",""}};
 for(auto key:{"profile","schema","strictSize","draft","generatedDigest","generatedText","createDatabase","collation"})if(old.contains(key))result[key]=old[key];
 QJsonObject columns,tables,fks;auto oc=old["columns"].toObject(),ot=old["tables"].toObject(),of=old["foreignKeys"].toObject();
 for(const auto &t:s.tables){tables[t.id]=ot.value(t.id).toString(t.name);for(const auto &c:t.columns){auto p=oc[c.id].toObject();if(p.isEmpty())p={{"name",c.name},{"type",physicalDefaultType(c.dataType,result["profile"].toString())},{"length",100},{"precision",c.dataType=="money"?MoneyPrecision:12},{"scale",c.dataType=="money"?MoneyScale:2},{"identity",false},{"start",1},{"defaultMode","none"},{"defaultValue",""},{"sourceType",c.dataType}};
   // A renamed logical element tracks its name unless a physical override exists.
   if(p.value("logicalName").toString(c.name)==p["name"].toString())p["name"]=c.name;
   p["logicalName"]=c.name;
   if(p["sourceType"].toString()!=c.dataType){p["type"]=physicalDefaultType(c.dataType,result["profile"].toString());p["identity"]=false;p["defaultMode"]="none";p["sourceType"]=c.dataType;if(c.dataType=="money"){p["precision"]=MoneyPrecision;p["scale"]=MoneyScale;}}
   columns[c.id]=p;
  }for(const auto &f:t.foreignKeys)fks[f.id]=of.contains(f.id)?of[f.id].toObject():QJsonObject{{"delete","NO ACTION"},{"update","NO ACTION"}};
 }
 result["columns"]=columns;result["tables"]=tables;result["foreignKeys"]=fks;return result;
}
bool validPhysicalStorage(const QJsonObject &p,QString &error){
 auto fail=[&]{error="Invalid physical configuration / Configurazione fisica non valida";return false;};
 if(p["version"]!=1||!physicalProfiles().contains(p["profile"].toString())||!p["schema"].isString()||p["schema"].toString().size()>128||!p["strictSize"].isBool())return fail();
 if(p["profile"]!="hsqldb-1.8"&&(!p["createDatabase"].isBool()||!QStringList{"utf8mb4_unicode_ci","utf8mb4_bin"}.contains(p["collation"].toString())))return fail();
 for(auto key:{"draft","generatedText","generatedDigest"})if(!p[key].isString()||p[key].toString().size()>(QString(key)=="generatedDigest"?64:2000000))return fail();
 for(auto key:{"columns","tables","foreignKeys"})if(!p[key].isObject()||p[key].toObject().size()>40000)return fail();
 for(auto value:p["tables"].toObject())if(!value.isString()||value.toString().size()>128)return fail();
 for(auto value:p["columns"].toObject()){
  if(!value.isObject())return fail();
  auto c=value.toObject();
  for(auto key:{"name","type","sourceType","defaultMode","defaultValue"})if(!c[key].isString()||c[key].toString().size()>10000)return fail();
  if(c["name"].toString().size()>128||!physicalTypes(p["profile"].toString()).contains(c["type"].toString())||!validGenericType(c["sourceType"].toString())||!c["identity"].isBool())return fail();
  if(!QStringList{"none","literal","current"}.contains(c["defaultMode"].toString()))return fail();
  for(auto key:{"length","precision","scale","start"})if(!c[key].isDouble()||c[key].toDouble()!=c[key].toInt()||c[key].toInt()<0)return fail();
  if(c["length"].toInt()>1000000||c["precision"].toInt()>65||c["scale"].toInt()>38||c["start"].toInt()>2000000000)return fail();
 }
 for(auto value:p["foreignKeys"].toObject()){if(!value.isObject())return fail();for(auto key:{"delete","update"})if(!QStringList{"NO ACTION","CASCADE","SET NULL"}.contains(value.toObject()[key].toString()))return fail();}
 return true;
}
QString physicalDigest(const LogicalSchema &s,const QJsonObject &p){auto config=p;for(auto k:{"draft","generatedDigest","generatedText","includeNotes","generatedNotesDigest"})config.remove(k);auto logical=s.json();auto tables=logical["tables"].toArray();for(int i=0;i<tables.size();++i){auto t=tables[i].toObject();t.remove("position");tables[i]=t;}logical["tables"]=tables;return QString::fromLatin1(QCryptographicHash::hash(QJsonDocument(QJsonObject{{"logical",logical},{"physical",config}}).toJson(QJsonDocument::Compact),QCryptographicHash::Sha256).toHex());}
static bool validName(QString s){if(s.trimmed().isEmpty()||s.size()>128)return false;for(auto ch:s)if(ch.category()==QChar::Other_Control||ch.category()==QChar::Other_Surrogate)return false;return true;}
SqlResult generateHsql(const Model &m,const QJsonObject &p,bool en){if(p["profile"]!="hsqldb-1.8"){SqlResult r;r.errors<<"HSQLDB profile required / Profilo HSQLDB richiesto";return r;}return generateSql(m,p,en);}
SqlResult generateSql(const Model &m,const QJsonObject &p,bool en){
 const QString profile=p["profile"].toString();const bool server=profile!="hsqldb-1.8",mysql=profile=="mysql-8.4";
 auto quote=[&](QString name){if(!server)return sqlIdentifier(name);return '`'+name.replace("`","``")+'`';};
 auto nameValid=[&](QString value,bool table=false){if(!validName(value))return false;if(!server)return true;return value.size()<=64&&!value.endsWith(' ')&&(!table||(!value.contains('.')&&!value.contains('/')&&!value.contains('\\')));};
 SqlResult r;auto tr=[&](const char *it,const char *eng){return QString::fromUtf8(en?eng:it);};QString error;LogicalSchema s;
 if(!validPhysicalStorage(p,error)){r.errors<<error;return r;}
 if(m.logical.isEmpty()||!LogicalSchema::parse(m.logical,s,error)){r.errors<<tr("Genera prima il modello logico.","Generate the logical model first.");return r;}
 if(logicalStale(m)){r.errors<<tr("Il concettuale è cambiato: aggiorna il modello logico.","The conceptual model changed: update the logical model.");return r;}
 r.errors+=s.issues(en);if(!r.errors.isEmpty())return r;
 auto cols=p["columns"].toObject(),tables=p["tables"].toObject(),actions=p["foreignKeys"].toObject();QString schema=p["schema"].toString();
 if(!nameValid(schema,true))r.errors<<tr("Nome schema non valido.","Invalid schema name.");
 auto tableName=[&](const LogicalTable &t){return tables[t.id].toString();};
 auto colName=[&](QString id){return cols[id].toObject()["name"].toString();};
 auto fullName=[&](const LogicalTable &t){return quote(schema)+"."+quote(tableName(t));};
 auto list=[&](QStringList ids){QStringList names;for(auto id:ids)names<<quote(colName(id));return names.join(", ");};
 QHash<QString,QJsonObject> effective;QSet<QString> usedTables;
 for(const auto &t:s.tables){QString tn=tableName(t);if(!nameValid(tn,true)||usedTables.contains(tn.toCaseFolded()))r.errors<<tr("Nome tabella non valido o duplicato: ","Invalid or duplicate table name: ")+tn;usedTables.insert(tn.toCaseFolded());QSet<QString> names;
  for(const auto &c:t.columns){QString label=tn+"."+colName(c.id);auto raw=cols[c.id].toObject();auto owner=physicalTypeOwner(s,c.id);auto spec=cols[owner].toObject();
   if(!nameValid(colName(c.id))||names.contains(colName(c.id).toCaseFolded()))r.errors<<tr("Nome colonna non valido o duplicato: ","Invalid or duplicate column name: ")+label;
   names.insert(colName(c.id).toCaseFolded());
   if(owner.isEmpty()||spec.isEmpty()){r.errors<<label+tr(": origine del tipo FK non risolta.",": unresolved FK type origin.");continue;}
   for(auto key:{"name","identity","start","defaultMode","defaultValue"})spec[key]=raw[key];
   QString type=spec["type"].toString();int length=spec["length"].toInt(),prec=spec["precision"].toInt(),scale=spec["scale"].toInt();
   bool compatible=c.dataType.isEmpty()||(c.dataType=="integer"&&QStringList{"SMALLINT","INTEGER","BIGINT"}.contains(type))||(c.dataType=="real"&&QStringList{"DECIMAL","DOUBLE"}.contains(type))||(c.dataType=="string"&&QStringList{"CHAR","VARCHAR","LONGVARCHAR","LONGTEXT"}.contains(type))||(c.dataType=="datetime"&&QStringList{"TIMESTAMP","DATETIME"}.contains(type))||physicalDefaultType(c.dataType,profile)==type;
   if(c.dataType=="money"&&(type!="DECIMAL"||prec!=MoneyPrecision||scale!=MoneyScale))r.errors<<label+tr(": Valuta richiede DECIMAL(19,4).",": Money requires DECIMAL(19,4).");
   if(type.isEmpty()||!compatible)r.errors<<label+tr(": scegli un tipo SQL coerente (per Reale: DECIMAL oppure DOUBLE).",": choose a compatible SQL type (Real: DECIMAL or DOUBLE).");
   if((type=="VARCHAR"||type=="CHAR")&&length<1)r.errors<<label+tr(": lunghezza non valida.",": invalid length.");
   if(type=="DECIMAL"&&(prec<1||scale>prec||prec>(server?65:38)||scale>(mysql?30:38)))r.errors<<label+tr(": precisione/scala non valide.",": invalid precision/scale.");
   if(raw["sourceType"].toString()!=c.dataType)r.errors<<label+tr(": tipo logico cambiato, rivedi le impostazioni.",": logical type changed, review settings.");
   if(spec["identity"].toBool()&&(owner!=c.id||t.primaryKey()!=QStringList{c.id}||!QStringList{"INTEGER","BIGINT"}.contains(type)||spec["defaultMode"]!="none"))r.errors<<label+tr(": autoincremento ammesso solo sulla PK semplice INTEGER/BIGINT, non FK, senza DEFAULT.",": identity requires a single INTEGER/BIGINT primary key, not an FK, without DEFAULT.");
   if(server){
    if((type=="CHAR"&&length>255)||(type=="VARCHAR"&&length>16383))r.errors<<label+tr(": lunghezza oltre il limite utf8mb4 (CHAR 255, VARCHAR 16383).",": length exceeds the utf8mb4 limit (CHAR 255, VARCHAR 16383).");
    if(spec["identity"].toBool()&&spec["start"].toInt()<1)r.errors<<label+tr(": AUTO_INCREMENT deve iniziare da almeno 1.",": AUTO_INCREMENT must start at 1 or higher.");
   }
   effective[c.id]=spec;
  }
 }
 if(!r.errors.isEmpty())return r;
 auto constraint=[&](QString kind,QString id){return quote(kind+"_"+QString::fromLatin1(QCryptographicHash::hash(id.toUtf8(),QCryptographicHash::Sha256).toHex().left(24)));};
 if(server){
  r.statements<<"SET NAMES utf8mb4;"<<"SET SESSION time_zone = '+00:00';"<<"SET SESSION sql_mode = 'STRICT_ALL_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE,ERROR_FOR_DIVISION_BY_ZERO,NO_ENGINE_SUBSTITUTION,NO_BACKSLASH_ESCAPES';";
  if(p["createDatabase"].toBool())r.statements<<"CREATE DATABASE "+quote(schema)+" CHARACTER SET utf8mb4 COLLATE "+p["collation"].toString()+";";
  r.warnings<<tr("InnoDB, utf8mb4, fuso UTC e modalità SQL rigorosa per la connessione corrente. Mantieni la modalità rigorosa anche nelle connessioni usate per inserire dati. Esegui l'intero script in una sola connessione, su un database vuoto.","InnoDB, utf8mb4, UTC time zone and strict SQL mode for the current connection. Keep strict mode in data-writing connections too. Run the entire script in one connection, against an empty database.");
 }else if(p["strictSize"].toBool())r.statements<<"SET PROPERTY \"sql.enforce_strict_size\" TRUE;";else r.warnings<<tr("Lunghezze e precisioni non forzate: dipendono dalla configurazione del database HSQLDB.","Lengths and precision are not enforced: they depend on HSQLDB database settings.");
 QJsonArray manifestTables;
 for(const auto &t:s.tables){QStringList lines;QJsonArray manifestCols,manifestFks,manifestUniques;
  for(const auto &c:t.columns){auto spec=effective[c.id];QString type=spec["type"].toString(),sqlType=type,def;int length=spec["length"].toInt(),prec=spec["precision"].toInt(),scale=spec["scale"].toInt();
   if(type=="VARCHAR"||type=="CHAR")sqlType+=QString("(%1)").arg(length);
   if(type=="DECIMAL")sqlType+=QString("(%1,%2)").arg(prec).arg(scale);
   QString mode=spec["defaultMode"].toString(),value=spec["defaultValue"].toString();bool valid=true;
   if(mode=="current"){if(type=="DATE")def="CURRENT_DATE";else if(type=="TIME")def="CURRENT_TIME";else if(type=="TIMESTAMP"||type=="DATETIME")def="CURRENT_TIMESTAMP";else valid=false;}
   if(mode=="literal"){
    if(QStringList{"VARCHAR","CHAR","LONGVARCHAR","LONGTEXT"}.contains(type)){valid=type=="LONGVARCHAR"||type=="LONGTEXT"||value.toUcs4().size()<=length;def="'"+value.replace("'","''")+"'";}
    else if(type=="BOOLEAN"){valid=value.compare("true",Qt::CaseInsensitive)==0||value.compare("false",Qt::CaseInsensitive)==0;def=value.toUpper();}
    else if(type=="DATE"||type=="TIME"||type=="TIMESTAMP"||type=="DATETIME"){
     if(type=="DATE")valid=QDate::fromString(value,"yyyy-MM-dd").isValid()&&value.size()==10;
     if(type=="TIME")valid=QTime::fromString(value,"HH:mm:ss").isValid()&&value.size()==8;
     if(type=="TIMESTAMP"||type=="DATETIME")valid=QDateTime::fromString(value,"yyyy-MM-dd HH:mm:ss").isValid()&&value.size()==19;
     if(server&&(type=="DATE"||type=="DATETIME"))valid&=value.left(4).toInt()>=1000;
     if(server&&type=="TIMESTAMP")valid&=value>="1970-01-01 00:00:01"&&value<="2038-01-19 03:14:07";
     def="'"+value+"'";
    }else{
     valid=QRegularExpression(type=="DOUBLE"?"^[+-]?[0-9]+(?:\\.[0-9]+)?(?:[eE][+-]?[0-9]+)?$":"^[+-]?[0-9]+(?:\\.[0-9]+)?$").match(value).hasMatch();
     if(QStringList{"INTEGER","SMALLINT","BIGINT"}.contains(type)){bool ok;auto number=value.toLongLong(&ok);valid&=ok&&(type!="INTEGER"||(number>=-2147483648LL&&number<=2147483647LL))&&(type!="SMALLINT"||(number>=-32768&&number<=32767));}
     if(type=="DOUBLE"){bool ok;double v=value.toDouble(&ok);valid&=ok&&std::isfinite(v);}
     if(type=="DECIMAL"){auto parts=QString(value).remove(QRegularExpression("^[+-]")).split('.');QString integer=parts[0];integer.remove(QRegularExpression("^0+"));valid&=integer.size()<=prec-scale&&parts.value(1).size()<=scale;}
     def=value;
    }
   }
   if(!valid)r.errors<<tableName(t)+"."+colName(c.id)+tr(": valore predefinito non valido.",": invalid default value.");
   // MySQL 8.4 reparses LONGTEXT default expressions: use a UTF-8 conversion
   // for literal text, avoiding apostrophe/backslash re-escaping in that reparse.
   if(mysql&&type=="LONGTEXT"&&mode=="literal")def="CONVERT(UNHEX('"+QString::fromLatin1(spec["defaultValue"].toString().toUtf8().toHex())+"') USING utf8mb4)";
   if(server&&!def.isEmpty()&&((mode=="current"&&type!="TIMESTAMP"&&type!="DATETIME")||(mysql&&type=="LONGTEXT")))def="("+def+")";
   QString line="    "+quote(colName(c.id))+" "+sqlType;
   if(spec["identity"].toBool())line+=server?QString(" NOT NULL AUTO_INCREMENT"):QString(" GENERATED BY DEFAULT AS IDENTITY (START WITH %1)").arg(spec["start"].toInt());
   else {if(!def.isEmpty())line+=" DEFAULT "+def;if(!c.nullable||c.pk)line+=" NOT NULL";else if(server)line+=" NULL";}
   lines<<line;manifestCols<<QJsonObject{{"name",colName(c.id)},{"type",type},{"nullable",c.nullable&&!c.pk},{"length",length},{"precision",prec},{"scale",scale},{"identity",spec["identity"]}};
  }
  // HSQLDB 1.8 automatically assigns the primary key to an identity column.
  bool identity=false;for(const auto &c:t.columns)identity|=effective[c.id]["identity"].toBool();
  if(!identity||server)lines<<"    CONSTRAINT "+constraint("PK",t.id)+" PRIMARY KEY ("+list(t.primaryKey())+")";
  QSet<QString> uniqueSeen;uniqueSeen.insert(t.primaryKey().join(QChar(31)));int uq=0;
  for(const auto &key:t.uniqueKeys){if(uniqueSeen.contains(key.join(QChar(31))))continue;uniqueSeen.insert(key.join(QChar(31)));lines<<"    CONSTRAINT "+constraint("UQ",t.id+QString::number(uq++))+" UNIQUE ("+list(key)+")";QJsonArray a;for(auto id:key)a<<colName(id);manifestUniques<<a;}
  for(const auto &f:t.foreignKeys)if(f.columns.size()>1){bool all=true;QStringList nulls,values;for(auto id:f.columns){all&=t.column(id)->nullable;nulls<<quote(colName(id))+" IS NULL";values<<quote(colName(id))+" IS NOT NULL";}if(all)lines<<"    CONSTRAINT "+constraint("CK",f.id)+" CHECK (("+nulls.join(" AND ")+") OR ("+values.join(" AND ")+"))";}
  if(server){
   // InnoDB cannot index TEXT or exceed 3072 bytes / 16 columns per key.
   QVector<QStringList> keys=t.uniqueKeys;keys<<t.primaryKey();for(const auto &f:t.foreignKeys)keys<<f.columns;
   for(const auto &key:keys){int bytes=0;for(auto id:key){auto c=effective[id];auto typ=c["type"].toString();if(typ=="LONGTEXT")r.errors<<tableName(t)+"."+colName(id)+tr(": LONGTEXT non può essere una chiave; scegli VARCHAR con lunghezza adeguata.",": LONGTEXT cannot be a key; choose a suitable VARCHAR length.");bytes+=(typ=="CHAR"||typ=="VARCHAR")?4*c["length"].toInt():typ=="DECIMAL"?32:8;}if(bytes>3072||key.size()>16)r.errors<<tableName(t)+tr(": chiave troppo grande per InnoDB (3072 byte, 16 colonne).",": key exceeds InnoDB limits (3072 bytes, 16 columns).");}
   if(t.columns.size()>1017||t.uniqueKeys.size()+t.foreignKeys.size()+1>64)r.errors<<tableName(t)+tr(": troppe colonne o indici per InnoDB.",": too many columns or indexes for InnoDB.");
   int rowBytes=(t.columns.size()+7)/8;for(auto c:t.columns){auto spec=effective[c.id];auto typ=spec["type"].toString();rowBytes+=(typ=="CHAR"||typ=="VARCHAR")?4*spec["length"].toInt()+2:typ=="DECIMAL"?32:typ=="LONGTEXT"?12:8;}
   if(rowBytes>65535)r.errors<<tableName(t)+tr(": dimensione massima della riga eccessiva; riduci i VARCHAR o usa LONGTEXT per colonne non chiave.",": maximum row size exceeded; shorten VARCHARs or use LONGTEXT for non-key columns.");
   for(const auto &c:t.columns)if(effective[c.id]["type"]=="BOOLEAN")lines<<"    CONSTRAINT "+constraint("BOOL",c.id)+" CHECK ("+quote(colName(c.id))+" IN (0, 1))";
   if(server){QSet<QString> checked;for(const auto &c:t.columns)if(effective[c.id]["type"]=="BOOLEAN")checked.insert(c.id);for(const auto &fk:t.foreignKeys){bool optional=fk.columns.size()>1;for(auto id:fk.columns)optional&=t.column(id)->nullable;if(optional)for(auto id:fk.columns)checked.insert(id);}
    for(const auto &fk:t.foreignKeys){auto a=actions[fk.id].toObject();if(a["delete"]!="NO ACTION"||a["update"]!="NO ACTION")for(auto id:fk.columns)if(checked.contains(id)){r.errors<<tableName(t)+tr(": Il motore non combina CASCADE/SET NULL e CHECK sulle stesse colonne. Usa NO ACTION per le FK booleane o composte facoltative.",": The engine cannot combine CASCADE/SET NULL and CHECK on the same columns. Use NO ACTION for boolean or optional composite FKs.");break;}}
   }
  }
  QString suffix;if(server){suffix=" ENGINE=InnoDB ROW_FORMAT=DYNAMIC DEFAULT CHARACTER SET=utf8mb4 COLLATE="+p["collation"].toString();for(const auto &c:t.columns)if(effective[c.id]["identity"].toBool())suffix+=" AUTO_INCREMENT="+QString::number(effective[c.id]["start"].toInt());}
  r.statements<<"CREATE TABLE "+fullName(t)+" (\n"+lines.join(",\n")+"\n)"+suffix+";";
  QJsonArray pk;for(auto id:t.primaryKey())pk<<colName(id);
  for(const auto &fk:t.foreignKeys){QJsonArray a,b;for(auto id:fk.columns)a<<colName(id);for(auto id:fk.targetColumns)b<<colName(id);manifestFks<<QJsonObject{{"columns",a},{"target",tableName(*s.table(fk.target))},{"targetColumns",b},{"delete",actions[fk.id].toObject()["delete"]},{"update",actions[fk.id].toObject()["update"]}};}
  manifestTables<<QJsonObject{{"name",tableName(t)},{"columns",manifestCols},{"primaryKey",pk},{"unique",manifestUniques},{"foreignKeys",manifestFks}};
 }
 for(const auto &t:s.tables)for(const auto &f:t.foreignKeys){QString statement="ALTER TABLE "+fullName(t)+" ADD CONSTRAINT "+constraint("FK",f.id)+" FOREIGN KEY ("+list(f.columns)+") REFERENCES "+fullName(*s.table(f.target))+" ("+list(f.targetColumns)+")";
  auto a=actions[f.id].toObject();for(auto key:{"delete","update"}){auto v=a[key].toString();if(v=="SET NULL")for(auto id:f.columns)if(!t.column(id)->nullable||t.column(id)->pk)r.errors<<tableName(t)+tr(": SET NULL richiede tutte le colonne FK facoltative.",": SET NULL requires all FK columns to be nullable.");if(v.isEmpty())r.errors<<tr("Azione FK mancante.","Missing FK action.");else if(v!="NO ACTION")statement+=" ON "+QString(key).toUpper()+" "+v;}
  r.statements<<statement+";";
 }
 for(const auto &msg:s.messages)if(msg.warning)r.warnings<<(en?msg.en:msg.it);
 if(!server)r.warnings.prepend(tr("Usa uno schema HSQLDB esistente e vuoto. Lo script non crea un file .odb. In Base: Strumenti → SQL, una istruzione alla volta; poi aggiorna le tabelle.","Use an existing empty HSQLDB schema. The script does not create an .odb file. In Base: Tools → SQL, one statement at a time; then refresh tables."));
 if(!r.errors.isEmpty()){r.statements.clear();return r;}
 r.manifest={{"profile",profile},{"schema",schema},{"tables",manifestTables}};
 r.text="-- Database Schema Studio 0.9.7 | "+physicalProfileName(profile)+"\n-- "+tr("Controlli statici superati; questo script non è stato eseguito dall'app.","Static checks passed; this script has not been executed by the app.")+"\n";
 for(auto warning:r.warnings)r.text+="-- "+warning.replace('\n',' ').replace('\r',' ')+"\n";
 r.text+="\n"+r.statements.join("\n\n")+"\n";if(r.text.size()>2000000){r.errors<<tr("SQL troppo grande: limite 2 milioni di caratteri.","SQL too large: limit is 2 million characters.");r.text.clear();r.statements.clear();}return r;
}
QVector<QPair<int,int>> sqlStatementSpans(const QString &text,bool server){
 QVector<QPair<int,int>> spans;int start=0,state=0;bool code=false;
 for(int i=0;i<text.size();++i){QChar c=text[i],next=i+1<text.size()?text[i+1]:QChar();
  if(state==3){if(c=='\n')state=0;continue;}if(state==4){if(c=='*'&&next=='/'){state=0;++i;}continue;}
  if(state==1||state==2||state==5){QChar quote=state==1?'\'':state==2?'"':'`';if(c==quote){if(next==quote)++i;else state=0;}continue;}
  if(server&&c=='#'){state=3;continue;}
  if(c=='-'&&next=='-'&&(!server||i+2>=text.size()||text[i+2].isSpace())){state=3;++i;continue;}if(c=='/'&&next=='*'){state=4;++i;continue;}
  if(c=='\''||c=='"'||c=='`'){state=c=='\''?1:c=='"'?2:5;code=true;continue;}
  if(c==';'){if(code)spans<<qMakePair(start,i+1-start);start=i+1;code=false;}else if(!c.isSpace())code=true;
 }
 if(code)spans<<qMakePair(start,int(text.size())-start);
 return spans;
}
