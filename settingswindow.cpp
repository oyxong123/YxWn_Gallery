#include "settingswindow.h"
#include "ui_settingswindow.h"
#include "mainwindow.h"
#include <QPushButton>
#include <QtDebug>

SettingsWindow::SettingsWindow(MainWindow* mainWindow, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SettingsWindow)
    , mw(mainWindow)
{
    ui->setupUi(this);
    QObject::connect(ui->btnApply, &QPushButton::clicked, this, &SettingsWindow::btnApply_clicked);
    QObject::connect(ui->btnCancel, &QPushButton::clicked, this, &SettingsWindow::reject);

    ui->inpAutoplayInterval->setValue(mw->autoplay.interval() / 1000);
}

SettingsWindow::~SettingsWindow()
{
    delete ui;
}

void SettingsWindow::btnApply_clicked() {
    if (mw->autoplay.interval() != ui->inpAutoplayInterval->value() * 1000) {
        mw->autoplay.setInterval(ui->inpAutoplayInterval->value() * 1000);
    }
    accept();
}
