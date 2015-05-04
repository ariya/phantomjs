/****************************************************************************
**
** Copyright (C) 2015 Digia Plc and/or its subsidiary(-ies).
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

#include "qwindowsnativeinterface.h"
#include "qwindowswindow.h"
#include "qwindowscontext.h"
#include "qwindowsopenglcontext.h"
#include "qwindowsopengltester.h"
#include "qwindowsintegration.h"
#include "qwindowsmime.h"

#include <QtGui/QWindow>
#include <QtGui/QOpenGLContext>

QT_BEGIN_NAMESPACE

enum ResourceType {
    RenderingContextType,
    EglContextType,
    EglDisplayType,
    EglConfigType,
    HandleType,
    GlHandleType,
    GetDCType,
    ReleaseDCType
};

static int resourceType(const QByteArray &key)
{
    static const char *names[] = { // match ResourceType
        "renderingcontext",
        "eglcontext",
        "egldisplay",
        "eglconfig",
        "handle",
        "glhandle",
        "getdc",
        "releasedc"
    };
    const char ** const end = names + sizeof(names) / sizeof(names[0]);
    const char **result = std::find(names, end, key);
    if (result == end)
        result = std::find(names, end, key.toLower());
    return int(result - names);
}

void *QWindowsNativeInterface::nativeResourceForWindow(const QByteArray &resource, QWindow *window)
{
    if (!window || !window->handle()) {
        qWarning("%s: '%s' requested for null window or window without handle.", __FUNCTION__, resource.constData());
        return 0;
    }
    QWindowsWindow *bw = static_cast<QWindowsWindow *>(window->handle());
    int type = resourceType(resource);
    if (type == HandleType)
        return bw->handle();
    switch (window->surfaceType()) {
    case QWindow::RasterSurface:
    case QWindow::RasterGLSurface:
        if (type == GetDCType)
            return bw->getDC();
        if (type == ReleaseDCType) {
            bw->releaseDC();
            return 0;
        }
        break;
    case QWindow::OpenGLSurface:
        break;
    }
    qWarning("%s: Invalid key '%s' requested.", __FUNCTION__, resource.constData());
    return 0;
}

static const char customMarginPropertyC[] = "WindowsCustomMargins";

QVariant QWindowsNativeInterface::windowProperty(QPlatformWindow *window, const QString &name) const
{
    QWindowsWindow *platformWindow = static_cast<QWindowsWindow *>(window);
    if (name == QLatin1String(customMarginPropertyC))
        return qVariantFromValue(platformWindow->customMargins());
    return QVariant();
}

QVariant QWindowsNativeInterface::windowProperty(QPlatformWindow *window, const QString &name, const QVariant &defaultValue) const
{
    const QVariant result = windowProperty(window, name);
    return result.isValid() ? result : defaultValue;
}

void QWindowsNativeInterface::setWindowProperty(QPlatformWindow *window, const QString &name, const QVariant &value)
{
    QWindowsWindow *platformWindow = static_cast<QWindowsWindow *>(window);
    if (name == QLatin1String(customMarginPropertyC))
        platformWindow->setCustomMargins(qvariant_cast<QMargins>(value));
}

QVariantMap QWindowsNativeInterface::windowProperties(QPlatformWindow *window) const
{
    QVariantMap result;
    const QString customMarginProperty = QLatin1String(customMarginPropertyC);
    result.insert(customMarginProperty, windowProperty(window, customMarginProperty));
    return result;
}

void *QWindowsNativeInterface::nativeResourceForIntegration(const QByteArray &resource)
{
#ifndef QT_NO_OPENGL
    if (resourceType(resource) == GlHandleType)
        return QWindowsIntegration::staticOpenGLContext()->moduleHandle();
#endif

    return 0;
}

#ifndef QT_NO_OPENGL
void *QWindowsNativeInterface::nativeResourceForContext(const QByteArray &resource, QOpenGLContext *context)
{
    if (!context || !context->handle()) {
        qWarning("%s: '%s' requested for null context or context without handle.", __FUNCTION__, resource.constData());
        return 0;
    }

    QWindowsOpenGLContext *glcontext = static_cast<QWindowsOpenGLContext *>(context->handle());
    switch (resourceType(resource)) {
    case RenderingContextType: // Fall through.
    case EglContextType:
        return glcontext->nativeContext();
    case EglDisplayType:
        return glcontext->nativeDisplay();
    case EglConfigType:
        return glcontext->nativeConfig();
    default:
        break;
    }

    qWarning("%s: Invalid key '%s' requested.", __FUNCTION__, resource.constData());
    return 0;
}
#endif // !QT_NO_OPENGL

/*!
    \brief Creates a non-visible window handle for filtering messages.
*/

void *QWindowsNativeInterface::createMessageWindow(const QString &classNameTemplate,
                                                   const QString &windowName,
                                                   void *eventProc) const
{
    QWindowsContext *ctx = QWindowsContext::instance();
    const HWND hwnd = ctx->createDummyWindow(classNameTemplate,
                                             (wchar_t*)windowName.utf16(),
                                             (WNDPROC)eventProc);
    return hwnd;
}

/*!
    \brief Registers a unique window class with a callback function based on \a classNameIn.
*/

QString QWindowsNativeInterface::registerWindowClass(const QString &classNameIn, void *eventProc) const
{
    return QWindowsContext::instance()->registerWindowClass(classNameIn, (WNDPROC)eventProc);
}

bool QWindowsNativeInterface::asyncExpose() const
{
    return QWindowsContext::instance()->asyncExpose();
}

void QWindowsNativeInterface::setAsyncExpose(bool value)
{
    QWindowsContext::instance()->setAsyncExpose(value);
}

void QWindowsNativeInterface::registerWindowsMime(void *mimeIn)
{
    QWindowsContext::instance()->mimeConverter().registerMime(reinterpret_cast<QWindowsMime *>(mimeIn));
}

void QWindowsNativeInterface::unregisterWindowsMime(void *mimeIn)
{
    QWindowsContext::instance()->mimeConverter().unregisterMime(reinterpret_cast<QWindowsMime *>(mimeIn));
}

int QWindowsNativeInterface::registerMimeType(const QString &mimeType)
{
    return QWindowsMime::registerMimeType(mimeType);
}

QVariant QWindowsNativeInterface::gpu() const
{
    return GpuDescription::detect().toVariant();
}

QT_END_NAMESPACE
