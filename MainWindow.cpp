#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QTimer>
#include <QProcess>
#include "NotPrjRel.h"
#include <QFileInfo>
#include <QDateTime>
#include <QFileDialog>
#include <QFileSystemWatcherEx.h>
#include <QElapsedTimer>
#include <QDebug>

/// Max error of system time synchronization (ms).
/// For the situation that use the vcs and working dir in different os,
/// they have error in the synchronization of 2 os.
int nMaxErrorOfSystime = 500;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowIcon(QIcon("://images/VcsSync.png"));
    ui->toolButton_About->setIcon(style()->standardIcon(QStyle::SP_MessageBoxInformation));
    ui->label_StateMsg->setText(tr("Stopped"));

    tmrQueryVsc = new QTimer(this);
    tmrQueryVsc->setSingleShot(true);
    tmrQueryVsc->setInterval(1000);
    connect(tmrQueryVsc, &QTimer::timeout, this, &MainWindow::slTmrQueryVsc);

    prcQueryVsc = new QProcess(this);

    sIniPath = "setting.ini";
    IniSetting setting(sIniPath);
    sPathVcs = setting.value("PathVcs", "").toString();
    sPathWorking = setting.value("PathWorking", "").toString();
    ui->lineEdit_PathVcs->setText(sPathVcs);
    ui->lineEdit_PathWorking->setText(sPathWorking);

    watcherVcs = new QFileSystemWatcherEx(this);
    connect(watcherVcs, &QFileSystemWatcher::fileChanged, this, &MainWindow::slWatcherVcs_FileChanged);
    connect(watcherVcs, &QFileSystemWatcher::directoryChanged, this, &MainWindow::slWatcherVcs_DirChanged);

    watcherWorking = new QFileSystemWatcherEx(this);
    connect(watcherWorking, &QFileSystemWatcher::fileChanged, this, &MainWindow::slWatcherWorking_FileChanged);
    connect(watcherWorking, &QFileSystemWatcher::directoryChanged, this, &MainWindow::slWatcherWorking_DirChanged);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    on_pushButton_Stop_clicked();
    Q_UNUSED(e)
}

void MainWindow::on_pushButton_PathVcsBrowse_clicked()
{
    QString sPathLast = ui->lineEdit_PathVcs->text();
    QString sDir = QFileDialog::getExistingDirectory(this, tr("Choose a directory"), sPathLast);
    if (!sDir.isEmpty())
        ui->lineEdit_PathVcs->setText(sDir);
}

void MainWindow::on_pushButton_PathWorkingBrowse_clicked()
{
    QString sPathLast = ui->lineEdit_PathWorking->text();
    QString sDir = QFileDialog::getExistingDirectory(this, tr("Choose a directory"), sPathLast);
    if (!sDir.isEmpty())
        ui->lineEdit_PathWorking->setText(sDir);
}

void MainWindow::on_pushButton_Start_clicked()
{
    sPathVcs = ui->lineEdit_PathVcs->text();
    sPathWorking = ui->lineEdit_PathWorking->text();

    prcQueryVsc->setWorkingDirectory(sPathVcs);
    vcs = vcs_NA;
    if (vcs == vcs_NA) {
        prcQueryVsc->setProgram("git");
        prcQueryVsc->setArguments({"rev-parse", "--is-inside-work-tree"});
        if (QPrcExe(prcQueryVsc))
            vcs = vcs_git;
    }
    if (vcs == vcs_NA) {
        prcQueryVsc->setProgram("svn");
        prcQueryVsc->setArguments({"info"});
        if (QPrcExe(prcQueryVsc))
            vcs = vcs_svn;
    }
    if (vcs == vcs_NA) {
        ui->plainTextEdit_MsgOutput->appendPlainText(tr("VCS directory not valid/supported"));
        return;
    }

    IniSetting setting(sIniPath);
    setting.setValue("PathVcs", sPathVcs);
    setting.setValue("PathWorking", sPathWorking);

    prcQueryVsc->setWorkingDirectory(sPathVcs);

    switch (vcs) {
    case vcs_git:
        prcQueryVsc->setProgram("git");
        prcQueryVsc->setArguments({"ls-files"});
        break;
    case vcs_svn:
        prcQueryVsc->setProgram("svn");
        prcQueryVsc->setArguments({"list", "-R"});
        break;
    default:
        break;
    }

    bStop = false;
    ui->label_StateMsg->setText(tr("Running"));
    QCoreApplication::processEvents();
    slTmrQueryVsc();
}

void MainWindow::on_pushButton_Stop_clicked()
{
    bStop = true;
    tmrQueryVsc->stop();
    ui->label_StateMsg->setText(tr("Stopped"));
}

void MainWindow::slTmrQueryVsc()
{
    if (bStop)
        return;

    QString sOutput;
    if (!QPrcExe(prcQueryVsc, &sOutput, -1)) {
        ui->plainTextEdit_MsgOutput->appendPlainText(sOutput);
        return;
    }

    QStringList sVcsFileList = sOutput.split("\n", QString::SkipEmptyParts);
    if (sVcsFileList.size() == 0)
        sVcsFileList = sOutput.split("\r", QString::SkipEmptyParts);

    int size = sVcsFileList.size();
    int nUpdateUiIntv = size / 100;
    if (nUpdateUiIntv == 0)
        nUpdateUiIntv = 10;
    ui->label_FileCount->setText(QString("0/%1").arg(size));

    for (int i=0; i < size; i++) {
        auto &sVcsFile = sVcsFileList[i];
        if (bStop) {
            ui->label_StateMsg->setText(tr("Stopped"));
            return;
        }

        if (sVcsFile.endsWith("\r")/* || sVcsFile.endsWith("\n")*/)
            sVcsFile.chop(1);
        if (sVcsFile.isEmpty())
            continue;

        QFileInfo fiVcs(sPathVcs + "/" + sVcsFile);
        QFileInfo fiWorking(sPathWorking + "/" + sVcsFile);

        // not exists: not checkout or deleted
        if (!fiVcs.exists()) {
            RemoveExistingPath(fiWorking);
            continue;
        }

        // git has no dir; vcs is dir: overwrite working file or dir
        if (vcs != vcs_git && fiVcs.isDir()) {
            if (fiWorking.isFile())
                QFile::remove(fiWorking.filePath());
            if (!fiWorking.exists()) {
                QDir dir;
                dir.mkpath(fiWorking.absolutePath());
            }
            continue;
        }

        int msWorkingMt2VcsMt = fiWorking.lastModified().msecsTo(fiVcs.lastModified());
        if (msWorkingMt2VcsMt > nMaxErrorOfSystime || !fiWorking.exists()) {
            CopyFileIncludeMTime(fiVcs, fiWorking);
        } else if (msWorkingMt2VcsMt < - nMaxErrorOfSystime) {
            CopyFileIncludeMTime(fiWorking, fiVcs);
        }

        if (i == size - 1 || i % nUpdateUiIntv == 0) {
            ui->label_FileCount->setText(QString("%0/%1").arg(i+1).arg(size));
            QCoreApplication::processEvents();
        }
    }

//    tmrQueryVsc->start();
    StartWatchPaths(sVcsFileList);
}

bool MainWindow::QPrcExe(QProcess *process, QString *mergedOutput, int timeout)
{
    process->setProcessChannelMode(QProcess::MergedChannels);
    process->start();
    QElapsedTimer eltm; eltm.start();
    bool ret = process->waitForFinished(timeout);
    qDebug()<< "QPrcExe waitForFinished" <<eltm.elapsed();
#ifdef Q_OS_WIN
    QString cmdOutput = QString::fromLocal8Bit(process->readAll());
#else
    QString cmdOutput = process->readAll();
#endif
    if (mergedOutput) *mergedOutput = cmdOutput;

    if (!ret || process->exitCode() != 0 || process->exitStatus() != QProcess::NormalExit) {
        QString args = process->arguments().join("§");
        QString err = tr("QProcess Exe(Prg: %1 Args: %2) err: %3").arg(process->program(), args, cmdOutput);
        if (mergedOutput) *mergedOutput = err;
        return false;
    }
    return true;
}

void MainWindow::RemoveExistingPath(const QFileInfo &fi)
{
    if (!fi.exists())
        return;

    if (fi.isFile())
        QFile::remove(fi.filePath());  // can not remove dir
    else {
        QDir dir(fi.filePath());
        dir.removeRecursively();  // may take a lot of time
    }
}

void MainWindow::CopyFileIncludeMTime(const QFileInfo &srcFi, const QFileInfo &destFi)
{
    QString srcPath = srcFi.filePath();  //absoluteFilePath
    QString destPath = destFi.filePath();

    RemoveExistingPath(destFi);

    QDir dirDest = destFi.dir();
    if (!dirDest.exists())
        dirDest.mkpath(dirDest.absolutePath());

    bool ret = QFile::copy(srcPath, destPath);
    if (!ret) {
        ui->plainTextEdit_MsgOutput->appendPlainText(tr("Copy %1 to %2 Failed").arg(srcPath, destPath));
        return;
    }

    ret = SetFileMTime(destPath, srcFi.lastModified());
    if (!ret) {
        ui->plainTextEdit_MsgOutput->appendPlainText(tr("Set %1's MTime(%2) Failed").arg(destPath, destFi.lastModified().toString("yyyy-MM-dd hh:mm:ss.zzz")));
    }
}

void MainWindow::StartWatchPaths(QStringList &paths)
{
    QStringList pathsWatch;

    int lastDirIdx = 0;
    for (int i=0; i<paths.size(); i++) {
        const QString &path = paths.at(i);
        if (vcs == vcs_git) {
            int lastSlash = path.lastIndexOf("/");
            if (lastSlash > -1) {
                QString pathDir = path.mid(0, lastSlash);
                if (paths.at(lastDirIdx) != pathDir) {
                    lastDirIdx = i;
                    paths.insert(i, pathDir);
                }
            }
        }
        pathsWatch << sPathVcs + "/" + path;
    }
    watcherVcs->addPaths(pathsWatch);
}

// Qt doc[not exact]{exact}: The fileChanged() signal is emitted when a file has been modified, renamed or removed from disk. Similarly, the directoryChanged() signal is emitted when [a directory or its contents is modified or removed] {its contents is added or removed}. Note that QFileSystemWatcher stops monitoring files once they have been renamed or removed from disk {if withdraw remove/rename, can NOT monitor again}, and directories once they have been removed from disk {if withdraw remove, can monitor again}.

void MainWindow::slWatcherVcs_FileChanged(const QString &path)
{
    // file changed/removed(rename); path is file's path
    QFileInfo fi(path);
    qDebug()<< fi.filePath();
//    CopyFileIncludeMTime(fi, );
}

void MainWindow::slWatcherVcs_DirChanged(const QString &path)
{
    // file or subdir added/deleted in directory; path is directory's path
    QFileInfo fi(path);
    qDebug()<< fi.filePath();
}

void MainWindow::slWatcherWorking_FileChanged(const QString &path)
{

}

void MainWindow::slWatcherWorking_DirChanged(const QString &path)
{

}

#include <SettingDlg.h>
void MainWindow::on_toolButton_Setting_clicked()
{
    SettingDlg settingDlg(this);
    settingDlg.exec();
}

#include <AboutDlg.h>
void MainWindow::on_toolButton_About_clicked()
{
    AboutDlg aboutDlg(this);
    aboutDlg.exec();
}
