TEMPLATE = app
TARGET = phantomjs
QT += network webkitwidgets
CONFIG += console precompile_header

PRECOMPILED_HEADER = config.h

DESTDIR = ../bin

RESOURCES = \
    phantomjs.qrc \
    ghostdriver/ghostdriver.qrc

CONFIG(static) {
    WEB_INSPECTOR_RESOURCES_DIR = $$(WEB_INSPECTOR_RESOURCES_DIR)
    isEmpty(WEB_INSPECTOR_RESOURCES_DIR): {
        error("You must set the environment variable WEB_INSPECTOR_RESOURCES_DIR to generated Web Inspector resources")
    }

    RESOURCES += \
        $(WEB_INSPECTOR_RESOURCES_DIR)/WebInspector.qrc
    message("Using Web Inspector resources from $(WEB_INSPECTOR_RESOURCES_DIR)")
}

HEADERS = \
    callback.h \
    childprocess.h \
    consts.h \
    crashdump.h \
    custompage.h \
    encoding.h \
    env.h \
    filesystem.h \
    phantom.h \
    phantompage.h \
    repl.h \
    settings.h \
    system.h \
    terminal.h \
    webpage.h \
    webpagerenderer.h \
    webserver.h \
    network\cookiejar.h \
    network\networkaccessmanager.h

SOURCES = \
    callback.cpp \
    childprocess.cpp \
    crashdump.cpp \
    custompage.cpp \
    encoding.cpp \
    env.cpp \
    filesystem.cpp \
    main.cpp \
    phantom.cpp \
    phantompage.cpp \
    repl.cpp \
    settings.cpp \
    system.cpp \
    terminal.cpp \
    webpage.cpp \
    webpagerenderer.cpp \
    webserver.cpp \
    network\cookiejar.cpp \
    network\networkaccessmanager.cpp

OTHER_FILES = \
    bootstrap.js \
    modules/child_process.js \
    modules/cookiejar.js \
    modules/fs.js \
    modules/webpage.js \
    modules/webserver.js \
    repl.js

include(mongoose/mongoose.pri)
include(linenoise/linenoise.pri)

win32: RC_FILE = phantomjs_win.rc

mac {
    QMAKE_CXXFLAGS += -fvisibility=hidden
    QMAKE_LFLAGS += '-sectcreate __TEXT __info_plist Info.plist'
    CONFIG -= app_bundle
# Uncomment to build a Mac OS X Universal Binary (i.e. x86 + ppc)
#    CONFIG += x86 ppc
}

win32-msvc* {
    # ingore warnings:
    # 4049 - locally defined symbol 'symbol' imported
    QMAKE_LFLAGS += /ignore:4049 /LARGEADDRESSAWARE
    CONFIG(static) {
        DEFINES += STATIC_BUILD
    }
}

linux {
    include($$PWD/qt-qpa-platform-plugin/phantom.pri)

    CONFIG += c++11
    QTPLUGIN.platforms = -
    LIBS += -L$$PWD/qt-qpa-platform-plugin/plugins/platforms
    LIBS += -lqphantom
}

openbsd* {
    LIBS += -L/usr/X11R6/lib
}
