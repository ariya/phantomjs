TARGET = cocoaprintersupport
MODULE = cocoaprintersupport
PLUGIN_TYPE = printsupport
PLUGIN_CLASS_NAME = QCocoaPrinterSupportPlugin
load(qt_plugin)

QT += gui-private printsupport-private
LIBS += -framework Cocoa

SOURCES += main.cpp

OTHER_FILES += cocoa.json
