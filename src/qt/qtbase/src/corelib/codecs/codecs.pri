# Qt core library codecs module

HEADERS += \
    codecs/qisciicodec_p.h \
    codecs/qlatincodec_p.h \
    codecs/qtextcodec_p.h \
    codecs/qtextcodec.h \
    codecs/qtsciicodec_p.h \
    codecs/qutfcodec_p.h

SOURCES += \
    codecs/qisciicodec.cpp \
    codecs/qlatincodec.cpp \
    codecs/qtextcodec.cpp \
    codecs/qtsciicodec.cpp \
    codecs/qutfcodec.cpp

contains(QT_CONFIG,icu) {
    HEADERS += \
        codecs/qicucodec_p.h
    SOURCES += \
        codecs/qicucodec.cpp
} else {
    HEADERS += \
        codecs/qsimplecodec_p.h \
        codecs/qgb18030codec_p.h \
        codecs/qeucjpcodec_p.h \
        codecs/qjiscodec_p.h \
        codecs/qsjiscodec_p.h \
        codecs/qeuckrcodec_p.h \
        codecs/qbig5codec_p.h

    SOURCES += \
        codecs/qsimplecodec.cpp \
        codecs/qgb18030codec.cpp \
        codecs/qjpunicode.cpp \
        codecs/qeucjpcodec.cpp \
        codecs/qjiscodec.cpp \
        codecs/qsjiscodec.cpp \
        codecs/qeuckrcodec.cpp \
        codecs/qbig5codec.cpp

    unix:!qnx:!mac:!ios:!linux-android-* {
            contains(QT_CONFIG,iconv) {
                    HEADERS += codecs/qiconvcodec_p.h
                    SOURCES += codecs/qiconvcodec.cpp
            } else:contains(QT_CONFIG,gnu-libiconv) {
                    HEADERS += codecs/qiconvcodec_p.h
                    SOURCES += codecs/qiconvcodec.cpp
                    DEFINES += GNU_LIBICONV
                    LIBS_PRIVATE *= -liconv
            } else:contains(QT_CONFIG,sun-libiconv) {
                    HEADERS += codecs/qiconvcodec_p.h
                    SOURCES += codecs/qiconvcodec.cpp
                    DEFINES += GNU_LIBICONV
            }
    } else:!win32-msvc* {
        DEFINES += QT_NO_ICONV
    }
    win32 {
        SOURCES += codecs/qwindowscodec.cpp
        HEADERS += codecs/qwindowscodec_p.h
    }
}
