
if(!equals(QT_MAJOR_VERSION, 5)|!equals(QT_MINOR_VERSION, 3)) {
    error("This program can only be compiled with Qt 5.3.x.")
}

TEMPLATE = app
TARGET = phantomjs
QT += network webkitwidgets
CONFIG += console

DESTDIR = ../bin

RESOURCES = phantomjs.qrc \
    ghostdriver/ghostdriver.qrc \

# Include resources for Windows only. Linux and OS X already have them.
# for more info see file: src\qt\qtwebkit\Source\WebCore\Target.pri:17
!winrt:win32: {
    RESOURCES += qt/qtwebkit/Source/WebCore/inspector/front-end/WebKit.qrc \
                 qt/qtwebkit/Source/WebCore/generated/InspectorBackendCommands.qrc
}

!winrt:!win32: {
    QTPLUGIN += qphantom
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

linux*|mac|openbsd* {
    INCLUDEPATH += breakpad/src

    SOURCES += breakpad/src/client/minidump_file_writer.cc \
      breakpad/src/common/convert_UTF.c \
      breakpad/src/common/md5.cc \
      breakpad/src/common/string_conversion.cc 
}

linux* {
    SOURCES += breakpad/src/client/linux/crash_generation/crash_generation_client.cc \
      breakpad/src/client/linux/handler/exception_handler.cc \
      breakpad/src/client/linux/log/log.cc \
      breakpad/src/client/linux/minidump_writer/linux_dumper.cc \
      breakpad/src/client/linux/minidump_writer/linux_ptrace_dumper.cc \
      breakpad/src/client/linux/minidump_writer/minidump_writer.cc \
      breakpad/src/common/linux/file_id.cc \
      breakpad/src/common/linux/guid_creator.cc \
      breakpad/src/common/linux/memory_mapped_file.cc \
      breakpad/src/common/linux/safe_readlink.cc
}

mac {
    SOURCES += breakpad/src/client/mac/crash_generation/crash_generation_client.cc \
      breakpad/src/client/mac/handler/exception_handler.cc \
      breakpad/src/client/mac/handler/minidump_generator.cc \
      breakpad/src/client/mac/handler/dynamic_images.cc \
      breakpad/src/client/mac/handler/breakpad_nlist_64.cc \
      breakpad/src/common/mac/bootstrap_compat.cc \
      breakpad/src/common/mac/file_id.cc \
      breakpad/src/common/mac/macho_id.cc \
      breakpad/src/common/mac/macho_utilities.cc \
      breakpad/src/common/mac/macho_walker.cc \
      breakpad/src/common/mac/string_utilities.cc

    OBJECTIVE_SOURCES += breakpad/src/common/mac/MachIPC.mm
}

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
    LIBS += -lCrypt32 -llibxml2
    INCLUDEPATH += breakpad/src
    SOURCES += breakpad/src/client/windows/handler/exception_handler.cc \
      breakpad/src/client/windows/crash_generation/crash_generation_client.cc \
      breakpad/src/common/windows/guid_string.cc
    CONFIG(static) {
        DEFINES += STATIC_BUILD
    }
}

openbsd* {
    LIBS += -L/usr/X11R6/lib
}
