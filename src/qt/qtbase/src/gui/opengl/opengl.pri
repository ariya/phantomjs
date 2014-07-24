# Qt gui library, opengl module

contains(QT_CONFIG, opengl):CONFIG += opengl
contains(QT_CONFIG, opengles2):CONFIG += opengles2
contains(QT_CONFIG, egl):CONFIG += egl

contains(QT_CONFIG, opengl)|contains(QT_CONFIG, opengles2) {

    HEADERS += opengl/qopengl.h \
               opengl/qopengl_p.h \
               opengl/qopenglfunctions.h \
               opengl/qopenglframebufferobject.h  \
               opengl/qopenglframebufferobject_p.h  \
               opengl/qopenglpaintdevice.h \
               opengl/qopenglbuffer.h \
               opengl/qopenglshaderprogram.h \
               opengl/qopenglextensions_p.h \
               opengl/qopenglgradientcache_p.h \
               opengl/qopengltexturecache_p.h \
               opengl/qopenglengineshadermanager_p.h \
               opengl/qopengl2pexvertexarray_p.h \
               opengl/qopenglpaintengine_p.h \
               opengl/qopenglengineshadersource_p.h \
               opengl/qopenglcustomshaderstage_p.h \
               opengl/qtriangulatingstroker_p.h \
               opengl/qopengltextureglyphcache_p.h \
               opengl/qopenglshadercache_p.h \
               opengl/qopenglshadercache_meego_p.h \
               opengl/qtriangulator_p.h \
               opengl/qrbtree_p.h \
               opengl/qopenglversionfunctions.h \
               opengl/qopenglversionfunctionsfactory_p.h \
               opengl/qopenglvertexarrayobject.h \
               opengl/qopengldebug.h \
               opengl/qopengltextureblitter_p.h

    SOURCES += opengl/qopengl.cpp \
               opengl/qopenglfunctions.cpp \
               opengl/qopenglframebufferobject.cpp \
               opengl/qopenglpaintdevice.cpp \
               opengl/qopenglbuffer.cpp \
               opengl/qopenglshaderprogram.cpp \
               opengl/qopenglgradientcache.cpp \
               opengl/qopengltexturecache.cpp \
               opengl/qopenglengineshadermanager.cpp \
               opengl/qopengl2pexvertexarray.cpp \
               opengl/qopenglpaintengine.cpp \
               opengl/qopenglcustomshaderstage.cpp \
               opengl/qtriangulatingstroker.cpp \
               opengl/qopengltextureglyphcache.cpp \
               opengl/qtriangulator.cpp \
               opengl/qopenglversionfunctions.cpp \
               opengl/qopenglversionfunctionsfactory.cpp \
               opengl/qopenglvertexarrayobject.cpp \
               opengl/qopengldebug.cpp \
               opengl/qopengltextureblitter.cpp

    !wince* {
        HEADERS += opengl/qopengltexture.h \
                   opengl/qopengltexture_p.h \
                   opengl/qopengltexturehelper_p.h \
                   opengl/qopenglpixeltransferoptions.h

        SOURCES += opengl/qopengltexture.cpp \
                   opengl/qopengltexturehelper.cpp \
                   opengl/qopenglpixeltransferoptions.cpp
    }

    !contains(QT_CONFIG, opengles2) {
        HEADERS += opengl/qopenglfunctions_1_0.h \
                   opengl/qopenglfunctions_1_1.h \
                   opengl/qopenglfunctions_1_2.h \
                   opengl/qopenglfunctions_1_3.h \
                   opengl/qopenglfunctions_1_4.h \
                   opengl/qopenglfunctions_1_5.h \
                   opengl/qopenglfunctions_2_0.h \
                   opengl/qopenglfunctions_2_1.h \
                   opengl/qopenglfunctions_3_0.h \
                   opengl/qopenglfunctions_3_1.h \
                   opengl/qopenglfunctions_3_2_core.h \
                   opengl/qopenglfunctions_3_3_core.h \
                   opengl/qopenglfunctions_4_0_core.h \
                   opengl/qopenglfunctions_4_1_core.h \
                   opengl/qopenglfunctions_4_2_core.h \
                   opengl/qopenglfunctions_4_3_core.h \
                   opengl/qopenglfunctions_3_2_compatibility.h \
                   opengl/qopenglfunctions_3_3_compatibility.h \
                   opengl/qopenglfunctions_4_0_compatibility.h \
                   opengl/qopenglfunctions_4_1_compatibility.h \
                   opengl/qopenglfunctions_4_2_compatibility.h \
                   opengl/qopenglfunctions_4_3_compatibility.h \
                   opengl/qopenglqueryhelper_p.h \
                   opengl/qopengltimerquery.h

        SOURCES += opengl/qopenglfunctions_1_0.cpp \
                   opengl/qopenglfunctions_1_1.cpp \
                   opengl/qopenglfunctions_1_2.cpp \
                   opengl/qopenglfunctions_1_3.cpp \
                   opengl/qopenglfunctions_1_4.cpp \
                   opengl/qopenglfunctions_1_5.cpp \
                   opengl/qopenglfunctions_2_0.cpp \
                   opengl/qopenglfunctions_2_1.cpp \
                   opengl/qopenglfunctions_3_0.cpp \
                   opengl/qopenglfunctions_3_1.cpp \
                   opengl/qopenglfunctions_3_2_core.cpp \
                   opengl/qopenglfunctions_3_3_core.cpp \
                   opengl/qopenglfunctions_4_0_core.cpp \
                   opengl/qopenglfunctions_4_1_core.cpp \
                   opengl/qopenglfunctions_4_2_core.cpp \
                   opengl/qopenglfunctions_4_3_core.cpp \
                   opengl/qopenglfunctions_3_2_compatibility.cpp \
                   opengl/qopenglfunctions_3_3_compatibility.cpp \
                   opengl/qopenglfunctions_4_0_compatibility.cpp \
                   opengl/qopenglfunctions_4_1_compatibility.cpp \
                   opengl/qopenglfunctions_4_2_compatibility.cpp \
                   opengl/qopenglfunctions_4_3_compatibility.cpp \
                   opengl/qopengltimerquery.cpp
    }

    contains(QT_CONFIG, opengles2) {
        HEADERS += opengl/qopenglfunctions_es2.h

        SOURCES += opengl/qopenglfunctions_es2.cpp
    }
}
