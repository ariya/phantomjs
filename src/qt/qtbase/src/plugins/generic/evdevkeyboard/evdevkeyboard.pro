TARGET = qevdevkeyboardplugin

PLUGIN_TYPE = generic
PLUGIN_EXTENDS = -
PLUGIN_CLASS_NAME = QEvdevKeyboardPlugin
load(qt_plugin)

QT += core-private platformsupport-private gui-private

SOURCES = main.cpp

OTHER_FILES += \
    evdevkeyboard.json

