include(../basicunix/basicunix.pri)

HEADERS += \
        $$QT_SOURCE_TREE/src/plugins/platforms/fontdatabases/fontconfig/qfontconfigdatabase.h

SOURCES += \
        $$QT_SOURCE_TREE/src/plugins/platforms/fontdatabases/fontconfig/qfontconfigdatabase.cpp

INCLUDEPATH += $$QT_SOURCE_TREE/src/plugins/platforms/fontdatabases/fontconfig
LIBS_PRIVATE += -lfontconfig


