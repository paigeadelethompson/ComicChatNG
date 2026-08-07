#include "balloon.h"

#include <QFontMetrics>
#include <QPainterPath>

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

    // Tail toward tip
    if (kind != BalloonKind::Action) {
        QPolygon poly;
        const QPoint base(rect.center().x(), rect.bottom());
        poly << base + QPoint(-8, 0) << base + QPoint(8, 0) << tip;
        p->setPen(Qt::black);
        p->setBrush(fill);
        p->drawPolygon(poly);
        if (kind == BalloonKind::Think) {
            p->drawEllipse(QRect(tip + QPoint(-4, -10), QSize(6, 6)));
            p->drawEllipse(QRect(tip + QPoint(-2, -4), QSize(4, 4)));
        }
    }

    p->setPen(Qt::black);
    p->drawText(rect.adjusted(8, 6, -8, -6), Qt::TextWordWrap | Qt::AlignLeft | Qt::AlignTop, text);
    p->restore();
}
