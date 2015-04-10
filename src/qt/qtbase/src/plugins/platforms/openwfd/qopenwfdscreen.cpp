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

#include "qopenwfdscreen.h"

#include "qopenwfdport.h"
#include "qopenwfdoutputbuffer.h"

#include <QtGui/QGuiApplication>

QOpenWFDScreen::QOpenWFDScreen(QOpenWFDPort *port)
    : mPort(port)
    , mFbo(0)
    , mOutputBuffers(BUFFER_NUM)
    , mCurrentRenderBufferIndex(0)
    , mStagedBackBufferIndex(-1)
    , mCommitedBackBufferIndex(-1)
    , mBackBufferIndex(-1)
{
    printf ("\n");
    printf ("Information of screen %p:\n", this);
    printf ("  width..........: %d\n", port->pixelSize().width());
    printf ("  height.........: %d\n", port->pixelSize().height());
    printf ("  physical width.: %f\n", port->physicalSize().width());
    printf ("  physical height: %f\n", port->physicalSize().height());
    printf ("\n");

    EGLDisplay display = mPort->device()->eglDisplay();
    EGLContext context = mPort->device()->eglContext();

    if (!eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,context)) {
        qDebug() << "screen: eglMakeCurrent FAILED";
    }

    glGenFramebuffers(1,&mFbo);
    glBindFramebuffer(GL_FRAMEBUFFER,mFbo);

    for (int i = 0; i < mOutputBuffers.size(); i++) {
        mOutputBuffers[i] = new QOpenWFDOutputBuffer(mPort->pixelSize(),mPort);
    }

    mStagedBackBufferIndex = mOutputBuffers.size()-1;
    mOutputBuffers[mStagedBackBufferIndex]->setAvailable(false);;
    commitStagedOutputBuffer();

    mOutputBuffers.at(mCurrentRenderBufferIndex)->bindToCurrentFbo();

    if (mPort->device()->isDeviceInitializedAndCommited()) {
        mPort->device()->commit(WFD_COMMIT_ENTIRE_PORT,mPort->handle());
    }

}

QOpenWFDScreen::~QOpenWFDScreen()
{
    for (int i = 0; i < mOutputBuffers.size(); i++) {
        delete mOutputBuffers[i];
    }
    glDeleteFramebuffers(1, &mFbo);
}

QRect QOpenWFDScreen::geometry() const
{
    return QRect(QPoint(),mPort->pixelSize());
}

int QOpenWFDScreen::depth() const
{
    return 32;
}

QImage::Format QOpenWFDScreen::format() const
{
    return QImage::Format_RGB32;
}

QSizeF QOpenWFDScreen::physicalSize() const
{
    return mPort->physicalSize();
}

QOpenWFDPort * QOpenWFDScreen::port() const
{
    return mPort;
}

void QOpenWFDScreen::swapBuffers()
{
    glFlush();

    setStagedBackBuffer(mCurrentRenderBufferIndex);
    mCurrentRenderBufferIndex = nextAvailableRenderBuffer();

    bindFramebuffer();
    mOutputBuffers.at(mCurrentRenderBufferIndex)->bindToCurrentFbo();
}

void QOpenWFDScreen::bindFramebuffer()
{
    glBindFramebuffer(GL_FRAMEBUFFER,mFbo);
}

void QOpenWFDScreen::setStagedBackBuffer(int bufferIndex)
{
    if (mStagedBackBufferIndex >= 0) {
        mOutputBuffers[mStagedBackBufferIndex]->setAvailable(true);
    }

    mOutputBuffers[bufferIndex]->setAvailable(false);;
    mStagedBackBufferIndex = bufferIndex;

    if (mCommitedBackBufferIndex < 0) {
        commitStagedOutputBuffer();
    }
}

void QOpenWFDScreen::commitStagedOutputBuffer()
{
    Q_ASSERT(mStagedBackBufferIndex >= 0);
    wfdBindSourceToPipeline(mPort->device()->handle(),
                            mPort->pipeline(),
                            mOutputBuffers.at(mStagedBackBufferIndex)->wfdSource(),
                            WFD_TRANSITION_AT_VSYNC,
                            0);
    mPort->device()->commit(WFD_COMMIT_PIPELINE,mPort->pipeline());
    mCommitedBackBufferIndex = mStagedBackBufferIndex;
    mStagedBackBufferIndex = -1;
}

int QOpenWFDScreen::nextAvailableRenderBuffer() const
{
    while (true) {
        for (int i = 0; i < mOutputBuffers.size(); i++) {
            if (mOutputBuffers.at(i)->isAvailable()) {
                return i;
            }
        }
        mPort->device()->waitForPipelineBindSourceCompleteEvent();
    }
}

void QOpenWFDScreen::pipelineBindSourceComplete()
{
    if (mBackBufferIndex >= 0) {
        mOutputBuffers[mBackBufferIndex]->setAvailable(true);
    }

    mBackBufferIndex = mCommitedBackBufferIndex;
    mCommitedBackBufferIndex = -1;

    if (mStagedBackBufferIndex >= 0) {
        commitStagedOutputBuffer();
    }
}
