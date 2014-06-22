/***************************************************************************
**
** Copyright (C) 2013 BlackBerry Limited. All rights reserved.
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

#ifndef QQNXEGLWINDOW_H
#define QQNXEGLWINDOW_H

#include "qqnxwindow.h"
#include <QtCore/QMutex>

QT_BEGIN_NAMESPACE

class QQnxGLContext;

class QQnxEglWindow : public QQnxWindow
{
public:
    QQnxEglWindow(QWindow *window, screen_context_t context, bool needRootWindow);
    ~QQnxEglWindow();

    void createEGLSurface();
    void destroyEGLSurface();
    void swapEGLBuffers();
    EGLSurface getSurface();

    void setPlatformOpenGLContext(QQnxGLContext *platformOpenGLContext);
    QQnxGLContext *platformOpenGLContext() const { return m_platformOpenGLContext; }

    void setGeometry(const QRect &rect);

protected:
    int pixelFormat() const;
    void resetBuffers();

private:
    QSize m_requestedBufferSize;

    // This mutex is used to protect access to the m_requestedBufferSize
    // member. This member is used in conjunction with QQnxGLContext::requestNewSurface()
    // to coordinate recreating the EGL surface which involves destroying any
    // existing EGL surface; resizing the native window buffers; and creating a new
    // EGL surface. All of this has to be done from the thread that is calling
    // QQnxGLContext::makeCurrent()
    mutable QMutex m_mutex;

    QQnxGLContext *m_platformOpenGLContext;
    QAtomicInt m_newSurfaceRequested;
    EGLSurface m_eglSurface;
};

QT_END_NAMESPACE

#endif // QQNXEGLWINDOW_H
