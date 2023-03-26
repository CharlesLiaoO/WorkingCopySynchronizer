#ifndef QFILESYSTEMWATCHEREX_H
#define QFILESYSTEMWATCHEREX_H

#include <QFileSystemWatcher>

#include <QTimer>
#include <QDir>

/// 监控目录以及目录下文件的修改，提供合并的信号、目录删除信号
/// \details 有些软件写文件时，如Notepad++，会先删了文件或把文件清空保存一次，
/// 然后写完内容后再保存一次，导致QFileSystemWatcher触发2次内容和目录修改的信号，
/// 此类通过延时检测合并多次信号为一次信号。
/// 另外，目录被删除时，QFileSystemWatcher可能不发射信号，此类增加定时器检测目录的增删。
/// Win：如果目录是网络文件，当网络硬件有联通，但文件不可达时，
/// Qt文件Api将造成长时间卡顿，Win的文件浏览器也存在这个问题。
class QFileSystemWatcherEx : public QFileSystemWatcher
{
    Q_OBJECT
public:
    using QFileSystemWatcher::QFileSystemWatcher;

    /// 设置为只检查目录存在与否，适合与QFileSystemModel配合使用。开始监控前调用
    void SetCheckDirExistOnly() { bCheckDirExistOnly = true; }
    /// 设置轮询引擎而非默认的通知式引擎，qt未公开实现方式，默认引擎无效时使用。开始监控前调用
    void SetPollerEngine() { setObjectName("_qt_autotest_force_engine_poller"); }
    /// 设置监控目录并开始监控。参数checkIntv为信号合并、检测目录的间隔时间，ms单位，默认100ms
    bool SetDir(const QString &dirPath, int checkIntv=100);
    /// 返回目录
    QString Dir() { return dirPath; }
    /// 停止监控。
    void StopMon();
    /// 恢复监控。
    void RestartMon() { SetDir(dirPath); }

Q_SIGNALS:
    /// 目录或其下项已修改
    void contentChanged(const QString &dirPath);
    /// 目录被删除
    void dirRemoved(const QString &dirPath);
    /// 目录被增加
    void dirAdded(const QString &dirPath);

private:
    QString dirPath;
    void ClearContentMon();
    bool AddDirPaths(const QDir &dir);

    QTimer tmSignalMerge;
    void qfswDirectoryChanged(const QString &path);
    void qfswFileChanged(const QString &path);
    void ContentChanged(const QString &path);
    void slSignalMergeTimerout();

    bool bNeedAddFilesAgain = false;

    QTimer tmCheckDirExists;
    bool bCheckDirExistOnly = false;
    bool bDirExists = false;
    void CheckDirExists();
};

#endif // QFILESYSTEMWATCHEREX_H
