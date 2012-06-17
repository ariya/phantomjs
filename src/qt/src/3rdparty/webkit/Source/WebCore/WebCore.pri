# Include file for WebCore

include(../common.pri)
include(features.pri)

# Uncomment this to enable Texture Mapper.
# CONFIG += texmap

QT *= network

SOURCE_DIR = $$replace(PWD, /WebCore, "")

contains(QT_CONFIG, qpa)|contains(QT_CONFIG, embedded): CONFIG += embedded

# Use a config-specific target to prevent parallel builds file clashes on Mac
mac: CONFIG(debug, debug|release): WEBCORE_TARGET = webcored
else: WEBCORE_TARGET = webcore

# Output in WebCore/<config>
CONFIG(debug, debug|release) : WEBCORE_DESTDIR = debug
else: WEBCORE_DESTDIR = release

CONFIG(standalone_package) {
    isEmpty(WC_GENERATED_SOURCES_DIR):WC_GENERATED_SOURCES_DIR = $$PWD/../WebCore/generated
} else {
    isEmpty(WC_GENERATED_SOURCES_DIR):WC_GENERATED_SOURCES_DIR = $$OUTPUT_DIR/WebCore/generated
}

V8_DIR = "$$[QT_INSTALL_PREFIX]/src/3rdparty/v8"

v8:!exists($${V8_DIR}/include/v8.h) {
    error("Cannot build with V8. Needed file $${V8_DIR}/include/v8.h does not exist.")
}

v8 {
    message(Using V8 with QtScript)
    QT += script
    INCLUDEPATH += $${V8_DIR}/include
    DEFINES *= V8_BINDING=1
    DEFINES += WTF_CHANGES=1
    DEFINES *= WTF_USE_V8=1
    DEFINES += USING_V8_SHARED
    linux-*:LIBS += -lv8
}

v8 {
    WEBCORE_INCLUDEPATH = \
        $$SOURCE_DIR/WebCore/bindings/v8 \
        $$SOURCE_DIR/WebCore/bindings/v8/custom \
        $$SOURCE_DIR/WebCore/bindings/v8/specialization \
        $$SOURCE_DIR/WebCore/bridge/qt/v8 \
        $$SOURCE_DIR/WebCore/testing/v8

} else {
    WEBCORE_INCLUDEPATH = \
        $$SOURCE_DIR/WebCore/bridge/jsc \
        $$SOURCE_DIR/WebCore/bindings/js \
        $$SOURCE_DIR/WebCore/bindings/js/specialization \
        $$SOURCE_DIR/WebCore/bridge/c \
        $$SOURCE_DIR/WebCore/testing/js
}

WEBCORE_INCLUDEPATH = \
    $$SOURCE_DIR/WebCore \
    $$SOURCE_DIR/WebCore/accessibility \
    $$SOURCE_DIR/WebCore/bindings \
    $$SOURCE_DIR/WebCore/bindings/generic \
    $$SOURCE_DIR/WebCore/bridge \
    $$SOURCE_DIR/WebCore/css \
    $$SOURCE_DIR/WebCore/dom \
    $$SOURCE_DIR/WebCore/dom/default \
    $$SOURCE_DIR/WebCore/editing \
    $$SOURCE_DIR/WebCore/fileapi \
    $$SOURCE_DIR/WebCore/history \
    $$SOURCE_DIR/WebCore/html \
    $$SOURCE_DIR/WebCore/html/canvas \
    $$SOURCE_DIR/WebCore/html/parser \
    $$SOURCE_DIR/WebCore/html/shadow \
    $$SOURCE_DIR/WebCore/inspector \
    $$SOURCE_DIR/WebCore/loader \
    $$SOURCE_DIR/WebCore/loader/appcache \
    $$SOURCE_DIR/WebCore/loader/archive \
    $$SOURCE_DIR/WebCore/loader/cache \
    $$SOURCE_DIR/WebCore/loader/icon \
    $$SOURCE_DIR/WebCore/mathml \
    $$SOURCE_DIR/WebCore/notifications \
    $$SOURCE_DIR/WebCore/page \
    $$SOURCE_DIR/WebCore/page/animation \
    $$SOURCE_DIR/WebCore/platform \
    $$SOURCE_DIR/WebCore/platform/animation \
    $$SOURCE_DIR/WebCore/platform/audio \
    $$SOURCE_DIR/WebCore/platform/graphics \
    $$SOURCE_DIR/WebCore/platform/graphics/filters \
    $$SOURCE_DIR/WebCore/platform/graphics/filters/arm \
    $$SOURCE_DIR/WebCore/platform/graphics/texmap \
    $$SOURCE_DIR/WebCore/platform/graphics/transforms \
    $$SOURCE_DIR/WebCore/platform/image-decoders \
    $$SOURCE_DIR/WebCore/platform/leveldb \
    $$SOURCE_DIR/WebCore/platform/mock \
    $$SOURCE_DIR/WebCore/platform/network \
    $$SOURCE_DIR/WebCore/platform/sql \
    $$SOURCE_DIR/WebCore/platform/text \
    $$SOURCE_DIR/WebCore/platform/text/transcoder \
    $$SOURCE_DIR/WebCore/plugins \
    $$SOURCE_DIR/WebCore/rendering \
    $$SOURCE_DIR/WebCore/rendering/mathml \
    $$SOURCE_DIR/WebCore/rendering/style \
    $$SOURCE_DIR/WebCore/rendering/svg \
    $$SOURCE_DIR/WebCore/storage \
    $$SOURCE_DIR/WebCore/svg \
    $$SOURCE_DIR/WebCore/svg/animation \
    $$SOURCE_DIR/WebCore/svg/graphics \
    $$SOURCE_DIR/WebCore/svg/graphics/filters \
    $$SOURCE_DIR/WebCore/svg/properties \
    $$SOURCE_DIR/WebCore/testing \
    $$SOURCE_DIR/WebCore/webaudio \
    $$SOURCE_DIR/WebCore/websockets \
    $$SOURCE_DIR/WebCore/wml \
    $$SOURCE_DIR/WebCore/workers \
    $$SOURCE_DIR/WebCore/xml \
    $$WEBCORE_INCLUDEPATH

WEBCORE_INCLUDEPATH = \
    $$SOURCE_DIR/WebCore/bridge/qt \
    $$SOURCE_DIR/WebCore/page/qt \
    $$SOURCE_DIR/WebCore/platform/graphics/qt \
    $$SOURCE_DIR/WebCore/platform/network/qt \
    $$SOURCE_DIR/WebCore/platform/qt \
    $$SOURCE_DIR/WebKit/qt/Api \
    $$SOURCE_DIR/WebKit/qt/WebCoreSupport \
    $$WEBCORE_INCLUDEPATH

# On Symbian PREPEND_INCLUDEPATH is the best way to make sure that WebKit headers
# are included before platform headers.
symbian {
    PREPEND_INCLUDEPATH = $$WEBCORE_INCLUDEPATH $$WC_GENERATED_SOURCES_DIR $$PREPEND_INCLUDEPATH
} else {
    INCLUDEPATH = $$WEBCORE_INCLUDEPATH $$WC_GENERATED_SOURCES_DIR $$INCLUDEPATH
}

symbian {
    v8 {
        QMAKE_CXXFLAGS.ARMCC += -OTime -O3
        QMAKE_CXXFLAGS.ARMCC += --fpu softvfp+vfpv2 --fpmode fast
        LIBS += -llibpthread
    }

    # RO text (code) section in qtwebkit.dll exceeds allocated space for gcce udeb target.
    # Move RW-section base address to start from 0x1000000 instead of the toolchain default 0x400000.
    QMAKE_LFLAGS.ARMCC += --rw-base 0x1000000
    QMAKE_LFLAGS.GCCE += -Tdata 0x1000000

    CONFIG += do_not_build_as_thumb

    CONFIG(release, debug|release): QMAKE_CXXFLAGS.ARMCC += -OTime -O3
    # Symbian plugin support
    LIBS += -lefsrv

    !CONFIG(QTDIR_build) {
        # Test if symbian OS comes with sqlite
        exists($${EPOCROOT}epoc32/release/armv5/lib/sqlite3.dso):CONFIG *= system-sqlite
    } else:!symbian-abld:!symbian-sbsv2 {
        # When bundled with Qt, all Symbian build systems extract their own sqlite files if
        # necessary, but on non-mmp based ones we need to specify this ourselves.
        include($$QT_SOURCE_TREE/src/plugins/sqldrivers/sqlite_symbian/sqlite_symbian.pri)
    }
}

contains(DEFINES, ENABLE_XSLT=1) {
    QT *= xmlpatterns
}

contains(DEFINES, ENABLE_SQLITE=1) {
    !system-sqlite:exists( $${SQLITE3SRCDIR}/sqlite3.c ) {
            INCLUDEPATH += $${SQLITE3SRCDIR}
            DEFINES += SQLITE_CORE SQLITE_OMIT_LOAD_EXTENSION SQLITE_OMIT_COMPLETE
            CONFIG(release, debug|release): DEFINES *= NDEBUG
            contains(DEFINES, ENABLE_SINGLE_THREADED=1): DEFINES += SQLITE_THREADSAFE=0
    } else {
        # Use sqlite3 from the underlying OS
        CONFIG(QTDIR_build) {
            QMAKE_CXXFLAGS *= $$QT_CFLAGS_SQLITE
            LIBS *= $$QT_LFLAGS_SQLITE
        } else {
            INCLUDEPATH += $${SQLITE3SRCDIR}
            LIBS += -lsqlite3
        }
    }
    wince*:DEFINES += HAVE_LOCALTIME_S=0
}

contains(DEFINES, ENABLE_NETSCAPE_PLUGIN_API=1) {
    unix:!symbian {
        mac {
            INCLUDEPATH += platform/mac
            # Note: XP_MACOSX is defined in npapi.h
        } else {
            !embedded {
                CONFIG += x11
                LIBS += -lXrender
            }
            maemo5 {
                DEFINES += MOZ_PLATFORM_MAEMO=5
            }
            contains(DEFINES, Q_WS_MAEMO_6) {
                DEFINES += MOZ_PLATFORM_MAEMO=6
            }
            DEFINES += XP_UNIX
            DEFINES += ENABLE_NETSCAPE_PLUGIN_METADATA_CACHE=1
        }
    }
    win32-* {
        LIBS += \
            -ladvapi32 \
            -lgdi32 \
            -lshell32 \
            -lshlwapi \
            -luser32 \
            -lversion
    }
}

contains(DEFINES, ENABLE_GEOLOCATION=1) {
    CONFIG *= mobility
    MOBILITY *= location
}

contains(DEFINES, ENABLE_DEVICE_ORIENTATION=1) {
    CONFIG *= mobility
    MOBILITY *= sensors
}

contains(DEFINES, WTF_USE_QT_MOBILITY_SYSTEMINFO=1) {
     CONFIG *= mobility
     MOBILITY *= systeminfo
}

contains(DEFINES, WTF_USE_QT_BEARER=1) {
    # Bearer management is part of Qt 4.7, so don't accidentially
    # pull in Qt Mobility when building against >= 4.7
    !greaterThan(QT_MINOR_VERSION, 6) {
        CONFIG *= mobility
        MOBILITY *= bearer
    }
}

contains(DEFINES, ENABLE_VIDEO=1) {
    contains(DEFINES, WTF_USE_QTKIT=1) {
        INCLUDEPATH += $$PWD/platform/graphics/mac

        LIBS += -framework AppKit -framework AudioUnit \
                -framework AudioToolbox -framework CoreAudio \
                -framework QuartzCore -framework QTKit

    } else:contains(DEFINES, WTF_USE_GSTREAMER=1) {
        DEFINES += ENABLE_GLIB_SUPPORT=1

        INCLUDEPATH += $$PWD/platform/graphics/gstreamer

        PKGCONFIG += glib-2.0 gio-2.0 gstreamer-0.10 gstreamer-app-0.10 gstreamer-base-0.10 gstreamer-interfaces-0.10 gstreamer-pbutils-0.10 gstreamer-plugins-base-0.10 gstreamer-video-0.10
    } else:contains(DEFINES, WTF_USE_QT_MULTIMEDIA=1) {
        CONFIG   *= mobility
        MOBILITY *= multimedia
    }
}

contains(DEFINES, ENABLE_WEBGL=1)|contains(CONFIG, texmap) {
    !contains(QT_CONFIG, opengl) {
        error( "This configuration needs an OpenGL enabled Qt. Your Qt is missing OpenGL.")
    }
    QT *= opengl
}

!CONFIG(webkit-debug):CONFIG(QTDIR_build) {
    # Remove the following 2 lines if you want debug information in WebCore
    CONFIG -= separate_debug_info
    CONFIG += no_debug_info
}

contains (CONFIG, text_breaking_with_icu) {
    LIBS += -licuuc
}

!CONFIG(QTDIR_build) {
    win32-*|wince* {
        DLLDESTDIR = $$OUTPUT_DIR/bin
        isEmpty(QT_SOURCE_TREE):build_pass: TARGET = $$qtLibraryTarget($$TARGET)

        dlltarget.commands = $(COPY_FILE) $(DESTDIR_TARGET) $$[QT_INSTALL_BINS]
        dlltarget.CONFIG = no_path
        INSTALLS += dlltarget
    }
    mac {
        LIBS += -framework Carbon -framework AppKit
    }
}

win32-* {
    INCLUDEPATH += $$SOURCE_DIR/WebCore/platform/win
    LIBS += -lgdi32
    LIBS += -lole32
    LIBS += -luser32
}

# Remove whole program optimizations due to miscompilations
win32-msvc2005|win32-msvc2008|win32-msvc2010|wince*:{
    QMAKE_CFLAGS_LTCG -= -GL
    QMAKE_CXXFLAGS_LTCG -= -GL

    # Disable incremental linking for windows 32bit OS debug build as WebKit is so big
    # that linker failes to link incrementally in debug mode.
    ARCH = $$(PROCESSOR_ARCHITECTURE)
    WOW64ARCH = $$(PROCESSOR_ARCHITEW6432)
    equals(ARCH, x86):{
        isEmpty(WOW64ARCH): QMAKE_LFLAGS_DEBUG += /INCREMENTAL:NO
    }
}

wince* {
    LIBS += -lmmtimer
    LIBS += -lole32
}

mac {
    LIBS_PRIVATE += -framework Carbon -framework AppKit
}

unix:!mac:*-g++*:QMAKE_CXXFLAGS += -ffunction-sections -fdata-sections
unix:!mac:*-g++*:QMAKE_LFLAGS += -Wl,--gc-sections
linux*-g++*:QMAKE_LFLAGS += $$QMAKE_LFLAGS_NOUNDEF

unix|win32-g++*:QMAKE_PKGCONFIG_REQUIRES = QtCore QtGui QtNetwork
unix:!mac:!symbian:CONFIG += link_pkgconfig

# Disable C++0x mode in WebCore for those who enabled it in their Qt's mkspec
*-g++*:QMAKE_CXXFLAGS -= -std=c++0x -std=gnu++0x

enable_fast_mobile_scrolling: DEFINES += ENABLE_FAST_MOBILE_SCROLLING=1

use_qt_mobile_theme: DEFINES += WTF_USE_QT_MOBILE_THEME=1

defineTest(prependWebCoreLib) {
    pathToWebCoreOutput = $$ARGS/$$WEBCORE_DESTDIR

    win32-msvc*|wince*|win32-icc {
        LIBS = -l$$WEBCORE_TARGET $$LIBS
        LIBS = -L$$pathToWebCoreOutput $$LIBS
        POST_TARGETDEPS += $${pathToWebCoreOutput}$${QMAKE_DIR_SEP}$${WEBCORE_TARGET}.lib
    } else:symbian {
        LIBS = -l$${WEBCORE_TARGET}.lib $$LIBS
        QMAKE_LIBDIR += $$pathToWebCoreOutput
        POST_TARGETDEPS += $${pathToWebCoreOutput}$${QMAKE_DIR_SEP}$${WEBCORE_TARGET}.lib
    } else {
        QMAKE_LIBDIR = $$pathToWebCoreOutput $$QMAKE_LIBDIR
        LIBS = -l$$WEBCORE_TARGET $$LIBS
        POST_TARGETDEPS += $${pathToWebCoreOutput}$${QMAKE_DIR_SEP}lib$${WEBCORE_TARGET}.a
    }
    
    CONFIG -= explicitlib
    CONFIG -= staticlib
    export(QMAKE_LIBDIR)
    export(POST_TARGETDEPS)
    export(CONFIG)
    export(LIBS)

    return(true)
}
