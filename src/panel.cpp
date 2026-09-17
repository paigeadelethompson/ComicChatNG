#include "panel.h"

#include <QPainter>

bool ComicPanel::containsSpeaker(const QString &nick) const {
  for (const PanelCharacter &ch : characters) {
    if (ch.nick.compare(nick, Qt::CaseInsensitive) == 0)
      return true;
  }
  return false;
}

void ComicPanel::layout() {
  if (size.width() < 160)
    size = QSize(320, 240);

  const int n = characters.size();
  if (n == 0)
    return;

  leftovers.clear();

  // Scale each body to ~55% of panel height, then slot them across the
  // floor from left to right. Speakers face the panel center so the
  // characters in a conversation look at each other (original behavior).
  const int groundY = size.height() - 12;
  const int spacing = size.width() / (n + 1);
  const int maxH = int(size.height() * 0.55);

  for (int i = 0; i < n; ++i) {
    PanelCharacter &ch = characters[i];
    if (ch.body.isNull())
      continue;

    const QSize orig = ch.body.size();
    QImage scaled = ch.body;
    if (scaled.height() > maxH) {
      const qreal f = qreal(maxH) / scaled.height();
      scaled = scaled.scaled(qRound(scaled.width() * f), maxH,
                             Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    const int x = spacing * (i + 1) - scaled.width() / 2;
    const int y = groundY - scaled.height();
    ch.bodyScaled = scaled;
    ch.bodyRect = QRect(x, y, scaled.width(), scaled.height());

    // Facing: if this character was addressing someone ("T" annotation), turn
    // toward that character — the original oriented talk-to pairs at each
    // other (AddTalkTos/EvalPair). Otherwise fall back to mirroring the
    // right-hand character so a pair turns toward the panel center.
    const bool rightSide = ch.bodyRect.center().x() >= size.width() / 2;
    bool flip = n > 1 && rightSide;
    if (!ch.talkTo.isEmpty()) {
      for (int j = 0; j < n; ++j) {
        if (j == i)
          continue;
        if (characters[j].nick.compare(ch.talkTo, Qt::CaseInsensitive) == 0) {
          flip = j < i; // target to our left → face left (mirrored)
          break;
        }
      }
    }
    ch.flip = flip;

    // Map the face tip through the same scale (and flip) into panel space.
    const qreal fx =
        scaled.width() ? qreal(scaled.width()) / orig.width() : 0.0;
    const qreal fy =
        scaled.height() ? qreal(scaled.height()) / orig.height() : 0.0;
    if (ch.faceTip.x() >= 0 && ch.faceTip.y() >= 0 && fx > 0 && fy > 0) {
      int tipX = qRound(ch.faceTip.x() * fx);
      if (ch.flip)
        tipX = scaled.width() - tipX;
      ch.facePoint = QPoint(ch.bodyRect.left() + tipX,
                            ch.bodyRect.top() + qRound(ch.faceTip.y() * fy));
    } else {
      ch.facePoint = QPoint(ch.bodyRect.center().x(),
                            ch.bodyRect.top() + ch.bodyRect.height() / 4);
    }
  }

  // Balloons are confined to the free band above every character: the original
  // Comic Chat always drew its bubbles in the top part of a frame, never over a
  // body, so a bubble can't grow over its own or another speaker.
  const int nbBalloons = balloons.size();
  if (nbBalloons == 0)
    return;

  const int gutter = 8;
  const int freeTop = 6;
  int freeBottom = size.height() / 2;
  for (const PanelCharacter &ch : characters) {
    if (!ch.bodyRect.isEmpty())
      freeBottom = qMin(freeBottom, ch.bodyRect.top() - gutter);
  }
  freeBottom = qMax(freeTop + 24, freeBottom);
  const QRect free(QPoint(6, freeTop), QPoint(size.width() - 7, freeBottom));

  struct BalloonSlot {
    int index;
    QSize size;
    QRect owner;
    QPoint face;
  };
  QVector<BalloonSlot> items;
  items.reserve(nbBalloons);

  for (int i = 0; i < nbBalloons; ++i) {
    const Balloon &b = balloons[i];
    QRect owner;
    QPoint face;
    for (const PanelCharacter &ch : characters) {
      if (ch.nick.compare(b.speaker, Qt::CaseInsensitive) == 0) {
        owner = ch.bodyRect;
        face = ch.facePoint;
        break;
      }
    }
    if (owner.isEmpty() && !characters.isEmpty()) {
      owner = characters.first().bodyRect;
      face = characters.first().facePoint;
    }
    if (face.isNull() && !owner.isEmpty())
      face = QPoint(owner.center().x(), owner.top() + owner.height() / 4);
    if (owner.isEmpty())
      owner = QRect(size.width() / 2 - 40, size.height() - 80, 80, 40);

    BalloonSlot slot;
    slot.index = i;
    slot.owner = owner;
    slot.face = face;
    slot.size = b.measure(free);
    if (slot.size.width() > free.width())
      slot.size.setWidth(free.width());
    if (slot.size.height() > free.height())
      slot.size.setHeight(free.height());
    items.append(slot);
  }

  // Pack the bubbles side by side into shelves; each bubble keeps its own slot
  // (a column of the free area) so bubbles in a frame never overlap each other.
  QVector<QVector<int>> rows;
  rows.push_back({});
  int usedW = 0;
  for (int i = 0; i < items.size(); ++i) {
    const int w = items[i].size.width();
    if (!rows.last().isEmpty() && usedW + gutter + w > free.width()) {
      rows.push_back({});
      usedW = w;
    } else if (rows.last().isEmpty()) {
      usedW = w;
    } else {
      usedW += gutter + w;
    }
    rows.last().push_back(i);
  }

  const int rowBase = free.height() / rows.size();
  int y = free.top();
  for (int r = 0; r < rows.size(); ++r) {
    const int rowH = (r == rows.size() - 1) ? free.bottom() - y + 1 : rowBase;
    int rowW = (rows[r].size() - 1) * gutter;
    for (int idx : rows[r])
      rowW += items[idx].size.width();
    int x = free.left() + (free.width() - rowW) / 2;
    for (int q = 0; q < rows[r].size(); ++q) {
      const int idx = rows[r][q];
      Balloon &b = balloons[items[idx].index];
      const int h = qMin(rowH, items[idx].size.height());
      b.layout(QRect(x, y, items[idx].size.width(), h), items[idx].owner,
               items[idx].face);
      if (!b.rest.isEmpty()) {
        BalloonContinuation c;
        c.kind = b.kind;
        c.speaker = b.speaker;
        c.text = b.rest;
        leftovers.append(c);
      }
      x += items[idx].size.width() + gutter;
    }
    y += rowH;
  }
}

QImage ComicPanel::render() const {
  QImage img(size, QImage::Format_ARGB32);
  img.fill(QColor(245, 245, 240));

  QPainter p(&img);
  p.setRenderHint(QPainter::SmoothPixmapTransform, true);
  p.setRenderHint(QPainter::Antialiasing, true);

  if (!backdrop.isNull())
    p.drawImage(QRect(QPoint(0, 0), size), backdrop);
  else {
    QLinearGradient g(0, 0, 0, size.height());
    g.setColorAt(0, QColor(180, 210, 240));
    g.setColorAt(1, QColor(220, 200, 160));
    p.fillRect(img.rect(), g);
  }

  // Panel border
  p.setPen(QPen(Qt::black, 3));
  p.setBrush(Qt::NoBrush);
  p.drawRect(img.rect().adjusted(1, 1, -2, -2));

  for (const PanelCharacter &ch : characters) {
    if (ch.bodyScaled.isNull())
      continue;
    if (ch.flip)
      p.drawImage(ch.bodyRect.topLeft(), ch.bodyScaled.flipped(Qt::Horizontal));
    else
      p.drawImage(ch.bodyRect.topLeft(), ch.bodyScaled);
  }

  for (const Balloon &b : balloons)
    b.paint(&p);

  p.end();
  return img;
}