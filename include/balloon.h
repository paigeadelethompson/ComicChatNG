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
    QPoint tip; // arrow tip toward face

    void layout(const QRect &panelRect, const QPoint &faceTip, int maxWidth = 180);
    void paint(QPainter *p) const;
};
