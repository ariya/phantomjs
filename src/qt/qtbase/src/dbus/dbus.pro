TARGET = QtDBus
QT = core-private
CONFIG += link_pkgconfig
MODULE_CONFIG = dbusadaptors dbusinterfaces

!contains(QT_LIBS_DBUS, .*dbus-1.*) {
    win32:CONFIG(debug, debug|release):QT_LIBS_DBUS += -ldbus-1d
    else:QT_LIBS_DBUS += -ldbus-1
}

DEFINES += DBUS_API_SUBJECT_TO_CHANGE
QMAKE_CXXFLAGS += $$QT_CFLAGS_DBUS
contains(QT_CONFIG, dbus-linked) {
    LIBS_PRIVATE += $$QT_LIBS_DBUS
    DEFINES += QT_LINKED_LIBDBUS
}

win32 { 
    wince*:LIBS_PRIVATE += -lws2
    else:LIBS_PRIVATE += -lws2_32 \
        -ladvapi32 \
        -lnetapi32 \
        -luser32
}

QMAKE_DOCS = $$PWD/doc/qtdbus.qdocconf

load(qt_module)

PUB_HEADERS = qdbusargument.h \
    qdbusconnectioninterface.h \
    qdbusmacros.h \
    qdbuserror.h \
    qdbusextratypes.h \
    qdbusmessage.h \
    qdbusserver.h \
    qdbusconnection.h \
    qdbusabstractinterface.h \
    qdbusinterface.h \
    qdbusabstractadaptor.h \
    qdbusreply.h \
    qdbusmetatype.h \
    qdbuspendingcall.h \
    qdbuspendingreply.h \
    qdbuscontext.h \
    qdbusvirtualobject.h \
    qdbusservicewatcher.h \
    qdbusunixfiledescriptor.h
HEADERS += $$PUB_HEADERS \
    qdbusconnection_p.h \
    qdbusconnectionmanager_p.h \
    qdbusmessage_p.h \
    qdbusinterface_p.h \
    qdbusxmlparser_p.h \
    qdbusabstractadaptor_p.h \
    qdbusargument_p.h \
    qdbusutil_p.h \
    qdbusabstractinterface_p.h \
    qdbuscontext_p.h \
    qdbusthreaddebug_p.h \
    qdbusintegrator_p.h \
    qdbuspendingcall_p.h \
    qdbus_symbols_p.h \
    qdbusintrospection_p.h
SOURCES += qdbusconnection.cpp \
    qdbusconnectioninterface.cpp \
    qdbuserror.cpp \
    qdbusintegrator.cpp \
    qdbusmessage.cpp \
    qdbusserver.cpp \
    qdbusabstractinterface.cpp \
    qdbusinterface.cpp \
    qdbusxmlparser.cpp \
    qdbusutil.cpp \
    qdbusintrospection.cpp \
    qdbusabstractadaptor.cpp \
    qdbusinternalfilters.cpp \
    qdbusmetaobject.cpp \
    qdbusxmlgenerator.cpp \
    qdbusmisc.cpp \
    qdbusargument.cpp \
    qdbusreply.cpp \
    qdbusmetatype.cpp \
    qdbusextratypes.cpp \
    qdbusmarshaller.cpp \
    qdbuscontext.cpp \
    qdbuspendingcall.cpp \
    qdbuspendingreply.cpp \
    qdbus_symbols.cpp \
    qdbusservicewatcher.cpp \
    qdbusunixfiledescriptor.cpp \
    qdbusvirtualobject.cpp
