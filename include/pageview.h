#pragma once

#include "panel.h"

#include <QScrollArea>
#include <QSize>
#include <QVector>
#include <QWidget>

class PageView : public QWidget {
  Q_OBJECT
public:
  explicit PageView(QWidget *parent = nullptr);

  void clear();
  void addPanel(const ComicPanel &panel);
  void replaceLastPanel(const ComicPanel &panel);
  ComicPanel lastPanel() const;
  int panelCount() const { return m_panels.size(); }

  QSize sizeHint() const override;

protected:
  void paintEvent(QPaintEvent *event) override;

private:
  QSize targetCell() const;
  void relayout();

  QVector<ComicPanel> m_panels;
  QVector<QImage> m_rendered;
  QSize m_cell;
  mutable int m_columns = 3;
  int m_gap = 8;
  int m_margin = 8;
};
