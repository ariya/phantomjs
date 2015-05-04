# The library is expected to be the same as in ES 2.0 (libGLESv2).
# The difference is the header and the presence of the functions in
# the library.

SOURCES = opengles31.cpp
INCLUDEPATH += $$QMAKE_INCDIR_OPENGL_ES2

for(p, QMAKE_LIBDIR_OPENGL_ES2) {
    exists($$p):LIBS += -L$$p
}

CONFIG -= qt
LIBS += $$QMAKE_LIBS_OPENGL_ES2
