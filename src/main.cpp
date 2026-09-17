#include "mainwindow.h"

#include <QApplication>
#include <QFontDatabase>

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  QApplication::setApplicationName(QStringLiteral("ComicChatNG"));
  QApplication::setOrganizationName(QStringLiteral("ComicChatNG"));
  QApplication::setApplicationVersion(QStringLiteral("2.5.0"));

  // The original Comic Chat ships with Comic Sans MS; embed it so balloons use
  // the same letterforms everywhere.
  QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/ComicSansMS.ttf"));

  MainWindow window;
  window.show();
  return app.exec();
}
