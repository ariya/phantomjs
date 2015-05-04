TARGET = qwinrt
CONFIG -= precompile_header

# For Windows Phone 8 we have to deploy fonts together with the application as DirectWrite
# is not supported here.
winphone:equals(WINSDK_VER, 8.0): {
    fonts.path = $$[QT_INSTALL_LIBS]/fonts
    fonts.files = $$QT_SOURCE_TREE/lib/fonts/DejaVu*.ttf
    INSTALLS += fonts
}

PLUGIN_TYPE = platforms
PLUGIN_CLASS_NAME = QWinRTIntegrationPlugin
!equals(TARGET, $$QT_DEFAULT_QPA_PLUGIN): PLUGIN_EXTENDS = -
load(qt_plugin)

QT += core-private gui-private platformsupport-private

DEFINES *= QT_NO_CAST_FROM_ASCII __WRL_NO_DEFAULT_LIB__ GL_GLEXT_PROTOTYPES

LIBS += $$QMAKE_LIBS_CORE

!if(winphone:equals(WINSDK_VER, 8.0)) {
    LIBS += -ldwrite
    INCLUDEPATH += $$QT_SOURCE_TREE/src/3rdparty/freetype/include
    DEFINES += QT_WINRT_USE_DWRITE
}

SOURCES = \
    main.cpp  \
    qwinrtbackingstore.cpp \
    qwinrtcursor.cpp \
    qwinrteglcontext.cpp \
    qwinrteventdispatcher.cpp \
    qwinrtfiledialoghelper.cpp \
    qwinrtfileengine.cpp \
    qwinrtfontdatabase.cpp \
    qwinrtinputcontext.cpp \
    qwinrtintegration.cpp \
    qwinrtmessagedialoghelper.cpp \
    qwinrtscreen.cpp \
    qwinrtservices.cpp \
    qwinrttheme.cpp \
    qwinrtwindow.cpp


HEADERS = \
    qwinrtbackingstore.h \
    qwinrtcursor.h \
    qwinrteglcontext.h \
    qwinrteventdispatcher.h \
    qwinrtfiledialoghelper.h \
    qwinrtfileengine.h \
    qwinrtfontdatabase.h \
    qwinrtinputcontext.h \
    qwinrtintegration.h \
    qwinrtmessagedialoghelper.h \
    qwinrtscreen.h \
    qwinrtservices.h \
    qwinrttheme.h \
    qwinrtwindow.h

winphone:equals(WINSDK_VER, 8.0): {
    SOURCES -= qwinrtplatformmessagedialoghelper.cpp
    HEADERS -= qwinrtplatformmessagedialoghelper.h
}

OTHER_FILES += winrt.json
