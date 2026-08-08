#include "pageview.h"

#include <QPainter>

PageView::PageView(QWidget *parent) : QWidget(parent) {
  setBackgroundRole(QPalette::Base);
  setAutoFillBackground(true);
  setMinimumSize(400, 300);
}

QSize PageView::targetCell() const {
  // Fit the preview to the scroll area, 3-4 per row.
  const int vw = parentWidget() ? parentWidget()->width() : 1200;
  const int avail = qMax(120, vw - 8);
  const int cols = qBound(2, avail / 220, 4);
  m_columns = cols;
  const int cellW = qMax(150, (avail - 8 - (cols - 1) * m_gap) / cols);
  return QSize(cellW, int(cellW * 0.78));
}

void PageView::clear() {
  m_panels.clear();
  m_rendered.clear();
  relayout();
}

void PageView::addPanel(const ComicPanel &panel) {
  const QSize cell = targetCell();
  if (m_cell != cell) {
    m_cell = cell;
    // Uniform grid: re-layout every existing panel at the new size.
    for (int i = 0; i < m_panels.size(); ++i) {
      m_panels[i].size = m_cell;
      m_panels[i].layout();
      m_rendered[i] = m_panels[i].render();
    }
  }

  ComicPanel copy = panel;
  copy.size = m_cell;
  copy.layout();
  m_panels.append(copy);
  m_rendered.append(copy.render());
  relayout();
}

void PageView::replaceLastPanel(const ComicPanel &panel) {
  if (m_panels.isEmpty()) {
    addPanel(panel);
    return;
  }
  ComicPanel copy = panel;
  copy.size = m_cell;
  copy.layout();
  m_panels.last() = copy;
  m_rendered.last() = copy.render();
  relayout();
}

ComicPanel PageView::lastPanel() const {
  if (m_panels.isEmpty())
    return ComicPanel();
  return m_panels.last();
}

QSize PageView::sizeHint() const {
  if (m_rendered.isEmpty())
    return QSize(640, 300);
  const QSize c = m_cell.isEmpty() ? targetCell() : m_cell;
  const int rows = (m_rendered.size() + m_columns - 1) / m_columns;
  return QSize(m_margin * 2 + m_columns * c.width() + (m_columns - 1) * m_gap,
               m_margin * 2 + rows * c.height() + (rows - 1) * m_gap);
}

void PageView::relayout() {
  // Size the widget to its content so the scroll area keeps real scroll bars.
  setFixedSize(sizeHint());
  update();
}

void PageView::paintEvent(QPaintEvent *) {
  QPainter p(this);
  p.fillRect(rect(), QColor(60, 60, 70));

  if (m_rendered.isEmpty()) {
    p.setPen(QColor(200, 200, 200));
    p.drawText(rect(), Qt::AlignCenter,
               tr("No comic panels yet — connect and say something."));
    return;
  }

  const int cw = m_rendered.first().width();
  const int ch = m_rendered.first().height();
  for (int i = 0; i < m_rendered.size(); ++i) {
    const int col = i % m_columns;
    const int row = i / m_columns;
    const int x = m_margin + col * (cw + m_gap);
    const int y = m_margin + row * (ch + m_gap);
    p.drawImage(x, y, m_rendered[i]);
  }
}