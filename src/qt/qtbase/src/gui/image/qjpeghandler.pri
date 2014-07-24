# common to plugin and built-in forms
INCLUDEPATH *= $$PWD
HEADERS += $$PWD/qjpeghandler_p.h
SOURCES += $$PWD/qjpeghandler.cpp
contains(QT_CONFIG, system-jpeg) {
    msvc: \
        LIBS += libjpeg.lib
    else: \
        LIBS += -ljpeg
} else {
    include($$PWD/../../3rdparty/libjpeg.pri)
}
