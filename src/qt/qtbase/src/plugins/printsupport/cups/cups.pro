TARGET = cupsprintersupport
MODULE = cupsprintersupport
PLUGIN_TYPE = printsupport
PLUGIN_CLASS_NAME = QCupsPrinterSupportPlugin
load(qt_plugin)

QT += core-private gui-private printsupport printsupport-private

LIBS_PRIVATE += -lcups

INCLUDEPATH += ../../../printsupport/kernel

SOURCES += main.cpp \
    qppdprintdevice.cpp \
    qcupsprintersupport.cpp \
    qcupsprintengine.cpp

HEADERS += qcupsprintersupport_p.h \
    qppdprintdevice.h \
    qcupsprintengine_p.h

OTHER_FILES += cups.json
