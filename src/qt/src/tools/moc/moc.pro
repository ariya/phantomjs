TEMPLATE	= app
TARGET		= moc

DEFINES	       += QT_MOC
DESTDIR         = ../../../bin
INCLUDEPATH	+= . 
DEPENDPATH	+= .
LIBS	        =
OBJECTS_DIR	= .

include(moc.pri)
HEADERS += qdatetime_p.h
SOURCES += main.cpp
include(../bootstrap/bootstrap.pri)

target.path=$$[QT_INSTALL_BINS]
INSTALLS += target
include(../../qt_targets.pri)
