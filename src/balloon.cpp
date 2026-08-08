#include "balloon.h"

#include <QFontMetrics>
#include <QPainterPath>

#include <cmath>

void Balloon::layout(const QRect &panelRect, const QRect &spriteRect,
                     const QPoint &facePt, int maxWidth) {
  sprite = spriteRect;
  faceP = facePt.isNull()
              ? QPoint(sprite.center().x(), sprite.top() + sprite.height() / 4)
              : facePt;
  QFont font(QStringLiteral("Comic Sans MS"), 10);
  if (!QFont(font).exactMatch())
    font = QFont(QStringLiteral("DejaVu Sans"), 10);
  QFontMetrics fm(font);

  const QString wrapped = fm.elidedText(text, Qt::ElideRight, maxWidth * 4);
  QRect br = fm.boundingRect(QRect(0, 0, maxWidth, 1000),
                             Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop,
                             text.isEmpty() ? wrapped : text);
  br.adjust(-10, -8, 10, 8);

  // Sit the balloon over the speaker's head, hugging it loosely so the
  // tail stays short (same as the original comic).
  QPoint topLeft(
      qBound(8, faceP.x() - br.width() / 2, panelRect.width() - 8 - br.width()),
      faceP.y() - br.height() - 10);
  topLeft.setY(
      qBound(6, topLeft.y(), qMax(6, panelRect.height() - 8 - br.height())));

  rect = QRect(topLeft, br.size());
}

void Balloon::paint(QPainter *p) const {
  p->save();
  QFont font(QStringLiteral("Comic Sans MS"), 10);
  if (!QFont(font).exactMatch())
    font = QFont(QStringLiteral("DejaVu Sans"), 10);
  p->setFont(font);

  QPainterPath path;
  if (kind == BalloonKind::Think)
    path.addRoundedRect(rect, 18, 18);
  else if (kind == BalloonKind::Action)
    path.addRect(rect);
  else
    path.addRoundedRect(rect, 12, 12);

  QColor fill = Qt::white;
  if (kind == BalloonKind::Whisper)
    fill = QColor(255, 255, 200);
  else if (kind == BalloonKind::Action)
    fill = QColor(240, 240, 255);

  p->setPen(QPen(Qt::black, kind == BalloonKind::Whisper ? 1 : 2,
                 kind == BalloonKind::Whisper ? Qt::DashLine : Qt::SolidLine));
  p->setBrush(fill);
  p->drawPath(path);

  // Short tail from the bubble to the speaker's face/head. Both endpoints
  // stay at the sprite, so the line never stretches across a whole body.
  if (kind != BalloonKind::Action && !sprite.isNull()) {
    const QPointF head = QPointF(faceP);
    const QPointF bodyTop =
        QPointF(sprite.left() + sprite.width() / 2, sprite.top() + 2);

    // The bubble edge that looks toward the face.
    QPointF base = QPointF(rect.center().x(), rect.bottom());
    const bool headAbove = head.y() < rect.top();
    if (headAbove)
      base = QPointF(rect.center().x(), rect.top());

    if (kind == BalloonKind::Think) {
      const qreal d =
          std::hypot(head.x() - bodyTop.x(), head.y() - bodyTop.y());
      const int dots = d > 0 ? int(d / 14) + 1 : 2;
      for (int i = 1; i <= dots; ++i) {
        const qreal f = qreal(i) / (dots + 1);
        const qreal r = i == dots ? 3.0 : 2.5;
        p->drawEllipse(QPointF(base + (bodyTop - base) * f), r, r);
      }
      for (int i = 1; i <= 3; ++i) {
        const qreal f = qreal(i) / 4;
        const qreal r = i == 3 ? 3.0 : 2.0;
        p->drawEllipse(
            QPointF(rect.center() + (head - QPointF(rect.center())) * f), r, r);
      }
    } else {
      // Arrow: from the bubble edge nearest the character, stop a little
      // short of the face so it doesn't cover the sprite.
      QPointF tip = head;
      const qreal d = std::hypot(head.x() - base.x(), head.y() - base.y());
      if (d > 1.0) {
        const QPointF u = (head - base) / d;
        tip = head - u * 6.0;
      }
      QPolygonF poly;
      poly << base + QPointF(-5, 0) << base + QPointF(5, 0) << tip;
      p->setPen(QPen(Qt::black, 1.5));
      p->setBrush(fill);
      p->drawPolygon(poly);
    }
  }

  p->setPen(Qt::black);
  p->drawText(rect.adjusted(8, 6, -8, -6),
              Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop, text);
  p->restore();
}