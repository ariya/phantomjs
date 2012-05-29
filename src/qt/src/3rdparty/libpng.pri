DEFINES *= QT_USE_BUNDLED_LIBPNG
!isEqual(QT_ARCH, i386):!isEqual(QT_ARCH, x86_64):DEFINES += PNG_NO_ASSEMBLER_CODE
INCLUDEPATH += $$PWD/libpng

SOURCES += $$PWD/libpng/privatepng.cpp

include($$PWD/zlib_dependency.pri)
