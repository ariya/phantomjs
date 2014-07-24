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

#include "qopenwfdoutputbuffer.h"

#include "qopenwfdport.h"

QOpenWFDOutputBuffer::QOpenWFDOutputBuffer(const QSize &size, QOpenWFDPort *port)
    : mPort(port)
    , mAvailable(true)
{
    qDebug() << "creating output buffer for size" << size;
    glGenRenderbuffers(1,&mRbo);
    glBindRenderbuffer(GL_RENDERBUFFER, mRbo);

    mGbm_buffer = gbm_bo_create(port->device()->gbmDevice(),
                  size.width(),
                  size.height(),
                  GBM_BO_FORMAT_XRGB8888,
                  GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);

    mEglImage = port->device()->eglCreateImage(port->device()->eglDisplay(),0, EGL_NATIVE_PIXMAP_KHR, mGbm_buffer, 0);

    port->device()->glEglImageTargetRenderBufferStorage(GL_RENDERBUFFER,mEglImage);

    mWfdSource = wfdCreateSourceFromImage(port->device()->handle(),port->pipeline(),mEglImage,WFD_NONE);
    if (mWfdSource == WFD_INVALID_HANDLE) {
        qWarning("failed to create wfdSource from image");
    }
}

QOpenWFDOutputBuffer::~QOpenWFDOutputBuffer()
{
    wfdDestroySource(mPort->device()->handle(),mWfdSource);
    if (!mPort->device()->eglDestroyImage(mPort->device()->eglDisplay(),mEglImage)) {
        qDebug() << "could not delete eglImage";
    }
    gbm_bo_destroy(mGbm_buffer);

    glDeleteRenderbuffers(1, &mRbo);
}

void QOpenWFDOutputBuffer::bindToCurrentFbo()
{
    glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                              GL_COLOR_ATTACHMENT0,
                              GL_RENDERBUFFER,
                              mRbo);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        qDebug() << "framebuffer not ready!";
    }
}
