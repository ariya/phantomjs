DEFINES += QT_NO_FONTCONFIG

HEADERS += \
        $$PWD/qbasicfontdatabase_p.h \
        $$QT_SOURCE_TREE/src/gui/text/qfontengine_ft_p.h

SOURCES += \
        $$PWD/qbasicfontdatabase.cpp \
        $$QT_SOURCE_TREE/src/gui/text/qfontengine_ft.cpp

CONFIG += opentype

contains(QT_CONFIG, freetype) {
    QT_FREETYPE_DIR = $$QT_SOURCE_TREE/src/3rdparty/freetype
    SOURCES += \
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

   win32 {
       SOURCES += \
                  $$QT_FREETYPE_DIR/src/base/ftsystem.c
   } else {
       SOURCES += \
                  $$QT_FREETYPE_DIR/builds/unix/ftsystem.c
      INCLUDEPATH += \
                  $$QT_FREETYPE_DIR/builds/unix
   }

   INCLUDEPATH += \
       $$QT_FREETYPE_DIR/src \
       $$QT_FREETYPE_DIR/include

   TR_EXCLUDE += $$QT_FREETYPE_DIR/*

   DEFINES += FT2_BUILD_LIBRARY
   contains(QT_CONFIG, system-zlib) {
        DEFINES += FT_CONFIG_OPTION_SYSTEM_ZLIB
        include($$PWD/../../../3rdparty/zlib_dependency.pri)
   }

} else:contains(QT_CONFIG, system-freetype) {
    # pull in the proper freetype2 include directory
    include($$QT_SOURCE_TREE/config.tests/unix/freetype/freetype.pri)
}

