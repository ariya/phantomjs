TARGET     = QtConcurrent
QT         = core-private
CONFIG    += exceptions

DEFINES   += QT_NO_USING_NAMESPACE
win32-msvc*|win32-icc:QMAKE_LFLAGS += /BASE:0x66000000

QMAKE_DOCS = $$PWD/doc/qtconcurrent.qdocconf

load(qt_module)

PRECOMPILED_HEADER = ../corelib/global/qt_pch.h

SOURCES += \
        qtconcurrentfilter.cpp \
        qtconcurrentmap.cpp \
        qtconcurrentrun.cpp \
        qtconcurrentthreadengine.cpp \
        qtconcurrentiteratekernel.cpp \

HEADERS += \
        qtconcurrent_global.h \
        qtconcurrentcompilertest.h \
        qtconcurrentexception.h \
        qtconcurrentfilter.h \
        qtconcurrentfilterkernel.h \
        qtconcurrentfunctionwrappers.h \
        qtconcurrentiteratekernel.h \
        qtconcurrentmap.h \
        qtconcurrentmapkernel.h \
        qtconcurrentmedian.h \
        qtconcurrentreducekernel.h \
        qtconcurrentrun.h \
        qtconcurrentrunbase.h \
        qtconcurrentstoredfunctioncall.h \
        qtconcurrentthreadengine.h

# private headers
HEADERS += \

contains(QT_CONFIG, clock-gettime) {
    linux-*|hpux-*|solaris-*: LIBS_PRIVATE *= -lrt
}
