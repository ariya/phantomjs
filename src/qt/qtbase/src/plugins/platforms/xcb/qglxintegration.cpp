/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QDebug>
#include <QLibrary>

#include "qxcbwindow.h"
#include "qxcbscreen.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/glx.h>

#include <QtGui/QOpenGLContext>
#include <QtGui/QOffscreenSurface>

#include "qglxintegration.h"
#include <QtPlatformSupport/private/qglxconvenience_p.h>
#include <QtPlatformHeaders/QGLXNativeContext>

#if defined(Q_OS_LINUX) || defined(Q_OS_BSD4)
#include <dlfcn.h>
#endif

QT_BEGIN_NAMESPACE

typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);

#ifndef GLX_CONTEXT_CORE_PROFILE_BIT_ARB
#define GLX_CONTEXT_CORE_PROFILE_BIT_ARB 0x00000001
#endif

#ifndef GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB
#define GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002
#endif

#ifndef GLX_CONTEXT_ES2_PROFILE_BIT_EXT
#define GLX_CONTEXT_ES2_PROFILE_BIT_EXT 0x00000004
#endif

#ifndef GLX_CONTEXT_PROFILE_MASK_ARB
#define GLX_CONTEXT_PROFILE_MASK_ARB 0x9126
#endif

#ifndef GL_CONTEXT_FLAG_DEBUG_BIT
#define GL_CONTEXT_FLAG_DEBUG_BIT 0x00000002
#endif

static Window createDummyWindow(Display *dpy, XVisualInfo *visualInfo, int screenNumber, Window rootWin)
{
    Colormap cmap = XCreateColormap(dpy, rootWin, visualInfo->visual, AllocNone);
    XSetWindowAttributes a;
    a.background_pixel = WhitePixel(dpy, screenNumber);
    a.border_pixel = BlackPixel(dpy, screenNumber);
    a.colormap = cmap;
    a.override_redirect = true;

    Window window = XCreateWindow(dpy, rootWin,
                                  0, 0, 100, 100,
                                  0, visualInfo->depth, InputOutput, visualInfo->visual,
                                  CWBackPixel|CWBorderPixel|CWColormap|CWOverrideRedirect, &a);
#ifndef QT_NO_DEBUG
    XStoreName(dpy, window, "Qt GLX dummy window");
#endif
    XFreeColormap(dpy, cmap);
    return window;
}

static Window createDummyWindow(Display *dpy, GLXFBConfig config, int screenNumber, Window rootWin)
{
    XVisualInfo *visualInfo = glXGetVisualFromFBConfig(dpy, config);
    if (!visualInfo)
        qFatal("Could not initialize GLX");
    Window window = createDummyWindow(dpy, visualInfo, screenNumber, rootWin);
    XFree(visualInfo);
    return window;
}

static inline QByteArray getGlString(GLenum param)
{
    if (const GLubyte *s = glGetString(param))
        return QByteArray(reinterpret_cast<const char*>(s));
    return QByteArray();
}

static void updateFormatFromContext(QSurfaceFormat &format)
{
    // Update the version, profile, and context bit of the format
    int major = 0, minor = 0;
    QByteArray versionString(getGlString(GL_VERSION));
    if (QPlatformOpenGLContext::parseOpenGLVersion(versionString, major, minor)) {
        format.setMajorVersion(major);
        format.setMinorVersion(minor);
    }

    format.setProfile(QSurfaceFormat::NoProfile);
    format.setOptions(QSurfaceFormat::FormatOptions());

    if (format.renderableType() == QSurfaceFormat::OpenGL) {
        if (format.version() < qMakePair(3, 0)) {
            format.setOption(QSurfaceFormat::DeprecatedFunctions);
            return;
        }

        // Version 3.0 onwards - check if it includes deprecated functionality or is
        // a debug context
        GLint value = 0;
        glGetIntegerv(GL_CONTEXT_FLAGS, &value);
        if (!(value & GL_CONTEXT_FLAG_FORWARD_COMPATIBLE_BIT))
            format.setOption(QSurfaceFormat::DeprecatedFunctions);
        if (value & GL_CONTEXT_FLAG_DEBUG_BIT)
            format.setOption(QSurfaceFormat::DebugContext);
        if (format.version() < qMakePair(3, 2))
            return;

        // Version 3.2 and newer have a profile
        value = 0;
        glGetIntegerv(GL_CONTEXT_PROFILE_MASK, &value);

        if (value & GL_CONTEXT_CORE_PROFILE_BIT)
            format.setProfile(QSurfaceFormat::CoreProfile);
        else if (value & GL_CONTEXT_COMPATIBILITY_PROFILE_BIT)
            format.setProfile(QSurfaceFormat::CompatibilityProfile);
    }
}

QGLXContext::QGLXContext(QXcbScreen *screen, const QSurfaceFormat &format, QPlatformOpenGLContext *share,
                         const QVariant &nativeHandle)
    : QPlatformOpenGLContext()
    , m_display(DISPLAY_FROM_XCB(screen))
    , m_config(0)
    , m_context(0)
    , m_shareContext(0)
    , m_format(format)
    , m_isPBufferCurrent(false)
    , m_swapInterval(-1)
    , m_ownsContext(nativeHandle.isNull())
{
    if (nativeHandle.isNull())
        init(screen, share);
    else
        init(screen, share, nativeHandle);
}

void QGLXContext::init(QXcbScreen *screen, QPlatformOpenGLContext *share)
{
    if (m_format.renderableType() == QSurfaceFormat::DefaultRenderableType)
        m_format.setRenderableType(QSurfaceFormat::OpenGL);
    if (m_format.renderableType() != QSurfaceFormat::OpenGL && m_format.renderableType() != QSurfaceFormat::OpenGLES)
        return;

    if (share)
        m_shareContext = static_cast<const QGLXContext*>(share)->glxContext();

    GLXFBConfig config = qglx_findConfig(DISPLAY_FROM_XCB(screen),screen->screenNumber(),m_format);
    m_config = config;
    XVisualInfo *visualInfo = 0;
    Window window = 0; // Temporary window used to query OpenGL context

    if (config) {
        // Resolve entry point for glXCreateContextAttribsARB
        glXCreateContextAttribsARBProc glXCreateContextAttribsARB = 0;
        glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc) glXGetProcAddress((const GLubyte*)"glXCreateContextAttribsARB");

        QList<QByteArray> glxExt = QByteArray(glXQueryExtensionsString(m_display, screen->screenNumber())).split(' ');
        bool supportsProfiles = glxExt.contains("GLX_ARB_create_context_profile");

        // Use glXCreateContextAttribsARB if available
        // Also, GL ES context creation requires GLX_EXT_create_context_es2_profile
        if (glxExt.contains("GLX_ARB_create_context") && glXCreateContextAttribsARB != 0
                && (m_format.renderableType() != QSurfaceFormat::OpenGLES || (supportsProfiles && glxExt.contains("GLX_EXT_create_context_es2_profile")))) {
            // Try to create an OpenGL context for each known OpenGL version in descending
            // order from the requested version.
            const int requestedVersion = m_format.majorVersion() * 10 + qMin(m_format.minorVersion(), 9);

            QVector<int> glVersions;
            if (m_format.renderableType() == QSurfaceFormat::OpenGL) {
                if (requestedVersion > 45)
                    glVersions << requestedVersion;

                // Don't bother with versions below 2.0
                glVersions << 45 << 44 << 43 << 42 << 41 << 40 << 33 << 32 << 31 << 30 << 21 << 20;
            } else if (m_format.renderableType() == QSurfaceFormat::OpenGLES) {
                if (requestedVersion > 31)
                    glVersions << requestedVersion;

                // Don't bother with versions below ES 2.0
                glVersions << 31 << 30 << 20;
                // ES does not support any format option
                m_format.setOptions(QSurfaceFormat::FormatOptions());
            }

            Q_ASSERT(glVersions.count() > 0);

            for (int i = 0; !m_context && i < glVersions.count(); i++) {
                const int version = glVersions[i];
                if (version > requestedVersion)
                    continue;

                const int majorVersion = version / 10;
                const int minorVersion = version % 10;

                QVector<int> contextAttributes;
                contextAttributes << GLX_CONTEXT_MAJOR_VERSION_ARB << majorVersion
                                  << GLX_CONTEXT_MINOR_VERSION_ARB << minorVersion;


                if (m_format.renderableType() == QSurfaceFormat::OpenGL) {
                    // If asking for OpenGL 3.2 or newer we should also specify a profile
                    if (version >= 32 && supportsProfiles) {
                        if (m_format.profile() == QSurfaceFormat::CoreProfile)
                            contextAttributes << GLX_CONTEXT_PROFILE_MASK_ARB << GLX_CONTEXT_CORE_PROFILE_BIT_ARB;
                        else
                            contextAttributes << GLX_CONTEXT_PROFILE_MASK_ARB << GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB;
                    }

                    int flags = 0;

                    if (m_format.testOption(QSurfaceFormat::DebugContext))
                        flags |= GLX_CONTEXT_DEBUG_BIT_ARB;

                    // A forward-compatible context may be requested for 3.0 and later
                    if (version >= 30 && !m_format.testOption(QSurfaceFormat::DeprecatedFunctions))
                        flags |= GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;

                    if (flags != 0)
                        contextAttributes << GLX_CONTEXT_FLAGS_ARB << flags;
                } else if (m_format.renderableType() == QSurfaceFormat::OpenGLES) {
                    contextAttributes << GLX_CONTEXT_PROFILE_MASK_ARB << GLX_CONTEXT_ES2_PROFILE_BIT_EXT;
                }

                contextAttributes << None;

                m_context = glXCreateContextAttribsARB(m_display, config, m_shareContext, true, contextAttributes.data());
                if (!m_context && m_shareContext) {
                    // re-try without a shared glx context
                    m_context = glXCreateContextAttribsARB(m_display, config, 0, true, contextAttributes.data());
                    if (m_context)
                        m_shareContext = 0;
                }
            }
        }

        // Could not create a context using glXCreateContextAttribsARB, falling back to glXCreateNewContext.
        if (!m_context) {
            // requesting an OpenGL ES context requires glXCreateContextAttribsARB, so bail out
            if (m_format.renderableType() == QSurfaceFormat::OpenGLES)
                return;

            m_context = glXCreateNewContext(m_display, config, GLX_RGBA_TYPE, m_shareContext, true);
            if (!m_context && m_shareContext) {
                // re-try without a shared glx context
                m_context = glXCreateNewContext(m_display, config, GLX_RGBA_TYPE, 0, true);
                if (m_context)
                    m_shareContext = 0;
            }
        }

        // Get the basic surface format details
        if (m_context)
            qglx_surfaceFormatFromGLXFBConfig(&m_format, DISPLAY_FROM_XCB(screen), config);

        // Create a temporary window so that we can make the new context current
        window = createDummyWindow(DISPLAY_FROM_XCB(screen), config, screen->screenNumber(), screen->root());
    } else {
        // requesting an OpenGL ES context requires glXCreateContextAttribsARB, so bail out
        if (m_format.renderableType() == QSurfaceFormat::OpenGLES)
            return;

        // Note that m_format gets updated with the used surface format
        visualInfo = qglx_findVisualInfo(m_display, screen->screenNumber(), &m_format);
        if (!visualInfo)
            qFatal("Could not initialize GLX");
        m_context = glXCreateContext(m_display, visualInfo, m_shareContext, true);
        if (!m_context && m_shareContext) {
            // re-try without a shared glx context
            m_shareContext = 0;
            m_context = glXCreateContext(m_display, visualInfo, Q_NULLPTR, true);
        }

        // Create a temporary window so that we can make the new context current
        window = createDummyWindow(DISPLAY_FROM_XCB(screen), visualInfo, screen->screenNumber(), screen->root());
        XFree(visualInfo);
    }

    // Query the OpenGL version and profile
    if (m_context && window) {
        GLXContext prevContext = glXGetCurrentContext();
        GLXDrawable prevDrawable = glXGetCurrentDrawable();
        glXMakeCurrent(m_display, window, m_context);
        updateFormatFromContext(m_format);

        // Make our context non-current
        glXMakeCurrent(m_display, prevDrawable, prevContext);
    }

    // Destroy our temporary window
    XDestroyWindow(m_display, window);
}

void QGLXContext::init(QXcbScreen *screen, QPlatformOpenGLContext *share, const QVariant &nativeHandle)
{
    if (!nativeHandle.canConvert<QGLXNativeContext>()) {
        qWarning("QGLXContext: Requires a QGLXNativeContext");
        return;
    }
    QGLXNativeContext handle = nativeHandle.value<QGLXNativeContext>();
    GLXContext context = handle.context();
    if (!context) {
        qWarning("QGLXContext: No GLXContext given");
        return;
    }

    // Use the provided Display, if available. If not, use our own. It may still work.
    Display *dpy = handle.display();
    if (!dpy)
        dpy = DISPLAY_FROM_XCB(screen);

    // Legacy contexts created using glXCreateContext are created using a visual
    // and the FBConfig cannot be queried. The only way to adapt these contexts
    // is to figure out the visual id.
    XVisualInfo *vinfo = 0;
    // If the VisualID is provided use it.
    VisualID vid = handle.visualId();
    if (!vid) {
        // In the absence of the VisualID figure it out from the window.
        Window wnd = handle.window();
        if (wnd) {
            XWindowAttributes attrs;
            XGetWindowAttributes(dpy, wnd, &attrs);
            vid = XVisualIDFromVisual(attrs.visual);
        }
    }
    if (vid) {
        XVisualInfo v;
        v.screen = screen->screenNumber();
        v.visualid = vid;
        int n = 0;
        vinfo = XGetVisualInfo(dpy, VisualScreenMask | VisualIDMask, &v, &n);
        if (n < 1) {
            XFree(vinfo);
            vinfo = 0;
        }
    }

    // For contexts created with an FBConfig using the modern functions providing the
    // visual or window is not mandatory. Just query the config from the context.
    GLXFBConfig config = 0;
    if (!vinfo) {
        int configId = 0;
        if (glXQueryContext(dpy, context, GLX_FBCONFIG_ID, &configId) != Success) {
            qWarning("QGLXContext: Failed to query config from the provided context");
            return;
        }

        GLXFBConfig *configs;
        int numConfigs = 0;
        static const int attribs[] = { GLX_FBCONFIG_ID, configId, None };
        configs = glXChooseFBConfig(dpy, screen->screenNumber(), attribs, &numConfigs);
        if (!configs || numConfigs < 1) {
            qWarning("QGLXContext: Failed to find config");
            return;
        }
        if (configs && numConfigs > 1) // this is suspicious so warn but let it continue
            qWarning("QGLXContext: Multiple configs for FBConfig ID %d", configId);

        config = configs[0];
        // Store the config.
        m_config = config;
    }

    Q_ASSERT(vinfo || config);

    int screenNumber = DefaultScreen(dpy);
    Window window;
    if (vinfo)
        window = createDummyWindow(dpy, vinfo, screenNumber, RootWindow(dpy, screenNumber));
    else
        window = createDummyWindow(dpy, config, screenNumber, RootWindow(dpy, screenNumber));
    if (!window) {
        qWarning("QGLXContext: Failed to create dummy window");
        return;
    }

    // Update OpenGL version and buffer sizes in our format.
    GLXContext prevContext = glXGetCurrentContext();
    GLXDrawable prevDrawable = glXGetCurrentDrawable();
    if (!glXMakeCurrent(dpy, window, context)) {
        qWarning("QGLXContext: Failed to make provided context current");
        return;
    }
    m_format = QSurfaceFormat();
    m_format.setRenderableType(QOpenGLContext::openGLModuleType() == QOpenGLContext::LibGL
                               ? QSurfaceFormat::OpenGL : QSurfaceFormat::OpenGLES);
    updateFormatFromContext(m_format);
    if (vinfo)
        qglx_surfaceFormatFromVisualInfo(&m_format, dpy, vinfo);
    else
        qglx_surfaceFormatFromGLXFBConfig(&m_format, dpy, config);
    glXMakeCurrent(dpy, prevDrawable, prevContext);
    XDestroyWindow(dpy, window);

    if (vinfo)
        XFree(vinfo);

    // Success. Store the context. From this point on isValid() is true.
    m_context = context;

    if (share)
        m_shareContext = static_cast<const QGLXContext*>(share)->glxContext();
}

QGLXContext::~QGLXContext()
{
    if (m_ownsContext)
        glXDestroyContext(m_display, m_context);
}

static QXcbScreen *screenForPlatformSurface(QPlatformSurface *surface)
{
    QSurface::SurfaceClass surfaceClass = surface->surface()->surfaceClass();
    if (surfaceClass == QSurface::Window) {
        return static_cast<QXcbScreen *>(static_cast<QXcbWindow *>(surface)->screen());
    } else if (surfaceClass == QSurface::Offscreen) {
        return static_cast<QXcbScreen *>(static_cast<QGLXPbuffer *>(surface)->screen());
    }
    return Q_NULLPTR;
}

QVariant QGLXContext::nativeHandle() const
{
    return QVariant::fromValue<QGLXNativeContext>(QGLXNativeContext(m_context));
}

bool QGLXContext::makeCurrent(QPlatformSurface *surface)
{
    bool success = false;
    Q_ASSERT(surface->surface()->supportsOpenGL());

    GLXDrawable glxDrawable = 0;
    QSurface::SurfaceClass surfaceClass = surface->surface()->surfaceClass();
    if (surfaceClass == QSurface::Window) {
        m_isPBufferCurrent = false;
        QXcbWindow *window = static_cast<QXcbWindow *>(surface);
        glxDrawable = window->xcb_window();
        success = glXMakeCurrent(m_display, glxDrawable, m_context);
    } else if (surfaceClass == QSurface::Offscreen) {
        m_isPBufferCurrent = true;
        QGLXPbuffer *pbuffer = static_cast<QGLXPbuffer *>(surface);
        glxDrawable = pbuffer->pbuffer();
        success = glXMakeContextCurrent(m_display, glxDrawable, glxDrawable, m_context);
    }

    if (success) {
        int interval = surface->format().swapInterval();
        QXcbScreen *screen = screenForPlatformSurface(surface);
        if (interval >= 0 && m_swapInterval != interval && screen) {
            m_swapInterval = interval;
            typedef void (*qt_glXSwapIntervalEXT)(Display *, GLXDrawable, int);
            typedef void (*qt_glXSwapIntervalMESA)(unsigned int);
            static qt_glXSwapIntervalEXT glXSwapIntervalEXT = 0;
            static qt_glXSwapIntervalMESA glXSwapIntervalMESA = 0;
            static bool resolved = false;
            if (!resolved) {
                resolved = true;
                QList<QByteArray> glxExt = QByteArray(glXQueryExtensionsString(m_display,
                                                                               screen->screenNumber())).split(' ');
                if (glxExt.contains("GLX_EXT_swap_control"))
                    glXSwapIntervalEXT = (qt_glXSwapIntervalEXT) getProcAddress("glXSwapIntervalEXT");
                if (glxExt.contains("GLX_MESA_swap_control"))
                    glXSwapIntervalMESA = (qt_glXSwapIntervalMESA) getProcAddress("glXSwapIntervalMESA");
            }
            if (glXSwapIntervalEXT)
                glXSwapIntervalEXT(m_display, glxDrawable, interval);
            else if (glXSwapIntervalMESA)
                glXSwapIntervalMESA(interval);
        }
    }

    return success;
}

void QGLXContext::doneCurrent()
{
    if (m_isPBufferCurrent)
        glXMakeContextCurrent(m_display, 0, 0, 0);
    else
        glXMakeCurrent(m_display, 0, 0);
    m_isPBufferCurrent = false;
}

void QGLXContext::swapBuffers(QPlatformSurface *surface)
{
    GLXDrawable glxDrawable = 0;
    if (surface->surface()->surfaceClass() == QSurface::Offscreen)
        glxDrawable = static_cast<QGLXPbuffer *>(surface)->pbuffer();
    else
        glxDrawable = static_cast<QXcbWindow *>(surface)->xcb_window();
    glXSwapBuffers(m_display, glxDrawable);

    if (surface->surface()->surfaceClass() == QSurface::Window) {
        QXcbWindow *platformWindow = static_cast<QXcbWindow *>(surface);
        // OpenGL context might be bound to a non-gui thread use QueuedConnection to sync
        // the window from the platformWindow's thread as QXcbWindow is no QObject, an
        // event is sent to QXcbConnection. (this is faster than a metacall)
        if (platformWindow->needsSync())
            platformWindow->postSyncWindowRequest();
    }
}

void (*QGLXContext::getProcAddress(const QByteArray &procName)) ()
{
    typedef void *(*qt_glXGetProcAddressARB)(const GLubyte *);
    static qt_glXGetProcAddressARB glXGetProcAddressARB = 0;
    static bool resolved = false;

    if (resolved && !glXGetProcAddressARB)
        return 0;
    if (!glXGetProcAddressARB) {
        QList<QByteArray> glxExt = QByteArray(glXGetClientString(m_display, GLX_EXTENSIONS)).split(' ');
        if (glxExt.contains("GLX_ARB_get_proc_address")) {
#if defined(Q_OS_LINUX) || defined(Q_OS_BSD4)
            void *handle = dlopen(NULL, RTLD_LAZY);
            if (handle) {
                glXGetProcAddressARB = (qt_glXGetProcAddressARB) dlsym(handle, "glXGetProcAddressARB");
                dlclose(handle);
            }
            if (!glXGetProcAddressARB)
#endif
            {
                extern const QString qt_gl_library_name();
//                QLibrary lib(qt_gl_library_name());
                QLibrary lib(QLatin1String("GL"));
                glXGetProcAddressARB = (qt_glXGetProcAddressARB) lib.resolve("glXGetProcAddressARB");
            }
        }
        resolved = true;
    }
    if (!glXGetProcAddressARB)
        return 0;
    return (void (*)())glXGetProcAddressARB(reinterpret_cast<const GLubyte *>(procName.constData()));
}

QSurfaceFormat QGLXContext::format() const
{
    return m_format;
}

bool QGLXContext::isSharing() const
{
    return m_shareContext != 0;
}

bool QGLXContext::isValid() const
{
    return m_context != 0;
}

bool QGLXContext::m_queriedDummyContext = false;
bool QGLXContext::m_supportsThreading = true;


// If this list grows to any significant size, change it a
// proper string table and make the implementation below use
// binary search.
static const char *qglx_threadedgl_blacklist_renderer[] = {
    "Chromium",                             // QTBUG-32225 (initialization fails)
    0
};

// This disables threaded rendering on anything using mesa, e.g.
// - nvidia/nouveau
// - amd/gallium
// - intel
// - some software opengl implementations
//
// The client glx vendor string is used to identify those setups as that seems to show the least
// variance between the bad configurations. It's always "Mesa Project and SGI". There are some
// configurations which don't use mesa and which can do threaded rendering (amd and nvidia chips
// with their own proprietary drivers).
//
// This, of course, is very broad and disables threaded rendering on a lot of devices which would
// be able to use it. However, the bugs listed below don't follow any easily recognizable pattern
// and we should rather be safe.
//
// http://cgit.freedesktop.org/xcb/libxcb/commit/?id=be0fe56c3bcad5124dcc6c47a2fad01acd16f71a will
// fix some of the issues. Basically, the proprietary drivers seem to have a way of working around
// a fundamental flaw with multithreaded access to xcb, but mesa doesn't. The blacklist should be
// reevaluated once that patch is released in some version of xcb.
static const char *qglx_threadedgl_blacklist_vendor[] = {
    "Mesa Project and SGI",                // QTCREATORBUG-10875 (crash in creator)
                                           // QTBUG-34492 (flickering in fullscreen)
                                           // QTBUG-38221
    0
};

void QGLXContext::queryDummyContext()
{
    if (m_queriedDummyContext)
        return;
    m_queriedDummyContext = true;

    static bool skip = qEnvironmentVariableIsSet("QT_OPENGL_NO_SANITY_CHECK");
    if (skip)
        return;

    QOpenGLContext *oldContext = QOpenGLContext::currentContext();
    QSurface *oldSurface = 0;
    if (oldContext)
        oldSurface = oldContext->surface();

    QScopedPointer<QSurface> surface;
    const char *glxvendor = glXGetClientString(glXGetCurrentDisplay(), GLX_VENDOR);
    if (glxvendor && !strcmp(glxvendor, "ATI")) {
        QWindow *window = new QWindow;
        window->resize(64, 64);
        window->setSurfaceType(QSurface::OpenGLSurface);
        window->create();
        surface.reset(window);
    } else {
        QOffscreenSurface *offSurface = new QOffscreenSurface;
        offSurface->create();
        surface.reset(offSurface);
    }

    QOpenGLContext context;
    context.create();
    context.makeCurrent(surface.data());

    m_supportsThreading = true;

    if (const char *renderer = (const char *) glGetString(GL_RENDERER)) {
        for (int i = 0; qglx_threadedgl_blacklist_renderer[i]; ++i) {
            if (strstr(renderer, qglx_threadedgl_blacklist_renderer[i]) != 0) {
                m_supportsThreading = false;
                break;
            }
        }
    }

    if (glxvendor) {
        for (int i = 0; qglx_threadedgl_blacklist_vendor[i]; ++i) {
            if (strstr(glxvendor, qglx_threadedgl_blacklist_vendor[i]) != 0) {
                m_supportsThreading = false;
                break;
            }
        }
    }

    context.doneCurrent();
    if (oldContext && oldSurface)
        oldContext->makeCurrent(oldSurface);
}

bool QGLXContext::supportsThreading()
{
    if (!m_queriedDummyContext)
        queryDummyContext();
    return m_supportsThreading;
}

QGLXPbuffer::QGLXPbuffer(QOffscreenSurface *offscreenSurface)
    : QPlatformOffscreenSurface(offscreenSurface)
    , m_format(offscreenSurface->requestedFormat())
    , m_screen(static_cast<QXcbScreen *>(offscreenSurface->screen()->handle()))
    , m_pbuffer(0)
{
    GLXFBConfig config = qglx_findConfig(DISPLAY_FROM_XCB(m_screen), m_screen->screenNumber(), m_format);

    if (config) {
        const int attributes[] = {
            GLX_PBUFFER_WIDTH, offscreenSurface->size().width(),
            GLX_PBUFFER_HEIGHT, offscreenSurface->size().height(),
            GLX_LARGEST_PBUFFER, False,
            GLX_PRESERVED_CONTENTS, False,
            None
        };

        m_pbuffer = glXCreatePbuffer(DISPLAY_FROM_XCB(m_screen), config, attributes);

        if (m_pbuffer)
            qglx_surfaceFormatFromGLXFBConfig(&m_format, DISPLAY_FROM_XCB(m_screen), config);
    }
}

QGLXPbuffer::~QGLXPbuffer()
{
    if (m_pbuffer)
        glXDestroyPbuffer(DISPLAY_FROM_XCB(m_screen), m_pbuffer);
}


QT_END_NAMESPACE
