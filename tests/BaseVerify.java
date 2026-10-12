import java.nio.file.*;
import java.util.*;
import javax.xml.parsers.*;
import org.w3c.dom.*;
import com.sun.star.uno.UnoRuntime;
import com.sun.star.uno.XComponentContext;
import com.sun.star.bridge.XUnoUrlResolver;
import com.sun.star.beans.XPropertySet;
import com.sun.star.beans.PropertyValue;
import com.sun.star.frame.XStorable;
import com.sun.star.frame.XDesktop;
import com.sun.star.sdb.XOfficeDatabaseDocument;
import com.sun.star.sdbc.XDataSource;
import com.sun.star.sdbc.XConnection;

// Dedicated headless LibreOffice profile and temporary .odb, never the user's documents/profile.
public class BaseVerify {
 static <T>T qi(Class<T> type,Object value){return UnoRuntime.queryInterface(type,value);}
 public static void main(String[] args)throws Exception{
  Path run=Files.createTempDirectory("DbSchemaStudio-Base090-");String pipe="dbschema"+UUID.randomUUID().toString().replace("-","");
  var process=new ProcessBuilder(args[0],"--headless","--norestore","--nodefault","--nofirststartwizard","-env:UserInstallation="+run.resolve("profile").toUri(),"--accept=pipe,name="+pipe+";urp;StarOffice.ComponentContext").redirectErrorStream(true).redirectOutput(run.resolve("office.log").toFile()).start();
  XComponentContext context=null;XConnection connection=null;Object document=null;
  try{
   var local=com.sun.star.comp.helper.Bootstrap.createInitialComponentContext(null);
   var resolver=qi(XUnoUrlResolver.class,local.getServiceManager().createInstanceWithContext("com.sun.star.bridge.UnoUrlResolver",local));
   long deadline=System.currentTimeMillis()+25000;while(context==null&&System.currentTimeMillis()<deadline){try{context=qi(XComponentContext.class,resolver.resolve("uno:pipe,name="+pipe+";urp;StarOffice.ComponentContext"));}catch(com.sun.star.connection.NoConnectException ex){Thread.sleep(200);}}
   if(context==null)throw new Exception("LibreOffice connection timeout");
   var manager=context.getServiceManager();document=manager.createInstanceWithContext("com.sun.star.sdb.OfficeDatabaseDocument",context);
   var db=qi(XOfficeDatabaseDocument.class,document);var ds=qi(XPropertySet.class,db.getDataSource());ds.setPropertyValue("URL","sdbc:embedded:hsqldb");
   qi(XStorable.class,document).storeAsURL(run.resolve("verification.odb").toUri().toString(),new PropertyValue[0]);
   connection=qi(XDataSource.class,db.getDataSource()).getConnection("SA","");
   System.out.println("Base SDBC: "+connection.getMetaData().getDatabaseProductName()+" "+connection.getMetaData().getDatabaseProductVersion());
   var factory=DocumentBuilderFactory.newInstance();factory.setFeature("http://apache.org/xml/features/disallow-doctype-decl",true);var root=factory.newDocumentBuilder().parse(Path.of(args[1]).toFile()).getDocumentElement();int count=0;
   for(Node n=root.getFirstChild();n!=null;n=n.getNextSibling())if(n instanceof Element element&&element.getTagName().equals("statement")){String sql=new String(Base64.getDecoder().decode(element.getTextContent()),java.nio.charset.StandardCharsets.UTF_8);connection.createStatement().execute(sql);count++;}
   connection.createStatement().execute("INSERT INTO \"Ordine\" (\"Data\") VALUES ('2026-09-20')");connection.createStatement().execute("INSERT INTO \"RigaOrdine\" VALUES (1,2,10)");
   var rs=connection.createStatement().executeQuery("SELECT COUNT(*) FROM \"RigaOrdine\"");if(!rs.next()||qi(com.sun.star.sdbc.XRow.class,rs).getInt(1)!=1)throw new Exception("Base row check failed");
   qi(XStorable.class,document).store();connection.close();connection=null;
   qi(com.sun.star.util.XCloseable.class,document).close(true);document=null;
   // Reload the saved .odb to verify persistence, not just an in-memory connection.
   var loader=qi(com.sun.star.frame.XComponentLoader.class,manager.createInstanceWithContext("com.sun.star.frame.Desktop",context));PropertyValue hidden=new PropertyValue();hidden.Name="Hidden";hidden.Value=true;
   document=loader.loadComponentFromURL(run.resolve("verification.odb").toUri().toString(),"_blank",0,new PropertyValue[]{hidden});db=qi(XOfficeDatabaseDocument.class,document);connection=db.getDataSource().getConnection("SA","");rs=connection.createStatement().executeQuery("SELECT COUNT(*) FROM \"RigaOrdine\"");if(!rs.next()||qi(com.sun.star.sdbc.XRow.class,rs).getInt(1)!=1)throw new Exception("Base persistence failed");
   System.out.println("PASS: "+count+" DDL statements via Base SDBC; identity, FK, save and reopen. "+run);
  }finally{
   if(connection!=null)connection.close();if(document!=null)qi(com.sun.star.util.XCloseable.class,document).close(true);
   if(context!=null)qi(XDesktop.class,context.getServiceManager().createInstanceWithContext("com.sun.star.frame.Desktop",context)).terminate();
   if(!process.waitFor(5,java.util.concurrent.TimeUnit.SECONDS))process.destroy();
  }
 }
}
