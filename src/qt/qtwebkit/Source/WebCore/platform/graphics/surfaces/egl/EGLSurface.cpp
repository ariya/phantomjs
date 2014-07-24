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
#include "EGLSurface.h"

#if USE(EGL) && USE(GRAPHICS_SURFACE)

#include "EGLConfigSelector.h"
#include "EGLHelper.h"
#include "GLPlatformContext.h"

#if PLATFORM(X11)
#include "EGLXSurface.h"
#endif

namespace WebCore {

PassOwnPtr<GLTransportSurface> EGLTransportSurface::createTransportSurface(const IntSize& size, SurfaceAttributes attributes)
{
    OwnPtr<GLTransportSurface> surface;
#if PLATFORM(X11)
    surface = adoptPtr(new EGLWindowTransportSurface(size, attributes));
#else
    UNUSED_PARAM(size);
    UNUSED_PARAM(attributes);
#endif

    if (surface)
        return surface.release();

    return nullptr;
}

PassOwnPtr<GLTransportSurfaceClient> EGLTransportSurface::createTransportSurfaceClient(const PlatformBufferHandle handle, const IntSize& size, bool hasAlpha)
{
    EGLHelper::resolveEGLBindings();
    OwnPtr<GLTransportSurfaceClient> client;
#if PLATFORM(X11)
    client = adoptPtr(new EGLXTransportSurfaceClient(handle, size, hasAlpha));
#else
    UNUSED_PARAM(handle);
    UNUSED_PARAM(size);
    UNUSED_PARAM(hasAlpha);
#endif

    if (client)
        return client.release();

    return nullptr;
}

EGLTransportSurface::EGLTransportSurface(const IntSize& size, SurfaceAttributes attributes)
    : GLTransportSurface(size, attributes)
{
    m_sharedDisplay = EGLHelper::eglDisplay();

    if (m_sharedDisplay == EGL_NO_DISPLAY)
        return;

    m_configSelector = adoptPtr(new EGLConfigSelector(attributes));
}

GLPlatformSurface::SurfaceAttributes EGLTransportSurface::attributes() const
{
    return m_configSelector->attributes();
}

EGLTransportSurface::~EGLTransportSurface()
{
}

void EGLTransportSurface::destroy()
{
    if (m_drawable == EGL_NO_SURFACE || m_sharedDisplay == EGL_NO_DISPLAY)
        return;

    GLTransportSurface::destroy();

    if (m_drawable) {
        eglDestroySurface(m_sharedDisplay, m_drawable);
        m_drawable = EGL_NO_SURFACE;
    }

    m_configSelector = nullptr;
}

PlatformSurfaceConfig EGLTransportSurface::configuration()
{
    return m_configSelector->surfaceContextConfig();
}

PassOwnPtr<GLPlatformSurface> EGLOffScreenSurface::createOffScreenSurface(SurfaceAttributes attributes)
{
    OwnPtr<GLPlatformSurface> surface;
#if PLATFORM(X11)
    surface = adoptPtr(new EGLPixmapSurface(attributes));
#else
    UNUSED_PARAM(attributes);
#endif

    if (surface)
        return surface.release();

    return nullptr;
}

EGLOffScreenSurface::EGLOffScreenSurface(SurfaceAttributes surfaceAttributes)
    : GLPlatformSurface(surfaceAttributes)
{
    m_sharedDisplay = EGLHelper::eglDisplay();

    if (m_sharedDisplay == EGL_NO_DISPLAY)
        return;

    m_configSelector = adoptPtr(new EGLConfigSelector(surfaceAttributes));
}

EGLOffScreenSurface::~EGLOffScreenSurface()
{
}

GLPlatformSurface::SurfaceAttributes EGLOffScreenSurface::attributes() const
{
    return m_configSelector->attributes();
}

PlatformSurfaceConfig EGLOffScreenSurface::configuration()
{
    return m_configSelector->pixmapContextConfig();
}

void EGLOffScreenSurface::destroy()
{
    if (m_sharedDisplay == EGL_NO_DISPLAY || m_drawable == EGL_NO_SURFACE)
        return;

    if (m_drawable) {
        eglDestroySurface(m_sharedDisplay, m_drawable);
        m_drawable = EGL_NO_SURFACE;
    }

    m_configSelector = nullptr;
}

}

#endif
