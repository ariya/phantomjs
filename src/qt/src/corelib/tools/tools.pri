# Qt tools module

HEADERS +=  \
        tools/qalgorithms.h \
        tools/qbitarray.h \
        tools/qbytearray.h \
        tools/qbytearraymatcher.h \
        tools/qbytedata_p.h \
        tools/qcache.h \
        tools/qchar.h \
        tools/qcontainerfwd.h \
        tools/qcryptographichash.h \
        tools/qdatetime.h \
        tools/qdatetime_p.h \
        tools/qeasingcurve.h \
        tools/qhash.h \
        tools/qline.h \
        tools/qlinkedlist.h \
        tools/qlist.h \
        tools/qlocale.h \
        tools/qlocale_p.h \
        tools/qlocale_tools_p.h \
        tools/qlocale_data_p.h \
        tools/qmap.h \
        tools/qmargins.h \
        tools/qcontiguouscache.h \
        tools/qpodlist_p.h \
        tools/qpoint.h \
        tools/qqueue.h \
        tools/qrect.h \
        tools/qregexp.h \
        tools/qringbuffer_p.h \
        tools/qscopedpointer.h \
        tools/qscopedpointer_p.h \
        tools/qscopedvaluerollback.h \
        tools/qshareddata.h \
        tools/qsharedpointer.h \
        tools/qsharedpointer_impl.h \
        tools/qset.h \
        tools/qsimd_p.h \
        tools/qsize.h \
        tools/qstack.h \
        tools/qstring.h \
        tools/qstringbuilder.h \
        tools/qstringlist.h \
        tools/qstringmatcher.h \
        tools/qtextboundaryfinder.h \
        tools/qtimeline.h \
        tools/qelapsedtimer.h \
        tools/qunicodetables_p.h \
        tools/qvarlengtharray.h \
        tools/qvector.h


SOURCES += \
        tools/qbitarray.cpp \
        tools/qbytearray.cpp \
        tools/qbytearraymatcher.cpp \
        tools/qcryptographichash.cpp \
        tools/qdatetime.cpp \
        tools/qeasingcurve.cpp \
        tools/qelapsedtimer.cpp \
        tools/qhash.cpp \
        tools/qline.cpp \
        tools/qlinkedlist.cpp \
        tools/qlist.cpp \
        tools/qlocale.cpp \
        tools/qlocale_tools.cpp \
        tools/qpoint.cpp \
        tools/qmap.cpp \
        tools/qmargins.cpp \
        tools/qcontiguouscache.cpp \
        tools/qrect.cpp \
        tools/qregexp.cpp \
        tools/qshareddata.cpp \
        tools/qsharedpointer.cpp \
        tools/qsimd.cpp \
        tools/qsize.cpp \
        tools/qstring.cpp \
        tools/qstringbuilder.cpp \
        tools/qstringlist.cpp \
        tools/qtextboundaryfinder.cpp \
        tools/qtimeline.cpp \
        tools/qvector.cpp \
        tools/qvsnprintf.cpp

!nacl:mac: {
    SOURCES += tools/qelapsedtimer_mac.cpp
    OBJECTIVE_SOURCES += tools/qlocale_mac.mm
}
else:blackberry {
    SOURCES += tools/qelapsedtimer_unix.cpp tools/qlocale_blackberry.cpp
    HEADERS += tools/qlocale_blackberry.h
}
else:symbian:SOURCES += tools/qelapsedtimer_symbian.cpp tools/qlocale_symbian.cpp
else:unix:SOURCES += tools/qelapsedtimer_unix.cpp tools/qlocale_unix.cpp
else:win32:SOURCES += tools/qelapsedtimer_win.cpp tools/qlocale_win.cpp
else:integrity:SOURCES += tools/qelapsedtimer_unix.cpp tools/qlocale_unix.cpp
else:SOURCES += tools/qelapsedtimer_generic.cpp

contains(QT_CONFIG, zlib):include($$PWD/../../3rdparty/zlib.pri)
else:include($$PWD/../../3rdparty/zlib_dependency.pri)

contains(QT_CONFIG,icu) {
    SOURCES += tools/qlocale_icu.cpp
    DEFINES += QT_USE_ICU
}

DEFINES += HB_EXPORT=Q_CORE_EXPORT
INCLUDEPATH += ../3rdparty/harfbuzz/src
HEADERS += ../3rdparty/harfbuzz/src/harfbuzz.h
SOURCES += ../3rdparty/harfbuzz/src/harfbuzz-buffer.c \
           ../3rdparty/harfbuzz/src/harfbuzz-gdef.c \
           ../3rdparty/harfbuzz/src/harfbuzz-gsub.c \
           ../3rdparty/harfbuzz/src/harfbuzz-gpos.c \
           ../3rdparty/harfbuzz/src/harfbuzz-impl.c \
           ../3rdparty/harfbuzz/src/harfbuzz-open.c \
           ../3rdparty/harfbuzz/src/harfbuzz-stream.c \
           ../3rdparty/harfbuzz/src/harfbuzz-shaper-all.cpp \
           tools/qharfbuzz.cpp
HEADERS += tools/qharfbuzz_p.h

INCLUDEPATH += ../3rdparty/md5 \
               ../3rdparty/md4

# Note: libm should be present by default becaue this is C++
!macx-icc:!vxworks:!symbian:unix:LIBS_PRIVATE += -lm

symbian {
    # QLocale Symbian implementation needs this
    LIBS += -lnumberconversion
}

