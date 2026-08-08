#include "emotionpicker.h"

#include <QFont>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QtMath>

namespace {
  constexpr int kEmoCount = 8; // same as NEMOTIONS in emotions.h
  constexpr qreal kPi = 3.14159265358979323846264338327950288;
  constexpr qreal kTwoPi = 2.0 * kPi;

  qreal normAngle(qreal a) {
    a = std::fmod(a, kTwoPi);
    if (a < 0)
      a += kTwoPi;
    return a;
  }
} // namespace

EmotionPicker::EmotionPicker(QWidget *parent) : QWidget(parent) {
  setMouseTracking(true);
  setMinimumSize(44, 44);
}

QPointF EmotionPicker::centre() const {
  return QPointF(width() / 2.0, height() / 2.0);
}

qreal EmotionPicker::radius() const {
  return qMax(qreal(8.0), qMin(width(), height()) / 2.0 - 16.0);
}

void EmotionPicker::setEmotion(const Emotion &e) {
  m_emotion.intensity = qBound(0.f, e.intensity, 1.f);
  m_emotion.emotion = float(qBound(0.0, normAngle(qreal(e.emotion)), kTwoPi));
  update();
}

QPointF EmotionPicker::knobPos() const {
  const qreal r = qMax(0.0, radius() * m_emotion.intensity);
  const QPointF c = centre();
  return c + QPointF(qCos(m_emotion.emotion), -qSin(m_emotion.emotion)) * r;
}

void EmotionPicker::updateFromPoint(const QPointF &p) {
  const QPointF c = centre();
  const QPointF v = p - c;
  const qreal r = radius();

  const qreal len = std::hypot(v.x(), v.y());
  const qreal intensity = qBound(0.0, len / r, 1.0);
  qreal angle = normAngle(std::atan2(-v.y(), v.x()));

  // Snap to the nearest of the eight named emotions for readability.
  const qreal step = kTwoPi / kEmoCount;
  const int idx = int(qRound(angle / step)) % kEmoCount;
  angle = idx * step;

  m_emotion.intensity = intensity < 0.05 ? 0.f : float(intensity);
  m_emotion.emotion = float(angle);

  update();
  emit emotionChanged(m_emotion);
}

void EmotionPicker::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    m_dragging = true;
    updateFromPoint(event->position());
    event->accept();
  }
}

void EmotionPicker::mouseMoveEvent(QMouseEvent *event) {
  if (m_dragging) {
    updateFromPoint(event->position());
    event->accept();
  }
}

void EmotionPicker::mouseReleaseEvent(QMouseEvent *event) {
  if (m_dragging && event->button() == Qt::LeftButton) {
    m_dragging = false;
    // Keep the chosen expression; the user can drag back to the centre to
    // clear.
    update();
    event->accept();
  }
}

void EmotionPicker::paintEvent(QPaintEvent *) {
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing, true);

  const QPointF c = centre();
  const qreal r = radius();

  p.setPen(QPen(QColor(120, 120, 120), 1));
  p.setBrush(QColor(255, 255, 255));
  p.drawEllipse(c, r, r);

  // Light cross lines.
  p.setPen(QPen(QColor(220, 220, 220), 1, Qt::DotLine));
  p.drawLine(QPointF(c.x() - r, c.y()), QPointF(c.x() + r, c.y()));
  p.drawLine(QPointF(c.x(), c.y() - r), QPointF(c.x(), c.y() + r));

  const qreal step = kTwoPi / kEmoCount;
  for (int i = 0; i < kEmoCount; ++i) {
    const qreal a = i * step;
    const QPointF pos = c + QPointF(qCos(a), -qSin(a)) * r;

    QPainterPath dot;
    dot.addEllipse(pos, 6, 6);
    p.fillPath(dot, QColor(70, 90, 160));

    const QPointF lp = c + QPointF(qCos(a), -qSin(a)) * (r + 15);
    QRectF lr(lp.x() - 36, lp.y() - 7, 72, 14);
    QFont f = p.font();
    f.setPointSizeF(7.5);
    f.setBold(true);
    p.setFont(f);
    p.setPen(QColor(20, 20, 20));
    p.drawText(lr, Qt::AlignCenter, emotionName(EmotionId(i + 1)));
  }

  // Centre.
  p.setPen(Qt::gray);
  p.setBrush(Qt::gray);
  p.drawEllipse(c, 3, 3);

  // Knob.
  const QPointF k = knobPos();
  p.setPen(QPen(Qt::black, 1.5));
  p.setBrush(QColor(40, 60, 160));
  p.drawEllipse(k, 9, 9);
  p.setBrush(Qt::white);
  p.drawEllipse(k, 3, 3);
}