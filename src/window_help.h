// Included within Window: presentation, contextual help and keyboard commands.
 QString copyrightText()const{return QString::fromUtf8("© 2026 Prof. ing. Raffaele Mele");}
 QString profileUrl()const{return "https://infotechlab.altervista.org";}
 QString richTip(QString text)const{return "<div style='max-width:360px;white-space:normal'>"+text.toHtmlEscaped().replace("\n","<br>")+"</div>";}
 void updateCredit(){
  credit->setFont(QFont("Segoe UI",qMin(fontSize,12)));
  const qreal ratio=devicePixelRatioF();QPixmap image(qRound(24*ratio),qRound(16*ratio));image.setDevicePixelRatio(ratio);image.fill(Qt::transparent);QPainter painter(&image);painter.fillRect(QRectF(0,0,8,16),QColor("#009246"));painter.fillRect(QRectF(8,0,8,16),Qt::white);painter.fillRect(QRectF(16,0,8,16),QColor("#ce2b37"));painter.setPen(theme.line);painter.drawRect(QRectF(.5,.5,23,15));painter.end();flag->setPixmap(image);flag->setAccessibleName(t("Bandiera italiana","Italian flag"));flag->setToolTip(t("Italia · autore del software didattico","Italy · educational software author"));
  credit->setText("<a style='color:"+theme.accent.name()+"' href='"+profileUrl()+"'>"+copyrightText()+"</a>");credit->setToolTip(richTip(t("Apri il sito InfoTechLab del Prof. ing. Raffaele Mele nel browser predefinito.\nÈ necessaria una connessione Internet.","Open Prof. ing. Raffaele Mele’s InfoTechLab website in your default browser.\nAn Internet connection is required.")));credit->setAccessibleName(copyrightText()+" · InfoTechLab");
 }
 QDialog *aboutDialog(){
  auto dialog=new QDialog(this);dialog->setObjectName("aboutDialog");dialog->setWindowTitle(t("Informazioni su Database Schema Studio","About Database Schema Studio"));auto layout=new QVBoxLayout(dialog);layout->setContentsMargins(24,24,24,24);layout->setSpacing(16);
  auto heading=new QLabel("Database Schema Studio");QFont font=heading->font();font.setPointSize(font.pointSize()+5);font.setBold(true);heading->setFont(font);layout->addWidget(heading);
  layout->addWidget(new QLabel(t("Versione ","Version ")+QCoreApplication::applicationVersion()));
  auto description=new QLabel(t("Modellazione concettuale per la didattica delle basi di dati.\nEntità, attributi e associazioni nelle viste Chen e UML-like.","Conceptual modeling for database education.\nEntities, attributes and relationships in Chen and UML-like views."));description->setWordWrap(true);layout->addWidget(description);
  // Author attribution is intentionally identical in both interface languages.
  auto legal=new QLabel("software didattico - "+copyrightText());legal->setObjectName("aboutCopyright");legal->setTextInteractionFlags(Qt::TextSelectableByMouse);layout->addWidget(legal);
  auto link=new QLabel("<a href='"+profileUrl()+"'>InfoTechLab · Prof. ing. Raffaele Mele</a>");link->setTextInteractionFlags(Qt::LinksAccessibleByMouse|Qt::LinksAccessibleByKeyboard);connect(link,&QLabel::linkActivated,dialog,[this](const QString&){openExternal(QUrl(profileUrl()));});layout->addWidget(link);
  auto buttons=new QDialogButtonBox(QDialogButtonBox::Close);connect(buttons,&QDialogButtonBox::rejected,dialog,&QDialog::reject);layout->addWidget(buttons);return dialog;
 }
 void about(){QScopedPointer<QDialog> dialog(aboutDialog());dialog->exec();}
 void showProperties(){properties->show();properties->raise();inspector();if(auto edit=formWidget->findChild<QLineEdit*>()){edit->setFocus();edit->selectAll();}}
 void insertContext(){if(model.note(selection)){editNote(selection);return;}if(logicalView){showProperties();return;}
  auto o=model.get(selection);
  if(selectionIds.size()==1&&o){if(o->kind=="entity"||o->kind=="attribute"){addAttribute();view->setFocus();}else showProperties();}
  else if(selectionIds.isEmpty())addEntity();
  else status->setText(t("Ins: seleziona una sola entità o un suo attributo per aggiungere un attributo.","Insert: select one entity or one of its attributes to add an attribute."));
 }
 bool diagramKey(QKeyEvent *event){
  // Consume only events delivered to the diagram/outline, never bubbled field events.
  if(event->matches(QKeySequence::Copy)&&selectionIds.size()==1&&model.note(selection)){copyNote();return true;}
  if(event->matches(QKeySequence::Paste)){pasteNoteAt(center());return true;}
  if(event->modifiers()!=Qt::NoModifier)return false;
  if(event->key()==Qt::Key_Insert){insertContext();return true;}
  if(event->key()==Qt::Key_F2&&selectionIds.size()==1){renameObject(selection);return true;}
  return false;
 }
 bool eventFilter(QObject *object,QEvent *event)override{
  if(object==tree&&event->type()==QEvent::KeyPress&&diagramKey(static_cast<QKeyEvent*>(event)))return true;
  return QMainWindow::eventFilter(object,event);
 }
 QString helpText(QString text)const{
  text.remove('&');
  const QVector<QPair<QString,QString>> entries={
   {t("Nuovo","New"),t("Crea un progetto concettuale vuoto. Prima di sostituire il progetto aperto puoi salvarne le modifiche.\nScorciatoia: Ctrl+N.","Create an empty conceptual project. You can save changes before replacing the open project.\nShortcut: Ctrl+N.")},
   {t("Apri…","Open…"),t("Apri un progetto .schema con oggetti e posizioni Chen/UML conservati.\nScorciatoia: Ctrl+O.","Open a .schema project with its objects and saved Chen/UML positions.\nShortcut: Ctrl+O.")},
   {t("Salva","Save"),t("Salva nel file .schema i modelli, le note e la disposizione degli elementi. Le proprietà nei campi vanno prima confermate con Applica modifiche.\nScorciatoia: Ctrl+S.","Save the models, notes and arrangement of elements to a .schema file. Confirm property fields with Apply changes first.\nShortcut: Ctrl+S.")},
   {t("Entità","Entity"),t("Aggiungi un’entità concettuale al centro della vista. Non è ancora una tabella SQL.\nCtrl+E; oppure Ins senza selezione.","Add a conceptual entity at the center of the view. It is not yet an SQL table.\nCtrl+E; or Insert with no selection.")},
   {t("Attributo","Attribute"),t("Aggiungi un attributo semplice all’entità selezionata. Se è selezionato un attributo, ne crea un altro della stessa entità. Nessun tipo SQL richiesto.\nIns nel diagramma o nell’elenco; anche Ctrl+T.","Add a simple attribute to the selected entity. If an attribute is selected, add another to its owner. No SQL type is required.\nInsert in the diagram or outline; also Ctrl+T.")},
   {t("Associazione","Relationship"),t("Collega due ruoli di entità. Una stessa entità può partecipare due volte in un’associazione ricorsiva. Imposta poi le cardinalità nelle proprietà.\nScorciatoia: Ctrl+R.","Connect two entity roles. The same entity may participate twice in a recursive relationship. Then set multiplicities in Properties.\nShortcut: Ctrl+R.")},
   {"ISA",t("Definisci una sottoclasse e la sua superclasse. La sottoclasse eredita identità e attributi. Ogni sottoclasse può avere una sola superclasse; la catena delle superclassi non può tornare al punto di partenza.\nScorciatoia: Ctrl+I.","Define a subclass and its superclass. The subclass inherits identity and attributes. Each subclass can have one superclass; following the chain of superclasses must not lead back to the starting point.\nShortcut: Ctrl+I.")},
   {t("Verifica modello","Validate model"),t("Controlla identificatori mancanti, nomi duplicati, ruoli ricorsivi e dipendenze ISA. Puoi salvare anche una bozza incompleta.\nScorciatoia: F7.","Check missing identifiers, duplicate names, recursive roles and ISA dependencies. Incomplete drafts can still be saved.\nShortcut: F7.")},
   {t("Elimina selezione","Delete selection"),t("Elimina gli oggetti selezionati. Eliminando un’entità rimuove anche attributi e collegamenti dipendenti. Ctrl+Z annulla l’intera operazione.\nScorciatoia: Canc.","Delete selected objects. Deleting an entity also removes its attributes and dependent links. Ctrl+Z undoes the whole operation.\nShortcut: Delete.")},
   {t("Esporta PDF…","Export PDF…"),t("Esporta il diagramma completo in un PDF vettoriale su una pagina A4 o A3. Usa un profilo chiaro per la stampa anche in modalità notte.\nScorciatoia: Ctrl+P.","Export the entire diagram as a vector PDF on one A4 or A3 page. Use a light print profile even in night mode.\nShortcut: Ctrl+P.")},
   {t("Aspetto e lingua","Appearance and language"),t("Scegli Italiano o English, tema giorno/notte/sistema, palette, colore accento e dimensione del testo. I nomi del tuo modello non vengono tradotti.","Choose Italiano or English, day/night/system theme, palette, accent color and text size. Your model’s names are not translated.")},
   {t("Nome","Name"),t("Nome dell’oggetto concettuale. Conferma con Applica modifiche. Per rinominare direttamente dal diagramma puoi usare F2 o il menu destro.","Conceptual object name. Confirm with Apply changes. Use F2 or the context menu to rename directly from the diagram.")},
   {t("Applica modifiche","Apply changes"),t("Conferma tutti i campi di questo pannello in una sola modifica annullabile. Cambiare selezione prima di applicare abbandona i valori ancora nei campi.","Confirm all fields in this panel as one undoable edit. Changing selection before applying discards unconfirmed field values.")}
  };
  for(const auto &entry:entries)if(text==entry.first||text==entry.first+"…")return entry.second;
  if(text=="+"||text==t("Ingrandisci","Zoom in"))return t("Aumenta lo zoom mantenendo il centro della vista. Ctrl+rotella ingrandisce invece sul punto sotto il mouse. Limite: 600%.","Zoom in around the view center. Ctrl+wheel instead zooms at the pointer. Maximum: 600%.");
  if(text=="−"||text==t("Riduci","Zoom out"))return t("Riduci lo zoom per vedere più oggetti. Ctrl+rotella usa il punto sotto il mouse come riferimento. Limite: 10%.","Zoom out to see more objects. Ctrl+wheel keeps the point under the pointer fixed. Minimum: 10%.");
  if(text==t("Adatta","Fit")||text==t("Adatta diagramma","Fit diagram"))return t("Inquadra tutti gli oggetti senza modificarne le posizioni.\nScorciatoia: Ctrl+0.","Fit all objects into the view without changing their positions.\nShortcut: Ctrl+0.");
  if(text.startsWith(t("Annulla","Undo")))return t("Annulla l’ultima modifica applicata. Lo spostamento di un gruppo si annulla in un solo passaggio.\nScorciatoia: Ctrl+Z.","Undo the last applied edit. A group move is undone in one step.\nShortcut: Ctrl+Z.");
  if(text.startsWith(t("Ripristina","Redo")))return t("Ripristina l’ultima modifica annullata.\nScorciatoia: Ctrl+Y oppure Ctrl+Maiusc+Z.","Redo the last undone edit.\nShortcut: Ctrl+Y or Ctrl+Shift+Z.");
  if(text.contains(t("ruolo","Role")))return t("Nome del ruolo svolto dall’entità in questa associazione. Nelle associazioni ricorsive usa nomi distinti per distinguere le due partecipazioni.","Name of the entity’s role in this relationship. In recursive relationships use distinct names for the two participations.");
  return {};
 }
 void helpWidget(QWidget *widget,QString text){widget->setToolTip(richTip(text));widget->setToolTipDuration(20000);widget->setWhatsThis(text);}
 QString objectHelp(const Object &o)const{
  QString text=o.name+"\n";
  if(o.kind=="entity"){text+=model.weak(o.id)?t("Per distinguere un elemento di questa entità debole serve anche l’identificatore dell’entità proprietaria.","Identifying an item of this weak entity also requires the identifier of its owner entity."):t("Entità concettuale: rappresenta oggetti con proprietà comuni.","Conceptual entity: represents objects with shared properties.");text+=t("\nAttributi: ","\nAttributes: ")+QString::number(model.attributes(o.id).size())+t(". Ins aggiunge un attributo; F2 rinomina.",". Insert adds an attribute; F2 renames.");}
  else if(o.kind=="attribute"){auto owner=model.get(o.owner);text+=t("Attributo di ","Attribute of ")+(owner?owner->name:QString())+". "+(o.key?t("Contribuisce all’identificatore.","Contributes to the identifier."):t("Non fa parte dell’identificatore.","Not part of the identifier."));text+=t("\nTipo di dato: ","\nData type: ")+genericTypeName(o.dataType,en);text+=t("\nIns aggiunge un altro attributo alla stessa entità.","\nInsert adds another attribute to the same entity.");}
  else if(o.kind=="relation"){auto a=model.get(o.a),b=model.get(o.b);text+=t("Per ogni ","For each ")+(a?a->name:"A")+": ("+o.cardA+") "+(b?b->name:"B")+"\n"+t("Per ogni ","For each ")+(b?b->name:"B")+": ("+o.cardB+") "+(a?a->name:"A");text+=t("\n0 = facoltativo; 1 = obbligatorio; N = molti. Ins apre le proprietà.","\n0 = optional; 1 = mandatory; N = many. Insert opens Properties.");}
  else{auto a=model.get(o.a),b=model.get(o.b);text+=(a?a->name:QString())+t(" è una sottoclasse di "," is a subclass of ")+(b?b->name:QString())+t(".\nIns apre le proprietà.",".\nInsert opens Properties.");}
  text+=t("\nTasto destro: azioni. Ctrl+clic: selezione multipla.","\nRight-click: actions. Ctrl+click: multiple selection.");return richTip(text);
 }
 void decorateInterface(){
  for(auto menu:findChildren<QMenu*>())menu->setToolTipsVisible(true);
  for(auto a:findChildren<QAction*>()){QString text=helpText(a->text());if(!text.isEmpty()){a->setToolTip(richTip(text));a->setWhatsThis(text);}}
  helpWidget(notationBox,t("Due rappresentazioni dello stesso modello. Chen usa entità, ellissi e rombi; UML-like mostra gli attributi dentro le entità. Ogni vista conserva le proprie posizioni.","Two representations of the same model. Chen uses entities, ellipses and diamonds; UML-like shows attributes inside entities. Each view keeps its own positions."));
  helpWidget(tree,t("Elenco degli oggetti. Ctrl+clic seleziona più elementi. Ins aggiunge un attributo all’entità selezionata; su un’associazione apre le proprietà.","Object outline. Ctrl+click selects multiple items. Insert adds an attribute to the selected entity; on a relationship it opens Properties."));
  view->setAccessibleName(t("Diagramma concettuale","Conceptual diagram"));
  helpWidget(view,t("Ctrl+rotella: zoom sul puntatore. Spazio+trascinamento: sposta il foglio. Ctrl+clic: selezione multipla. Tasto destro: azioni. Ins: inserimento contestuale. F1: guida.","Ctrl+wheel: zoom at pointer. Space+drag: pan. Ctrl+click: multiple selection. Right-click: actions. Insert: contextual insertion. F1: help."));
  updateCredit();
 }
 void localizeHistory(){
  const QVector<QPair<QString,QString>> names={{"Aggiungi nota","Add note"},{"Modifica nota","Edit note"},{"Incolla nota","Paste note"},{"Disponi figure","Arrange shapes"},{"Impostazioni fisiche e SQL","Physical settings and SQL"},{"Importa RISE","Import RISE"},{"Traduci modello logico","Map logical model"},{"Modifica proprietà logiche","Edit logical properties"},{"Sposta oggetti","Move objects"},{"Sposta selezione","Move selection"},{"Allinea selezione","Align selection"},{"Rinomina","Rename"},{"Modifica identificatore","Change identifier"},{"Modifica cardinalità","Change multiplicity"},{"Relazione identificante","Identifying relationship"},{"Crea entità","Create entity"},{"Crea attributo","Create attribute"},{"Crea collegamento","Create link"},{"Elimina oggetti","Delete objects"},{"Modifica proprietà","Edit properties"},{"Recupera progetto","Recover project"}};
  for(int i=0;i<history.count();++i){auto command=const_cast<QUndoCommand*>(history.command(i));for(const auto &pair:names)if(command->text()==pair.first||command->text()==pair.second){command->setText(en?pair.second:pair.first);break;}}
 }
