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
#ifdef Q_OS_WIN
#include <windows.h>
#else
#include <utime.h>
#endif
bool SetMTimeDisk(const QString &filePath, const QDateTime &dtLocal)
{
#ifdef Q_OS_WIN
    // _utime("cl/Logs.txt", NULL);  //NULL 当前时间。返回-1 失败。wince没有此接口

    // SetFileTime wince有
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

//    i=SetFileTime(hFile,&ftUTC,(LPFILETIME) NULL, (LPFILETIME) NULL);  //创建时间
//    i=SetFileTime(hFile,(LPFILETIME) NULL,&ftUTC2, (LPFILETIME) NULL);  //访问时间
    bool ret = SetFileTime(hFile, NULL, NULL, &ftUtc);  //修改时间
    CloseHandle(hFile);
    return ret;
#else
    const QDateTime &dtUtc = dtLocal;
    struct tm tmModify;
    tmModify.tm_year  = dtUtc.date().year() - 1900;
    tmModify.tm_mon   = dtUtc.date().month() - 1;
    tmModify.tm_mday  = dtUtc.date().day();
    tmModify.tm_hour  = dtUtc.time().hour();
    tmModify.tm_min   = dtUtc.time().minute();
    tmModify.tm_sec   = dtUtc.time().second();

    struct utimbuf ut;  //二者需要同时修改，否则修改不成功
    ut.modtime = mktime(&tmModify);  // 最后修改时间
    time(&ut.actime);  //最后访问时间(当前时间)

    QByteArray ary= filePath.toLocal8Bit();
    char *cFilePath = ary.data();

    if(0 == utime(cFilePath, &ut))
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
