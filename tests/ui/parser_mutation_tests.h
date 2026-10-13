#pragma once
static void parserMutationTests(std::function<void(QString,bool)> check){
 QRandomGenerator random(0xDB510001);const auto seed=shopExample();const auto original=seed.json();const auto bytes=QJsonDocument(original).toJson(QJsonDocument::Compact);int rejected=0,accepted=0;bool atomic=true,stable=true;
 for(int trial=0;trial<1000;++trial){auto input=bytes;int edits=1+random.bounded(5);for(int n=0;n<edits;++n){int pos=random.bounded(int(input.size()));if(trial%3==0)input.truncate(pos);else input[pos]=char(32+random.bounded(95));if(input.isEmpty())break;}Model result=seed;QString error;if(Model::parse(input,result,error)){++accepted;Model again;stable&=Model::parse(QJsonDocument(result.json()).toJson(),again,error)&&again.json()==result.json();}else{++rejected;atomic&=result.json()==original;}}
 check("1000 deterministic JSON mutations terminate with atomic rejection",atomic&&rejected>300);
 check("Accepted JSON mutations retain a stable canonical roundtrip",stable&&accepted>0);
 QFile fixture(QCoreApplication::applicationDirPath()+"/fixtures/iscrizioni.rise");if(!fixture.open(QIODevice::ReadOnly)){check("RISE mutation seed available",false);return;}auto xml=fixture.readAll();RiseImport baseline;QString error;if(!readRise(xml,baseline,error,true)){check("RISE mutation seed parses",false);return;}atomic=true;stable=true;rejected=accepted=0;
 for(int trial=0;trial<300;++trial){auto input=xml;int pos=random.bounded(int(input.size()));if(trial%2==0)input.truncate(pos);else input[pos]=char(32+random.bounded(95));RiseImport result=baseline;if(readRise(input,result,error,true)){++accepted;Model normalized;stable&=Model::parse(QJsonDocument(result.model.json()).toJson(),normalized,error);}else{++rejected;atomic&=result.model.json()==baseline.model.json()&&result.name==baseline.name;}}
 check("300 deterministic RISE mutations terminate without replacing output on failure",atomic&&rejected>150&&stable);
}
