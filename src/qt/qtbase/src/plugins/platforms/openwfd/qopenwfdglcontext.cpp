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

#include "qopenwfdglcontext.h"

#include "qopenwfdwindow.h"
#include "qopenwfdscreen.h"

QOpenWFDGLContext::QOpenWFDGLContext(QOpenWFDDevice *device)
    : QPlatformOpenGLContext()
    , mWfdDevice(device)
{
}

QSurfaceFormat QOpenWFDGLContext::format() const
{
    return QSurfaceFormat();
}

bool QOpenWFDGLContext::makeCurrent(QPlatformSurface *surface)
{
    Q_ASSERT(surface->surface()->surfaceType() == QSurface::OpenGLSurface);

    EGLDisplay display = mWfdDevice->eglDisplay();
    EGLContext context = mWfdDevice->eglContext();
    if (!eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,context)) {
        qDebug() << "GLContext: eglMakeCurrent FAILED!";
    }

    QPlatformWindow *window = static_cast<QPlatformWindow *>(surface);
    QOpenWFDScreen *screen = static_cast<QOpenWFDScreen *>(QPlatformScreen::platformScreenForWindow(window->window()));
    screen->bindFramebuffer();
    return true;
}

void QOpenWFDGLContext::doneCurrent()
{
    //do nothing :)
}

void QOpenWFDGLContext::swapBuffers(QPlatformSurface *surface)
{
    glFlush();

    QPlatformWindow *window = static_cast<QPlatformWindow *>(surface);
    QOpenWFDScreen *screen = static_cast<QOpenWFDScreen *>(QPlatformScreen::platformScreenForWindow(window->window()));

    screen->swapBuffers();
}

void (*QOpenWFDGLContext::getProcAddress(const QByteArray &procName)) ()
{
    return eglGetProcAddress(procName.data());
}

EGLContext QOpenWFDGLContext::eglContext() const
{
    return mWfdDevice->eglContext();
}


