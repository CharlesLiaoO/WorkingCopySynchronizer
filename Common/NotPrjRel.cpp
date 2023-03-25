#include "NotPrjRel.h"

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

#include <QDateTime>
#include <QFileInfo>
#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <sys/stat.h>
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
    tspec[1].tv_nsec = dt.toMSecsSinceEpoch() * 1000000;

    // AT_FDCWD not define in some system
    if (0 == utimensat(/*AT_FDCWD*/0, baPath.data(), tspec, 0))
        return true;
    else
        return false;
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
    sync();  // Another sync() call in QSetting's destructor will not do actual work, because there's a IfChanged flag in sync()
#ifdef Q_OS_UNIX
    if (sysFileSync)
        system(QString("sync -d %1").arg(iniFilePath).toStdString().c_str());
#endif
}
