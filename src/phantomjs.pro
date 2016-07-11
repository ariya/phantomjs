if(!equals(QT_MAJOR_VERSION, 5)|!equals(QT_MINOR_VERSION, 6)) {
    error("This program can only be compiled with Qt 5.6.x.")
}

TEMPLATE = app
TARGET = phantomjs
QT += network webkitwidgets
CONFIG += console

DESTDIR = ../bin

RESOURCES = phantomjs.qrc \
    ghostdriver/ghostdriver.qrc

win32 {
    RESOURCES += \
     qt/qtwebkit/Source/WebCore/inspector/front-end/WebKit.qrc \
     qt/qtwebkit/Source/WebCore/generated/InspectorBackendCommands.qrc
}

HEADERS = \
    callback.h \
    childprocess.h \
    config.h \
    consts.h \
    cookiejar.h \
    crashdump.h \
    encoding.h \
    env.h \
    filesystem.h \
    networkaccessmanager.h \
    phantom.h \
    repl.h \
    system.h \
    terminal.h \
    utils.h \
    webpage.h \
    webserver.h

SOURCES = \
    callback.cpp \
    childprocess.cpp \
    config.cpp \
    cookiejar.cpp \
    crashdump.cpp \
    encoding.cpp \
    env.cpp \
    filesystem.cpp \
    main.cpp \
    networkaccessmanager.cpp \
    phantom.cpp \
    repl.cpp \
    system.cpp \
    terminal.cpp \
    utils.cpp \
    webpage.cpp \
    webserver.cpp

OTHER_FILES = \
    bootstrap.js \
    configurator.js \
    modules/child_process.js \
    modules/cookiejar.js \
    modules/fs.js \
    modules/webpage.js \
    modules/webserver.js \
    repl.js

include(mongoose/mongoose.pri)
include(linenoise/linenoise.pri)
include(qcommandline/qcommandline.pri)

win32: RC_FILE = phantomjs_win.rc
os2:   RC_FILE = phantomjs_os2.rc

mac {
    QMAKE_CXXFLAGS += -fvisibility=hidden
    QMAKE_LFLAGS += '-sectcreate __TEXT __info_plist Info.plist'
    CONFIG -= app_bundle
# Uncomment to build a Mac OS X Universal Binary (i.e. x86 + ppc)
#    CONFIG += x86 ppc
}

win32-msvc* {
    DEFINES += NOMINMAX \
        WIN32_LEAN_AND_MEAN \
        _CRT_SECURE_NO_WARNINGS
    # ingore warnings:
    # 4049 - locally defined symbol 'symbol' imported
    QMAKE_LFLAGS += /ignore:4049 /LARGEADDRESSAWARE
    LIBS += -lCrypt32 -lzlib
    CONFIG(static) {
        DEFINES += STATIC_BUILD
    }
}

openbsd* {
    LIBS += -L/usr/X11R6/lib
}
