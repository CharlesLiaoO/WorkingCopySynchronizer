QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

include(Common/IfMsvcAddUtf8.pri)

include(Common/SetInstallPathAndInstall.pri)
# Default rules for deployment.
#qnx: target.path = /tmp/$${TARGET}/bin
#else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

INCLUDEPATH += Common

SOURCES += \
    Common/AboutDlg.cpp \
    Common/NotPrjRel.cpp \
    SettingDlg.cpp \
    main.cpp \
    MainWindow.cpp

HEADERS += \
    Common/AboutDlg.h \
    Common/NotPrjRel.h \
    SettingDlg.h \
    MainWindow.h

FORMS += \
    Common/AboutDlg.ui \
    SettingDlg.ui \
    MainWindow.ui

TRANSLATIONS += \
    Res/translations/zh_CN.ts

#CONFIG += lrelease
#CONFIG += embed_translations

RESOURCES += \
    Res/res.qrc
