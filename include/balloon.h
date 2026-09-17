#pragma once

#include <QColor>
#include <QPainter>
#include <QPoint>
#include <QRect>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QVector>

enum class BalloonKind {
  Say,
  Think,
  Whisper,
  Action,
};

struct Balloon {
  BalloonKind kind = BalloonKind::Say;
  QString speaker;
  QString text;
  QString displayText; // run text actually drawn (shortened if it won't fit)
  QString rest;        // text that spilled out of the frame (continuation panel)
  QRect rect;
  QRect sprite; // speaker sprite box in panel coords
  QPoint faceP; // speaker's face anchor in panel coords

  // Natural bubble size for the given free area (kept inside it).
  QSize measure(const QRect &freeRect) const;
  void layout(const QRect &slot, const QRect &spriteRect, const QPoint &facePt);
  // Truncate `text` to `box`, filling displayText and rest. True if it split.
  bool fitText(const QRect &box);
  void paint(QPainter *p) const;

private:
  // Lay the (uppercased) text into lines that fit `box`, storing the per-line
  // text and their boxes relative to box.topLeft(). Mirrors the original
  // client, whose balloon outline follows the ragged edges of these lines.
  void build(const QRect &box);

  QVector<QRect> m_lineRects; // line boxes, relative to rect.topLeft()
  QStringList m_lines;
};
