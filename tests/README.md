# Collaudo di sviluppo

Prima di avviare un test, attendere la fine della compilazione e del packaging. Usare una cartella di collaudo in `work` con eseguibile, tutte le DLL del pacchetto e plugin accanto all’eseguibile; copiarvi anche `fixtures` e `test-fixtures` dalla build. Per offscreen aggiungere `platforms/qoffscreen.dll` dall’SDK. Non avviare il prodotto da cartelle ancora in costruzione e non affidarsi al PATH del processo chiamante.

Eseguire `check-runtime.ps1 -RuntimeFolder <cartella-collaudo>` prima dell’avvio: controlla firma del pacchetto completato e dipendenze DLL ricorsive. `package.ps1` esegue lo stesso controllo prima di terminare. Nei comandi automatizzati usare `Start-Process -Wait -PassThru -WindowStyle Hidden`, verificare ExitCode e interrompere la procedura in caso di errore prima di leggere o copiare i risultati.

Dalla cartella di collaudo completa:

```powershell
.\DbSchemaStudio.exe --self-test "$env:TEMP\DbSchemaStudio-tests"
.\DbSchemaStudio.exe --display-test "$env:TEMP\DbSchemaStudio-display"
```

I test non usano il documento aperto dell'utente. La suite necessita di `fixtures/iscrizioni.rise` accanto all'eseguibile; fixture e risultati non sono distribuiti nel pacchetto utenti. Per test senza schermo si può usare il plugin Qt offscreen presente nell'SDK; per collaudi delle dimensioni della finestra utilizzare il plugin Windows con uno schermo adeguato.

Conservare qui soltanto sorgenti/fixture riproducibili. Scrivere risultati e immagini in una cartella temporanea o in `work`, non negli esempi del prodotto. `fixtures/test-limite-azienda` è un caso complesso di regressione, non un tutorial.

Per ripetere il test limite dopo la build: `tests/verify_limite.ps1`. Copia i modelli in una cartella temporanea e verifica chiavi composte, ISA, provenienza dei tipi e rigenerazione; i risultati restano nella cartella temporanea.

## SQL HSQLDB / Base

`--self-test` produce casi `hsql-*.xml` con SQL e metadati attesi nella cartella temporanea indicata. `build.ps1` copia gli esempi in `test-fixtures` accanto alla build; CMake li copia analogamente. Nessun risultato va nella distribuzione utenti.

Con JDK 17 o superiore e il jar HSQLDB di LibreOffice:

```powershell
& "$env:JAVA_HOME\bin\javac.exe" -cp 'C:\Program Files\LibreOffice\program\classes\*' -d $env:TEMP tests/HsqlVerify.java tests/BaseVerify.java
& "$env:JAVA_HOME\bin\java.exe" -cp "$env:TEMP;C:\Program Files\LibreOffice\program\classes\hsqldb.jar" HsqlVerify "$env:TEMP\DbSchemaStudio-tests"
$env:UNO_PATH='C:\Program Files\LibreOffice\program'
& "$env:JAVA_HOME\bin\java.exe" -cp "$env:TEMP;C:\Program Files\LibreOffice\program\classes\*" BaseVerify 'C:\Program Files\LibreOffice\program\soffice.exe' "$env:TEMP\DbSchemaStudio-tests\hsql-ordini.xml"
```

Adattare percorsi JDK/LibreOffice. Il primo test usa database JDBC in memoria; il secondo avvia LibreOffice headless con un profilo temporaneo separato, crea un .odb temporaneo e ne controlla la riapertura. Non usa documenti o profili personali. Per dati modificati manualmente non attribuire le verifiche al testo originale.

Profilo di riferimento: HSQLDB dichiara versione **1.8.0** (jar distribuito da LibreOffice, SHA256 `ff1ad9cae10cafaa440c3e966f383c68ef25a7b490c1a893bd9117b13f738716`); Java Temurin 21.0.5, Windows x64; LibreOffice build `bce0998afefdbc355585ca324285661a2170ba77`. Questo identifica il motore effettivamente provato, senza estendere la compatibilità a HSQLDB 2.x.

Controllo GUI fisico/SQL a DPI nativi: `DbSchemaStudio.exe --sql-display-test <cartella-temporanea>`. Usare sempre un pacchetto completo; impostare QT_QPA_PLATFORM=windows per i test di dimensionamento.

## MySQL / MariaDB (0.9.1)

`--self-test` genera anche `mysql-8.4-*.json` e `mariadb-10.11-*.json`, con DDL, manifest dei metadati e prove di inserimento. `server_verify.py` usa PyMySQL (solo sviluppo), connessioni loopback e database temporanei con prefisso univoco; rifiuta server il cui datadir non contenga `DbSchemaStudio-091-`. Avviare i motori con dati nuovi in directory dedicate, `--no-defaults`, bind 127.0.0.1, nessun servizio Windows, pagina InnoDB 16 KiB. Non usare server personali. Il test richiede root locale senza password esclusivamente su queste istanze effimere; terminarle dopo il collaudo.

```powershell
python tests/server_verify.py <corpus-temporaneo> --port 39318 --profile mysql-8.4
python tests/server_verify.py <corpus-temporaneo> --port 39319 --profile mariadb-10.11
```

Riferimenti: MySQL Community Server **8.4.8** Win64 (archivio ufficiale `https://cdn.mysql.com/archives/mysql-8.4/mysql-8.4.8-winx64.zip`); MariaDB **10.11.16** Win64 (`https://archive.mariadb.org/mariadb-10.11.16/winx64-packages/`), SHA256 ZIP `b1659bd9fe816624632c6b445ce4ad1ed6758a199e35f89e2448aef99828527b`, verificato contro sha256sums.txt ufficiale. Nessun motore, driver Python o risultato di collaudo entra nella distribuzione utenti.

Il corpus comprende esercizi, Ordini, test limite, tipi/default, chiavi composte, UNIQUE, CHECK booleani, rifiuto di FK orfane/NULL/duplicati e azioni FK. Le combinazioni CHECK+CASCADE/SET NULL vietate dai motori sono testate come errori statici; le azioni compatibili vengono eseguite sul server. Le prove GUI native coprono entrambe le lingue e tutti e tre i profili.
