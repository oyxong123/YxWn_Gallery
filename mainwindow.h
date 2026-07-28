#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDir>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QTimer>
#include <QSystemTrayIcon>
#include <QMenu>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    QTimer autoplay;
    QMediaPlayer player;  // This set of player + audio instances needs to be declared on top of the file to ensure it doesn't get destroyed when the runtime reaches the end of a function.
    QAudioOutput *audio = new QAudioOutput;
    QSystemTrayIcon tray;
    QMenu trayMenu;
    QAction *wallpaperModeAction = new QAction;
    QAction *windowModeAction = new QAction;
    QAction *exitAction = new QAction;

private slots:
    void generateMedia();
    void btnGenerate_clicked();
    void btnSelectFolder_clicked();
    void btnRefresh_clicked();
    void btnSettings_clicked();
    void btnPlayPause_clicked();
    void btnRewind_clicked();
    void btnSkip_clicked();
    void player_positionChanged(qint64 position);
    void slrProgressBar_pressed();
    void slrProgressBar_moved(int value);
    void slrProgressBar_released();
    void chkAutoplay_clicked(Qt::CheckState state);
    void chkEchoesThisDay_clicked(Qt::CheckState state);
    void chkWinnie_clicked(Qt::CheckState state);
    void chkYxHdd_clicked(Qt::CheckState state);
    void chkYxLaptop_clicked(Qt::CheckState state);
    void spnEchoesThisDay_valueChanged();
    void tray_clicked(QSystemTrayIcon::ActivationReason reason);
    void trayExitAction_clicked(bool checked);
    void trayWindowModeAction_clicked(bool checked);

public slots:
    void trayWallpaperModeAction_clicked(bool checked);

signals:

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    QString displayFilePath;
    QStringList completeFilePathList;
    QStringList filteredFilePathList; // new
    QStringList prevCompleteFilePathList;
    QDir rootDirPath;
    QDir prevRootDirPath;
    void retrieveSelectFolder();
    void retrieveWinnie();
    void retrieveYxHdd();
    void retrieveYxLaptop();
    void retrievePreset(QStringList partialDirList); // new
    void retrieveFiles(QStringList partialDirList);
    void filterFiles();
    QStringList filterFiles_userFileExt(QStringList filePathList); // new
    QStringList filterFiles_year(QStringList filePathList); // new
    QStringList fileExtFilters_full();
    QStringList fileExtFilters_user();

    Ui::MainWindow *ui;
    bool forceExit = false;
    QString prevWallpaperPath;
    void displayMedia();
    void attachAppAsWallpaper();
    void restoreAppAsWindow();
    HWND getDesktopWorkerW();
    QDir getSeagateDrivePath();
    QString formatTime(qint64 ms);
    QString findDriveByDeviceName(const QString &deviceName);
};
#endif // MAINWINDOW_H
