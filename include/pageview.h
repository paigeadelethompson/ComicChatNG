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

  // One row of the comic-strip title card: a small avatar icon and the name.
  struct TitleStar {
    QString nick;
    QImage icon;
  };

  void clear();
  void setTitle(const QString &title, const QVector<TitleStar> &stars);
  void addPanel(const ComicPanel &panel);
  void replaceLastPanel(const ComicPanel &panel);
  ComicPanel lastPanel() const;
  int panelCount() const { return m_panels.size(); }

  // Re-lay-out every existing panel for a new viewport width (window resized).
  void reflowForWidth(int viewportWidth);

  QSize sizeHint() const override;

protected:
  void paintEvent(QPaintEvent *event) override;

private:
  QSize computeCell(int viewportWidth) const;
  QSize targetCell() const;
  void relayout();
  void renderTitle();

  QVector<ComicPanel> m_panels;
  QVector<QImage> m_rendered;
  QVector<TitleStar> m_stars;
  QString m_title;
  QImage m_titleImage;
  bool m_hasTitle = false;
  QSize m_cell;
  mutable int m_columns = 3;
  int m_gap = 8;
  int m_margin = 8;
};
