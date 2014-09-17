# common to plugin and built-in forms
INCLUDEPATH *= $$PWD
HEADERS += $$PWD/qtiffhandler_p.h
SOURCES += $$PWD/qtiffhandler.cpp
contains(QT_CONFIG, system-tiff) {
        if(unix|win32-g++*):LIBS += -ltiff
        else:win32:         LIBS += libtiff.lib
} else {
    include($$PWD/../../3rdparty/libtiff.pri)
}
