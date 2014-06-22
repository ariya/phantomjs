option(host_build)
QT = core-private
force_bootstrap: QT += bootstrap_dbus-private
else: QT += dbus-private
DEFINES += QT_NO_CAST_FROM_ASCII
QMAKE_CXXFLAGS += $$QT_CFLAGS_DBUS

include(../moc/moc.pri)

SOURCES += qdbuscpp2xml.cpp

load(qt_tool)
