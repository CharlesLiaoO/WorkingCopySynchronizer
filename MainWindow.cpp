#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QTimer>
#include <QProcess>
#include "NotPrjRel.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->toolButton_About->setIcon(style()->standardIcon(QStyle::SP_MessageBoxInformation));
    ui->label_StateMsg->setText(tr("已停止"));

    tmrQueryVsc = new QTimer(this);
    prcQueryVsc = new QProcess(this);
    connect(tmrQueryVsc, &QTimer::timeout, this, &MainWindow::slTmrQueryVsc);

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

#include <QFileDialog>
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
    tmrQueryVsc->start(1000);
    ui->label_StateMsg->setText(tr("正在运行"));
}

void MainWindow::on_pushButton_Stop_clicked()
{
    bStop = true;
    ui->label_StateMsg->setText(tr("已停止"));
    if (tmrQueryVsc->isActive()) {
        tmrQueryVsc->stop(); }
}

#include <QFileInfo>
#include <QDateTime>
void MainWindow::slTmrQueryVsc()
{
    QString sOutput;
    if (!QPrcExe(prcQueryVsc, &sOutput)) {
        ui->plainTextEdit_MsgOutput->appendPlainText(sOutput);
        return;
    }

    QStringList sVcsFileList = sOutput.split("\n", QString::SkipEmptyParts);
    if (sVcsFileList.size() == 0)
        sVcsFileList = sOutput.split("\r", QString::SkipEmptyParts);

    for (auto &sVcsFile: sVcsFileList) {
        if (bStop) {
            ui->label_StateMsg->setText(tr("已停止"));
            return;
        }

        sVcsFile = sVcsFile.trimmed();
        if (sVcsFile.isEmpty())
            continue;

        QFileInfo fiVcs(sPathVcs + "/" + sVcsFile);
        if (fiVcs.isDir())
            continue;
        QFileInfo fiWorking(sPathWorking + "/" + sVcsFile);

        QDateTime dtFiVcsLm = fiVcs.lastModified();
        QDateTime dtFiWorkingLm = fiWorking.lastModified();
        if (dtFiVcsLm > dtFiWorkingLm) {
            CopyFileIncludeMTime(fiVcs, fiWorking);
        } else if (dtFiWorkingLm > dtFiVcsLm) {
            CopyFileIncludeMTime(fiWorking, fiVcs);
        }

        QCoreApplication::processEvents();
    }
}

bool MainWindow::QPrcExe(QProcess *process, QString *mergedOutput)
{
    process->setProcessChannelMode(QProcess::MergedChannels);
    process->start();
    bool ret = process->waitForFinished(3000);
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

#include "NotPrjRel.h"
void MainWindow::CopyFileIncludeMTime(const QFileInfo &srcFi, const QFileInfo &destFi)
{
    QString srcPath = srcFi.filePath();  //absoluteFilePath
    QString destPath = destFi.filePath();

    if (QFile::exists(destPath)) {
        QFile::remove(destPath);  //can remove dir??
    }

    QDir dirDest = destFi.dir();
    if (!dirDest.exists())
        dirDest.mkpath(dirDest.absolutePath());

    bool ret = QFile::copy(srcPath, destPath);
    if (!ret) {
        ui->plainTextEdit_MsgOutput->appendPlainText(tr("Copy %1 to %2 Failed").arg(srcPath, destPath));
        return;
    }

    ret = SetMTimeDisk(destPath, srcFi.lastModified());
    if (!ret) {
        ui->plainTextEdit_MsgOutput->appendPlainText(tr("Set %1's MTime(%2) Failed").arg(destPath, destFi.lastModified().toString("yyyy-MM-dd hh:mm:ss.zzz")));
    }
}
