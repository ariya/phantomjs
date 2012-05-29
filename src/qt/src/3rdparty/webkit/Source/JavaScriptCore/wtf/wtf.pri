# wtf - qmake build info

SOURCES += \
    wtf/Assertions.cpp \
    wtf/ByteArray.cpp \
    wtf/CryptographicallyRandomNumber.cpp \
    wtf/CurrentTime.cpp \
    wtf/DateMath.cpp \
    wtf/dtoa.cpp \
    wtf/DecimalNumber.cpp \
    wtf/FastMalloc.cpp \
    wtf/gobject/GOwnPtr.cpp \
    wtf/gobject/GRefPtr.cpp \
    wtf/HashTable.cpp \
    wtf/MD5.cpp \
    wtf/MainThread.cpp \
    wtf/NullPtr.cpp \
    wtf/OSRandomSource.cpp \
    wtf/qt/MainThreadQt.cpp \
    wtf/qt/StringQt.cpp \
    wtf/qt/ThreadingQt.cpp \
    wtf/PageAllocationAligned.cpp \
    wtf/PageBlock.cpp \
    wtf/ParallelJobsGeneric.cpp \
    wtf/RandomNumber.cpp \
    wtf/RefCountedLeakCounter.cpp \
    wtf/SHA1.cpp \
    wtf/StackBounds.cpp \
    wtf/TCSystemAlloc.cpp \
    wtf/ThreadingNone.cpp \
    wtf/Threading.cpp \
    wtf/TypeTraits.cpp \
    wtf/WTFThreadData.cpp \
    wtf/text/AtomicString.cpp \
    wtf/text/CString.cpp \
    wtf/text/StringBuilder.cpp \
    wtf/text/StringImpl.cpp \
    wtf/text/StringStatics.cpp \
    wtf/text/WTFString.cpp \
    wtf/unicode/CollatorDefault.cpp \
    wtf/unicode/icu/CollatorICU.cpp \
    wtf/unicode/UTF8.cpp

linux-*:!contains(DEFINES, USE_QTMULTIMEDIA=1) {
    !contains(QT_CONFIG, no-pkg-config):system(pkg-config --exists glib-2.0 gio-2.0 gstreamer-0.10): {
        DEFINES += ENABLE_GLIB_SUPPORT=1
        PKGCONFIG += glib-2.0 gio-2.0
        CONFIG += link_pkgconfig
    }
}

unix:!symbian: SOURCES += wtf/OSAllocatorPosix.cpp
symbian: SOURCES += wtf/OSAllocatorSymbian.cpp
win*|wince*: SOURCES += wtf/OSAllocatorWin.cpp
