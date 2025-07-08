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
    void btnGenerate_clicked();
    void filterFiles();
    void retrieveFiles();
    void btnSelectFolder_clicked();
    void btnPlayPause_clicked();
    void btnRewind_clicked();
    void btnSkip_clicked();
    void playerPositionChanged(qint64 position);
    void sliderPressed();
    void sliderMoved(int value);
    void sliderReleased();
    void chkEchoesThisDay_clicked(Qt::CheckState state);
    void chkAutoplay_clicked(Qt::CheckState state);
    void btnSettings_clicked();
    void tray_clicked(QSystemTrayIcon::ActivationReason reason);
    void btnRefresh_clicked();
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
    Ui::MainWindow *ui;
    QDir dirImages;
    QStringList pathList;
    QString pathRand;
    QString previousWallpaperPath;
    HWND getDesktopWorkerW();
    void attachAppAsWallpaper();
    void restoreAppAsWindow();
    bool forceExit = false;
};
#endif // MAINWINDOW_H
