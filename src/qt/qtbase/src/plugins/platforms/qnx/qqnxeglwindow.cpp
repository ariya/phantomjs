/***************************************************************************
**
** Copyright (C) 2013 - 2014 BlackBerry Limited. All rights reserved.
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


#include "qqnxeglwindow.h"
#include "qqnxscreen.h"
#include "qqnxglcontext.h"

#include <QDebug>

#include <errno.h>

#if defined(QQNXEGLWINDOW_DEBUG)
#define qEglWindowDebug qDebug
#else
#define qEglWindowDebug QT_NO_QDEBUG_MACRO
#endif

QT_BEGIN_NAMESPACE

QQnxEglWindow::QQnxEglWindow(QWindow *window, screen_context_t context, bool needRootWindow) :
    QQnxWindow(window, context, needRootWindow),
    m_platformOpenGLContext(0),
    m_newSurfaceRequested(true),
    m_eglSurface(EGL_NO_SURFACE)
{
    initWindow();

    // Set window usage
    const int val = SCREEN_USAGE_OPENGL_ES2;
    const int result = screen_set_window_property_iv(nativeHandle(), SCREEN_PROPERTY_USAGE, &val);
    if (result != 0)
        qFatal("QQnxEglWindow: failed to set window alpha usage, errno=%d", errno);

    m_requestedBufferSize = shouldMakeFullScreen() ? screen()->geometry().size() : window->geometry().size();
}

QQnxEglWindow::~QQnxEglWindow()
{
    // Cleanup EGL surface if it exists
    destroyEGLSurface();
}

void QQnxEglWindow::createEGLSurface()
{
    if (!m_requestedBufferSize.isValid()) {
        qWarning("QQNX: Trying to create 0 size EGL surface. "
               "Please set a valid window size before calling QOpenGLContext::makeCurrent()");
        return;
    }

    // update the window's buffers before we create the EGL surface
    setBufferSize(m_requestedBufferSize);

    const EGLint eglSurfaceAttrs[] =
    {
        EGL_RENDER_BUFFER, EGL_BACK_BUFFER,
        EGL_NONE
    };

    qEglWindowDebug() << "Creating EGL surface" << platformOpenGLContext()->getEglDisplay()
                   << platformOpenGLContext()->getEglConfig();

    // Create EGL surface
    m_eglSurface = eglCreateWindowSurface(platformOpenGLContext()->getEglDisplay(),
                                          platformOpenGLContext()->getEglConfig(),
                                          (EGLNativeWindowType) nativeHandle(), eglSurfaceAttrs);
    if (m_eglSurface == EGL_NO_SURFACE) {
        const EGLenum error = QQnxGLContext::checkEGLError("eglCreateWindowSurface");
        qWarning("QQNX: failed to create EGL surface, err=%d", error);
    }
}

void QQnxEglWindow::destroyEGLSurface()
{
    // Destroy EGL surface if it exists
    if (m_eglSurface != EGL_NO_SURFACE) {
        EGLBoolean eglResult = eglDestroySurface(platformOpenGLContext()->getEglDisplay(), m_eglSurface);
        if (eglResult != EGL_TRUE)
            qFatal("QQNX: failed to destroy EGL surface, err=%d", eglGetError());
    }

    m_eglSurface = EGL_NO_SURFACE;
}

void QQnxEglWindow::swapEGLBuffers()
{
    qEglWindowDebug() << Q_FUNC_INFO;
    // Set current rendering API
    EGLBoolean eglResult = eglBindAPI(EGL_OPENGL_ES_API);
    if (eglResult != EGL_TRUE)
        qFatal("QQNX: failed to set EGL API, err=%d", eglGetError());

    // Post EGL surface to window
    eglResult = eglSwapBuffers(m_platformOpenGLContext->getEglDisplay(), m_eglSurface);
    if (eglResult != EGL_TRUE)
        qFatal("QQNX: failed to swap EGL buffers, err=%d", eglGetError());

    windowPosted();
}

EGLSurface QQnxEglWindow::getSurface()
{
    if (m_newSurfaceRequested.testAndSetOrdered(true, false)) {
        const QMutexLocker locker(&m_mutex); //Set geomety must not reset the requestedBufferSize till
                                             //the surface is created
        if (m_eglSurface != EGL_NO_SURFACE) {
            platformOpenGLContext()->doneCurrent();
            destroyEGLSurface();
        }
        createEGLSurface();
    }

    return m_eglSurface;
}

void QQnxEglWindow::setGeometry(const QRect &rect)
{
    //If this is the root window, it has to be shown fullscreen
    const QRect &newGeometry = shouldMakeFullScreen() ? screen()->geometry() : rect;

    //We need to request that the GL context updates
    // the EGLsurface on which it is rendering.
    {
        // We want the setting of the atomic bool in the GL context to be atomic with
        // setting m_requestedBufferSize and therefore extended the scope to include
        // that test.
        const QMutexLocker locker(&m_mutex);
        m_requestedBufferSize = newGeometry.size();
        if (m_platformOpenGLContext != 0 && bufferSize() != newGeometry.size())
            m_newSurfaceRequested.testAndSetRelease(false, true);
    }
    QQnxWindow::setGeometry(newGeometry);
}

void QQnxEglWindow::setPlatformOpenGLContext(QQnxGLContext *platformOpenGLContext)
{
    // This function does not take ownership of the platform gl context.
    // It is owned by the frontend QOpenGLContext
    m_platformOpenGLContext = platformOpenGLContext;
}

int QQnxEglWindow::pixelFormat() const
{
    if (!m_platformOpenGLContext) //The platform GL context was not set yet
        return -1;

    const QSurfaceFormat format = m_platformOpenGLContext->format();
    // Extract size of color channels from window format
    const int redSize = format.redBufferSize();
    if (redSize == -1)
        qFatal("QQnxWindow: red size not defined");

    const int greenSize = format.greenBufferSize();
    if (greenSize == -1)
        qFatal("QQnxWindow: green size not defined");

    const int blueSize = format.blueBufferSize();
    if (blueSize == -1)
        qFatal("QQnxWindow: blue size not defined");

    // select matching native format
    if (redSize == 5 && greenSize == 6 && blueSize == 5)
        return SCREEN_FORMAT_RGB565;
    else if (redSize == 8 && greenSize == 8 && blueSize == 8)
        return SCREEN_FORMAT_RGBA8888;

    qFatal("QQnxWindow: unsupported pixel format");
}

void QQnxEglWindow::resetBuffers()
{
    m_requestedBufferSize = QSize();
}

QT_END_NAMESPACE
