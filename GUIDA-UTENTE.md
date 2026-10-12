# Database Schema Studio

Database Schema Studio ti aiuta a descrivere una situazione attraverso entità e associazioni, trasformarla in tabelle e preparare il codice SQL per creare il database. Per avviare il programma, apri **DbSchemaStudio.exe**. Se lo trasferisci su un altro computer, estrai l’intero ZIP: l’eseguibile deve rimanere insieme alle librerie e alle sottocartelle.

## Cominciare con un esempio

All’avvio trovi **Ordini**. Un ordine può contenere più righe; ogni riga appartiene a un solo ordine. Il numero della riga è significativo all’interno di quell’ordine: due ordini diversi possono avere entrambi una riga numero 1.

Puoi modificare l’esempio oppure aprire **File → Nuovo** per partire da un foglio vuoto. Il menu **Esempi** contiene altri casi ed esercizi. Ogni diagramma ha una nota che racconta la situazione e suggerisce che cosa osservare. Usa **Salva con nome** per conservare una tua versione.

La voce **Aiuto → Guida illustrata: Ordini** apre un percorso con immagini. **Come procedere** spiega invece i passaggi principali direttamente nell’applicazione.

## Descrivere la situazione: il concettuale

Crea le entità, poi selezionane una e premi **Ins** per aggiungere un attributo. Nel pannello delle proprietà puoi cambiare nomi e valori; premi **Applica modifiche** prima di selezionare un altro elemento.

L’identificatore permette di distinguere un elemento da tutti gli altri. Per un’entità normale, seleziona **Parte della chiave** sugli attributi che lo compongono. Per un’entità debole, **Chiave parziale** indica gli attributi che identificano un elemento insieme all’identificatore dell’entità proprietaria. Nell’esempio Ordini, NumeroRiga è una chiave parziale: serve anche sapere a quale ordine appartiene.

Usa **Associazione** per collegare le entità. Le cardinalità indicano se la partecipazione è obbligatoria e quante volte può ripetersi. Selezionando l’associazione, i contrassegni A e B sul diagramma aiutano a riconoscere i ruoli nelle proprietà. Un’associazione può collegare anche un’entità a se stessa, come Persona nei ruoli figlio e genitore.

Con **ISA** puoi descrivere una sottoclasse, per esempio Privato rispetto a Cliente. La sottoclasse eredita gli attributi e l’identificatore della superclasse. Ogni sottoclasse ha una sola superclasse; i collegamenti non possono formare cicli.

Puoi assegnare un tipo generale agli attributi, come Intero, Data o Stringa. Il tipo è facoltativo in questa fase. Per gli importi è disponibile **Valuta**, che verrà tradotto in DECIMAL(19,4): diciannove cifre totali, di cui quattro decimali. Non comprende un simbolo o un codice di moneta. Se ti serve una precisione diversa, usa Reale e definiscila nella fase SQL.

Premi **F7** per verificare il modello. Puoi comunque salvare una bozza incompleta e riprenderla in seguito.

## Leggere Chen e UML-like

Le due viste mostrano lo stesso modello concettuale. Chen disegna gli attributi separatamente, mentre UML-like li elenca all’interno delle entità. Cambiare vista non genera le tabelle.

Una linea tratteggiata vicino a un’entità indica che la sua partecipazione è facoltativa; una linea continua indica che è obbligatoria. La zampa di gallina evidenzia la molteplicità N. Nella vista UML-like, i numeri sono accanto all’entità che viene contata: vicino a Prodotto, N indica quanti prodotti può fornire un fornitore. Il tratteggio segue invece la partecipazione dell’entità adiacente. Il triangolino sul nome dell’associazione ne suggerisce il verso di lettura, non un flusso di dati.

In **Vista → Identificatori UML-like** scegli se usare `{ID}` e `{ID locale}`, oppure `[PK]` e `[PK locale]`. Queste ultime etichette sono una convenzione per leggere il concettuale: non cambiano le chiavi del modello. Una chiave locale identifica l’elemento solo insieme alla chiave del proprietario. Gli identificatori alternativi delle sottoclassi rimangono distinti. Le etichette sulle colonne del logico indicano invece le chiavi delle tabelle.

## Ottenere le tabelle: il logico

Premi **Genera logico** e leggi **Dal concettuale alle tabelle**. Il riepilogo descrive le tabelle proposte e i loro collegamenti. La chiave primaria, abbreviata PK, distingue le righe di una tabella. La chiave esterna, abbreviata FK, fa riferimento a una chiave di un’altra tabella, oppure della stessa tabella nelle associazioni ricorsive.

Un’associazione molti a molti richiede una tabella con gli abbinamenti. Per un’associazione uno a uno puoi scegliere su quale lato inserire la chiave esterna. Le sottoclassi hanno tabelle collegate a quella della superclasse. Premi **Applica traduzione** per ottenere il diagramma logico.

Puoi modificare i nomi e i tipi delle colonne. **Mostra origine** ti riporta all’elemento concettuale corrispondente. Se personalizzi un tipo, **Ereditato** ripristina quello proveniente dal concettuale. Le colonne delle chiavi esterne seguono il tipo delle chiavi a cui fanno riferimento.

Dopo una modifica al concettuale, usa **Aggiorna logico**. Prima di applicare, controlla soprattutto le rimozioni proposte. I nomi personalizzati e le posizioni degli elementi rimasti vengono conservati. **Ctrl+Z** consente di annullare la traduzione.

Il riquadro **Cosa resta da controllare** descrive le regole che le sole tabelle e chiavi non garantiscono. Per esempio, il riferimento all’ordine impedisce che una riga appartenga a un ordine inesistente, ma non impone che ogni ordine abbia almeno una riga. **Mostra dettagli** apre le spiegazioni aggiuntive. Nei modelli grandi il riepilogo mostra una prima parte delle tabelle e dei controlli; gli altri sono consultabili nei dettagli.

## Preparare ed eseguire il codice SQL

Premi **F8** e scegli il motore: **HSQLDB 1.8**, **MySQL 8.4** oppure **MariaDB 10.11**. Se usi XAMPP, WampServer o Uniform Server, verifica quale motore e versione contiene la tua installazione: il nome del pacchetto non basta a scegliere il profilo.

Controlla i tipi delle colonne, la lunghezza delle stringhe e la precisione dei numeri. Per Reale, DECIMAL conserva valori decimali esatti, mentre DOUBLE rappresenta valori approssimati. Valuta usa sempre DECIMAL(19,4). Le chiavi primarie semplici di tipo INTEGER o BIGINT, che non siano anche chiavi esterne, possono avere un valore automatico.

Premi **Genera SQL** per vedere il codice. L’editor offre colorazione, ricerca e sostituzione, annullamento e zoom con Ctrl+rotella. Puoi modificare il testo, copiarlo o salvarlo in UTF-8. **Applica e chiudi** conserva impostazioni e bozza nel progetto; ricordati poi di salvare il file .schema. La modifica del codice non modifica il diagramma.

L’applicazione non esegue lo script. Aprilo nel programma che gestisce il database ed esegui le istruzioni nell’ordine previsto. Le tabelle da creare non devono già esistere. Se cambi motore o modifichi il modello, rigenera il codice. La rigenerazione chiede conferma prima di sostituire le modifiche manuali alla bozza.

### HSQLDB con LibreOffice Base

Puoi partire da `esempi/Ordini-guida/Ordini-03-fisico-HSQLDB.schema`. Apri in Base un database che utilizza **HSQLDB**, poi scegli **Strumenti → SQL**. Nell’anteprima dell’app puoi posizionare il cursore su un’istruzione e usare **Copia istruzione**. Esegui le istruzioni una alla volta, nell’ordine, e aggiorna l’elenco delle tabelle in Base.

Questo profilo richiede un database già esistente e non crea un file .odb. Il nome dello schema iniziale è PUBLIC. Non usarlo con un database Firebird. L’opzione sui limiti di lunghezza e precisione modifica un’impostazione dell’intero database HSQLDB: leggine la descrizione prima di attivarla.

### MySQL e MariaDB

Gli esempi `Ordini-04-fisico-MySQL.schema` e `Ordini-05-fisico-MariaDB.schema` contengono impostazioni per i due motori. Indica il nome del database e seleziona **Crea il database** se deve essere creato; in questo caso servono i relativi permessi sul server.

Scegli se il confronto dei testi deve distinguere maiuscole e accenti. Le tabelle usano InnoDB e Unicode utf8mb4. Esegui lo script completo nella stessa connessione: imposta UTC e una modalità SQL rigorosa, da mantenere anche quando inserisci dati.

Per Data e ora viene proposto DATETIME. Puoi usare TIMESTAMP, tenendo conto del suo intervallo più limitato. Il cambio di motore adatta alcuni tipi e reimposta il nome del contenitore: ricontrollali prima di generare.

Una chiave esterna facoltativa composta da più colonne richiede che i campi siano tutti compilati oppure tutti NULL. Il codice aggiunge un controllo per questa regola. Per questi collegamenti e per le chiavi esterne booleane, i profili server richiedono NO ACTION; CASCADE e SET NULL sono disponibili sui collegamenti compatibili. LONGTEXT non può essere usato come chiave. Eventuali impostazioni incompatibili vengono segnalate prima della generazione.

I profili si riferiscono a MySQL 8.4 e MariaDB 10.11: non presupporre che lo stesso script sia adatto a versioni precedenti.

## Disporre il diagramma e aggiungere note

Usa **Ctrl+rotella** per ingrandire e **Ctrl+0** per inquadrare tutto. Con **Spazio+trascinamento** sposti il foglio. **Vista → Mostra griglia** aggiunge punti di riferimento; **Aggancia alla griglia** fa coincidere gli elementi con quei punti durante lo spostamento. Tieni **Alt** per spostarli liberamente. Attivare la griglia non modifica da solo le posizioni e i punti non vengono stampati nel PDF.

Seleziona più figure con **Ctrl+clic** o con un rettangolo di selezione. Il menu **Disponi**, presente anche nel menu del tasto destro, permette di allinearle. Un quadratino indica la figura usata come riferimento. Per cambiarla, fai clic su una figura già selezionata. **Distribuisci** rende uguali gli spazi fra almeno tre figure; se manca spazio, allontana quelle estreme. Puoi annullare queste operazioni con Ctrl+Z.

Premi **Nota** per aggiungere una spiegazione o la traccia di un esercizio. Fai doppio clic sul riquadro per modificare il testo. Trascinalo per spostarlo; dopo averlo selezionato, trascina l’angolo inferiore destro per ridimensionarlo. Se il testo non è tutto visibile, allarga il riquadro o apri l’editor. Ctrl+C e Ctrl+V copiano una nota anche fra progetti.

Il testo della nota rimane uguale nel concettuale e nel logico, mentre puoi scegliere una posizione diversa in ciascuna vista. **Vista → Mostra note** permette di nasconderle. Nell’esportazione PDF puoi scegliere se includerle. Le note non vengono trasferite al codice SQL e non aggiungono regole al modello.

## Attributi delle associazioni e importazione RISE

Se una proprietà descrive un abbinamento, come il prezzo di un acquisto, puoi rappresentarla con un’entità associativa. Il tasto destro su un’associazione offre **Attributi dell’associazione…**: trovi una spiegazione illustrata e l’esempio Persona–Acquisto–ModelloAuto. Aprire l’esempio non trasforma l’associazione selezionata.

Per un file RISE usa **File → Importa RISE**. Leggi il riepilogo: indica quali informazioni possono essere importate e quali restano escluse. Controlla gli identificatori proposti e salva il risultato come nuovo progetto .schema.

## Conservare il lavoro

Salva i progetti in una cartella personale. Il file .schema raccoglie i modelli e le note; le preferenze di aspetto e lingua appartengono invece all’applicazione. Puoi scegliere italiano o inglese senza cambiare i nomi che hai dato alle entità.

Sono disponibili una copia precedente del salvataggio e il recupero automatico. Conserva comunque una copia dei lavori importanti. I progetti con note richiedono la versione 0.9.4 o successiva. Ogni progetto può contenere fino a 500 note, ciascuna di non più di 10.000 caratteri. Le bozze SQL già salvate vengono conservate anche aprendo progetti precedenti.

software didattico - © 2026 Prof. ing. Raffaele Mele
https://infotechlab.altervista.org
