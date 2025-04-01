#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDir>
#include <QMediaPlayer>
#include <QAudioOutput>

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

private slots:
    void btnGenerate_clicked();
    void btnSelectFolder_clicked();
    void btnPlayPause_clicked();
    void btnRewind_clicked();
    void btnSkip_clicked();
    void playerPositionChanged(qint64 position);
    void sliderPressed();
    void sliderMoved(int value);
    void sliderReleased();

signals:
    // Any custom signals

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:  // Global variables
    Ui::MainWindow *ui;
    QAudioOutput *audio = new QAudioOutput;
    QMediaPlayer player;  // This set of player + audio instances needs to be declared on top of the file to ensure it doesn't get destroyed when the runtime reaches the end of a function.
    QDir dirImages;
    QStringList pathList;
    QString pathRand;
};
#endif // MAINWINDOW_H
