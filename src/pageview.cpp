#include "pageview.h"

#include <QFont>
#include <QFontMetrics>
#include <QPainter>

PageView::PageView(QWidget *parent) : QWidget(parent) {
  setBackgroundRole(QPalette::Base);
  setAutoFillBackground(true);
  setMinimumSize(400, 300);
}

QSize PageView::computeCell(int viewportWidth) const {
  // Fit the preview to the scroll area, 3-4 per row.
  const int avail = qMax(120, viewportWidth - 8);
  const int cols = qBound(2, avail / 220, 4);
  m_columns = cols;
  const int cellW = qMax(150, (avail - 8 - (cols - 1) * m_gap) / cols);
  return QSize(cellW, int(cellW * 0.78));
}

QSize PageView::targetCell() const {
  return computeCell(parentWidget() ? parentWidget()->width() : 1200);
}

void PageView::reflowForWidth(int viewportWidth) {
  const QSize cell = computeCell(viewportWidth);
  if (cell == m_cell)
    return relayout();
  m_cell = cell;
  // Uniform grid: re-layout every existing panel at the new size.
  for (int i = 0; i < m_panels.size(); ++i) {
    m_panels[i].size = m_cell;
    m_panels[i].layout();
    m_rendered[i] = m_panels[i].render();
  }
  if (m_hasTitle)
    renderTitle();
  relayout();
}

void PageView::clear() {
  m_panels.clear();
  m_rendered.clear();
  relayout();
}

void PageView::setTitle(const QString &title, const QVector<TitleStar> &stars) {
  m_title = title;
  m_stars = stars;
  m_hasTitle = !m_title.isEmpty() || !m_stars.isEmpty();
  if (m_cell.isEmpty())
    computeCell(parentWidget() ? parentWidget()->width() : 1200);
  renderTitle();
  relayout();
}

void PageView::renderTitle() {
  if (!m_hasTitle) {
    m_titleImage = QImage();
    return;
  }
  if (m_cell.isEmpty() || m_cell.width() < 80 || m_cell.height() < 60) {
    m_titleImage = QImage();
    return;
  }

  QImage img(m_cell, QImage::Format_ARGB32);
  img.fill(Qt::white);
  QPainter p(&img);
  p.setRenderHint(QPainter::Antialiasing, true);
  p.setRenderHint(QPainter::SmoothPixmapTransform, true);
  p.setRenderHint(QPainter::TextAntialiasing, true);

  const int W = m_cell.width();
  const int H = m_cell.height();
  int y = qRound(H * 0.05);

  // Title caption, wrapped (never elided) so "SIGHTED IN CYBERSPACE" and the
  // other long captions break onto a second line like the original.
  if (!m_title.isEmpty()) {
    QFont titleFont(QStringLiteral("Comic Sans MS"));
    titleFont.setBold(true);
    titleFont.setPixelSize(qBound(10, W / 13, 26));
    p.setFont(titleFont);
    p.setPen(Qt::black);
    const QFontMetrics tfm(titleFont);
    const int titleW = W - 16;
    const QRect wrapped =
        tfm.boundingRect(QRect(0, 0, titleW, H),
                         Qt::TextWordWrap | Qt::AlignHCenter | Qt::AlignTop,
                         m_title);
    const QRect titleRect(8, y, titleW, wrapped.height());
    p.drawText(titleRect, Qt::TextWordWrap | Qt::AlignHCenter | Qt::AlignTop,
               m_title);
    y = titleRect.bottom() + 3;
  }

  // "STARRING" caption.
  QFont starFont(QStringLiteral("Comic Sans MS"));
  starFont.setBold(true);
  starFont.setPixelSize(qBound(8, W / 30, 16));
  p.setFont(starFont);
  const QFontMetrics sfm(starFont);
  const QRect starRect(8, y, W - 16, sfm.height());
  p.drawText(starRect, Qt::AlignHCenter | Qt::AlignTop,
             QStringLiteral("STARRING"));
  y = starRect.bottom() + 3;

  // Star rows: one fixed icon column and one fixed name column, so every
  // avatar and every name lines up; the block is centred as a whole.
  const int rowH = qMax(14, H / 9);
  const int iconSize = qBound(12, rowH - 4, 34);
  const int iconGap = 6;
  QFont nickFont(QStringLiteral("Comic Sans MS"));
  nickFont.setBold(true);
  nickFont.setPixelSize(qBound(7, W / 34, 13));
  p.setFont(nickFont);
  const QFontMetrics nfm(nickFont);

  int maxNameW = 0;
  for (const TitleStar &star : m_stars)
    maxNameW = qMax(maxNameW, nfm.horizontalAdvance(star.nick));
  maxNameW = qMin(maxNameW, W / 2);
  const int x0 = qMax(6, (W - (iconSize + iconGap + maxNameW)) / 2);

  for (const TitleStar &star : m_stars) {
    if (y + rowH > H - 4)
      break;
    const QRect iconBox(x0, y + (rowH - iconSize) / 2, iconSize, iconSize);
    if (!star.icon.isNull()) {
      const QImage icon = star.icon.scaled(iconBox.size(), Qt::KeepAspectRatio,
                                           Qt::SmoothTransformation);
      const QRect dst(iconBox.center().x() - icon.width() / 2,
                      iconBox.center().y() - icon.height() / 2, icon.width(),
                      icon.height());
      p.drawImage(dst, icon);
    }
    p.setPen(Qt::black);
    const QRect nickRect(x0 + iconSize + iconGap, y, maxNameW, rowH);
    p.drawText(nickRect, Qt::AlignLeft | Qt::AlignVCenter,
               nfm.elidedText(star.nick, Qt::ElideRight, nickRect.width()));
    y += rowH;
  }

  p.end();
  m_titleImage = img;
}

void PageView::addPanel(const ComicPanel &panel) {
  reflowForWidth(parentWidget() ? parentWidget()->width() : 1200);

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
  const int cells = m_rendered.size() + (m_hasTitle ? 1 : 0);
  if (cells == 0)
    return QSize(640, 300);
  const QSize c = m_cell.isEmpty() ? targetCell() : m_cell;
  const int rows = (cells + m_columns - 1) / m_columns;
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
  // The comic page is white paper, like the original client.
  p.fillRect(rect(), QColor(255, 255, 255));

  if (m_rendered.isEmpty() && !m_hasTitle) {
    p.setPen(QColor(90, 90, 90));
    p.drawText(rect(), Qt::AlignCenter,
               tr("No comic panels yet — connect and say something."));
    return;
  }

  const int cw = m_cell.width();
  const int ch = m_cell.height();
  int cell = 0;
  if (m_hasTitle && !m_titleImage.isNull()) {
    p.drawImage(m_margin, m_margin, m_titleImage);
    ++cell;
  }
  for (int i = 0; i < m_rendered.size(); ++i, ++cell) {
    const int col = cell % m_columns;
    const int row = cell / m_columns;
    const int x = m_margin + col * (cw + m_gap);
    const int y = m_margin + row * (ch + m_gap);
    p.drawImage(x, y, m_rendered[i]);
  }
}