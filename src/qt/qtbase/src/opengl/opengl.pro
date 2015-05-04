TARGET     = QtOpenGL
QT         = core-private gui-private widgets-private

DEFINES   += QT_NO_USING_NAMESPACE
win32-msvc*|win32-icc:QMAKE_LFLAGS += /BASE:0x63000000
solaris-cc*:QMAKE_CXXFLAGS_RELEASE -= -O2
irix-cc*:QMAKE_CXXFLAGS += -no_prelink -ptused

QMAKE_DOCS = $$PWD/doc/qtopengl.qdocconf

load(qt_module)

contains(QT_CONFIG, opengl):CONFIG += opengl
contains(QT_CONFIG, opengles2):CONFIG += opengles2

HEADERS += qgl.h \
           qgl_p.h \
           qglcolormap.h \
           qglfunctions.h \
           qglpixelbuffer.h \
           qglpixelbuffer_p.h \
           qglframebufferobject.h  \
           qglframebufferobject_p.h  \
           qglpaintdevice_p.h \
           qglbuffer.h \
           qtopenglglobal.h

SOURCES += qgl.cpp \
           qglcolormap.cpp \
           qglfunctions.cpp \
           qglpixelbuffer.cpp \
           qglframebufferobject.cpp \
           qglpaintdevice.cpp \
           qglbuffer.cpp \

HEADERS +=  qglshaderprogram.h \
            qgraphicsshadereffect_p.h \
            gl2paintengineex/qglgradientcache_p.h \
            gl2paintengineex/qglengineshadermanager_p.h \
            gl2paintengineex/qgl2pexvertexarray_p.h \
            gl2paintengineex/qpaintengineex_opengl2_p.h \
            gl2paintengineex/qglengineshadersource_p.h \
            gl2paintengineex/qglcustomshaderstage_p.h \
            gl2paintengineex/qtextureglyphcache_gl_p.h \
            gl2paintengineex/qglshadercache_p.h \
            gl2paintengineex/qglshadercache_meego_p.h

SOURCES +=  qglshaderprogram.cpp \
            qgraphicsshadereffect.cpp \
            gl2paintengineex/qglgradientcache.cpp \
            gl2paintengineex/qglengineshadermanager.cpp \
            gl2paintengineex/qgl2pexvertexarray.cpp \
            gl2paintengineex/qpaintengineex_opengl2.cpp \
            gl2paintengineex/qglcustomshaderstage.cpp \
            gl2paintengineex/qtextureglyphcache_gl.cpp
