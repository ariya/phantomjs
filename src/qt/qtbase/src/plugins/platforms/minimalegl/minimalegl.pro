TARGET = qminimalegl

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QMinimalEglIntegrationPlugin
!equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
load(qt_plugin)

QT += core-private gui-private platformsupport-private

#DEFINES += QEGL_EXTRA_DEBUG

#DEFINES += Q_OPENKODE

#Avoid X11 header collision
DEFINES += MESA_EGL_NO_X11_HEADERS

SOURCES =   main.cpp \
            qminimaleglintegration.cpp \
            qminimaleglwindow.cpp \
            qminimaleglbackingstore.cpp \
            qminimaleglscreen.cpp

HEADERS =   qminimaleglintegration.h \
            qminimaleglwindow.h \
            qminimaleglbackingstore.h \
            qminimaleglscreen.h

CONFIG += egl qpa/genericunixfontdatabase

OTHER_FILES += \
    minimalegl.json
