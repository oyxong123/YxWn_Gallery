#include "mainwindow.h"

#include <QApplication>
#include <QScreen>
#include <QStyle>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    QScreen *screen = a.primaryScreen();
    QRect screenGeo = screen->geometry();
    screenGeo.setTop(screenGeo.top() - 40);  // Add window margin.
    w.setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, w.size(), screenGeo));

    return a.exec();
}
