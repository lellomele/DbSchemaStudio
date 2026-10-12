# Esercizi guidati

Apri gli esempi dal menu **Esempi → Esercizi guidati**. Ogni file contiene il concettuale con identificatori semplici, utili per generare il logico. Le note nei diagrammi raccontano la situazione e suggeriscono che cosa osservare. Salva una copia per le tue modifiche.

| Situazione descritta | Come leggere il diagramma |
|---|---|
| **Passeggeri e voli** — Un passeggero viaggia su uno o più voli; un volo può essere scelto da più passeggeri. | Passeggero (1,N), Volo (0,N). Volo indica una singola partenza. |
| **Studenti e corsi di laurea** — Ogni studente è iscritto a un solo corso di laurea, che può accogliere più studenti. | Studente (1,1), CorsoLaurea (0,N). |
| **Scelta dei corsi universitari** — Ogni studente sceglie almeno quattro corsi; ogni corso può essere scelto da più studenti. | Studente (1,N), Corso (0,N). Il minimo quattro richiede un controllo aggiuntivo: il promemoria nel nome dell’associazione non lo impone. |
| **Corsi e docenti** — Ogni corso ha un docente; un docente può insegnare più corsi. | Corso (1,1), Docente (0,N). |
| **Persone e genitori** — Le persone hanno genitori e possono avere figli. | Associazione ricorsiva: Figlio (1,N), Genitore (0,N). Il numero esatto dei genitori non è fissato. |
| **Persone e madri** — Ogni persona ha una madre; una madre può avere più figli. | Associazione ricorsiva: Figlio (1,1), Madre (0,N). |
| **Pezzi e parti di un prodotto** — I pezzi sono composti da parti, usate a loro volta per comporre pezzi. | Composto (1,N), Componente (1,N), sulla stessa entità Pezzo. |
| **Ricette e ingredienti** — Le ricette usano ingredienti e possono richiamare altre ricette. | Ricetta (1,N), Ingrediente (0,N); richiamo ricorsivo (0,N) su entrambi i ruoli. |

“Può” è interpretato come facoltativo. Per corsi/docenti e corsi di laurea, se la presenza di iscritti o incarichi è obbligatoria, modifica (0,N) in (1,N).

Negli esempi sulle persone, decidi come registrare chi ha genitori non presenti nell’archivio. Occorre anche impedire che una persona sia genitore di se stessa o che una catena di legami di parentela torni al punto di partenza. In Pezzi e parti, (1,N) su entrambi i lati significa che ogni pezzo deve avere componenti ed essere parte di un altro pezzo. Se i pezzi sono in numero finito, questo porta a tornare a un pezzo già incontrato. Per descrivere un prodotto reale ammetti anche parti elementari e prodotti finali usando (0,N), e controlla che non si formino cicli. Per le ricette, usa (0,N) sugli ingredienti diretti se ammetti preparazioni composte solo da altre ricette. Quantità e unità richiedono entità associative; i cicli vanno controllati.

Nel logico vengono generate tabelle e chiavi. **Dal concettuale alle tabelle** spiega il risultato, senza garantire automaticamente tutti i vincoli delle tracce.
