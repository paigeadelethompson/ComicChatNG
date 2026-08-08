#include "balloon.h"

#include <QFontMetrics>
#include <QPainterPath>

#include <cmath>

void Balloon::layout(const QRect &panelRect, const QPoint &faceTip, int maxWidth)
{
    tip = faceTip;
    QFont font(QStringLiteral("Comic Sans MS"), 10);
    if (!QFont(font).exactMatch())
        font = QFont(QStringLiteral("DejaVu Sans"), 10);
    QFontMetrics fm(font);

    const QString wrapped = fm.elidedText(text, Qt::ElideRight, maxWidth * 4);
    QRect br = fm.boundingRect(QRect(0, 0, maxWidth, 1000),
                               Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop,
                               text.isEmpty() ? wrapped : text);
    br.adjust(-10, -8, 10, 8);

    // Prefer above the face tip.
    QPoint topLeft(faceTip.x() - br.width() / 2, faceTip.y() - br.height() - 24);
    if (topLeft.x() < 8)
        topLeft.setX(8);
    if (topLeft.y() < 8)
        topLeft.setY(faceTip.y() + 20);
    if (topLeft.x() + br.width() > panelRect.width() - 8)
        topLeft.setX(panelRect.width() - 8 - br.width());
    if (topLeft.y() + br.height() > panelRect.height() - 8)
        topLeft.setY(qMax(8, panelRect.height() - 8 - br.height()));

    rect = QRect(topLeft, br.size());
}

void Balloon::paint(QPainter *p) const
{
    p->save();
    QFont font(QStringLiteral("Comic Sans MS"), 10);
    if (!QFont(font).exactMatch())
        font = QFont(QStringLiteral("DejaVu Sans"), 10);
    p->setFont(font);

    QPainterPath path;
    if (kind == BalloonKind::Think) {
        path.addRoundedRect(rect, 18, 18);
    } else if (kind == BalloonKind::Action) {
        path.addRect(rect);
    } else {
        path.addRoundedRect(rect, 12, 12);
    }

    QColor fill = Qt::white;
    if (kind == BalloonKind::Whisper)
        fill = QColor(255, 255, 220);
    else if (kind == BalloonKind::Action)
        fill = QColor(240, 240, 255);

    p->setPen(QPen(Qt::black, kind == BalloonKind::Whisper ? 1 : 2,
                   kind == BalloonKind::Whisper ? Qt::DashLine : Qt::SolidLine));
    p->setBrush(fill);
    p->drawPath(path);

    // Tail toward tip, attached on the edge facing the speaker. The base sits on
    // that edge so the tail never crosses the bubble body.
    if (kind != BalloonKind::Action) {
        QPolygon poly;
        const qreal dx = tip.x() - rect.center().x();
        const qreal dy = tip.y() - rect.center().y();
        QPoint base;
        bool horizontal = false;
        if (qAbs(dy) >= qAbs(dx)) {
            const bool below = dy >= 0;
            const int bx = qBound(rect.left() + 8, tip.x(), rect.right() - 8);
            base = QPoint(bx, below ? rect.bottom() : rect.top());
        } else {
            horizontal = true;
            const bool left = dx <= 0;
            const int by = qBound(rect.top() + 8, tip.y(), rect.bottom() - 8);
            base = QPoint(left ? rect.left() : rect.right(), by);
        }
        const QPoint d1 = horizontal ? QPoint(0, -8) : QPoint(-8, 0);
        // Stop the tail just short of the head so it points at, but doesn't
        // cross over, the character.
        const QPointF dirF = QPointF(tip - base);
        const qreal len = std::hypot(dirF.x(), dirF.y());
        QPointF end(tip);
        if (len > 9.0)
            end = QPointF(tip) - (dirF / len) * 9.0;
        poly << base + d1 << base - d1 << end.toPoint();
        p->setPen(Qt::black);
        p->setBrush(fill);
        p->drawPolygon(poly);
        if (kind == BalloonKind::Think) {
            // Trailing dots sit right toward the head.
            if (len > 0.5) {
                const QPointF u = dirF / len;
                const qreal off = qMin(qreal(9), len / 3);
                p->drawEllipse(QRectF(end + u * (off * 0) + QPointF(-2, -2), QSizeF(5, 5)));
                p->drawEllipse(QRectF(end + u * (off * 1) + QPointF(-2, -2), QSizeF(4, 4)));
                p->drawEllipse(QRectF(end + u * (off * 2) + QPointF(-2, -2), QSizeF(3, 3)));
            }
        }
    }

    p->setPen(Qt::black);
    p->drawText(rect.adjusted(8, 6, -8, -6), Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop, text);
    p->restore();
}
