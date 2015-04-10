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
#include "GLXSurface.h"

#if USE(ACCELERATED_COMPOSITING) && USE(GLX)

namespace WebCore {

static PFNGLXBINDTEXIMAGEEXTPROC pGlXBindTexImageEXT = 0;
static PFNGLXRELEASETEXIMAGEEXTPROC pGlXReleaseTexImageEXT = 0;

static bool resolveGLMethods()
{
    static bool resolved = false;
    if (resolved)
        return true;

    pGlXBindTexImageEXT = reinterpret_cast<PFNGLXBINDTEXIMAGEEXTPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glXBindTexImageEXT")));
    pGlXReleaseTexImageEXT = reinterpret_cast<PFNGLXRELEASETEXIMAGEEXTPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glXReleaseTexImageEXT")));

    resolved = pGlXBindTexImageEXT && pGlXReleaseTexImageEXT;

    return resolved;
}

static int glxAttributes[] = {
    GLX_TEXTURE_FORMAT_EXT,
    GLX_TEXTURE_FORMAT_RGBA_EXT,
    GLX_TEXTURE_TARGET_EXT,
    GLX_TEXTURE_2D_EXT,
    0
};

static bool isMesaGLX()
{
    static bool isMesa = !!strstr(glXGetClientString(X11Helper::nativeDisplay(), GLX_VENDOR), "Mesa");
    return isMesa;
}

GLXTransportSurface::GLXTransportSurface(const IntSize& size, SurfaceAttributes attributes)
    : GLTransportSurface(size, attributes)
{
    m_sharedDisplay = X11Helper::nativeDisplay();
    attributes |= GLPlatformSurface::DoubleBuffered;
    m_configSelector = adoptPtr(new GLXConfigSelector(attributes));
    OwnPtrX11<XVisualInfo> visInfo(m_configSelector->visualInfo(m_configSelector->surfaceContextConfig()));

    if (!visInfo.get()) {
        destroy();
        return;
    }

    X11Helper::createOffScreenWindow(&m_bufferHandle, *visInfo.get(), size);

    if (!m_bufferHandle) {
        destroy();
        return;
    }

    m_drawable = m_bufferHandle;
}

GLXTransportSurface::~GLXTransportSurface()
{
}

PlatformSurfaceConfig GLXTransportSurface::configuration()
{
    return m_configSelector->surfaceContextConfig();
}

void GLXTransportSurface::setGeometry(const IntRect& newRect)
{
    GLTransportSurface::setGeometry(newRect);
    X11Helper::resizeWindow(newRect, m_drawable);
    // Force resize of GL surface after window resize.
    glXSwapBuffers(sharedDisplay(), m_drawable);
}

void GLXTransportSurface::swapBuffers()
{
    if (!m_drawable)
        return;

    glXSwapBuffers(sharedDisplay(), m_drawable);
}

void GLXTransportSurface::destroy()
{
    GLTransportSurface::destroy();

    if (m_bufferHandle) {
        X11Helper::destroyWindow(m_bufferHandle);
        m_bufferHandle = 0;
        m_drawable = 0;
    }

    m_configSelector = nullptr;
}

GLPlatformSurface::SurfaceAttributes GLXTransportSurface::attributes() const
{
    return m_configSelector->attributes();
}

GLXOffScreenSurface::GLXOffScreenSurface(SurfaceAttributes surfaceAttributes)
    : GLPlatformSurface(surfaceAttributes)
    , m_pixmap(0)
    , m_glxPixmap(0)
{
    initialize(surfaceAttributes);
}

GLXOffScreenSurface::~GLXOffScreenSurface()
{
}

void GLXOffScreenSurface::initialize(SurfaceAttributes attributes)
{
    m_sharedDisplay = X11Helper::nativeDisplay();

    m_configSelector = adoptPtr(new GLXConfigSelector(attributes));

    OwnPtrX11<XVisualInfo> visualInfo(m_configSelector->visualInfo(m_configSelector->pixmapContextConfig()));
    X11Helper::createPixmap(&m_pixmap, *visualInfo.get());

    if (!m_pixmap) {
        destroy();
        return;
    }

    m_glxPixmap = glXCreateGLXPixmap(m_sharedDisplay, visualInfo.get(), m_pixmap);

    if (!m_glxPixmap) {
        destroy();
        return;
    }

    m_drawable = m_glxPixmap;
}

PlatformSurfaceConfig GLXOffScreenSurface::configuration()
{
    return m_configSelector->pixmapContextConfig();
}

void GLXOffScreenSurface::destroy()
{
    freeResources();
}

void GLXOffScreenSurface::freeResources()
{
    Display* display = sharedDisplay();

    if (!display)
        return;

    if (m_glxPixmap) {
        glXDestroyGLXPixmap(display, m_glxPixmap);
        glXWaitGL();
        m_glxPixmap = 0;
    }

    if (m_pixmap) {
        X11Helper::destroyPixmap(m_pixmap);
        m_pixmap = 0;
    }

    m_configSelector = nullptr;
    m_drawable = 0;
}

GLXTransportSurfaceClient::GLXTransportSurfaceClient(const PlatformBufferHandle handle, bool hasAlpha)
    : GLTransportSurfaceClient()
{
    if (!resolveGLMethods())
        return;

    XWindowAttributes attr;
    Display* display = X11Helper::nativeDisplay();
    if (!XGetWindowAttributes(display, handle, &attr))
        return;

    // Ensure that the window is mapped.
    if (attr.map_state == IsUnmapped || attr.map_state == IsUnviewable)
        return;

    ScopedXPixmapCreationErrorHandler handler;

    XRenderPictFormat* format = XRenderFindVisualFormat(display, attr.visual);
    m_xPixmap = XCompositeNameWindowPixmap(display, handle);

    if (!m_xPixmap)
        return;

    glxAttributes[1] = (format->depth == 32 && hasAlpha) ? GLX_TEXTURE_FORMAT_RGBA_EXT : GLX_TEXTURE_FORMAT_RGB_EXT;

    GLPlatformSurface::SurfaceAttributes sharedSurfaceAttributes = GLPlatformSurface::Default;

    if (hasAlpha)
        sharedSurfaceAttributes = GLPlatformSurface::SupportAlpha;

    GLXConfigSelector configSelector(sharedSurfaceAttributes);

    m_glxPixmap = glXCreatePixmap(display, configSelector.surfaceClientConfig(format->depth, XVisualIDFromVisual(attr.visual)), m_xPixmap, glxAttributes);

    if (!m_glxPixmap || !handler.isValidOperation()) {
        destroy();
        return;
    }

    createTexture();
    glXWaitX();
    pGlXBindTexImageEXT(display, m_glxPixmap, GLX_FRONT_EXT, 0);
}

GLXTransportSurfaceClient::~GLXTransportSurfaceClient()
{
}

void GLXTransportSurfaceClient::destroy()
{
    Display* display = X11Helper::nativeDisplay();
    if (!display)
        return;

    if (m_texture) {
        pGlXReleaseTexImageEXT(display, m_glxPixmap, GLX_FRONT_EXT);
        GLTransportSurfaceClient::destroy();
    }

    if (m_glxPixmap) {
        glXDestroyPixmap(display, m_glxPixmap);
        m_glxPixmap = 0;
        glXWaitGL();
    }

    if (m_xPixmap) {
        X11Helper::destroyPixmap(m_xPixmap);
        m_xPixmap = 0;
    }
}

void GLXTransportSurfaceClient::prepareTexture()
{
    if (isMesaGLX() && m_texture) {
        Display* display = X11Helper::nativeDisplay();
        glBindTexture(GL_TEXTURE_2D, m_texture);
        // Mesa doesn't re-bind texture to the front buffer on glXSwapBufer
        // Manually release previous lock and rebind texture to surface to ensure frame updates.
        pGlXReleaseTexImageEXT(display, m_glxPixmap, GLX_FRONT_EXT);
        pGlXBindTexImageEXT(display, m_glxPixmap, GLX_FRONT_EXT, 0);
    }
}

}

#endif
