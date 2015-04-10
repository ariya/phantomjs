TARGET     = QtGui
QT = core-private

contains(QT_CONFIG, opengl.*): MODULE_CONFIG = opengl

DEFINES   += QT_NO_USING_NAMESPACE

QMAKE_DOCS = $$PWD/doc/qtgui.qdocconf

MODULE_PLUGIN_TYPES = \
    platforms \
    platformthemes \
    platforminputcontexts \
    generic \
    iconengines \
    imageformats

# This is here only because the platform plugin is no module, obviously.
win32:contains(QT_CONFIG, angle)|contains(QT_CONFIG, dynamicgl) {
    MODULE_AUX_INCLUDES = \
        \$\$QT_MODULE_INCLUDE_BASE/QtANGLE
}

load(qt_module)

# Code coverage with TestCocoon
# The following is required as extra compilers use $$QMAKE_CXX instead of $(CXX).
# Without this, testcocoon.prf is read only after $$QMAKE_CXX is used by the
# extra compilers.
testcocoon {
    load(testcocoon)
}

mac:!ios: LIBS_PRIVATE += -framework Cocoa

CONFIG += simd optimize_full

include(accessible/accessible.pri)
include(kernel/kernel.pri)
include(image/image.pri)
include(text/text.pri)
include(painting/painting.pri)
include(util/util.pri)
include(math3d/math3d.pri)
include(opengl/opengl.pri)
include(animation/animation.pri)
include(itemmodels/itemmodels.pri)

QMAKE_LIBS += $$QMAKE_LIBS_GUI

load(cmake_functions)

win32: CMAKE_WINDOWS_BUILD = True

contains(QT_CONFIG, angle) {
    CMAKE_GL_INCDIRS = $$CMAKE_INCLUDE_DIR
    CMAKE_ANGLE_EGL_DLL_RELEASE = libEGL.dll
    CMAKE_ANGLE_EGL_IMPLIB_RELEASE = libEGL.lib
    CMAKE_ANGLE_GLES2_DLL_RELEASE = libGLESv2.dll
    CMAKE_ANGLE_GLES2_IMPLIB_RELEASE = libGLESv2.lib
    CMAKE_ANGLE_EGL_DLL_DEBUG = libEGLd.dll
    CMAKE_ANGLE_EGL_IMPLIB_DEBUG = libEGLd.lib
    CMAKE_ANGLE_GLES2_DLL_DEBUG = libGLESv2d.dll
    CMAKE_ANGLE_GLES2_IMPLIB_DEBUG = libGLESv2d.lib

    CMAKE_QT_OPENGL_IMPLEMENTATION = GLESv2
} else {
    contains(QT_CONFIG, egl) {
        CMAKE_EGL_LIBS = $$cmakeProcessLibs($$QMAKE_LIBS_EGL)
        !isEmpty(QMAKE_LIBDIR_EGL): CMAKE_EGL_LIBDIR += $$cmakeTargetPath($$QMAKE_LIBDIR_EGL)
    }

    contains(QT_CONFIG, opengles2) {
        !isEmpty(QMAKE_INCDIR_OPENGL_ES2): CMAKE_GL_INCDIRS = $$cmakeTargetPaths($$QMAKE_INCDIR_OPENGL_ES2)
        CMAKE_OPENGL_INCDIRS = $$cmakePortablePaths($$QMAKE_INCDIR_OPENGL_ES2)
        CMAKE_OPENGL_LIBS = $$cmakeProcessLibs($$QMAKE_LIBS_OPENGL_ES2)
        !isEmpty(QMAKE_LIBDIR_OPENGL_ES2): CMAKE_OPENGL_LIBDIR = $$cmakePortablePaths($$QMAKE_LIBDIR_OPENGL_ES2)
        CMAKE_GL_HEADER_NAME = GLES2/gl2.h
        CMAKE_QT_OPENGL_IMPLEMENTATION = GLESv2
    } else:contains(QT_CONFIG, opengl) {
        !isEmpty(QMAKE_INCDIR_OPENGL): CMAKE_GL_INCDIRS = $$cmakeTargetPaths($$QMAKE_INCDIR_OPENGL)
        CMAKE_OPENGL_INCDIRS = $$cmakePortablePaths($$QMAKE_INCDIR_OPENGL)
        !contains(QT_CONFIG, dynamicgl): CMAKE_OPENGL_LIBS = $$cmakeProcessLibs($$QMAKE_LIBS_OPENGL)
        !isEmpty(QMAKE_LIBDIR_OPENGL): CMAKE_OPENGL_LIBDIR = $$cmakePortablePaths($$QMAKE_LIBDIR_OPENGL)
        CMAKE_GL_HEADER_NAME = GL/gl.h
        mac: CMAKE_GL_HEADER_NAME = gl.h
        CMAKE_QT_OPENGL_IMPLEMENTATION = GL
    }
}

contains(QT_CONFIG, egl): CMAKE_EGL_INCDIRS = $$cmakePortablePaths($$QMAKE_INCDIR_EGL)

QMAKE_DYNAMIC_LIST_FILE = $$PWD/QtGui.dynlist
