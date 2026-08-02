#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "settingswindow.h"
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
#include <QLineEdit>
#include <QProgressDialog>
#include <QThread>
#include <QImageReader>

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
    QObject::connect(&player, &QMediaPlayer::positionChanged, this, &MainWindow::player_positionChanged);
    QObject::connect(ui->slrProgressBar, &QAbstractSlider::sliderPressed, this, &MainWindow::slrProgressBar_pressed);
    QObject::connect(ui->slrProgressBar, &QAbstractSlider::sliderMoved, this, &MainWindow::slrProgressBar_moved);
    QObject::connect(ui->slrProgressBar, &QAbstractSlider::sliderReleased, this, &MainWindow::slrProgressBar_released);
    QObject::connect(ui->chkEchoesThisDay, &QCheckBox::checkStateChanged, this, &MainWindow::chkEchoesThisDay_clicked);
    QObject::connect(ui->chkAutoplay, &QCheckBox::checkStateChanged, this, &MainWindow::chkAutoplay_clicked);
    QObject::connect(ui->chkYxHdd, &QCheckBox::checkStateChanged, this, &MainWindow::chkYxHdd_clicked);
    QObject::connect(ui->chkYxLaptop, &QCheckBox::checkStateChanged, this, &MainWindow::chkYxLaptop_clicked);
    QObject::connect(ui->chkWinnie, &QCheckBox::checkStateChanged, this, &MainWindow::chkWinnie_clicked);
    QObject::connect(ui->spnEchoesThisDay, &QSpinBox::valueChanged, this, &MainWindow::spnEchoesThisDay_valueChanged);
    QObject::connect(&autoplay, &QTimer::timeout, this, &MainWindow::generateMedia);
    QObject::connect(ui->btnSettings, &QPushButton::clicked, this, &MainWindow::btnSettings_clicked);
    QObject::connect(&tray, &QSystemTrayIcon::activated, this, &MainWindow::tray_clicked);

    // Save previous wallpaper.
    wchar_t path[MAX_PATH];
    SystemParametersInfoW(SPI_GETDESKWALLPAPER, MAX_PATH, path, 0);
    prevWallpaperPath = QString::fromWCharArray(path);

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
    int currentYear = QDate::currentDate().year();
    player.setVideoOutput(ui->vid);
    player.setAudioOutput(audio);
    ui->vid->hide();
    ui->img->hide();
    ui->contPlayerPanel->setEnabled(false);
    ui->btnRewind->setIcons(QIcon(":/system/resources/btnRewind.png"), QIcon(":/system/resources/btnRewindHover.png"), QIcon(":/system/resources/btnRewindPressed.png"));
    ui->btnSkip->setIcons(QIcon(":/system/resources/btnSkip.png"), QIcon(":/system/resources/btnSkipHover.png"), QIcon(":/system/resources/btnSkipPressed.png"));
    ui->btnPlayPause->setIcons(QIcon(":/system/resources/btnPlay.png"), QIcon(":/system/resources/btnPlayHover.png"), QIcon(":/system/resources/btnPlayPressed.png"));
    ui->spnEchoesThisDay->setValue(currentYear);
    ui->spnEchoesThisDay->setMaximum(currentYear);
    ui->spnEchoesThisDay->setVisible(false);
    restoreAppAsWindow();  // Show App after GUI is initialized. (Note: This function would only work if it is called after the system tray icon is initialized and the previous wallpaper saved)

    // Retrieve state/settings.
    QSettings settings("YxWn", "YxWn_Gallery");
    if (settings.contains("Settings")){  // If settings had been initialized.
        ui->chkEchoesThisDay->setCheckState(static_cast<Qt::CheckState>(settings.value("Echoes of This Day").toInt()));
        ui->spnEchoesThisDay->setVisible(static_cast<Qt::CheckState>(settings.value("Echoes of This Day").toInt()));
        ui->chkAutoplay->setCheckState(static_cast<Qt::CheckState>(settings.value("Autoplay").toInt()));
        ui->chkYxHdd->setCheckState(static_cast<Qt::CheckState>(settings.value("Yu Xuan HDD").toInt()));
        ui->chkYxLaptop->setCheckState(static_cast<Qt::CheckState>(settings.value("Yu Xuan Laptop").toInt()));
        ui->chkWinnie->setCheckState(static_cast<Qt::CheckState>(settings.value("Winnie").toInt()));
        autoplay.setInterval(settings.value("Autoplay Interval").toInt());
        if (settings.value("Mute").toBool() || (settings.value("Desktop Wallpaper").toBool() && settings.value("Run as Wallpaper on Startup").toBool() && settings.value("Mute in Wallpaper").toBool())) {
            audio->setMuted(true);
        } else {
            audio->setMuted(false);
        }
        if (settings.value("Rmb Folder").toBool()) {
            QDir savedRootDirPath = QDir(settings.value("Root Dir Path").toString());
            if (savedRootDirPath.exists() && savedRootDirPath.path() != '.'){  // Prevent missing directory from being accessed.
                rootDirPath = savedRootDirPath;
                ui->lblPath->setText("Path: " + savedRootDirPath.path());
                ui->lblPath->adjustSize();
                ui->lblPath->setToolTip(savedRootDirPath.path());
                if (settings.value("Refresh Folder on Startup").toBool()) {
                    if (ui->chkWinnie->checkState() != Qt::Unchecked) retrieveWinnie();
                    else if (ui->chkYxHdd->checkState() != Qt::Unchecked) retrieveYxHdd();
                    else if (ui->chkYxLaptop->checkState() != Qt::Unchecked) retrieveYxLaptop();
                    else retrieveSelectFolder();
                }
                else {
                    completeFilePathList = settings.value("Complete File Path List").toStringList();  // Reduces computation from reindexing the directory.
                    filterFiles();
                }
            }
            else {
                ui->lblPath->setText("Path: ");
                ui->lblPath->adjustSize();
            }
        }
        if (settings.value("Rmb Folder").toBool() && settings.value("Rmb File").toBool()) {  // Only check whether to reopen file state if state of folder is retained.
            QDir savedRootDirPath = QDir(settings.value("Root Dir Path").toString());
            QString savedDisplayFilePath = settings.value("Display File Path").toString();
            if (savedRootDirPath.exists(savedDisplayFilePath)) {  // Prevent missing media file from being accessed.
                rootDirPath = savedRootDirPath;
                displayFilePath = savedDisplayFilePath;

                displayMedia();
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
        // Initialize settings for first-time users.
        autoplay.setInterval(3000);  // Default autoplay time interval.
        settings.setValue("Mute", false);
        settings.setValue("Mute in Wallpaper", true);
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

    show();  // Display app.

    qDebug() << "Initialize: " << perf.elapsed() << "ms";
}

MainWindow::~MainWindow()
{
    // Save state/settings.
    QSettings settings("YxWn", "YxWn_Gallery");
    settings.setValue("Settings", "Created");
    if (settings.value("Rmb Folder").toBool()) settings.setValue("Root Dir Path", rootDirPath.path());
    if (settings.value("Rmb Folder").toBool()) settings.setValue("Complete File Path List", completeFilePathList);
    if (settings.value("Rmb Folder").toBool() && settings.value("Rmb File").toBool()) settings.setValue("Display File Path", displayFilePath);
    settings.setValue("Echoes of This Day", ui->chkEchoesThisDay->checkState());
    settings.setValue("Autoplay", ui->chkAutoplay->checkState());
    settings.setValue("Yu Xuan HDD", ui->chkYxHdd->checkState());
    settings.setValue("Yu Xuan Laptop", ui->chkYxLaptop->checkState());
    settings.setValue("Winnie", ui->chkWinnie->checkState());
    settings.sync();

    delete ui;
    delete audio;

    SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, (PVOID)prevWallpaperPath.utf16(), SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);  // Reset desktop wallpaper to specific img.
}

void MainWindow::generateMedia()
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
    ui->lblCurrentTime->setText("00:00:00");
    ui->lblRemainingTime->setText("00:00:00");
    ui->slrProgressBar->setValue(0);
    if (filteredFilePathList.isEmpty()) {
        displayFilePath.clear();
        ui->img->clear();
        ui->lblFilePath->setText("Folder: ");
        ui->lblFilePath->adjustSize();
        ui->lblFileName->setText("Name: ");
        ui->lblFileName->adjustSize();
        return;
    }

    int idx = QRandomGenerator::global()->bounded(filteredFilePathList.size());
    QString generatedFilePath = rootDirPath.absoluteFilePath(filteredFilePathList[idx]);
    if (generatedFilePath == displayFilePath && filteredFilePathList.length() > 1) {  // Prevent the same media file from being generated. (Edge case: Needs to have at least 2 paths to prevent infinite loop)
        generateMedia();
        return;
    }
    else displayFilePath = generatedFilePath;
    if (!QFile::exists(displayFilePath)) return;  // Prevent missing media file from being accessed.

    displayMedia();

    qDebug() << "Generate Media: " << perf.elapsed() << "ms";
}

void MainWindow::btnGenerate_clicked()
{
    generateMedia();
}

void MainWindow::btnSelectFolder_clicked()
{
    QString dir = QFileDialog::getExistingDirectory();  // Get user-selected directory from windows dialog box.
    if (dir.isEmpty()) return;

    ui->chkYxHdd->setCheckState(Qt::Unchecked);
    ui->chkYxLaptop->setCheckState(Qt::Unchecked);
    ui->chkWinnie->setCheckState(Qt::Unchecked);

    rootDirPath.setPath(dir);
    ui->lblPath->setText("Path: " + rootDirPath.path());
    ui->lblPath->adjustSize();
    ui->lblPath->setToolTip(rootDirPath.path());

    retrieveSelectFolder();
}

void MainWindow::btnRefresh_clicked() {
    if (ui->chkWinnie->checkState() != Qt::Unchecked) retrieveWinnie();
    else if (ui->chkYxHdd->checkState() != Qt::Unchecked) retrieveYxHdd();
    else if (ui->chkYxLaptop->checkState() != Qt::Unchecked) retrieveYxLaptop();
    else retrieveSelectFolder();
}

void MainWindow::btnSettings_clicked() {
    autoplay.stop();
    player.pause();
    ui->btnPlayPause->setIcons(QIcon(":/system/resources/btnPlay.png"), QIcon(":/system/resources/btnPlayHover.png"), QIcon(":/system/resources/btnPlayPressed.png"));

    // Open Settings Window
    SettingsWindow* sw = new SettingsWindow(this);
    sw->setAttribute(Qt::WA_DeleteOnClose);
    sw->setWindowTitle("Settings");
    sw->exec();

    // After Settings Window closed
    filterFiles();
    chkAutoplay_clicked(ui->chkAutoplay->checkState());  // Resume autoplay if chkAutoplay is checked.
    QSettings settings("YxWn", "YxWn_Gallery");
    if (windowFlags() == Qt::FramelessWindowHint && settings.value("Mute in Wallpaper").toBool()) {
        audio->setMuted(true);
    } else {
        audio->setMuted(settings.value("Mute").toBool());
    }
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

void MainWindow::player_positionChanged(qint64 position){
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

void MainWindow::slrProgressBar_pressed() {
    player.pause();
}

void MainWindow::slrProgressBar_moved(int value) {
    player.setPosition(value);
}

void MainWindow::slrProgressBar_released() {
    player.play();
}

void MainWindow::chkAutoplay_clicked(Qt::CheckState state) {
    if (state == Qt::Unchecked) autoplay.stop();
    else autoplay.start();
}

void MainWindow::chkEchoesThisDay_clicked(Qt::CheckState state) {
    if (state == Qt::Unchecked) {
        ui->spnEchoesThisDay->setVisible(false);
        filteredFilePathList = completeFilePathList;
    }
    else {  // Qt::Checked or Qt::PartiallyChecked
        ui->spnEchoesThisDay->setVisible(true);
        int currentYear = QDate::currentDate().year();
        if (ui->spnEchoesThisDay->value() == currentYear) filterFiles();
        else ui->spnEchoesThisDay->setValue(currentYear);
    }
}

void MainWindow::chkWinnie_clicked(Qt::CheckState state) {
    if (state == Qt::Unchecked) {
        if (!prevRootDirPath.exists() || prevRootDirPath.path() == '.'){
            rootDirPath = QDir();
            completeFilePathList.clear();
            filteredFilePathList.clear();
            displayFilePath.clear();
            ui->lblPath->setText("Path: ");
            ui->lblPath->adjustSize();
            ui->lblFilePath->setText("Folder: ");
            ui->lblFilePath->adjustSize();
            ui->lblFileName->setText("Name: ");
            ui->lblFileName->adjustSize();
            return;
        }
        else {
            rootDirPath = prevRootDirPath;
            completeFilePathList = prevCompleteFilePathList;
            ui->lblPath->setText("Path: " + rootDirPath.path());
            ui->lblPath->adjustSize();

            filterFiles();
            generateMedia();
        }
    }
    else {
        ui->chkYxHdd->setCheckState(Qt::Unchecked);
        ui->chkYxLaptop->setCheckState(Qt::Unchecked);

        prevRootDirPath = rootDirPath;
        prevCompleteFilePathList = completeFilePathList;
        rootDirPath = QDir();
        displayFilePath.clear();
        ui->lblFilePath->setText("Folder: ");
        ui->lblFilePath->adjustSize();
        ui->lblFileName->setText("Name: ");
        ui->lblFileName->adjustSize();

        retrieveWinnie();
    }
}

void MainWindow::chkYxHdd_clicked(Qt::CheckState state) {
    if (state == Qt::Unchecked) {
        if (!prevRootDirPath.exists() || prevRootDirPath.path() == '.'){
            rootDirPath = QDir();
            completeFilePathList.clear();
            filteredFilePathList.clear();
            displayFilePath.clear();
            ui->lblPath->setText("Path: ");
            ui->lblPath->adjustSize();
            ui->lblFilePath->setText("Folder: ");
            ui->lblFilePath->adjustSize();
            ui->lblFileName->setText("Name: ");
            ui->lblFileName->adjustSize();
            return;
        }
        else {
            rootDirPath = prevRootDirPath;
            completeFilePathList = prevCompleteFilePathList;
            ui->lblPath->setText("Path: " + rootDirPath.path());
            ui->lblPath->adjustSize();

            filterFiles();
            generateMedia();
        }
    }
    else {
        ui->chkWinnie->setCheckState(Qt::Unchecked);
        ui->chkYxLaptop->setCheckState(Qt::Unchecked);

        prevRootDirPath = rootDirPath;
        prevCompleteFilePathList = completeFilePathList;
        rootDirPath = QDir();
        displayFilePath.clear();
        ui->lblFilePath->setText("Folder: ");
        ui->lblFilePath->adjustSize();
        ui->lblFileName->setText("Name: ");
        ui->lblFileName->adjustSize();

        retrieveYxHdd();
    }
}

void MainWindow::chkYxLaptop_clicked(Qt::CheckState state) {
    if (state == Qt::Unchecked) {
        if (!prevRootDirPath.exists() || prevRootDirPath.path() == '.'){
            rootDirPath = QDir();
            completeFilePathList.clear();
            filteredFilePathList.clear();
            displayFilePath.clear();
            ui->lblPath->setText("Path: ");
            ui->lblPath->adjustSize();
            ui->lblFilePath->setText("Folder: ");
            ui->lblFilePath->adjustSize();
            ui->lblFileName->setText("Name: ");
            ui->lblFileName->adjustSize();
            return;
        }
        else {
            rootDirPath = prevRootDirPath;
            completeFilePathList = prevCompleteFilePathList;
            ui->lblPath->setText("Path: " + rootDirPath.path());
            ui->lblPath->adjustSize();

            filterFiles();
            generateMedia();
        }
    }
    else {
        ui->chkYxHdd->setCheckState(Qt::Unchecked);
        ui->chkWinnie->setCheckState(Qt::Unchecked);

        prevRootDirPath = rootDirPath;
        prevCompleteFilePathList = completeFilePathList;
        rootDirPath = QDir();
        displayFilePath.clear();
        ui->lblFilePath->setText("Folder: ");
        ui->lblFilePath->adjustSize();
        ui->lblFileName->setText("Name: ");
        ui->lblFileName->adjustSize();

        retrieveYxLaptop();
    }
}

void MainWindow::spnEchoesThisDay_valueChanged() {
    QTimer::singleShot(0, ui->spnEchoesThisDay->findChild<QLineEdit*>(), &QLineEdit::deselect); // Prevent text highlighting of spnEchoesThisDay.

    filterFiles();
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

void MainWindow::trayExitAction_clicked(bool checked) {
    Q_UNUSED(checked);
    forceExit = true;
    qApp -> quit();
}

void MainWindow::trayWindowModeAction_clicked(bool checked) {
    Q_UNUSED(checked);
    restoreAppAsWindow();
    show();
}

void MainWindow::trayWallpaperModeAction_clicked(bool checked) {
    Q_UNUSED(checked);
    attachAppAsWallpaper();
    show();
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
                float posRatio = static_cast<float>(localPos.x()) / slrWidth;  // Get the current clicked position on the slider in relation to its entire bar. (x coordinate = horizontal prgDialog bar) (how many duration passed over (/) the total duration)
                int newSliderPos = player.duration() * posRatio;  // Multiply the ratio with the total media duration to get the new time position that the user have clicked on.
                player.setPosition(newSliderPos);
                return true;
            }
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Space) generateMedia();
    else if (ui->chkEchoesThisDay->checkState() != Qt::Unchecked && (event->key() == Qt::Key_Up || event->key() == Qt::Key_Down)) {

        // Arrow key refreshes filteredFilePathList based on current spnEchoesThisDay value.
        if (event->key() == Qt::Key_Up) ui->spnEchoesThisDay->stepUp();
        else ui->spnEchoesThisDay->stepDown();

        // Prevent text highlighting of spnEchoesThisDay.
        QLineEdit *lineEdit = ui->spnEchoesThisDay->findChild<QLineEdit*>();
        if (lineEdit) {
            QTimer::singleShot(0, lineEdit, &QLineEdit::deselect);
        }
    }
    QWidget::keyPressEvent(event);
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

void MainWindow::retrieveSelectFolder() {
    QStringList partialDirList = {
        rootDirPath.path()
    };

    retrievePreset(partialDirList);
}

void MainWindow::retrieveWinnie() {
    QDir drivePath = getSeagateDrivePath();
    if (!drivePath.exists()) return;
    rootDirPath.setPath(drivePath.path());
    ui->lblPath->setText("Path: " + rootDirPath.path());
    ui->lblPath->adjustSize();
    ui->lblPath->setToolTip(rootDirPath.path());

    QStringList partialDirList = {
        "个人project/Winnie Lo 罗玲玲",
        "个人project/Beatbreak Trio"
    };

    retrievePreset(partialDirList);
}

void MainWindow::retrieveYxHdd() {
    QDir drivePath = getSeagateDrivePath();

    if (!drivePath.exists()) return;
    rootDirPath.setPath(drivePath.path());
    ui->lblPath->setText("Path: " + rootDirPath.path());
    ui->lblPath->adjustSize();
    ui->lblPath->setToolTip(rootDirPath.path());

    QStringList partialDirList = {
        "Media files",  // Media Files
        "Career", // Career
        "个人project/Artwork Room",  // Art
        "个人project/Gaming Cafe",  // Games
        "个人project/Language Cottage",  // Language
        "个人project/Life",  // Life
        "个人project/Music Square",  // Music
        "个人project/Pets",  // Pets
        "个人project/Precious Moments",  // Precious Moments
        "个人project/Programming Life",  // Programming
    };

    retrievePreset(partialDirList);
}

void MainWindow::retrieveYxLaptop() {
    QDir desktopPath = QDir("C:/Users/Admin/Desktop");
    if (!desktopPath.exists()) return;
    rootDirPath.setPath(desktopPath.path());
    ui->lblPath->setText("Path: " + rootDirPath.path());
    ui->lblPath->adjustSize();
    ui->lblPath->setToolTip(rootDirPath.path());

    QStringList partialDirList = {
        "Artwork Room",  // Art
        "Language Cottage", // Language
        "Life",  // Life
        "Music Square",  // Music
        "Pending Uploads"  // Pending Uploads
        "Precious Moments",  // Precious Moments
        "Programming Life",  // Programming
    };

    retrievePreset(partialDirList);
}

void MainWindow::retrievePreset(QStringList partialDirList) {
    QElapsedTimer perf;
    perf.start();

    completeFilePathList.clear();
    filteredFilePathList.clear();

    retrieveFiles(partialDirList);

    filterFiles();

    qDebug() << "Retrieve Preset: " << perf.elapsed() << "ms";
}

void MainWindow::retrieveFiles(QStringList partialDirList) {
    QElapsedTimer perf;
    perf.start();

    int fileCount = partialDirList.size();
    QProgressDialog prgDialog("Retrieving files...", "Cancel", 0, fileCount, this);
    prgDialog.setWindowModality(Qt::WindowModal);
    prgDialog.setMinimumDuration(1);
    // prgDialog.show();
    for (int i = 0; i < fileCount; i++) {
        prgDialog.setValue(i);

        QString partialDirPath = QDir(rootDirPath.path()).filePath(partialDirList[i]);
        QDirIterator iterator(partialDirPath, fileExtFilters_full(), QDir::Files, QDirIterator::Subdirectories);  // Automatically ignores "." and ".."
        while (iterator.hasNext()){
            completeFilePathList.append(iterator.next());

            QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);

            if (prgDialog.wasCanceled()) break;
        }

        if (prgDialog.wasCanceled()) {
            completeFilePathList.clear();  // Revert to original file path list before retrieving (empty list).
            break;
        }
    }
    prgDialog.setValue(fileCount);

    qDebug() << "Retrieve Files: " << perf.elapsed() << "ms";
}

void MainWindow::filterFiles() {
    QElapsedTimer perf;
    perf.start();

    QStringList filePathList = completeFilePathList;

    QSettings settings("YxWn", "YxWn_Gallery");
    QStringList fileExitFilters;
    if (settings.value("Include Picture").toBool() || settings.value("Include Video").toBool() || settings.value("Include Audio").toBool())
        filePathList = filterFiles_userFileExt(filePathList);

    if (ui->chkEchoesThisDay->isChecked()) filePathList = filterFiles_year(filePathList);

    filteredFilePathList = filePathList;

    qDebug() << "Filter Files: " << perf.elapsed() << "ms";
}

QStringList MainWindow::filterFiles_userFileExt(QStringList filePathList) {
    QStringList filteredFilePaths;
    QRegularExpression regex(QString("\\.(%1)$").arg(fileExtFilters_user().join("|")), QRegularExpression::CaseInsensitiveOption);  // Gets all files with fileExt that matches the fileExtFilters enabled by user.

    int fileCount = filePathList.size();
    QProgressDialog prgDialog("Filtering files (File Extension)...", "Cancel", 0, fileCount, this);
    prgDialog.setWindowModality(Qt::WindowModal);
    prgDialog.setMinimumDuration(1);
    for (int i = 0; i < fileCount; i++) {
        prgDialog.setValue(i);

        if (prgDialog.wasCanceled()) {
            filteredFilePaths = filePathList;  // Revert to original file path list before filtering.
            break;
        }

        if (regex.match(filePathList[i]).hasMatch()) {
            filteredFilePaths.append(filePathList[i]);  // Add paths that match the regex.
        }
    }
    prgDialog.setValue(fileCount);
    return filteredFilePaths;
}

QStringList MainWindow::filterFiles_year(QStringList filePathList) {
    QStringList filteredFilePaths;
    QDate dateToday = QDate::currentDate();
    QString setYear = QString::number(ui->spnEchoesThisDay->value());
    QString month = dateToday.toString("MM");
    QString day = dateToday.toString("dd");
    // static QRegularExpression regex(QString(".*2025[-_]?04[-_]?06.*"));  // Debug Use: Use on qttestfolder.
    QRegularExpression regex(QString(".*%1[-_]?%2[-_]?%3.*").arg(setYear, month, day));  // Production use.

    int fileCount = filePathList.size();
    QProgressDialog prgDialog("Filtering files (Year)...", "Cancel", 0, fileCount, this);
    prgDialog.setWindowModality(Qt::WindowModal);
    prgDialog.setMinimumDuration(1);
    for (int i = 0; i < fileCount; i++) {
        prgDialog.setValue(i);

        if (prgDialog.wasCanceled()) {
            filteredFilePaths = filePathList;  // Revert to original file path list before filtering.
            break;
        }

        if (regex.match(filePathList[i]).hasMatch()) {
            filteredFilePaths.append(filePathList[i]);  // Add paths that match the regex.
        }
    }
    prgDialog.setValue(fileCount);
    return filteredFilePaths;
}

QStringList MainWindow::fileExtFilters_full() {
    // List of all the file extensions searched by this app. (Case insensitive, eg. jpg = JPG)
    QStringList fileExitFilters;
    fileExitFilters << "*.png" << "*.jpg" << "*.jfif" << "*.jpeg" << "*.gif" << "*.mp4" << "*.mkv" << "*.mp3" << "*.wav";
    return fileExitFilters;
}

QStringList MainWindow::fileExtFilters_user() {
    // List of all the file extensions enabled by the user. (Case insensitive, eg. jpg = JPG)
    QSettings settings("YxWn", "YxWn_Gallery");
    QStringList fileExitFilters;
    if (settings.value("Include Picture").toBool()) fileExitFilters << "png" << "jpg" << "jfif" << "jpeg";
    if (settings.value("Include Video").toBool()) fileExitFilters << "gif" << "mp4" << "mkv";
    if (settings.value("Include Audio").toBool()) fileExitFilters << "mp3" << "wav";
    return fileExitFilters;
}

void MainWindow::displayMedia() {
    // This will only pop up if loading the media blocks the thread for more than 4 seconds (default minimum time for prgDialog bar).
    QProgressDialog prgDialog("Loading media from disk...", "Cancel", 0, 0, this);
    prgDialog.setWindowModality(Qt::WindowModal);
    prgDialog.setMinimumDuration(3);

    QString fileExt = QFileInfo(displayFilePath).suffix().toLower();  // Change all letters lowercase. (eg. JPG to jpg)
    if (fileExt == "png" || fileExt == "jpg" || fileExt == "jpeg" || fileExt == "jfif"){
        player.stop();
        QPixmap displayImg(displayFilePath);
        int width = ui->img->width();
        int height = ui->img->height();
        ui->img->setPixmap(displayImg.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        ui->img->setAlignment(Qt::AlignCenter);
        ui->vid->hide();
        ui->contPlayerPanel->setEnabled(false);
        ui->img->show();
    }
    else if (fileExt == "gif" || fileExt == "mp4" || fileExt == "mkv"){
        ui->img->hide();
        player.setSource(QUrl(displayFilePath));
        ui->btnPlayPause->setIcons(QIcon(":/system/resources/btnPause.png"), QIcon(":/system/resources/btnPauseHover.png"), QIcon(":/system/resources/btnPausePressed.png"));
        ui->vid->show();
        ui->contPlayerPanel->setEnabled(true);
        player.play();
    }
    else if (fileExt == "mp3" || fileExt == "wav"){
        ui->vid->hide();
        player.setSource(QUrl(displayFilePath));
        QPixmap displayImg(":/system/resources/imgMusic.png");
        int width = ui->img->width();
        int height = ui->img->height();
        ui->img->setPixmap(displayImg.scaled(width, height, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        ui->img->setAlignment(Qt::AlignCenter);
        ui->btnPlayPause->setIcons(QIcon(":/system/resources/btnPause.png"), QIcon(":/system/resources/btnPauseHover.png"), QIcon(":/system/resources/btnPausePressed.png"));
        ui->img->show();
        ui->contPlayerPanel->setEnabled(true);
        player.play();
    }

    QString pathFolder = QFileInfo(displayFilePath).path().remove(rootDirPath.path());
    if (pathFolder.isEmpty()) pathFolder = "-";
    else pathFolder.removeFirst();
    ui->lblFilePath->setText("Folder: " + pathFolder);
    ui->lblFilePath->adjustSize();
    ui->lblFilePath->setToolTip(pathFolder);
    ui->lblFileName->setText("Name: " + QFileInfo(displayFilePath).fileName());
    ui->lblFileName->adjustSize();
    ui->lblFileName->setToolTip(QFileInfo(displayFilePath).fileName());
    ui->contPlayerPanel->setCurrentIndex(1);

    prgDialog.reset();
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
    QSettings settings("YxWn", "YxWn_Gallery");
    if (settings.value("Mute in Wallpaper").toBool()) audio->setMuted(true);
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
    SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, (PVOID)prevWallpaperPath.utf16(), SPIF_UPDATEINIFILE | SPIF_SENDWININICHANGE);  // Reset desktop wallpaper to specific img.
    QSettings settings("YxWn", "YxWn_Gallery");
    audio->setMuted(settings.value("Mute").toBool());
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

QDir MainWindow::getSeagateDrivePath() {
    QDir drivePath = QDir(findDriveByDeviceName("SeagateYx4t") + "YuXuanFiles");
    return drivePath;
}

QString MainWindow::formatTime(qint64 ms) {
    int hours = (ms / 3600000);
    int mins = (ms / 60000) % 60;
    int secs = (ms / 1000) % 60;
    char buffer[9];
    std::sprintf(buffer, "%02d:%02d:%02d", hours, mins, secs);  // Produces either 01:01:01 or 101:59:59. (Meaning hour can exceed 100 (very unlikely to happen during usage))
    return QString::fromUtf8(buffer);
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

