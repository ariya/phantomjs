TARGET  = qjpeg

PLUGIN_TYPE = imageformats
PLUGIN_CLASS_NAME = QJpegPlugin
load(qt_plugin)

QT += core-private

QTDIR_build:REQUIRES = "!contains(QT_CONFIG, no-jpeg)"

include(../../../gui/image/qjpeghandler.pri)
SOURCES += main.cpp
HEADERS += main.h
OTHER_FILES += jpeg.json
