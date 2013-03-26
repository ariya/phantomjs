TEMPLATE = app
TARGET = phantomjs
QT += network webkit
CONFIG += console

DESTDIR = ../bin

RESOURCES = phantomjs.qrc \
    ghostdriver/ghostdriver.qrc \
    qt/src/3rdparty/webkit/Source/WebCore/inspector/front-end/WebKit.qrc \
    qt/src/3rdparty/webkit/Source/WebCore/generated/InspectorBackendStub.qrc

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
    childprocess.h \
    repl.h

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
    childprocess.cpp \
    repl.cpp

OTHER_FILES += \
    bootstrap.js \
    configurator.js \
    modules/fs.js \
    modules/webpage.js \
    modules/webserver.js \
    modules/child_process.js \
    repl.js

include(gif/gif.pri)
include(mongoose/mongoose.pri)
include(linenoise/linenoise.pri)
include(qcommandline/qcommandline.pri)

linux*|mac {
    INCLUDEPATH += breakpad/src

    SOURCES += breakpad/src/client/minidump_file_writer.cc \
      breakpad/src/common/convert_UTF.c \
      breakpad/src/common/md5.cc \
      breakpad/src/common/string_conversion.cc 

    QTPLUGIN += \
        qcncodecs \
        qjpcodecs \
        qkrcodecs \
        qtwcodecs
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
    LIBS += -lCrypt32
    INCLUDEPATH += breakpad/src
    SOURCES += breakpad/src/client/windows/handler/exception_handler.cc \
      breakpad/src/client/windows/crash_generation/crash_generation_client.cc \
      breakpad/src/common/windows/guid_string.cc
    CONFIG(static) {
        DEFINES += STATIC_BUILD
        QTPLUGIN += \
            qcncodecs \
            qjpcodecs \
            qkrcodecs \
            qtwcodecs \
            qico
    }
}
