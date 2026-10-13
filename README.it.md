# Database Schema Studio

[English](README.md)

Un’applicazione desktop per imparare a progettare un database: disegna il modello concettuale con entità e associazioni, trasformalo in uno schema logico e genera le istruzioni SQL per creare le tabelle.

## Funzionalità

- Viste Chen e UML-like, identificatori, entità deboli, associazioni binarie e ricorsive, ereditarietà semplice.
- Tabelle logiche con chiavi primarie, chiavi esterne e spiegazione della loro origine.
- Generazione SQL per HSQLDB 1.8, MySQL 8.4 e MariaDB 10.11, con anteprima modificabile e colorazione della sintassi.
- Note nei diagrammi, griglia con aggancio, allineamento, esportazione PDF e importazione degli elementi RISE supportati.
- Interfaccia italiana e inglese, temi giorno e notte, supporto agli schermi ad alta risoluzione, esempi e guida illustrata Ordini.

## Installazione su Windows

Il repository contiene il codice sorgente; per usare l’app è sufficiente scaricare lo ZIP Windows indicato qui sotto.

1. Scarica **DbSchemaStudio-1.0.0-beta.1-Windows-x64.zip** dalla pagina [Release](https://github.com/lellomele/DbSchemaStudio/releases/tag/v1.0.0-beta.1).
2. Estrai tutto l’archivio in una cartella.
3. Apri **DbSchemaStudio.exe** all’interno della cartella.

Mantieni le DLL e le sottocartelle insieme all’eseguibile. Per utilizzare il pacchetto non serve installare Qt separatamente. La versione distribuita richiede Windows 10 o 11 a 64 bit.

## Utilizzo

Parti dall’esempio Ordini oppure scegli **File → Nuovo**. Aggiungi entità e attributi, indica gli identificatori e collega le entità. Premi **F7** per verificare il concettuale, poi **Genera logico** per esaminare e creare le tabelle. Con **F8** scegli il motore del database e generi il codice SQL.

Salva il progetto in un file `.schema`. Il menu **Esempi** apre altri modelli e **Aiuto** permette di consultare la guida illustrata. Puoi cambiare la lingua dell’interfaccia in **Aspetto e lingua**; quella iniziale è l’italiano. I nomi inseriti nel modello non vengono tradotti.

Per approfondire, consulta la [guida utente](GUIDA-UTENTE.md) e la [guida illustrata Ordini](guide/Guida-rapida-Ordini.pdf).

La versione **1.0.0-beta.1** è una beta. Conserva una copia di sicurezza dei tuoi progetti.

## Requisiti e limiti

L’applicazione prepara il codice SQL, ma non si collega al database e non esegue le istruzioni. Usa un programma adatto al motore scelto per eseguire lo script. Il profilo HSQLDB richiede un database HSQLDB 1.8 già esistente, anche in LibreOffice Base: non crea file `.odb` e non è adatto a Firebird. I profili MySQL e MariaDB possono includere anche la creazione del database.

Le associazioni concettuali sono binarie, comprese quelle ricorsive. Per gli attributi di un’associazione usa un’entità associativa esplicita. Alcune regole, come la presenza di almeno una riga per ogni ordine, richiedono controlli ulteriori rispetto alle chiavi primarie ed esterne: leggi il riepilogo della trasformazione prima di utilizzare il codice SQL. L’importazione RISE segnala gli elementi non traducibili. Non sono distribuiti eseguibili per Linux.

## Compilazione su Windows

Servono C++17, un compilatore MinGW-w64 e un’installazione Qt 6 compatibile con il compilatore, con Widgets e PrintSupport. Il pacchetto usa Qt 6.11.2. Dalla cartella dei sorgenti, esegui in PowerShell adattando i percorsi degli SDK:

```powershell
.\build.ps1 -QtPrefix 'C:\Qt\mingw64' -CompilerPrefix 'C:\msys64\mingw64' -BuildDirectory '.\build'
.\package.ps1 -QtPrefix 'C:\Qt\mingw64' -CompilerPrefix 'C:\msys64\mingw64' -Executable '.\build\DbSchemaStudio.exe' -Destination '.\dist\DbSchemaStudio'
```

Gli script richiedono la struttura di cartelle Qt usata da MSYS2. Scegli una destinazione vuota per il pacchetto e avvia l’eseguibile soltanto dopo il completamento. Non copiare né avviare da solo l’eseguibile della compilazione. È disponibile anche `CMakeLists.txt`, per CMake 3.21 o successivo e Qt 6.8 o successivo.

## Copyright

Software didattico — © 2026 Prof. ing. Raffaele Mele. [InfoTechLab](https://infotechlab.altervista.org)

Il pacchetto Windows contiene avvisi e licenze delle dipendenze nella cartella `licenses`. L’icona dell’applicazione e il suo sorgente si trovano in `assets`.
