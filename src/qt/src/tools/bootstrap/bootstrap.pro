TEMPLATE = lib
TARGET = bootstrap
CONFIG += staticlib

CONFIG += console qtinc 
CONFIG -= qt
build_all:!build_pass {
    CONFIG -= build_all
    CONFIG += release
}
mac:CONFIG -= app_bundle incremental

DEFINES += \
        QT_BOOTSTRAPPED \
        QT_LITE_UNICODE \
        QT_NO_CAST_FROM_ASCII \
        QT_NO_CAST_TO_ASCII \
        QT_NO_CODECS \
        QT_NO_DATASTREAM \
        QT_NO_GEOM_VARIANT \
        QT_NO_LIBRARY \
        QT_NO_QOBJECT \
        QT_NO_STL \
        QT_NO_SYSTEMLOCALE \
        QT_NO_TEXTSTREAM \
        QT_NO_THREAD \
        QT_NO_UNICODETABLES \
        QT_NO_USING_NAMESPACE \
        QT_NO_DEPRECATED

win32:DEFINES += QT_NODLL

INCLUDEPATH += $$QT_BUILD_TREE/include \
            $$QT_BUILD_TREE/include/QtCore \
            $$QT_BUILD_TREE/include/QtXml

DEPENDPATH += $$INCLUDEPATH \
              ../../corelib/global \
              ../../corelib/kernel \
              ../../corelib/tools \
              ../../corelib/io \
              ../../corelib/codecs \
              ../../xml

SOURCES += \
           ../../corelib/codecs/qisciicodec.cpp \
           ../../corelib/codecs/qlatincodec.cpp \
           ../../corelib/codecs/qsimplecodec.cpp \
           ../../corelib/codecs/qtextcodec.cpp \
           ../../corelib/codecs/qtsciicodec.cpp \
           ../../corelib/codecs/qutfcodec.cpp \
           ../../corelib/global/qglobal.cpp \
           ../../corelib/global/qmalloc.cpp \
           ../../corelib/global/qnumeric.cpp \
           ../../corelib/io/qabstractfileengine.cpp \
           ../../corelib/io/qbuffer.cpp \
           ../../corelib/io/qdatastream.cpp \
           ../../corelib/io/qdir.cpp \
           ../../corelib/io/qdiriterator.cpp \
           ../../corelib/io/qfile.cpp \
           ../../corelib/io/qfileinfo.cpp \
           ../../corelib/io/qfilesystementry.cpp \
           ../../corelib/io/qfilesystemengine.cpp \
           ../../corelib/io/qfsfileengine.cpp \
           ../../corelib/io/qfsfileengine_iterator.cpp \
           ../../corelib/io/qiodevice.cpp \
           ../../corelib/io/qtemporaryfile.cpp \
           ../../corelib/io/qtextstream.cpp \
           ../../corelib/kernel/qmetatype.cpp \
           ../../corelib/kernel/qvariant.cpp \
           ../../corelib/kernel/qsystemerror.cpp \
           ../../corelib/tools/qbitarray.cpp \
           ../../corelib/tools/qbytearray.cpp \
           ../../corelib/tools/qbytearraymatcher.cpp \
           ../../corelib/tools/qdatetime.cpp \
           ../../corelib/tools/qhash.cpp \
           ../../corelib/tools/qlist.cpp \
           ../../corelib/tools/qlocale.cpp \
           ../../corelib/tools/qlocale_tools.cpp \
           ../../corelib/tools/qmap.cpp \
           ../../corelib/tools/qregexp.cpp \
           ../../corelib/tools/qstring.cpp \
           ../../corelib/tools/qstringlist.cpp \
           ../../corelib/tools/qvector.cpp \
           ../../corelib/tools/qvsnprintf.cpp \
           ../../corelib/xml/qxmlutils.cpp \
           ../../corelib/xml/qxmlstream.cpp \
           ../../xml/dom/qdom.cpp \
           ../../xml/sax/qxml.cpp

unix:SOURCES += ../../corelib/io/qfilesystemengine_unix.cpp \
                ../../corelib/io/qfilesystemiterator_unix.cpp \
                ../../corelib/io/qfsfileengine_unix.cpp

win32:SOURCES += ../../corelib/io/qfilesystemengine_win.cpp \
                 ../../corelib/io/qfilesystemiterator_win.cpp \
                 ../../corelib/io/qfsfileengine_win.cpp \
                 ../../corelib/plugin/qsystemlibrary.cpp \

mac: OBJECTIVE_SOURCES += ../../corelib/tools/qlocale_mac.mm
else:symbian:SOURCES += ../../corelib/tools/qlocale_symbian.cpp
else:unix:SOURCES += ../../corelib/tools/qlocale_unix.cpp
else:win32:SOURCES += ../../corelib/tools/qlocale_win.cpp

macx: {
   QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.4 #enables weak linking for 10.4 (exported)
   SOURCES += ../../corelib/io/qfilesystemengine_mac.cpp
   SOURCES += ../../corelib/kernel/qcore_mac.cpp
   LIBS += -framework CoreServices
}

if(contains(QT_CONFIG, zlib)|cross_compile):include(../../3rdparty/zlib.pri)
else:include(../../3rdparty/zlib_dependency.pri)

lib.CONFIG = dummy_install
INSTALLS += lib

# Make dummy "sis" and "freeze" target to keep recursive "make sis/freeze" working.
sis_target.target = sis
sis_target.commands =
sis_target.depends = first
QMAKE_EXTRA_TARGETS += sis_target
freeze_target.target = freeze
freeze_target.commands =
freeze_target.depends = first
QMAKE_EXTRA_TARGETS += freeze_target
