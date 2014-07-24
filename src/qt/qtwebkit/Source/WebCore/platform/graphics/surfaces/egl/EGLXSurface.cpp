/*
 * Copyright (C) 2013 Intel Corporation. All rights reserved.
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
#include "EGLXSurface.h"

#if PLATFORM(X11) && USE(EGL) && USE(GRAPHICS_SURFACE)

#include "EGLConfigSelector.h"
#include "EGLHelper.h"
#include "GLPlatformContext.h"

namespace WebCore {

EGLWindowTransportSurface::EGLWindowTransportSurface(const IntSize& size, GLPlatformSurface::SurfaceAttributes attributes)
    : EGLTransportSurface(size, attributes)
{
    if (!m_configSelector)
        return;

    if (!m_configSelector->surfaceContextConfig()) {
        destroy();
        return;
    }

    EGLint visualId = m_configSelector->nativeVisualId(m_configSelector->surfaceContextConfig());

    if (visualId == -1) {
        destroy();
        return;
    }

    NativeWrapper::createOffScreenWindow(&m_bufferHandle, visualId, m_configSelector->attributes() & GLPlatformSurface::SupportAlpha, size);

    if (!m_bufferHandle) {
        destroy();
        return;
    }

    m_drawable = eglCreateWindowSurface(m_sharedDisplay, m_configSelector->surfaceContextConfig(), static_cast<EGLNativeWindowType>(m_bufferHandle), 0);

    if (m_drawable == EGL_NO_SURFACE) {
        LOG_ERROR("Failed to create EGL surface(%d).", eglGetError());
        destroy();
    }
}

EGLWindowTransportSurface::~EGLWindowTransportSurface()
{
}

void EGLWindowTransportSurface::swapBuffers()
{
    if (!eglSwapBuffers(m_sharedDisplay, m_drawable))
        LOG_ERROR("Failed to SwapBuffers(%d).", eglGetError());
}

void EGLWindowTransportSurface::destroy()
{
    EGLTransportSurface::destroy();

    if (m_bufferHandle) {
        NativeWrapper::destroyWindow(m_bufferHandle);
        m_bufferHandle = 0;
    }
}

EGLPixmapSurface::EGLPixmapSurface(GLPlatformSurface::SurfaceAttributes surfaceAttributes)
    : EGLOffScreenSurface(surfaceAttributes)
{
    if (!m_configSelector)
        return;

    EGLConfig config = m_configSelector->pixmapContextConfig();

    if (!config) {
        destroy();
        return;
    }

    EGLint visualId = m_configSelector->nativeVisualId(config);

    if (visualId == -1) {
        destroy();
        return;
    }

    NativePixmap pixmap;
    NativeWrapper::createPixmap(&pixmap, visualId, m_configSelector->attributes() & GLPlatformSurface::SupportAlpha);
    m_bufferHandle = pixmap;

    if (!m_bufferHandle) {
        destroy();
        return;
    }

    m_drawable = eglCreatePixmapSurface(m_sharedDisplay, config, static_cast<EGLNativePixmapType>(m_bufferHandle), 0);

    if (m_drawable == EGL_NO_SURFACE) {
        LOG_ERROR("Failed to create EGL surface(%d).", eglGetError());
        destroy();
    }
}

EGLPixmapSurface::~EGLPixmapSurface()
{
}

void EGLPixmapSurface::destroy()
{
    EGLOffScreenSurface::destroy();

    if (m_bufferHandle) {
        NativeWrapper::destroyPixmap(m_bufferHandle);
        m_bufferHandle = 0;
    }
}

EGLXTransportSurfaceClient::EGLXTransportSurfaceClient(const PlatformBufferHandle handle, const IntSize& size, bool hasAlpha)
    : GLTransportSurfaceClient()
    , m_image(0)
    , m_size(size)
    , m_totalBytes(0)
{
    if (!handle)
        return;

    m_handle = handle;
    XWindowAttributes attr;

    if (!XGetWindowAttributes(NativeWrapper::nativeDisplay(), m_handle, &attr))
        return;

    createTexture();
    GLPlatformSurface::SurfaceAttributes sharedSurfaceAttributes = GLPlatformSurface::Default;

    if (hasAlpha)
        sharedSurfaceAttributes = GLPlatformSurface::SupportAlpha;

    EGLConfigSelector configSelector(sharedSurfaceAttributes);
    EGLConfig config = configSelector.surfaceClientConfig(XVisualIDFromVisual(attr.visual));
    m_eglImage = adoptPtr(new EGLTextureFromPixmap(m_handle, hasAlpha, config));

    if (!m_eglImage->isValid() || eglGetError() != EGL_SUCCESS)
        destroy();

    if (m_eglImage)
        return;

    m_totalBytes = m_size.width() * m_size.height() * 4;

#if USE(OPENGL_ES_2)
    m_format = GraphicsContext3D::RGBA;
    static bool bgraSupported = GLPlatformContext::supportsGLExtension("GL_EXT_texture_format_BGRA8888");
    if (bgraSupported)
        m_format = GraphicsContext3D::BGRA;
#endif

    createTexture();
    prepareTexture();
}

EGLXTransportSurfaceClient::~EGLXTransportSurfaceClient()
{
}

void EGLXTransportSurfaceClient::destroy()
{
    GLTransportSurfaceClient::destroy();

    if (m_eglImage) {
        m_eglImage->destroy();
        m_eglImage = nullptr;
    }

    eglWaitGL();

    if (m_image) {
        XDestroyImage(m_image);
        m_image = 0;
    }
}

void EGLXTransportSurfaceClient::prepareTexture()
{
    ::glBindTexture(GL_TEXTURE_2D, m_texture);

    if (m_eglImage) {
        m_eglImage->reBindTexImage();
        return;
    }

    // Fallback to use XImage in case EGLImage and TextureToPixmap are not supported.
    m_image = XGetImage(NativeWrapper::nativeDisplay(), m_handle, 0, 0, m_size.width(), m_size.height(), AllPlanes, ZPixmap);

#if USE(OPENGL_ES_2)
    if (m_format != GraphicsContext3D::BGRA) {
        for (unsigned i = 0; i < m_totalBytes; i += 4)
            std::swap(m_image->data[i], m_image->data[i + 2]);
    }
#endif

    glTexImage2D(GL_TEXTURE_2D, 0, m_format, m_size.width(), m_size.height(), 0, m_format, GL_UNSIGNED_BYTE, m_image->data);

    if (m_image) {
        XDestroyImage(m_image);
        m_image = 0;
    }
}

EGLTextureFromPixmap::EGLTextureFromPixmap(const NativePixmap handle, bool hasAlpha, EGLConfig config)
    : m_eglImage(0)
    , m_surface(EGL_NO_SURFACE)
{
    if (!handle)
        return;

    static bool textureFromPixmapSupported = GLPlatformContext::supportsEGLExtension(EGLHelper::eglDisplay(), "EGL_NOK_texture_from_pixmap");

    if (textureFromPixmapSupported) {
        const EGLint pixmapAttribs[] = { EGL_TEXTURE_FORMAT, hasAlpha ? EGL_TEXTURE_RGBA : EGL_TEXTURE_RGB, EGL_TEXTURE_TARGET, EGL_TEXTURE_2D, EGL_NONE };
        m_surface = eglCreatePixmapSurface(EGLHelper::eglDisplay(), config, handle, pixmapAttribs);

        if (m_surface != EGL_NO_SURFACE && !eglBindTexImage(EGLHelper::eglDisplay(), m_surface, EGL_BACK_BUFFER))
            destroy();
    }

    if (m_surface != EGL_NO_SURFACE)
        return;

    static const EGLint imageAttrs[] = { EGL_IMAGE_PRESERVED_KHR, EGL_TRUE, EGL_NONE };
    EGLHelper::createEGLImage(&m_eglImage, EGL_NATIVE_PIXMAP_KHR, (EGLClientBuffer)(handle), imageAttrs);

    if (m_eglImage) {
        EGLHelper::imageTargetTexture2DOES(m_eglImage);
        EGLint error = eglGetError();

        if (error != EGL_SUCCESS)
            destroy();
    }
}

EGLTextureFromPixmap::~EGLTextureFromPixmap()
{
}

void EGLTextureFromPixmap::destroy()
{
    eglWaitNative(EGL_CORE_NATIVE_ENGINE);

    if (m_surface != EGL_NO_SURFACE)
        eglReleaseTexImage(EGLHelper::eglDisplay(), m_surface, EGL_BACK_BUFFER);

    if (m_eglImage) {
        EGLHelper::destroyEGLImage(m_eglImage);
        m_eglImage = 0;
    }

    if (m_surface != EGL_NO_SURFACE) {
        eglDestroySurface(EGLHelper::eglDisplay(), m_surface);
        m_surface = EGL_NO_SURFACE;
    }

    eglWaitGL();
}

bool EGLTextureFromPixmap::isValid() const
{
    if (m_surface || m_eglImage)
        return true;

    return false;
}

bool EGLTextureFromPixmap::bindTexImage()
{
    if (m_surface != EGL_NO_SURFACE) {
        bool success = eglBindTexImage(EGLHelper::eglDisplay(), m_surface, EGL_BACK_BUFFER);
        return success;
    }

    if (m_eglImage) {
        EGLHelper::imageTargetTexture2DOES(m_eglImage);
        return true;
    }

    return false;
}

bool EGLTextureFromPixmap::reBindTexImage()
{
    if (m_surface != EGL_NO_SURFACE) {
        bool success = eglReleaseTexImage(EGLHelper::eglDisplay(), m_surface, EGL_BACK_BUFFER);

        if (success)
            success = eglBindTexImage(EGLHelper::eglDisplay(), m_surface, EGL_BACK_BUFFER);

        return success;
    }

    if (m_eglImage) {
        EGLHelper::imageTargetTexture2DOES(m_eglImage);
        return true;
    }

    return false;
}

}

#endif
