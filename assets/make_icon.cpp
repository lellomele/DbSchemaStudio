// Vector master for the application icon. Qt paints all raster sizes.
#include <QGuiApplication>
#include <QImage>
#include <QPainter>
#include <QDir>
int main(int argc,char **argv){
 QGuiApplication app(argc,argv);if(argc!=2)return 2;QDir dir(QString::fromLocal8Bit(argv[1]));if(!QDir().mkpath(dir.absolutePath()))return 3;
 for(int size:{16,20,24,32,40,48,64,128,256}){
  const int supersampling=4;QImage image(size*supersampling,size*supersampling,QImage::Format_ARGB32_Premultiplied);image.fill(Qt::transparent);
  QPainter p(&image);p.setRenderHint(QPainter::Antialiasing);p.scale(image.width()/256.,image.height()/256.);
  p.setPen(Qt::NoPen);p.setBrush(QColor("#17263d"));p.drawRoundedRect(QRectF(4,4,248,248),44,44);
  p.setPen(QPen(QColor("#eef5ff"),9,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
  p.drawLine(QPointF(126,128),QPointF(156,128));p.drawLine(QPointF(156,77),QPointF(156,181));p.drawLine(QPointF(156,77),QPointF(185,77));p.drawLine(QPointF(156,181),QPointF(185,181));
  p.setPen(Qt::NoPen);p.setBrush(QColor("#438cff"));p.drawRect(QRectF(28,67,104,123));p.drawEllipse(QRectF(28,171,104,38));
  p.setBrush(QColor("#87bdff"));p.drawEllipse(QRectF(28,48,104,38));
  p.setPen(QPen(QColor("#deedff"),5,Qt::SolidLine,Qt::RoundCap));p.setBrush(Qt::NoBrush);
  p.drawArc(QRectF(30,104,100,36),180*16,180*16);p.drawArc(QRectF(30,148,100,36),180*16,180*16);
  auto table=[&](QRectF r,QColor color){p.setPen(Qt::NoPen);p.setBrush(color);p.drawRoundedRect(r,10,10);p.setPen(QPen(QColor("#17263d"),size<=24?6:5,Qt::SolidLine,Qt::RoundCap));p.drawLine(r.topLeft()+QPointF(12,21),r.topRight()+QPointF(-12,21));if(size>20)p.drawLine(r.topLeft()+QPointF(12,38),r.topRight()+QPointF(-12,38));};
  table(QRectF(177,45,57,65),QColor("#ffc457"));table(QRectF(177,150,57,65),QColor("#45dfad"));p.end();
  auto target=image.scaled(size,size,Qt::IgnoreAspectRatio,Qt::SmoothTransformation);if(!target.save(dir.filePath(QString("app-%1.png").arg(size))))return 4;
 }
 return 0;
}
