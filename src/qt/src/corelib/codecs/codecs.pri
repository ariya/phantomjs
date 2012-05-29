# Qt core library codecs module

HEADERS += \
	codecs/qisciicodec_p.h \
	codecs/qlatincodec_p.h \
	codecs/qsimplecodec_p.h \
	codecs/qtextcodec_p.h \
	codecs/qtextcodec.h \
	codecs/qtsciicodec_p.h \
	codecs/qutfcodec_p.h \
	codecs/qtextcodecplugin.h

SOURCES += \
	codecs/qisciicodec.cpp \
	codecs/qlatincodec.cpp \
	codecs/qsimplecodec.cpp \
	codecs/qtextcodec.cpp \
	codecs/qtsciicodec.cpp \
	codecs/qutfcodec.cpp \
	codecs/qtextcodecplugin.cpp

unix {
	SOURCES += codecs/qfontlaocodec.cpp

        contains(QT_CONFIG,iconv) {
                HEADERS += codecs/qiconvcodec_p.h
                SOURCES += codecs/qiconvcodec.cpp
        } else:contains(QT_CONFIG,gnu-libiconv) {
                HEADERS += codecs/qiconvcodec_p.h
                SOURCES += codecs/qiconvcodec.cpp

                DEFINES += GNU_LIBICONV
                !mac:LIBS_PRIVATE *= -liconv
        } else:contains(QT_CONFIG,sun-libiconv) {
                HEADERS += codecs/qiconvcodec_p.h
                SOURCES += codecs/qiconvcodec.cpp
                DEFINES += GNU_LIBICONV
        } else:!symbian {
                # no iconv, so we put all plugins in the library
                HEADERS += \
                        ../plugins/codecs/cn/qgb18030codec.h \
                        ../plugins/codecs/jp/qeucjpcodec.h \
                        ../plugins/codecs/jp/qjiscodec.h \
                        ../plugins/codecs/jp/qsjiscodec.h \ 
                        ../plugins/codecs/kr/qeuckrcodec.h \
                        ../plugins/codecs/tw/qbig5codec.h \
                        ../plugins/codecs/jp/qfontjpcodec.h
                SOURCES += \
                        ../plugins/codecs/cn/qgb18030codec.cpp \
                        ../plugins/codecs/jp/qjpunicode.cpp \
                        ../plugins/codecs/jp/qeucjpcodec.cpp \
                        ../plugins/codecs/jp/qjiscodec.cpp \
                        ../plugins/codecs/jp/qsjiscodec.cpp \ 
                        ../plugins/codecs/kr/qeuckrcodec.cpp \
                        ../plugins/codecs/tw/qbig5codec.cpp \
                        ../plugins/codecs/jp/qfontjpcodec.cpp
        }
}
symbian:LIBS += -lcharconv
