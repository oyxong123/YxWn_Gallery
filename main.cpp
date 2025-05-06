#include "mainwindow.h"

#include <QApplication>
#include <QScreen>
#include <QStyle>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    QScreen *screen = a.primaryScreen();
    QRect screenGeo = screen->availableGeometry();
    screenGeo.setTop(screenGeo.top() + 30);
    w.setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, w.size(), screenGeo));
    // w.setWindowState(Qt::WindowFullScreen);

    return a.exec();
}
