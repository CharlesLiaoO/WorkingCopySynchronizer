#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QProcess;
class QFileInfo;
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void closeEvent(QCloseEvent *e);

private slots:
    void on_pushButton_PathVcsBrowse_clicked();
    void on_pushButton_PathWorkingBrowse_clicked();
    void on_pushButton_Start_clicked();
    void on_pushButton_Stop_clicked();

    void slTmrQueryVsc();
    bool QPrcExe(QProcess *process, QString *mergedOutput=nullptr, int timeout=1000);
    void RemoveExistingPath(const QFileInfo &fi);
    void CopyFileIncludeMTime(const QFileInfo &srcFi, const QFileInfo &destFi);

    void on_toolButton_Setting_clicked();
    void on_toolButton_About_clicked();

private:
    Ui::MainWindow *ui;

    QString sPathVcs;
    QString sPathWorking;
    QString sIniPath;

    bool bStop;
    QTimer *tmrQueryVsc;
    QProcess *prcQueryVsc;

    enum VCS {
        vcs_NA,
        vcs_git,
        vcs_svn,
    } vcs;
};
#endif // MAINWINDOW_H
