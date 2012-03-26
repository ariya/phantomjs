# common to plugin and built-in forms
INCLUDEPATH *= $$PWD
HEADERS += $$PWD/qmnghandler_p.h
SOURCES += $$PWD/qmnghandler.cpp
contains(QT_CONFIG, system-mng) {
        if(unix|win32-g++*):LIBS += -lmng
        else:win32:         LIBS += libmng.lib
} else {
    include($$PWD/../../3rdparty/libmng.pri)
}
