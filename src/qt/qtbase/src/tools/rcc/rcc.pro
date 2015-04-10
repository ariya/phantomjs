option(host_build)
CONFIG += force_bootstrap

DEFINES += QT_RCC QT_NO_CAST_FROM_ASCII

include(rcc.pri)
SOURCES += main.cpp

load(qt_tool)
