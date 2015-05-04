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

#include "qkmsscreen.h"
#include "qkmsdevice.h"
#include "qkmscontext.h"
#include "qkmswindow.h"
#include "qkmsintegration.h"

#include <QtGui/QOpenGLContext>
#include <QtPlatformSupport/private/qeglconvenience_p.h>

QT_BEGIN_NAMESPACE

QKmsContext::QKmsContext(QOpenGLContext *context, QKmsDevice *device)
    : m_device(device)
{
    EGLDisplay display = m_device->eglDisplay();
    EGLConfig config = q_configFromGLFormat(display, QKmsScreen::tweakFormat(context->format()));
    m_format = q_glFormatFromConfig(display, config);

    //Initialize EGLContext
    static EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, context->format().majorVersion(),
        EGL_NONE
    };

    eglBindAPI(EGL_OPENGL_ES_API);

    EGLContext share = EGL_NO_CONTEXT;
    if (context->shareContext())
        share = static_cast<QKmsContext *>(context->shareContext()->handle())->eglContext();

    m_eglContext = eglCreateContext(display, config, share, contextAttribs);
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
    Q_ASSERT(surface->surface()->supportsOpenGL());

    EGLDisplay display = m_device->eglDisplay();
    EGLSurface eglSurface;

    if (surface->surface()->surfaceClass() == QSurface::Window) {
        QPlatformWindow *window = static_cast<QPlatformWindow *>(surface);
        QKmsScreen *screen = static_cast<QKmsScreen *>(QPlatformScreen::platformScreenForWindow(window->window()));
        eglSurface = screen->eglSurface();
        screen->waitForPageFlipComplete();
    } else {
        eglSurface = static_cast<QKmsOffscreenWindow *>(surface)->surface();
    }

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
