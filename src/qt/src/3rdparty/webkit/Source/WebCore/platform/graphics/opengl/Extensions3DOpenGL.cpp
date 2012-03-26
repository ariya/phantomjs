/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#if ENABLE(WEBGL)

#include "Extensions3DOpenGL.h"

#include "GraphicsContext3D.h"
#include <wtf/Vector.h>

#if PLATFORM(MAC)
#include "ANGLE/ShaderLang.h"
#include <OpenGL/gl.h>
#elif PLATFORM(GTK)
#include "OpenGLShims.h"
#endif

namespace WebCore {

Extensions3DOpenGL::Extensions3DOpenGL(GraphicsContext3D* context)
    : m_initializedAvailableExtensions(false)
    , m_context(context)
{
}

Extensions3DOpenGL::~Extensions3DOpenGL()
{
}

bool Extensions3DOpenGL::supports(const String& name)
{
    // Note on support for BGRA:
    //
    // For OpenGL ES2.0, requires checking for
    // GL_EXT_texture_format_BGRA8888 and GL_EXT_read_format_bgra.
    // For desktop GL, BGRA has been supported since OpenGL 1.2.
    //
    // However, note that the GL ES2 extension requires the
    // internalFormat to glTexImage2D() be GL_BGRA, while desktop GL
    // will not accept GL_BGRA (must be GL_RGBA), so this must be
    // checked on each platform. Desktop GL offers neither
    // GL_EXT_texture_format_BGRA8888 or GL_EXT_read_format_bgra, so
    // treat them as unsupported here.
    if (!m_initializedAvailableExtensions) {
        String extensionsString(reinterpret_cast<const char*>(::glGetString(GL_EXTENSIONS)));
        Vector<String> availableExtensions;
        extensionsString.split(" ", availableExtensions);
        for (size_t i = 0; i < availableExtensions.size(); ++i)
            m_availableExtensions.add(availableExtensions[i]);
        m_initializedAvailableExtensions = true;
    }
    
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
    if (name == "GL_OES_vertex_array_object")
        return m_availableExtensions.contains("GL_APPLE_vertex_array_object");

    // Desktop GL always supports the standard derivative functions
    if (name == "GL_OES_standard_derivatives")
        return true;

    return m_availableExtensions.contains(name);
}

void Extensions3DOpenGL::ensureEnabled(const String& name)
{
#if PLATFORM(MAC)
    if (name == "GL_OES_standard_derivatives") {
        // Enable support in ANGLE (if not enabled already)
        ANGLEWebKitBridge& compiler = m_context->m_compiler;
        ShBuiltInResources ANGLEResources = compiler.getResources();
        if (!ANGLEResources.OES_standard_derivatives) {
            ANGLEResources.OES_standard_derivatives = 1;
            compiler.setResources(ANGLEResources);
        }
    }
#else
    ASSERT_UNUSED(name, supports(name));
#endif
}

bool Extensions3DOpenGL::isEnabled(const String& name)
{
#if PLATFORM(MAC)
    if (name == "GL_OES_standard_derivatives") {
        ANGLEWebKitBridge& compiler = m_context->m_compiler;
        return compiler.getResources().OES_standard_derivatives;
    }
#endif
    return supports(name);
}

int Extensions3DOpenGL::getGraphicsResetStatusARB()
{
    return GraphicsContext3D::NO_ERROR;
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
#if !PLATFORM(GTK) && defined(GL_APPLE_vertex_array_object) && GL_APPLE_vertex_array_object
    GLuint array = 0;
    glGenVertexArraysAPPLE(1, &array);
    return array;
#else
    return 0;
#endif
}

void Extensions3DOpenGL::deleteVertexArrayOES(Platform3DObject array)
{
    if (!array)
        return;
    
    m_context->makeContextCurrent();
#if !PLATFORM(GTK) && defined(GL_APPLE_vertex_array_object) && GL_APPLE_vertex_array_object
    glDeleteVertexArraysAPPLE(1, &array);
#endif
}

GC3Dboolean Extensions3DOpenGL::isVertexArrayOES(Platform3DObject array)
{
    if (!array)
        return GL_FALSE;
    
    m_context->makeContextCurrent();
#if !PLATFORM(GTK) && defined(GL_APPLE_vertex_array_object) && GL_APPLE_vertex_array_object
    return glIsVertexArrayAPPLE(array);
#else
    return GL_FALSE;
#endif
}

void Extensions3DOpenGL::bindVertexArrayOES(Platform3DObject array)
{
    if (!array)
        return;

    m_context->makeContextCurrent();
#if !PLATFORM(GTK) && defined(GL_APPLE_vertex_array_object) && GL_APPLE_vertex_array_object
    glBindVertexArrayAPPLE(array);
#endif
}

} // namespace WebCore

#endif // ENABLE(WEBGL)
