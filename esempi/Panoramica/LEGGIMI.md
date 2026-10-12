# Negozio e acquisti

Questi esempi contengono il modello concettuale, le tabelle generate e una configurazione SQL per HSQLDB. Aprili e usa il selettore dei livelli per seguire il passaggio dal disegno alle tabelle. Con **F8** puoi leggere o rigenerare il codice, anche per MySQL o MariaDB. Salva una copia prima di fare le tue prove.

## Negozio

**Negozio-panoramica.schema** descrive clienti privati e aziende che acquistano prodotti. Gli ordini hanno righe e possono avere una fattura. I prodotti appartengono a categorie, che possono contenere altre categorie.

Comincia da Cliente: Privato e Azienda ne ereditano l’identificatore, mentre CodiceFiscale e PartitaIVA offrono identificatori alternativi. Osserva poi RigaOrdine, che viene identificata insieme all’ordine. Confronta il collegamento fra ordine e fattura con quello fra prodotti e categorie: nel primo caso la molteplicità massima è uno, nel secondo è molti su entrambi i lati.

Le otto entità danno origine a nove tabelle. La tabella in più raccoglie gli abbinamenti fra prodotti e categorie. PrezzoUnitario usa Valuta; PesoKg usa un numero approssimato, DOUBLE. **Aiuto → Esempio Negozio: cosa osservare** spiega i passaggi.

L’esempio ammette una sola fattura per ordine. Alcune regole richiedono altri controlli: per esempio, che un ordine abbia almeno una riga o che una categoria non contenga se stessa attraverso una catena di sottocategorie.

## Acquisto

**Acquisto-entita-associativa.schema** descrive persone che acquistano modelli di automobili. La data, il prezzo e la targa appartengono al singolo acquisto. Per questo sono raccolti in Acquisto, collegata a Persona e ModelloAuto.

IdAcquisto distingue ogni operazione e permette di registrare più acquisti dello stesso modello da parte della stessa persona. Nel logico, osserva i due riferimenti nella tabella Acquisto e il tipo Valuta di PrezzoAcquisto.
