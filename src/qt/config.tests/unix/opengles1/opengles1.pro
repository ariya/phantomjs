SOURCES = opengles1.cpp
INCLUDEPATH += $$QMAKE_INCDIR_OPENGL_ES1

for(p, QMAKE_LIBDIR_OPENGL_ES1) {
    exists($$p):LIBS += -L$$p
}

CONFIG -= qt
LIBS += $$QMAKE_LIBS_OPENGL_ES1
mac {
    DEFINES += BUILD_ON_MAC
    CONFIG -= app_bundle
}
