/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "GLXContext.h"

#if USE(ACCELERATED_COMPOSITING) && USE(GLX)

#include "X11Helper.h"

namespace WebCore {

typedef GLXContext (*GLXCREATECONTEXTATTRIBSARBPROC)(Display*, GLXFBConfig, GLXContext, Bool, const int*);
static GLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB = 0;

static int Attribs[] = {
    GLX_CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB,
    GLX_LOSE_CONTEXT_ON_RESET_ARB,
    0 };

static void initializeARBExtensions()
{
    static bool initialized = false;
    if (initialized)
        return;

    initialized = true;
    if (GLPlatformContext::supportsGLXExtension(X11Helper::nativeDisplay(), "GLX_ARB_create_context_robustness"))
        glXCreateContextAttribsARB = reinterpret_cast<GLXCREATECONTEXTATTRIBSARBPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glXCreateContextAttribsARB")));
}

GLXOffScreenContext::GLXOffScreenContext()
    : GLPlatformContext()
{
}

bool GLXOffScreenContext::initialize(GLPlatformSurface* surface, PlatformContext sharedContext)
{
    if (!surface)
        return false;

    Display* x11Display = surface->sharedDisplay();
    if (!x11Display)
        return false;

    GLXFBConfig config = surface->configuration();

    if (config) {
        initializeARBExtensions();

        if (glXCreateContextAttribsARB)
            m_contextHandle = glXCreateContextAttribsARB(x11Display, config, sharedContext, true, Attribs);

        if (m_contextHandle) {
            // The GLX_ARB_create_context_robustness spec requires that a context created with
            // GLX_CONTEXT_ROBUST_ACCESS_BIT_ARB bit set must also support GL_ARB_robustness or
            // a version of OpenGL incorporating equivalent functionality.
            // The spec also defines similar requirements for attribute GLX_CONTEXT_RESET_NOTIFICATION_STRATEGY_ARB.
            if (platformMakeCurrent(surface) && GLPlatformContext::supportsGLExtension("GL_ARB_robustness"))
                m_resetLostContext = true;
            else
                glXDestroyContext(x11Display, m_contextHandle);
        }

        bool supportsAlpha = surface->attributes() & GLPlatformSurface::SupportAlpha;
        if (!m_contextHandle)
            m_contextHandle = glXCreateNewContext(x11Display, config, supportsAlpha ? GLX_RGBA_TYPE : 0, sharedContext, true);

        if (m_contextHandle)
            return true;
    }

    return false;
}

GLXOffScreenContext::~GLXOffScreenContext()
{
}

bool GLXOffScreenContext::isCurrentContext() const
{
    return m_contextHandle == glXGetCurrentContext();
}

bool GLXOffScreenContext::platformMakeCurrent(GLPlatformSurface* surface)
{
    return glXMakeCurrent(surface->sharedDisplay(), surface->drawable(), m_contextHandle);
}

void GLXOffScreenContext::platformReleaseCurrent()
{
    Display* x11Display = X11Helper::nativeDisplay();
    if (!x11Display)
        return;

    glXMakeCurrent(x11Display, 0, 0);
}

void GLXOffScreenContext::freeResources()
{
    Display* x11Display = X11Helper::nativeDisplay();
    if (!x11Display)
        return;

    if (m_contextHandle)
        glXDestroyContext(x11Display, m_contextHandle);

    m_contextHandle = 0;
}

void GLXOffScreenContext::destroy()
{
    freeResources();
    GLPlatformContext::destroy();
}

}

#endif
