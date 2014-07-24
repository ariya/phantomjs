SOURCES += tst_lexgen.cpp
TARGET = tst_lexgen
include(../lexgen.pri)
QT = core testlib
DEFINES += SRCDIR=\\\"$$PWD\\\"
