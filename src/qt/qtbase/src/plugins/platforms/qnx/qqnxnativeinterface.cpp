/***************************************************************************
**
** Copyright (C) 2012 Research In Motion
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

#include "qqnxnativeinterface.h"

#include "qqnxglcontext.h"
#include "qqnxscreen.h"
#include "qqnxwindow.h"
#if defined(QQNX_IMF)
#include "qqnxinputcontext_imf.h"
#endif

#include "qqnxintegration.h"

#include <QtGui/QOpenGLContext>
#include <QtGui/QScreen>
#include <QtGui/QWindow>

QT_BEGIN_NAMESPACE

QQnxNativeInterface::QQnxNativeInterface(QQnxIntegration *integration)
    : m_integration(integration)
{
}

void *QQnxNativeInterface::nativeResourceForWindow(const QByteArray &resource, QWindow *window)
{
    if (resource == "windowGroup" && window && window->screen()) {
        QQnxScreen * const screen = static_cast<QQnxScreen *>(window->screen()->handle());
        if (screen) {
            screen_window_t screenWindow = reinterpret_cast<screen_window_t>(window->winId());
            QQnxWindow *qnxWindow = screen->findWindow(screenWindow);
            // We can't just call data() instead of constData() here, since that would detach
            // and the lifetime of the char * would not be long enough. Therefore the const_cast.
            return qnxWindow ? const_cast<char *>(qnxWindow->groupName().constData()) : 0;
        }
    }

    return 0;
}

void *QQnxNativeInterface::nativeResourceForScreen(const QByteArray &resource, QScreen *screen)
{
    if (resource == "QObject*" && screen)
        return static_cast<QObject*>(static_cast<QQnxScreen*>(screen->handle()));

    return 0;
}

void *QQnxNativeInterface::nativeResourceForIntegration(const QByteArray &resource)
{
#ifdef Q_OS_BLACKBERRY
    if (resource == "navigatorEventHandler")
        return m_integration->navigatorEventHandler();
#endif

    return 0;
}

void *QQnxNativeInterface::nativeResourceForContext(const QByteArray &resource, QOpenGLContext *context)
{
    if (resource == "eglcontext" && context)
        return static_cast<QQnxGLContext*>(context->handle())->getEglContext();

    return 0;
}

void QQnxNativeInterface::setWindowProperty(QPlatformWindow *window, const QString &name, const QVariant &value)
{
    QQnxWindow *qnxWindow = static_cast<QQnxWindow*>(window);

    if (name == QStringLiteral("mmRendererWindowName")) {
        qnxWindow->setMMRendererWindowName(value.toString());
    } else if (name == QStringLiteral("qnxWindowGroup")) {
        if (value.isNull())
            qnxWindow->joinWindowGroup(QByteArray());
        else if (value.canConvert<QByteArray>())
            qnxWindow->joinWindowGroup(value.toByteArray());
    }
}

QPlatformNativeInterface::NativeResourceForIntegrationFunction QQnxNativeInterface::nativeResourceFunctionForIntegration(const QByteArray &resource)
{
#if defined(QQNX_IMF)
    if (resource == "blackberryIMFSetHighlightColor")
        return reinterpret_cast<NativeResourceForIntegrationFunction>(QQnxInputContext::setHighlightColor);
    if (resource == "blackberryIMFCheckSpelling")
        return reinterpret_cast<NativeResourceForIntegrationFunction>(QQnxInputContext::checkSpelling);
#else
    Q_UNUSED(resource)
#endif
    return 0;
}

QT_END_NAMESPACE
