VPATH += $$PWD
INCLUDEPATH += $$PWD

DEFINES += HAVE_CONFIG_H
DEFINES += HAVE_STDINT_H
DEFINES += HAVE_FCNTL_H
DEFINES += HAVE_UNISTD_H
DEFINES += HAVE_STDARG_H

SOURCES += gif_err.c
SOURCES += gifalloc.c
SOURCES += egif_lib.c
SOURCES += gif_hash.c
SOURCES += quantize.c
SOURCES += gifwriter.cpp

HEADERS += gif_hash.h
HEADERS += gif_lib_private.h
HEADERS += gif_lib.h
HEADERS += gifwriter.h
