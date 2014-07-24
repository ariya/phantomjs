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

#include "qwindowsnativeinterface.h"
#include "qwindowswindow.h"
#include "qwindowscontext.h"

#if defined(QT_OPENGL_ES_2) || defined(QT_OPENGL_DYNAMIC)
#  include "qwindowseglcontext.h"
#  include <QtGui/QOpenGLContext>
#endif

#if !defined(QT_NO_OPENGL) && !defined(QT_OPENGL_ES_2)
#  include "qwindowsglcontext.h"
#endif

#if !defined(QT_NO_OPENGL)
#  include <QtGui/QOpenGLFunctions>
#endif

#include <QtGui/QWindow>

QT_BEGIN_NAMESPACE

void *QWindowsNativeInterface::nativeResourceForWindow(const QByteArray &resource, QWindow *window)
{
    if (!window || !window->handle()) {
        qWarning("%s: '%s' requested for null window or window without handle.", __FUNCTION__, resource.constData());
        return 0;
    }
    QWindowsWindow *bw = static_cast<QWindowsWindow *>(window->handle());
    if (resource == "handle")
        return bw->handle();
    switch (window->surfaceType()) {
    case QWindow::RasterSurface:
    case QWindow::RasterGLSurface:
        if (resource == "getDC")
            return bw->getDC();
        if (resource == "releaseDC") {
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

#ifndef QT_NO_OPENGL
void *QWindowsNativeInterface::nativeResourceForContext(const QByteArray &resource, QOpenGLContext *context)
{
    if (!context || !context->handle()) {
        qWarning("%s: '%s' requested for null context or context without handle.", __FUNCTION__, resource.constData());
        return 0;
    }
#if defined(QT_OPENGL_ES_2) || defined(QT_OPENGL_DYNAMIC)
    if (QOpenGLContext::openGLModuleType() != QOpenGLContext::LibGL) {
        QWindowsEGLContext *windowsEglContext = static_cast<QWindowsEGLContext *>(context->handle());
        if (resource == QByteArrayLiteral("eglDisplay"))
            return windowsEglContext->eglDisplay();
        if (resource == QByteArrayLiteral("eglContext"))
            return windowsEglContext->eglContext();
        if (resource == QByteArrayLiteral("eglConfig"))
            return windowsEglContext->eglConfig();
    }
#endif // QT_OPENGL_ES_2 || QT_OPENGL_DYNAMIC
#if !defined(QT_OPENGL_ES_2)
    if (QOpenGLContext::openGLModuleType() == QOpenGLContext::LibGL) {
        QWindowsGLContext *windowsContext = static_cast<QWindowsGLContext *>(context->handle());
        if (resource == QByteArrayLiteral("renderingContext"))
            return windowsContext->renderingContext();
    }
#endif // QT_OPENGL_ES_2 || QT_OPENGL_DYNAMIC

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

QT_END_NAMESPACE
