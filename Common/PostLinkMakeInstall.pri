msvc: QMAKE_POST_LINK += nmake install
else: win32-g++: QMAKE_POST_LINK += mingw32-make install
else: !win32: QMAKE_POST_LINK += make install
#message(QMAKE_POST_LINK $$QMAKE_POST_LINK)
