# Qt kernel module

HEADERS += \
	text/qfont.h \
	text/qfontdatabase.h \
	text/qfontengine_p.h \
	text/qfontengineglyphcache_p.h \
	text/qfontinfo.h \
	text/qfontmetrics.h \
	text/qfont_p.h \
	text/qfontsubset_p.h \
	text/qtextcontrol_p.h \
	text/qtextcontrol_p_p.h \
	text/qtextengine_p.h \
	text/qtextlayout.h \
	text/qtextformat.h \
	text/qtextformat_p.h \
	text/qtextobject.h \
	text/qtextobject_p.h \
	text/qtextoption.h \
	text/qfragmentmap_p.h \
	text/qtextdocument.h \
	text/qtextdocument_p.h \
	text/qtexthtmlparser_p.h \
	text/qabstracttextdocumentlayout.h \
	text/qtextdocumentlayout_p.h \
	text/qtextcursor.h \
        text/qtextcursor_p.h \
	text/qtextdocumentfragment.h \
	text/qtextdocumentfragment_p.h \
	text/qtextimagehandler_p.h \
	text/qtexttable.h \
	text/qtextlist.h \
	text/qsyntaxhighlighter.h \
	text/qtextdocumentwriter.h \
	text/qcssparser_p.h \
	text/qtexttable_p.h \
	text/qzipreader_p.h \
	text/qzipwriter_p.h \
	text/qtextodfwriter_p.h \
	text/qstatictext_p.h \
	text/qstatictext.h \
        text/qrawfont.h \
        text/qrawfont_p.h \
    text/qglyphrun.h \
    text/qglyphrun_p.h

SOURCES += \
	text/qfont.cpp \
	text/qfontengine.cpp \
	text/qfontsubset.cpp \
	text/qfontmetrics.cpp \
	text/qfontdatabase.cpp \
	text/qtextcontrol.cpp \
	text/qtextengine.cpp \
	text/qtextlayout.cpp \
	text/qtextformat.cpp \
	text/qtextobject.cpp \
	text/qtextoption.cpp \
	text/qfragmentmap.cpp \
	text/qtextdocument.cpp \
	text/qtextdocument_p.cpp \
	text/qtexthtmlparser.cpp \
	text/qabstracttextdocumentlayout.cpp \
	text/qtextdocumentlayout.cpp \
	text/qtextcursor.cpp \
	text/qtextdocumentfragment.cpp \
	text/qtextimagehandler.cpp \
	text/qtexttable.cpp \
	text/qtextlist.cpp \
	text/qtextdocumentwriter.cpp \
	text/qsyntaxhighlighter.cpp \
	text/qcssparser.cpp \
	text/qzip.cpp \
	text/qtextodfwriter.cpp \
	text/qstatictext.cpp \
        text/qrawfont.cpp \
    text/qglyphrun.cpp

win32 {
	SOURCES += \
		text/qfont_win.cpp \
                text/qfontengine_win.cpp \
                text/qrawfont_win.cpp
	HEADERS += text/qfontengine_win_p.h
}

contains(QT_CONFIG, directwrite) {
    LIBS_PRIVATE += -ldwrite
    HEADERS += text/qfontenginedirectwrite_p.h
    SOURCES += text/qfontenginedirectwrite.cpp
}

unix:x11 {
	HEADERS += \
		text/qfontengine_x11_p.h \
		text/qfontdatabase_x11.cpp \
		text/qfontengine_ft_p.h
	SOURCES += \
		text/qfont_x11.cpp \
		text/qfontengine_x11.cpp \
                text/qfontengine_ft.cpp \
                text/qrawfont_ft.cpp
}

!embedded:!qpa:!x11:mac {
        HEADERS += \
                text/qfontengine_mac_p.h
	SOURCES += \
                text/qfont_mac.cpp \
                text/qrawfont_mac.cpp
        OBJECTIVE_SOURCES += \
                text/qfontengine_mac.mm
}
!embedded:!x11:mac {
        OBJECTIVE_HEADERS += \
                text/qfontengine_coretext_p.h
        OBJECTIVE_SOURCES += \
                text/qfontengine_coretext.mm
        contains(QT_CONFIG, harfbuzz) {
            DEFINES += QT_ENABLE_HARFBUZZ_FOR_MAC
        }
}

embedded {
	SOURCES += \
		text/qfont_qws.cpp \
		text/qfontengine_qws.cpp \
		text/qfontengine_ft.cpp \
		text/qfontengine_qpf.cpp \
                text/qabstractfontengine_qws.cpp \
                text/qrawfont_ft.cpp
	HEADERS += \
		text/qfontengine_ft_p.h \
		text/qfontengine_qpf_p.h \
		text/qabstractfontengine_qws.h \
		text/qabstractfontengine_p.h
	DEFINES += QT_NO_FONTCONFIG
}

qpa {
	SOURCES += \
                text/qfont_qpa.cpp \
                text/qfontengine_qpa.cpp \
                text/qplatformfontdatabase_qpa.cpp \
                text/qrawfont_qpa.cpp

	HEADERS += \
                text/qplatformfontdatabase_qpa.h

	DEFINES += QT_NO_FONTCONFIG
        DEFINES += QT_NO_FREETYPE
}

symbian {
	SOURCES += \
		text/qfont_s60.cpp
	contains(QT_CONFIG, freetype) {
		SOURCES += \
                        text/qfontengine_ft.cpp \
                        text/qrawfont_ft.cpp
		HEADERS += \
			text/qfontengine_ft_p.h
		DEFINES += \
			QT_NO_FONTCONFIG
	} else {
		SOURCES += \
			text/qfontengine_s60.cpp
		HEADERS += \
			text/qfontengine_s60_p.h
	}
	LIBS += -lfntstr -lecom
}

!qpa {
contains(QT_CONFIG, freetype) {
    SOURCES += \
	../3rdparty/freetype/src/base/ftbase.c \
	../3rdparty/freetype/src/base/ftbbox.c \
	../3rdparty/freetype/src/base/ftdebug.c \
	../3rdparty/freetype/src/base/ftglyph.c \
	../3rdparty/freetype/src/base/ftinit.c \
	../3rdparty/freetype/src/base/ftmm.c \
	../3rdparty/freetype/src/base/fttype1.c \
	../3rdparty/freetype/src/base/ftsynth.c \
	../3rdparty/freetype/src/base/ftbitmap.c \
	../3rdparty/freetype/src/bdf/bdf.c \
	../3rdparty/freetype/src/cache/ftcache.c \
	../3rdparty/freetype/src/cff/cff.c \
	../3rdparty/freetype/src/cid/type1cid.c \
	../3rdparty/freetype/src/gzip/ftgzip.c \
	../3rdparty/freetype/src/pcf/pcf.c \
	../3rdparty/freetype/src/pfr/pfr.c \
	../3rdparty/freetype/src/psaux/psaux.c \
	../3rdparty/freetype/src/pshinter/pshinter.c \
	../3rdparty/freetype/src/psnames/psmodule.c \
	../3rdparty/freetype/src/raster/raster.c \
	../3rdparty/freetype/src/sfnt/sfnt.c \
	../3rdparty/freetype/src/smooth/smooth.c \
	../3rdparty/freetype/src/truetype/truetype.c \
	../3rdparty/freetype/src/type1/type1.c \
	../3rdparty/freetype/src/type42/type42.c \
	../3rdparty/freetype/src/winfonts/winfnt.c \
	../3rdparty/freetype/src/lzw/ftlzw.c\
          ../3rdparty/freetype/src/otvalid/otvalid.c\
          ../3rdparty/freetype/src/otvalid/otvbase.c\
          ../3rdparty/freetype/src/otvalid/otvgdef.c\
          ../3rdparty/freetype/src/otvalid/otvjstf.c\
          ../3rdparty/freetype/src/otvalid/otvcommn.c\
          ../3rdparty/freetype/src/otvalid/otvgpos.c\
          ../3rdparty/freetype/src/otvalid/otvgsub.c\
          ../3rdparty/freetype/src/otvalid/otvmod.c\
          ../3rdparty/freetype/src/autofit/afangles.c\
          ../3rdparty/freetype/src/autofit/afglobal.c\
          ../3rdparty/freetype/src/autofit/aflatin.c\
          ../3rdparty/freetype/src/autofit/afmodule.c\
          ../3rdparty/freetype/src/autofit/afdummy.c\
          ../3rdparty/freetype/src/autofit/afhints.c\
          ../3rdparty/freetype/src/autofit/afloader.c\
          ../3rdparty/freetype/src/autofit/autofit.c

    symbian {
        SOURCES += \
            ../3rdparty/freetype/src/base/ftsystem.c
    } else {
        SOURCES += \
            ../3rdparty/freetype/builds/unix/ftsystem.c
        INCLUDEPATH += \
            ../3rdparty/freetype/builds/unix
    }

    INCLUDEPATH += \
	../3rdparty/freetype/src \
	../3rdparty/freetype/include

    DEFINES += FT2_BUILD_LIBRARY FT_CONFIG_OPTION_SYSTEM_ZLIB
    
    embedded:CONFIG += opentype
} else:contains(QT_CONFIG, system-freetype) {
    embedded:CONFIG += opentype
    # pull in the proper freetype2 include directory
    include($$QT_SOURCE_TREE/config.tests/unix/freetype/freetype.pri)
    LIBS_PRIVATE += -lfreetype
}

contains(QT_CONFIG, fontconfig) {
    CONFIG += opentype
}
}#!qpa

DEFINES += QT_NO_OPENTYPE
INCLUDEPATH += ../3rdparty/harfbuzz/src
