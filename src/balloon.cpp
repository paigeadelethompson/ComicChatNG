#include "balloon.h"

#include <QFont>
#include <QFontMetrics>
#include <QPainterPath>
#include <QPointF>
#include <QStringList>
#include <QVector>

#include <cmath>

namespace {

QFont comicFont(bool italic = false) {
  // Original: IDS_DFLT_COMICSPNTSIZE = 12, IDS_COMICS_BOLD_DFLT = 700.
  QFont f(QStringLiteral("Comic Sans MS"), 12, QFont::Bold);
  if (!QFont(f).exactMatch())
    f = QFont(QStringLiteral("DejaVu Sans"), 12, QFont::Bold);
  f.setItalic(italic);
  return f;
}

// The original's balloon geometry was expressed in units against a ~187-unit
// line height: XBORDER=100, YBORDER=40, TOPBORDER=-20, waves 70/300, pen 28.
// These helpers keep the same proportions relative to the current line height.
int balloonPadX(const QFontMetrics &fm) {
  return qMax(5, qRound(fm.height() * 0.53));
}
int balloonPadY(const QFontMetrics &fm) {
  return qMax(2, qRound(fm.height() * 0.21));
}
int balloonTopPad(const QFontMetrics &fm) {
  return qMax(1, qRound(fm.height() * 0.11));
}
qreal balloonWaveAmp(const QFontMetrics &fm) {
  return qBound(qreal(2.5), fm.height() * 0.37, qreal(10.0));
}
qreal balloonWaveLen(const QFontMetrics &fm) {
  return qBound(qreal(16.0), fm.height() * 1.6, qreal(44.0));
}
qreal balloonPenW(const QFontMetrics &fm) {
  return qMax(qreal(2.0), fm.height() * 0.15);
}
// Extra margin the wavy, thick-stroked outline needs beyond the text centerline.
int balloonOver(const QFontMetrics &fm) {
  return int(std::ceil(balloonWaveAmp(fm) + balloonPenW(fm) / 2.0)) + 1;
}

// Greedy word wrap using the same metric that drawing uses.
QStringList wrapText(const QFontMetrics &fm, const QString &text, int maxWidth) {
  QStringList out;
  const QStringList paragraphs = text.split(QLatin1Char('\n'));
  for (const QString &para : paragraphs) {
    const QStringList words = para.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (words.isEmpty()) {
      out.append(QString());
      continue;
    }
    QString cur;
    for (QString w : words) {
      while (maxWidth > 1 && fm.horizontalAdvance(w) > maxWidth) {
        if (!cur.isEmpty()) {
          out.append(cur);
          cur.clear();
        }
        int lo = 1, hi = w.size();
        while (lo < hi) {
          const int mid = (lo + hi + 1) / 2;
          if (fm.horizontalAdvance(w.left(mid)) <= maxWidth)
            lo = mid;
          else
            hi = mid - 1;
        }
        out.append(w.left(lo));
        w = w.mid(lo);
      }
      const QString cand = cur.isEmpty() ? w : cur + QLatin1Char(' ') + w;
      if (cur.isEmpty() || fm.horizontalAdvance(cand) <= maxWidth)
        cur = cand;
      else {
        out.append(cur);
        cur = w;
      }
    }
    if (!cur.isEmpty())
      out.append(cur);
  }
  if (out.isEmpty())
    out.append(QString());
  return out;
}

// Sprinkle points that make an edge undulate; the caller smooths them into the
// wavy border. Mirrors AddWavies() in the original client, where the balloon
// outline was a single beta-spline with the tail inserted into it.
void addWavies(QVector<QPointF> &pts, const QPointF &a, const QPointF &b,
               qreal amp, qreal interval) {
  const qreal dist = std::hypot(b.x() - a.x(), b.y() - a.y());
  if (dist < 1.0 || amp <= 0.0 || interval <= 1.0 || dist / interval < 2.0) {
    pts.append(b);
    return;
  }
  const int iWaves = int(dist / interval);
  const qreal waveLen = dist / iWaves;
  const QPointF unit((b.x() - a.x()) / dist, (b.y() - a.y()) / dist);
  const QPointF inc = unit * waveLen;
  // The original's normal was (unit.y, -unit.x), but its y axis points up. Qt's
  // points down, so flip the normal to keep the wavies bulging *outward*.
  const QPointF extra(-unit.y() * amp, unit.x() * amp);
  QPointF base = a;
  for (int i = 0; i < iWaves - 1; ++i) {
    base += inc;
    pts.append((i & 1) ? base : base + extra);
  }
  pts.append(b);
}

// Port of CBeta (tension = 5.0, bias = 1.0) from the original spline.cpp. The
// old balloon outline is a *closed beta-spline*, which approximates its control
// points: it rounds off the corners and the wave tips instead of running
// through them the way Catmull-Rom does. Using the same matrix is what makes
// the bubble read as the soft Comic Chat cloud.
QPainterPath betaClosed(const QVector<QPointF> &cps) {
  QPainterPath path;
  const int n = cps.size();
  if (n < 3)
    return path;

  const double tension = 5.0, bias = 1.0;
  const double b2 = bias * bias, b3 = bias * b2;
  const double d = 1.0 / (tension + 2.0 * b3 + 4.0 * (b2 + bias) + 2.0);
  double m[4][4];
  m[0][0] = -2.0 * b3;
  m[0][1] = 2.0 * (tension + b3 + b2 + bias);
  m[0][2] = -2.0 * (tension + b2 + bias + 1.0);
  m[0][3] = 2.0;
  m[1][0] = 6.0 * b3;
  m[1][1] = -3.0 * (tension + 2.0 * (b3 + b2));
  m[1][2] = 3.0 * (tension + 2.0 * b2);
  m[1][3] = 0.0;
  m[2][0] = -6.0 * b3;
  m[2][1] = 6.0 * (b3 - bias);
  m[2][2] = 6.0 * bias;
  m[2][3] = 0.0;
  m[3][0] = 2.0 * b3;
  m[3][1] = tension + 4.0 * (b2 + bias);
  m[3][2] = 2.0;
  m[3][3] = 0.0;
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j)
      m[i][j] *= d;

  // GetKnot() for a closed spline (CSpline::GetKnot in the original).
  auto knot = [&](int index) -> QPointF {
    if (index == 0)
      return cps[n - 1];
    if (index == n + 1)
      return cps[0];
    if (index == n + 2)
      return cps[1];
    return cps[index - 1];
  };
  auto row = [&](int r, const QPointF &k0, const QPointF &k1, const QPointF &k2,
                 const QPointF &k3) -> QPointF {
    return QPointF(
        qRound(m[r][0] * k0.x() + m[r][1] * k1.x() + m[r][2] * k2.x() +
               m[r][3] * k3.x()),
        qRound(m[r][0] * k0.y() + m[r][1] * k1.y() + m[r][2] * k2.y() +
               m[r][3] * k3.y()));
  };

  const int nKnots = n + 3; // CSpline::KnotCount() when closed
  QPointF k0 = knot(0), k1 = knot(1), k2 = knot(2), k3 = knot(3);
  for (int i = 0;; ++i) {
    const QPointF c0 = row(3, k0, k1, k2, k3);
    const QPointF c1 = row(2, k0, k1, k2, k3);
    const QPointF c2 = row(1, k0, k1, k2, k3);
    const QPointF c3 = row(0, k0, k1, k2, k3);
    const QPointF b1 = c0 + c1 / 3.0;
    const QPointF b2 = b1 + (c1 + c2) / 3.0;
    const QPointF b3 = c0 + c1 + c2 + c3;
    if (i == 0)
      path.moveTo(c0);
    path.cubicTo(b1, b2, b3);
    if (i + 4 == nKnots)
      break;
    k0 = k1;
    k1 = k2;
    k2 = k3;
    k3 = knot(i + 4);
  }
  path.closeSubpath();
  return path;
}

// A run of consecutive text lines whose edge x stays (roughly) put.
struct HRun {
  int x;
  int start;
  int end;
};

// Port of GetFilters(): merge line edges into runs so the outline steps only on
// dramatic indents instead of jittering on every line.
QVector<HRun> makeRuns(const QVector<int> &edge, bool left, int lineH) {
  const int t1 = -qRound(lineH * 0.41);
  const int t2 = qRound(lineH * 0.41);
  QVector<HRun> runs;
  runs.append({edge[0], 0, 0});
  for (int i = 1; i < edge.size(); ++i) {
    const int cur = runs.last().x;
    const int delta = edge[i] - cur;
    if (left) {
      if (delta <= t1) {
        runs.last().end = i - 1;
        runs.append({edge[i], i, i});
      } else if (delta <= 0) {
        runs.last().x = edge[i];
      } else if (delta >= t2) {
        const int next = (i + 1 < edge.size()) ? edge[i + 1] : edge[i];
        if (next - runs.last().x >= t2) {
          runs.last().end = i - 1;
          runs.append({qMin(edge[i], next), i, i});
        }
      }
    } else {
      if (delta >= -t1) {
        runs.last().end = i - 1;
        runs.append({edge[i], i, i});
      } else if (delta >= 0) {
        runs.last().x = edge[i];
      } else if (delta <= -t2) {
        const int next = (i + 1 < edge.size()) ? edge[i + 1] : edge[i];
        if (next - runs.last().x <= -t2) {
          runs.last().end = i - 1;
          runs.append({qMax(edge[i], next), i, i});
        }
      }
    }
  }
  runs.last().end = edge.size() - 1;
  return runs;
}

} // namespace

void Balloon::build(const QRect &box) {
  const bool italic = (kind == BalloonKind::Whisper);
  const QFontMetrics fm(comicFont(italic));
  const int lineH = qMax(1, fm.height());
  const int padX = balloonPadX(fm);
  const int padY = balloonPadY(fm);
  const int topPad = balloonTopPad(fm);
  const int over = balloonOver(fm);

  const QString up = text.toUpper();
  const int maxTextW = qMax(16, box.width() - 2 * padX - 2 * over);
  const int tw = qMin(qMax(16, fm.horizontalAdvance(up)), maxTextW);
  QStringList all = wrapText(fm, up, tw);

  const int maxTextH = qMax(lineH, box.height() - 2 * over - topPad - padY);
  const int maxLines = qMax(1, maxTextH / lineH);
  if (all.size() > maxLines) {
    const QStringList restLines = all.mid(maxLines);
    all = all.mid(0, maxLines);
    all.last() += QStringLiteral("...");
    rest = QStringLiteral("...") + restLines.join(QLatin1Char(' '));
  } else {
    rest.clear();
  }

  m_lines = all;
  m_lineRects.clear();
  const bool leftJustify = (kind == BalloonKind::Action);
  for (int i = 0; i < all.size(); ++i) {
    const int lw = qMin(tw, fm.horizontalAdvance(all[i]));
    const int lx = leftJustify ? 0 : (tw - lw) / 2;
    m_lineRects.append(
        QRect(over + padX + lx, over + topPad + i * lineH, lw, lineH));
  }
  displayText = all.join(QLatin1Char('\n'));
}

QSize Balloon::measure(const QRect &freeRect) const {
  const bool italic = (kind == BalloonKind::Whisper);
  const QFontMetrics fm(comicFont(italic));
  const int lineH = qMax(1, fm.height());
  const int padX = balloonPadX(fm);
  const int padY = balloonPadY(fm);
  const int topPad = balloonTopPad(fm);
  const int over = balloonOver(fm);

  const QString up = text.toUpper();
  const int maxTextW = qMax(16, freeRect.width() - 2 * padX - 2 * over);
  const int tw = qMin(qMax(16, fm.horizontalAdvance(up)), maxTextW);
  QStringList all = wrapText(fm, up, tw);

  const int maxTextH = qMax(lineH, freeRect.height() - 2 * over - topPad - padY);
  const int maxLines = qMax(1, maxTextH / lineH);
  if (all.size() > maxLines)
    all = all.mid(0, maxLines);

  int maxLineW = 0;
  for (const QString &l : all)
    maxLineW = qMax(maxLineW, fm.horizontalAdvance(l));
  const int textW = qMax(tw, maxLineW);
  const int outerW = textW + 2 * padX + 2 * over;
  const int outerH = all.size() * lineH + topPad + padY + 2 * over;
  return QSize(qMin(freeRect.width(), outerW), qMin(freeRect.height(), outerH));
}

void Balloon::layout(const QRect &slot, const QRect &spriteRect,
                     const QPoint &facePt) {
  sprite = spriteRect;
  faceP = facePt.isNull()
              ? QPoint(sprite.center().x(), sprite.top() + sprite.height() / 4)
              : facePt;

  const QSize s = measure(slot);
  const int left = slot.left() + 2;
  const int right = qMax(left, slot.left() + slot.width() - 2 - s.width());
  const int x = qBound(left, faceP.x() - s.width() / 2, right);
  rect = QRect(x, slot.top(), s.width(), s.height());
  build(rect);
}

bool Balloon::fitText(const QRect &box) {
  build(box);
  return !rest.isEmpty();
}

void Balloon::paint(QPainter *p) const {
  p->save();

  const bool isAction = kind == BalloonKind::Action;
  const bool isThink = kind == BalloonKind::Think;
  const bool isWhisper = kind == BalloonKind::Whisper;

  p->setFont(comicFont(isWhisper));
  const QFontMetrics fm(comicFont(isWhisper));
  const qreal amp = balloonWaveAmp(fm);
  const qreal wl = balloonWaveLen(fm);
  const qreal penW = balloonPenW(fm);
  const int padX = balloonPadX(fm);
  const int over = balloonOver(fm);
  const int topPad = balloonTopPad(fm);
  const int lineH = qMax(1, fm.height());

  QColor fill = Qt::white;
  if (isWhisper)
    fill = QColor(255, 255, 200);

  if (isAction) {
    // "Action" bubbles are plain rectangles (the original CBWoodringBox).
    const QRectF r = QRectF(rect).adjusted(penW / 2, penW / 2, -penW / 2,
                                           -penW / 2);
    p->setPen(QPen(Qt::black, penW, Qt::SolidLine, Qt::RoundCap,
                   Qt::RoundJoin));
    p->setBrush(fill);
    p->drawRect(r);
  } else {
    // Build the closed outline that hugs the ragged text edges, then insert the
    // tail into it — a port of CreateBalloonSpline() + AddArrow().
    const int n = qMin(m_lines.size(), m_lineRects.size());
    if (n > 0) {
      QVector<int> leftEdge(n), rightEdge(n);
      for (int i = 0; i < n; ++i) {
        leftEdge[i] = rect.left() + m_lineRects[i].left() - padX;
        rightEdge[i] = rect.left() + m_lineRects[i].right() + padX;
      }
      const qreal topY = rect.top() + over;
      const qreal bottomY = rect.bottom() - over;
      auto lineTop = [&](int i) {
        return qreal(rect.top() + over + topPad + i * lineH);
      };
      QVector<QPointF> pts;
      auto wavy = [&](qreal x, qreal y) {
        const QPointF to(x, y);
        if (pts.isEmpty()) {
          pts.append(to);
          return;
        }
        if (std::abs(to.x() - pts.last().x()) <= 0.01 &&
            std::abs(to.y() - pts.last().y()) <= 0.01)
          return;
        addWavies(pts, pts.last(), to, amp, wl);
      };

      const QVector<HRun> lr = makeRuns(leftEdge, true, lineH);
      const QVector<HRun> rr = makeRuns(rightEdge, false, lineH);
      const qreal bLeft = leftEdge[n - 1];
      const qreal bRight = rightEdge[n - 1];

      // The tail is an opening cut into the bottom edge (the original's
      // BreakSpline()): the outline runs from one lip of the gap, all the way
      // around the balloon, to the other lip, then out to the tail point. That
      // keeps the point sharp and leaves no line across the base of the tail.
      const bool wantTail = !isThink && !sprite.isNull() && sprite.width() > 0;
      qreal gapLeft = bLeft, gapRight = bRight;
      QPointF tip;
      if (wantTail) {
        const qreal span = qMax(qreal(1.0), bRight - bLeft);
        const qreal xb =
            qBound(bLeft + span * 0.15, qreal(faceP.x()), bRight - span * 0.15);
        // Original gap width = 80 units against a ~187-unit line height.
        const qreal halfGap = qMax(qreal(3.0), lineH * 0.43);
        gapLeft = qMax(bLeft, xb - halfGap);
        gapRight = qMin(bRight, xb + halfGap);

        // Aim at the speaker's face, clamped to 45 degrees, with the original
        // MINTAILHEIGHT minimum.
        qreal dx = faceP.x() - xb;
        qreal dy = faceP.y() - bottomY;
        if (dy < lineH * 0.53)
          dy = lineH * 0.53;
        if (std::abs(dx) > std::abs(dy))
          dx = (dx < 0 ? -1.0 : 1.0) * std::abs(dy);
        tip = QPointF(xb + dx, bottomY + dy);
      }

      // Start at the right lip of the gap (or the bottom-right corner).
      wavy(wantTail ? gapRight : bRight, bottomY);
      if (wantTail)
        wavy(bRight, bottomY);

      // Right side, bottom to top.
      for (int k = rr.size() - 1; k >= 0; --k) {
        const qreal x = rr[k].x;
        if (k == rr.size() - 1)
          wavy(x, bottomY);
        else
          wavy(x, lineTop(rr[k + 1].start));
        const qreal ty = (k == 0) ? topY : lineTop(rr[k].start);
        wavy(x, ty);
      }

      // Across the top, then down the left side.
      wavy(lr[0].x, topY);
      for (int k = 0; k < lr.size(); ++k) {
        const qreal x = lr[k].x;
        if (k == 0)
          wavy(x, topY);
        else
          wavy(x, lineTop(lr[k].start));
        const qreal by = (k == lr.size() - 1) ? bottomY : lineTop(lr[k + 1].start);
        wavy(x, by);
      }

      if (wantTail) {
        wavy(gapLeft, bottomY);
        // Repeat the tip so the approximating beta-spline comes to a point.
        pts.append(tip);
        pts.append(tip);
        pts.append(tip);
      } else {
        wavy(bLeft, bottomY);
      }

      // Close back to the first point — AddWavies(pts[last], pts[0]) in the
      // original.
      wavy(pts.first().x(), pts.first().y());

      // A closed beta-spline supplies its own closing segment, so drop the
      // repeated first point (but keep intentional duplicates such as the tail).
      if (pts.size() > 1 &&
          std::abs(pts.first().x() - pts.last().x()) < 0.01 &&
          std::abs(pts.first().y() - pts.last().y()) < 0.01)
        pts.removeLast();

      const QPainterPath path = betaClosed(pts);
      if (isWhisper) {
        // Original whisper: thick white nimbus under a dashed black outline.
        p->setPen(QPen(Qt::white, penW * 3.5, Qt::SolidLine, Qt::RoundCap,
                       Qt::RoundJoin));
        p->setBrush(fill);
        p->drawPath(path);
        p->setPen(QPen(Qt::black, penW, Qt::DashLine, Qt::RoundCap,
                       Qt::RoundJoin));
        p->setBrush(Qt::NoBrush);
        p->drawPath(path);
      } else {
        p->setPen(QPen(Qt::black, penW, Qt::SolidLine, Qt::RoundCap,
                       Qt::RoundJoin));
        p->setBrush(fill);
        p->drawPath(path);
      }
    }
  }

  if (isThink && !sprite.isNull()) {
    // Thought bubble "trail": a few little circles drifting toward the head.
    const QPointF head(faceP);
    const QPointF bodyTop(sprite.left() + sprite.width() / 2.0,
                          sprite.top() + 2.0);
    const bool headAbove = head.y() < rect.top();
    const QPointF base(rect.center().x(),
                       headAbove ? qreal(rect.top()) : qreal(rect.bottom()));
    const qreal d = std::hypot(head.x() - bodyTop.x(), head.y() - bodyTop.y());
    const int dots = d > 0 ? int(d / 14) + 1 : 2;
    p->setPen(QPen(Qt::black, 1.5));
    p->setBrush(Qt::white);
    for (int i = 1; i <= dots; ++i) {
      const qreal f = qreal(i) / (dots + 1);
      const qreal r = i == dots ? 3.0 : 2.5;
      p->drawEllipse(base + (bodyTop - base) * f, r, r);
    }
    for (int i = 1; i <= 3; ++i) {
      const qreal f = qreal(i) / 4;
      const qreal r = i == 3 ? 3.0 : 2.0;
      p->drawEllipse(rect.center() + (head - QPointF(rect.center())) * f, r, r);
    }
  }

  // Draw the laid-out lines exactly where the outline was built around them.
  p->setPen(Qt::black);
  const int hAlign = isAction ? int(Qt::AlignLeft) : int(Qt::AlignHCenter);
  for (int i = 0; i < m_lines.size() && i < m_lineRects.size(); ++i) {
    const QRect lr = m_lineRects[i].translated(rect.topLeft());
    p->drawText(lr, hAlign | Qt::AlignTop, m_lines[i]);
  }
  p->restore();
}
