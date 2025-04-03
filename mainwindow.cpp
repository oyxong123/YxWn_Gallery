#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <cstdio>
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
#include <QKeyEvent>

// Page to reference for resizing GUI dynamically based on window size: https://doc.qt.io/qt-6/layout.html

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->slrProgressBar->installEventFilter(this);
    setFocusPolicy(Qt::StrongFocus);
    QObject::connect(ui->btnGenerate, &QPushButton::clicked, this, &MainWindow::btnGenerate_clicked);
    QObject::connect(ui->btnSelectFolder, &QPushButton::clicked, this, &MainWindow::btnSelectFolder_clicked);
    QObject::connect(ui->btnPlayPause, &QPushButton::clicked, this, &MainWindow::btnPlayPause_clicked);
    QObject::connect(ui->btnRewind, &QPushButton::clicked, this, &MainWindow::btnRewind_clicked);
    QObject::connect(ui->btnSkip, &QPushButton::clicked, this, &MainWindow::btnSkip_clicked);
    QObject::connect(&player, &QMediaPlayer::positionChanged, this, &MainWindow::playerPositionChanged);
    QObject::connect(ui->slrProgressBar, &QAbstractSlider::sliderPressed, this, &MainWindow::sliderPressed);
    QObject::connect(ui->slrProgressBar, &QAbstractSlider::sliderMoved, this, &MainWindow::sliderMoved);
    QObject::connect(ui->slrProgressBar, &QAbstractSlider::sliderReleased, this, &MainWindow::sliderReleased);

    // Initialize
    player.setVideoOutput(ui->vid);
    player.setAudioOutput(audio);
    ui->vid->hide();
    ui->img->hide();
    ui->contPlayerPanel->hide();

    ui->btnPlayPause->setIcon(QIcon("resources/btnPause.png"));
    ui->btnPlayPause->setIconSize(ui->btnPlayPause->size());  // Size only need to set once, it will retain when image changes.
    ui->btnRewind->setIcon(QIcon("resources/btnRewind.png"));
    ui->btnRewind->setIconSize(ui->btnRewind->size());
    ui->btnSkip->setIcon(QIcon("resources/btnSkip.png"));
    ui->btnSkip->setIconSize(ui->btnSkip->size());
}

MainWindow::~MainWindow()
{
    delete ui;
    delete audio;
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
    ui->img->clear();  // Clear opened media before opening the next one.
    player.stop();
    player.setSource(QUrl());
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
    QString fileExtension = QFileInfo(pathRand).suffix().toLower();  // Change all letters lowercase. (eg. JPG to jpg)
    if (fileExtension == "png" || fileExtension == "jpg" || fileExtension == "jpeg" || fileExtension == "jfif"){
        player.stop();
        QPixmap imgRand(pathRand);
        int width = ui->img->width();
        int height = ui->img->height();
        ui->img->setPixmap(imgRand.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        ui->img->setAlignment(Qt::AlignCenter);
        ui->vid->hide();
        ui->contPlayerPanel->hide();
        ui->img->show();
    }
    else if (fileExtension == "gif" || fileExtension == "mp4" || fileExtension == "mkv"){
        ui->img->hide();
        player.setSource(QUrl(pathRand));
        ui->btnPlayPause->setIcon(QIcon("resources/btnPause.png"));
        ui->vid->show();
        ui->contPlayerPanel->show();
        player.play();
    }
    else if (fileExtension == "mp3" || fileExtension == "wav"){
        ui->vid->hide();
        player.setSource(QUrl(pathRand));
        QPixmap imgMusic("resources/imgMusic.png");
        int width = ui->img->width();
        int height = ui->img->height();
        ui->img->setPixmap(imgMusic.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        ui->img->setAlignment(Qt::AlignCenter);
        ui->btnPlayPause->setIcon(QIcon("resources/btnPause.png"));
        ui->img->show();
        ui->contPlayerPanel->show();
        player.play();
    }
    QString pathFolder = QFileInfo(pathRand).path().remove(dirImages.path());
    if (pathFolder.isEmpty()) pathFolder = "-";
    else pathFolder.removeFirst();
    ui->lblFilePath->setText("Folder: " + pathFolder);
    ui->lblFilePath->adjustSize();
    ui->lblFileName->setText("Name: " + QFileInfo(pathRand).fileName());
    ui->lblFileName->adjustSize();
    ui->contPlayerPanel->setCurrentIndex(1);
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

void MainWindow::btnPlayPause_clicked()
{
    if (player.isPlaying()){
        player.pause();
        ui->btnPlayPause->setIcon(QIcon("resources/btnPlay.png"));
    }
    else {
        if (player.duration() - player.position() < 100) player.setPosition(0);  // Restart the video if the almost ended. (Paused by program due to reached the last few frame of video)
        player.play();
        ui->btnPlayPause->setIcon(QIcon("resources/btnPause.png"));
    }
}

void MainWindow::btnRewind_clicked()
{
    qint64 newPosition = player.position() - 10000;
    player.setPosition(newPosition);  // Sets to 0 if it is a negative value.
}

void MainWindow::btnSkip_clicked()
{
    qint64 newPosition = player.position() + 30000;
    player.setPosition(newPosition);  // Sets to last millisecond of duration if the value is higher than duration.
}

QString formatTime(qint64 ms) {
    int hours = (ms / 3600000);
    int mins = (ms / 60000) % 60;
    int secs = (ms / 1000) % 60;
    char buffer[9];
    std::sprintf(buffer, "%02d:%02d:%02d", hours, mins, secs);  // Produces either 01:01:01 or 101:59:59. (Meaning hour can exceed 100 (very unlikely to happen during usage))
    return QString::fromUtf8(buffer);
}

void MainWindow::playerPositionChanged(qint64 position){
    qint64 duration = player.duration();
    if (duration - position < 100){
        player.pause();
        ui->btnPlayPause->setIcon(QIcon("resources/btnPlay.png"));
    }
    ui->slrProgressBar->setMaximum(player.duration() - 100);  // Offset for last-frame pause.
    ui->slrProgressBar->setValue(position);
    ui->lblCurrentTime->setText(formatTime(position));
    ui->lblRemainingTime->setText(formatTime(duration - position));
}

void MainWindow::sliderPressed() {
    player.pause();
}

void MainWindow::sliderMoved(int value) {
    player.setPosition(value);
}

void MainWindow::sliderReleased() {
    player.play();
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    if (obj == ui->slrProgressBar && event->type() == QEvent::MouseButtonPress) {
        Qt::MouseButtons btns = QApplication::mouseButtons();  // Gets the current state of all mouse buttons.
        QPoint localPos = ui->slrProgressBar->mapFromGlobal(QCursor::pos());  // Get the local coordinate of the slider. (Eg. The value will be negative if the mouse is to the left or top of the slider)
        bool clickOnSlider = (btns & Qt::LeftButton) && (ui->slrProgressBar->rect().contains(localPos));  // Checking whether the mouseclick position is within the slider widget (4 sides of rectangle).
        if (clickOnSlider) {
            // Get coordinate of the player's current position. (Conversion to float is necessary to calculate ratios.)
            float currentPos = static_cast<float>(player.position());
            float currentDuration = static_cast<float>(player.duration());
            float slrWidth = static_cast<float>(ui->slrProgressBar->width());
            float currentRatio = currentPos / currentDuration;
            int currentXCoordinate = slrWidth * currentRatio;
            int lowerBound = currentXCoordinate - 4;
            int higherBound = currentXCoordinate + 4;
            if (localPos.x() < lowerBound || localPos.x() > higherBound) {  // Skip event filter if the user wants to click on the slider head.
                float posRatio = static_cast<float>(localPos.x()) / slrWidth;  // Get the current clicked position on the slider in relation to its entire bar. (x coordinate = horizontal progress bar) (how many duration passed over (/) the total duration)
                int newSliderPos = player.duration() * posRatio;  // Multiply the ratio with the total media duration to get the new time position that the user have clicked on.
                player.setPosition(newSliderPos);
                return true;
            }
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Space) {
        MainWindow::btnGenerate_clicked();
    }
    QWidget::keyPressEvent(event);
}
