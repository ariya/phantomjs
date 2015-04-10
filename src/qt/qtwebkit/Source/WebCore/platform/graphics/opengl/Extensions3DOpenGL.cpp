/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if USE(3D_GRAPHICS)

#include "Extensions3DOpenGL.h"

#include "GraphicsContext3D.h"
#include <wtf/Vector.h>

#if PLATFORM(MAC)
#include "ANGLE/ShaderLang.h"
#include <OpenGL/gl.h>
#elif PLATFORM(GTK) || PLATFORM(EFL) || PLATFORM(QT) || PLATFORM(WIN)
#include "OpenGLShims.h"
#endif

namespace WebCore {

Extensions3DOpenGL::Extensions3DOpenGL(GraphicsContext3D* context)
    : Extensions3DOpenGLCommon(context)
{
}

Extensions3DOpenGL::~Extensions3DOpenGL()
{
}


void Extensions3DOpenGL::blitFramebuffer(long srcX0, long srcY0, long srcX1, long srcY1, long dstX0, long dstY0, long dstX1, long dstY1, unsigned long mask, unsigned long filter)
{
    ::glBlitFramebufferEXT(srcX0, srcY0, srcX1, srcY1, dstX0, dstY0, dstX1, dstY1, mask, filter);
}

void Extensions3DOpenGL::renderbufferStorageMultisample(unsigned long target, unsigned long samples, unsigned long internalformat, unsigned long width, unsigned long height)
{
    ::glRenderbufferStorageMultisampleEXT(target, samples, internalformat, width, height);
}

Platform3DObject Extensions3DOpenGL::createVertexArrayOES()
{
    m_context->makeContextCurrent();
    GLuint array = 0;
#if (PLATFORM(GTK) || PLATFORM(QT) || PLATFORM(EFL)) || PLATFORM(WIN)
    if (isVertexArrayObjectSupported())
        glGenVertexArrays(1, &array);
#elif defined(GL_APPLE_vertex_array_object) && GL_APPLE_vertex_array_object
    glGenVertexArraysAPPLE(1, &array);
#endif
    return array;
}

void Extensions3DOpenGL::deleteVertexArrayOES(Platform3DObject array)
{
    if (!array)
        return;

    m_context->makeContextCurrent();
#if (PLATFORM(GTK) || PLATFORM(QT) || PLATFORM(EFL) || PLATFORM(WIN))
    if (isVertexArrayObjectSupported())
        glDeleteVertexArrays(1, &array);
#elif defined(GL_APPLE_vertex_array_object) && GL_APPLE_vertex_array_object
    glDeleteVertexArraysAPPLE(1, &array);
#endif
}

GC3Dboolean Extensions3DOpenGL::isVertexArrayOES(Platform3DObject array)
{
    if (!array)
        return GL_FALSE;

    m_context->makeContextCurrent();
#if (PLATFORM(GTK) || PLATFORM(QT) || PLATFORM(EFL) || PLATFORM(WIN))
    if (isVertexArrayObjectSupported())
        return glIsVertexArray(array);
#elif defined(GL_APPLE_vertex_array_object) && GL_APPLE_vertex_array_object
    return glIsVertexArrayAPPLE(array);
#endif
    return GL_FALSE;
}

void Extensions3DOpenGL::bindVertexArrayOES(Platform3DObject array)
{
    m_context->makeContextCurrent();
#if (PLATFORM(GTK) || PLATFORM(QT) || PLATFORM(EFL) || PLATFORM(WIN))
    if (isVertexArrayObjectSupported())
        glBindVertexArray(array);
#elif defined(GL_APPLE_vertex_array_object) && GL_APPLE_vertex_array_object
    glBindVertexArrayAPPLE(array);
#else
    UNUSED_PARAM(array);
#endif
}

void Extensions3DOpenGL::copyTextureCHROMIUM(GC3Denum, Platform3DObject, Platform3DObject, GC3Dint, GC3Denum)
{
    // FIXME: implement this function and add GL_CHROMIUM_copy_texture in supports().
    return;
}

void Extensions3DOpenGL::insertEventMarkerEXT(const String&)
{
    // FIXME: implement this function and add GL_EXT_debug_marker in supports().
    return;
}

void Extensions3DOpenGL::pushGroupMarkerEXT(const String&)
{
    // FIXME: implement this function and add GL_EXT_debug_marker in supports().
    return;
}

void Extensions3DOpenGL::popGroupMarkerEXT(void)
{
    // FIXME: implement this function and add GL_EXT_debug_marker in supports().
    return;
}

bool Extensions3DOpenGL::supportsExtension(const String& name)
{
    // GL_ANGLE_framebuffer_blit and GL_ANGLE_framebuffer_multisample are "fake". They are implemented using other
    // extensions. In particular GL_EXT_framebuffer_blit and GL_EXT_framebuffer_multisample
    if (name == "GL_ANGLE_framebuffer_blit")
        return m_availableExtensions.contains("GL_EXT_framebuffer_blit");
    if (name == "GL_ANGLE_framebuffer_multisample")
        return m_availableExtensions.contains("GL_EXT_framebuffer_multisample");

    // Desktop GL always supports GL_OES_rgb8_rgba8.
    if (name == "GL_OES_rgb8_rgba8")
        return true;

    // If GL_ARB_texture_float is available then we report GL_OES_texture_float and
    // GL_OES_texture_half_float as available.
    if (name == "GL_OES_texture_float" || name == "GL_OES_texture_half_float")
        return m_availableExtensions.contains("GL_ARB_texture_float");

    // GL_OES_vertex_array_object
    if (name == "GL_OES_vertex_array_object") {
#if (PLATFORM(GTK) || PLATFORM(QT) || PLATFORM(EFL))
        return m_availableExtensions.contains("GL_ARB_vertex_array_object");
#else
        return m_availableExtensions.contains("GL_APPLE_vertex_array_object");
#endif
    }

    // Desktop GL always supports the standard derivative functions
    if (name == "GL_OES_standard_derivatives")
        return true;

    // Desktop GL always supports UNSIGNED_INT indices
    if (name == "GL_OES_element_index_uint")
        return true;

    if (name == "GL_EXT_texture_filter_anisotropic")
        return m_availableExtensions.contains("GL_EXT_texture_filter_anisotropic");

    if (name == "GL_EXT_draw_buffers") {
#if PLATFORM(MAC)
        return m_availableExtensions.contains("GL_ARB_draw_buffers");
#else
        // FIXME: implement support for other platforms.
        return false;
#endif
    }
    return m_availableExtensions.contains(name);
}

void Extensions3DOpenGL::drawBuffersEXT(GC3Dsizei n, const GC3Denum* bufs)
{
    //  FIXME: implement support for other platforms.
#if PLATFORM(MAC)
    ::glDrawBuffersARB(n, bufs);
#else
    UNUSED_PARAM(n);
    UNUSED_PARAM(bufs);
#endif
}

String Extensions3DOpenGL::getExtensions()
{
    return String(reinterpret_cast<const char*>(::glGetString(GL_EXTENSIONS)));
}

#if (PLATFORM(GTK) || PLATFORM(QT) || PLATFORM(EFL) || PLATFORM(WIN))
bool Extensions3DOpenGL::isVertexArrayObjectSupported()
{
    static const bool supportsVertexArrayObject = supports("GL_OES_vertex_array_object");
    return supportsVertexArrayObject;
}
#endif

} // namespace WebCore

#endif // USE(3D_GRAPHICS)
