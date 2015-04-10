# -*-mode:sh-*-
# Qt image handling

# Qt kernel module

HEADERS += \
        image/qbitmap.h \
        image/qimage.h \
        image/qimage_p.h \
        image/qimageiohandler.h \
        image/qimagereader.h \
        image/qimagewriter.h \
        image/qmovie.h \
        image/qnativeimage_p.h \
        image/qpaintengine_pic_p.h \
        image/qpicture.h \
        image/qpicture_p.h \
        image/qpictureformatplugin.h \
        image/qpixmap.h \
        image/qpixmap_raster_p.h \
        image/qpixmap_blitter_p.h \
        image/qpixmapcache.h \
        image/qpixmapcache_p.h \
        image/qplatformpixmap.h \
        image/qimagepixmapcleanuphooks_p.h \
        image/qicon.h \
        image/qicon_p.h \
        image/qiconloader_p.h \
        image/qiconengine.h \
        image/qiconengineplugin.h \

SOURCES += \
        image/qbitmap.cpp \
        image/qimage.cpp \
        image/qimage_conversions.cpp \
        image/qimageiohandler.cpp \
        image/qimagereader.cpp \
        image/qimagewriter.cpp \
        image/qpaintengine_pic.cpp \
        image/qpicture.cpp \
        image/qpictureformatplugin.cpp \
        image/qpixmap.cpp \
        image/qpixmapcache.cpp \
        image/qplatformpixmap.cpp \
        image/qmovie.cpp \
        image/qpixmap_raster.cpp \
        image/qpixmap_blitter.cpp \
        image/qnativeimage.cpp \
        image/qimagepixmapcleanuphooks.cpp \
        image/qicon.cpp \
        image/qiconloader.cpp \
        image/qiconengine.cpp \
        image/qiconengineplugin.cpp \


win32:!winrt: SOURCES += image/qpixmap_win.cpp

NO_PCH_SOURCES += image/qimage_compat.cpp
false: SOURCES += $$NO_PCH_SOURCES # Hack for QtCreator

# Built-in image format support
HEADERS += \
        image/qbmphandler_p.h \
        image/qppmhandler_p.h \
        image/qxbmhandler_p.h \
        image/qxpmhandler_p.h

SOURCES += \
        image/qbmphandler.cpp \
        image/qppmhandler.cpp \
        image/qxbmhandler.cpp \
        image/qxpmhandler.cpp

!contains(QT_CONFIG, no-png):include($$PWD/qpnghandler.pri)
else:DEFINES *= QT_NO_IMAGEFORMAT_PNG

contains(QT_CONFIG, jpeg):include($$PWD/qjpeghandler.pri)
contains(QT_CONFIG, gif):include($$PWD/qgifhandler.pri)

# SIMD
contains(QT_CPU_FEATURES.$$QT_ARCH, neon) {
    SOURCES += image/qimage_neon.cpp
}
contains(QT_CPU_FEATURES.$$QT_ARCH, sse2) {
    SOURCES += image/qimage_sse2.cpp
    SSSE3_SOURCES += image/qimage_ssse3.cpp
}
MIPS_DSPR2_SOURCES += image/qimage_mips_dspr2.cpp
MIPS_DSPR2_ASM += image/qimage_mips_dspr2_asm.S
