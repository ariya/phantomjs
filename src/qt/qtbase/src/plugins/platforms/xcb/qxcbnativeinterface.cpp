/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qxcbnativeinterface.h"

#include "qxcbscreen.h"
#include "qxcbwindow.h"
#include "qxcbintegration.h"
#include "qxcbsystemtraytracker.h"

#include <private/qguiapplication_p.h>
#include <QtCore/QMap>

#include <QtCore/QDebug>

#include <QtGui/qopenglcontext.h>
#include <QtGui/qscreen.h>

#if defined(XCB_USE_EGL)
#include "QtPlatformSupport/private/qeglplatformcontext_p.h"
#elif defined (XCB_USE_GLX)
#include "qglxintegration.h"
#endif

#ifdef XCB_USE_XLIB
#  include <X11/Xlib.h>
#else
#  include <stdio.h>
#endif

#include <algorithm>

QT_BEGIN_NAMESPACE

// return QXcbNativeInterface::ResourceType for the key.
static int resourceType(const QByteArray &key)
{
    static const QByteArray names[] = { // match QXcbNativeInterface::ResourceType
        QByteArrayLiteral("display"),  QByteArrayLiteral("egldisplay"),
        QByteArrayLiteral("connection"), QByteArrayLiteral("screen"),
        QByteArrayLiteral("eglcontext"),
        QByteArrayLiteral("glxcontext"), QByteArrayLiteral("apptime"),
        QByteArrayLiteral("appusertime"), QByteArrayLiteral("hintstyle"),
        QByteArrayLiteral("startupid"), QByteArrayLiteral("traywindow"),
        QByteArrayLiteral("gettimestamp"), QByteArrayLiteral("x11screen"),
        QByteArrayLiteral("rootwindow")
    };
    const QByteArray *end = names + sizeof(names) / sizeof(names[0]);
    const QByteArray *result = std::find(names, end, key);
    if (result == end)
        result = std::find(names, end, key.toLower());
    return int(result - names);
}

QXcbNativeInterface::QXcbNativeInterface() :
    m_genericEventFilterType(QByteArrayLiteral("xcb_generic_event_t"))

{
}

void QXcbNativeInterface::beep() // For QApplication::beep()
{
    QPlatformScreen *screen = QGuiApplication::primaryScreen()->handle();
    xcb_connection_t *connection = static_cast<QXcbScreen *>(screen)->xcb_connection();
    xcb_bell(connection, 0);
}

static inline QXcbSystemTrayTracker *systemTrayTracker(const QScreen *s)
{
    return static_cast<const QXcbScreen *>(s->handle())->connection()->systemTrayTracker();
}

bool QXcbNativeInterface::systemTrayAvailable(const QScreen *screen) const
{
    return systemTrayTracker(screen);
}

bool QXcbNativeInterface::requestSystemTrayWindowDock(const QWindow *window)
{
    const QPlatformWindow *platformWindow = window->handle();
    if (!platformWindow)
        return false;
    QXcbSystemTrayTracker *trayTracker = systemTrayTracker(window->screen());
    if (!trayTracker)
        return false;
    trayTracker->requestSystemTrayWindowDock(static_cast<const QXcbWindow *>(platformWindow)->xcb_window());
    return true;
}

QRect QXcbNativeInterface::systemTrayWindowGlobalGeometry(const QWindow *window)
{
    if (const QPlatformWindow *platformWindow = window->handle())
        if (const QXcbSystemTrayTracker *trayTracker = systemTrayTracker(window->screen()))
            return trayTracker->systemTrayWindowGlobalGeometry(static_cast<const QXcbWindow *>(platformWindow)->xcb_window());
    return QRect();
}

void *QXcbNativeInterface::nativeResourceForIntegration(const QByteArray &resourceString)
{
    void *result = 0;
    switch (resourceType(resourceString)) {
    case StartupId:
        result = startupId();
        break;
    case X11Screen:
        result = x11Screen();
        break;
    case RootWindow:
        result = rootWindow();
        break;
    default:
        break;
    }

    return result;
}

void *QXcbNativeInterface::nativeResourceForContext(const QByteArray &resourceString, QOpenGLContext *context)
{
    void *result = 0;
    switch (resourceType(resourceString)) {
    case EglContext:
        result = eglContextForContext(context);
        break;
    case GLXContext:
        result = glxContextForContext(context);
        break;
    default:
        break;
    }

    return result;
}

void *QXcbNativeInterface::nativeResourceForScreen(const QByteArray &resource, QScreen *screen)
{
    void *result = 0;
    const QXcbScreen *xcbScreen = static_cast<QXcbScreen *>(screen->handle());
    switch (resourceType(resource)) {
    case Display:
#ifdef XCB_USE_XLIB
        result = xcbScreen->connection()->xlib_display();
#endif
        break;
    case AppTime:
        result = appTime(xcbScreen);
        break;
    case AppUserTime:
        result = appUserTime(xcbScreen);
        break;
    case ScreenHintStyle:
        result = reinterpret_cast<void *>(xcbScreen->hintStyle() + 1);
        break;
    case TrayWindow:
        if (QXcbSystemTrayTracker *s = systemTrayTracker(screen))
            result = (void *)quintptr(s->trayWindow());
        break;
    case GetTimestamp:
        result = getTimestamp(xcbScreen);
        break;
    default:
        break;
    }
    return result;
}

void *QXcbNativeInterface::nativeResourceForWindow(const QByteArray &resourceString, QWindow *window)
{
    void *result = 0;
    switch (resourceType(resourceString)) {
    case Display:
        result = displayForWindow(window);
        break;
    case EglDisplay:
        result = eglDisplayForWindow(window);
        break;
    case Connection:
        result = connectionForWindow(window);
        break;
    case Screen:
        result = screenForWindow(window);
        break;
    default:
        break;
    }

    return result;
}

QPlatformNativeInterface::NativeResourceForScreenFunction QXcbNativeInterface::nativeResourceFunctionForScreen(const QByteArray &resource)
{
    const QByteArray lowerCaseResource = resource.toLower();
    if (lowerCaseResource == "setapptime")
        return NativeResourceForScreenFunction(setAppTime);
    else if (lowerCaseResource == "setappusertime")
        return NativeResourceForScreenFunction(setAppUserTime);
    return 0;
}

void *QXcbNativeInterface::appTime(const QXcbScreen *screen)
{
    return reinterpret_cast<void *>(quintptr(screen->connection()->time()));
}

void *QXcbNativeInterface::appUserTime(const QXcbScreen *screen)
{
    return reinterpret_cast<void *>(quintptr(screen->connection()->netWmUserTime()));
}

void *QXcbNativeInterface::getTimestamp(const QXcbScreen *screen)
{
    return reinterpret_cast<void *>(quintptr(screen->connection()->getTimestamp()));
}

void *QXcbNativeInterface::startupId()
{
    QXcbIntegration* integration = static_cast<QXcbIntegration *>(QGuiApplicationPrivate::platformIntegration());
    QXcbConnection *defaultConnection = integration->defaultConnection();
    if (defaultConnection)
        return reinterpret_cast<void *>(const_cast<char *>(defaultConnection->startupId().constData()));
    return 0;
}

void *QXcbNativeInterface::x11Screen()
{
    QXcbIntegration *integration = static_cast<QXcbIntegration *>(QGuiApplicationPrivate::platformIntegration());
    QXcbConnection *defaultConnection = integration->defaultConnection();
    if (defaultConnection)
        return reinterpret_cast<void *>(defaultConnection->primaryScreen());
    return 0;
}

void *QXcbNativeInterface::rootWindow()
{
    QXcbIntegration *integration = static_cast<QXcbIntegration *>(QGuiApplicationPrivate::platformIntegration());
    QXcbConnection *defaultConnection = integration->defaultConnection();
    if (defaultConnection)
        return reinterpret_cast<void *>(defaultConnection->rootWindow());
    return 0;
}

void QXcbNativeInterface::setAppTime(QScreen* screen, xcb_timestamp_t time)
{
    static_cast<QXcbScreen *>(screen->handle())->connection()->setTime(time);
}

void QXcbNativeInterface::setAppUserTime(QScreen* screen, xcb_timestamp_t time)
{
    static_cast<QXcbScreen *>(screen->handle())->connection()->setNetWmUserTime(time);
}

QPlatformNativeInterface::NativeResourceForContextFunction QXcbNativeInterface::nativeResourceFunctionForContext(const QByteArray &resource)
{
    QByteArray lowerCaseResource = resource.toLower();
    if (lowerCaseResource == "get_egl_context") {
        return eglContextForContext;
    }
    return 0;
}

QXcbScreen *QXcbNativeInterface::qPlatformScreenForWindow(QWindow *window)
{
    QXcbScreen *screen;
    if (window) {
        screen = static_cast<QXcbScreen *>(window->screen()->handle());
    } else {
        screen = static_cast<QXcbScreen *>(QGuiApplication::primaryScreen()->handle());
    }
    return screen;
}

void *QXcbNativeInterface::displayForWindow(QWindow *window)
{
#if defined(XCB_USE_XLIB)
    QXcbScreen *screen = qPlatformScreenForWindow(window);
    return screen->connection()->xlib_display();
#else
    Q_UNUSED(window);
    return 0;
#endif
}

void *QXcbNativeInterface::eglDisplayForWindow(QWindow *window)
{
#if defined(XCB_USE_EGL)
    QXcbScreen *screen = qPlatformScreenForWindow(window);
    return screen->connection()->egl_display();
#else
    Q_UNUSED(window)
    return 0;
#endif
}

void *QXcbNativeInterface::connectionForWindow(QWindow *window)
{
    QXcbScreen *screen = qPlatformScreenForWindow(window);
    return screen->xcb_connection();
}

void *QXcbNativeInterface::screenForWindow(QWindow *window)
{
    QXcbScreen *screen = qPlatformScreenForWindow(window);
    return screen->screen();
}

void * QXcbNativeInterface::eglContextForContext(QOpenGLContext *context)
{
    Q_ASSERT(context);
#if defined(XCB_USE_EGL)
    QEGLPlatformContext *eglPlatformContext = static_cast<QEGLPlatformContext *>(context->handle());
    return eglPlatformContext->eglContext();
#else
    Q_UNUSED(context);
    return 0;
#endif
}

void *QXcbNativeInterface::glxContextForContext(QOpenGLContext *context)
{
    Q_ASSERT(context);
#if defined(XCB_USE_GLX)
    QGLXContext *glxPlatformContext = static_cast<QGLXContext *>(context->handle());
    return glxPlatformContext->glxContext();
#else
    Q_UNUSED(context);
    return 0;
#endif

}

QT_END_NAMESPACE
