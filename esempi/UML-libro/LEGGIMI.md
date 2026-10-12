# Fornitori e prodotti

Apri **Fornitori-Prodotti-UML.schema**, scegli il livello Concettuale e la vista UML-like. Il diagramma descrive fornitori che possono fornire più prodotti; ogni prodotto ha esattamente un fornitore.

Un fornitore può essere registrato prima dei suoi prodotti. Per questo la linea vicino a Fornitore è tratteggiata: la partecipazione è facoltativa. Vicino a Prodotto la linea è continua, perché ogni prodotto deve avere un fornitore. La zampa di gallina indica che un fornitore può fornire molti prodotti.

Il triangolino vicino al nome aiuta a leggere la frase «Fornitore fornisce Prodotto». Le etichette {ID}, oppure [PK] se hai scelto questa modalità in Vista, indicano gli identificatori. Prezzo ha tipo Valuta e diventa DECIMAL(19,4) nel codice SQL.

Passa a Chen per osservare le stesse informazioni con un’altra notazione. Premi poi Genera logico e cerca, nella tabella Prodotto, il riferimento al fornitore.
