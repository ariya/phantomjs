# JavaScriptCore - Qt4 build info

include(../common.pri)

VPATH += $$PWD

# Use a config-specific target to prevent parallel builds file clashes on Mac
mac: CONFIG(debug, debug|release): JAVASCRIPTCORE_TARGET = jscored
else: JAVASCRIPTCORE_TARGET = jscore

# Output in JavaScriptCore/<config>
CONFIG(debug, debug|release) : JAVASCRIPTCORE_DESTDIR = debug
else: JAVASCRIPTCORE_DESTDIR = release

CONFIG(standalone_package) {
    isEmpty(JSC_GENERATED_SOURCES_DIR):JSC_GENERATED_SOURCES_DIR = $$PWD/generated
} else {
    isEmpty(JSC_GENERATED_SOURCES_DIR):JSC_GENERATED_SOURCES_DIR = $$OUTPUT_DIR/JavaScriptCore/generated
}

JAVASCRIPTCORE_INCLUDEPATH = \
    $$PWD \
    $$PWD/.. \
    $$PWD/../ThirdParty \
    $$PWD/assembler \
    $$PWD/bytecode \
    $$PWD/bytecompiler \
    $$PWD/heap \
    $$PWD/dfg \
    $$PWD/debugger \
    $$PWD/interpreter \
    $$PWD/jit \
    $$PWD/parser \
    $$PWD/profiler \
    $$PWD/runtime \
    $$PWD/wtf \
    $$PWD/wtf/gobject \
    $$PWD/wtf/symbian \
    $$PWD/wtf/unicode \
    $$PWD/yarr \
    $$PWD/API \
    $$PWD/ForwardingHeaders \
    $$JSC_GENERATED_SOURCES_DIR

symbian {
    PREPEND_INCLUDEPATH = $$JAVASCRIPTCORE_INCLUDEPATH $$PREPEND_INCLUDEPATH
} else {
    INCLUDEPATH = $$JAVASCRIPTCORE_INCLUDEPATH $$INCLUDEPATH
}

symbian {
    LIBS += -lhal
    INCLUDEPATH *= $$MW_LAYER_SYSTEMINCLUDE
}

win32-*: DEFINES += _HAS_TR1=0

DEFINES += BUILDING_JavaScriptCore BUILDING_WTF

# CONFIG += text_breaking_with_icu

contains (CONFIG, text_breaking_with_icu) {
    DEFINES += WTF_USE_QT_ICU_TEXT_BREAKING=1
}

wince* {
    INCLUDEPATH += $$QT_SOURCE_TREE/src/3rdparty/ce-compat
    INCLUDEPATH += $$PWD/../JavaScriptCore/os-win32
}


defineTest(prependJavaScriptCoreLib) {
    # Argument is the relative path to JavaScriptCore.pro's qmake output
    pathToJavaScriptCoreOutput = $$ARGS/$$JAVASCRIPTCORE_DESTDIR

    win32-msvc*|wince*|win32-icc {
        LIBS = -l$$JAVASCRIPTCORE_TARGET $$LIBS
        LIBS = -L$$pathToJavaScriptCoreOutput $$LIBS
        POST_TARGETDEPS += $${pathToJavaScriptCoreOutput}$${QMAKE_DIR_SEP}$${JAVASCRIPTCORE_TARGET}.lib
    } else:symbian {
        LIBS = -l$${JAVASCRIPTCORE_TARGET}.lib $$LIBS
        # The default symbian build system does not use library paths at all. However when building with
        # qmake's symbian makespec that uses Makefiles
        QMAKE_LIBDIR += $$pathToJavaScriptCoreOutput
        POST_TARGETDEPS += $${pathToJavaScriptCoreOutput}$${QMAKE_DIR_SEP}$${JAVASCRIPTCORE_TARGET}.lib
    } else {
        # Make sure jscore will be early in the list of libraries to workaround a bug in MinGW
        # that can't resolve symbols from QtCore if libjscore comes after.
        QMAKE_LIBDIR = $$pathToJavaScriptCoreOutput $$QMAKE_LIBDIR
        LIBS = -l$$JAVASCRIPTCORE_TARGET $$LIBS
        POST_TARGETDEPS += $${pathToJavaScriptCoreOutput}$${QMAKE_DIR_SEP}lib$${JAVASCRIPTCORE_TARGET}.a
    }

    win32-* {
        LIBS += -lwinmm
    }

    # The following line is to prevent qmake from adding jscore to libQtWebKit's prl dependencies.
    # The compromise we have to accept by disabling explicitlib is to drop support to link QtWebKit and QtScript
    # statically in applications (which isn't used often because, among other things, of licensing obstacles).
    CONFIG -= explicitlib
    CONFIG -= staticlib

    export(QMAKE_LIBDIR)
    export(LIBS)
    export(POST_TARGETDEPS)
    export(CONFIG)

    return(true)
}

