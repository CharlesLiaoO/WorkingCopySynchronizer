#include "QFileSystemWatcherEx.h"

#include <QDir>
#include <QDebug>

bool QFileSystemWatcherEx::SetDir(const QString &dirPath, int checkIntv)
{
    StopMon();  //清除之前的

    QDir dir(dirPath);
    if (!dir.exists()) {  //dir.exists(":")->true
        this->dirPath = dirPath;  //保存不存在的路径
        return false;
    }

    this->dirPath = dir.path();  //normalize
    bDirExists = true;

    // qt5.9.9 Win: 目录被删除时，QFileSystemWatcher不会发射directoryChanged...
    tmCheckDirExists.setInterval(checkIntv);
    connect(&tmCheckDirExists, &QTimer::timeout, this, &QFileSystemWatcherEx::CheckDirExists);
    tmCheckDirExists.start();

    if (bCheckDirExistOnly)
        return true;

    // use QFileSystemWatcher to monitor content
    tmSignalMerge.setInterval(checkIntv);
    connect(this, &QFileSystemWatcher::directoryChanged, this, &QFileSystemWatcherEx::qfswDirectoryChanged);
    connect(this, &QFileSystemWatcher::fileChanged, this, &QFileSystemWatcherEx::qfswFileChanged);
    connect(&tmSignalMerge, &QTimer::timeout, this, &QFileSystemWatcherEx::slSignalMergeTimerout);

    return AddDirPaths(dir);
}

bool QFileSystemWatcherEx::AddDirPaths(const QDir &dir)
{
    QStringList existedPaths = files() + directories();  //addPaths重复添加会错误返回已存在的，这里避免

    // addPath(s)每次调用都打印强制引擎提示，所以这里一次调用
    QStringList paths;
    if (!existedPaths.contains(dirPath))
        paths<< dirPath;
    // 监控目录时，不能监控目录下文件内容的修改，所以增加目录下文件的监控
    QFileInfoList fiList = dir.entryInfoList(QDir::Files);
    foreach (auto &fi, fiList) {
        if (!existedPaths.contains(fi.filePath()))
            paths<< fi.filePath();
    }

    if (!paths.isEmpty())  //paths空时有打印
        paths = addPaths(paths);  //return not monitored paths，
    if (!paths.isEmpty()) {
        ClearContentMon();
        return false;
    }
    return true;
}

void QFileSystemWatcherEx::ClearContentMon()
{
    if (!files().isEmpty())  //avoid QFileSystemWatcher warning
        removePaths(files());
    if (!directories().isEmpty())
        removePaths(directories());
}

void QFileSystemWatcherEx::StopMon()
{
    ClearContentMon();
    tmSignalMerge.stop();
    tmCheckDirExists.stop();
}

void QFileSystemWatcherEx::qfswDirectoryChanged(const QString &path)
{
    // 内容修改为目录。如目录下增删子项时，则此时的path为增删的子项，不是目录
    ContentChanged(path);

    // 重新添加所有监控路径
    bNeedAddFilesAgain = true;  //延时添加
}

void QFileSystemWatcherEx::qfswFileChanged(const QString &path)
{
    // 文件子项内容修改或增删
    ContentChanged(path);
}

void QFileSystemWatcherEx::ContentChanged(const QString &path)
{
    Q_UNUSED(path)
    if (!tmSignalMerge.isActive())
        tmSignalMerge.start();
    //    qDebug()<< __FUNCTION__<< path;
}

void QFileSystemWatcherEx::slSignalMergeTimerout()
{
    tmSignalMerge.stop();

    if (bNeedAddFilesAgain) {
        bNeedAddFilesAgain = false;
        //        qDebug()<< files();
        //        ClearContentMon();  //已监控文件被删除时，QFSW会自动删除文件监控
        if (!AddDirPaths(QDir(dirPath)))
            qWarning()<< QString("QFileSystemWatcherEx: re AddDirPaths(%1) failed").arg(dirPath);
    }
    emit contentChanged(dirPath);
}

void QFileSystemWatcherEx::CheckDirExists()
{
    if (QFileInfo::exists(dirPath) != bDirExists) {
        StopMon();  //删除/恢复目录开始后的tmSignalMerge周期内，不发射内容修改信号
        bDirExists = !bDirExists;  //由于dirAdded可能被外部用于调用SetDir()，进而bDirExists=true，导致了死循环，所以标识取反需要在信号发射前

        if (bDirExists)
            emit dirAdded(dirPath);
        else
            emit dirRemoved(dirPath);
    }
}
