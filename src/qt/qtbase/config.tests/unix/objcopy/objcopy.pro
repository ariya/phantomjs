SOURCES = objcopy.cpp
CONFIG -= qt

QMAKE_POST_LINK += $$QMAKE_OBJCOPY --only-keep-debug objcopy objcopy.debug && $$QMAKE_OBJCOPY --strip-debug objcopy && $$QMAKE_OBJCOPY --add-gnu-debuglink=objcopy.debug objcopy
