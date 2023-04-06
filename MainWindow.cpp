#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QStyle>
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
    setWindowIcon(QIcon("://images/WorkingCopySynchronizer.png"));
    ui->toolButton_About->setIcon(style()->standardIcon(QStyle::SP_MessageBoxInformation));
    ui->label_StateMsg->setText(tr("Stopped"));

    tmrQueryVsc = new QTimer(this);
    tmrQueryVsc->setSingleShot(true);
    tmrQueryVsc->setInterval(2000);
    connect(tmrQueryVsc, &QTimer::timeout, this, &MainWindow::slTmrQueryVsc);

    tmrUuInLoop = new QTimer(this);
    tmrUuInLoop->setInterval(500);
    connect(tmrUuInLoop, &QTimer::timeout, this, [&](){
        UpdateFileCountLabel();
//        qDebug()<< QTime::currentTime()<< "timeout";
    });

    prcQueryVsc = new QProcess(this);
    connect(prcQueryVsc, (void(QProcess::*)(int, QProcess::ExitStatus))&QProcess::finished, this, &MainWindow::slPrcQueryVscFinished);

    sIniPath = "setting.ini";
    IniSetting setting(sIniPath);
    sPathMaster = setting.value("PathMaster", "").toString();
    sPathBranch = setting.value("PathBranch", "").toString();
    ui->lineEdit_PathMaster->setText(sPathMaster);
    ui->lineEdit_PathBranch->setText(sPathBranch);

    bRunOnce = setting.value("RunOnce", false).toBool();
    ui->checkBox_RunOnce->blockSignals(true);
    ui->checkBox_RunOnce->setChecked(bRunOnce);
    ui->checkBox_RunOnce->blockSignals(false);
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

void MainWindow::on_pushButton_PathMasterBrowse_clicked()
{
    QString sPathLast = ui->lineEdit_PathMaster->text();
    QString sDir = QFileDialog::getExistingDirectory(this, tr("Choose the Master directory"), sPathLast);
    if (!sDir.isEmpty())
        ui->lineEdit_PathMaster->setText(sDir);
}

void MainWindow::on_pushButton_PathBranchBrowse_clicked()
{
    QString sPathLast = ui->lineEdit_PathBranch->text();
    QString sDir = QFileDialog::getExistingDirectory(this, tr("Choose the Branch directory"), sPathLast);
    if (!sDir.isEmpty())
        ui->lineEdit_PathBranch->setText(sDir);
}

void MainWindow::on_pushButton_Start_clicked()
{
    ui->pushButton_Start->setEnabled(false);
    ui->checkBox_RunOnce->setEnabled(false);

    sPathMaster = ui->lineEdit_PathMaster->text();
    sPathBranch = ui->lineEdit_PathBranch->text();

    QProcess prcIdnetifyVsc;
    prcIdnetifyVsc.setWorkingDirectory(sPathMaster);
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
    setting.setValue("PathMaster", sPathMaster);
    setting.setValue("PathBranch", sPathBranch);

    prcQueryVsc->setWorkingDirectory(sPathMaster);

    switch (vcs) {
    case vcs_git:
        prcQueryVsc->setProgram("git");
        sQueryArgList = QStringList{"ls-files"};
        sQueryArgList = QStringList{"ls-tree", "-r", "--name-only", "HEAD"};  // without cached changes
        break;
    case vcs_svn:
        prcQueryVsc->setProgram("svn");
        sQueryArgList = QStringList{"list", "-R"};
        break;
    default:
        break;
    }
    prcQueryVsc->setArguments(sQueryArgList);

    bStop = false;
    ui->label_StateMsg->setText(tr("Running"));
    slTmrQueryVsc();
}

void MainWindow::on_pushButton_Stop_clicked()
{
    bStop = true;
    tmrUuInLoop->stop();
    tmrQueryVsc->stop();
    if (prcQueryVsc->state() != QProcess::NotRunning)
        prcQueryVsc->kill();
    ui->label_StateMsg->setText(tr("Stopped"));
    ui->pushButton_Start->setEnabled(true);
    ui->checkBox_RunOnce->setEnabled(true);
}

void MainWindow::on_checkBox_RunOnce_toggled(bool checked)
{
    bRunOnce = checked;
    IniSetting setting(sIniPath);
    setting.setValue("RunOnce", bRunOnce);
}

void MainWindow::slTmrQueryVsc()
{
    if (bStop)
        return;

    ui->label_FileCount->setText("0/0");

    bAppendDeal = false;
    prcQueryVsc->start();
}

bool MainWindow::QPrcExeSync(QProcess *process, QString *mergedOutput, int timeout)
{
    process->setProcessChannelMode(QProcess::MergedChannels);
    process->start();
//    QElapsedTimer eltm; eltm.start();
    bool ret = process->waitForFinished(timeout);
//    qDebug()<< __FUNCTION__ <<eltm.elapsed();

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

    if (!bAppendDeal) {
        sVcsFileList = QStringSplitNewline(QString::fromLocal8Bit(process->readAllStandardOutput()));
        if (vcs == vcs_svn) {
            bAppendDeal = true;
            process->setArguments({"st", "-q"});
            process->start();
            return;
        }
    } else {
        AppendDeal(QString::fromLocal8Bit(process->readAllStandardOutput()));
        prcQueryVsc->setArguments(sQueryArgList);
        bAppendDeal = false;
    }

    nVcsFileCount = sVcsFileList.size();
    ui->label_FileCount->setText(QString("0/%1").arg(nVcsFileCount));
    QCoreApplication::processEvents();
    tmrUuInLoop->start();

    bool copyedLastTime = false;
    int &i = nCurVcsFileIdx;
    for (i=0; i < nVcsFileCount; i++) {
        auto &sVcsFile = sVcsFileList[i];
        if (bStop)
            return;

        QFileInfo fiMaster(sPathMaster + "/" + sVcsFile);
        QFileInfo fiBranch(sPathBranch + "/" + sVcsFile);

        UpdateUiInLoop(i, copyedLastTime);

        // not exists: not checkout or deleted
        if (!fiMaster.exists()) {
            RemoveExistingPath(fiBranch);
            continue;
        }

        // git has no dir; dir: overwrite working file or dir
        if (vcs != vcs_git && fiMaster.isDir()) {
            if (fiBranch.isFile())
                QFile::remove(fiBranch.filePath());
            if (!fiBranch.exists()) {
                QDir dir;
                dir.mkpath(fiBranch.absolutePath());
            }
            continue;
        }

        int msBranchMt2MasterMt = fiBranch.lastModified().msecsTo(fiMaster.lastModified());
        if (msBranchMt2MasterMt > nMaxErrorOfSystime || !fiBranch.exists()) {
            CopyFileAndMTime(fiMaster, fiBranch);
            copyedLastTime = true;
        } else if (msBranchMt2MasterMt < - nMaxErrorOfSystime) {
            CopyFileAndMTime(fiBranch, fiMaster);
            copyedLastTime = true;
        } else
            copyedLastTime = false;
    }

    tmrUuInLoop->stop();

    if (bRunOnce) {
        on_pushButton_Stop_clicked();  //set stopped state
    } else
        tmrQueryVsc->start();
}

void MainWindow::AppendDeal(const QString &msg)
{
    QStringList slTmp = QStringSplitNewline(msg);
    for (auto &sFileChange: slTmp) {
        if (sFileChange.startsWith("A")) {
            sVcsFileList.append(sFileChange.mid(8));
        }
        // svn list already excluded the deleted files
//        else if (sFileChange.startsWith("D") || sFileChange.startsWith("!")) {
//            sVcsFileList.removeOne(sFileChange.mid(8));
//        }
    }
}

void MainWindow::UpdateUiInLoop(int fiIdx, bool copyed)
{
    // intv value based on computer performance
    static int nIntvToSleep = 99;
    static int nIntvToPrcEvts = 999;

    if (fiIdx % nIntvToSleep == 0)
        QThread::msleep(1);  // reduce cpu usage

    // QCoreApplication::processEvents(): for user interact or tmrUu
    if (copyed) {
        QCoreApplication::processEvents();  // when copyed, processEvents immediately
//        qDebug()<< QTime::currentTime()<< "copyed";
    } else if (fiIdx % nIntvToPrcEvts == 0)
        QCoreApplication::processEvents();

    if (fiIdx == nVcsFileCount - 1) {
        UpdateFileCountLabel();
//        qDebug()<< QTime::currentTime()<< "last one ---------";
    }
}

void MainWindow::UpdateFileCountLabel()
{
    ui->label_FileCount->setText(QString("%1/%2").arg(nCurVcsFileIdx + 1).arg(nVcsFileCount));
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
    QString sepChar;
    if (vcs == vcs_git)
        sepChar = "\n";
    else
        sepChar = "\r\n";
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
        bStop = true;
        ui->label_StateMsg->setText(tr("Stopped"));
        ui->pushButton_Start->setEnabled(true);
        ui->checkBox_RunOnce->setEnabled(true);
    }
}

void MainWindow::CopyFileAndMTime(const QFileInfo &srcFi, const QFileInfo &destFi)
{
    QString srcPath = srcFi.filePath();  //absoluteFilePath
    QString destPath = destFi.filePath();

    RemoveExistingPath(destFi);

    QDir dirDest = destFi.dir();
    if (!dirDest.exists())
        dirDest.mkpath(dirDest.absolutePath());

    bool ret = QFile::copy(srcPath, destPath);
    if (!ret) {
        DispMsg(tr("Copy %1 to %2 failed").arg(srcPath, destPath));
        return;
    }

    ret = SetFileMTime(destPath, srcFi.lastModified());
    if (!ret) {
        DispMsg(tr("Set %1's modified time(%2) failed").arg(destPath, destFi.lastModified().toString("yyyy-MM-dd hh:mm:ss.zzz")));
    }
}

#include <SettingDlg.h>
#include <QMessageBox>
void MainWindow::on_toolButton_Setting_clicked()
{
    QMessageBox::information(this, "", "Nothing yet~");
    return;

    SettingDlg settingDlg(this);
    settingDlg.exec();
}

#include <AboutDlg.h>
void MainWindow::on_toolButton_About_clicked()
{
    AboutDlg aboutDlg(this);
    aboutDlg.exec();
}
