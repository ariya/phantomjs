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
#include "EGLContext.h"

#if USE(EGL)

#include <wtf/text/WTFString.h>

namespace WebCore {

static const EGLint contextAttributes[] = {
#if USE(OPENGL_ES_2)
    EGL_CONTEXT_CLIENT_VERSION, 2,
#endif
    EGL_NONE
};

static const EGLint contextRobustnessAttributes[] = {
#if USE(OPENGL_ES_2)
    EGL_CONTEXT_CLIENT_VERSION, 2,
#endif
    EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY_EXT,
    EGL_LOSE_CONTEXT_ON_RESET_EXT,
    EGL_NONE
};

static bool isRobustnessExtSupported(EGLDisplay display)
{
    static bool didQueryForRobustnessExtension = false;
    static bool isRobustnessExtensionSupported = false;

    if (!didQueryForRobustnessExtension) {
        didQueryForRobustnessExtension = true;
        isRobustnessExtensionSupported = GLPlatformContext::supportsEGLExtension(display, "EGL_EXT_create_context_robustness");
    }

    return isRobustnessExtensionSupported;
}

EGLOffScreenContext::EGLOffScreenContext()
    : GLPlatformContext()
    , m_display(0)
{
}

bool EGLOffScreenContext::initialize(GLPlatformSurface* surface, PlatformContext sharedContext)
{
    if (!surface)
        return false;

    if (!eglBindAPI(eglAPIVersion)) {
        LOG_ERROR("Failed to set EGL API(%d).", eglGetError());
        return false;
    }

    m_display = surface->sharedDisplay();
    if (!m_display)
        return false;

    EGLConfig config = surface->configuration();
    if (!config)
        return false;

    if (isRobustnessExtSupported(m_display))
        m_contextHandle = eglCreateContext(m_display, config, sharedContext, contextRobustnessAttributes);

    if (m_contextHandle != EGL_NO_CONTEXT) {
        // The EGL_EXT_create_context_robustness spec requires that a context created with
        // EGL_CONTEXT_OPENGL_RESET_NOTIFICATION_STRATEGY_EXT bit set must also support GL_EXT_robustness or
        // a version of OpenGL incorporating equivalent functionality.
        // The spec also defines similar requirements for attribute EGL_LOSE_CONTEXT_ON_RESET_EXT.
        if (platformMakeCurrent(surface) && (GLPlatformContext::supportsGLExtension("GL_EXT_robustness")))
            m_resetLostContext = true;
        else
            eglDestroyContext(m_display, m_contextHandle);
    }

    if (m_contextHandle == EGL_NO_CONTEXT)
        m_contextHandle = eglCreateContext(m_display, config, sharedContext, contextAttributes);

    if (m_contextHandle != EGL_NO_CONTEXT)
        return true;

    return false;
}

EGLOffScreenContext::~EGLOffScreenContext()
{
}

bool EGLOffScreenContext::isCurrentContext() const
{
    return m_contextHandle == eglGetCurrentContext();
}

bool EGLOffScreenContext::platformMakeCurrent(GLPlatformSurface* surface)
{
    if (!eglMakeCurrent(m_display, surface->drawable(), surface->drawable(), m_contextHandle)) {
        LOG_ERROR("Failed to make context current(%d).", eglGetError());

        if (m_resetLostContext && eglGetError() == EGL_CONTEXT_LOST) {
            LOG_ERROR("Lost current context.");
            m_contextLost = true;
        }

        return false;
    }

    return true;
}

void EGLOffScreenContext::platformReleaseCurrent()
{
    eglMakeCurrent(m_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

void EGLOffScreenContext::freeResources()
{
    if (m_contextHandle == EGL_NO_CONTEXT)
        return;

    eglDestroyContext(m_display, m_contextHandle);
    m_contextHandle = EGL_NO_CONTEXT;
    m_display = 0;
}

void EGLOffScreenContext::destroy()
{
    GLPlatformContext::destroy();
    freeResources();
}

}

#endif
