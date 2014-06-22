TARGET = qnmbearer

PLUGIN_TYPE = bearer
PLUGIN_CLASS_NAME = QNetworkManagerEnginePlugin
load(qt_plugin)

QT = core network-private dbus

HEADERS += qnmdbushelper.h \
           qnetworkmanagerservice.h \
           qnetworkmanagerengine.h \
           ../qnetworksession_impl.h \
           ../qbearerengine_impl.h

SOURCES += main.cpp \
           qnmdbushelper.cpp \
           qnetworkmanagerservice.cpp \
           qnetworkmanagerengine.cpp \
           ../qnetworksession_impl.cpp

OTHER_FILES += networkmanager.json
