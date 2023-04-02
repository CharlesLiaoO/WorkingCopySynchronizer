#ifndef NOTPRJREL_H
#define NOTPRJREL_H

#include <QLocale>
#include <QSettings>

// Not Project Related APIs
// 与项目无关的一些API


/// Convert a number to string, keep 3 decimal places
/// 将数值转成字串（保留3位小数）
inline QString StrF3(double val) {
    return QString::number(val, 'f', 3);
}

/// Convert a number to string, keep least decimal places
/// 将数值转成字串（保留最少位小数）
inline QString StrF(double val) {
#if QT_VERSION >= 0x050700
    return QString::number(val, 'f', QLocale::FloatingPointShortest);
#else
    // QLocale::FloatingPointShortest的qt实现复杂，不参考
    // When QString::number()'s arg prec > 6, result may not accurate. Athough there's some way to avoid the not accurate result, this function should not be used in scenes with too many decimal places that are difficult to read
    // 当QString::number()的参数prec > 6时，结果可能不精确。虽然有办法避免不精确的结果，但这个函数不应该用在人难读的太多位小数的场景中
    QString tmp = QString::number(val, 'f'/*, 16*/);
    if (tmp.contains('.')) {
        while (tmp.endsWith('0'))
            tmp.chop(1);
        if (tmp.endsWith('.'))
            tmp.chop(1);
    }
    return tmp;
#endif
}

/// Get file name with No Suffix
/// 获取文件无后缀（No Suffix）的名字
QString GetFileNameNS(const QString &path);
/// Get file path with No Suffix
/// 获取文件无后缀（No Suffix）的路径
QString GetFilePathNS(const QString &path);
/// Get file suffix
/// 获取文件的后缀
QString GetFileSuffix(const QString &path);

class QFileInfo;
void RemoveExistingPath(const QFileInfo &fi);

/// Set file's modify time by local time.
/// 设置文件的修改时间（使用当地时间）
bool SetFileMTime(const QString &filePath, const QDateTime &dtLocal);

/// A convenient QSettings of ini fomart. Support group arg in constructor, use utf8 econding
/// 便利的Ini格式的QSettings。在构造函数中支持组名，使用utf8编码
class IniSetting : public QSettings
{
public:
    /// \a sysFileSync: whether to call the system call sync() in destructor on unix system
    IniSetting(const QString &iniFilePath, const QString &groupName="", QObject *parent=nullptr, bool sysFileSync=true);
    ~IniSetting();
private:
    QString iniFilePath;
    bool sysFileSync;
};

#endif // NOTPRJREL_H
