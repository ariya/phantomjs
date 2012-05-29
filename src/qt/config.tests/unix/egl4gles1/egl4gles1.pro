SOURCES = egl4gles1.cpp

for(p, QMAKE_LIBDIR_EGL) {
    exists($$p):LIBS += -L$$p
}

!isEmpty(QMAKE_INCDIR_EGL): INCLUDEPATH += $$QMAKE_INCDIR_EGL
!isEmpty(QMAKE_LIBS_EGL): LIBS += $$QMAKE_LIBS_EGL

CONFIG -= qt
