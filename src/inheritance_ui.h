// Included within Window.
 QString attributeKeySuffix(const Object &a)const{if(!a.key)return {};if(model.weak(a.owner))return uml&&!logicalView&&umlPkLabels?t(" [PK locale]"," [local PK]"):t(" {ID locale}"," {local ID}");if(!model.ancestors(a.owner).isEmpty())return t(" {alternativa}"," {alternate}");return uml&&!logicalView&&umlPkLabels?" [PK]":" {ID}";}
 QString inheritedLabel(const Object &attribute)const{
  auto owner=model.get(attribute.owner);
  return attributeLabel(attribute)+attributeKeySuffix(attribute)+t(" · da "," · from ")+(owner?owner->name:QString());
 }
 QString attributeOrigin(const Object &a)const{auto owner=model.get(a.owner);return t(" · da "," · from ")+(owner?owner->name:QString());}
 qreal conceptualAttributeWidth(const Object &a,bool inherited)const{return Node::attributeWidth(attributeLabel(a),attributeKeySuffix(a).trimmed(),inherited?attributeOrigin(a):QString(),inherited);}
 void configureAttributeLabel(Node *row,const Object &a,bool inherited)const{row->badgePrefix=attributeLabel(a);row->badgeText=attributeKeySuffix(a).trimmed();row->badgeTail=inherited?attributeOrigin(a):QString();row->label=row->badgePrefix+attributeKeySuffix(a)+row->badgeTail;if(a.key&&model.weak(a.owner)&&umlPkLabels)row->setToolTip(richTip(t("[PK locale] · Chiave parziale: identifica l’elemento soltanto insieme alla chiave dell’entità proprietaria.","[local PK] · Partial key: identifies this item only together with the owner entity’s key.")));}
 QString inheritedTip(const Object &attribute)const{
  return richTip(inheritedLabel(attribute)+t("\nQuesto attributo proviene dalla superclasse. Per modificarlo, selezionalo nell’elenco: la modifica sarà visibile anche nelle sottoclassi.","\nThis attribute comes from the superclass. Select it in the outline to edit it; the change will also appear in its subclasses."));
 }
 QString keySummary(QString entity)const{
  QStringList result;struct Step{QString entity;QSet<QString> path;};QVector<Step> todo{{entity,{}}};int budget=2000;
  while(!todo.isEmpty()&&--budget>0){auto step=todo.takeLast();if(step.path.contains(step.entity)){result<<t("[ciclo]","[cycle]");continue;}step.path.insert(step.entity);auto e=model.get(step.entity);if(!e)continue;
   QString parent;for(const auto &o:model.objects)if(o.kind=="isa"&&o.a==step.entity){parent=o.b;break;}
   if(!parent.isEmpty()){todo<<Step{parent,step.path};continue;}
   for(auto attribute:model.attributes(step.entity))if(attribute.key)result<<e->name+"."+attribute.name;
   for(const auto &o:model.objects)if(o.kind=="relation"&&o.identifying&&o.b==step.entity)todo<<Step{o.a,step.path};
  }
  if(!todo.isEmpty())result<<t("[troppi componenti]","[too many components]");
  return result.isEmpty()?t("da definire","not defined"):result.join(" + ");
 }
 void inheritanceInspector(const QString &entity){
  auto inherited=model.inheritedAttributes(entity);if(inherited.isEmpty())return;
  note(t("ATTRIBUTI EREDITATI · sola lettura","INHERITED ATTRIBUTES · read only"));
  for(const auto &attribute:inherited){auto link=new QLabel("<a href='#'>"+inheritedLabel(attribute).toHtmlEscaped()+"</a>");link->setObjectName("inheritedOrigin");link->setWordWrap(true);link->setTextInteractionFlags(Qt::LinksAccessibleByMouse|Qt::LinksAccessibleByKeyboard);link->setToolTip(inheritedTip(attribute));form->addRow(link);connect(link,&QLabel::linkActivated,this,[this,id=attribute.id]{QTimer::singleShot(0,this,[this,id]{selectOnly(id);fitSelection();});});}
  note(t("Le modifiche all’origine si riflettono in tutte le sottoclassi. Non aggiungere di nuovo questi attributi.","Edits to the source are reflected in all subclasses. Do not add these attributes again."));
 }
 void setSceneNodeTheme(){for(auto item:scene->items())if(auto node=dynamic_cast<Node*>(item))node->theme=theme;}
 QString mappingChanges(const LogicalSchema &previous,const LogicalSchema &next)const{
  auto changes=logicalDifferences(previous,next);QString html="<h3>"+t("Confronto con il logico salvato","Comparison with saved logical model")+"</h3>";
  if(changes.isEmpty())return html+"<p>"+t("Nessuna modifica a tabelle, colonne, chiavi o posizioni.","No changes to tables, columns, keys or positions.")+"</p>";
  int added=0,removed=0,modified=0;for(const auto &c:changes){added+=c.action=="added";removed+=c.action=="removed";modified+=c.action=="changed";}
  html+="<p>"+t("Aggiunti: ","Added: ")+QString::number(added)+t(" · Rimossi: "," · Removed: ")+QString::number(removed)+t(" · Modificati: "," · Changed: ")+QString::number(modified)+"</p>";
  const QMap<QString,QString> fields={{"name",t("Nome","Name")},{"primaryKey","PK"},{"unique","UNIQUE"},{"position",t("Posizione","Position")},{"type",t("Tipo","Type")},{"sourceType",t("Tipo di origine","Source type")},{"override",t("Tipo personalizzato","Type override")},{"pk",t("Parte della PK","Part of PK")},{"nullable",t("Ammette NULL","Allows NULL")},{"target",t("Tabella referenziata","Referenced table")},{"columns",t("Colonne FK","FK columns")},{"referenced",t("Colonne riferite","Referenced columns")}};
  auto display=[&](QString field,QJsonValue value){if(value.isBool())return value.toBool()?t("sì","yes"):t("no","no");if(field=="type"||field=="sourceType")return genericTypeName(value.toString(),en);return value.toString().isEmpty()?QString("—"):value.toString();};
  html+="<table cellspacing='6'><tr><th>"+t("Azione","Action")+"</th><th>"+t("Elemento","Element")+"</th><th>"+t("Dettaglio","Details")+"</th></tr>";
  for(const auto &c:changes){QStringList details;for(auto i=c.after.begin();i!=c.after.end();++i)if(c.action!="changed"||c.before[i.key()]!=i.value()){QString value=display(i.key(),i.value());if(c.action=="changed")value=display(i.key(),c.before[i.key()])+" → "+value;details<<(fields.value(i.key(),i.key())+": "+value).toHtmlEscaped();}if(c.action=="removed")details<<t("L’elemento e le sue personalizzazioni verranno rimossi.","The element and its customizations will be removed.");
   html+="<tr><td>"+(c.action=="added"?t("Aggiunto","Added"):c.action=="removed"?t("Rimosso","Removed"):t("Modificato","Changed"))+"</td><td>"+(c.element=="table"?t("Tabella ","Table "):c.element=="column"?t("Colonna ","Column "):QString("FK "))+c.label.toHtmlEscaped()+"</td><td>"+details.join("<br>")+"</td></tr>";}
  return html+"</table><p>"+t("Controlla le rimozioni prima di applicare. L’intera rigenerazione è annullabile.","Review removals before applying. The whole regeneration can be undone.")+"</p>";
 }
 void originInspector(const QString &column){
  const auto origin=columnOrigin(logicalCache,column);note(t("PROVENIENZA DELLA COLONNA","COLUMN ORIGIN"));for(const auto &step:origin.columns)note(step);
  auto attr=model.get(origin.attribute);auto owner=attr?model.get(attr->owner):nullptr;
  if(origin.complete&&attr&&owner)note(t("Attributo concettuale: ","Conceptual attribute: ")+owner->name+"."+attr->name);else note(t("Origine non disponibile nel concettuale corrente. Verifica il rapporto e aggiorna il logico.","Source not available in the current conceptual model. Review the report and update the logical model."));
  for(auto id:origin.relations){auto relation=model.get(id);if(!relation)continue;note((relation->kind=="isa"?QString("ISA: "):t("Associazione: ","Relationship: "))+relation->name);}
 }
