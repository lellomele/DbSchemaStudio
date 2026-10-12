"""Single source for shipped example notes and the embedded example descriptions."""
from pathlib import Path
import json

ROOT=Path(__file__).resolve().parents[1]
TEXTS={'orders': 'Ordini e righe dell’ordine\n'
           'Un ordine raccoglie le righe che descrivono ciò che viene richiesto. Può essere ancora '
           'vuoto, per esempio mentre lo stiamo compilando. Ogni riga appartiene invece a un solo '
           'ordine.\n'
           'NumeroOrdine distingue un ordine dagli altri. NumeroRiga distingue le righe dello '
           'stesso ordine: la riga 1 dell’ordine 15 è diversa dalla riga 1 dell’ordine 28. Per '
           'questo RigaOrdine è un’entità debole.\n'
           'Generando il logico, osserva la chiave di RigaOrdine: contiene sia NumeroRiga sia il '
           'riferimento a Ordine. Prova poi a rendere obbligatoria la presenza di almeno una riga '
           'per ordine e leggi il controllo aggiuntivo indicato nel riepilogo.',
 'shop': 'Un negozio e i suoi clienti\n'
         'Il negozio vende prodotti a clienti privati e aziende. I clienti effettuano ordini; ogni '
         'riga di un ordine riguarda un prodotto. Un ordine può avere una fattura. I prodotti sono '
         'raggruppati in categorie, che possono contenere altre categorie.\n'
         'Osserva Cliente e le sue sottoclassi Privato e Azienda, poi confronta l’associazione fra '
         'ordine e fattura con quella fra prodotti e categorie. RigaOrdine dipende dall’ordine '
         'anche per essere identificata.\n'
         'Passando al logico puoi seguire la trasformazione di queste strutture in tabelle e '
         'collegamenti. Il codice SQL è configurato per HSQLDB. Il riepilogo segnala le regole che '
         'richiedono altri controlli, come la presenza di righe negli ordini.',
 'purchase': 'Gli acquisti di automobili\n'
             'Una persona può acquistare più automobili, anche dello stesso modello in momenti '
             'diversi. Ogni acquisto riguarda una persona e un modello di automobile. '
             'DataAcquisto, PrezzoAcquisto e Targa descrivono quel particolare acquisto.\n'
             'Queste informazioni appartengono all’entità Acquisto: il prezzo pagato, per esempio, '
             'può cambiare da una vendita all’altra anche quando il modello è lo stesso. '
             'IdAcquisto permette di distinguere ogni operazione.\n'
             'Nel logico, la tabella Acquisto contiene i riferimenti alla persona e al modello. '
             'PrezzoAcquisto ha tipo Valuta: nel codice SQL diventa DECIMAL(19,4), adatto a '
             'conservare importi con quattro cifre decimali.',
 'suppliers': 'Fornitori e prodotti\n'
              'Un fornitore può fornire più prodotti, mentre ogni prodotto viene fornito da un '
              'solo fornitore. Ammettiamo di registrare un fornitore anche prima di aver inserito '
              'i suoi prodotti: la sua partecipazione è quindi facoltativa.\n'
              'Nella vista UML-like, la zampa di gallina accanto a Prodotto indica che i prodotti '
              'possono essere molti. Il tratteggio vicino a Fornitore indica che un fornitore può '
              'non avere ancora prodotti collegati.\n'
              'Genera il logico e cerca nella tabella Prodotto la colonna che rimanda al '
              'fornitore. Se vuoi ammettere soltanto fornitori con almeno un prodotto, modifica la '
              'cardinalità e leggi nel riepilogo quale ulteriore controllo serve.',
 'Passeggeri_e_voli': 'Passeggeri e voli\n'
                      'Un passeggero viaggia su uno o più voli. Uno stesso volo può essere scelto '
                      'da più passeggeri, ma può anche non avere ancora prenotazioni. In questo '
                      'esempio, un volo è una singola partenza, con una propria data, e non una '
                      'rotta generica.\n'
                      'Il passeggero partecipa ad almeno un volo (1,N); il volo può avere zero o '
                      'più passeggeri (0,N). L’associazione è quindi molti a molti.\n'
                      'Genera il logico e osserva la tabella che raccoglie gli abbinamenti fra '
                      'passeggeri e voli. Le chiavi esterne controllano che i riferimenti siano '
                      'validi; per verificare che ogni passeggero abbia almeno un volo occorre un '
                      'controllo ulteriore.',
 'Studenti_e_corsi_di_laurea': 'Studenti e corsi di laurea\n'
                               'Ogni studente deve essere iscritto a un solo corso di laurea. Un '
                               'corso può accogliere più studenti; in questo esempio può anche '
                               'essere registrato prima di ricevere iscrizioni.\n'
                               'Lo studente partecipa all’associazione una sola volta e '
                               'obbligatoriamente (1,1). Il corso di laurea può partecipare zero o '
                               'più volte (0,N).\n'
                               'Quando generi il logico, cerca nella tabella Studente il '
                               'riferimento al corso di laurea: per ogni studente quel valore è '
                               'obbligatorio. Se vuoi descrivere soltanto corsi che hanno già '
                               'iscritti, puoi cambiare il minimo del corso da zero a uno e '
                               'discutere come verificare questa regola.',
 'Scelta_dei_corsi': 'La scelta dei corsi universitari\n'
                     'Ogni studente deve scegliere almeno quattro corsi. Uno stesso corso può '
                     'essere scelto da più studenti, oppure non avere ancora iscritti. Uno '
                     'studente e un corso possono quindi partecipare a più abbinamenti.\n'
                     'Il diagramma rappresenta l’associazione molti a molti con (1,N) per Studente '
                     'e (0,N) per Corso. Il minimo esatto di quattro non è rappresentato da queste '
                     'cardinalità: la frase nella nota e il nome dell’associazione ricordano la '
                     'richiesta, ma non la fanno rispettare.\n'
                     'Nel logico viene creata una tabella con le scelte degli studenti. Per '
                     'controllare il requisito completo bisogna anche contare i corsi scelti da '
                     'ciascuno studente e verificare che siano almeno quattro.',
 'Corsi_e_docenti': 'Corsi e docenti\n'
                    'Ogni corso universitario è tenuto da un solo docente. Un docente può '
                    'insegnare in più corsi; ammettiamo anche di registrare docenti ai quali non è '
                    'stato ancora assegnato un corso.\n'
                    'La partecipazione di Corso è obbligatoria e singola (1,1). Quella di Docente '
                    'è facoltativa e può ripetersi (0,N).\n'
                    'Passando al logico, osserva il riferimento al docente nella tabella Corso. '
                    'Deve essere compilato per ogni corso. Se consideri soltanto docenti già '
                    'incaricati, imposta (1,N) per Docente: il riepilogo ti ricorda che la '
                    'presenza di almeno un corso per ciascun docente richiede un altro controllo.',
 'Persone_e_genitori': 'Persone e genitori\n'
                       'Una persona può essere figlia di altre persone e, a sua volta, avere '
                       'figli. Per descrivere questi legami colleghiamo Persona a se stessa, '
                       'distinguendo i ruoli figlio e genitore.\n'
                       'Nel ruolo di figlio ogni persona ha uno o più genitori (1,N); nel ruolo di '
                       'genitore può avere zero o più figli (0,N). Il diagramma non fissa il '
                       'numero esatto dei genitori.\n'
                       'Nel logico compare una tabella che registra le coppie figlio-genitore. Per '
                       'usare il modello con dati reali bisogna decidere come trattare i genitori '
                       'non presenti nell’archivio. Servono inoltre controlli per impedire che una '
                       'persona sia genitore di se stessa o che una catena di legami torni alla '
                       'persona di partenza.',
 'Persone_e_madri': 'Persone e madri\n'
                    'Ogni persona ha una madre; una stessa madre può avere più figli. '
                    'L’associazione collega Persona a se stessa, una volta nel ruolo di figlio e '
                    'una volta nel ruolo di madre.\n'
                    'Ogni persona ha un solo collegamento verso la propria madre (1,1), mentre può '
                    'non avere figli oppure averne più di uno (0,N). Il ruolo madre indica quindi '
                    'come viene usato quel lato dell’associazione, non un’entità separata.\n'
                    'Nel logico, una colonna della tabella Persona rimanda alla stessa tabella. '
                    'Discuti come registrare una persona quando sua madre non è presente '
                    'nell’archivio e come impedire riferimenti a se stessa o catene di parentela '
                    'circolari.',
 'Pezzi_e_parti': 'I pezzi e le parti di un prodotto\n'
                  'Un pezzo è composto da altri pezzi, usati come parti. La stessa entità Pezzo '
                  'compare quindi nei ruoli composto e componente. Seguendo la frase '
                  'dell’esercizio, entrambi i ruoli hanno cardinalità (1,N).\n'
                  'Questa lettura comporta una conseguenza: ogni pezzo deve avere componenti ed '
                  'essere componente di un altro pezzo. In un archivio finito si finisce così per '
                  'tornare a un pezzo già incontrato.\n'
                  'Per descrivere un prodotto reale occorre ammettere anche pezzi elementari, '
                  'senza componenti, e prodotti finali, non contenuti in altri pezzi. Prova a '
                  'usare (0,N) e discuti come evitare i cicli. Per registrare la quantità di ogni '
                  'componente, puoi introdurre un’entità associativa.',
 'Ricette_e_ingredienti': 'Ricette e ingredienti\n'
                          'Una ricetta utilizza ingredienti e può richiamare altre ricette. Per '
                          'esempio, la preparazione di un piatto di pasta può richiedere un pesto '
                          'descritto in una ricetta separata.\n'
                          'Il diagramma contiene un’associazione molti a molti fra ricette e '
                          'ingredienti e un’associazione di Ricetta con se stessa, per i richiami '
                          'ad altre preparazioni. Una ricetta può non richiamarne altre e può non '
                          'essere richiamata da nessuna.\n'
                          'Nel logico troverai due tabelle di collegamento. Se ammetti ricette '
                          'formate soltanto da altre ricette, rendi facoltativo l’uso di '
                          'ingredienti diretti. Per aggiungere quantità e unità di misura puoi '
                          'introdurre un’entità associativa; controlla anche che i richiami non '
                          'formino cicli.',
 'limit': 'Un’azienda: esempio per il test limite\n'
          'Questo esempio riunisce numerose entità e associazioni per verificare il comportamento '
          'dell’applicazione con un diagramma ampio. È adatto a chi ha già esplorato gli esempi '
          'più piccoli.\n'
          'Comincia da un’entità, osserva il suo identificatore e segui le associazioni che la '
          'collegano alle altre. Passa poi al modello logico e usa Mostra origine per capire da '
          'quali elementi concettuali provengono le colonne.\n'
          'Salva una copia prima di modificare il progetto. Dopo aver cambiato il concettuale, '
          'leggi il riepilogo di Aggiorna logico: alcune modifiche possono richiedere la '
          'sostituzione o la rimozione di tabelle e colonne.'}

def key_for(path):
 if path.stem.startswith('Ordini-'):return 'orders'
 if path.stem=='Negozio-panoramica':return 'shop'
 if path.stem=='Acquisto-entita-associativa':return 'purchase'
 if path.stem=='Fornitori-Prodotti-UML':return 'suppliers'
 if 'Test-limite-Azienda' in path.stem:return 'limit'
 return path.stem

def note_for(model,key):
 objects=model['objects'];entities=[o for o in objects if o['kind']=='entity']
 def position(layout):
  xs=[o[layout][0] for o in entities] or [0]
  bottom=0
  for o in objects:
   if layout=='uml' and o['kind']=='attribute':continue
   count=sum(a.get('owner')==o['id'] and a['kind']=='attribute' for a in objects)
   half=max(78,48+count*30)/2+50 if layout=='uml' and o['kind']=='entity' else 80
   bottom=max(bottom,o[layout][1]+half)
  return [(min(xs)+max(xs))/2,bottom+180]
 tables=model.get('logical',{}).get('tables',[])
 if tables:
  lx=[t['position'][0] for t in tables];bottom=max(t['position'][1]+max(80,48+len(t['columns'])*30)/2 for t in tables)
  logical=[(min(lx)+max(lx))/2,bottom+200]
 else:
  junctions=sum(o['kind']=='relation' and o['cardA'].endswith('N') and o['cardB'].endswith('N') for o in objects)
  logical=[215,((max(1,len(entities)+junctions)-1)//3)*360+500]
 return {'id':'note-example-'+key,'text':TEXTS[key],'size':[720,300],'chen':position('chen'),'uml':position('uml'),'logical':logical}

def write_header():
 lines=['#pragma once','#include "model.h"','#include "logical.h"','// Generated from tools/update_example_notes.py. Keep descriptions in that script.',
 'inline void annotateExample(Model &model,const QString &key){',' QString text;']
 for key,text in TEXTS.items():lines.append(f' if(key=={json.dumps(key)})text=QString::fromUtf8({json.dumps(text,ensure_ascii=False)});')
 lines.extend([''' if(text.isEmpty())return;
 DiagramNote note;note.id="note-example-"+key;note.text=text;note.size={720,300};
 auto position=[&](bool uml){double left=1e9,right=-1e9,bottom=0;for(const auto &o:model.objects){if(o.kind=="entity"){auto pos=uml?o.uml:o.chen;left=qMin(left,pos.x());right=qMax(right,pos.x());}if(uml&&o.kind=="attribute")continue;double half=uml&&o.kind=="entity"?qMax(78.,48.+model.attributes(o.id).size()*30.)/2+50:80;bottom=qMax(bottom,(uml?o.uml:o.chen).y()+half);}return QPointF(left<=right?(left+right)/2:0,bottom+180);};
 note.chen=position(false);note.uml=position(true);LogicalSchema schema;QString error;
 if(LogicalSchema::parse(model.logical,schema,error)&&!schema.tables.isEmpty()){double left=1e9,right=-1e9,bottom=0;for(const auto &t:schema.tables){left=qMin(left,t.position.x());right=qMax(right,t.position.x());bottom=qMax(bottom,t.position.y()+qMax(80.,48.+t.columns.size()*30.)/2);}note.logical={(left+right)/2,bottom+200};}
 else{int count=0;for(const auto &o:model.objects)count+=o.kind=="entity"||(o.kind=="relation"&&o.cardA.endsWith("N")&&o.cardB.endsWith("N"));note.logical={215,double(((qMax(1,count)-1)/3)*360+500)};}
 if(auto existing=model.note(note.id))*existing=note;else model.notes<<note;
}
inline Model ordersExample(){auto model=Model::example();annotateExample(model,"orders");return model;}
'''])
 (ROOT/'src/example_notes.h').write_text('\n'.join(lines),encoding='utf-8')

def main():
 paths=sorted((ROOT/'esempi').rglob('*.schema'))+sorted((ROOT/'tests/fixtures/test-limite-azienda').glob('*.schema'))
 for path in paths:
  model=json.loads(path.read_text(encoding='utf-8-sig'));key=key_for(path);note=note_for(model,key)
  model['notes']=[n for n in model.get('notes',[]) if n['id']!=note['id']]+[note];model['version']=5
  path.write_text(json.dumps(model,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
 write_header();print(f'Annotated {len(paths)} projects')

if __name__=='__main__':main()
