#ifndef SETTINGSWINDOW_H
#define SETTINGSWINDOW_H

#include <QDialog>
#include "mainwindow.h"

namespace Ui {
class SettingsWindow;
}

class SettingsWindow : public QDialog
{
    Q_OBJECT

public:
    explicit SettingsWindow(MainWindow* mainWindow, QWidget *parent = nullptr);
    ~SettingsWindow();

private slots:
    void btnApply_clicked();

private:
    Ui::SettingsWindow *ui;
    MainWindow* mw;
};

#endif // SETTINGSWINDOW_H
