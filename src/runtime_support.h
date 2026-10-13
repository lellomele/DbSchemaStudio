#pragma once
#include "model.h"
#include <cstdio>

// A widget may still be delivering the click that requested its replacement.
// Remove it from searches/layouts now, but release it after event delivery.
static void retireWidget(QWidget *widget){
 if(!widget)return;
 widget->hide();widget->setEnabled(false);widget->setParent(nullptr);widget->deleteLater();
}

// Identifiers may have no spaces. Wrap them without changing the stored text.
class WrappedLabel:public QLabel {
 void layoutText(QTextDocument &document,int width)const{
  document.setDocumentMargin(0);document.setDefaultFont(font());
  QTextOption option;option.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
  document.setDefaultTextOption(option);document.setPlainText(text());document.setTextWidth(qMax(1,width));
 }
public:
 explicit WrappedLabel(const QString &text):QLabel(text){setWordWrap(true);setTextFormat(Qt::PlainText);}
 QSize minimumSizeHint()const override{return {0,fontMetrics().height()};}
 int heightForWidth(int width)const override{QTextDocument document;layoutText(document,width);return qCeil(document.size().height());}
 void paintEvent(QPaintEvent *)override{
  QTextDocument document;layoutText(document,contentsRect().width());QPainter painter(this);painter.translate(contentsRect().topLeft());
  QAbstractTextDocumentLayout::PaintContext context;context.palette=palette();context.palette.setColor(QPalette::Text,palette().color(QPalette::WindowText));document.documentLayout()->draw(&painter,context);
 }
};

// Atomic copies keep the previous file intact when writing or committing fails.
static bool atomicCopy(const QString &source,const QString &target,QString &error){
 QFile input(source);if(!input.open(QIODevice::ReadOnly)){error=input.errorString();return false;}
 if(input.size()>20*1024*1024){error="File too large / File troppo grande (20 MB)";return false;}
 QSaveFile output(target);if(!output.open(QIODevice::WriteOnly)){error=output.errorString();return false;}
 while(!input.atEnd()){auto bytes=input.read(65536);if(input.error()!=QFile::NoError||output.write(bytes)!=bytes.size()){error=input.error()!=QFile::NoError?input.errorString():output.errorString();return false;}}
 if(!output.commit()){error=output.errorString();return false;}return true;
}
static bool saveWithBackup(const QString &path,const Model &model,QString &error){
 if(QFile::exists(path)&&!atomicCopy(path,path+".bak",error))return false;
 return saveModel(path,model,error);
}
static QString diagnosticPath;
static void diagnosticMessage(QtMsgType type,const QMessageLogContext &,const QString &message){
 static QMutex mutex;QMutexLocker guard(&mutex);
 if(QFileInfo(diagnosticPath).size()>1024*1024){QFile::remove(diagnosticPath+".previous");QFile::rename(diagnosticPath,diagnosticPath+".previous");}
 QFile file(diagnosticPath);if(file.open(QIODevice::WriteOnly|QIODevice::Append))file.write((QDateTime::currentDateTimeUtc().toString(Qt::ISODate)+" "+QString::number(int(type))+" "+message.left(4000)+"\n").toUtf8());
 if(type==QtFatalMsg)std::abort();
}
static void initializeDiagnostics(const QString &directory){
 diagnosticPath=directory+"/diagnostics.log";
 if(QFileInfo(diagnosticPath).size()>1024*1024){QFile::remove(diagnosticPath+".previous");QFile::rename(diagnosticPath,diagnosticPath+".previous");}
 qInstallMessageHandler(diagnosticMessage);
}

class DialogSizer:public QObject {
public:
 explicit DialogSizer(QObject *parent):QObject(parent){}
 bool eventFilter(QObject *object,QEvent *event)override{
  if(event->type()==QEvent::Show)if(auto dialog=qobject_cast<QDialog*>(object))if(auto screen=dialog->screen()){
   const auto available=screen->availableGeometry().size()-QSize(32,80);
   dialog->setMaximumSize(available);dialog->resize(dialog->size().boundedTo(available));
  }
  return false;
 }
};
