#pragma once

#include <QColor>
#include <QPainter>
#include <QPoint>
#include <QRect>
#include <QString>

enum class BalloonKind {
    Say,
    Think,
    Whisper,
    Action,
};

struct Balloon
{
    BalloonKind kind = BalloonKind::Say;
    QString speaker;
    QString text;
    QRect rect;
    QRect sprite;   // speaker sprite box in panel coords
    QPoint faceP;   // speaker's face anchor in panel coords

    void layout(const QRect &panelRect, const QRect &spriteRect,
                const QPoint &facePt, int maxWidth = 180);
    void paint(QPainter *p) const;
};