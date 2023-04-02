#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QTimer>
#include <QProcess>
#include "NotPrjRel.h"
#include <QFileInfo>
#include <QDateTime>
#include <QFileDialog>
#include <QElapsedTimer>
#include <QThread>
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
    tmrQueryVsc->setInterval(2000);
    connect(tmrQueryVsc, &QTimer::timeout, this, &MainWindow::slTmrQueryVsc);

    prcQueryVsc = new QProcess(this);
    connect(prcQueryVsc, (void(QProcess::*)(int, QProcess::ExitStatus))&QProcess::finished, this, &MainWindow::slPrcQueryVscFinished);

    sIniPath = "setting.ini";
    IniSetting setting(sIniPath);
    sPathVcs = setting.value("PathVcs", "").toString();
    sPathWorking = setting.value("PathWorking", "").toString();
    ui->lineEdit_PathVcs->setText(sPathVcs);
    ui->lineEdit_PathWorking->setText(sPathWorking);
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
    ui->pushButton_Start->setEnabled(false);

    sPathVcs = ui->lineEdit_PathVcs->text();
    sPathWorking = ui->lineEdit_PathWorking->text();

    QProcess prcIdnetifyVsc;
    prcIdnetifyVsc.setWorkingDirectory(sPathVcs);
    vcs = vcs_NA;
    if (vcs == vcs_NA) {
        prcIdnetifyVsc.setProgram("git");
        prcIdnetifyVsc.setArguments({"rev-parse", "--is-inside-work-tree"});
        if (QPrcExeSync(&prcIdnetifyVsc))
            vcs = vcs_git;
    }
    if (vcs == vcs_NA) {
        prcIdnetifyVsc.setProgram("svn");
        prcIdnetifyVsc.setArguments({"info"});
        if (QPrcExeSync(&prcIdnetifyVsc))
            vcs = vcs_svn;
    }
    if (vcs == vcs_NA) {
        DispMsg(tr("VCS directory not valid/supported"));
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
    ui->label_FileCount->setText("0/0");
    ui->label_StateMsg->setText(tr("Running"));
    slTmrQueryVsc();
}

void MainWindow::on_pushButton_Stop_clicked()
{
    bStop = true;
    tmrQueryVsc->stop();
    if (prcQueryVsc->state() != QProcess::NotRunning)
        prcQueryVsc->kill();
    ui->label_StateMsg->setText(tr("Stopped"));
    ui->pushButton_Start->setEnabled(true);
}

void MainWindow::slTmrQueryVsc()
{
    if (bStop)
        return;

    prcQueryVsc->start();
}

bool MainWindow::QPrcExeSync(QProcess *process, QString *mergedOutput, int timeout)
{
    process->setProcessChannelMode(QProcess::MergedChannels);
    process->start();
    //    QElapsedTimer eltm; eltm.start();
    bool ret = process->waitForFinished(timeout);
    //    qDebug()<< "QPrcExe waitForFinished" <<eltm.elapsed();

    if (!ret || process->exitCode() != 0 || process->exitStatus() != QProcess::NormalExit) {
        if (mergedOutput)
            *mergedOutput = WrapQPrcErrMsg(process);
        return false;
    } else {
        if (mergedOutput)
            *mergedOutput = QString::fromLocal8Bit(process->readAllStandardOutput());
        return true;
    }
}

void MainWindow::slPrcQueryVscFinished(int exitCode, int exitStatus)
{
    QProcess *process = (QProcess *)sender();
    if (exitCode != 0 || exitStatus != QProcess::NormalExit) {
        DispMsg(WrapQPrcErrMsg(process));
        return;
    }

    QStringList sVcsFileList = QStringSplitNewline(QString::fromLocal8Bit(process->readAllStandardOutput()));

    int size = sVcsFileList.size();
    int nUpdateUiIntv = size / 100;
    if (nUpdateUiIntv == 0)
        nUpdateUiIntv = 10;
    ui->label_FileCount->setText(QString("0/%1").arg(size));

    for (int i=0; i < size; i++) {
        auto &sVcsFile = sVcsFileList[i];
        if (bStop) {
            return;
        }

        QFileInfo fiVcs(sPathVcs + "/" + sVcsFile);
        QFileInfo fiWorking(sPathWorking + "/" + sVcsFile);

        // not exists: not checkout or deleted
        if (!fiVcs.exists()) {
            RemoveExistingPath(fiWorking);
            continue;
        }

        // git has no dir; dir: overwrite working file or dir
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
            QThread::msleep(1);  // reduce cpu usage, takes <=100ms for whole 'for' loop
        }
    }

    tmrQueryVsc->start();
}

QString MainWindow::WrapQPrcErrMsg(QProcess *process, const QString &argSep)
{
    QString err = tr("QProcess %1(%2) exe err: %3").arg(process->program(),
                                                        process->arguments().join(argSep),
                                                        QString::fromLocal8Bit(process->readAllStandardError()));
    return err;
}

QStringList MainWindow::QStringSplitNewline(const QString &str)
{
#if defined(Q_OS_WIN)
    QString sepChar = "\r\n";
#elif defined(Q_OS_UNIX)
    QString sepChar = "\n";
#else
    QString sepChar = "\r";
#endif
    QStringList ret = str.split(sepChar, QString::SkipEmptyParts);
    return ret;
}

void MainWindow::DispMsg(const QString &msg, bool err)
{
    ui->plainTextEdit_MsgOutput->appendPlainText(msg);
    if (err) {
        ui->label_StateMsg->setText(tr("Stopped"));
        ui->pushButton_Start->setEnabled(true);
    }
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
        DispMsg(tr("Copy %1 to %2 Failed").arg(srcPath, destPath));
        return;
    }

    ret = SetFileMTime(destPath, srcFi.lastModified());
    if (!ret) {
        DispMsg(tr("Set %1's MTime(%2) Failed").arg(destPath, destFi.lastModified().toString("yyyy-MM-dd hh:mm:ss.zzz")));
    }
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
