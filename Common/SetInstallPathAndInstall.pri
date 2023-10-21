# 保持Qt默认的“不同构建套件不同输出目录”的同时，便捷地构建一个统一的工作目录
# 实现原理：设置安装目录到自己的工作目录且在构建后安装。在qt app创建后，切换到统一的工作目录

# 使用：
# 1、在pro文件的 INSTALLS += target 之前include()此文件。
# 2、在main.cpp中，qt app对象构建后，添加设置工作目录的代码：QDir::setCurrent(MyDestDir);

MyDestDir = $$PWD/../bin
target.path += $$MyDestDir
DEFINES += MyDestDir=\"\\\"$$MyDestDir\"\\\"
message(-- MyDestDir=$$MyDestDir)

include(PostLinkMakeInstall.pri)
