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
    void on_checkBox_RunOnce_toggled(bool checked);

    bool QPrcExeSync(QProcess *process, QString *mergedOutput=nullptr, int timeout=1000);
    QString WrapQPrcErrMsg(QProcess *process, const QString &argSep="§");
    void slTmrQueryVsc();
    void slPrcQueryVscFinished(int exitCode, int exitStatus);
    QStringList QStringSplitNewline(const QString &str);
    void DispMsg(const QString &msg, bool err=true);

    void CopyFileAndMTime(const QFileInfo &srcFi, const QFileInfo &destFi);

    void on_toolButton_Setting_clicked();
    void on_toolButton_About_clicked();

private:
    Ui::MainWindow *ui;

    enum VCS {
        vcs_NA,
        vcs_git,
        vcs_svn,
    } vcs;

    QString sPathVcs;
    QString sPathWorking;
    QString sIniPath;

    bool bStop;
    QTimer *tmrQueryVsc;
    QProcess *prcQueryVsc;
    QStringList sQueryArgList;
    QStringList sVcsFileList;

    bool bAppendDeal;
    void AppendDeal(const QString &msg);

    bool bRunOnce;

    int nCurVcsFileIdx;
    int nVcsFileCount;
    void UpdateUiInLoop(int fiIdx, bool copyed);
    QTimer *tmrUuInLoop;
    void UpdateFileCountLabel();
};
#endif // MAINWINDOW_H
