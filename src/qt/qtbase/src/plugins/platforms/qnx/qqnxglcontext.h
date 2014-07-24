/***************************************************************************
**
** Copyright (C) 2011 - 2012 Research In Motion
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

#ifndef QQNXGLCONTEXT_H
#define QQNXGLCONTEXT_H

#include <qpa/qplatformopenglcontext.h>
#include <QtGui/QSurfaceFormat>
#include <QtCore/QAtomicInt>
#include <QtCore/QSize>

#include <EGL/egl.h>

QT_BEGIN_NAMESPACE

class QQnxWindow;

class QQnxGLContext : public QPlatformOpenGLContext
{
public:
    QQnxGLContext(QOpenGLContext *glContext);
    virtual ~QQnxGLContext();

    static EGLenum checkEGLError(const char *msg);

    static void initialize();
    static void shutdown();

    void requestSurfaceChange();

    bool makeCurrent(QPlatformSurface *surface);
    void doneCurrent();
    void swapBuffers(QPlatformSurface *surface);
    QFunctionPointer getProcAddress(const QByteArray &procName);

    virtual QSurfaceFormat format() const { return m_windowFormat; }
    bool isSharing() const;

    static EGLDisplay getEglDisplay();
    EGLConfig getEglConfig() const { return m_eglConfig;}
    EGLContext getEglContext() const { return m_eglContext; }

private:
    //Can be static because different displays returne the same handle
    static EGLDisplay ms_eglDisplay;

    QSurfaceFormat m_windowFormat;
    QOpenGLContext *m_glContext;

    EGLConfig m_eglConfig;
    EGLContext m_eglContext;
    EGLContext m_eglShareContext;
    EGLSurface m_currentEglSurface;

    static EGLint *contextAttrs(const QSurfaceFormat &format);
};

QT_END_NAMESPACE

#endif // QQNXGLCONTEXT_H
