SOURCES = opengles2.cpp
INCLUDEPATH += $$QMAKE_INCDIR_OPENGL_ES2

for(p, QMAKE_LIBDIR_OPENGL_ES2) {
    exists($$p):LIBS += -L$$p
}

CONFIG -= qt
LIBS += $$QMAKE_LIBS_OPENGL_ES2
mac {
    DEFINES += BUILD_ON_MAC
    CONFIG -= app_bundle
}
