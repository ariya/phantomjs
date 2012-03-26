# -*-mode:sh-*-
# Qt image handling

# Qt kernel module

HEADERS += \
        image/qbitmap.h \
        image/qicon.h \
        image/qicon_p.h \
        image/qiconloader_p.h \
        image/qiconengine.h \
        image/qiconengineplugin.h \
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
        image/qpixmapdata_p.h \
        image/qpixmapdatafactory_p.h \
        image/qpixmapfilter_p.h \
        image/qimagepixmapcleanuphooks_p.h \
        image/qvolatileimage_p.h \
        image/qvolatileimagedata_p.h \
        image/qnativeimagehandleprovider_p.h

SOURCES += \
        image/qbitmap.cpp \
        image/qicon.cpp \
        image/qiconloader.cpp \
        image/qimage.cpp \
        image/qimageiohandler.cpp \
        image/qimagereader.cpp \
        image/qimagewriter.cpp \
        image/qpaintengine_pic.cpp \
        image/qpicture.cpp \
        image/qpictureformatplugin.cpp \
        image/qpixmap.cpp \
        image/qpixmapcache.cpp \
        image/qpixmapdata.cpp \
        image/qpixmapdatafactory.cpp \
        image/qpixmapfilter.cpp \
        image/qiconengine.cpp \
        image/qiconengineplugin.cpp \
        image/qmovie.cpp \
        image/qpixmap_raster.cpp \
        image/qpixmap_blitter.cpp \
        image/qnativeimage.cpp \
        image/qimagepixmapcleanuphooks.cpp \
        image/qvolatileimage.cpp

win32 {
    SOURCES += image/qpixmap_win.cpp
}
else:embedded {
    SOURCES += image/qpixmap_qws.cpp
}
else:qpa {
    SOURCES += image/qpixmap_qpa.cpp
}
else:x11 {
    HEADERS += image/qpixmap_x11_p.h
    SOURCES += image/qpixmap_x11.cpp
}
else:mac {
    HEADERS += image/qpixmap_mac_p.h
    SOURCES += image/qpixmap_mac.cpp
}
else:symbian {
    HEADERS += image/qpixmap_raster_symbian_p.h
    SOURCES += image/qpixmap_raster_symbian.cpp
}

!symbian|contains(S60_VERSION, 3.1)|contains(S60_VERSION, 3.2) {
    SOURCES += image/qvolatileimagedata.cpp
}
else {
    SOURCES += image/qvolatileimagedata_symbian.cpp
}

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
contains(QT_CONFIG, mng):include($$PWD/qmnghandler.pri)
contains(QT_CONFIG, tiff):include($$PWD/qtiffhandler.pri)
contains(QT_CONFIG, gif):include($$PWD/qgifhandler.pri)

# SIMD
NEON_SOURCES += image/qimage_neon.cpp
SSE2_SOURCES += image/qimage_sse2.cpp
SSSE3_SOURCES += image/qimage_ssse3.cpp
