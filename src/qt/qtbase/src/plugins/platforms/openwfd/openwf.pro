TARGET = qopenwf

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QOpenWFDIntegrationPlugin
!equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
load(qt_plugin)

QT += core-private gui-private platformsupport-private

CONFIG += qpa/genericunixfontdatabase

HEADERS += \
    qopenwfddevice.h \
    qopenwfdintegration.h \
    qopenwfdnativeinterface.h \
    qopenwfdscreen.h \
    qopenwfdbackingstore.h \
    qopenwfdevent.h \
    qopenwfdglcontext.h \
    qopenwfdoutputbuffer.h \
    qopenwfdport.h \
    qopenwfdwindow.h \
    qopenwfdportmode.h

SOURCES += \
    main.cpp \
    qopenwfddevice.cpp \
    qopenwfdintegration.cpp \
    qopenwfdnativeinterface.cpp \
    qopenwfdscreen.cpp \
    qopenwfdbackingstore.cpp \
    qopenwfdevent.cpp \
    qopenwfdglcontext.cpp \
    qopenwfdoutputbuffer.cpp \
    qopenwfdport.cpp \
    qopenwfdportmode.cpp \
    qopenwfdwindow.cpp

LIBS += -lWFD -lgbm -lGLESv2 -lEGL

