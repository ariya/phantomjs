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

#ifndef GLContextGLX_h
#define GLContextGLX_h

#if USE(GLX)

#include "GLContext.h"

typedef struct __GLXcontextRec* GLXContext;
typedef unsigned long GLXPbuffer;
typedef unsigned long GLXPixmap;
typedef unsigned char GLubyte;
typedef unsigned long Pixmap;
typedef unsigned long XID;
typedef void* ContextKeyType;

namespace WebCore {

class GLContextGLX : public GLContext {
    WTF_MAKE_NONCOPYABLE(GLContextGLX);
public:
    static PassOwnPtr<GLContextGLX> createContext(XID window, GLContext* sharingContext);
    static PassOwnPtr<GLContextGLX> createWindowContext(XID window, GLContext* sharingContext);

    virtual ~GLContextGLX();
    virtual bool makeContextCurrent();
    virtual void swapBuffers();
    virtual void waitNative();
    virtual bool canRenderToDefaultFramebuffer();
    virtual IntSize defaultFrameBufferSize();
    virtual cairo_device_t* cairoDevice();

#if USE(3D_GRAPHICS)
    virtual PlatformGraphicsContext3D platformContext();
#endif

private:
    static PassOwnPtr<GLContextGLX> createPbufferContext(GLXContext sharingContext);
    static PassOwnPtr<GLContextGLX> createPixmapContext(GLXContext sharingContext);

    GLContextGLX(GLXContext);
    GLContextGLX(GLXContext, Pixmap, GLXPixmap);

    GLXContext m_context;
    XID m_window;
    GLXPbuffer m_pbuffer;
    Pixmap m_pixmap;
    GLXPixmap m_glxPixmap;
    cairo_device_t* m_cairoDevice;
};

} // namespace WebCore

#endif // USE(GLX)

#endif // GLContextGLX_h
