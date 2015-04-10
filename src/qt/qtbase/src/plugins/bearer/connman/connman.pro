TARGET = qconnmanbearer

PLUGIN_TYPE = bearer
PLUGIN_CLASS_NAME = QConnmanEnginePlugin
load(qt_plugin)

QT = core network-private dbus
CONFIG += link_pkgconfig
packagesExist(connectionagent) { DEFINES += QT_HAS_CONNECTIONAGENT }

HEADERS += qconnmanservice_linux_p.h \
           qofonoservice_linux_p.h \
           qconnmanengine.h \
           ../qnetworksession_impl.h \
           ../qbearerengine_impl.h

SOURCES += main.cpp \
           qconnmanservice_linux.cpp \
           qofonoservice_linux.cpp \
           qconnmanengine.cpp \
           ../qnetworksession_impl.cpp

OTHER_FILES += connman.json

