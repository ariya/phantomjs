TARGET = qnmbearer

PLUGIN_TYPE = bearer
PLUGIN_CLASS_NAME = QNetworkManagerEnginePlugin
load(qt_plugin)

QT = core network-private dbus

HEADERS += qnetworkmanagerservice.h \
           qnetworkmanagerengine.h \
           ../linux_common/qofonoservice_linux_p.h \
           ../qnetworksession_impl.h \
           ../qbearerengine_impl.h

SOURCES += main.cpp \
           qnetworkmanagerservice.cpp \
           qnetworkmanagerengine.cpp \
           ../linux_common/qofonoservice_linux.cpp \
           ../qnetworksession_impl.cpp

OTHER_FILES += networkmanager.json
