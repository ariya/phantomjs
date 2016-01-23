
if(!equals(QT_MAJOR_VERSION, 5)|!equals(QT_MINOR_VERSION, 5)) {
    error("This program can only be compiled with Qt 5.5.x.")
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

HEADERS += \
    phantom.h \
    callback.h \
    webpage.h \
    webserver.h \
    consts.h \
    utils.h \
    networkaccessmanager.h \
    cookiejar.h \
    filesystem.h \
    system.h \
    env.h \
    terminal.h \
    encoding.h \
    config.h \
    childprocess.h \
    repl.h \
    crashdump.h

SOURCES += phantom.cpp \
    callback.cpp \
    webpage.cpp \
    webserver.cpp \
    main.cpp \
    utils.cpp \
    networkaccessmanager.cpp \
    cookiejar.cpp \
    filesystem.cpp \
    system.cpp \
    env.cpp \
    terminal.cpp \
    encoding.cpp \
    config.cpp \
    childprocess.cpp \
    repl.cpp \
    crashdump.cpp

OTHER_FILES += \
    bootstrap.js \
    configurator.js \
    modules/fs.js \
    modules/webpage.js \
    modules/webserver.js \
    modules/child_process.js \
    modules/cookiejar.js \
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
    QMAKE_LFLAGS += /ignore:4049
    LIBS += -lCrypt32 -lzlib
    CONFIG(static) {
        DEFINES += STATIC_BUILD
    }
}

openbsd* {
    LIBS += -L/usr/X11R6/lib
}
