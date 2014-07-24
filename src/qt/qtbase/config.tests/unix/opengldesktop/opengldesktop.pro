SOURCES = opengldesktop.cpp
INCLUDEPATH += $$QMAKE_INCDIR_OPENGL

for(p, QMAKE_LIBDIR_OPENGL) {
    exists($$p):LIBS += -L$$p
}

CONFIG -= qt
LIBS += $$QMAKE_LIBS_OPENGL

mac:DEFINES += Q_OS_MAC
