TARGET = qxcb

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QXcbIntegrationPlugin
!equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
load(qt_plugin)

QT += core-private gui-private platformsupport-private

SOURCES = \
        qxcbclipboard.cpp \
        qxcbconnection.cpp \
        qxcbintegration.cpp \
        qxcbkeyboard.cpp \
        qxcbmime.cpp \
        qxcbdrag.cpp \
        qxcbscreen.cpp \
        qxcbwindow.cpp \
        qxcbbackingstore.cpp \
        qxcbwmsupport.cpp \
        qxcbmain.cpp \
        qxcbnativeinterface.cpp \
        qxcbcursor.cpp \
        qxcbimage.cpp \
        qxcbxsettings.cpp \
        qxcbsystemtraytracker.cpp

HEADERS = \
        qxcbclipboard.h \
        qxcbconnection.h \
        qxcbintegration.h \
        qxcbkeyboard.h \
        qxcbdrag.h \
        qxcbmime.h \
        qxcbobject.h \
        qxcbscreen.h \
        qxcbwindow.h \
        qxcbbackingstore.h \
        qxcbwmsupport.h \
        qxcbnativeinterface.h \
        qxcbcursor.h \
        qxcbimage.h \
        qxcbxsettings.h \
        qxcbsystemtraytracker.h

LIBS += $$QMAKE_LIBS_DYNLOAD

# needed by GLX, Xcursor ...
contains(QT_CONFIG, xcb-xlib) {
    DEFINES += XCB_USE_XLIB
    LIBS += -lX11 -lX11-xcb

    *-maemo* {
        contains(QT_CONFIG, xinput2) {
            # XInput2 support for Harmattan.
            DEFINES += XCB_USE_XINPUT2_MAEMO
            SOURCES += qxcbconnection_maemo.cpp
            LIBS += -lXi
        }
        DEFINES += XCB_USE_MAEMO_WINDOW_PROPERTIES
    } else {
        contains(QT_CONFIG, xinput2) {
            DEFINES += XCB_USE_XINPUT2
            SOURCES += qxcbconnection_xi2.cpp
            LIBS += -lXi
        }
    }
}

# to support custom cursors with depth > 1
contains(QT_CONFIG, xcb-render) {
    DEFINES += XCB_USE_RENDER
    LIBS += -lxcb-render -lxcb-render-util -lXrender
}

# build with session management support
contains(QT_CONFIG, xcb-sm) {
    DEFINES += XCB_USE_SM
    LIBS += -lSM -lICE
    SOURCES += qxcbsessionmanager.cpp
    HEADERS += qxcbsessionmanager.h
}

contains(QT_CONFIG, opengl) {
    contains(QT_CONFIG, xcb-xlib):!contains(QT_CONFIG, opengles2) {
        DEFINES += XCB_USE_GLX
        HEADERS += qglxintegration.h
        SOURCES += qglxintegration.cpp
        LIBS += $$QMAKE_LIBS_DYNLOAD
        contains(QT_CONFIG, xcb-glx) {
            DEFINES += XCB_HAS_XCB_GLX
            LIBS += -lxcb-glx
        }
    } else:contains(QT_CONFIG, egl):contains(QT_CONFIG, egl_x11) {
        DEFINES += XCB_USE_EGL
        CONFIG += egl
        HEADERS += qxcbeglsurface.h

        # EGL on MeeGo 1.2 Harmattan needs this macro to map EGLNativeDisplayType
        # and other types to the correct X11 types
        DEFINES += SUPPORT_X11
    }
}

DEFINES += $$QMAKE_DEFINES_XCB
LIBS += $$QMAKE_LIBS_XCB
QMAKE_CXXFLAGS += $$QMAKE_CFLAGS_XCB

CONFIG += qpa/genericunixfontdatabase

contains(QT_CONFIG, dbus) {
DEFINES += XCB_USE_IBUS
QT += dbus
LIBS += -ldbus-1
}

OTHER_FILES += xcb.json README

contains(QT_CONFIG, xcb-qt) {
    DEFINES += XCB_USE_RENDER
    XCB_DIR = ../../../3rdparty/xcb
    INCLUDEPATH += $$XCB_DIR/include $$XCB_DIR/sysinclude
    LIBS += -lxcb -L$$OUT_PWD/xcb-static -lxcb-static
} else {
    LIBS += -lxcb -lxcb-image -lxcb-icccm -lxcb-sync -lxcb-xfixes -lxcb-shm -lxcb-randr -lxcb-shape -lxcb-keysyms
    !contains(DEFINES, QT_NO_XKB):LIBS += -lxcb-xkb
}

# libxkbcommon
contains(QT_CONFIG, xkbcommon-qt): {
    QT_CONFIG += use-xkbcommon-x11support
    include(../../../3rdparty/xkbcommon.pri)
} else {
    LIBS += $$QMAKE_LIBS_XKBCOMMON
    QMAKE_CXXFLAGS += $$QMAKE_CFLAGS_XKBCOMMON
}
