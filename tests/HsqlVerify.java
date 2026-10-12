import java.nio.file.*;
import java.nio.charset.StandardCharsets;
import java.sql.*;
import java.util.*;
import javax.xml.parsers.*;
import org.w3c.dom.*;

// Run on XML cases emitted by --self-test. Uses only Java/JDBC; no production DB.
public class HsqlVerify {
 static int checks=0;
 static void check(boolean ok,String message){checks++;if(!ok)throw new AssertionError(message);}
 static List<Element> children(Element parent,String tag){var out=new ArrayList<Element>();for(Node n=parent.getFirstChild();n!=null;n=n.getNextSibling())if(n instanceof Element e&&e.getTagName().equals(tag))out.add(e);return out;}
 static String decode(Element e){return new String(Base64.getDecoder().decode(e.getTextContent()),StandardCharsets.UTF_8);}
 static String q(String s){return "\""+s.replace("\"","\"\"")+"\"";}
 static String names(Element parent,String tag){var result=new ArrayList<String>();for(var e:children(parent,tag))result.add(e.getTextContent());return String.join("|",result);}
 static int rule(String action){return action.equals("CASCADE")?DatabaseMetaData.importedKeyCascade:action.equals("SET NULL")?DatabaseMetaData.importedKeySetNull:DatabaseMetaData.importedKeyNoAction;}
 static String scalar(Connection c,String sql)throws Exception{try(var st=c.createStatement();var rs=st.executeQuery(sql)){check(rs.next(),"query returns a row");return rs.getString(1);}}
 static void run(Path path)throws Exception{
  var factory=DocumentBuilderFactory.newInstance();factory.setFeature("http://apache.org/xml/features/disallow-doctype-decl",true);
  var root=factory.newDocumentBuilder().parse(path.toFile()).getDocumentElement();String name=root.getAttribute("name"),schema=root.getAttribute("schema");
  try(var c=DriverManager.getConnection("jdbc:hsqldb:mem:test"+UUID.randomUUID().toString().replace("-",""),"SA","")){
   var metadata=c.getMetaData();System.out.println(name+" | "+metadata.getDatabaseProductName()+" "+metadata.getDatabaseProductVersion());
   for(var statement:children(root,"statement"))try(var st=c.createStatement()){st.execute(decode(statement));checks++;}catch(SQLException ex){throw new Exception(name+" DDL: "+decode(statement),ex);}
   int tables=0;try(var rs=metadata.getTables(null,schema,"%",new String[]{"TABLE"})){while(rs.next())tables++;}check(tables==children(root,"table").size(),name+" table count");
   for(var t:children(root,"table")){
    String table=t.getAttribute("name");var found=new HashSet<String>();
    try(var rs=metadata.getColumns(null,schema,table,"%")){while(rs.next()){
     String cn=rs.getString("COLUMN_NAME");found.add(cn);Element expected=null;for(var col:children(t,"column"))if(col.getAttribute("name").equals(cn))expected=col;check(expected!=null,table+" unexpected column "+cn);var col=expected;
     String type=col.getAttribute("type"),actual=rs.getString("TYPE_NAME");
     check(actual.equals(type)||(type.equals("LONGVARCHAR")&&actual.equals("VARCHAR"))||(type.equals("BOOLEAN")&&actual.equals("BIT")),table+"."+cn+" type "+actual+" expected "+type);
     check((rs.getInt("NULLABLE")==DatabaseMetaData.columnNullable)==Boolean.parseBoolean(col.getAttribute("nullable")),table+"."+cn+" nullability");
     if(type.equals("CHAR")||type.equals("VARCHAR"))check(rs.getInt("COLUMN_SIZE")==Integer.parseInt(col.getAttribute("length")),table+"."+cn+" length");
     if(type.equals("DECIMAL")){check(rs.getInt("COLUMN_SIZE")==Integer.parseInt(col.getAttribute("precision")),cn+" precision");check(rs.getInt("DECIMAL_DIGITS")==Integer.parseInt(col.getAttribute("scale")),cn+" scale");}
    }}check(found.size()==children(t,"column").size(),table+" column count");
    var pk=new TreeMap<Integer,String>();try(var rs=metadata.getPrimaryKeys(null,schema,table)){while(rs.next())pk.put(rs.getInt("KEY_SEQ"),rs.getString("COLUMN_NAME"));}check(String.join("|",pk.values()).equals(names(t,"pk")),table+" PK");
    var fkGroups=new HashMap<String,TreeMap<Integer,String>>();var fkRules=new HashMap<String,String>();try(var rs=metadata.getImportedKeys(null,schema,table)){while(rs.next()){String fk=rs.getString("FK_NAME");fkGroups.computeIfAbsent(fk,k->new TreeMap<>()).put(rs.getInt("KEY_SEQ"),rs.getString("FKCOLUMN_NAME")+"="+rs.getString("PKTABLE_NAME")+"."+rs.getString("PKCOLUMN_NAME"));fkRules.put(fk,rs.getInt("DELETE_RULE")+"/"+rs.getInt("UPDATE_RULE"));}}
    check(fkGroups.size()==children(t,"fk").size(),table+" FK count");for(var fk:children(t,"fk")){var wanted=new ArrayList<String>();for(var pair:children(fk,"pair"))wanted.add(pair.getAttribute("column")+"="+fk.getAttribute("target")+"."+pair.getAttribute("targetColumn"));String match=null;for(var entry:fkGroups.entrySet())if(new ArrayList<>(entry.getValue().values()).equals(wanted))match=entry.getKey();check(match!=null,table+" FK tuple "+wanted);check(fkRules.get(match).equals(rule(fk.getAttribute("delete"))+"/"+rule(fk.getAttribute("update"))),table+" FK actions");}
    var indexes=new HashMap<String,TreeMap<Integer,String>>();try(var rs=metadata.getIndexInfo(null,schema,table,true,false)){while(rs.next()){String index=rs.getString("INDEX_NAME"),column=rs.getString("COLUMN_NAME");if(index!=null&&column!=null)indexes.computeIfAbsent(index,k->new TreeMap<>()).put(rs.getInt("ORDINAL_POSITION"),column);}}
    for(var u:children(t,"unique")){String wanted=names(u,"columnName");check(indexes.values().stream().anyMatch(index->String.join("|",index.values()).equals(wanted)),table+" UNIQUE "+wanted);}
   }
   for(var cmd:children(root,"execute")){boolean rejected=false;try(var st=c.createStatement()){st.execute(decode(cmd));}catch(SQLException ex){String state=ex.getSQLState();if(state!=null&&(state.startsWith("23")||state.startsWith("22")))rejected=true;else throw new Exception("Unexpected SQL error in "+decode(cmd)+" state="+state,ex);}check(rejected==Boolean.parseBoolean(cmd.getAttribute("reject")),name+" expected acceptance/rejection: "+decode(cmd));}
   if(name.equals("money")){check(scalar(c,"SELECT \"Quantita\" FROM \"RigaOrdine\" WHERE \"NumeroRiga\"=1").equals("123.4567"),"Money fractional digits");check(scalar(c,"SELECT \"Quantita\" FROM \"RigaOrdine\" WHERE \"NumeroRiga\"=2").equals("-999999999999999.9999"),"Money full range");check(scalar(c,"SELECT \"Quantita\" FROM \"RigaOrdine\" WHERE \"NumeroRiga\"=3").equals("-12.3456"),"Money default");}
   if(name.equals("ordini"))check(scalar(c,"SELECT MIN(\"NumeroOrdine\") FROM \"Ordine\"").equals("10"),"Identity starts at 10");
   if(name.equals("types-and-constraints")){check(scalar(c,"SELECT \"Amount\" FROM \"Child\" WHERE \"Id\"=1").equals("-12.30"),"Negative exact default");check(scalar(c,"SELECT \"Text\" FROM \"Child\" WHERE \"Id\"=1").equals("O'Brien; -- testo"),"Quoted string default");}
   if(name.equals("referential-actions"))check(scalar(c,"SELECT COUNT(*) FROM \"Child\" WHERE \"ParentA\" IS NULL AND \"ParentB\" IS NULL").equals("2"),"SET NULL clears entire composite FK");
   try(var st=c.createStatement()){st.execute("SHUTDOWN");}
  }
 }
 public static void main(String[] args)throws Exception{Class.forName("org.hsqldb.jdbcDriver");int cases=0;try(var files=Files.list(Path.of(args[0]))){for(var path:files.filter(p->p.getFileName().toString().startsWith("hsql-")&&p.toString().endsWith(".xml")).sorted().toList()){run(path);cases++;}}check(cases>=13,"Expected full fixture corpus");System.out.println("PASS: "+cases+" cases, "+checks+" checks");}
}
