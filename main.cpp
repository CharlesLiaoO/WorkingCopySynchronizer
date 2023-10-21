#include "MainWindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QTextCodec>
#include <QDir>
#include <QDebug>

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);  // linux need but window not

    QApplication a(argc, argv);
    qDebug()<< QDir::currentPath();
    QDir::setCurrent(MyDestDir);
    qDebug()<< QDir::currentPath();

#ifdef Q_OS_UNIX
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
#endif

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "" + QLocale(locale).name();
        if (translator.load(":/translations/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }
    MainWindow w;
    w.show();
    return a.exec();
}
