/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the qmake spec of the Qt Toolkit.
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

#include "qeglfshooks.h"
#include <EGL/eglvivante.h>
#include <QDebug>

QT_BEGIN_NAMESPACE

class QEglFSImx6Hooks : public QEglFSHooks
{
public:
    QEglFSImx6Hooks();
    virtual QSize screenSize() const;
    virtual EGLNativeWindowType createNativeWindow(QPlatformWindow *window, const QSize &size, const QSurfaceFormat &format);
    virtual void destroyNativeWindow(EGLNativeWindowType window);
    virtual EGLNativeDisplayType platformDisplay() const;

private:
    QSize mScreenSize;
    EGLNativeDisplayType mNativeDisplay;
};


QEglFSImx6Hooks::QEglFSImx6Hooks()
{
    int width, height;

    bool multiBufferNotEnabledYet = qEnvironmentVariableIsEmpty("FB_MULTI_BUFFER");
    bool multiBuffer = qEnvironmentVariableIsEmpty("QT_EGLFS_IMX6_NO_FB_MULTI_BUFFER");
    if (multiBufferNotEnabledYet && multiBuffer) {
        qWarning() << "QEglFSImx6Hooks will set environment variable FB_MULTI_BUFFER=2 to enable double buffering and vsync.\n"
                   << "If this is not desired, you can override this via: export QT_EGLFS_IMX6_NO_FB_MULTI_BUFFER=1";
        qputenv("FB_MULTI_BUFFER", "2");
    }

    mNativeDisplay = fbGetDisplayByIndex(framebufferIndex());
    fbGetDisplayGeometry(mNativeDisplay, &width, &height);
    mScreenSize.setHeight(height);
    mScreenSize.setWidth(width);
}

QSize QEglFSImx6Hooks::screenSize() const
{
    return mScreenSize;
}

EGLNativeDisplayType QEglFSImx6Hooks::platformDisplay() const
{
    return mNativeDisplay;
}

EGLNativeWindowType QEglFSImx6Hooks::createNativeWindow(QPlatformWindow *window, const QSize &size, const QSurfaceFormat &format)
{
    Q_UNUSED(window)
    Q_UNUSED(format)

    EGLNativeWindowType eglWindow = fbCreateWindow(mNativeDisplay, 0, 0, size.width(), size.height());
    return eglWindow;
}


void QEglFSImx6Hooks::destroyNativeWindow(EGLNativeWindowType window)
{
    fbDestroyWindow(window);
}

QEglFSImx6Hooks eglFSImx6Hooks;
QEglFSHooks *platformHooks = &eglFSImx6Hooks;

QT_END_NAMESPACE
