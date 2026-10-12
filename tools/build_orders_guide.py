from pathlib import Path
import argparse, json
from reportlab.pdfgen import canvas
from reportlab.lib.pagesizes import A4, landscape
from reportlab.lib.colors import HexColor, white
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.platypus import Paragraph, Table, TableStyle
from reportlab.lib.styles import ParagraphStyle
from PIL import Image

parser=argparse.ArgumentParser();parser.add_argument('assets');parser.add_argument('output');args=parser.parse_args()
assets=Path(args.assets);output=Path(args.output);output.parent.mkdir(parents=True,exist_ok=True)
metadata=json.loads((assets/'verifica-guida.json').read_text());assert metadata['failures']==0
version=metadata['version']
pdfmetrics.registerFont(TTFont('Segoe','C:/Windows/Fonts/segoeui.ttf'))
pdfmetrics.registerFont(TTFont('SegoeBold','C:/Windows/Fonts/segoeuib.ttf'))
pdfmetrics.registerFontFamily('Segoe',normal='Segoe',bold='SegoeBold',italic='Segoe',boldItalic='SegoeBold')
W,H=landscape(A4);ink=HexColor('#172334');blue=HexColor('#234fa2');muted=HexColor('#47576c');line=HexColor('#d5deeb');paper=HexColor('#f5f7fb')
c=canvas.Canvas(str(output),pagesize=(W,H));c.setTitle('Guida rapida - Ordini dal concettuale al logico');c.setAuthor('Database Schema Studio - Prof. ing. Raffaele Mele');c.setSubject('Guida illustrata per Database Schema Studio '+version)
def para(text,x,top,width,size=11.5,color=ink,bold=False,maxheight=None):
 style=ParagraphStyle('p',fontName='SegoeBold' if bold else 'Segoe',fontSize=size,leading=size*1.4,textColor=color,spaceAfter=0)
 p=Paragraph(text,style);_,height=p.wrap(width,H)
 assert top+height<H-36,(text[:60],top,height)
 if maxheight:assert height<=maxheight,(text[:60],height,maxheight)
 p.drawOn(c,x,H-top-height);return height
def page(number,title,subtitle):
 c.setFillColor(blue);c.rect(0,H-6,W,6,fill=1,stroke=0)
 para('DATABASE SCHEMA STUDIO '+version+'  /  GUIDA RAPIDA',40,20,720,9,muted)
 para(f'{number}. {title}',40,44,W-80,22,bold=True)
 para(subtitle,40,81,W-80,11,muted)
 c.setStrokeColor(line);c.line(40,34,W-40,34)
 c.setFont('Segoe',8.5);c.setFillColor(muted);c.drawString(40,18,'Software didattico - © 2026 Prof. ing. Raffaele Mele')
 c.setFont('Segoe',8.5);c.setFillColor(muted);c.drawRightString(W-40,18,f'{number} / 7')
def picture(name,x,top,width,height,caption=None):
 path=assets/(name+'.png');iw,ih=Image.open(path).size;scale=min(width/iw,height/ih);dw,dh=iw*scale,ih*scale
 c.setFillColor(white);c.setStrokeColor(line);c.roundRect(x-1,H-top-height-1,width+2,height+2,5,fill=1,stroke=1)
 c.drawImage(str(path),x+(width-dw)/2,H-top-dh,dw,dh,mask='auto')
 if caption:para(caption,x,top+height+5,width,9.2,muted)
def box(text,x,top,width,height):
 c.setFillColor(paper);c.roundRect(x,H-top-height,width,height,6,fill=1,stroke=0);para(text,x+14,top+12,width-28,11,maxheight=height-20)

page(1,'Crea Ordine e RigaOrdine','Seguiamo un ordine e le sue righe: prima li descriviamo con un diagramma, poi li trasformiamo in tabelle.')
para('<b>1.</b> Apri <b>File → Nuovo</b> e resta nel livello <b>1 · Concettuale</b>. Scegli la notazione <b>Chen · min-max</b>.<br/><b>2.</b> Premi <b>Entità</b> (oppure Ctrl+E). Nelle proprietà cambia il nome in <b>Ordine</b> e premi <b>Applica modifiche</b>.<br/><b>3.</b> Crea nello stesso modo <b>RigaOrdine</b>. Trascina i rettangoli per tenerli distanziati.',40,119,W-80,12,maxheight=110)
picture('01-entita',90,235,W-180,135,'I due rettangoli rappresentano gli ordini e le righe che li compongono.')
box('<b>Come leggere l’esempio</b><br/>Un ordine raccoglie le righe di ciò che viene richiesto. Ogni riga appartiene a un solo ordine. La riga 1 può comparire in ordini diversi: per riconoscerla serve anche il numero dell’ordine.',40,408,470,105)
box('<b>Comandi utili</b><br/>Ctrl+rotella: zoom<br/>Ctrl+0: adatta il diagramma<br/>Ctrl+Z: annulla',530,408,W-570,105)
c.showPage()

page(2,'Aggiungi attributi, tipi e chiave','Gli attributi descrivono le informazioni da conservare. In questo esempio usiamo i tipi Intero e Data.')
para('Seleziona <b>Ordine</b> e premi <b>Ins</b>. Rinomina il nuovo attributo <b>NumeroOrdine</b>, scegli <b>Intero</b> e seleziona <b>Parte della chiave</b>. Premi <b>Applica modifiche</b>.<br/><br/>Aggiungi <b>Data</b> con tipo <b>Data</b>, senza selezionare la chiave. Ripeti l’inserimento su <b>RigaOrdine</b> per NumeroRiga e Quantita.',40,116,360,11.5,maxheight=154)
rows=[['Entità','Attributo','Tipo'],['Ordine','NumeroOrdine','Intero'],['Ordine','Data','Data'],['RigaOrdine','NumeroRiga','Intero'],['RigaOrdine','Quantita','Intero']]
t=Table(rows,colWidths=[110,150,90],rowHeights=29);t.setStyle(TableStyle([('FONTNAME',(0,0),(-1,-1),'Segoe'),('FONTNAME',(0,0),(-1,0),'SegoeBold'),('FONTSIZE',(0,0),(-1,-1),10.5),('TEXTCOLOR',(0,0),(-1,-1),ink),('BACKGROUND',(0,0),(-1,0),paper),('GRID',(0,0),(-1,-1),0.4,line),('VALIGN',(0,0),(-1,-1),'MIDDLE'),('LEFTPADDING',(0,0),(-1,-1),8)]));t.wrapOn(c,350,200);t.drawOn(c,40,H-290-145)
para('Per <b>NumeroRiga</b> imposteremo la chiave parziale dopo aver creato l’associazione identificante. <b>Data</b> e <b>Quantita</b> non appartengono alla chiave.',40,451,360,11,maxheight=68)
picture('02-parte-della-chiave',450,116,W-490,355,'Proprietà di NumeroOrdine: è la chiave completa dell’entità Ordine.')
c.showPage()

page(3,'Collega le entità e indica la chiave parziale','Per riconoscere una riga dobbiamo sapere anche a quale ordine appartiene.')
picture('03-associazione',40,118,350,393,'Il ruolo A è Ordine; il ruolo B è RigaOrdine.')
para('Premi <b>Associazione</b> (Ctrl+R). Inserisci <b>Contiene</b>, scegli <b>A = Ordine</b> e <b>B = RigaOrdine</b>, poi conferma.<br/><br/>Nelle proprietà imposta i ruoli <b>ordine</b> e <b>riga</b>, le cardinalità <b>(0,N)</b> e <b>(1,1)</b>, quindi seleziona <b>A identifica B</b> e applica.',425,117,375,11,maxheight=160)
para('Seleziona ora <b>NumeroRiga</b>, marca <b>Chiave parziale</b> e applica. Non inserire una copia di NumeroOrdine dentro RigaOrdine: verrà aggiunta nella traduzione al logico.',425,267,375,10.5,maxheight=68)
picture('04-chiave-parziale',440,337,350,167)
para('Con (0,N) possiamo registrare un ordine ancora vuoto, mentre lo stiamo compilando.',425,514,375,9.8,muted,maxheight=30)
c.showPage()

page(4,'Controlla il modello concettuale','Premi F7 per verificare nomi, identificatori e collegamenti. Leggi le segnalazioni prima di proseguire.')
para('<b>Chiave di Ordine:</b> NumeroOrdine. <b>Chiave completa di RigaOrdine:</b> chiave dell’Ordine + NumeroRiga.<br/>L’ordine 15 può avere la riga 1 e l’ordine 28 può avere anch’esso la riga 1: sono righe diverse.',40,114,W-80,11.5,maxheight=50)
picture('05-concettuale-Chen',40,178,W-80,171,'In Chen, il doppio bordo distingue l’entità debole e l’associazione che la identifica. Il tratteggio indica che un ordine può non avere righe.')
picture('06-concettuale-UML',40,382,W-80,103,'In UML-like gli attributi sono dentro le entità. {ID locale} indica la chiave parziale; in Vista puoi scegliere anche [PK] e [PK locale].')
para('<b>Vista → Mostra tipi di dato</b> controlla le etichette. Cambiare notazione non genera il logico. Salva con <b>Ctrl+S</b>.',40,516,W-80,11,maxheight=28)
c.showPage()

page(5,'Trasforma il disegno in tabelle','Premi Genera logico oppure seleziona il livello 2 · Logico.')
para('Nel riepilogo, <b>Ordine</b> diventa una tabella con NumeroOrdine come chiave primaria (PK). <b>RigaOrdine</b> riceve una colonna che rimanda all’ordine: è una chiave esterna (FK) e fa parte della sua chiave primaria insieme a NumeroRiga. Leggi la spiegazione e premi <b>Applica traduzione</b>.',40,117,W-80,11.5,maxheight=66)
picture('07-traduzione',40,193,W-80,316,'Il diagramma concettuale resta disponibile. Puoi tornare a osservarlo oppure annullare la traduzione con Ctrl+Z.')
c.showPage()

page(6,'Verifica le chiavi e salva entrambi i livelli','Seleziona una chiave esterna per capire da quale attributo del concettuale proviene.')
picture('08-logico',40,117,W-80,166,'PK indica la chiave primaria e FK una chiave esterna. NN significa che il valore deve essere presente.')
para('<b>La chiave della riga contiene due colonne:</b> NumeroRiga + ordine_NumeroOrdine. La seconda colonna indica l’ordine a cui appartiene la riga. Puoi cambiarne il nome nel logico.',40,310,W-80,11.5,maxheight=48)
picture('09-provenienza',40,369,360,150,'Mostra origine concettuale seleziona NumeroOrdine. Puoi anche risalire all’associazione Contiene.')
para('<b>Salva</b> con Ctrl+S: il file .schema conserva concettuale e logico. <b>File → Esporta PDF</b> esporta il livello visualizzato.<br/><br/>Se modifichi il concettuale, usa <b>Aggiorna logico</b>. Controlla il riepilogo delle modifiche e delle rimozioni. <b>Mostra dettagli</b> apre gli approfondimenti.<br/><br/>Nel pacchetto trovi <b>Ordini-01-concettuale.schema</b> e <b>Ordini-02-logico.schema</b>, entrambi con una nota descrittiva.',432,369,369,10.7,maxheight=157)
c.showPage()

page(7,'Aggiungi una spiegazione al diagramma','La nota racconta la situazione rappresentata. Puoi modificarla o aggiungerne altre per spiegare il tuo lavoro.')
para('Premi <b>Nota</b> e scrivi una spiegazione o la traccia di un esercizio. Fai <b>doppio clic</b> sul riquadro per modificarla. Puoi spostare la nota trascinandola e allargarla dall’angolo inferiore destro, dopo averla selezionata.',40,117,W-80,11.5,maxheight=65)
picture('11-nota',40,190,490,287,'Nota descrittiva dell’esempio Ordini. Vista → Mostra note permette di nasconderla.')
box('<b>La stessa spiegazione nei due modelli</b><br/>La nota si legge sia nel concettuale sia nel logico. Puoi spostarla in una posizione diversa in ciascuna vista. Il testo serve a spiegare il disegno: non aggiunge regole al database e non viene inserito nel codice SQL.',550,190,W-590,185)
box('<b>Condividi il lavoro</b><br/>Usa <b>File → Esporta PDF</b> per condividere il diagramma, con o senza note.<br/><br/>Per preparare le istruzioni che creano le tabelle, premi <b>F8</b> e scegli il motore del database.',550,390,W-590,153)
para('Premi <b>Ctrl+S</b> per salvare il progetto e le sue note. Questa guida si riferisce alla versione '+version+'.',40,516,490,10.5,maxheight=36)
c.showPage();c.save()
print(output)
