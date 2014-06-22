/*
 * Copyright (C) 2012 Igalia, S.L.
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
#include "GLContextEGL.h"

#if USE(EGL)

#include "GraphicsContext3D.h"
#include <wtf/OwnPtr.h>

#if USE(CAIRO)
#include <cairo.h>
#endif

#if USE(OPENGL_ES_2)
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#else
#include "OpenGLShims.h"
#endif

#if ENABLE(ACCELERATED_2D_CANVAS)
#include <cairo-gl.h>
#endif

namespace WebCore {

static EGLDisplay gSharedEGLDisplay = EGL_NO_DISPLAY;

#if USE(OPENGL_ES_2)
static const EGLenum gGLAPI = EGL_OPENGL_ES_API;
#else
static const EGLenum gGLAPI = EGL_OPENGL_API;
#endif

static EGLDisplay sharedEGLDisplay()
{
    static bool initialized = false;
    if (!initialized) {
        initialized = true;
#if PLATFORM(X11)
        gSharedEGLDisplay = eglGetDisplay(GLContext::sharedX11Display());
#else
        gSharedEGLDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
#endif
        if (gSharedEGLDisplay != EGL_NO_DISPLAY && (!eglInitialize(gSharedEGLDisplay, 0, 0) || !eglBindAPI(gGLAPI)))
            gSharedEGLDisplay = EGL_NO_DISPLAY;
    }
    return gSharedEGLDisplay;
}

static const EGLint gContextAttributes[] = {
#if USE(OPENGL_ES_2)
    EGL_CONTEXT_CLIENT_VERSION, 2,
#endif
    EGL_NONE
};

static bool getEGLConfig(EGLConfig* config, GLContextEGL::EGLSurfaceType surfaceType)
{
    EGLint attributeList[] = {
#if USE(OPENGL_ES_2)
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
#else
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
#endif
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_STENCIL_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_SURFACE_TYPE, EGL_NONE,
        EGL_NONE
    };

    switch (surfaceType) {
    case GLContextEGL::PbufferSurface:
        attributeList[13] = EGL_PBUFFER_BIT;
        break;
    case GLContextEGL::PixmapSurface:
        attributeList[13] = EGL_PIXMAP_BIT;
        break;
    case GLContextEGL::WindowSurface:
        attributeList[13] = EGL_WINDOW_BIT;
        break;
    }

    EGLint numberConfigsReturned;
    return eglChooseConfig(sharedEGLDisplay(), attributeList, config, 1, &numberConfigsReturned) && numberConfigsReturned;
}

PassOwnPtr<GLContextEGL> GLContextEGL::createWindowContext(EGLNativeWindowType window, GLContext* sharingContext)
{
    EGLContext eglSharingContext = sharingContext ? static_cast<GLContextEGL*>(sharingContext)->m_context : 0;

    EGLDisplay display = sharedEGLDisplay();
    if (display == EGL_NO_DISPLAY)
        return nullptr;

    EGLConfig config;
    if (!getEGLConfig(&config, WindowSurface))
        return nullptr;

    EGLContext context = eglCreateContext(display, config, eglSharingContext, gContextAttributes);
    if (context == EGL_NO_CONTEXT)
        return nullptr;

    EGLSurface surface = eglCreateWindowSurface(display, config, window, 0);
    if (surface == EGL_NO_SURFACE)
        return nullptr;

    return adoptPtr(new GLContextEGL(context, surface, WindowSurface));
}

PassOwnPtr<GLContextEGL> GLContextEGL::createPbufferContext(EGLContext sharingContext)
{
    EGLDisplay display = sharedEGLDisplay();
    if (display == EGL_NO_DISPLAY)
        return nullptr;

    EGLConfig config;
    if (!getEGLConfig(&config, PbufferSurface))
        return nullptr;

    EGLContext context = eglCreateContext(display, config, sharingContext, gContextAttributes);
    if (context == EGL_NO_CONTEXT)
        return nullptr;

    static const int pbufferAttributes[] = { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };
    EGLSurface surface = eglCreatePbufferSurface(display, config, pbufferAttributes);
    if (surface == EGL_NO_SURFACE) {
        eglDestroyContext(display, context);
        return nullptr;
    }

    return adoptPtr(new GLContextEGL(context, surface, PbufferSurface));
}

PassOwnPtr<GLContextEGL> GLContextEGL::createPixmapContext(EGLContext sharingContext)
{
#if PLATFORM(X11)
    EGLDisplay display = sharedEGLDisplay();
    if (display == EGL_NO_DISPLAY)
        return nullptr;

    EGLConfig config;
    if (!getEGLConfig(&config, PixmapSurface))
        return nullptr;

    EGLContext context = eglCreateContext(display, config, sharingContext, gContextAttributes);
    if (context == EGL_NO_CONTEXT)
        return nullptr;

    EGLint depth;
    if (!eglGetConfigAttrib(display, config, EGL_DEPTH_SIZE, &depth))
        return nullptr;

    Pixmap pixmap = XCreatePixmap(sharedX11Display(), DefaultRootWindow(sharedX11Display()), 1, 1, depth);
    if (!pixmap)
        return nullptr;

    EGLSurface surface = eglCreatePixmapSurface(display, config, pixmap, 0);

    if (surface == EGL_NO_SURFACE)
        return nullptr;

    return adoptPtr(new GLContextEGL(context, surface, PixmapSurface));
#else
    return nullptr;
#endif
}

PassOwnPtr<GLContextEGL> GLContextEGL::createContext(EGLNativeWindowType window, GLContext* sharingContext)
{
    if (!sharedEGLDisplay())
        return nullptr;

    static bool initialized = false;
    static bool success = true;
    if (!initialized) {
#if !USE(OPENGL_ES_2)
        success = initializeOpenGLShims();
#endif
        initialized = true;
    }
    if (!success)
        return nullptr;

    EGLContext eglSharingContext = sharingContext ? static_cast<GLContextEGL*>(sharingContext)->m_context : 0;
    OwnPtr<GLContextEGL> context = window ? createWindowContext(window, sharingContext) : nullptr;
    if (!context)
        context = createPixmapContext(eglSharingContext);

    if (!context)
        context = createPbufferContext(eglSharingContext);
    
    return context.release();
}

GLContextEGL::GLContextEGL(EGLContext context, EGLSurface surface, EGLSurfaceType type)
    : m_context(context)
    , m_surface(surface)
    , m_type(type)
    , m_cairoDevice(0)
{
}

GLContextEGL::~GLContextEGL()
{
#if USE(CAIRO)
    if (m_cairoDevice)
        cairo_device_destroy(m_cairoDevice);
#endif

    EGLDisplay display = sharedEGLDisplay();
    if (m_context) {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroyContext(display, m_context);
    }

    if (m_surface)
        eglDestroySurface(display, m_surface);
}

bool GLContextEGL::canRenderToDefaultFramebuffer()
{
    return m_type == WindowSurface;
}

IntSize GLContextEGL::defaultFrameBufferSize()
{
    if (!canRenderToDefaultFramebuffer())
        return IntSize();

    EGLint width, height;
    if (!eglQuerySurface(sharedEGLDisplay(), m_surface, EGL_WIDTH, &width)
        || !eglQuerySurface(sharedEGLDisplay(), m_surface, EGL_HEIGHT, &height))
        return IntSize();

    return IntSize(width, height);
}

bool GLContextEGL::makeContextCurrent()
{
    ASSERT(m_context && m_surface);

    GLContext::makeContextCurrent();
    if (eglGetCurrentContext() == m_context)
        return true;

    return eglMakeCurrent(sharedEGLDisplay(), m_surface, m_surface, m_context);
}

void GLContextEGL::swapBuffers()
{
    ASSERT(m_surface);
    eglSwapBuffers(sharedEGLDisplay(), m_surface);
}

void GLContextEGL::waitNative()
{
    eglWaitNative(EGL_CORE_NATIVE_ENGINE);
}

cairo_device_t* GLContextEGL::cairoDevice()
{
    if (m_cairoDevice)
        return m_cairoDevice;

#if ENABLE(ACCELERATED_2D_CANVAS)
    m_cairoDevice = cairo_egl_device_create(sharedEGLDisplay(), m_context);
#endif

    return m_cairoDevice;
}

#if ENABLE(WEBGL)
PlatformGraphicsContext3D GLContextEGL::platformContext()
{
    return m_context;
}
#endif

} // namespace WebCore

#endif // USE(EGL)
