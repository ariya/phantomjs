include(wrappers/wrappers.pri)

TARGET = qandroidbearer

PLUGIN_TYPE = bearer
PLUGIN_CLASS_NAME = QAndroidBearerEnginePlugin
load(qt_plugin)

QT = core-private network-private

HEADERS += qandroidbearerengine.h \
           ../../qnetworksession_impl.h \
           ../../qbearerengine_impl.h

SOURCES += main.cpp \
           qandroidbearerengine.cpp \
           ../../qnetworksession_impl.cpp
