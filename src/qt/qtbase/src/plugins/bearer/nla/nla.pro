TARGET = qnlabearer

PLUGIN_TYPE = bearer
PLUGIN_CLASS_NAME = QNlaEnginePlugin
load(qt_plugin)

QT = core core-private network network-private

!wince* {
    LIBS += -lws2_32
} else {
    LIBS += -lws2
}

HEADERS += qnlaengine.h \
           ../platformdefs_win.h \
           ../qnetworksession_impl.h \
           ../qbearerengine_impl.h

SOURCES += main.cpp \
           qnlaengine.cpp \
           ../qnetworksession_impl.cpp

OTHER_FILES += nla.json
