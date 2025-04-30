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
    if (settings.value("Rmb Folder").toBool()) ui->chkRmbFolder->setCheckState(Qt::Checked);
    else ui->chkRmbFile->setEnabled(false);
    if (settings.value("Rmb File").toBool()) ui->chkRmbFile->setCheckState(Qt::Checked);
    if (settings.value("Exit On Close").toBool()) ui->chkExitOnClose->setCheckState(Qt::Checked);
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
        if (settings.value("Rmb Folder").toBool() == true) settings.setValue("Rmb Folder", false);
    }
    else {
        if (settings.value("Rmb Folder").toBool() == false) settings.setValue("Rmb Folder", true);
    }
    if (ui->chkRmbFile->checkState() == Qt::Unchecked) {
        if (settings.value("Rmb File").toBool() == true) settings.setValue("Rmb File", false);
    }
    else {
        if (settings.value("Rmb File").toBool() == false) settings.setValue("Rmb File", true);
    }
    if (ui->chkExitOnClose->checkState() == Qt::Unchecked) {
        if (settings.value("Exit On Close").toBool() == true) settings.setValue("Exit On Close", false);
    }
    else {
        if (settings.value("Exit On Close").toBool() == false) settings.setValue("Exit On Close", true);
    }
    settings.sync();
    accept();
}
