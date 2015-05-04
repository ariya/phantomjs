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

#include <private/qguiapplication_p.h>
#include "qkmsnativeinterface.h"
#include "qkmsdevice.h"

#include "qscreen.h"
#include "qkmscontext.h"
#include <QOpenGLContext>

class QKmsResourceMap : public QMap<QByteArray, QKmsNativeInterface::ResourceType>
{
public:
    QKmsResourceMap()
        :QMap<QByteArray, QKmsNativeInterface::ResourceType>()
    {
        insert("egldisplay", QKmsNativeInterface::EglDisplay);
        insert("eglcontext", QKmsNativeInterface::EglContext);
    }
};

Q_GLOBAL_STATIC(QKmsResourceMap, qKmsResourceMap)

void *QKmsNativeInterface::nativeResourceForWindow(const QByteArray &resourceString, QWindow *window)
{
    QByteArray lowerCaseResource = resourceString.toLower();
    ResourceType resource = qKmsResourceMap()->value(lowerCaseResource);
    void *result = 0;
    switch (resource) {
    case EglDisplay:
        result = eglDisplayForWindow(window);
        break;
    case EglContext:
        result = eglContextForWindow(window);
        break;
    default:
        result = 0;
    }
    return result;
}

QPlatformNativeInterface::NativeResourceForContextFunction QKmsNativeInterface::nativeResourceFunctionForContext(const QByteArray &resource)
{
    QByteArray lowerCaseResource = resource.toLower();
    if (lowerCaseResource == "get_egl_context") {
        return eglContextForContext;
    }
    return 0;
}



void *QKmsNativeInterface::eglDisplayForWindow(QWindow *window)
{
    QKmsScreen *screen = qPlatformScreenForWindow(window);
    if (!screen)
        return 0;
    QKmsDevice *device = screen->device();
    if (!device)
        return 0;
    return device->eglDisplay();
}

void *QKmsNativeInterface::eglContextForWindow(QWindow *)
{
    return 0;
}

QKmsScreen *QKmsNativeInterface::qPlatformScreenForWindow(QWindow *window)
{
    QScreen *screen = window ? window->screen() : QGuiApplication::primaryScreen();
    return static_cast<QKmsScreen *>(screen->handle());
}

void *QKmsNativeInterface::eglContextForContext(QOpenGLContext *context)
{
    Q_ASSERT(context);

    QKmsContext *eglPlatformContext = static_cast<QKmsContext *>(context->handle());

    return eglPlatformContext->eglContext();
}
