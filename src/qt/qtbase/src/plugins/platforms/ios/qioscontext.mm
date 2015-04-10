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

#include "qioscontext.h"
#include "qioswindow.h"

#include <dlfcn.h>

#include <QtGui/QOpenGLContext>

#import <OpenGLES/EAGL.h>
#import <QuartzCore/CAEAGLLayer.h>

QIOSContext::QIOSContext(QOpenGLContext *context)
    : QPlatformOpenGLContext()
    , m_sharedContext(static_cast<QIOSContext *>(context->shareHandle()))
    , m_eaglContext([[EAGLContext alloc]
        initWithAPI:kEAGLRenderingAPIOpenGLES2
        sharegroup:m_sharedContext ? [m_sharedContext->m_eaglContext sharegroup] : nil])
    , m_format(context->format())
{
    m_format.setRenderableType(QSurfaceFormat::OpenGLES);
    m_format.setMajorVersion(2);
    m_format.setMinorVersion(0);

    // iOS internally double-buffers its rendering using copy instead of flipping,
    // so technically we could report that we are single-buffered so that clients
    // could take advantage of the unchanged buffer, but this means clients (and Qt)
    // will also assume that swapBufferes() is not needed, which is _not_ the case.
    m_format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
}

QIOSContext::~QIOSContext()
{
    [EAGLContext setCurrentContext:m_eaglContext];

    foreach (const FramebufferObject &framebufferObject, m_framebufferObjects)
        deleteBuffers(framebufferObject);

    [EAGLContext setCurrentContext:nil];
    [m_eaglContext release];
}

void QIOSContext::deleteBuffers(const FramebufferObject &framebufferObject)
{
    if (framebufferObject.handle)
        glDeleteFramebuffers(1, &framebufferObject.handle);
    if (framebufferObject.colorRenderbuffer)
        glDeleteRenderbuffers(1, &framebufferObject.colorRenderbuffer);
    if (framebufferObject.depthRenderbuffer)
        glDeleteRenderbuffers(1, &framebufferObject.depthRenderbuffer);
}

QSurfaceFormat QIOSContext::format() const
{
    return m_format;
}

#define QT_IOS_GL_STATUS_CASE(val) case val: return QLatin1Literal(#val)

static QString fboStatusString(GLenum status)
{
    switch (status) {
        QT_IOS_GL_STATUS_CASE(GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT);
        QT_IOS_GL_STATUS_CASE(GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS);
        QT_IOS_GL_STATUS_CASE(GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT);
        QT_IOS_GL_STATUS_CASE(GL_FRAMEBUFFER_UNSUPPORTED);
    default:
        return QString(QStringLiteral("unknown status: %x")).arg(status);
    }
}

bool QIOSContext::makeCurrent(QPlatformSurface *surface)
{
    Q_ASSERT(surface && surface->surface()->surfaceType() == QSurface::OpenGLSurface);

    [EAGLContext setCurrentContext:m_eaglContext];

    // For offscreen surfaces we don't prepare a default FBO
    if (surface->surface()->surfaceClass() == QSurface::Offscreen)
        return true;

    FramebufferObject &framebufferObject = backingFramebufferObjectFor(surface);

    // We bind the default FBO even if it's incomplete, so that clients who
    // call glCheckFramebufferStatus as a result of this function returning
    // false will get a matching error code.
    glBindFramebuffer(GL_FRAMEBUFFER, framebufferObject.handle);

    return framebufferObject.isComplete;
}

void QIOSContext::doneCurrent()
{
    [EAGLContext setCurrentContext:nil];
}

void QIOSContext::swapBuffers(QPlatformSurface *surface)
{
    Q_ASSERT(surface && surface->surface()->surfaceType() == QSurface::OpenGLSurface);

    if (surface->surface()->surfaceClass() == QSurface::Offscreen)
        return; // Nothing to do

    FramebufferObject &framebufferObject = backingFramebufferObjectFor(surface);

    [EAGLContext setCurrentContext:m_eaglContext];
    glBindRenderbuffer(GL_RENDERBUFFER, framebufferObject.colorRenderbuffer);
    [m_eaglContext presentRenderbuffer:GL_RENDERBUFFER];
}

QIOSContext::FramebufferObject &QIOSContext::backingFramebufferObjectFor(QPlatformSurface *surface) const
{
    // We keep track of default-FBOs in the root context of a share-group. This assumes
    // that the contexts form a tree, where leaf nodes are always destroyed before their
    // parents. If that assumption (based on the current implementation) doesn't hold we
    // should probably use QOpenGLMultiGroupSharedResource to track the shared default-FBOs.
    if (m_sharedContext)
        return m_sharedContext->backingFramebufferObjectFor(surface);

    Q_ASSERT(surface && surface->surface()->surfaceClass() == QSurface::Window);
    QIOSWindow *window = static_cast<QIOSWindow *>(surface);

    FramebufferObject &framebufferObject = m_framebufferObjects[window];

    // Set up an FBO for the window if it hasn't been created yet
    if (!framebufferObject.handle) {
        [EAGLContext setCurrentContext:m_eaglContext];

        glGenFramebuffers(1, &framebufferObject.handle);
        glBindFramebuffer(GL_FRAMEBUFFER, framebufferObject.handle);

        glGenRenderbuffers(1, &framebufferObject.colorRenderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, framebufferObject.colorRenderbuffer);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER,
            framebufferObject.colorRenderbuffer);

        if (m_format.depthBufferSize() > 0 || m_format.stencilBufferSize() > 0) {
            glGenRenderbuffers(1, &framebufferObject.depthRenderbuffer);
            glBindRenderbuffer(GL_RENDERBUFFER, framebufferObject.depthRenderbuffer);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER,
                framebufferObject.depthRenderbuffer);

            if (m_format.stencilBufferSize() > 0)
                glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER,
                    framebufferObject.depthRenderbuffer);
        }

        connect(window, SIGNAL(destroyed(QObject*)), this, SLOT(windowDestroyed(QObject*)));
    }

    // Ensure that the FBO's buffers match the size of the layer
    UIView *view = reinterpret_cast<UIView *>(window->winId());
    CAEAGLLayer *layer = static_cast<CAEAGLLayer *>(view.layer);
    if (framebufferObject.renderbufferWidth != (layer.frame.size.width * layer.contentsScale) ||
        framebufferObject.renderbufferHeight != (layer.frame.size.height * layer.contentsScale)) {

        [EAGLContext setCurrentContext:m_eaglContext];
        glBindFramebuffer(GL_FRAMEBUFFER, framebufferObject.handle);

        glBindRenderbuffer(GL_RENDERBUFFER, framebufferObject.colorRenderbuffer);
        [m_eaglContext renderbufferStorage:GL_RENDERBUFFER fromDrawable:layer];

        glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &framebufferObject.renderbufferWidth);
        glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &framebufferObject.renderbufferHeight);

        if (framebufferObject.depthRenderbuffer) {
            glBindRenderbuffer(GL_RENDERBUFFER, framebufferObject.depthRenderbuffer);

            // FIXME: Support more fine grained control over depth/stencil buffer sizes
            if (m_format.stencilBufferSize() > 0)
                glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8_OES,
                    framebufferObject.renderbufferWidth, framebufferObject.renderbufferHeight);
            else
                glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16,
                    framebufferObject.renderbufferWidth, framebufferObject.renderbufferHeight);
        }

        framebufferObject.isComplete = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;

        if (!framebufferObject.isComplete) {
            qWarning("QIOSContext failed to make complete framebuffer object (%s)",
                qPrintable(fboStatusString(glCheckFramebufferStatus(GL_FRAMEBUFFER))));
        }
    }

    return framebufferObject;
}

GLuint QIOSContext::defaultFramebufferObject(QPlatformSurface *surface) const
{
    if (surface->surface()->surfaceClass() == QSurface::Offscreen) {
        // Binding and rendering to the zero-FBO on iOS seems to be
        // no-ops, so we can safely return 0 here, even if it's not
        // really a valid FBO on iOS.
        return 0;
    }

    return backingFramebufferObjectFor(surface).handle;
}

void QIOSContext::windowDestroyed(QObject *object)
{
    QIOSWindow *window = static_cast<QIOSWindow *>(object);
    if (m_framebufferObjects.contains(window)) {
        EAGLContext *originalContext = [EAGLContext currentContext];
        [EAGLContext setCurrentContext:m_eaglContext];
        deleteBuffers(m_framebufferObjects[window]);
        m_framebufferObjects.remove(window);
        [EAGLContext setCurrentContext:originalContext];
    }
}

QFunctionPointer QIOSContext::getProcAddress(const QByteArray& functionName)
{
    return QFunctionPointer(dlsym(RTLD_DEFAULT, functionName.constData()));
}

bool QIOSContext::isValid() const
{
    return m_eaglContext;
}

bool QIOSContext::isSharing() const
{
    return m_sharedContext;
}

#include "moc_qioscontext.cpp"

