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

#ifndef GLContext_h
#define GLContext_h

#include "GraphicsContext3D.h"
#include "Widget.h"
#include <wtf/Noncopyable.h>
#include <wtf/PassOwnPtr.h>

typedef struct _cairo_device cairo_device_t;

#if PLATFORM(X11)
typedef struct _XDisplay Display;
#endif

namespace WebCore {

class GLContext {
    WTF_MAKE_NONCOPYABLE(GLContext);
public:
    static PassOwnPtr<GLContext> createContextForWindow(uint64_t windowHandle, GLContext* sharingContext);
    static PassOwnPtr<GLContext> createOffscreenContext(GLContext* sharing = 0);
    static GLContext* getCurrent();
    static GLContext* sharingContext();

    GLContext();
    virtual ~GLContext();
    virtual bool makeContextCurrent();
    virtual void swapBuffers() = 0;
    virtual void waitNative() = 0;
    virtual bool canRenderToDefaultFramebuffer() = 0;
    virtual IntSize defaultFrameBufferSize() = 0;
    virtual cairo_device_t* cairoDevice() = 0;

#if PLATFORM(X11)
    static Display* sharedX11Display();
    static void cleanupSharedX11Display();
#endif

    static void addActiveContext(GLContext*);
    static void removeActiveContext(GLContext*);
    static void cleanupActiveContextsAtExit();

#if USE(3D_GRAPHICS)
    virtual PlatformGraphicsContext3D platformContext() = 0;
#endif
};

} // namespace WebCore

#endif // GLContext_h
