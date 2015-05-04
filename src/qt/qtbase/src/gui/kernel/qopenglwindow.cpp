/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qopenglwindow.h"
#include "qpaintdevicewindow_p.h"
#include <QtGui/QOpenGLFramebufferObject>
#include <QtGui/QOpenGLPaintDevice>
#include <QtGui/QOpenGLFunctions>
#include <QtGui/private/qopengltextureblitter_p.h>
#include <QtGui/private/qopenglextensions_p.h>
#include <QtGui/private/qopenglcontext_p.h>
#include <QtGui/QMatrix4x4>
#include <QtGui/QOffscreenSurface>

QT_BEGIN_NAMESPACE

/*!
  \class QOpenGLWindow
  \inmodule QtGui
  \since 5.4
  \brief The QOpenGLWindow class is a convenience subclass of QWindow to perform OpenGL painting.

  QOpenGLWindow is an enhanced QWindow that allows easily creating windows that
  perform OpenGL rendering using an API that is compatible with QOpenGLWidget
  and is similar to the legacy QGLWidget. Unlike QOpenGLWidget, QOpenGLWindow
  has no dependency on the widgets module and offers better performance.

  A typical application will subclass QOpenGLWindow and reimplement the following
  virtual functions:

  \list

  \li initializeGL() to perform OpenGL resource initialization

  \li resizeGL() to set up the transformation matrices and other window size dependent resources

  \li paintGL() to issue OpenGL commands or draw using QPainter

  \endlist

  To schedule a repaint, call the update() function. Note that this will not
  immediately result in a call to paintGL(). Calling update() multiple times in
  a row will not change the behavior in any way.

  This is a slot so it can be connected to a \l QTimer::timeout() signal to
  perform animation. Note however that in the modern OpenGL world it is a much
  better choice to rely on synchronization to the vertical refresh rate of the
  display. See \l{QSurfaceFormat::setSwapInterval()}{setSwapInterval()} on a
  description of the swap interval. With a swap interval of \c 1, which is the
  case on most systems by default, the
  \l{QOpenGLContext::swapBuffers()}{swapBuffers()} call, that is executed
  internally by QOpenGLWindow after each repaint, will block and wait for
  vsync. This means that whenever the swap is done, an update can be scheduled
  again by calling update(), without relying on timers.

  To request a specific configuration for the context, use setFormat()
  like for any other QWindow. This allows, among others, requesting a
  given OpenGL version and profile, or enabling depth and stencil
  buffers.

  Unlike QWindow, QOpenGLWindow allows opening a painter on itself and perform
  QPainter-based drawing.

  QOpenGLWindow supports multiple update behaviors. The default,
  \c NoPartialUpdate is equivalent to a regular, OpenGL-based QWindow or the
  legacy QGLWidget. In contrast, \c PartialUpdateBlit and \c PartialUpdateBlend are
  more in line with QOpenGLWidget's way of working, where there is always an
  extra, dedicated framebuffer object present. These modes allow, by
  sacrificing some performance, redrawing only a smaller area on each paint and
  having the rest of the content preserved from of the previous frame. This is
  useful for applications than render incrementally using QPainter, because
  this way they do not have to redraw the entire window content on each
  paintGL() call.

  Similarly to QOpenGLWidget, QOpenGLWindow supports the Qt::AA_ShareOpenGLContexts
  attribute. When enabled, the OpenGL contexts of all QOpenGLWindow instances will share
  with each other. This allows accessing each other's shareable OpenGL resources.

  For more information on graphics in Qt, see \l {Graphics}.
 */

/*!
  \enum QOpenGLWindow::UpdateBehavior

  This enum describes the update strategy of the QOpenGLWindow.

  \value NoPartialUpdate Indicates that the entire window surface will
  redrawn on each update and so no additional framebuffers are needed.
  This is the setting used in most cases and is equivalent to how drawing
  directly via QWindow would function.

  \value PartialUpdateBlit Indicates that the drawing performed in paintGL()
  does not cover the entire window. In this case an extra framebuffer object
  is created under the hood, and rendering performed in paintGL() will target
  this framebuffer. This framebuffer is then blitted onto the window surface's
  default framebuffer after each paint. This allows having QPainter-based drawing
  code in paintGL() which only repaints a smaller area at a time, because, unlike
  NoPartialUpdate, the previous content is preserved.

  \value PartialUpdateBlend Similar to PartialUpdateBlit, but instead of using
  framebuffer blits, the contents of the extra framebuffer is rendered by
  drawing a textured quad with blending enabled. This, unlike PartialUpdateBlit,
  allows alpha blended content and works even when the glBlitFramebuffer is
  not available. Performance-wise this setting is likely to be somewhat slower
  than PartialUpdateBlit.
 */

/*!
    \fn void QOpenGLWindow::frameSwapped()

    This signal is emitted after the potentially blocking
    \l{QOpenGLContext::swapBuffers()}{buffer swap} has been done. Applications
    that wish to continuously repaint synchronized to the vertical refresh,
    should issue an update() upon this signal. This allows for a much smoother
    experience compared to the traditional usage of timers.
*/

// GLES2 builds won't have these constants with the suffixless names
#ifndef GL_READ_FRAMEBUFFER
#define GL_READ_FRAMEBUFFER 0x8CA8
#endif
#ifndef GL_DRAW_FRAMEBUFFER
#define GL_DRAW_FRAMEBUFFER 0x8CA9
#endif

class QOpenGLWindowPaintDevice : public QOpenGLPaintDevice
{
public:
    QOpenGLWindowPaintDevice(QOpenGLWindow *window) : m_window(window) { }
    void ensureActiveTarget() Q_DECL_OVERRIDE;

    QOpenGLWindow *m_window;
};

class QOpenGLWindowPrivate : public QPaintDeviceWindowPrivate
{
    Q_DECLARE_PUBLIC(QOpenGLWindow)
public:
    QOpenGLWindowPrivate(QOpenGLWindow::UpdateBehavior updateBehavior)
        : updateBehavior(updateBehavior)
        , hasFboBlit(false)
    {
    }

    ~QOpenGLWindowPrivate()
    {
        Q_Q(QOpenGLWindow);
        if (q->isValid()) {
            q->makeCurrent(); // this works even when the platformwindow is destroyed
            paintDevice.reset(0);
            fbo.reset(0);
            blitter.destroy();
            q->doneCurrent();
        }
    }

    static QOpenGLWindowPrivate *get(QOpenGLWindow *w) { return w->d_func(); }

    void bindFBO()
    {
        if (updateBehavior > QOpenGLWindow::NoPartialUpdate)
            fbo->bind();
        else
            QOpenGLFramebufferObject::bindDefault();
    }

    void beginPaint(const QRegion &region) Q_DECL_OVERRIDE
    {
        Q_UNUSED(region);
        Q_Q(QOpenGLWindow);

        if (!context) {
            context.reset(new QOpenGLContext);
            context->setShareContext(qt_gl_global_share_context());
            context->setFormat(q->requestedFormat());
            if (!context->create())
                qWarning("QOpenGLWindow::beginPaint: Failed to create context");
            if (!context->makeCurrent(q))
                qWarning("QOpenGLWindow::beginPaint: Failed to make context current");

            paintDevice.reset(new QOpenGLWindowPaintDevice(q));
            if (updateBehavior == QOpenGLWindow::PartialUpdateBlit)
                hasFboBlit = QOpenGLFramebufferObject::hasOpenGLFramebufferBlit();

            q->initializeGL();
        } else {
            context->makeCurrent(q);
        }

        const int deviceWidth = q->width() * q->devicePixelRatio();
        const int deviceHeight = q->height() * q->devicePixelRatio();
        const QSize deviceSize(deviceWidth, deviceHeight);
        if (updateBehavior > QOpenGLWindow::NoPartialUpdate) {
            if (!fbo || fbo->size() != deviceSize) {
                QOpenGLFramebufferObjectFormat fboFormat;
                fboFormat.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
                if (q->requestedFormat().samples() > 0) {
                    if (updateBehavior != QOpenGLWindow::PartialUpdateBlend)
                        fboFormat.setSamples(q->requestedFormat().samples());
                    else
                        qWarning("QOpenGLWindow: PartialUpdateBlend does not support multisampling");
                }
                fbo.reset(new QOpenGLFramebufferObject(deviceSize, fboFormat));
                markWindowAsDirty();
            }
        } else {
            markWindowAsDirty();
        }

        paintDevice->setSize(QSize(deviceWidth, deviceHeight));
        paintDevice->setDevicePixelRatio(q->devicePixelRatio());
        context->functions()->glViewport(0, 0, deviceWidth, deviceHeight);

        context->functions()->glBindFramebuffer(GL_FRAMEBUFFER, context->defaultFramebufferObject());

        q->paintUnderGL();

        if (updateBehavior > QOpenGLWindow::NoPartialUpdate)
            fbo->bind();
    }

    void endPaint() Q_DECL_OVERRIDE
    {
        Q_Q(QOpenGLWindow);

        if (updateBehavior > QOpenGLWindow::NoPartialUpdate)
            fbo->release();

        context->functions()->glBindFramebuffer(GL_FRAMEBUFFER, context->defaultFramebufferObject());

        if (updateBehavior == QOpenGLWindow::PartialUpdateBlit && hasFboBlit) {
            const int deviceWidth = q->width() * q->devicePixelRatio();
            const int deviceHeight = q->height() * q->devicePixelRatio();
            QOpenGLExtensions extensions(context.data());
            extensions.glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo->handle());
            extensions.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, context->defaultFramebufferObject());
            extensions.glBlitFramebuffer(0, 0, deviceWidth, deviceHeight,
                                         0, 0, deviceWidth, deviceHeight,
                                         GL_COLOR_BUFFER_BIT, GL_NEAREST);
        } else if (updateBehavior > QOpenGLWindow::NoPartialUpdate) {
            if (updateBehavior == QOpenGLWindow::PartialUpdateBlend) {
                context->functions()->glEnable(GL_BLEND);
                context->functions()->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            }
            if (!blitter.isCreated())
                blitter.create();

            QRect windowRect(QPoint(0, 0), fbo->size());
            QMatrix4x4 target = QOpenGLTextureBlitter::targetTransform(windowRect, windowRect);
            blitter.bind();
            blitter.blit(fbo->texture(), target, QOpenGLTextureBlitter::OriginBottomLeft);
            blitter.release();

            if (updateBehavior == QOpenGLWindow::PartialUpdateBlend)
                context->functions()->glDisable(GL_BLEND);
        }

        q->paintOverGL();
    }

    void flush(const QRegion &region) Q_DECL_OVERRIDE
    {
        Q_UNUSED(region);
        Q_Q(QOpenGLWindow);
        context->swapBuffers(q);
        emit q->frameSwapped();
    }

    QOpenGLWindow::UpdateBehavior updateBehavior;
    bool hasFboBlit;
    QScopedPointer<QOpenGLContext> context;
    QScopedPointer<QOpenGLFramebufferObject> fbo;
    QScopedPointer<QOpenGLWindowPaintDevice> paintDevice;
    QOpenGLTextureBlitter blitter;
    QColor backgroundColor;
    QScopedPointer<QOffscreenSurface> offscreenSurface;
};

void QOpenGLWindowPaintDevice::ensureActiveTarget()
{
    QOpenGLWindowPrivate::get(m_window)->bindFBO();
}

/*!
  Constructs a new QOpenGLWindow with the given \a parent and \a updateBehavior.

  \sa QOpenGLWindow::UpdateBehavior
 */
QOpenGLWindow::QOpenGLWindow(QOpenGLWindow::UpdateBehavior updateBehavior, QWindow *parent)
    : QPaintDeviceWindow(*(new QOpenGLWindowPrivate(updateBehavior)), parent)
{
    setSurfaceType(QSurface::OpenGLSurface);
}

/*!
  \return the update behavior for this QOpenGLWindow.
*/
QOpenGLWindow::UpdateBehavior QOpenGLWindow::updateBehavior() const
{
    Q_D(const QOpenGLWindow);
    return d->updateBehavior;
}

/*!
  \return \c true if the window's OpenGL resources, like the context, have
  been successfully initialized. Note that the return value is always \c false
  until the window becomes exposed (shown).
*/
bool QOpenGLWindow::isValid() const
{
    Q_D(const QOpenGLWindow);
    return d->context && d->context->isValid();
}

/*!
  Prepares for rendering OpenGL content for this window by making the
  corresponding context current and binding the framebuffer object, if there is
  one, in that context context.

  It is not necessary to call this function in most cases, because it is called
  automatically before invoking paintGL(). It is provided nonetheless to support
  advanced, multi-threaded scenarios where a thread different than the GUI or main
  thread may want to update the surface or framebuffer contents. See QOpenGLContext
  for more information on threading related issues.

  This function is suitable for calling also when the underlying platform window
  is already destroyed. This means that it is safe to call this function from
  a QOpenGLWindow subclass' destructor. If there is no native window anymore,
  an offscreen surface is used instead. This ensures that OpenGL resource
  cleanup operations in the destructor will always work, as long as
  this function is called first.

  \sa QOpenGLContext, context(), paintGL(), doneCurrent()
 */
void QOpenGLWindow::makeCurrent()
{
    Q_D(QOpenGLWindow);

    if (!isValid())
        return;

    // The platform window may be destroyed at this stage and therefore
    // makeCurrent() may not safely be called with 'this'.
    if (handle()) {
        d->context->makeCurrent(this);
    } else {
        if (!d->offscreenSurface) {
            d->offscreenSurface.reset(new QOffscreenSurface);
            d->offscreenSurface->setFormat(d->context->format());
            d->offscreenSurface->create();
        }
        d->context->makeCurrent(d->offscreenSurface.data());
    }

    d->bindFBO();
}

/*!
  Releases the context.

  It is not necessary to call this function in most cases, since the widget
  will make sure the context is bound and released properly when invoking
  paintGL().

  \sa makeCurrent()
 */
void QOpenGLWindow::doneCurrent()
{
    Q_D(QOpenGLWindow);

    if (!isValid())
        return;

    d->context->doneCurrent();
}

/*!
  \return The QOpenGLContext used by this window or \c 0 if not yet initialized.
 */
QOpenGLContext *QOpenGLWindow::context() const
{
    Q_D(const QOpenGLWindow);
    return d->context.data();
}

/*!
  The framebuffer object handle used by this window.

  When the update behavior is set to \c NoPartialUpdate, there is no separate
  framebuffer object. In this case the returned value is the ID of the
  default framebuffer.

  Otherwise the value of the ID of the framebuffer object or \c 0 if not
  yet initialized.
 */
GLuint QOpenGLWindow::defaultFramebufferObject() const
{
    Q_D(const QOpenGLWindow);
    if (d->updateBehavior > NoPartialUpdate && d->fbo)
        return d->fbo->handle();
    else if (QOpenGLContext *ctx = QOpenGLContext::currentContext())
        return ctx->defaultFramebufferObject();
    else
        return 0;
}

extern Q_GUI_EXPORT QImage qt_gl_read_framebuffer(const QSize &size, bool alpha_format, bool include_alpha);

/*!
  Returns a 32-bit RGB image of the framebuffer.

  \note This is a potentially expensive operation because it relies on
  glReadPixels() to read back the pixels. This may be slow and can stall the
  GPU pipeline.

  \note When used together with update behavior \c NoPartialUpdate, the returned
  image may not contain the desired content when called after the front and back
  buffers have been swapped (unless preserved swap is enabled in the underlying
  windowing system interface). In this mode the function reads from the back
  buffer and the contents of that may not match the content on the screen (the
  front buffer). In this case the only place where this function can safely be
  used is paintGL() or paintOverGL().
 */
QImage QOpenGLWindow::grabFramebuffer()
{
    if (!isValid())
        return QImage();

    makeCurrent();
    return qt_gl_read_framebuffer(size() * devicePixelRatio(), false, false);
}

/*!
  This virtual function is called once before the first call to paintGL() or
  resizeGL(). Reimplement it in a subclass.

  This function should set up any required OpenGL resources and state.

  There is no need to call makeCurrent() because this has already been done
  when this function is called. Note however that the framebuffer, in case
  partial update mode is used, is not yet available at this stage, so avoid
  issuing draw calls from here. Defer such calls to paintGL() instead.

  \sa paintGL(), resizeGL()
 */
void QOpenGLWindow::initializeGL()
{
}

/*!
  This virtual function is called whenever the widget has been resized.
  Reimplement it in a subclass. The new size is passed in \a w and \a h.

  \note This is merely a convenience function in order to provide an API that is
  compatible with QOpenGLWidget. Unlike with QOpenGLWidget, derived classes are
  free to choose to override resizeEvent() instead of this function.

  \note Avoid issuing OpenGL commands from this function as there may not be a
  context current when it is invoked. If it cannot be avoided, call makeCurrent().

  \note Scheduling updates from here is not necessary. The windowing systems
  will send expose events that trigger an update automatically.

  \sa initializeGL(), paintGL()
 */
void QOpenGLWindow::resizeGL(int w, int h)
{
    Q_UNUSED(w);
    Q_UNUSED(h);
}

/*!
  This virtual function is called whenever the window contents needs to be
  painted. Reimplement it in a subclass.

  There is no need to call makeCurrent() because this has already
  been done when this function is called.

  Before invoking this function, the context and the framebuffer, if there is
  one, are bound, and the viewport is set up by a call to glViewport(). No
  other state is set and no clearing or drawing is performed by the framework.

  \note When using a partial update behavior, like \c PartialUpdateBlend, the
  output of the previous paintGL() call is preserved and, after the additional
  drawing perfomed in the current invocation of the function, the content is
  blitted or blended over the content drawn directly to the window in
  paintUnderGL().

  \sa initializeGL(), resizeGL(), paintUnderGL(), paintOverGL(), UpdateBehavior
 */
void QOpenGLWindow::paintGL()
{
}

/*!
  The virtual function is called before each invocation of paintGL().

  When the update mode is set to \c NoPartialUpdate, there is no difference
  between this function and paintGL(), performing rendering in either of them
  leads to the same result.

  The difference becomes significant when using \c PartialUpdateBlend, where an
  extra framebuffer object is used. There, paintGL() targets this additional
  framebuffer object, which preserves its contents, while paintUnderGL() and
  paintOverGL() target the default framebuffer, i.e. directly the window
  surface, the contents of which is lost after each displayed frame.

  \note Avoid relying on this function when the update behavior is
  \c PartialUpdateBlit. This mode involves blitting the extra framebuffer used by
  paintGL() onto the default framebuffer after each invocation of paintGL(),
  thus overwriting all drawing generated in this function.

  \sa paintGL(), paintOverGL(), UpdateBehavior
 */
void QOpenGLWindow::paintUnderGL()
{
}

/*!
  This virtual function is called after each invocation of paintGL().

  When the update mode is set to NoPartialUpdate, there is no difference
  between this function and paintGL(), performing rendering in either of them
  leads to the same result.

  Like paintUnderGL(), rendering in this function targets the default
  framebuffer of the window, regardless of the update behavior. It gets called
  after paintGL() has returned and the blit (PartialUpdateBlit) or quad drawing
  (PartialUpdateBlend) has been done.

  \sa paintGL(), paintUnderGL(), UpdateBehavior
 */
void QOpenGLWindow::paintOverGL()
{
}

/*!
  Paint \a event handler. Calls paintGL().

  \sa paintGL()
 */
void QOpenGLWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    paintGL();
}

/*!
  Resize \a event handler. Calls resizeGL().

  \sa resizeGL()
 */
void QOpenGLWindow::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event);
    resizeGL(width(), height());
}

/*!
  \internal
 */
int QOpenGLWindow::metric(PaintDeviceMetric metric) const
{
    Q_D(const QOpenGLWindow);

    switch (metric) {
        case PdmDepth:
            if (d->paintDevice)
                return d->paintDevice->depth();
            break;
        case PdmDevicePixelRatio:
            if (d->paintDevice)
                return devicePixelRatio();
            break;
        default:
            break;
    }
    return QPaintDeviceWindow::metric(metric);

}

/*!
  \internal
 */
QPaintDevice *QOpenGLWindow::redirected(QPoint *) const
{
    Q_D(const QOpenGLWindow);
    if (QOpenGLContext::currentContext() == d->context.data())
        return d->paintDevice.data();
    return 0;
}

QT_END_NAMESPACE
