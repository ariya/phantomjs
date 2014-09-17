INCLUDEPATH *= $$PWD
HEADERS += $$PWD/qpnghandler_p.h
SOURCES += $$PWD/qpnghandler.cpp
contains(QT_CONFIG, system-png) {
    if(unix|win32-g++*): LIBS_PRIVATE  += -lpng
    else:win32:          LIBS += libpng.lib

} else {
    include($$PWD/../../3rdparty/libpng.pri)
}
