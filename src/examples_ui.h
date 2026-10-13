// Examples are parsed before replacing the current project; assets load on demand.
 void installExample(const Model &example,bool notation){beginDocument();model=example;logicalView=false;uml=notation;selection.clear();selectionIds.clear();filePath.clear();history.clear();buildMenus();refresh();fit();}
 bool openBundledExample(const QString &relative){
  Model candidate;QString error;const auto path=QCoreApplication::applicationDirPath()+"/esempi/"+relative;
  if(!loadModel(path,candidate,error)){QMessageBox::warning(this,t("Esempio non disponibile","Example unavailable"),t("Non è stato possibile aprire l’esempio. Estrai il pacchetto completo. Il progetto attuale è conservato.\n","Could not open the example. Extract the complete package. Your current project is preserved.\n")+localizedError(error,en));return false;}
  if(!maybeSave())return false;
  installExample(candidate,true);return true;
 }
 void examplesMenu(){
  auto menu=menus->addMenu(t("&Esempi","&Examples"));menu->setObjectName("examplesMenu");
  action(menu,t("Ordini","Orders"),{},[this]{if(maybeSave())installExample(ordersExample(),false);})->setObjectName("exampleOrders");
  action(menu,t("Negozio — panoramica","Shop — overview"),{},[this]{loadTeachingExample(true);})->setObjectName("exampleShop");
  action(menu,t("Acquisto — entità associativa","Purchase — associative entity"),{},[this]{loadTeachingExample(false);})->setObjectName("examplePurchase");
  action(menu,t("Fornitori e prodotti","Suppliers and products"),{},[this]{openBundledExample("UML-libro/Fornitori-Prodotti-UML.schema");})->setObjectName("exampleSuppliers");
  auto exercises=menu->addMenu(t("Esercizi guidati","Guided exercises"));exercises->setObjectName("guidedExamples");
  const QStringList files={"Passeggeri_e_voli","Studenti_e_corsi_di_laurea","Scelta_dei_corsi","Corsi_e_docenti","Persone_e_genitori","Persone_e_madri","Pezzi_e_parti","Ricette_e_ingredienti"};
  const QStringList it={"Passeggeri e voli","Studenti e corsi di laurea","Scelta dei corsi universitari","Corsi e docenti","Persone e genitori","Persone e madri","Pezzi e parti di un prodotto","Ricette e ingredienti"};
  const QStringList english={"Passengers and flights","Students and degree courses","Choosing university courses","Courses and teachers","People and parents","People and mothers","Product parts and components","Recipes and ingredients"};
  for(int i=0;i<files.size();++i)action(exercises,(en?english:it)[i],{},[this,path=files[i]]{openBundledExample("Esercizi-guidati/"+path+".schema");})->setObjectName("exampleExercise"+QString::number(i));
 }
