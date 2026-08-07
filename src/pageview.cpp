#include "pageview.h"

#include <QPainter>

PageView::PageView(QWidget *parent)
    : QWidget(parent)
{
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
    setMinimumSize(400, 300);
}

void PageView::clear()
{
    m_panels.clear();
    m_rendered.clear();
    relayout();
}

void PageView::addPanel(const ComicPanel &panel)
{
    ComicPanel copy = panel;
    copy.layout();
    m_panels.append(copy);
    m_rendered.append(copy.render());
    relayout();
}

QSize PageView::sizeHint() const
{
    if (m_rendered.isEmpty())
        return QSize(640, 480);
    const int rows = (m_rendered.size() + m_columns - 1) / m_columns;
    const int pw = m_rendered.first().width();
    const int ph = m_rendered.first().height();
    return QSize(m_margin * 2 + m_columns * pw + (m_columns - 1) * m_gap,
                 m_margin * 2 + rows * ph + (rows - 1) * m_gap);
}

void PageView::relayout()
{
    updateGeometry();
    update();
    if (parentWidget())
        parentWidget()->updateGeometry();
}

void PageView::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.fillRect(rect(), QColor(60, 60, 70));

    if (m_rendered.isEmpty()) {
        p.setPen(QColor(200, 200, 200));
        p.drawText(rect(), Qt::AlignCenter, tr("No comic panels yet — connect and say something."));
        return;
    }

    const int pw = m_rendered.first().width();
    const int ph = m_rendered.first().height();
    for (int i = 0; i < m_rendered.size(); ++i) {
        const int col = i % m_columns;
        const int row = i / m_columns;
        const int x = m_margin + col * (pw + m_gap);
        const int y = m_margin + row * (ph + m_gap);
        p.drawImage(x, y, m_rendered[i]);
    }
}
