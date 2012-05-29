# common to plugin and built-in forms
INCLUDEPATH *= $$PWD
HEADERS += $$PWD/qjpeghandler_p.h
SOURCES += $$PWD/qjpeghandler.cpp
contains(QT_CONFIG, system-jpeg) {
    if(unix|win32-g++*): LIBS += -ljpeg
    else:win32:          LIBS += libjpeg.lib
} else {
    include($$PWD/../../3rdparty/libjpeg.pri)
}
