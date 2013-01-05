VPATH += $$PWD
INCLUDEPATH += $$PWD

SOURCES += mongoose.c
HEADERS += mongoose.h
linux*:LIBS += -ldl
win32:LIBS += -lWs2_32
