DEFINES += QT_NO_FONTCONFIG
HEADERS += \
        $$QT_SOURCE_TREE/src/plugins/platforms/fontdatabases/basicunix/qbasicunixfontdatabase.h \
        $$QT_SOURCE_TREE/src/gui/text/qfontengine_ft_p.h

SOURCES += \
        $$QT_SOURCE_TREE/src/plugins/platforms/fontdatabases/basicunix/qbasicunixfontdatabase.cpp \
        $$QT_SOURCE_TREE/src/gui/text/qfontengine_ft.cpp

INCLUDEPATH += $$QT_SOURCE_TREE/src/3rdparty/harfbuzz/src

INCLUDEPATH += $$QT_SOURCE_TREE/src/plugins/platforms/fontdatabases/basicunix

CONFIG += opentype

contains(QT_CONFIG, freetype) {
    SOURCES += \
               $$QT_SOURCE_TREE/src/3rdparty/freetype/src/base/ftbase.c \
               $$QT_SOURCE_TREE/src/3rdparty/freetype/src/base/ftbbox.c \
               $$QT_SOURCE_TREE/src/3rdparty/freetype/src/base/ftdebug.c \
               $$QT_SOURCE_TREE/src/3rdparty/freetype/src/base/ftglyph.c \
               $$QT_SOURCE_TREE/src/3rdparty/freetype/src/base/ftinit.c \
               $$QT_SOURCE_TREE/src/3rdparty/freetype/src/base/ftmm.c \
               $$QT_SOURCE_TREE/src/3rdparty/freetype/src/base/fttype1.c \
               $$QT_SOURCE_TREE/src/3rdparty/freetype/src/base/ftsynth.c \
               $$QT_SOURCE_TREE/src/3rdparty/freetype/src/base/ftbitmap.c \
               $$QT_SOURCE_TREE/src/3rdparty/freetype/src/bdf/bdf.c \
               $$QT_SOURCE_TREE/src/3rdparty/freetype/src/cache/ftcache.c \
               $$QT_SOURCE_TREE/src/3rdparty/freetype/src/cff/cff.c \
               $$QT_SOURCE_TREE/src/3rdparty/freetype/src/cid/type1cid.c \
               $$QT_SOURCE_TREE/src/3rdparty/freetype/src/gzip/ftgzip.c \
               $$QT_SOURCE_TREE/src/3rdparty/freetype/src/pcf/pcf.c \
               $$QT_SOURCE_TREE/src/3rdparty/freetype/src/pfr/pfr.c \
               $$QT_SOURCE_TREE/src/3rdparty/freetype/src/psaux/psaux.c \
               $$QT_SOURCE_TREE/src/3rdparty/freetype/src/pshinter/pshinter.c \
               $$QT_SOURCE_TREE/src/3rdparty/freetype/src/psnames/psmodule.c \
               $$QT_SOURCE_TREE/src/3rdparty/freetype/src/raster/raster.c \
               $$QT_SOURCE_TREE/src/3rdparty/freetype/src/sfnt/sfnt.c \
               $$QT_SOURCE_TREE/src/3rdparty/freetype/src/smooth/smooth.c \
               $$QT_SOURCE_TREE/src/3rdparty/freetype/src/truetype/truetype.c \
               $$QT_SOURCE_TREE/src/3rdparty/freetype/src/type1/type1.c \
               $$QT_SOURCE_TREE/src/3rdparty/freetype/src/type42/type42.c \
               $$QT_SOURCE_TREE/src/3rdparty/freetype/src/winfonts/winfnt.c \
               $$QT_SOURCE_TREE/src/3rdparty/freetype/src/lzw/ftlzw.c\
               $$QT_SOURCE_TREE/src/3rdparty/freetype/src/otvalid/otvalid.c\
               $$QT_SOURCE_TREE/src/3rdparty/freetype/src/otvalid/otvbase.c\
               $$QT_SOURCE_TREE/src/3rdparty/freetype/src/otvalid/otvgdef.c\
               $$QT_SOURCE_TREE/src/3rdparty/freetype/src/otvalid/otvjstf.c\
               $$QT_SOURCE_TREE/src/3rdparty/freetype/src/otvalid/otvcommn.c\
               $$QT_SOURCE_TREE/src/3rdparty/freetype/src/otvalid/otvgpos.c\
               $$QT_SOURCE_TREE/src/3rdparty/freetype/src/otvalid/otvgsub.c\
               $$QT_SOURCE_TREE/src/3rdparty/freetype/src/otvalid/otvmod.c\
               $$QT_SOURCE_TREE/src/3rdparty/freetype/src/autofit/afangles.c\
               $$QT_SOURCE_TREE/src/3rdparty/freetype/src/autofit/afglobal.c\
               $$QT_SOURCE_TREE/src/3rdparty/freetype/src/autofit/aflatin.c\
               $$QT_SOURCE_TREE/src/3rdparty/freetype/src/autofit/afmodule.c\
               $$QT_SOURCE_TREE/src/3rdparty/freetype/src/autofit/afdummy.c\
               $$QT_SOURCE_TREE/src/3rdparty/freetype/src/autofit/afhints.c\
               $$QT_SOURCE_TREE/src/3rdparty/freetype/src/autofit/afloader.c\
               $$QT_SOURCE_TREE/src/3rdparty/freetype/src/autofit/autofit.c

               symbian {
                   SOURCES += \
                              $$QT_SOURCE_TREE/src/3rdparty/freetype/src/base/ftsystem.c
               } else {
                   SOURCES += \
                              $$QT_SOURCE_TREE/src/3rdparty/freetype/builds/unix/ftsystem.c
                  INCLUDEPATH += \
                              $$QT_SOURCE_TREE/src/3rdparty/freetype/builds/unix
               }

               INCLUDEPATH += \
                   $$QT_SOURCE_TREE/src/3rdparty/freetype/src \
                   $$QT_SOURCE_TREE/src/3rdparty/freetype/include

               DEFINES += FT2_BUILD_LIBRARY
               contains(QT_CONFIG, system-zlib) {
                    DEFINES += FT_CONFIG_OPTION_SYSTEM_ZLIB
               }

    } else:contains(QT_CONFIG, system-freetype) {
        # pull in the proper freetype2 include directory
        include($$QT_SOURCE_TREE/config.tests/unix/freetype/freetype.pri)
        LIBS_PRIVATE += -lfreetype
    }

