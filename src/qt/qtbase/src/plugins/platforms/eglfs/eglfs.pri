QT += core-private gui-private platformsupport-private

# Avoid X11 header collision
DEFINES += MESA_EGL_NO_X11_HEADERS

# Uncomment these to enable the X hooks, allowing to test the platform
# plugin in a regular X11 environment (as long as EGL is available).
# EGLFS_PLATFORM_HOOKS_SOURCES += qeglfshooks_x11.cpp
# LIBS += -lX11 -lX11-xcb -lxcb

SOURCES +=  $$PWD/qeglfsintegration.cpp \
            $$PWD/qeglfswindow.cpp \
            $$PWD/qeglfsscreen.cpp \
            $$PWD/qeglfshooks_stub.cpp \
            $$PWD/qeglfscontext.cpp

HEADERS +=  $$PWD/qeglfsintegration.h \
            $$PWD/qeglfswindow.h \
            $$PWD/qeglfsscreen.h \
            $$PWD/qeglfshooks.h \
            $$PWD/qeglfscontext.h

QMAKE_LFLAGS += $$QMAKE_LFLAGS_NOUNDEF

INCLUDEPATH += $$PWD

!isEmpty(EGLFS_PLATFORM_HOOKS_SOURCES) {
    HEADERS += $$EGLFS_PLATFORM_HOOKS_HEADERS
    SOURCES += $$EGLFS_PLATFORM_HOOKS_SOURCES
    LIBS    += $$EGLFS_PLATFORM_HOOKS_LIBS
    DEFINES += EGLFS_PLATFORM_HOOKS
}

CONFIG += egl qpa/genericunixfontdatabase

RESOURCES += $$PWD/cursor.qrc

OTHER_FILES += \
    $$PWD/eglfs.json
