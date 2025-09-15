#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "settingswindow.h"
#include "progressbarwindow.h"
#include "qcustompushbutton.h"
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
#include <QRegularExpression>
#include <QSettings>
#include <QSystemTrayIcon>
#include <QElapsedTimer>
#include <QMessageBox>
#include <QApplication>
#include <QStorageInfo>
#include <windows.h>

// Page to reference for resizing GUI dynamically based on window size: https://doc.qt.io/qt-6/layout.html

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    QElapsedTimer perf;
    perf.start();

    ui->setupUi(this);
    ui->slrProgressBar->installEventFilter(this);
    setFocusPolicy(Qt::StrongFocus);
    QObject::connect(ui->btnGenerate, &QPushButton::clicked, this, &MainWindow::btnGenerate_clicked);
    QObject::connect(ui->btnSelectFolder, &QPushButton::clicked, this, &MainWindow::btnSelectFolder_clicked);
    QObject::connect(ui->btnPlayPause, &QPushButton::clicked, this, &MainWindow::btnPlayPause_clicked);
    QObject::connect(ui->btnRewind, &QPushButton::clicked, this, &MainWindow::btnRewind_clicked);
    QObject::connect(ui->btnSkip, &QPushButton::clicked, this, &MainWindow::btnSkip_clicked);
    QObject::connect(ui->btnRefresh, &QPushButton::clicked, this, &MainWindow::btnRefresh_clicked);
    QObject::connect(&player, &QMediaPlayer::positionChanged, this, &MainWindow::playerPositionChanged);
    QObject::connect(ui->slrProgressBar, &QAbstractSlider::sliderPressed, this, &MainWindow::sliderPressed);
    QObject::connect(ui->slrProgressBar, &QAbstractSlider::sliderMoved, this, &MainWindow::sliderMoved);
    QObject::connect(ui->slrProgressBar, &QAbstractSlider::sliderReleased, this, &MainWindow::sliderReleased);
    QObject::connect(ui->chkEchoesThisDay, &QCheckBox::checkStateChanged, this, &MainWindow::chkEchoesThisDay_clicked);
    QObject::connect(ui->chkAutoplay, &QCheckBox::checkStateChanged, this, &MainWindow::chkAutoplay_clicked);
    QObject::connect(ui->chkYxHdd, &QCheckBox::checkStateChanged, this, &MainWindow::chkYxHdd_clicked);
    QObject::connect(ui->chkYxLaptop, &QCheckBox::checkStateChanged, this, &MainWindow::chkYxLaptop_clicked);
    QObject::connect(ui->chkWinnie, &QCheckBox::checkStateChanged, this, &MainWindow::chkWinnie_clicked);
    QObject::connect(&autoplay, &QTimer::timeout, this, &MainWindow::btnGenerate_clicked);
    QObject::connect(ui->btnSettings, &QPushButton::clicked, this, &MainWindow::btnSettings_clicked);
    QObject::connect(&tray, &QSystemTrayIcon::activated, this, &MainWindow::tray_clicked);

    // Save previous wallpaper.
    wchar_t path[MAX_PATH];
    SystemParametersInfoW(SPI_GETDESKWALLPAPER, MAX_PATH, path, 0);
    previousWallpaperPath = QString::fromWCharArray(path);

    // Initialize system tray icon.
    windowModeAction = trayMenu.addAction("Window Mode");
    windowModeAction->setCheckable(true);
    QObject::connect(windowModeAction, &QAction::triggered, this, &MainWindow::trayWindowModeAction_clicked);
    trayMenu.addSeparator();
    exitAction = trayMenu.addAction("Exit");
    QObject::connect(exitAction, &QAction::triggered, this, &MainWindow::trayExitAction_clicked);
    tray.setIcon(QIcon(":/system/resources/Terriermon16.png"));
    tray.setToolTip("YxWn-Gallery");
    tray.setContextMenu(&trayMenu);
    tray.show();

    // Initialize GUI.
    player.setVideoOutput(ui->vid);
    player.setAudioOutput(audio);
    ui->vid->hide();
    ui->img->hide();
    ui->contPlayerPanel->setEnabled(false);
    ui->btnRewind->setIcons(QIcon(":/system/resources/btnRewind.png"), QIcon(":/system/resources/btnRewindHover.png"), QIcon(":/system/resources/btnRewindPressed.png"));
    ui->btnSkip->setIcons(QIcon(":/system/resources/btnSkip.png"), QIcon(":/system/resources/btnSkipHover.png"), QIcon(":/system/resources/btnSkipPressed.png"));
    ui->btnPlayPause->setIcons(QIcon(":/system/resources/btnPlay.png"), QIcon(":/system/resources/btnPlayHover.png"), QIcon(":/system/resources/btnPlayPressed.png"));
    restoreAppAsWindow();  // Needs to be after system tray icon is initialized and previous wallpaper is saved.

    // Retrieve state/settings.
    QSettings settings("YxWn", "YxWn_Gallery");
    if (settings.contains("Settings")){  // If settings were created.
        ui->chkEchoesThisDay->setCheckState(static_cast<Qt::CheckState>(settings.value("Echoes of This Day").toInt()));
        ui->chkAutoplay->setCheckState(static_cast<Qt::CheckState>(settings.value("Autoplay").toInt()));
        ui->chkYxHdd->setCheckState(static_cast<Qt::CheckState>(settings.value("Yu Xuan HDD").toInt()));
        ui->chkYxLaptop->setCheckState(static_cast<Qt::CheckState>(settings.value("Yu Xuan Laptop").toInt()));
        ui->chkWinnie->setCheckState(static_cast<Qt::CheckState>(settings.value("Winnie").toInt()));
        autoplay.setInterval(settings.value("Autoplay Interval").toInt());
        audio->setMuted(settings.value("Mute").toBool());
        if (settings.value("Rmb Folder").toBool()) {
            dirImages = QDir(settings.value("Current Directory").toString());
            if (dirImages.exists() && dirImages.path() != '.'){  // Prevent missing directory from being accessed.
                ui->lblPath->setText("Path: " + dirImages.path());
                ui->lblPath->adjustSize();
                ui->lblPath->setToolTip(dirImages.path());
                if (settings.value("Refresh Folder on Startup").toBool() && ui->chkYxHdd->checkState() == Qt::Unchecked && ui->chkYxLaptop->checkState() == Qt::Unchecked && ui->chkWinnie->checkState() == Qt::Unchecked) {
                    retrieveFiles();
                    filterFiles();
                }
                else pathList = settings.value("Path List").toStringList();  // Reduces computation from reindexing the directory.
            }
            else {
                ui->lblPath->setText("Path: ");
                ui->lblPath->adjustSize();
            }
        }
        if (settings.value("Rmb Folder").toBool() && settings.value("Rmb File").toBool()) {  // Only check whether to reopen file state if state of folder is retained.
            pathRand = settings.value("Current File").toString();
            if (dirImages.exists(pathRand)) {  // Prevent missing media file from being accessed.
                QString pathFolder = QFileInfo(pathRand).path().remove(dirImages.path());
                if (pathFolder.isEmpty()) pathFolder = "-";
                else pathFolder.removeFirst();
                ui->lblFilePath->setText("Folder: " + pathFolder);
                ui->lblFilePath->adjustSize();
                ui->lblFileName->setText("Name: " + QFileInfo(pathRand).fileName());
                ui->lblFileName->adjustSize();
                QString fileExtension = QFileInfo(pathRand).suffix().toLower();  // Change all letters lowercase. (eg. JPG to jpg)
                if (fileExtension == "png" || fileExtension == "jpg" || fileExtension == "jpeg" || fileExtension == "jfif"){
                    QPixmap imgRand(pathRand);
                    int width = ui->img->width();
                    int height = ui->img->height();
                    ui->img->setPixmap(imgRand.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                    ui->img->setAlignment(Qt::AlignCenter);
                    ui->contPlayerPanel->setEnabled(false);
                    ui->img->show();
                }
                else if (fileExtension == "gif" || fileExtension == "mp4" || fileExtension == "mkv"){
                    player.setSource(QUrl(pathRand));
                    ui->btnPlayPause->setIcons(QIcon(":/system/resources/btnPause.png"), QIcon(":/system/resources/btnPauseHover.png"), QIcon(":/system/resources/btnPausePressed.png"));
                    ui->vid->show();
                    ui->contPlayerPanel->setEnabled(true);
                    player.play();
                }
                else if (fileExtension == "mp3" || fileExtension == "wav"){
                    player.setSource(QUrl(pathRand));
                    QPixmap imgMusic(":/system/resources/imgMusic.png");
                    int width = ui->img->width();
                    int height = ui->img->height();
                    ui->img->setPixmap(imgMusic.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                    ui->img->setAlignment(Qt::AlignCenter);
                    ui->btnPlayPause->setIcons(QIcon(":/system/resources/btnPause.png"), QIcon(":/system/resources/btnPauseHover.png"), QIcon(":/system/resources/btnPausePressed.png"));
                    ui->img->show();
                    ui->contPlayerPanel->setEnabled(true);
                    player.play();
                }
            }
            else {
                ui->lblFilePath->setText("Folder: ");
                ui->lblFilePath->adjustSize();
                ui->lblFileName->setText("Name: ");
                ui->lblFileName->adjustSize();
            }
        }
        if (settings.value("Desktop Wallpaper").toBool()) {
            wallpaperModeAction = trayMenu.addAction("Wallpaper Mode");
            wallpaperModeAction->setCheckable(true);
            QObject::connect(wallpaperModeAction, &QAction::triggered, this, &MainWindow::trayWallpaperModeAction_clicked);
            trayMenu.insertAction(windowModeAction, wallpaperModeAction);
        }
        if (settings.value("Desktop Wallpaper").toBool() && settings.value("Run as Wallpaper on Startup").toBool()) {
            attachAppAsWallpaper();
        }
    }
    else {
        autoplay.setInterval(3000);  // Default autoplay time interval.
        settings.setValue("Rmb Folder", false);
        settings.setValue("Refresh Folder on Startup", false);
        settings.setValue("Rmb File", false);
        settings.setValue("Exit On Close", false);
        settings.setValue("Desktop Wallpaper", false);
        settings.setValue("Run as Wallpaper on Startup", false);
        settings.setValue("Include Picture", true);
        settings.setValue("Include Video", true);
        settings.setValue("Include Audio", true);
    }

    // Display app.
    show();

    qDebug() << "Initialize: " << perf.elapsed() << "ms";
}

MainWindow::~MainWindow()
{
    // Save state/settings.
    QSettings settings("YxWn", "YxWn_Gallery");
    settings.setValue("Settings", "Created");
    if (settings.value("Rmb Folder").toBool()) settings.setValue("Current Directory", dirImages.path());
    if (settings.value("Rmb Folder").toBool()) settings.setValue("Path List", pathList);
    if (settings.value("Rmb Folder").toBool() && settings.value("Rmb File").toBool()) settings.setValue("Current File", pathRand);
    settings.setValue("Echoes of This Day", ui->chkEchoesThisDay->checkState());
    settings.setValue("Autoplay", ui->chkAutoplay->checkState());
    settings.setValue("Yu Xuan HDD", ui->chkYxHdd->checkState());
    settings.setValue("Yu Xuan Laptop", ui->chkYxLaptop->checkState());
    settings.setValue("Winnie", ui->chkWinnie->checkState());
    settings.sync();

    delete ui;
    delete audio;

    SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, (PVOID)previousWallpaperPath.utf16(), SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);  // Reset desktop wallpaper to specific img.
}

HWND MainWindow::getDesktopWorkerW() {
    HWND progman = FindWindow(L"Progman", NULL);
    SendMessageTimeout(progman, 0x052C, 0, 0, SMTO_NORMAL, 1000, nullptr);

    HWND workerw = NULL;
    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        HWND defview = FindWindowEx(hwnd, NULL, L"SHELLDLL_DefView", NULL);
        if (defview != NULL) {
            HWND* ret = reinterpret_cast<HWND*>(lParam);
            *ret = FindWindowEx(NULL, hwnd, L"WorkerW", NULL);
            return FALSE; // stop enumerating
        }
        return TRUE;
    }, reinterpret_cast<LPARAM>(&workerw));

    return workerw;
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

    QElapsedTimer perf;
    perf.start();

    ui->img->clear();  // Clear opened media before opening the next one.
    player.stop();
    player.setSource(QUrl());
    if (pathList.isEmpty()) {
        pathRand.clear();
        ui->img->clear();
        ui->lblFilePath->setText("Folder: ");
        ui->lblFilePath->adjustSize();
        ui->lblFileName->setText("Name: ");
        ui->lblFileName->adjustSize();
        return;
    }
    int indexRand = QRandomGenerator::global()->bounded(pathList.size());
    QString pathRandTemp = dirImages.absoluteFilePath(pathList[indexRand]);
    if (pathRandTemp == pathRand && pathList.length() > 1) {  // Prevent the same media file from being generated. (Edge case: Needs to have at least 2 paths to prevent infinite loop)
        btnGenerate_clicked();
        return;
    }
    else pathRand = pathRandTemp;
    if (!QFile::exists(pathRand)) {  // Prevent missing media file from being accessed.
        return;
    }
    QString fileExtension = QFileInfo(pathRand).suffix().toLower();  // Change all letters lowercase. (eg. JPG to jpg)
    if (fileExtension == "png" || fileExtension == "jpg" || fileExtension == "jpeg" || fileExtension == "jfif"){
        player.stop();
        QPixmap imgRand(pathRand);
        int width = ui->img->width();
        int height = ui->img->height();
        ui->img->setPixmap(imgRand.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        ui->img->setAlignment(Qt::AlignCenter);
        ui->vid->hide();
        ui->contPlayerPanel->setEnabled(false);
        ui->img->show();
    }
    else if (fileExtension == "gif" || fileExtension == "mp4" || fileExtension == "mkv"){
        ui->img->hide();
        player.setSource(QUrl(pathRand));
        ui->btnPlayPause->setIcons(QIcon(":/system/resources/btnPause.png"), QIcon(":/system/resources/btnPauseHover.png"), QIcon(":/system/resources/btnPausePressed.png"));
        ui->vid->show();
        ui->contPlayerPanel->setEnabled(true);
        player.play();
    }
    else if (fileExtension == "mp3" || fileExtension == "wav"){
        ui->vid->hide();
        player.setSource(QUrl(pathRand));
        QPixmap imgMusic(":/system/resources/imgMusic.png");
        int width = ui->img->width();
        int height = ui->img->height();
        ui->img->setPixmap(imgMusic.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        ui->img->setAlignment(Qt::AlignCenter);
        ui->btnPlayPause->setIcons(QIcon(":/system/resources/btnPause.png"), QIcon(":/system/resources/btnPauseHover.png"), QIcon(":/system/resources/btnPausePressed.png"));
        ui->img->show();
        ui->contPlayerPanel->setEnabled(true);
        player.play();
    }
    QString pathFolder = QFileInfo(pathRand).path().remove(dirImages.path());
    if (pathFolder.isEmpty()) pathFolder = "-";
    else pathFolder.removeFirst();
    ui->lblFilePath->setText("Folder: " + pathFolder);
    ui->lblFilePath->adjustSize();
    ui->lblFilePath->setToolTip(pathFolder);
    ui->lblFileName->setText("Name: " + QFileInfo(pathRand).fileName());
    ui->lblFileName->adjustSize();
    ui->lblFileName->setToolTip(QFileInfo(pathRand).fileName());
    ui->contPlayerPanel->setCurrentIndex(1);

    qDebug() << "Generate File: " << perf.elapsed() << "ms";
}

void MainWindow::filterFiles() {
    QElapsedTimer perf;
    perf.start();

    if (!pathList.empty() && ui->chkEchoesThisDay->isChecked()){
        QDate date = QDate::currentDate();
        QString year = date.toString("yyyy");
        QString decade = year.first(2);
        QString month = date.toString("MM");
        QString day = date.toString("dd");
        QRegularExpression regex(QString(".*%1\\d{2}[-_]?%2[-_]?%3.*").arg(decade, month, day));  // Production use
        // static QRegularExpression regex(QString(".*2025[-_]?04[-_]?06.*"));  // Debug Use: Use on qttestfolder.
        QStringList filteredPaths;
        for (QStringList::const_iterator it = pathList.cbegin(); it != pathList.cend(); ++it) {
            if (regex.match(*it).hasMatch()) {
                filteredPaths.append(*it);  // Add paths that match the regex
            }
        }
        pathList = filteredPaths;
    }

    qDebug() << "Filter Files: " << perf.elapsed() << "ms";
}

void MainWindow::retrieveFiles() {
    QElapsedTimer perf;
    perf.start();

    QStringList filters = retrieveFiles_getFilters();
    pathList.clear();  // Clear off buffer of files from previously selected dir.
    retrieveFiles_iterate(dirImages.path(), filters);

    qDebug() << "Retrieve Files: " << perf.elapsed() << "ms";
}

QStringList MainWindow::retrieveFiles_getFilters() {
    QSettings settings("YxWn", "YxWn_Gallery");
    QStringList filters;
    // filters << "*.png" << "*.jpg" << "*.jfif" << "*.jpeg" << "*.mp4" << "*.gif" << "*.mkv" << "*.mp3" << "*.wav";  // Specify the file extensions to accept. (Case insensitive, eg. jpg = JPG)
    if (settings.value("Include Picture").toBool()) {
        filters << "*.png" << "*.jpg" << "*.jfif" << "*.jpeg";
    }
    if (settings.value("Include Video").toBool()) {
        filters << "*.gif" << "*.mp4" << "*.mkv";
    }
    if (settings.value("Include Audio").toBool()) {
        filters << "*.mp3" << "*.wav";
    }
    return filters;
}

void MainWindow::retrieveFiles_iterate(QString dirPath, QStringList filters) {
    QDirIterator iterator(dirPath, filters, QDir::Files, QDirIterator::Subdirectories);  // Automatically ignores "." and ".."
    while (iterator.hasNext()){
        pathList.append(iterator.next());
    }
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
    ui->chkYxHdd->setCheckState(Qt::Unchecked);
    ui->chkYxLaptop->setCheckState(Qt::Unchecked);
    ui->chkWinnie->setCheckState(Qt::Unchecked);
    dirImages.setPath(dir);
    ui->lblPath->setText("Path: " + dirImages.path());
    ui->lblPath->adjustSize();
    ui->lblPath->setToolTip(dirImages.path());
    retrieveFiles();
    filterFiles();
}

void MainWindow::btnPlayPause_clicked()
{
    if (player.isPlaying()){
        player.pause();
        ui->btnPlayPause->setIcons(QIcon(":/system/resources/btnPlay.png"), QIcon(":/system/resources/btnPlayHover.png"), QIcon(":/system/resources/btnPlayPressed.png"));
        ui->btnPlayPause->setIcon(QIcon(":/system/resources/btnPlayHover.png"));
    }
    else {
        if (player.duration() - player.position() < 100) player.setPosition(0);  // Restart the video if the almost ended. (Paused by program due to reached the last few frame of video)
        player.play();
        ui->btnPlayPause->setIcons(QIcon(":/system/resources/btnPause.png"), QIcon(":/system/resources/btnPauseHover.png"), QIcon(":/system/resources/btnPausePressed.png"));
        ui->btnPlayPause->setIcon(QIcon(":/system/resources/btnPauseHover.png"));
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
        ui->btnPlayPause->setIcons(QIcon(":/system/resources/btnPlay.png"), QIcon(":/system/resources/btnPlayHover.png"), QIcon(":/system/resources/btnPlayPressed.png"));
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
        btnGenerate_clicked();
    }
    QWidget::keyPressEvent(event);
}

void MainWindow::chkEchoesThisDay_clicked(Qt::CheckState state) {
    if (state == Qt::Unchecked) {
        if (ui->chkYxHdd->checkState() != Qt::Unchecked) retrieveYxHddFiles();
        else if (ui->chkYxLaptop->checkState() != Qt::Unchecked) retrieveYxLaptopFiles();
        else retrieveFiles();
    }
    else {  // Qt::Checked or Qt::PartiallyChecked
        filterFiles();
    }
}

void MainWindow::chkAutoplay_clicked(Qt::CheckState state) {
    if (state == Qt::Unchecked) {
        autoplay.stop();
    }
    else {
        autoplay.start();
    }
}

void MainWindow::chkYxHdd_clicked(Qt::CheckState state) {
    if (state == Qt::Unchecked) {
        if (!previousDirImages.exists() || previousDirImages.path() == '.'){
            dirImages = QDir();
            pathList.clear();
            pathRand.clear();
            ui->lblPath->setText("Path: ");
            ui->lblPath->adjustSize();
            ui->lblFilePath->setText("Folder: ");
            ui->lblFilePath->adjustSize();
            ui->lblFileName->setText("Name: ");
            ui->lblFileName->adjustSize();
            return;
        }
        dirImages = previousDirImages;
        pathList = previousPathList;
        ui->lblPath->setText("Path: " + dirImages.path());
        ui->lblPath->adjustSize();
        btnGenerate_clicked();
    }
    else {
        ui->chkWinnie->setCheckState(Qt::Unchecked);
        ui->chkYxLaptop->setCheckState(Qt::Unchecked);
        previousDirImages = dirImages;
        previousPathList = pathList;
        dirImages = QDir();
        pathRand.clear();
        ui->lblFilePath->setText("Folder: ");
        ui->lblFilePath->adjustSize();
        ui->lblFileName->setText("Name: ");
        ui->lblFileName->adjustSize();
        retrieveYxHddFiles();
        filterFiles();
    }
}

void MainWindow::retrieveYxHddFiles() {
    QElapsedTimer perf;
    perf.start();

    QDir drivePath = getSeagateDrivePath();
    if (!drivePath.exists()) return;
    dirImages.setPath(drivePath.path());
    ui->lblPath->setText("Path: " + dirImages.path());
    ui->lblPath->adjustSize();
    ui->lblPath->setToolTip(dirImages.path());
    pathList.clear();

    QStringList dirList = {
        "Random files/Since 2023/2025",  // Media 2025
        "个人project/Precious Moments",  // Precious Moments
        "个人project/Life",  // Life
        "个人project/Programming Life",  // Programming
        "个人project/Artwork Room",  // Art
        "个人project/Music Square",  // Music
    };
    QStringList filters = retrieveFiles_getFilters();
    for (const QString &partialDir : dirList){
        QString fullDir = QDir(dirImages.path()).filePath(partialDir);
        retrieveFiles_iterate(fullDir, filters);
    }

    qDebug() << "Retrieve Files: " << perf.elapsed() << "ms";
}

QDir MainWindow::getSeagateDrivePath() {
    QDir drivePath = QDir(findDriveByDeviceName("Seagate Yx 2t") + "YuXuanFiles");
    return drivePath;
}

QString MainWindow::findDriveByDeviceName(const QString &deviceName) {
    foreach (const QStorageInfo &storage, QStorageInfo::mountedVolumes()) {
        if (storage.isValid() && storage.isReady()) {
            QString label = storage.displayName(); // Or storage.name().

            if (label.contains(deviceName, Qt::CaseInsensitive)) {
                return storage.rootPath(); // The mount point or drive letter.
            }
        }
    }
    return QString(); // If no drive found.
}

void MainWindow::chkYxLaptop_clicked(Qt::CheckState state) {
    if (state == Qt::Unchecked) {
        if (!previousDirImages.exists() || previousDirImages.path() == '.'){
            dirImages = QDir();
            pathList.clear();
            pathRand.clear();
            ui->lblPath->setText("Path: ");
            ui->lblPath->adjustSize();
            ui->lblFilePath->setText("Folder: ");
            ui->lblFilePath->adjustSize();
            ui->lblFileName->setText("Name: ");
            ui->lblFileName->adjustSize();
            return;
        }
        dirImages = previousDirImages;
        pathList = previousPathList;
        ui->lblPath->setText("Path: " + dirImages.path());
        ui->lblPath->adjustSize();
        btnGenerate_clicked();
    }
    else {
        ui->chkYxHdd->setCheckState(Qt::Unchecked);
        ui->chkWinnie->setCheckState(Qt::Unchecked);
        previousDirImages = dirImages;
        previousPathList = pathList;
        dirImages = QDir();
        pathRand.clear();
        ui->lblFilePath->setText("Folder: ");
        ui->lblFilePath->adjustSize();
        ui->lblFileName->setText("Name: ");
        ui->lblFileName->adjustSize();
        retrieveYxLaptopFiles();
        filterFiles();
    }
}

void MainWindow::retrieveYxLaptopFiles() {
    QElapsedTimer perf;
    perf.start();

    QDir desktopPath = QDir("C:/Users/Admin/Desktop");
    if (!desktopPath.exists()) return;
    dirImages.setPath(desktopPath.path());
    ui->lblPath->setText("Path: " + dirImages.path());
    ui->lblPath->adjustSize();
    ui->lblPath->setToolTip(dirImages.path());
    pathList.clear();

    QStringList dirList = {
        "Precious Moments",  // Precious Moments
        "Life",  // Life
        "Programming Life",  // Programming
        "Artwork Room",  // Art
        "Music Square",  // Music
        // "Pending Uploads"  // Pending Uploads
    };
    QStringList filters = retrieveFiles_getFilters();
    for (const QString &partialDir : dirList){
        QString fullDir = QDir(dirImages.path()).filePath(partialDir);
        retrieveFiles_iterate(fullDir, filters);
    }

    qDebug() << "Retrieve Files: " << perf.elapsed() << "ms";
}

void MainWindow::chkWinnie_clicked(Qt::CheckState state) {
    if (state == Qt::Unchecked) {
        if (!previousDirImages.exists() || previousDirImages.path() == '.'){
            dirImages = QDir();
            pathList.clear();
            pathRand.clear();
            ui->lblPath->setText("Path: ");
            ui->lblPath->adjustSize();
            ui->lblFilePath->setText("Folder: ");
            ui->lblFilePath->adjustSize();
            ui->lblFileName->setText("Name: ");
            ui->lblFileName->adjustSize();
            return;
        }
        dirImages = previousDirImages;
        pathList = previousPathList;
        ui->lblPath->setText("Path: " + dirImages.path());
        ui->lblPath->adjustSize();
        btnGenerate_clicked();
    }
    else {
        ui->chkYxHdd->setCheckState(Qt::Unchecked);
        ui->chkYxLaptop->setCheckState(Qt::Unchecked);
        previousDirImages = dirImages;
        previousPathList = pathList;
        dirImages = QDir();
        pathRand.clear();
        ui->lblFilePath->setText("Folder: ");
        ui->lblFilePath->adjustSize();
        ui->lblFileName->setText("Name: ");
        ui->lblFileName->adjustSize();
        QDir drivePath = getSeagateDrivePath();
        if (!drivePath.exists()) return;
        QString partialDir = "个人project/Winnie Lo 罗玲玲";
        QString fullDir = QDir(drivePath.path()).filePath(partialDir);
        dirImages.setPath(fullDir);
        ui->lblPath->setText("Path: " + dirImages.path());
        ui->lblPath->adjustSize();
        ui->lblPath->setToolTip(dirImages.path());
        retrieveFiles();
        filterFiles();
    }
}

void MainWindow::btnSettings_clicked() {
    autoplay.stop();
    player.pause();
    ui->btnPlayPause->setIcons(QIcon(":/system/resources/btnPlay.png"), QIcon(":/system/resources/btnPlayHover.png"), QIcon(":/system/resources/btnPlayPressed.png"));
    SettingsWindow* sw = new SettingsWindow(this);
    sw->setAttribute(Qt::WA_DeleteOnClose);
    sw->setWindowTitle("Settings");
    sw->exec();
    chkAutoplay_clicked(ui->chkAutoplay->checkState());  // Resume autoplay if chkAutoplay is checked.
}

void MainWindow::tray_clicked(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger) {  // The icon was clicked.
        if (windowFlags() == (Qt::Window | Qt::FramelessWindowHint)) {  // All window flags that are implicitly set when 'Qt::FramelessWindowHint' is set.
            restoreAppAsWindow();
        }
        else if (windowFlags() == (Qt::Window | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint | Qt::WindowFullscreenButtonHint)) {  // All window flags that are implicitly set when 'Qt::Window' is set.
            attachAppAsWallpaper();
        }

        // Bring the applicaiton to the front.
        show();
        raise();
        activateWindow();
    }
}

void MainWindow::closeEvent(QCloseEvent *event) {
    QSettings settings("YxWn", "YxWn_Gallery");
    if (!forceExit && !settings.value("Exit On Close").toBool()){
        hide();  // Close the app window.
        if (settings.value("Desktop Wallpaper").toBool()) {
            attachAppAsWallpaper();
            show();
        }
        event->ignore();  // Prevent app from quitting.
    }
}

void MainWindow::btnRefresh_clicked() {
    if (ui->chkYxHdd->checkState() != Qt::Unchecked) retrieveYxHddFiles();
    else if (ui->chkYxLaptop->checkState() != Qt::Unchecked) retrieveYxLaptopFiles();
    else retrieveFiles();
    filterFiles();
    QMessageBox msgBox;
    msgBox.setText("Folder refreshed.");
    msgBox.exec();
}


void MainWindow::attachAppAsWallpaper() {  // Will only be applied when 'show()' function is called.
    if (isMinimized()) showNormal();  // Restore from minimized state.
    wallpaperModeAction->setChecked(true);
    windowModeAction->setChecked(false);
    setWindowFlags(Qt::FramelessWindowHint);
    QScreen *screen = qApp->primaryScreen();
    QRect screenGeo = screen->geometry();
    screenGeo.setTop(screenGeo.top() - 40);  // Add window margin.
    setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), screenGeo));  // Should be set after the set window flags (type) to prevent false position calculation.
    HWND hwnd = (HWND)winId();
    HWND workerw = getDesktopWorkerW();
    SetParent(hwnd, workerw);  // Application position (geometry) needs to be set before this function call.
}

void MainWindow::restoreAppAsWindow() {
    wallpaperModeAction->setChecked(false);
    windowModeAction->setChecked(true);
    setWindowFlags(Qt::Window);  // Need to call 'show()' function to apply.
    QScreen *screen = qApp->primaryScreen();
    QRect screenGeo = screen->geometry();
    screenGeo.setTop(screenGeo.top() - 10);  // Add window margin.
    setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), screenGeo));  // Should be set after the set window flags (type) to prevent false position calculation.
    HWND hwnd = (HWND)winId();
    SetParent(hwnd, NULL);
    SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, (PVOID)previousWallpaperPath.utf16(), SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);  // Reset desktop wallpaper to specific img.
}

void MainWindow::trayExitAction_clicked(bool checked) {
    Q_UNUSED(checked);
    forceExit = true;
    qApp -> quit();
}

void MainWindow::trayWallpaperModeAction_clicked(bool checked) {
    Q_UNUSED(checked);
    attachAppAsWallpaper();
    show();
}

void MainWindow::trayWindowModeAction_clicked(bool checked) {
    Q_UNUSED(checked);
    restoreAppAsWindow();
    show();
}
