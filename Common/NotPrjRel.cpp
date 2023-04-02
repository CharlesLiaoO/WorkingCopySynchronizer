#include "NotPrjRel.h"

#include <QFileInfo>
#include <QDir>
#include <QDateTime>

QString GetFileNameNS(const QString &path)
{
    QString unixPath = path;
    unixPath.replace('\\', '/');
    QString fileName = unixPath.mid(unixPath.lastIndexOf('/') + 1);
    return GetFilePathNS(fileName);
}

QString GetFilePathNS(const QString &path)
{
    return path.mid(0, path.lastIndexOf('.'));
}

QString GetFileSuffix(const QString &path)
{
    return path.mid(path.lastIndexOf('.') + 1);
}

void RemoveExistingPath(const QFileInfo &fi)
{
    if (!fi.exists())
        return;

    if (fi.isFile()) {
#ifdef Q_OS_WIN
        if (!fi.isWritable())
            QFile::setPermissions(fi.filePath(), QFileDevice::WriteOther | QFileDevice::ReadOther);
#endif
        QFile::remove(fi.filePath());  // can not remove dir
    } else {
        QDir dir(fi.filePath());
        dir.removeRecursively();  // may take a lot of time
    }
}

#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <sys/stat.h>
#include <errno.h>
extern int errno;
#endif
bool SetFileMTime(const QString &filePath, const QDateTime &dtLocal)
{
#ifdef Q_OS_WIN
//    SetFileTime, valid in wince
    QDateTime dtUtc = dtLocal.toUTC();

    HANDLE hFile = CreateFileW(filePath.toStdWString().data(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    FILETIME ftUtc;
    SYSTEMTIME stUtc;
    stUtc.wYear         = dtUtc.date().year();
    stUtc.wMonth        = dtUtc.date().month();
    stUtc.wDay          = dtUtc.date().day();
    stUtc.wHour         = dtUtc.time().hour();
    stUtc.wMinute       = dtUtc.time().minute();
    stUtc.wSecond       = dtUtc.time().second();
    stUtc.wMilliseconds = dtUtc.time().msec();
    if(!SystemTimeToFileTime(&stUtc, &ftUtc))
        return false;

    bool ret = SetFileTime(hFile, NULL, NULL, &ftUtc);
    CloseHandle(hFile);
    return ret;
#else
    const QDateTime &dt = dtLocal;

    QFileInfo fi(filePath);
    QByteArray baPath= fi.absoluteFilePath().toLocal8Bit();

    timespec tspec[2];
    tspec[0].tv_nsec = UTIME_NOW;
    tspec[1].tv_sec = dt.toMSecsSinceEpoch() / 1000;
    tspec[1].tv_nsec = dt.toMSecsSinceEpoch() % 1000 * 1000000;

    // AT_FDCWD not define in some system
    int ret = utimensat(/*AT_FDCWD*/0, baPath.constData(), tspec, 0);
    if (0 == ret)
        return true;
    else {
        qDebug("%s", strerror(errno));
        return false;
    }
#endif
}

IniSetting::IniSetting(const QString &iniFilePath, const QString &groupName, QObject *parent, bool sysFileSync) :
    QSettings(iniFilePath, QSettings::IniFormat, parent)
{
    this->iniFilePath = iniFilePath;
    this->sysFileSync = sysFileSync;
    setIniCodec("UTF-8");
    if (!groupName.isEmpty())
        beginGroup(groupName);
}

IniSetting::~IniSetting()
{
    if (!QFileInfo::exists(iniFilePath))
        return;  // avoid warning msg
    sync();  // Another sync() call in QSetting's destructor will not do actual work, because there's a IfChanged flag in sync()
#ifdef Q_OS_UNIX
    if (sysFileSync)
        system(QString("sync %1").arg(iniFilePath).toStdString().c_str());
#endif
}
