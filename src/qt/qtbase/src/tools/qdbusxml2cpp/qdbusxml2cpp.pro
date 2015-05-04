option(host_build)
QT = core-private
force_bootstrap: QT += bootstrap_dbus-private
else: QT += dbus-private
DEFINES += QT_NO_CAST_FROM_ASCII
QMAKE_CXXFLAGS += $$QT_HOST_CFLAGS_DBUS

SOURCES = qdbusxml2cpp.cpp

load(qt_tool)
