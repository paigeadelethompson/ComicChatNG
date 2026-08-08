#include "icons.h"

#include <QIcon>
#include <QPixmap>

namespace icons {
  QIcon fromResource(const QString &cell) {
    return QIcon(QPixmap(QStringLiteral(":/icons/%1").arg(cell)));
  }

  QIcon main(int index) {
    return fromResource(QStringLiteral("sliced/toolbar_%1.png").arg(index));
  }

  QIcon text(int index) {
    return fromResource(QStringLiteral("sliced/texttool_%1.png").arg(index));
  }

  QIcon user(int index) {
    return fromResource(QStringLiteral("sliced/usertool_%1.png").arg(index));
  }

  QIcon member(int index) {
    return fromResource(QStringLiteral("sliced/member_%1.png").arg(index));
  }

  QIcon saybar(int index) {
    return fromResource(QStringLiteral("sliced/saybar_%1.png").arg(index));
  }

  QIcon window() { return fromResource(QStringLiteral("chat.png")); }

  QIcon room() { return fromResource(QStringLiteral("room.png")); }
} // namespace icons