# Collaudo di sviluppo

## Preparazione e sicurezza

Compilare con `build.ps1` (Release) oppure `build.ps1 -Configuration Debug`, in cartelle distinte. Attendere la fine della compilazione, quindi usare `package.ps1` con una destinazione vuota. Non avviare l'eseguibile della build da solo. Copiare `fixtures` e `test-fixtures` dalla build nel pacchetto di collaudo; per i test senza schermo aggiungere `platforms/qoffscreen.dll` dall'SDK Qt.

Prima dell'avvio eseguire `check-runtime.ps1 -RuntimeFolder <pacchetto>`. Negli script usare `Start-Process -Wait -PassThru -WindowStyle Hidden` e controllare ExitCode prima di utilizzare i risultati. Le modalità di prova creano automaticamente un profilo temporaneo; non usano le preferenze o il documento dell'utente. Conservare risultati, schermate e database temporanei fuori dal repository e dalla distribuzione.

## Suite e GUI

Passare all'eseguibile del pacchetto completo una delle seguenti opzioni, seguita da una cartella di risultati:

| Opzione | Copertura |
| --- | --- |
| `--self-test` | Concettuale, logico, SQL, salvataggi, annullamenti, recupero, importazione RISE, PDF, note, esempi e regressioni; mutazioni deterministiche di JSON e XML |
| `--display-test` | GUI nativa, italiano/inglese, temi, caratteri ingranditi, dialoghi e schermate |
| `--sql-display-test` | Finestra SQL nei tre profili e nelle due lingue |
| `--benchmark` | Tre misure per Ordini, test limite e modello da 150 entità: disegno, selezione, modifica, serializzazione, traduzione e SQL |
| `--guide-assets` | Immagini della guida Ordini |

Usare `QT_QPA_PLATFORM=offscreen` per la suite e le misure; `windows` per le prove di dimensionamento. Per un secondo fattore DPI usare `QT_SCALE_FACTOR`, tenendo conto che moltiplica quello dello schermo.

`tests/session_verify.ps1 -Runtime <pacchetto>` prova il blocco della seconda istanza, termina soltanto il proprio processo di prova e verifica il recupero in un nuovo processo. Profilo e documenti sono temporanei. `tests/verify_limite.ps1 -BuildDirectory <build>` prepara un runtime completo separato e verifica chiavi composte, ISA, provenienza dei tipi e rigenerazione del caso complesso.

## HSQLDB e LibreOffice Base

`--self-test` produce i file `hsql-*.xml` con SQL e metadati attesi. Con JDK 17 o superiore e LibreOffice:

```powershell
& "$env:JAVA_HOME\bin\javac.exe" -cp 'C:\Program Files\LibreOffice\program\classes\*' -d $env:TEMP tests/HsqlVerify.java tests/BaseVerify.java
& "$env:JAVA_HOME\bin\java.exe" -cp "$env:TEMP;C:\Program Files\LibreOffice\program\classes\hsqldb.jar" HsqlVerify '<corpus-temporaneo>'
$env:UNO_PATH='C:\Program Files\LibreOffice\program'
& "$env:JAVA_HOME\bin\java.exe" -cp "$env:TEMP;C:\Program Files\LibreOffice\program\classes\*" BaseVerify 'C:\Program Files\LibreOffice\program\soffice.exe' '<corpus-temporaneo>\hsql-ordini.xml'
```

Adattare i percorsi. HsqlVerify usa database JDBC in memoria; BaseVerify avvia LibreOffice headless con un profilo separato, crea un `.odb` temporaneo e lo riapre. Il riferimento è HSQLDB **1.8.0**, jar LibreOffice SHA256 `ff1ad9cae10cafaa440c3e966f383c68ef25a7b490c1a893bd9117b13f738716`; la compatibilità non si estende a HSQLDB 2.x.

## MySQL e MariaDB

Il corpus comprende `mysql-8.4-*.json` e `mariadb-10.11-*.json`. `server_verify.py` richiede PyMySQL, solo per sviluppo. Avviare motori isolati con datadir nuovi contenenti `DbSchemaStudio-beta1-`, `--no-defaults`, bind `127.0.0.1`, porta libera, nessun servizio Windows e pagina InnoDB da 16 KiB. Configurare root locale senza password esclusivamente su queste istanze effimere; terminarle al termine. Non usare server personali.

```powershell
python tests/server_verify.py '<corpus-temporaneo>' --port <porta> --profile mysql-8.4 --expected-datadir '<datadir-esatto>'
python tests/server_verify.py '<corpus-temporaneo>' --port <porta> --profile mariadb-10.11 --expected-datadir '<datadir-esatto>'
```

Il driver rifiuta datadir diversi da quello indicato. Riferimenti: MySQL Community Server **8.4.8** Win64 e MariaDB **10.11.16** Win64. Il corpus copre esempi, tipi e default, cardinalità binarie e ricorsive, chiavi composte, UNIQUE, CHECK, FK, azioni referenziali e rifiuto di dati non validi. Le combinazioni non consentite dal motore vengono controllate prima della generazione.

Motori, driver, fixture e risultati non fanno parte del pacchetto utenti. Una suite superata documenta i percorsi provati; non dimostra l'assenza di ogni possibile difetto.
