#include "panel.h"

#include <QPainter>

void ComicPanel::layout()
{
    if (size.width() < 160)
        size = QSize(320, 240);

    const int n = characters.size();
    if (n == 0)
        return;

    const int groundY = size.height() - 12;
    const int spacing = size.width() / (n + 1);

    for (int i = 0; i < n; ++i) {
        PanelCharacter &ch = characters[i];
        if (ch.body.isNull())
            continue;

        // Scale body to fit ~55% of panel height.
        const int maxH = int(size.height() * 0.55);
        QImage scaled = ch.body;
        if (scaled.height() > maxH) {
            const qreal f = qreal(maxH) / scaled.height();
            scaled = scaled.scaled(qRound(scaled.width() * f), maxH,
                                   Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        }
        if (ch.flip)
            scaled = scaled.mirrored(true, false);

        const qreal fx = ch.body.width() ? qreal(scaled.width()) / ch.body.width() : 0.0;
        const qreal fy = ch.body.height() ? qreal(scaled.height()) / ch.body.height() : 0.0;

        const int x = spacing * (i + 1) - scaled.width() / 2;
        const int y = groundY - scaled.height();
        ch.body = scaled;
        ch.bodyRect = QRect(x, y, scaled.width(), scaled.height());

        // Map the face tip through the same scale/flip into panel space.
        if (ch.faceTip.x() >= 0 && ch.faceTip.y() >= 0 && fx > 0 && fy > 0) {
            int tipX = qRound(ch.faceTip.x() * fx);
            if (ch.flip)
                tipX = scaled.width() - tipX;
            ch.faceTip = QPoint(ch.bodyRect.left() + tipX,
                                ch.bodyRect.top() + qRound(ch.faceTip.y() * fy));
        } else {
            ch.faceTip = QPoint(ch.bodyRect.center().x(), ch.bodyRect.top() + ch.bodyRect.height() / 4);
        }
    }

    for (Balloon &b : balloons) {
        QPoint tip = QPoint(size.width() / 2, size.height() / 3);
        for (const PanelCharacter &ch : characters) {
            if (ch.nick.compare(b.speaker, Qt::CaseInsensitive) == 0) {
                tip = ch.faceTip;
                break;
            }
        }
        b.layout(QRect(QPoint(0, 0), size), tip);
    }
}

QImage ComicPanel::render() const
{
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
        if (!ch.body.isNull())
            p.drawImage(ch.bodyRect.topLeft(), ch.body);
    }

    for (const Balloon &b : balloons)
        b.paint(&p);

    p.end();
    return img;
}
