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
    QObject::connect(ui->chkDesktopWallpaper, &QCheckBox::clicked, this, &SettingsWindow::chkDesktopWallpaper_clicked);
    QObject::connect(ui->btnApply, &QPushButton::clicked, this, &SettingsWindow::btnApply_clicked);
    QObject::connect(ui->btnCancel, &QPushButton::clicked, this, &SettingsWindow::reject);

    QSettings settings("YxWn", "YxWn_Gallery");
    ui->inpAutoplayInterval->setValue(mw->autoplay.interval() / 1000);
    if (mw->audio->isMuted()) ui->chkMute->setCheckState(Qt::Checked);
    if (settings.value("Rmb Folder").toBool()) ui->chkRmbFolder->setCheckState(Qt::Checked);
    else {
        ui->chkRmbFile->setEnabled(false);
        ui->chkRefreshFolderOnStartup->setEnabled(false);
    }
    if (settings.value("Refresh Folder on Startup").toBool()) ui->chkRefreshFolderOnStartup->setCheckState(Qt::Checked);
    if (settings.value("Rmb File").toBool()) ui->chkRmbFile->setCheckState(Qt::Checked);
    if (settings.value("Exit On Close").toBool()) ui->chkExitOnClose->setCheckState(Qt::Checked);
    if (settings.value("Desktop Wallpaper").toBool()) ui->chkDesktopWallpaper->setCheckState(Qt::Checked);
    if (settings.value("Run as Wallpaper on Startup").toBool()) ui->chkRunAsWallpaper->setCheckState(Qt::Checked);
    if (settings.value("Include Picture").toBool()) ui->chkIncludePic->setCheckState(Qt::Checked);
    if (settings.value("Include Video").toBool()) ui->chkIncludeVid->setCheckState(Qt::Checked);
    if (settings.value("Include Audio").toBool()) ui->chkIncludeAud->setCheckState(Qt::Checked);
}

SettingsWindow::~SettingsWindow()
{
    delete ui;
}

void SettingsWindow::chkRmbFolder_clicked() {
    if (ui->chkRmbFolder->checkState() == Qt::Unchecked) {
        ui->chkRmbFile->setEnabled(false);
        ui->chkRefreshFolderOnStartup->setEnabled(false);
    }
    else {
        ui->chkRmbFile->setEnabled(true);
        ui->chkRefreshFolderOnStartup->setEnabled(true);
    }
}

void SettingsWindow::chkDesktopWallpaper_clicked() {
    if (ui->chkDesktopWallpaper->checkState() == Qt::Unchecked) {
        ui->chkRunAsWallpaper->setEnabled(false);
    }
    else {
        ui->chkRunAsWallpaper->setEnabled(true);
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
    if (ui->chkRefreshFolderOnStartup->checkState() == Qt::Unchecked) {
        if (settings.value("Refresh Folder on Startup").toBool() == true) settings.setValue("Refresh Folder on Startup", false);
    }
    else {
        if (settings.value("Refresh Folder on Startup").toBool() == false) settings.setValue("Refresh Folder on Startup", true);
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
    if (ui->chkDesktopWallpaper->checkState() == Qt::Unchecked) {
        if (settings.value("Desktop Wallpaper").toBool() == true) {
            settings.setValue("Desktop Wallpaper", false);
            mw->trayMenu.removeAction(mw->wallpaperModeAction);
            QObject::disconnect(mw->wallpaperModeAction, &QAction::triggered, mw, &MainWindow::trayWallpaperModeAction_clicked);
        }
    }
    else {
        if (settings.value("Desktop Wallpaper").toBool() == false) {
            settings.setValue("Desktop Wallpaper", true);
            mw->wallpaperModeAction = mw->trayMenu.addAction("Wallpaper Mode");
            mw->wallpaperModeAction->setCheckable(true);
            QObject::connect(mw->wallpaperModeAction, &QAction::triggered, mw, &MainWindow::trayWallpaperModeAction_clicked);
            mw->trayMenu.insertAction(mw->windowModeAction, mw->wallpaperModeAction);
        }
    }
    if (ui->chkRunAsWallpaper->checkState() == Qt::Unchecked) {
        if (settings.value("Run as Wallpaper on Startup").toBool() == true) settings.setValue("Run as Wallpaper on Startup", false);
    }
    else {
        if (settings.value("Run as Wallpaper on Startup").toBool() == false) settings.setValue("Run as Wallpaper on Startup", true);
    }
    if (ui->chkIncludePic->checkState() == Qt::Unchecked) {
        if (settings.value("Include Picture").toBool() == true) settings.setValue("Include Picture", false);
    }
    else {
        if (settings.value("Include Picture").toBool() == false) settings.setValue("Include Picture", true);
    }
    if (ui->chkIncludeVid->checkState() == Qt::Unchecked) {
        if (settings.value("Include Video").toBool() == true) settings.setValue("Include Video", false);
    }
    else {
        if (settings.value("Include Video").toBool() == false) settings.setValue("Include Video", true);
    }
    if (ui->chkIncludeAud->checkState() == Qt::Unchecked) {
        if (settings.value("Include Audio").toBool() == true) settings.setValue("Include Audio", false);
    }
    else {
        if (settings.value("Include Audio").toBool() == false) settings.setValue("Include Audio", true);
    }
    settings.sync();
    accept();
}
