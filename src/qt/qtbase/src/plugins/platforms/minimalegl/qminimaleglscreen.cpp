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

#include "qminimaleglscreen.h"
#include "qminimaleglwindow.h"

#include <QtPlatformSupport/private/qeglconvenience_p.h>
#include <QtPlatformSupport/private/qeglplatformcontext_p.h>

#ifdef Q_OPENKODE
#include <KD/kd.h>
#include <KD/NV_initialize.h>
#endif //Q_OPENKODE

QT_BEGIN_NAMESPACE

// #define QEGL_EXTRA_DEBUG

class QMinimalEglContext : public QEGLPlatformContext
{
public:
    QMinimalEglContext(const QSurfaceFormat &format, QPlatformOpenGLContext *share, EGLDisplay display)
        : QEGLPlatformContext(format, share, display)
    {
    }

    EGLSurface eglSurfaceForPlatformSurface(QPlatformSurface *surface)
    {
        QMinimalEglWindow *window = static_cast<QMinimalEglWindow *>(surface);
        QMinimalEglScreen *screen = static_cast<QMinimalEglScreen *>(window->screen());
        return screen->surface();
    }
};

QMinimalEglScreen::QMinimalEglScreen(EGLNativeDisplayType display)
    : m_depth(32)
    , m_format(QImage::Format_Invalid)
    , m_platformContext(0)
    , m_surface(0)
{
#ifdef QEGL_EXTRA_DEBUG
    qWarning("QEglScreen %p\n", this);
#endif

    EGLint major, minor;

    if (!eglBindAPI(EGL_OPENGL_ES_API)) {
        qWarning("Could not bind GL_ES API\n");
        qFatal("EGL error");
    }

    m_dpy = eglGetDisplay(display);
    if (m_dpy == EGL_NO_DISPLAY) {
        qWarning("Could not open egl display\n");
        qFatal("EGL error");
    }
    qWarning("Opened display %p\n", m_dpy);

    if (!eglInitialize(m_dpy, &major, &minor)) {
        qWarning("Could not initialize egl display\n");
        qFatal("EGL error");
    }

    qWarning("Initialized display %d %d\n", major, minor);
}

QMinimalEglScreen::~QMinimalEglScreen()
{
    if (m_surface)
        eglDestroySurface(m_dpy, m_surface);

    eglTerminate(m_dpy);
}

void QMinimalEglScreen::createAndSetPlatformContext() const {
    const_cast<QMinimalEglScreen *>(this)->createAndSetPlatformContext();
}

void QMinimalEglScreen::createAndSetPlatformContext()
{
    QSurfaceFormat platformFormat;

    QByteArray depthString = qgetenv("QT_QPA_EGLFS_DEPTH");
    if (depthString.toInt() == 16) {
        platformFormat.setDepthBufferSize(16);
        platformFormat.setRedBufferSize(5);
        platformFormat.setGreenBufferSize(6);
        platformFormat.setBlueBufferSize(5);
        m_depth = 16;
        m_format = QImage::Format_RGB16;
    } else {
        platformFormat.setDepthBufferSize(24);
        platformFormat.setStencilBufferSize(8);
        platformFormat.setRedBufferSize(8);
        platformFormat.setGreenBufferSize(8);
        platformFormat.setBlueBufferSize(8);
        m_depth = 32;
        m_format = QImage::Format_RGB32;
    }

    if (!qEnvironmentVariableIsEmpty("QT_QPA_EGLFS_MULTISAMPLE"))
        platformFormat.setSamples(4);

    EGLConfig config = q_configFromGLFormat(m_dpy, platformFormat);

    EGLNativeWindowType eglWindow = 0;
#ifdef Q_OPENKODE
    if (kdInitializeNV() == KD_ENOTINITIALIZED) {
        qFatal("Did not manage to initialize openkode");
    }
    KDWindow *window = kdCreateWindow(m_dpy,config,0);

    kdRealizeWindow(window,&eglWindow);
#endif

#ifdef QEGL_EXTRA_DEBUG
    q_printEglConfig(m_dpy, config);
#endif

    m_surface = eglCreateWindowSurface(m_dpy, config, eglWindow, NULL);
    if (m_surface == EGL_NO_SURFACE) {
        qWarning("Could not create the egl surface: error = 0x%x\n", eglGetError());
        eglTerminate(m_dpy);
        qFatal("EGL error");
    }
    //    qWarning("Created surface %dx%d\n", w, h);

    QEGLPlatformContext *platformContext = new QMinimalEglContext(platformFormat, 0, m_dpy);
    m_platformContext = platformContext;

    EGLint w,h;                    // screen size detection
    eglQuerySurface(m_dpy, m_surface, EGL_WIDTH, &w);
    eglQuerySurface(m_dpy, m_surface, EGL_HEIGHT, &h);

    m_geometry = QRect(0,0,w,h);

}

QRect QMinimalEglScreen::geometry() const
{
    if (m_geometry.isNull()) {
        createAndSetPlatformContext();
    }
    return m_geometry;
}

int QMinimalEglScreen::depth() const
{
    return m_depth;
}

QImage::Format QMinimalEglScreen::format() const
{
    if (m_format == QImage::Format_Invalid)
        createAndSetPlatformContext();
    return m_format;
}
QPlatformOpenGLContext *QMinimalEglScreen::platformContext() const
{
    if (!m_platformContext) {
        QMinimalEglScreen *that = const_cast<QMinimalEglScreen *>(this);
        that->createAndSetPlatformContext();
    }
    return m_platformContext;
}

QT_END_NAMESPACE
