#pragma once
#include "model.h"

struct RiseNote {QString it,en;};
struct RiseDiagram {QString name; QMap<QString,QPointF> centers;};
struct RiseImport {
 Model model;
 QString name,engine;
 QVector<RiseNote> notes;
 QVector<RiseDiagram> diagrams;
 QSet<QString> keyCandidates;
 int operations=0;
 Model positioned(int diagram) const;
 QString report(bool english=false) const;
};
// Atomic: a failed read never changes the output. No RISE runtime is used.
bool readRise(const QByteArray &bytes,RiseImport &out,QString &error,bool english=false);
bool loadRise(const QString &path,RiseImport &out,QString &error,bool english=false);
