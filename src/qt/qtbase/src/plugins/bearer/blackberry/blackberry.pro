TARGET = qbbbearer

PLUGIN_TYPE = bearer
PLUGIN_CLASS_NAME = QBBEnginePlugin
load(qt_plugin)

QT = core-private network-private

# Uncomment this to enable debugging output for the plugin
#DEFINES += QBBENGINE_DEBUG

HEADERS += qbbengine.h \
           ../qnetworksession_impl.h \
           ../qbearerengine_impl.h

SOURCES += qbbengine.cpp \
           ../qnetworksession_impl.cpp \
           main.cpp

OTHER_FILES += blackberry.json
