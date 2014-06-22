TARGET = qgtk2

PLUGIN_TYPE = platformthemes
PLUGIN_EXTENDS = -
PLUGIN_CLASS_NAME = QGtk2ThemePlugin
load(qt_plugin)

QT += core-private gui-private platformsupport-private

CONFIG += X11
QMAKE_CXXFLAGS += $$QT_CFLAGS_QGTK2
LIBS += $$QT_LIBS_QGTK2

HEADERS += \
        qgtk2dialoghelpers.h \
        qgtk2theme.h

SOURCES += \
        main.cpp \
        qgtk2dialoghelpers.cpp \
        qgtk2theme.cpp \
