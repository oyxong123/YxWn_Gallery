#include "progressbarwindow.h"
#include "ui_progressbarwindow.h"

ProgressBarWindow::ProgressBarWindow(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ProgressBarWindow)
{
    ui->setupUi(this);
}

ProgressBarWindow::~ProgressBarWindow()
{
    delete ui;
}
