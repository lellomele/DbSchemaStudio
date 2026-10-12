# L’esempio Ordini

Un ordine contiene le righe di ciò che viene richiesto. Ogni riga appartiene a un solo ordine e il suo numero la distingue dalle altre righe di quell’ordine. In questo esempio ammettiamo anche ordini ancora vuoti.

I file mostrano le tappe dello stesso lavoro. Aprili con **File → Apri** e segui la guida disponibile in **Aiuto → Guida illustrata: Ordini**.

- **Ordini-01-concettuale.schema** descrive Ordine, RigaOrdine e l’associazione Contiene. NumeroOrdine identifica l’ordine; NumeroRiga è una chiave parziale. I numeri hanno tipo Intero e Data ha tipo Data.
- **Ordini-02-logico.schema** contiene anche le tabelle. Osserva la chiave di RigaOrdine: unisce NumeroRiga e il riferimento all’ordine. Questo riferimento è anche una chiave esterna.
- **Ordini-03-fisico-HSQLDB.schema** aggiunge impostazioni e codice per HSQLDB 1.8, utilizzabile con un database HSQLDB di LibreOffice Base.
- **Ordini-04-fisico-MySQL.schema** e **Ordini-05-fisico-MariaDB.schema** preparano lo stesso database per MySQL 8.4 e MariaDB 10.11.

Nei tre esempi SQL il primo numero automatico di ordine è 10. Premi **F8** per leggere le impostazioni e rigenerare il codice. Gli esempi MySQL e MariaDB propongono di creare `database_esempio`: scegli un nome diverso se esiste già. L’app prepara le istruzioni; per creare le tabelle devi eseguirle nel programma del database.

Usa **Salva con nome** per conservare le tue prove senza sostituire gli esempi.
