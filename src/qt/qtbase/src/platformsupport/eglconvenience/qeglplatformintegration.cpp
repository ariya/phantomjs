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

#include <QtGui/QWindow>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOffscreenSurface>
#include <QtGui/QGuiApplication>
#include <QtGui/private/qguiapplication_p.h>
#include <qpa/qwindowsysteminterface.h>
#include <qpa/qplatforminputcontextfactory_p.h>

#include <QtPlatformSupport/private/qgenericunixfontdatabase_p.h>
#include <QtPlatformSupport/private/qgenericunixservices_p.h>
#include <QtPlatformSupport/private/qgenericunixeventdispatcher_p.h>
#include <QtPlatformSupport/private/qfbvthandler_p.h>

#if !defined(QT_NO_EVDEV) && (!defined(Q_OS_ANDROID) || defined(Q_OS_ANDROID_NO_SDK))
#include <QtPlatformSupport/private/qevdevmousemanager_p.h>
#include <QtPlatformSupport/private/qevdevkeyboardmanager_p.h>
#include <QtPlatformSupport/private/qevdevtouch_p.h>
#endif

#include <QtPlatformHeaders/qeglfsfunctions.h>

#include "qeglplatformintegration_p.h"
#include "qeglplatformcontext_p.h"
#include "qeglplatformwindow_p.h"
#include "qeglplatformbackingstore_p.h"
#include "qeglplatformscreen_p.h"
#include "qeglplatformcursor_p.h"

QT_BEGIN_NAMESPACE

/*!
    \class QEGLPlatformIntegration
    \brief Base class for EGL-based QPlatformIntegration implementations.
    \since 5.2
    \internal
    \ingroup qpa

    This class provides most of the necessary platform integration for
    an EGL-based Unix system. Platform plugins must subclass this and
    reimplement the virtuals for creating platform screens and windows
    since they will most likely wish to use a subclass for these.

    The backing store, native interface accessors, font database,
    basic capability flags, etc. are provided out of the box, no
    further customization is needed.

    \note It is critical that this class' implementation of
    initialize() is called. Therefore subclasses should either avoid
    to reimplement this function or call the base class
    implementation.
 */

QEGLPlatformIntegration::QEGLPlatformIntegration()
    : m_screen(0),
      m_display(EGL_NO_DISPLAY),
      m_inputContext(0),
      m_fontDb(new QGenericUnixFontDatabase),
      m_services(new QGenericUnixServices),
      m_kbdMgr(0)
{
}

QEGLPlatformIntegration::~QEGLPlatformIntegration()
{
    delete m_screen;
    if (m_display != EGL_NO_DISPLAY)
        eglTerminate(m_display);
}

void QEGLPlatformIntegration::initialize()
{
    m_display = eglGetDisplay(nativeDisplay());
    if (m_display == EGL_NO_DISPLAY)
        qFatal("Could not open egl display");

    EGLint major, minor;
    if (!eglInitialize(m_display, &major, &minor))
        qFatal("Could not initialize egl display");

    m_screen = createScreen();
    screenAdded(m_screen);

    m_inputContext = QPlatformInputContextFactory::create();

    m_vtHandler.reset(new QFbVtHandler);
}

QAbstractEventDispatcher *QEGLPlatformIntegration::createEventDispatcher() const
{
    return createUnixEventDispatcher();
}

QPlatformServices *QEGLPlatformIntegration::services() const
{
    return m_services.data();
}

QPlatformFontDatabase *QEGLPlatformIntegration::fontDatabase() const
{
    return m_fontDb.data();
}

QPlatformBackingStore *QEGLPlatformIntegration::createPlatformBackingStore(QWindow *window) const
{
    return new QEGLPlatformBackingStore(window);
}

QPlatformWindow *QEGLPlatformIntegration::createPlatformWindow(QWindow *window) const
{
    QWindowSystemInterface::flushWindowSystemEvents();
    QEGLPlatformWindow *w = createWindow(window);
    w->create();
    if (window->type() != Qt::ToolTip)
        w->requestActivateWindow();
    return w;
}

QPlatformOpenGLContext *QEGLPlatformIntegration::createPlatformOpenGLContext(QOpenGLContext *context) const
{
    QEGLPlatformScreen *screen = static_cast<QEGLPlatformScreen *>(context->screen()->handle());
    // If there is a "root" window into which raster and QOpenGLWidget content is
    // composited, all other contexts must share with its context.
    QOpenGLContext *compositingContext = screen ? screen->compositingContext() : 0;
    QPlatformOpenGLContext *share = compositingContext ? compositingContext->handle() : context->shareHandle();
    QVariant nativeHandle = context->nativeHandle();
    QPlatformOpenGLContext *platformContext = createContext(context->format(),
                                                            share,
                                                            display(),
                                                            &nativeHandle);
    context->setNativeHandle(nativeHandle);
    return platformContext;
}

QPlatformOffscreenSurface *QEGLPlatformIntegration::createPlatformOffscreenSurface(QOffscreenSurface *surface) const
{
    QEGLPlatformScreen *screen = static_cast<QEGLPlatformScreen *>(surface->screen()->handle());
    return createOffscreenSurface(screen->display(), surface->requestedFormat(), surface);
}

bool QEGLPlatformIntegration::hasCapability(QPlatformIntegration::Capability cap) const
{
    switch (cap) {
    case ThreadedPixmaps: return true;
    case OpenGL: return true;
    case ThreadedOpenGL: return true;
    case WindowManagement: return false;
    case RasterGLSurface: return true;
    default: return QPlatformIntegration::hasCapability(cap);
    }
}

QPlatformNativeInterface *QEGLPlatformIntegration::nativeInterface() const
{
    return const_cast<QEGLPlatformIntegration *>(this);
}

enum ResourceType {
    EglDisplay,
    EglWindow,
    EglContext,
    EglConfig,
    NativeDisplay,
    XlibDisplay
};

static int resourceType(const QByteArray &key)
{
    static const QByteArray names[] = { // match ResourceType
        QByteArrayLiteral("egldisplay"),
        QByteArrayLiteral("eglwindow"),
        QByteArrayLiteral("eglcontext"),
        QByteArrayLiteral("eglconfig"),
        QByteArrayLiteral("nativedisplay"),
        QByteArrayLiteral("display")
    };
    const QByteArray *end = names + sizeof(names) / sizeof(names[0]);
    const QByteArray *result = std::find(names, end, key);
    if (result == end)
        result = std::find(names, end, key.toLower());
    return int(result - names);
}

void *QEGLPlatformIntegration::nativeResourceForIntegration(const QByteArray &resource)
{
    void *result = 0;

    switch (resourceType(resource)) {
    case EglDisplay:
        result = m_screen->display();
        break;
    case NativeDisplay:
        result = reinterpret_cast<void*>(nativeDisplay());
        break;
    default:
        break;
    }

    return result;
}

void *QEGLPlatformIntegration::nativeResourceForScreen(const QByteArray &resource, QScreen *)
{
    void *result = 0;

    switch (resourceType(resource)) {
    case XlibDisplay:
        // Play nice when using the x11 hooks: Be compatible with xcb that allows querying
        // the X Display pointer, which is nothing but our native display.
        result = reinterpret_cast<void*>(nativeDisplay());
        break;
    default:
        break;
    }

    return result;
}

void *QEGLPlatformIntegration::nativeResourceForWindow(const QByteArray &resource, QWindow *window)
{
    void *result = 0;

    switch (resourceType(resource)) {
    case EglDisplay:
        if (window && window->handle())
            result = static_cast<QEGLPlatformScreen *>(window->handle()->screen())->display();
        else
            result = m_screen->display();
        break;
    case EglWindow:
        if (window && window->handle())
            result = reinterpret_cast<void*>(static_cast<QEGLPlatformWindow *>(window->handle())->eglWindow());
        break;
    default:
        break;
    }

    return result;
}

void *QEGLPlatformIntegration::nativeResourceForContext(const QByteArray &resource, QOpenGLContext *context)
{
    void *result = 0;

    switch (resourceType(resource)) {
    case EglContext:
        if (context->handle())
            result = static_cast<QEGLPlatformContext *>(context->handle())->eglContext();
        break;
    case EglConfig:
        if (context->handle())
            result = static_cast<QEGLPlatformContext *>(context->handle())->eglConfig();
        break;
    case EglDisplay:
        if (context->handle())
            result = static_cast<QEGLPlatformContext *>(context->handle())->eglDisplay();
        break;
    default:
        break;
    }

    return result;
}

static void *eglContextForContext(QOpenGLContext *context)
{
    Q_ASSERT(context);

    QEGLPlatformContext *handle = static_cast<QEGLPlatformContext *>(context->handle());
    if (!handle)
        return 0;

    return handle->eglContext();
}

QPlatformNativeInterface::NativeResourceForContextFunction QEGLPlatformIntegration::nativeResourceFunctionForContext(const QByteArray &resource)
{
    QByteArray lowerCaseResource = resource.toLower();
    if (lowerCaseResource == "get_egl_context")
        return NativeResourceForContextFunction(eglContextForContext);

    return 0;
}

QFunctionPointer QEGLPlatformIntegration::platformFunction(const QByteArray &function) const
{
#if !defined(QT_NO_EVDEV) && (!defined(Q_OS_ANDROID) || defined(Q_OS_ANDROID_NO_SDK))
    if (function == QEglFSFunctions::loadKeymapTypeIdentifier())
        return QFunctionPointer(loadKeymapStatic);
#else
    Q_UNUSED(function)
#endif

    return 0;
}

void QEGLPlatformIntegration::loadKeymapStatic(const QString &filename)
{
#if !defined(QT_NO_EVDEV) && (!defined(Q_OS_ANDROID) || defined(Q_OS_ANDROID_NO_SDK))
    QEGLPlatformIntegration *self = static_cast<QEGLPlatformIntegration *>(QGuiApplicationPrivate::platformIntegration());
    if (self->m_kbdMgr)
        self->m_kbdMgr->loadKeymap(filename);
    else
        qWarning("QEGLPlatformIntegration: Cannot load keymap, no keyboard handler found");
#else
    Q_UNUSED(filename);
#endif
}

void QEGLPlatformIntegration::createInputHandlers()
{
#if !defined(QT_NO_EVDEV) && (!defined(Q_OS_ANDROID) || defined(Q_OS_ANDROID_NO_SDK))
    m_kbdMgr = new QEvdevKeyboardManager(QLatin1String("EvdevKeyboard"), QString() /* spec */, this);
    QEvdevMouseManager *mouseMgr = new QEvdevMouseManager(QLatin1String("EvdevMouse"), QString() /* spec */, this);
    Q_FOREACH (QScreen *screen, QGuiApplication::screens()) {
        QEGLPlatformCursor *cursor = static_cast<QEGLPlatformCursor *>(screen->handle()->cursor());
        if (cursor)
            cursor->setMouseDeviceDiscovery(mouseMgr->deviceDiscovery());
    }
    new QEvdevTouchScreenHandlerThread(QString() /* spec */, this);
#endif
}

QT_END_NAMESPACE
