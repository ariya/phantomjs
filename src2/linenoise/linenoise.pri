VPATH += $$PWD/src
INCLUDEPATH += $$PWD/src

DEFINES += USE_UTF8

SOURCES += linenoise.c \
    utf8.c
HEADERS += linenoise.h \
    utf8.h
