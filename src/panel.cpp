#include "panel.h"

#include <QPainter>

bool ComicPanel::containsSpeaker(const QString &nick) const
{
    for (const PanelCharacter &ch : characters) {
        if (ch.nick.compare(nick, Qt::CaseInsensitive) == 0)
            return true;
    }
    return false;
}

void ComicPanel::layout()
{
    if (size.width() < 160)
        size = QSize(320, 240);

    const int n = characters.size();
    if (n == 0)
        return;

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

        // Facing: characters on the right side of the panel are mirrored so a
        // pair turns toward each other (mirrors the original's m_flip).
        const bool rightSide = ch.bodyRect.center().x() >= size.width() / 2;
        ch.flip = n > 1 && rightSide;

        // Map the face tip through the same scale (and flip) into panel space.
        const qreal fx = scaled.width() ? qreal(scaled.width()) / orig.width() : 0.0;
        const qreal fy = scaled.height() ? qreal(scaled.height()) / orig.height() : 0.0;
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

    // Each balloon clings to its own speaker.
    for (Balloon &b : balloons) {
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

        const int maxW = qMax(80, size.width() / qMax(1, balloons.size()) - 20);
        b.layout(QRect(QPoint(0, 0), size), owner, face, maxW);
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