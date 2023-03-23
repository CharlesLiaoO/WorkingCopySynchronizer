# Set install path to a 'bin' dir and install after build.
# 设置安装目录到目录'bin'且在构建后安装。
target.path += $$PWD/../bin
include(PostLinkMakeInstall.pri)

# Additionally, in QtCreator's project config view, add custom run config for every build kit, set the exe as 'bin's exe and the working dir as 'bin'.
# This is my way to make the working dir unified but the build output dir independent, between different build configs.
# If you don't need, don't include this pri and no need to add custom run config.
# 另外的，在QtCreator的项目配置视图中，为每个构建套件添加自定义运行配置，设置可执行文件为'bin'下的可执行文件，工作目录为'bin'。
# 这是我在不同构建配置中，使工作目录统一到bin，但构建输出目录独立的方法。
# 如果你不需要，不要包含此pri，也不需要添加自定义运行配置。
