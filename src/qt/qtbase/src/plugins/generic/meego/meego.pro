TARGET = qmeegointegration

PLUGIN_TYPE = generic
PLUGIN_EXTENDS = -
PLUGIN_CLASS_NAME = QMeeGoIntegrationPlugin
load(qt_plugin)

SOURCES = qmeegointegration.cpp \
          main.cpp \
          contextkitproperty.cpp
HEADERS = qmeegointegration.h \
          contextkitproperty.h

QT = core-private gui-private dbus gui-private
