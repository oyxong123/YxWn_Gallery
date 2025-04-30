#include "settingswindow.h"
#include "ui_settingswindow.h"
#include "mainwindow.h"
#include <QPushButton>
#include <QtDebug>
#include <QSettings>

SettingsWindow::SettingsWindow(MainWindow* mainWindow, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SettingsWindow)
    , mw(mainWindow)
{
    ui->setupUi(this);
    QObject::connect(ui->chkRmbFolder, &QCheckBox::clicked, this, &SettingsWindow::chkRmbFolder_clicked);
    QObject::connect(ui->btnApply, &QPushButton::clicked, this, &SettingsWindow::btnApply_clicked);
    QObject::connect(ui->btnCancel, &QPushButton::clicked, this, &SettingsWindow::reject);

    QSettings settings("YxWn", "YxWn_Gallery");
    ui->inpAutoplayInterval->setValue(mw->autoplay.interval() / 1000);
    if (mw->audio->isMuted()) ui->chkMute->setCheckState(Qt::Checked);
    if (settings.value("RmbFolder").toBool() == true) ui->chkRmbFolder->setCheckState(Qt::Checked);
    else ui->chkRmbFile->setEnabled(false);
    if (settings.value("RmbFile").toBool() == true) ui->chkRmbFile->setCheckState(Qt::Checked);
}

SettingsWindow::~SettingsWindow()
{
    delete ui;
}

void SettingsWindow::chkRmbFolder_clicked() {
    if (ui->chkRmbFolder->checkState() == Qt::Unchecked) {
        ui->chkRmbFile->setEnabled(false);
    }
    else {
        ui->chkRmbFile->setEnabled(true);
    }
}

void SettingsWindow::btnApply_clicked() {
    QSettings settings("YxWn", "YxWn_Gallery");
    if (mw->autoplay.interval() != ui->inpAutoplayInterval->value() * 1000) {
        mw->autoplay.setInterval(ui->inpAutoplayInterval->value() * 1000);
        settings.setValue("Autoplay Interval", mw->autoplay.interval());
    }
    if (ui->chkMute->checkState() == Qt::Unchecked) {
        if (mw->audio->isMuted() == true) {
            mw->audio->setMuted(false);
            settings.setValue("Mute", false);
        }
    }
    else {
        if (mw->audio->isMuted() == false)
        {
            mw->audio->setMuted(true);
            settings.setValue("Mute", true);
        }
    }
    if (ui->chkRmbFolder->checkState() == Qt::Unchecked) {
        if (settings.value("RmbFolder").toBool() == true) settings.setValue("RmbFolder", false);
    }
    else {
        if (settings.value("RmbFolder").toBool() == false) settings.setValue("RmbFolder", true);
    }

    if (ui->chkRmbFile->checkState() == Qt::Unchecked) {
        if (settings.value("RmbFile").toBool() == true) settings.setValue("RmbFile", false);
    }
    else {
        if (settings.value("RmbFile").toBool() == false) settings.setValue("RmbFile", true);
    }
    settings.sync();
    accept();
}
