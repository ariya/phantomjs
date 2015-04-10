SOURCES = glxfbconfig.cpp
CONFIG += x11
INCLUDEPATH += $$QMAKE_INCDIR_OPENGL

for(p, QMAKE_LIBDIR_OPENGL) {
    exists($$p):LIBS += -L$$p
}

CONFIG -= qt
LIBS += -lGL
