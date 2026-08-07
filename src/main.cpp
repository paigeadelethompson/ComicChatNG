#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("ComicChatNG"));
    QApplication::setOrganizationName(QStringLiteral("ComicChatNG"));
    QApplication::setApplicationVersion(QStringLiteral("2.5.0"));

    MainWindow window;
    window.show();
    return app.exec();
}
