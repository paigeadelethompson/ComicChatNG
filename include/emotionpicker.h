#pragma once

#include "emotions.h"

#include <QPointF>
#include <QWidget>

// Two-dimensional joystick for picking a character expression, mirroring the
// original "emotion wheel": angle = which emotion, distance from centre =
// intensity, centre = neutral.
class EmotionPicker : public QWidget
{
    Q_OBJECT
public:
    explicit EmotionPicker(QWidget *parent = nullptr);

    Emotion emotion() const { return m_emotion; }
    void setEmotion(const Emotion &e);

signals:
    void emotionChanged(const Emotion &e);

protected:
    QSize sizeHint() const override { return QSize(170, 170); }
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    QPointF centre() const;
    qreal radius() const;
    QPointF knobPos() const;
    void updateFromPoint(const QPointF &pos);
    void setFromEmotion(const Emotion &e);

    Emotion m_emotion;
    bool m_dragging = false;
};