#pragma once

#include "balloon.h"
#include "emotions.h"

#include <QImage>
#include <QString>
#include <QVector>

struct PanelCharacter {
  QString nick;
  QString avatarName;
  Emotion emotion;
  QImage body;       // rendered at original art resolution
  QImage bodyScaled; // laid-out as drawn (set by layout)
  QRect bodyRect;
  QPoint faceTip;   // face point in ORIGINAL image coordinates
  QPoint facePoint; // face point mapped into panel coordinates (set by layout)
  QString talkTo;   // nick this character is addressing (orient toward them)
  bool flip = false;
};

// Balloon text that spilled out of a frame, to be drawn as a continuation
// panel (original Comic Chat split long messages this way).
struct BalloonContinuation {
  BalloonKind kind;
  QString speaker;
  QString text;
};

struct ComicPanel {
  QImage backdrop;
  QVector<PanelCharacter> characters;
  QVector<Balloon> balloons;
  QVector<BalloonContinuation> leftovers;
  QSize size = QSize(320, 240);

  bool isEmpty() const { return characters.isEmpty(); }
  bool containsSpeaker(const QString &nick) const;
  void layout();
  QImage render() const;
};
