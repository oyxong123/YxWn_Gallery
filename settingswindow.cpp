#include "settingswindow.h"
#include "ui_settingswindow.h"
#include "mainwindow.h"

SettingsWindow::SettingsWindow(MainWindow* mainWindow, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SettingsWindow)
    , mw(mainWindow)
{
    ui->setupUi(this);
    ui->inpAutoplayInterval->setValue(mw.autoplay.interval());
}

SettingsWindow::~SettingsWindow()
{
    delete ui;
}
