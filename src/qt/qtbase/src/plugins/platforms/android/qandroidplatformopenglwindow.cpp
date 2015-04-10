/****************************************************************************
**
** Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
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

#include "qandroidplatformopenglwindow.h"

#include "androidjnimain.h"

#include <QSurfaceFormat>

#include <qpa/qwindowsysteminterface.h>

#include <android/native_window.h>
#include <android/native_window_jni.h>

QT_BEGIN_NAMESPACE

QAndroidPlatformOpenGLWindow::QAndroidPlatformOpenGLWindow(QWindow *window, EGLDisplay display)
    :QAndroidPlatformWindow(window), m_eglDisplay(display)
{
    lockSurface();
    m_nativeSurfaceId = QtAndroid::createSurface(this, geometry(), bool(window->flags() & Qt::WindowStaysOnTopHint), 32);
    m_surfaceWaitCondition.wait(&m_surfaceMutex);
    unlockSurface();
}

QAndroidPlatformOpenGLWindow::~QAndroidPlatformOpenGLWindow()
{
    m_surfaceWaitCondition.wakeOne();
    lockSurface();
    if (m_nativeSurfaceId != -1)
        QtAndroid::destroySurface(m_nativeSurfaceId);
    clearEgl();
    unlockSurface();
}

void QAndroidPlatformOpenGLWindow::setGeometry(const QRect &rect)
{
    if (rect == geometry())
        return;

    QAndroidPlatformWindow::setGeometry(rect);
    QtAndroid::setSurfaceGeometry(m_nativeSurfaceId, rect);
}

EGLSurface QAndroidPlatformOpenGLWindow::eglSurface(EGLConfig config)
{
    QMutexLocker lock(&m_surfaceMutex);
    if (m_eglSurface == EGL_NO_SURFACE) {
        m_surfaceMutex.unlock();
        checkNativeSurface(config);
        m_surfaceMutex.lock();
    }
    return m_eglSurface;
}

void QAndroidPlatformOpenGLWindow::checkNativeSurface(EGLConfig config)
{
    QMutexLocker lock(&m_surfaceMutex);
    if (m_nativeSurfaceId == -1 || !m_androidSurfaceObject.isValid())
        return;

    createEgl(config);

    // we've create another surface, the window should be repainted
    QWindowSystemInterface::handleExposeEvent(window(), QRegion(geometry()));
}

void QAndroidPlatformOpenGLWindow::createEgl(EGLConfig config)
{
    clearEgl();
    QJNIEnvironmentPrivate env;
    m_nativeWindow = ANativeWindow_fromSurface(env, m_androidSurfaceObject.object());
    m_androidSurfaceObject = QJNIObjectPrivate();
    m_eglSurface = eglCreateWindowSurface(m_eglDisplay, config, m_nativeWindow, NULL);
    if (m_eglSurface == EGL_NO_SURFACE) {
        EGLint error = eglGetError();
        eglTerminate(m_eglDisplay);
        qFatal("EGL Error : Could not create the egl surface: error = 0x%x\n", error);
    }
}

void QAndroidPlatformOpenGLWindow::clearEgl()
{
    eglMakeCurrent(m_eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
    if (m_eglSurface != EGL_NO_SURFACE) {
        eglDestroySurface(m_eglDisplay, m_eglSurface);
        m_eglSurface = EGL_NO_SURFACE;
    }

    if (m_nativeWindow) {
        ANativeWindow_release(m_nativeWindow);
        m_nativeWindow = 0;
    }
}

void QAndroidPlatformOpenGLWindow::surfaceChanged(JNIEnv *jniEnv, jobject surface, int w, int h)
{
    Q_UNUSED(jniEnv);
    Q_UNUSED(w);
    Q_UNUSED(h);
    lockSurface();
    m_androidSurfaceObject = surface;
    m_surfaceWaitCondition.wakeOne();
    unlockSurface();

    // repaint the window
    QWindowSystemInterface::handleExposeEvent(window(), QRegion(geometry()));
}

QT_END_NAMESPACE
