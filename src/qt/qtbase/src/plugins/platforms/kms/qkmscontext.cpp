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

#include "qkmsscreen.h"
#include "qkmsdevice.h"
#include "qkmscontext.h"
#include "qkmswindow.h"

#include <QOpenGLContext>

#include <QtPlatformSupport/private/qeglconvenience_p.h>

QT_BEGIN_NAMESPACE

QKmsContext::QKmsContext(QOpenGLContext *context, QKmsDevice *device)
    : QPlatformOpenGLContext()
    , m_device(device)
{
    EGLDisplay display = m_device->eglDisplay();
    EGLConfig config = q_configFromGLFormat(display, QKmsScreen::tweakFormat(context->format()), true);
    m_format = q_glFormatFromConfig(display, config);

    //Initialize EGLContext
    static EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, context->format().majorVersion(),
        EGL_NONE
    };

    eglBindAPI(EGL_OPENGL_ES_API);
    m_eglContext = eglCreateContext(display, config, 0, contextAttribs);
    if (m_eglContext == EGL_NO_CONTEXT) {
        qWarning("QKmsContext::QKmsContext(): eglError: %x, this: %p",
                 eglGetError(), this);
        m_eglContext = 0;
    }
}

bool QKmsContext::isValid() const
{
    return m_eglContext != EGL_NO_CONTEXT;
}

bool QKmsContext::makeCurrent(QPlatformSurface *surface)
{
    Q_ASSERT(surface->surface()->surfaceType() == QSurface::OpenGLSurface);

    EGLDisplay display = m_device->eglDisplay();

    QPlatformWindow *window = static_cast<QPlatformWindow *>(surface);
    QKmsScreen *screen = static_cast<QKmsScreen *> (QPlatformScreen::platformScreenForWindow(window->window()));

    EGLSurface eglSurface = screen->eglSurface();

    screen->waitForPageFlipComplete();

    bool ok = eglMakeCurrent(display, eglSurface, eglSurface, m_eglContext);
    if (!ok)
        qWarning("QKmsContext::makeCurrent(): eglError: %x, this: %p",
                 eglGetError(), this);

    return true;
}

void QKmsContext::doneCurrent()
{
    bool ok = eglMakeCurrent(m_device->eglDisplay(), EGL_NO_SURFACE, EGL_NO_SURFACE,
                             EGL_NO_CONTEXT);
    if (!ok)
        qWarning("QKmsContext::doneCurrent(): eglError: %x, this: %p",
                 eglGetError(), this);

}

void QKmsContext::swapBuffers(QPlatformSurface *surface)
{
    //Cast context to a window surface and get the screen the context
    //is on and call swapBuffers on that screen.
    QPlatformWindow *window = static_cast<QPlatformWindow *>(surface);
    QKmsScreen *screen = static_cast<QKmsScreen *> (QPlatformScreen::platformScreenForWindow(window->window()));
    screen->swapBuffers();
}

void (*QKmsContext::getProcAddress(const QByteArray &procName)) ()
{
    return eglGetProcAddress(procName.data());
}


EGLContext QKmsContext::eglContext() const
{
    return m_eglContext;
}

QSurfaceFormat QKmsContext::format() const
{
    return m_format;
}

QT_END_NAMESPACE
