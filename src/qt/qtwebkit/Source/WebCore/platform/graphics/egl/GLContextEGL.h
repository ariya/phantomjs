/*
 * Copyright (C) 2012 Igalia S.L.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301 USA
 */

#ifndef GLContextEGL_h
#define GLContextEGL_h

#if USE(EGL)

#include "GLContext.h"

#include <EGL/egl.h>

namespace WebCore {

class GLContextEGL : public GLContext {
    WTF_MAKE_NONCOPYABLE(GLContextEGL);
public:
    enum EGLSurfaceType { PbufferSurface, WindowSurface, PixmapSurface };
    static PassOwnPtr<GLContextEGL> createContext(EGLNativeWindowType, GLContext* sharingContext = 0);
    static PassOwnPtr<GLContextEGL> createWindowContext(EGLNativeWindowType, GLContext* sharingContext);

    virtual ~GLContextEGL();
    virtual bool makeContextCurrent();
    virtual void swapBuffers();
    virtual void waitNative();
    virtual bool canRenderToDefaultFramebuffer();
    virtual IntSize defaultFrameBufferSize();
    virtual cairo_device_t* cairoDevice();

#if ENABLE(WEBGL)
    virtual PlatformGraphicsContext3D platformContext();
#endif

private:
    static PassOwnPtr<GLContextEGL> createPbufferContext(EGLContext sharingContext);
    static PassOwnPtr<GLContextEGL> createPixmapContext(EGLContext sharingContext);

    static void addActiveContext(GLContextEGL*);
    static void cleanupSharedEGLDisplay(void);

    GLContextEGL(EGLContext, EGLSurface, EGLSurfaceType);

    EGLContext m_context;
    EGLSurface m_surface;
    EGLSurfaceType m_type;
    cairo_device_t* m_cairoDevice;
};

} // namespace WebCore

#endif // USE(EGL)

#endif // GLContextEGL_h
