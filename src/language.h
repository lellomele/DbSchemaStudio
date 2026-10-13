#pragma once
#include <QtWidgets>

class EducationalStyle:public QProxyStyle {
public:
 EducationalStyle():QProxyStyle("Fusion"){}
 int styleHint(StyleHint hint,const QStyleOption *option=nullptr,const QWidget *widget=nullptr,QStyleHintReturn *data=nullptr)const override{
  if(hint==SH_ToolTip_WakeUpDelay)return 1200;
  return QProxyStyle::styleHint(hint,option,widget,data);
 }
};
static void setUiLanguage(bool english){
 static QTranslator *translator=[] {auto value=new QTranslator(qApp);if(!value->load(QCoreApplication::applicationDirPath()+"/translations/qtbase_it.qm"))qWarning("Italian Qt catalogue unavailable");return value;}();
 qApp->removeTranslator(translator);if(!english)qApp->installTranslator(translator);
 QLocale::setDefault(QLocale(english?QLocale::English:QLocale::Italian));
}

// Keep document parser diagnostics independent of the GUI language.
static QString localizedError(QString error,bool english){
 const QVector<QPair<QString,QString>> messages={
  {"Invalid logical model","Modello logico non valido"},
  {"Invalid physical configuration / Configurazione fisica non valida","Configurazione fisica non valida"},
  {"Physical settings require format v4 / Il fisico richiede il formato v4","Il fisico richiede il formato v4"},
  {"Invalid generic data type","Tipo di dato generico non valido"},
  {"Only attributes have a generic data type","Il tipo generico è consentito solo sugli attributi"},
  {"Invalid JSON / JSON non valido","JSON non valido"},
  {"Unsupported format / Formato non supportato","Formato non supportato"},
  {"Too many objects / Troppi oggetti","Troppi oggetti"},
  {"File too large / File troppo grande (20 MB)","File troppo grande (limite 20 MB)"},
  {"Invalid object identity or values","Identità o valori dell’oggetto non validi"},
  {"Invalid object references or flags / Riferimenti o opzioni non validi","Riferimenti o opzioni dell’oggetto non validi"},
  {"Missing or invalid field: ","Campo mancante o non valido: "},
  {"Invalid object","Oggetto non valido"},{"Invalid flags","Indicatori non validi"},
  {"Invalid position","Posizione non valida"},{"Position out of range","Posizione fuori dai limiti"},
  {"Attribute owner missing","Entità proprietaria dell’attributo mancante"},
  {"Relationship endpoint missing","Entità di un estremo dell’associazione mancante"}
 };
 for(const auto &m:messages)if(error.startsWith(m.first))return (english?m.first.section(" / ",0,0):m.second)+error.mid(m.first.size());
 if(error.contains(" / "))return english?error.section(" / ",0,0):error.section(" / ",1);
 return error; // QFile diagnostics are translated by the Qt catalogue.
}
