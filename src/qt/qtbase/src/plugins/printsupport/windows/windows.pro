TARGET = windowsprintersupport
MODULE = windowsprintersupport
PLUGIN_TYPE = printsupport
PLUGIN_CLASS_NAME = QWindowsPrinterSupportPlugin
load(qt_plugin)

QT *= core-private
QT *= gui-private
QT *= printsupport-private

INCLUDEPATH *= $$QT_SOURCE_TREE/src/printsupport/kernel

SOURCES += \
    main.cpp \
    qwindowsprintersupport.cpp \
    qwindowsprintdevice.cpp \

HEADERS += \
    qwindowsprintersupport.h \
    qwindowsprintdevice.h \

OTHER_FILES += windows.json

LIBS += -lwinspool -lcomdlg32 -lgdi32 -luser32
