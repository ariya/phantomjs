TEMPLATE = app
TARGET = phantomjs
QT += network webkit
CONFIG += console

# Comment to enable Debug Messages
DEFINES += QT_NO_DEBUG_OUTPUT QT_NO_WARNING_OUTPUT

DESTDIR = ../bin

RESOURCES = phantomjs.qrc

HEADERS += csconverter.h \
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
    repl.h \
    replcompletable.h

SOURCES += phantom.cpp \
    callback.cpp \
    webpage.cpp \
    webserver.cpp \
    main.cpp \
    csconverter.cpp \
    utils.cpp \
    networkaccessmanager.cpp \
    cookiejar.cpp \
    filesystem.cpp \
    system.cpp \
    env.cpp \
    terminal.cpp \
    encoding.cpp \
    config.cpp \
    repl.cpp \
    replcompletable.cpp

OTHER_FILES += usage.txt \
    bootstrap.js \
    configurator.js \
    modules/fs.js \
    modules/webpage.js \
    modules/webserver.js \
    repl.js

include(gif/gif.pri)
include(mongoose/mongoose.pri)
include(linenoise/linenoise.pri)

win32: RC_FILE = phantomjs_win.rc
os2:   RC_FILE = phantomjs_os2.rc

mac {
    QMAKE_CXXFLAGS += -fvisibility=hidden
    QMAKE_LFLAGS += '-sectcreate __TEXT __info_plist Info.plist'
    CONFIG -= app_bundle
# Uncomment to build a Mac OS X Universal Binary (i.e. x86 + ppc)
#    CONFIG += x86 ppc
}
CONFIG(static) {
    DEFINES += STATIC_BUILD
}
