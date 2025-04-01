#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDir>
#include <QString>
#include <QRandomGenerator>
#include <QDirIterator>
#include <QFileDialog>
#include <QFileInfo>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QVideoWidget>
#include <QUrl>
#include <QtDebug>
#include <QTimer>
#include <QStyle>
#include <QScreen>

// Page to reference for resizing GUI dynamically based on window size: https://doc.qt.io/qt-6/layout.html

QDir dirImages;
QStringList pathList;
QString pathRand;

// This set of instances needs to be declared on top of the file to ensure it doesn't get destroyed when the runtime reaches the end of a function.
QMediaPlayer player;
QAudioOutput *audio = new QAudioOutput;

void onPlayerPositionChanged(qint64 position){
    qint64 duration = player.duration();
    if (duration - position < 100){
        player.pause();
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QObject::connect(ui->btnGenerate, &QPushButton::clicked, this, &MainWindow::btnGenerate_clicked);
    QObject::connect(ui->btnSelectFolder, &QPushButton::clicked, this, &MainWindow::btnSelectFolder_clicked);

    // Initialize
    QObject::connect(&player, &QMediaPlayer::positionChanged, &onPlayerPositionChanged);
    player.setVideoOutput(ui->vid);
    player.setAudioOutput(audio);
    ui->vid->hide();
    ui->img->hide();

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::btnGenerate_clicked()
{
    /**
     * 1. Select random file path from the selected directory (includes files from subdirectories).
     * 2. Ensure it's not the same file as the previously opened file.
     * 3. Check the type of file extension selected.
     * 4. Display the file based on file extension. (Only open image, video, and audio file)
     * Note: Can't display .webp files.
    **/
    if (pathList.isEmpty()) {
        ui->img->clear();
        ui->lblFilePath->setText("File Path: ");
        ui->lblFilePath->adjustSize();
        ui->lblFileName->setText("Name: ");
        ui->lblFileName->adjustSize();
        return;
    }
    int indexRand = QRandomGenerator::global()->bounded(pathList.size());
    QString pathRandTemp = dirImages.absoluteFilePath(pathList[indexRand]);
    if (pathRandTemp == pathRand) {  // Prevent the same media file from being generated.
        btnGenerate_clicked();
        return;
    }
    else pathRand = pathRandTemp;
    QString fileExtension = QFileInfo(pathRand).suffix().toLower();  // Change all letters lowercase (eg. JPG to jpg)
    if (fileExtension == "png" || fileExtension == "jpg" || fileExtension == "jpeg" || fileExtension == "jfif"){
        player.stop();
        QPixmap imgRand(pathRand);
        int width = ui->img->width();
        int height = ui->img->height();
        ui->img->setPixmap(imgRand.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        ui->img->setAlignment(Qt::AlignCenter);
        ui->vid->hide();
        ui->img->show();
    }
    else if (fileExtension == "gif" || fileExtension == "mp4" || fileExtension == "mkv"){
        ui->img->hide();
        player.setSource(QUrl(pathRand));
        ui->vid->show();
        player.play();
    }
    else if (fileExtension == "mp3" || fileExtension == "wav"){
        ui->img->hide();
        player.setSource(QUrl(pathRand));
        ui->vid->show();
        player.play();
    }
    ui->lblFilePath->setText("File Path: " + QFileInfo(pathRand).path());
    ui->lblFilePath->adjustSize();
    ui->lblFileName->setText("Name: " + QFileInfo(pathRand).fileName());
    ui->lblFileName->adjustSize();
}

void MainWindow::btnSelectFolder_clicked()
{
    /**
     * 1. Click on button.
     * 2. Dialog appears for users to select a dir path.
     * 3. Save the dir path the user selected.
     * 4. Display to user the path they selected.
     * 5. Retrieve all the files within the dir (including files from subdirectories) based on specified file extensions.
    **/
    QString dir = QFileDialog::getExistingDirectory();
    if (dir.isEmpty()) return;
    dirImages.setPath(dir);
    ui->lblPath->setText("Path: " + dirImages.path());
    ui->lblPath->adjustSize();
    QStringList filters;
    filters << "*.png" << "*.jpg" << "*.jfif" << "*.jpeg" << "*.mp4" << "*.gif" << "*.mkv" << "*.mp3" << "*.wav";  // Specify the file extensions to accept. (Case insensitive, eg. jpg = JPG)
    pathList.clear();  // Clear off buffer of files from previously selected dir.
    QDirIterator iterator(dirImages.path(), filters, QDir::Files, QDirIterator::Subdirectories);  // Automatically ignores "." and ".."
    while (iterator.hasNext()){
        pathList.append(iterator.next());
    }
}

