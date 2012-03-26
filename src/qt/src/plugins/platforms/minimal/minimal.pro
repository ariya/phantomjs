TARGET = qminimal
include(../../qpluginbase.pri)

QTDIR_build:DESTDIR = $$QT_BUILD_TREE/plugins/platforms

SOURCES =   main.cpp \
            qminimalintegration.cpp \
            qminimalwindowsurface.cpp \
            qfontconfigdatabase.cpp

HEADERS =   qminimalintegration.h \
            qminimalwindowsurface.h \
            qfontconfigdatabase.h

include(../fontdatabases/basicunix/basicunix.pri)

target.path += $$[QT_INSTALL_PLUGINS]/platforms
INSTALLS += target
LIBS += -lfontconfig