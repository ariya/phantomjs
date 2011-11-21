VPATH += $$PWD
INCLUDEPATH += $$PWD

SOURCES += mongoose.c
HEADERS += mongoose.h
unix:LIBS += -ldl
