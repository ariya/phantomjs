TARGET = qphantom
CONFIG += static

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = PhantomIntegrationPlugin
!equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
load(qt_plugin)

SOURCES += $$PWD/main.cpp

include(phantom.pri)
