#pragma once

#include "balloon.h"
#include "emotions.h"

#include <QImage>
#include <QString>
#include <QVector>

struct PanelCharacter {
    QString nick;
    QString avatarName;
    Emotion emotion;
    QImage body;
    QRect bodyRect;
    QPoint faceTip;
    bool flip = false;
};

struct ComicPanel {
    QImage backdrop;
    QVector<PanelCharacter> characters;
    QVector<Balloon> balloons;
    QSize size = QSize(320, 240);

    bool isEmpty() const { return characters.isEmpty(); }
    bool containsSpeaker(const QString &nick) const;
    void layout();
    QImage render() const;
};
