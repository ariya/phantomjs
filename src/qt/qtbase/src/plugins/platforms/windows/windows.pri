# Note: OpenGL32 must precede Gdi32 as it overwrites some functions.
LIBS *= -lole32
!wince*:LIBS *= -luser32 -lwinspool -limm32 -lwinmm  -loleaut32

contains(QT_CONFIG, opengl):!contains(QT_CONFIG, opengles2):!contains(QT_CONFIG, dynamicgl): LIBS *= -lopengl32

mingw: LIBS *= -luuid
# For the dialog helpers:
!wince*:LIBS *= -lshlwapi -lshell32
!wince*:LIBS *= -ladvapi32
wince*:DEFINES *= QT_LIBINFIX=L"\"\\\"$${QT_LIBINFIX}\\\"\""

DEFINES *= QT_NO_CAST_FROM_ASCII

contains(QT_CONFIG, directwrite) {
    LIBS *= -ldwrite
    SOURCES += $$PWD/qwindowsfontenginedirectwrite.cpp
    HEADERS += $$PWD/qwindowsfontenginedirectwrite.h
} else {
    DEFINES *= QT_NO_DIRECTWRITE
}

SOURCES += \
    $$PWD/qwindowswindow.cpp \
    $$PWD/qwindowsintegration.cpp \
    $$PWD/qwindowscontext.cpp \
    $$PWD/qwindowsscreen.cpp \
    $$PWD/qwindowskeymapper.cpp \
    $$PWD/qwindowsfontengine.cpp \
    $$PWD/qwindowsfontdatabase.cpp \
    $$PWD/qwindowsmousehandler.cpp \
    $$PWD/qwindowsguieventdispatcher.cpp \
    $$PWD/qwindowsole.cpp \
    $$PWD/qwindowsmime.cpp \
    $$PWD/qwindowsinternalmimedata.cpp \
    $$PWD/qwindowscursor.cpp \
    $$PWD/qwindowsinputcontext.cpp \
    $$PWD/qwindowstheme.cpp \
    $$PWD/qwindowsdialoghelpers.cpp \
    $$PWD/qwindowsservices.cpp \
    $$PWD/qwindowsnativeimage.cpp \
    $$PWD/qwindowsnativeinterface.cpp

HEADERS += \
    $$PWD/qwindowswindow.h \
    $$PWD/qwindowsintegration.h \
    $$PWD/qwindowscontext.h \
    $$PWD/qwindowsscreen.h \
    $$PWD/qwindowskeymapper.h \
    $$PWD/qwindowsfontengine.h \
    $$PWD/qwindowsfontdatabase.h \
    $$PWD/qwindowsmousehandler.h \
    $$PWD/qwindowsguieventdispatcher.h \
    $$PWD/qtwindowsglobal.h \
    $$PWD/qtwindows_additional.h \
    $$PWD/qwindowsole.h \
    $$PWD/qwindowsmime.h \
    $$PWD/qwindowsinternalmimedata.h \
    $$PWD/qwindowscursor.h \
    $$PWD/array.h \
    $$PWD/qwindowsinputcontext.h \
    $$PWD/qwindowstheme.h \
    $$PWD/qwindowsdialoghelpers.h \
    $$PWD/qwindowsservices.h \
    $$PWD/qplatformfunctions_wince.h \
    $$PWD/qwindowsnativeimage.h \
    $$PWD/qwindowsnativeinterface.h

INCLUDEPATH += $$PWD

contains(QT_CONFIG, opengles2) {
    SOURCES += $$PWD/qwindowseglcontext.cpp
    HEADERS += $$PWD/qwindowseglcontext.h
} else: contains(QT_CONFIG,opengl) {
    SOURCES += $$PWD/qwindowsglcontext.cpp
    HEADERS += $$PWD/qwindowsglcontext.h
}

# Dynamic GL needs both WGL and EGL
contains(QT_CONFIG,dynamicgl) {
    SOURCES += $$PWD/qwindowseglcontext.cpp
    HEADERS += $$PWD/qwindowseglcontext.h
}

!contains( DEFINES, QT_NO_CLIPBOARD ) {
    SOURCES += $$PWD/qwindowsclipboard.cpp
    HEADERS += $$PWD/qwindowsclipboard.h
}

# drag and drop on windows only works if a clipboard is available
!contains( DEFINES, QT_NO_DRAGANDDROP ) {
    !win32:SOURCES += $$PWD/qwindowsdrag.cpp
    !win32:HEADERS += $$PWD/qwindowsdrag.h
    win32:!contains( DEFINES, QT_NO_CLIPBOARD ) {
        HEADERS += $$PWD/qwindowsdrag.h
        SOURCES += $$PWD/qwindowsdrag.cpp
    }
}

!wince*:!contains( DEFINES, QT_NO_TABLETEVENT ) {
    INCLUDEPATH += $$QT_SOURCE_TREE/src/3rdparty/wintab
    HEADERS += $$PWD/qwindowstabletsupport.h
    SOURCES += $$PWD/qwindowstabletsupport.cpp
}

!wince*:!contains( DEFINES, QT_NO_SESSIONMANAGER ) {
    SOURCES += $$PWD/qwindowssessionmanager.cpp
    HEADERS += $$PWD/qwindowssessionmanager.h
}

contains(QT_CONFIG, freetype) {
    DEFINES *= QT_NO_FONTCONFIG
    QT_FREETYPE_DIR = $$QT_SOURCE_TREE/src/3rdparty/freetype

    HEADERS += \
               $$PWD/qwindowsfontdatabase_ft.h
    SOURCES += \
               $$PWD/qwindowsfontdatabase_ft.cpp \
               $$QT_FREETYPE_DIR/src/base/ftbase.c \
               $$QT_FREETYPE_DIR/src/base/ftbbox.c \
               $$QT_FREETYPE_DIR/src/base/ftdebug.c \
               $$QT_FREETYPE_DIR/src/base/ftglyph.c \
               $$QT_FREETYPE_DIR/src/base/ftinit.c \
               $$QT_FREETYPE_DIR/src/base/ftmm.c \
               $$QT_FREETYPE_DIR/src/base/fttype1.c \
               $$QT_FREETYPE_DIR/src/base/ftsynth.c \
               $$QT_FREETYPE_DIR/src/base/ftbitmap.c \
               $$QT_FREETYPE_DIR/src/bdf/bdf.c \
               $$QT_FREETYPE_DIR/src/cache/ftcache.c \
               $$QT_FREETYPE_DIR/src/cff/cff.c \
               $$QT_FREETYPE_DIR/src/cid/type1cid.c \
               $$QT_FREETYPE_DIR/src/gzip/ftgzip.c \
               $$QT_FREETYPE_DIR/src/pcf/pcf.c \
               $$QT_FREETYPE_DIR/src/pfr/pfr.c \
               $$QT_FREETYPE_DIR/src/psaux/psaux.c \
               $$QT_FREETYPE_DIR/src/pshinter/pshinter.c \
               $$QT_FREETYPE_DIR/src/psnames/psmodule.c \
               $$QT_FREETYPE_DIR/src/raster/raster.c \
               $$QT_FREETYPE_DIR/src/sfnt/sfnt.c \
               $$QT_FREETYPE_DIR/src/smooth/smooth.c \
               $$QT_FREETYPE_DIR/src/truetype/truetype.c \
               $$QT_FREETYPE_DIR/src/type1/type1.c \
               $$QT_FREETYPE_DIR/src/type42/type42.c \
               $$QT_FREETYPE_DIR/src/winfonts/winfnt.c \
               $$QT_FREETYPE_DIR/src/lzw/ftlzw.c\
               $$QT_FREETYPE_DIR/src/otvalid/otvalid.c\
               $$QT_FREETYPE_DIR/src/otvalid/otvbase.c\
               $$QT_FREETYPE_DIR/src/otvalid/otvgdef.c\
               $$QT_FREETYPE_DIR/src/otvalid/otvjstf.c\
               $$QT_FREETYPE_DIR/src/otvalid/otvcommn.c\
               $$QT_FREETYPE_DIR/src/otvalid/otvgpos.c\
               $$QT_FREETYPE_DIR/src/otvalid/otvgsub.c\
               $$QT_FREETYPE_DIR/src/otvalid/otvmod.c\
               $$QT_FREETYPE_DIR/src/autofit/afangles.c\
               $$QT_FREETYPE_DIR/src/autofit/afglobal.c\
               $$QT_FREETYPE_DIR/src/autofit/aflatin.c\
               $$QT_FREETYPE_DIR/src/autofit/afmodule.c\
               $$QT_FREETYPE_DIR/src/autofit/afdummy.c\
               $$QT_FREETYPE_DIR/src/autofit/afhints.c\
               $$QT_FREETYPE_DIR/src/autofit/afloader.c\
               $$QT_FREETYPE_DIR/src/autofit/autofit.c

   SOURCES += $$QT_FREETYPE_DIR/src/base/ftsystem.c


   INCLUDEPATH += \
       $$QT_FREETYPE_DIR/src \
       $$QT_FREETYPE_DIR/include

   TR_EXCLUDE += $$QT_FREETYPE_DIR/*

   DEFINES += FT2_BUILD_LIBRARY
   contains(QT_CONFIG, system-zlib) {
        DEFINES += FT_CONFIG_OPTION_SYSTEM_ZLIB
   }
} else:contains(QT_CONFIG, system-freetype) {
    include($$QT_SOURCE_TREE/src/platformsupport/fontdatabases/basic/basic.pri)
    HEADERS += \
               $$PWD/qwindowsfontdatabase_ft.h
    SOURCES += \
               $$PWD/qwindowsfontdatabase_ft.cpp
}

contains(QT_CONFIG, accessibility):include($$PWD/accessible/accessible.pri)
