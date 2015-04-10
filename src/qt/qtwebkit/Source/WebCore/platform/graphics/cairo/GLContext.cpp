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
#include "GLContext.h"

#if USE(OPENGL)

#if USE(EGL)
#include "GLContextEGL.h"
#endif

#if USE(GLX)
#include "GLContextGLX.h"
#endif

#include <wtf/ThreadSpecific.h>

#if PLATFORM(X11)
#include <X11/Xlib.h>
#endif

#if PLATFORM(GTK)
#include <gdk/gdk.h>
#ifndef GTK_API_VERSION_2
#ifdef GDK_WINDOWING_WAYLAND
#include <gdk/gdkwayland.h>
#endif
#endif
#endif

using WTF::ThreadSpecific;

namespace WebCore {

class ThreadGlobalGLContext {
public:
    static ThreadSpecific<ThreadGlobalGLContext>* staticGLContext;

    void setContext(GLContext* context) { m_context = context; }
    GLContext* context() { return m_context; }

private:
    GLContext* m_context;
};

ThreadSpecific<ThreadGlobalGLContext>* ThreadGlobalGLContext::staticGLContext;

inline ThreadGlobalGLContext* currentContext()
{
    if (!ThreadGlobalGLContext::staticGLContext)
        ThreadGlobalGLContext::staticGLContext = new ThreadSpecific<ThreadGlobalGLContext>;
    return *ThreadGlobalGLContext::staticGLContext;
}

GLContext* GLContext::sharingContext()
{
    DEFINE_STATIC_LOCAL(OwnPtr<GLContext>, sharing, (createOffscreenContext()));
    return sharing.get();
}

#if PLATFORM(X11)
// We do not want to call glXMakeContextCurrent using different Display pointers,
// because it might lead to crashes in some drivers (fglrx). We use a shared display
// pointer here.
static Display* gSharedX11Display = 0;
Display* GLContext::sharedX11Display()
{
    if (!gSharedX11Display)
        gSharedX11Display = XOpenDisplay(0);
    return gSharedX11Display;
}

void GLContext::cleanupSharedX11Display()
{
    if (!gSharedX11Display)
        return;
    XCloseDisplay(gSharedX11Display);
    gSharedX11Display = 0;
}
#endif // PLATFORM(X11)

// Because of driver bugs, exiting the program when there are active pbuffers
// can crash the X server (this has been observed with the official Nvidia drivers).
// We need to ensure that we clean everything up on exit. There are several reasons
// that GraphicsContext3Ds will still be alive at exit, including user error (memory
// leaks) and the page cache. In any case, we don't want the X server to crash.
typedef Vector<GLContext*> ActiveContextList;
static ActiveContextList& activeContextList()
{
    DEFINE_STATIC_LOCAL(ActiveContextList, activeContexts, ());
    return activeContexts;
}

void GLContext::addActiveContext(GLContext* context)
{
    static bool addedAtExitHandler = false;
    if (!addedAtExitHandler) {
        atexit(&GLContext::cleanupActiveContextsAtExit);
        addedAtExitHandler = true;
    }
    activeContextList().append(context);
}

static bool gCleaningUpAtExit = false;

void GLContext::removeActiveContext(GLContext* context)
{
    // If we are cleaning up the context list at exit, don't bother removing the context
    // from the list, since we don't want to modify the list while it's being iterated.
    if (gCleaningUpAtExit)
        return;

    ActiveContextList& contextList = activeContextList();
    size_t i = contextList.find(context);
    if (i != notFound)
        contextList.remove(i);
}

void GLContext::cleanupActiveContextsAtExit()
{
    gCleaningUpAtExit = true;

    ActiveContextList& contextList = activeContextList();
    for (size_t i = 0; i < contextList.size(); ++i)
        delete contextList[i];

#if PLATFORM(X11)
    cleanupSharedX11Display();
#endif
}



PassOwnPtr<GLContext> GLContext::createContextForWindow(uint64_t windowHandle, GLContext* sharingContext)
{
#if PLATFORM(GTK) && defined(GDK_WINDOWING_WAYLAND) && USE(EGL)
    GdkDisplay* display = gdk_display_manager_get_default_display(gdk_display_manager_get());

    if (GDK_IS_WAYLAND_DISPLAY(display)) {
        if (OwnPtr<GLContext> eglContext = GLContextEGL::createContext(windowHandle, sharingContext))
            return eglContext.release();
        return nullptr;
    }
#endif

#if USE(GLX)
    if (OwnPtr<GLContext> glxContext = GLContextGLX::createContext(windowHandle, sharingContext))
        return glxContext.release();
#endif
#if USE(EGL)
    if (OwnPtr<GLContext> eglContext = GLContextEGL::createContext(windowHandle, sharingContext))
        return eglContext.release();
#endif
    return nullptr;
}

GLContext::GLContext()
{
    addActiveContext(this);
}

PassOwnPtr<GLContext> GLContext::createOffscreenContext(GLContext* sharingContext)
{
    return createContextForWindow(0, sharingContext);
}

GLContext::~GLContext()
{
    if (this == currentContext()->context())
        currentContext()->setContext(0);
    removeActiveContext(this);
}

bool GLContext::makeContextCurrent()
{
    currentContext()->setContext(this);
    return true;
}

GLContext* GLContext::getCurrent()
{
    return currentContext()->context();
}

} // namespace WebCore

#endif // USE(OPENGL)

