/*
 * Copyright (C) 2011, 2012 Igalia, S.L.
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
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "GLContextGLX.h"

#if USE(GLX)
#include "GraphicsContext3D.h"
#include "OpenGLShims.h"
#include <GL/glx.h>
#include <cairo.h>
#include <wtf/OwnPtr.h>

#if ENABLE(ACCELERATED_2D_CANVAS)
#include <cairo-gl.h>
#endif

namespace WebCore {

PassOwnPtr<GLContextGLX> GLContextGLX::createWindowContext(XID window, GLContext* sharingContext)
{
    Display* display = sharedX11Display();
    XWindowAttributes attributes;
    if (!XGetWindowAttributes(display, window, &attributes))
        return nullptr;

    XVisualInfo visualInfo;
    visualInfo.visualid = XVisualIDFromVisual(attributes.visual);

    int numReturned = 0;
    XVisualInfo* visualInfoList = XGetVisualInfo(display, VisualIDMask, &visualInfo, &numReturned);

    GLXContext glxSharingContext = sharingContext ? static_cast<GLContextGLX*>(sharingContext)->m_context : 0;
    GLXContext context = glXCreateContext(display, visualInfoList, glxSharingContext, True);
    XFree(visualInfoList);

    if (!context)
        return nullptr;

    // GLXPbuffer and XID are both the same types underneath, so we have to share
    // a constructor here with the window path.
    GLContextGLX* contextWrapper = new GLContextGLX(context);
    contextWrapper->m_window = window;
    return adoptPtr(contextWrapper);
}

PassOwnPtr<GLContextGLX> GLContextGLX::createPbufferContext(GLXContext sharingContext)
{
    int fbConfigAttributes[] = {
        GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT,
        GLX_RENDER_TYPE, GLX_RGBA_BIT,
        GLX_RED_SIZE, 1,
        GLX_GREEN_SIZE, 1,
        GLX_BLUE_SIZE, 1,
        GLX_ALPHA_SIZE, 1,
        GLX_DOUBLEBUFFER, GL_FALSE,
        0
    };

    int returnedElements;
    Display* display = sharedX11Display();
    GLXFBConfig* configs = glXChooseFBConfig(display, 0, fbConfigAttributes, &returnedElements);
    if (!returnedElements) {
        XFree(configs);
        return nullptr;
    }

    // We will be rendering to a texture, so our pbuffer does not need to be large.
    static const int pbufferAttributes[] = { GLX_PBUFFER_WIDTH, 1, GLX_PBUFFER_HEIGHT, 1, 0 };
    GLXPbuffer pbuffer = glXCreatePbuffer(display, configs[0], pbufferAttributes);
    if (!pbuffer) {
        XFree(configs);
        return nullptr;
    }

    GLXContext context = glXCreateNewContext(display, configs[0], GLX_RGBA_TYPE, sharingContext, GL_TRUE);
    XFree(configs);
    if (!context) {
        glXDestroyPbuffer(display, pbuffer);
        return nullptr;
    }

    // GLXPbuffer and XID are both the same types underneath, so we have to share
    // a constructor here with the window path.
    GLContextGLX* contextWrapper = new GLContextGLX(context);
    contextWrapper->m_pbuffer = pbuffer;
    return adoptPtr(contextWrapper);
}

PassOwnPtr<GLContextGLX> GLContextGLX::createPixmapContext(GLXContext sharingContext)
{
    static int visualAttributes[] = {
        GLX_RGBA,
        GLX_RED_SIZE, 1,
        GLX_GREEN_SIZE, 1,
        GLX_BLUE_SIZE, 1,
        GLX_ALPHA_SIZE, 1,
        0
    };

    Display* display = sharedX11Display();
    XVisualInfo* visualInfo = glXChooseVisual(display, DefaultScreen(display), visualAttributes);
    if (!visualInfo)
        return nullptr;

    GLXContext context = glXCreateContext(display, visualInfo, sharingContext, GL_TRUE);
    if (!context) {
        XFree(visualInfo);
        return nullptr;
    }

    Pixmap pixmap = XCreatePixmap(display, DefaultRootWindow(display), 1, 1, visualInfo->depth);
    if (!pixmap) {
        XFree(visualInfo);
        return nullptr;
    }

    GLXPixmap glxPixmap = glXCreateGLXPixmap(display, visualInfo, pixmap);
    if (!glxPixmap) {
        XFreePixmap(display, pixmap);
        XFree(visualInfo);
        return nullptr;
    }

    XFree(visualInfo);
    return adoptPtr(new GLContextGLX(context, pixmap, glxPixmap));
}

PassOwnPtr<GLContextGLX> GLContextGLX::createContext(XID window, GLContext* sharingContext)
{
    if (!sharedX11Display())
        return nullptr;

    static bool initialized = false;
    static bool success = true;
    if (!initialized) {
        success = initializeOpenGLShims();
        initialized = true;
    }
    if (!success)
        return nullptr;

    GLXContext glxSharingContext = sharingContext ? static_cast<GLContextGLX*>(sharingContext)->m_context : 0;
    OwnPtr<GLContextGLX> context = window ? createWindowContext(window, sharingContext) : nullptr;
    if (!context)
        context = createPbufferContext(glxSharingContext);
    if (!context)
        context = createPixmapContext(glxSharingContext);
    if (!context)
        return nullptr;

    return context.release();
}

GLContextGLX::GLContextGLX(GLXContext context)
    : m_context(context)
    , m_window(0)
    , m_pbuffer(0)
    , m_pixmap(0)
    , m_glxPixmap(0)
    , m_cairoDevice(0)
{
}

GLContextGLX::GLContextGLX(GLXContext context, Pixmap pixmap, GLXPixmap glxPixmap)
    : m_context(context)
    , m_window(0)
    , m_pbuffer(0)
    , m_pixmap(pixmap)
    , m_glxPixmap(glxPixmap)
    , m_cairoDevice(0)
{
}

GLContextGLX::~GLContextGLX()
{
    if (m_cairoDevice)
        cairo_device_destroy(m_cairoDevice);

    if (m_context) {
        // This may be necessary to prevent crashes with NVidia's closed source drivers. Originally
        // from Mozilla's 3D canvas implementation at: http://bitbucket.org/ilmari/canvas3d/
        glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
        glXMakeCurrent(sharedX11Display(), None, None);
        glXDestroyContext(sharedX11Display(), m_context);
    }

    if (m_pbuffer) {
        glXDestroyPbuffer(sharedX11Display(), m_pbuffer);
        m_pbuffer = 0;
    }
    if (m_glxPixmap) {
        glXDestroyGLXPixmap(sharedX11Display(), m_glxPixmap);
        m_glxPixmap = 0;
    }
    if (m_pixmap) {
        XFreePixmap(sharedX11Display(), m_pixmap);
        m_pixmap = 0;
    }
}

bool GLContextGLX::canRenderToDefaultFramebuffer()
{
    return m_window;
}

IntSize GLContextGLX::defaultFrameBufferSize()
{
    if (!canRenderToDefaultFramebuffer() || !m_window)
        return IntSize();

    int x, y;
    Window rootWindow;
    unsigned int width, height, borderWidth, depth;
    if (!XGetGeometry(sharedX11Display(), m_window, &rootWindow, &x, &y, &width, &height, &borderWidth, &depth))
        return IntSize();

    return IntSize(width, height);
}

bool GLContextGLX::makeContextCurrent()
{
    ASSERT(m_context && (m_window || m_pbuffer || m_glxPixmap));

    GLContext::makeContextCurrent();
    if (glXGetCurrentContext() == m_context)
        return true;

    if (m_window)
        return glXMakeCurrent(sharedX11Display(), m_window, m_context);

    if (m_pbuffer)
        return glXMakeCurrent(sharedX11Display(), m_pbuffer, m_context);

    return ::glXMakeCurrent(sharedX11Display(), m_glxPixmap, m_context);
}

void GLContextGLX::swapBuffers()
{
    if (m_window)
        glXSwapBuffers(sharedX11Display(), m_window);
}

void GLContextGLX::waitNative()
{
    glXWaitX();
}

cairo_device_t* GLContextGLX::cairoDevice()
{
    if (m_cairoDevice)
        return m_cairoDevice;

#if ENABLE(ACCELERATED_2D_CANVAS)
    m_cairoDevice = cairo_glx_device_create(sharedX11Display(), m_context);
#endif

    return m_cairoDevice;
}

#if USE(3D_GRAPHICS)
PlatformGraphicsContext3D GLContextGLX::platformContext()
{
    return m_context;
}
#endif

} // namespace WebCore

#endif // USE(GLX)
