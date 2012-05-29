TARGET     = QtXml
QPRO_PWD   = $$PWD
QT         = core
DEFINES   += QT_BUILD_XML_LIB QT_NO_USING_NAMESPACE
win32-msvc*|win32-icc:QMAKE_LFLAGS += /BASE:0x61000000

unix|win32-g++*:QMAKE_PKGCONFIG_REQUIRES = QtCore

include(../qbase.pri)

PRECOMPILED_HEADER = ../corelib/global/qt_pch.h

win32-borland {
        QMAKE_CFLAGS_WARN_ON        += -w-use
        QMAKE_CXXFLAGS_WARN_ON        += -w-use
}

include(dom/dom.pri)
include(sax/sax.pri)
include(stream/stream.pri)

symbian:TARGET.UID3=0x2001B2E0
