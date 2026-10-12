#pragma once
#include "physical.h"
#include "example_notes.h"

// Small, deterministic examples used by the welcome screen and the Help menu.
struct TeachingBuilder {
 Model model;
 void entity(QString id,QString name,QPointF pos){Object o;o.id=id;o.kind="entity";o.name=name;o.uml=pos;o.chen=pos*1.6;model.objects<<o;}
 void attribute(QString owner,QString id,QString name,QString type,bool key=false){Object o;o.id=id;o.kind="attribute";o.name=name;o.owner=owner;o.dataType=type;o.key=key;int index=model.attributes(owner).size();auto center=model.get(owner)->chen;o.chen=center+QPointF((index-1)*235,-175);model.objects<<o;}
 void relation(QString id,QString name,QString a,QString b,QString ca,QString cb,QPointF pos,bool identifying=false,QString ra={},QString rb={}){Object o;o.id=id;o.kind="relation";o.name=name;o.a=a;o.b=b;o.cardA=ca;o.cardB=cb;o.uml=pos;o.chen=pos*1.6;o.identifying=identifying;o.roleA=ra;o.roleB=rb;model.objects<<o;}
 void isa(QString id,QString child,QString parent,QPointF pos){relation(id,"ISA",child,parent,"1,1","0,N",pos);model.objects.last().kind="isa";}
 Model finish(){
  QStringList errors;auto logical=generateLogical(model,{},errors);if(!errors.isEmpty())return model;
  // Leave space for long generated FK names. Layouts remain independent.
  for(auto &table:logical.tables)if(auto source=model.get(table.source))table.position=source->uml*1.6;
  model.logical=logical.json();model.physical=physicalDefaults(logical);
  auto columns=model.physical["columns"].toObject();for(auto it=columns.begin();it!=columns.end();++it){auto c=it.value().toObject();if(c["sourceType"]=="real")c["type"]="DOUBLE";it.value()=c;}model.physical["columns"]=columns;
  auto sql=generateSql(model,model.physical);if(sql.errors.isEmpty()){model.physical["draft"]=sql.text;model.physical["generatedText"]=sql.text;model.physical["generatedDigest"]=physicalDigest(logical,model.physical);}return model;
 }
};
inline Model shopExample(){
 TeachingBuilder b;
 b.entity("privato","Privato",{0,0});b.attribute("privato","cf","CodiceFiscale","string",true);
 b.entity("azienda","Azienda",{600,0});b.attribute("azienda","iva","PartitaIVA","string",true);
 b.entity("cliente","Cliente",{300,360});b.attribute("cliente","idcliente","IdCliente","integer",true);b.attribute("cliente","nome","Nome","string");b.attribute("cliente","attivo","Attivo","boolean");
 b.entity("ordine","Ordine",{1000,360});b.attribute("ordine","idordine","IdOrdine","integer",true);b.attribute("ordine","dataordine","DataOrdine","date");
 b.entity("fattura","Fattura",{1000,0});b.attribute("fattura","numero","NumeroFattura","string",true);b.attribute("fattura","emissione","DataEmissione","date");
 b.entity("riga","RigaOrdine",{1000,850});b.attribute("riga","numeroriga","NumeroRiga","integer",true);b.attribute("riga","quantita","Quantita","integer");b.attribute("riga","prezzo","PrezzoUnitario","money");
 b.entity("prodotto","Prodotto",{300,850});b.attribute("prodotto","idprodotto","IdProdotto","integer",true);b.attribute("prodotto","descrizione","Descrizione","string");b.attribute("prodotto","peso","PesoKg","real");
 b.entity("categoria","Categoria",{-250,850});b.attribute("categoria","idcategoria","IdCategoria","integer",true);b.attribute("categoria","denominazione","Denominazione","string");
 b.isa("isa_privato","privato","cliente",{100,175});b.isa("isa_azienda","azienda","cliente",{500,175});
 b.relation("effettua","Effettua","cliente","ordine","0,N","1,1",{650,360});
 b.relation("documenta","DocumentatoDa","ordine","fattura","0,1","1,1",{1000,175});
 b.relation("contiene","Contiene","ordine","riga","1,N","1,1",{1000,605},true);
 b.relation("riferisce","Riguarda","riga","prodotto","1,1","0,N",{650,850});
 b.relation("classifica","Classificazione","prodotto","categoria","0,N","0,N",{25,850});
 b.relation("gerarchia","Comprende","categoria","categoria","0,N","0,1",{-580,850},false,"superiore","sottocategoria");
 auto model=b.finish();annotateExample(model,"shop");return model;
}
inline Model purchaseExample(){
 TeachingBuilder b;b.entity("persona","Persona",{0,200});b.attribute("persona","cf","CodiceFiscale","string",true);b.attribute("persona","nome","Nome","string");
 b.entity("acquisto","Acquisto",{700,200});b.attribute("acquisto","idacquisto","IdAcquisto","integer",true);b.attribute("acquisto","data","DataAcquisto","date");b.attribute("acquisto","prezzo","PrezzoAcquisto","money");b.attribute("acquisto","targa","Targa","string");
 b.entity("modello","ModelloAuto",{1450,200});b.attribute("modello","codice","CodiceModello","string",true);b.attribute("modello","descrizione","Descrizione","string");
 b.relation("effettua","Effettua","persona","acquisto","0,N","1,1",{340,200});b.relation("riguarda","Riguarda","acquisto","modello","1,1","0,N",{1080,200});auto model=b.finish();annotateExample(model,"purchase");return model;
}
