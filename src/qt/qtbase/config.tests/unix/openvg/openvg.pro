SOURCES += openvg.cpp

!isEmpty(QMAKE_INCDIR_OPENVG): INCLUDEPATH += $$QMAKE_INCDIR_OPENVG
!isEmpty(QMAKE_LIBDIR_OPENVG): LIBS += -L$$QMAKE_LIBDIR_OPENVG
!isEmpty(QMAKE_LIBS_OPENVG): LIBS += $$QMAKE_LIBS_OPENVG

# Some OpenVG engines (e.g. ShivaVG) are implemented on top of OpenGL.
# Add the extra includes and libraries for that case.
openvg_on_opengl {
    !isEmpty(QMAKE_INCDIR_OPENGL): INCLUDEPATH += $$QMAKE_INCDIR_OPENGL
    !isEmpty(QMAKE_LIBDIR_OPENGL): LIBS += -L$$QMAKE_LIBDIR_OPENGL
    !isEmpty(QMAKE_LIBS_OPENGL): LIBS += $$QMAKE_LIBS_OPENGL
}

lower_case_includes {
    DEFINES += QT_LOWER_CASE_VG_INCLUDES
}

CONFIG -= qt
