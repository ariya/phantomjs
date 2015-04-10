TARGET     = QtXml
QT         = core-private

DEFINES   += QT_NO_USING_NAMESPACE
win32-msvc*|win32-icc:QMAKE_LFLAGS += /BASE:0x61000000

QMAKE_DOCS = $$PWD/doc/qtxml.qdocconf

load(qt_module)

HEADERS += qtxmlglobal.h

PRECOMPILED_HEADER =

include(dom/dom.pri)
include(sax/sax.pri)
