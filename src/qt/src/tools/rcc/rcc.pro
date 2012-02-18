TEMPLATE = app
TARGET = rcc

DESTDIR = ../../../bin
DEFINES += QT_RCC
INCLUDEPATH += .
DEPENDPATH += .

include(rcc.pri)
HEADERS += ../../corelib/kernel/qcorecmdlineargs_p.h
SOURCES += main.cpp
include(../bootstrap/bootstrap.pri)

target.path=$$[QT_INSTALL_BINS]
INSTALLS += target
include(../../qt_targets.pri)
