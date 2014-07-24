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

#ifndef EGLXSurface_h
#define EGLXSurface_h

#if PLATFORM(X11) && USE(EGL) && USE(GRAPHICS_SURFACE)

#include "EGLSurface.h"
#include <glx/X11Helper.h>

namespace WebCore {

typedef X11Helper NativeWrapper;
typedef Pixmap NativePixmap;

// Contents of the surface are backed by native window.
class EGLWindowTransportSurface : public EGLTransportSurface {
public:
    EGLWindowTransportSurface(const IntSize&, GLPlatformSurface::SurfaceAttributes);
    virtual ~EGLWindowTransportSurface();
    virtual void swapBuffers() OVERRIDE;
    virtual void destroy() OVERRIDE;
};

class EGLPixmapSurface : public EGLOffScreenSurface {
public:
    EGLPixmapSurface(GLPlatformSurface::SurfaceAttributes);
    virtual ~EGLPixmapSurface();
    virtual void destroy() OVERRIDE;
};

class EGLTextureFromPixmap {
public:
    EGLTextureFromPixmap(const NativePixmap, bool, EGLConfig);
    virtual ~EGLTextureFromPixmap();
    bool bindTexImage();
    bool isValid() const;
    bool reBindTexImage();
    void destroy();

private:
    EGLImageKHR m_eglImage;
    EGLSurface m_surface;
};

class EGLXTransportSurfaceClient : public GLTransportSurfaceClient {
public:
    EGLXTransportSurfaceClient(const PlatformBufferHandle, const IntSize&, bool);
    virtual ~EGLXTransportSurfaceClient();
    virtual void prepareTexture() OVERRIDE;
    virtual void destroy() OVERRIDE;

private:
    XImage* m_image;
    IntSize m_size;
    PlatformBufferHandle m_handle;
    GLuint m_format;
    unsigned m_totalBytes;
    OwnPtr<EGLTextureFromPixmap> m_eglImage;
};

}

#endif

#endif
