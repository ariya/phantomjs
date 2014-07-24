TARGET = qwinrt
CONFIG -= precompile_header

# For Windows Phone 8 we have to deploy fonts together with the application as DirectWrite
# is not supported here.
# TODO: Add a condition/remove this block if Windows Phone 8.1 supports DirectWrite
winphone {
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

!winphone {
    LIBS += -ldwrite
    INCLUDEPATH += $$QT_SOURCE_TREE/src/3rdparty/freetype/include
}

SOURCES = \
    main.cpp  \
    qwinrtbackingstore.cpp \
    qwinrtcursor.cpp \
    qwinrteglcontext.cpp \
    qwinrteventdispatcher.cpp \
    qwinrtfontdatabase.cpp \
    qwinrtinputcontext.cpp \
    qwinrtintegration.cpp \
    qwinrtplatformmessagedialoghelper.cpp \
    qwinrtplatformtheme.cpp \
    qwinrtscreen.cpp \
    qwinrtservices.cpp \
    qwinrtwindow.cpp

HEADERS = \
    qwinrtbackingstore.h \
    qwinrtcursor.h \
    qwinrteglcontext.h \
    qwinrteventdispatcher.h \
    qwinrtfontdatabase.h \
    qwinrtinputcontext.h \
    qwinrtintegration.h \
    qwinrtplatformmessagedialoghelper.h \
    qwinrtplatformtheme.h \
    qwinrtscreen.h \
    qwinrtservices.h \
    qwinrtwindow.h

BLIT_INPUT = $$PWD/blit.hlsl
fxc_blitps.commands = fxc.exe /nologo /T ps_4_0_level_9_1 /E blitps /Vn q_blitps /Fh ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}
fxc_blitps.output = $$OUT_PWD/blitps.h
fxc_blitps.input = BLIT_INPUT
fxc_blitps.dependency_type = TYPE_C
fxc_blitps.variable_out = HEADERS
fxc_blitps.CONFIG += target_predeps
fxc_blitvs.commands = fxc.exe /nologo /T vs_4_0_level_9_1 /E blitvs /Vn q_blitvs /Fh ${QMAKE_FILE_OUT} ${QMAKE_FILE_NAME}
fxc_blitvs.output = $$OUT_PWD/blitvs.h
fxc_blitvs.input = BLIT_INPUT
fxc_blitvs.dependency_type = TYPE_C
fxc_blitvs.variable_out = HEADERS
fxc_blitvs.CONFIG += target_predeps
QMAKE_EXTRA_COMPILERS += fxc_blitps fxc_blitvs

winphone {
    SOURCES -= qwinrtplatformmessagedialoghelper.cpp
    HEADERS -= qwinrtplatformmessagedialoghelper.h
}

OTHER_FILES += winrt.json \
    blit.hlsl
