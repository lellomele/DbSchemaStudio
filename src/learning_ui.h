// Included in Window. Bounded summaries, escaped model text and lazy details.
 QColor learningColor(int kind)const{
  if(preset==3)return theme.surface;
  const QStringList light={"#eaf2ff","#e5f5ed","#fff1d5","#fde9e7"},dark={"#20344e","#1b3d32","#483819","#482723"};
  return QColor((isDark()?dark:light).value(kind%4));
 }
 QString learningCard(QString title,QString body,int kind=0)const{
  auto bg=learningColor(kind);auto ink=Theme::textOn(bg);
  return QString("<table width='100%' cellpadding='10' cellspacing='0' bgcolor='%1' style='color:%2'><tr><td><b>%3</b>%4</td></tr></table><br>").arg(bg.name(),ink.name(),title.toHtmlEscaped(),body);
 }
 QString colorHelpHtml(QString html)const{
  html.replace(QRegularExpression("<p><b>([^<]+)</b>"),"<h3>\\1</h3><p>");
  QRegularExpression sections("<h[23]>(.*?)</h[23]>(.*?)(?=<h[23]>|$)",QRegularExpression::DotMatchesEverythingOption);auto matches=sections.globalMatch(html);QString result;int index=0;
  while(matches.hasNext()){auto m=matches.next();result+=learningCard(m.captured(1),m.captured(2),index++%3);}return result.isEmpty()?learningCard({},html):result;
 }
 QString tableSummary(const LogicalSchema &schema)const{
  QStringList entries;int shown=0;
  for(const auto &table:schema.tables){if(shown++==20)break;QStringList keys;for(auto id:table.primaryKey())if(auto c=table.column(id))keys<<c->name;
   QString entry="<li><b>"+table.name.toHtmlEscaped()+"</b>: "+keys.join(" + ").toHtmlEscaped()+t(" permette di distinguere una riga dalle altre: è la chiave primaria."," distinguishes each row from the others: it is the primary key.");
   int links=0;for(const auto &fk:table.foreignKeys){if(links++==3){entry+=t(" Puoi leggere gli altri collegamenti in Mostra dettagli."," You can read about the other links in Show details.");break;}auto target=schema.table(fk.target);if(!target)continue;QStringList columns;bool optional=false;for(auto id:fk.columns)if(auto c=table.column(id)){columns<<c->name;optional|=c->nullable;}
    entry+="<br>"+columns.join(" + ").toHtmlEscaped()+t(" collega la riga a "," links the row to ")+target->name.toHtmlEscaped()+(optional?t(" (collegamento facoltativo)."," (optional link)."):".");
   }entries<<entry+"</li>";
  }
  QString text="<p>"+t("Il modello logico contiene ","The logical model contains ")+QString::number(schema.tables.size())+(schema.tables.size()==1?t(" tabella."," table."):t(" tabelle."," tables."))+"</p><ul>"+entries.join("")+"</ul>";
  if(schema.tables.size()>20)text+="<p>"+t("Qui trovi le prime 20 tabelle. Mostra dettagli per l’elenco completo.","The first 20 tables are shown here. Show details for the complete list.")+"</p>";
  return learningCard(t("Dal concettuale alle tabelle","From concepts to tables"),text,0);
 }
 QString simpleWarning(const LogicalSchema &schema,const LogicalMessage &message)const{
  if(message.en.contains("minimum participation",Qt::CaseInsensitive)||message.en.contains("Minimum owner participation")){
   for(const auto &table:schema.tables){
    if(table.source==message.source&&table.foreignKeys.size()==2)return table.name+t(": verifica che siano presenti tutti i collegamenti obbligatori. La tabella registra gli abbinamenti, ma non obbliga ogni elemento ad averne almeno uno.",": check that all required links exist. The table stores pairs, but does not require every item to belong to at least one pair.");
    for(const auto &fk:table.foreignKeys)if(fk.source==message.source){auto target=schema.table(fk.target);if(target)return t("Il database non garantisce, da solo, che ogni riga di ","The database alone does not ensure that each row in ")+target->name+t(" abbia almeno una riga collegata in "," has at least one related row in ")+table.name+".";}
   }
  }
  if(message.en.contains("optional composite FK"))return t("Per un collegamento facoltativo composto da più campi, compila tutti i campi oppure lasciali tutti vuoti. Controlla questa regola quando generi SQL.","For an optional link using several fields, fill all fields or leave all of them empty. Check this rule when generating SQL.");
  return en?message.en:message.it;
 }
 QString remainingChecks(const LogicalSchema &schema)const{
  QStringList messages;int warningCount=0;for(const auto &message:schema.messages)if(message.warning){++warningCount;if(messages.size()<8)messages<<simpleWarning(schema,message);}
  if(messages.isEmpty())return {};
  QString text="<ul>";for(int i=0;i<qMin(8,messages.size());++i)text+="<li>"+messages[i].toHtmlEscaped()+"</li>";text+="</ul>";
  if(warningCount>8)text+="<p>"+t("Altri controlli nei dettagli: ","More checks in details: ")+QString::number(warningCount-8)+".</p>";
  return learningCard(t("Cosa resta da controllare","What still needs checking"),text,2);
 }
 QString schemaReport(const LogicalSchema &schema)const{return tableSummary(schema)+remainingChecks(schema);}
 QDialog *mappingExplanationDialog(){
  auto dialog=new QDialog(this);dialog->setObjectName("mappingExplanation");dialog->setWindowTitle(t("Dal concettuale alle tabelle","From concepts to tables"));auto layout=new QVBoxLayout(dialog);auto browser=new QTextBrowser;browser->setObjectName("learningSummary");browser->setOpenExternalLinks(false);layout->addWidget(browser,1);auto details=new QCheckBox(t("Mostra dettagli","Show details"));details->setObjectName("showMappingDetails");layout->addWidget(details);auto buttons=new QDialogButtonBox(QDialogButtonBox::Close);layout->addWidget(buttons);connect(buttons,&QDialogButtonBox::rejected,dialog,&QDialog::reject);
  LogicalSchema schema;QString error;bool valid=LogicalSchema::parse(model.logical,schema,error);const bool stale=logicalStale(model);
  QString summary=valid?schemaReport(schema):learningCard(t("Modello non disponibile","Model unavailable"),"<p>"+localizedError(error,en).toHtmlEscaped()+"</p>",3);
  if(valid&&stale)summary=learningCard(t("Da aggiornare","Needs updating"),"<p>"+t("Il concettuale è cambiato. Questa spiegazione riguarda le tabelle generate in precedenza.","The conceptual model has changed. This explanation describes the previously generated tables.")+"</p>",2)+summary;
  browser->setHtml(summary);details->setEnabled(valid);connect(details,&QCheckBox::toggled,dialog,[this,browser,summary,schema](bool show){browser->setHtml(summary+(show?learningCard(t("Dettagli della trasformazione","Mapping details"),schemaDetails(schema),1):QString()));});dialog->resize(800,schema.tables.size()<=4?420:650);return dialog;
 }
