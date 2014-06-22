TARGET = qevdevmouseplugin

PLUGIN_TYPE = generic
PLUGIN_EXTENDS = -
PLUGIN_CLASS_NAME = QEvdevMousePlugin
load(qt_plugin)

QT += core-private platformsupport-private gui-private

SOURCES = main.cpp

OTHER_FILES += \
    evdevmouse.json

