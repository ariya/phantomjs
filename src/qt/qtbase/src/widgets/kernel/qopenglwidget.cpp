/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#include "qopenglwidget.h"
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLFramebufferObject>
#include <QtGui/QOffscreenSurface>
#include <QtGui/QOpenGLFunctions>
#include <QtGui/QWindow>
#include <QtGui/QGuiApplication>
#include <QtGui/QScreen>
#include <QtGui/QOpenGLPaintDevice>
#include <QtGui/qpa/qplatformwindow.h>
#include <QtGui/qpa/qplatformintegration.h>
#include <QtGui/private/qguiapplication_p.h>
#include <QtGui/private/qopenglextensions_p.h>
#include <QtGui/private/qfont_p.h>
#include <QtGui/private/qopenglpaintdevice_p.h>
#include <QtWidgets/private/qwidget_p.h>

QT_BEGIN_NAMESPACE

/*!
  \class QOpenGLWidget
  \inmodule QtWidgets
  \since 5.4

  \brief The QOpenGLWidget class is a widget for rendering OpenGL graphics.

  QOpenGLWidget provides functionality for displaying OpenGL graphics
  integrated into a Qt application. It is very simple to use: Make
  your class inherit from it and use the subclass like any other
  QWidget, except that you have the choice between using QPainter and
  standard OpenGL rendering commands.

  QOpenGLWidget provides three convenient virtual functions that you
  can reimplement in your subclass to perform the typical OpenGL
  tasks:

  \list
  \li paintGL() - Renders the OpenGL scene. Gets called whenever the widget
  needs to be updated.
  \li resizeGL() - Sets up the OpenGL viewport, projection, etc. Gets
  called whenever the widget has been resized (and also when it
  is shown for the first time because all newly created widgets get a
  resize event automatically).
  \li initializeGL() - Sets up the OpenGL resources and state. Gets called
  once before the first time resizeGL() or paintGL() is called.
  \endlist

  If you need to trigger a repaint from places other than paintGL() (a
  typical example is when using \l{QTimer}{timers} to animate scenes),
  you should call the widget's update() function to schedule an update.

  Your widget's OpenGL rendering context is made current when
  paintGL(), resizeGL(), or initializeGL() is called. If you need to
  call the standard OpenGL API functions from other places (e.g. in
  your widget's constructor or in your own paint functions), you
  must call makeCurrent() first.

  All rendering happens into an OpenGL framebuffer
  object. makeCurrent() ensure that it is bound in the context. Keep
  this in mind when creating and binding additional framebuffer
  objects in the rendering code in paintGL(). Never re-bind the
  framebuffer with ID 0. Instead, call defaultFramebufferObject() to
  get the ID that should be bound.

  QOpenGLWidget allows using different OpenGL versions and profiles
  when the platform supports it. Just set the requested format via
  setFormat(). Keep in mind however that having multiple QOpenGLWidget
  instances in the same window requires that they all use the same
  format, or at least formats that do not make the contexts
  non-sharable. To overcome this issue, prefer using
  QSurfaceFormat::setDefaultFormat() instead of setFormat().

  \section1 Painting Techniques

  As described above, subclass QOpenGLWidget to render pure 3D content in the
  following way:

  \list

  \li Reimplement the initializeGL() and resizeGL() functions to
  set up the OpenGL state and provide a perspective transformation.

  \li Reimplement paintGL() to paint the 3D scene, calling only
  OpenGL functions.

  \endlist

  It is also possible to draw 2D graphics onto a QOpenGLWidget subclass using QPainter:

  \list

  \li In paintGL(), instead of issuing OpenGL commands, construct a QPainter
      object for use on the widget.

  \li Draw primitives using QPainter's member functions.

  \li Direct OpenGL commands can still be issued. However, you must make sure
  these are enclosed by a call to the painter's beginNativePainting() and
  endNativePainting().

  \endlist

  When performing drawing using QPainter only, it is also possible to perform
  the painting like it is done for ordinary widgets: by reimplementing paintEvent().

  \list

  \li Reimplement the paintEvent() function.

  \li Construct a QPainter object targeting the widget. Either pass the widget to the
  constructor or the QPainter::begin() function.

  \li Draw primitives using QPainter's member functions.

  \li Painting finishes then the QPainter instance is destroyed. Alternatively,
  call QPainter::end() explicitly.

  \endlist

  \section1 OpenGL Function Calls, Headers and QOpenGLFunctions

  When making OpenGL function calls, it is strongly recommended to avoid calling
  the functions directly. Instead, prefer using QOpenGLFunctions (when making
  portable applications) or the versioned variants (for example,
  QOpenGLFunctions_3_2_Core and similar, when targeting modern, desktop-only
  OpenGL). This way the application will work correctly in all Qt build
  configurations, including the ones that perform dynamic OpenGL implementation
  loading which means applications are not directly linking to an GL
  implementation and thus direct function calls are not feasible.

  In paintGL() the current context is always accessible by caling
  QOpenGLContext::currentContext(). From this context an already initialized,
  ready-to-be-used QOpenGLFunctions instance is retrievable by calling
  QOpenGLContext::functions(). An alternative to prefixing every GL call is to
  inherit from QOpenGLFunctions and call
  QOpenGLFunctions::initializeOpenGLFunctions() in initializeGL().

  As for the OpenGL headers, note that in most cases there will be no need to
  directly include any headers like GL.h. The OpenGL-related Qt headers will
  include qopengl.h which will in turn include an appropriate header for the
  system. This might be an OpenGL ES 3.x or 2.0 header, the highest version that
  is available, or a system-provided gl.h. In addition, a copy of the extension
  headers (called glext.h on some systems) is provided as part of Qt both for
  OpenGL and OpenGL ES. These will get included automatically on platforms where
  feasible. This means that constants and function pointer typedefs from ARB,
  EXT, OES extensions are automatically available.

  \section1 Code Examples

  To get started, the simplest QOpenGLWidget subclass could like like the following:

  \snippet code/doc_gui_widgets_qopenglwidget.cpp 0

  Alternatively, the prefixing of each and every OpenGL call can be avoided by deriving
  from QOpenGLFunctions instead:

  \snippet code/doc_gui_widgets_qopenglwidget.cpp 1

  To get a context compatible with a given OpenGL version or profile, or to
  request depth and stencil buffers, call setFormat():

  \snippet code/doc_gui_widgets_qopenglwidget.cpp 2

  With OpenGL 3.0+ contexts, when portability is not important, the versioned
  QOpenGLFunctions variants give easy access to all the modern OpenGL functions
  available in a given version:

  \snippet code/doc_gui_widgets_qopenglwidget.cpp 3

  As described above, it is simpler and more robust to set the requested format
  globally so that it applies to all windows and contexts during the lifetime of
  the application. Below is an example of this:

  \snippet code/doc_gui_widgets_qopenglwidget.cpp 6

  \section1 Relation to QGLWidget

  The legacy QtOpenGL module (classes prefixed with QGL) provides a widget
  called QGLWidget. QOpenGLWidget is intended to be a modern replacement for
  it. Therefore, especially in new applications, the general recommendation is
  to use QOpenGLWidget.

  While the API is very similar, there is an important difference between the
  two: QOpenGLWidget always renders offscreen, using framebuffer
  objects. QGLWidget on the other hand uses a native window and surface. The
  latter causes issues when using it in complex user interfaces since, depending
  on the platform, such native child widgets may have various limitations,
  regarding stacking orders for example. QOpenGLWidget avoids this by not
  creating a separate native window.

  Due to being backed by a framebuffer object, the behavior of QOpenGLWidget is
  very similar to QOpenGLWindow with the update behavior set to \c
  PartialUpdateBlit or \c PartialUpdateBlend. This means that the contents are
  preserved between paintGL() calls so that incremental rendering is
  possible. With QGLWidget (and naturally QOpenGLWindow with the default update
  behavior) this is usually not the case because swapping the buffers leaves the
  back buffer with undefined contents.

  \note Most applications do not need incremental rendering because they will
  render everything in the view on every paint call. In this case it is
  important to call glClear() as early as possible in paintGL(). This helps
  mobile GPUs that use a tile-based architecture to recognize that the tile
  buffer does not need to be reloaded with the framebuffer's previous
  contents. Omitting the clear call can lead to significant performance drops on
  such systems.

  \note Avoid calling winId() on a QOpenGLWidget. This function triggers the creation of
  a native window, resulting in reduced performance and possibly rendering glitches.

  \section1 Differences to QGLWidget

  Besides the main conceptual difference of being backed by a framebuffer object, there
  are a number of smaller, internal differences between QOpenGLWidget and the older
  QGLWidget:

  \list

  \li OpenGL state when invoking paintGL(). QOpenGLWidget sets up the viewport via
  glViewport(). It does not perform any clearing.

  \li Clearing when starting to paint via QPainter. Unlike regular widgets, QGLWidget
  defaulted to a value of \c true for
  \l{QWidget::autoFillBackground()}{autoFillBackground}. It then performed clearing to the
  palette's background color every time QPainter::begin() was used. QOpenGLWidget does not
  follow this: \l{QWidget::autoFillBackground()}{autoFillBackground} defaults to false,
  like for any other widget. The only exception is when being used as a viewport for other
  widgets like QGraphicsView. In such a case autoFillBackground will be automatically set
  to true to ensure compatibility with QGLWidget-based viewports.

  \endlist

  \section1 Multisampling

  To enable multisampling, set the number of requested samples on the
  QSurfaceFormat that is passed to setFormat(). On systems that do not support
  it the request may get ignored.

  Multisampling support requires support for multisampled renderbuffers and
  framebuffer blits. On OpenGL ES 2.0 implementations it is likely that these
  will not be present. This means that multisampling will not be available. With
  modern OpenGL versions and OpenGL ES 3.0 and up this is usually not a problem
  anymore.

  \section1 Threading

  Performing offscreen rendering on worker threads, for example to generate
  textures that are then used in the GUI/main thread in paintGL(), are supported
  by exposing the widget's QOpenGLContext so that additional contexts sharing
  with it can be created on each thread.

  Drawing directly to the QOpenGLWidget's framebuffer outside the GUI/main
  thread is possible by reimplementing paintEvent() to do nothing. The context's
  thread affinity has to be changed via QObject::moveToThread(). After that,
  makeCurrent() and doneCurrent() are usable on the worker thread. Be careful to
  move the context back to the GUI/main thread afterwards.

  Unlike QGLWidget, triggering a buffer swap just for the QOpenGLWidget is not
  possible since there is no real, onscreen native surface for it. Instead, it
  is up to the widget stack to manage composition and buffer swaps on the gui
  thread. When a thread is done updating the framebuffer, call update() \b{on
  the GUI/main thread} to schedule composition.

  Extra care has to be taken to avoid using the framebuffer when the GUI/main
  thread is performing compositing. The signals aboutToCompose() and
  frameSwapped() will be emitted when the composition is starting and
  ending. They are emitted on the GUI/main thread. This means that by using a
  direct connection aboutToCompose() can block the GUI/main thread until the
  worker thread has finished its rendering. After that, the worker thread must
  perform no further rendering until the frameSwapped() signal is emitted. If
  this is not acceptable, the worker thread has to implement a double buffering
  mechanism. This involves drawing using an alternative render target, that is
  fully controlled by the thread, e.g. an additional framebuffer object, and
  blitting to the QOpenGLWidget's framebuffer at a suitable time.

  \section1 Context Sharing

  When multiple QOpenGLWidgets are added as children to the same top-level
  widget, their contexts will share with each other. This does not apply for
  QOpenGLWidget instances that belong to different windows.

  This means that all QOpenGLWidgets in the same window can access each other's
  sharable resources, like textures, and there is no need for an extra "global
  share" context, as was the case with QGLWidget.

  To set up sharing between QOpenGLWidget instances belonging to different
  windows, set the Qt::AA_ShareOpenGLContexts application attribute before
  instantiating QApplication. This will trigger sharing between all
  QOpenGLWidget instances without any further steps.

  Creating extra QOpenGLContext instances that share resources like textures
  with the QOpenGLWidget's context is also possible. Simply pass the pointer
  returned from context() to QOpenGLContext::setShareContext() before calling
  QOpenGLContext::create(). The resulting context can also be used on a
  different thread, allowing threaded generation of textures and asynchronous
  texture uploads.

  Note that QOpenGLWidget expects a standard conformant implementation of
  resource sharing when it comes to the underlying graphics drivers. For
  example, some drivers, in particular for mobile and embedded hardware, have
  issues with setting up sharing between an existing context and others that are
  created later. Some other drivers may behave in unexpected ways when trying to
  utilize shared resources between different threads.

  \section1 Resource Initialization and Cleanup

  The QOpenGLWidget's associated OpenGL context is guaranteed to be current
  whenever initializeGL() and paintGL() are invoked. Do not attempt to create
  OpenGL resources before initializeGL() is called. For example, attempting to
  compile shaders, initialize vertex buffer objects or upload texture data will
  fail when done in a subclass's constructor. These operations must be deferred
  to initializeGL(). Some of Qt's OpenGL helper classes, like QOpenGLBuffer or
  QOpenGLVertexArrayObject, have a matching deferred behavior: they can be
  instantiated without a context, but all initialization is deferred until a
  create(), or similar, call. This means that they can be used as normal
  (non-pointer) member variables in a QOpenGLWidget subclass, but the create()
  or similar function can only be called from initializeGL(). Be aware however
  that not all classes are designed like this. When in doubt, make the member
  variable a pointer and create and destroy the instance dynamically in
  initializeGL() and the destructor, respectively.

  Releasing the resources also needs the context to be current. Therefore
  destructors that perform such cleanup are expected to call makeCurrent()
  before moving on to destroy any OpenGL resources or wrappers. Avoid deferred
  deletion via \l{QObject::deleteLater()}{deleteLater()} or the parenting
  mechanism of QObject. There is no guarantee the correct context will be
  current at the time the instance in question is really destroyed.

  A typical subclass will therefore often look like the following when it comes
  to resource initialization and destruction:

  \snippet code/doc_gui_widgets_qopenglwidget.cpp 4

  This is naturally not the only possible solution. One alternative is to use
  the \l{QOpenGLContext::aboutToBeDestroyed()}{aboutToBeDestroyed()} signal of
  QOpenGLContext. By connecting a slot, using direct connection, to this signal,
  it is possible to perform cleanup whenever the the underlying native context
  handle, or the entire QOpenGLContext instance, is going to be released. The
  following snippet is in principal equivalent to the previous one:

  \snippet code/doc_gui_widgets_qopenglwidget.cpp 5

  \note For widgets that change their associated top-level window multiple times
  during their lifetime, a combined approach is essential. Whenever the widget
  or a parent of it gets reparented so that the top-level window becomes
  different, the widget's associated context is destroyed and a new one is
  created. This is then followed by a call to initializeGL() where all OpenGL
  resources must get reinitialized. Due to this the only option to perform
  proper cleanup is to connect to the context's aboutToBeDestroyed()
  signal. Note that the context in question may not be the current one when the
  signal gets emitted. Therefore it is good practice to call makeCurrent() in
  the connected slot. Additionally, the same cleanup steps must be performed
  from the derived class' destructor, since the slot connected to the signal
  will not get invoked when the widget is being destroyed.

  \note When Qt::AA_ShareOpenGLContexts is set, the widget's context never
  changes, not even when reparenting because the widget's associated texture is
  guaranteed to be accessible also from the new top-level's context.

  Proper cleanup is especially important due to context sharing. Even though
  each QOpenGLWidget's associated context is destroyed together with the
  QOpenGLWidget, the sharable resources in that context, like textures, will
  stay valid until the top-level window, in which the QOpenGLWidget lived, is
  destroyed. Additionally, settings like Qt::AA_ShareOpenGLContexts and some Qt
  modules may trigger an even wider scope for sharing contexts, potentially
  leading to keeping the resources in question alive for the entire lifetime of
  the application. Therefore the safest and most robust is always to perform
  explicit cleanup for all resources and resource wrappers used in the
  QOpenGLWidget.

  \section1 Limitations

  Putting other widgets underneath and making the QOpenGLWidget transparent will
  not lead to the expected results: The widgets underneath will not be
  visible. This is because in practice the QOpenGLWidget is drawn before all
  other regular, non-OpenGL widgets, and so see-through type of solutions are
  not feasible. Other type of layouts, like having widgets on top of the
  QOpenGLWidget, will function as expected.

  When absolutely necessary, this limitation can be overcome by setting the
  Qt::WA_AlwaysStackOnTop attribute on the QOpenGLWidget. Be aware however that
  this breaks stacking order, for example it will not be possible to have other
  widgets on top of the QOpenGLWidget, so it should only be used in situations
  where a semi-transparent QOpenGLWidget with other widgets visible underneath
  is required.

  Note that this does not apply when there are no other widgets underneath and
  the intention is to have a semi-transparent window. In that case the
  traditional approach of setting Qt::WA_TranslucentBackground is sufficient.

  \e{OpenGL is a trademark of Silicon Graphics, Inc. in the United States and other
  countries.}

  \sa QOpenGLFunctions, QOpenGLWindow, Qt::AA_ShareOpenGLContexts
*/

/*!
    \fn void QOpenGLWidget::aboutToCompose()

    This signal is emitted when the widget's top-level window is about to begin
    composing the textures of its QOpenGLWidget children and the other widgets.
*/

/*!
    \fn void QOpenGLWidget::frameSwapped()

    This signal is emitted after the widget's top-level window has finished
    composition and returned from its potentially blocking
    QOpenGLContext::swapBuffers() call.
*/

/*!
    \fn void QOpenGLWidget::aboutToResize()

    This signal is emitted when the widget's size is changed and therefore the
    framebuffer object is going to be recreated.
*/

/*!
    \fn void QOpenGLWidget::resized()

    This signal is emitted right after the framebuffer object has been recreated
    due to resizing the widget.
*/

class QOpenGLWidgetPaintDevicePrivate : public QOpenGLPaintDevicePrivate
{
public:
    QOpenGLWidgetPaintDevicePrivate(QOpenGLWidget *widget)
        : QOpenGLPaintDevicePrivate(QSize()),
          w(widget) { }

    void beginPaint() Q_DECL_OVERRIDE;

    QOpenGLWidget *w;
};

class QOpenGLWidgetPaintDevice : public QOpenGLPaintDevice
{
public:
    QOpenGLWidgetPaintDevice(QOpenGLWidget *widget)
        : QOpenGLPaintDevice(*new QOpenGLWidgetPaintDevicePrivate(widget)) { }
    void ensureActiveTarget() Q_DECL_OVERRIDE;
};

class QOpenGLWidgetPrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QOpenGLWidget)
public:
    QOpenGLWidgetPrivate()
        : context(0),
          fbo(0),
          resolvedFbo(0),
          surface(0),
          initialized(false),
          fakeHidden(false),
          paintDevice(0),
          inBackingStorePaint(false),
          flushPending(false)
    {
        requestedFormat = QSurfaceFormat::defaultFormat();
    }

    ~QOpenGLWidgetPrivate()
    {
        reset();
    }

    void reset();
    void recreateFbo();

    GLuint textureId() const Q_DECL_OVERRIDE;

    void initialize();
    void invokeUserPaint();
    void render();

    QImage grabFramebuffer() Q_DECL_OVERRIDE;
    void beginBackingStorePainting() Q_DECL_OVERRIDE { inBackingStorePaint = true; }
    void endBackingStorePainting() Q_DECL_OVERRIDE { inBackingStorePaint = false; }
    void beginCompose() Q_DECL_OVERRIDE;
    void endCompose() Q_DECL_OVERRIDE;
    void initializeViewportFramebuffer() Q_DECL_OVERRIDE;
    void resizeViewportFramebuffer() Q_DECL_OVERRIDE;
    void resolveSamples() Q_DECL_OVERRIDE;

    QOpenGLContext *context;
    QOpenGLFramebufferObject *fbo;
    QOpenGLFramebufferObject *resolvedFbo;
    QOffscreenSurface *surface;
    bool initialized;
    bool fakeHidden;
    QOpenGLPaintDevice *paintDevice;
    bool inBackingStorePaint;
    QSurfaceFormat requestedFormat;
    bool flushPending;
};

void QOpenGLWidgetPaintDevicePrivate::beginPaint()
{
    // NB! autoFillBackground is and must be false by default. Otherwise we would clear on
    // every QPainter begin() which is not desirable. This is only for legacy use cases,
    // like using QOpenGLWidget as the viewport of a graphics view, that expect clearing
    // with the palette's background color.
    if (w->autoFillBackground()) {
        QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();
        if (w->testAttribute(Qt::WA_TranslucentBackground)) {
            f->glClearColor(0, 0, 0, 0);
        } else {
            QColor c = w->palette().brush(w->backgroundRole()).color();
            float alpha = c.alphaF();
            f->glClearColor(c.redF() * alpha, c.greenF() * alpha, c.blueF() * alpha, alpha);
        }
        f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    }
}

void QOpenGLWidgetPaintDevice::ensureActiveTarget()
{
    QOpenGLWidgetPaintDevicePrivate *d = static_cast<QOpenGLWidgetPaintDevicePrivate *>(d_ptr.data());
    QOpenGLWidgetPrivate *wd = static_cast<QOpenGLWidgetPrivate *>(QWidgetPrivate::get(d->w));
    if (!wd->initialized)
        return;

    if (QOpenGLContext::currentContext() != wd->context)
        d->w->makeCurrent();
    else
        wd->fbo->bind();

    // When used as a viewport, drawing is done via opening a QPainter on the widget
    // without going through paintEvent(). We will have to make sure a glFlush() is done
    // before the texture is accessed also in this case.
    wd->flushPending = true;
}

GLuint QOpenGLWidgetPrivate::textureId() const
{
    Q_Q(const QOpenGLWidget);
    if (!q->isWindow() && q->internalWinId()) {
        qWarning() << "QOpenGLWidget cannot be used as a native child widget."
                   << "Consider setting Qt::AA_DontCreateNativeWidgetAncestors and Siblings.";
        return 0;
    }
    return resolvedFbo ? resolvedFbo->texture() : (fbo ? fbo->texture() : 0);
}

void QOpenGLWidgetPrivate::reset()
{
    Q_Q(QOpenGLWidget);

    // Destroy the OpenGL resources first. These need the context to be current.
    if (initialized)
        q->makeCurrent();

    delete paintDevice;
    paintDevice = 0;
    delete fbo;
    fbo = 0;
    delete resolvedFbo;
    resolvedFbo = 0;

    if (initialized)
        q->doneCurrent();

    // Delete the context first, then the surface. Slots connected to
    // the context's aboutToBeDestroyed() may still call makeCurrent()
    // to perform some cleanup.
    delete context;
    context = 0;
    delete surface;
    surface = 0;
    initialized = fakeHidden = inBackingStorePaint = false;
}

void QOpenGLWidgetPrivate::recreateFbo()
{
    Q_Q(QOpenGLWidget);

    emit q->aboutToResize();

    context->makeCurrent(surface);

    delete fbo;
    fbo = 0;
    delete resolvedFbo;
    resolvedFbo = 0;

    int samples = context->format().samples();
    QOpenGLExtensions *extfuncs = static_cast<QOpenGLExtensions *>(context->functions());
    if (!extfuncs->hasOpenGLExtension(QOpenGLExtensions::FramebufferMultisample))
        samples = 0;

    QOpenGLFramebufferObjectFormat format;
    format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    format.setSamples(samples);

    const QSize deviceSize = q->size() * q->devicePixelRatio();
    fbo = new QOpenGLFramebufferObject(deviceSize, format);
    if (samples > 0)
        resolvedFbo = new QOpenGLFramebufferObject(deviceSize);

    fbo->bind();
    context->functions()->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    paintDevice->setSize(deviceSize);
    paintDevice->setDevicePixelRatio(q->devicePixelRatio());

    emit q->resized();
}

void QOpenGLWidgetPrivate::beginCompose()
{
    Q_Q(QOpenGLWidget);
    if (flushPending) {
        flushPending = false;
        q->makeCurrent();
        context->functions()->glFlush();
    }
    emit q->aboutToCompose();
}

void QOpenGLWidgetPrivate::endCompose()
{
    Q_Q(QOpenGLWidget);
    emit q->frameSwapped();
}

void QOpenGLWidgetPrivate::initialize()
{
    Q_Q(QOpenGLWidget);
    if (initialized)
        return;

    // Get our toplevel's context with which we will share in order to make the
    // texture usable by the underlying window's backingstore.
    QWidget *tlw = q->window();
    QOpenGLContext *shareContext = get(tlw)->shareContext();
    if (!shareContext) {
        qWarning("QOpenGLWidget: Cannot be used without a context shared with the toplevel.");
        return;
    }

    QScopedPointer<QOpenGLContext> ctx(new QOpenGLContext);
    ctx->setShareContext(shareContext);
    ctx->setFormat(requestedFormat);
    if (!ctx->create()) {
        qWarning("QOpenGLWidget: Failed to create context");
        return;
    }

    // Propagate settings that make sense only for the tlw.
    QSurfaceFormat tlwFormat = tlw->windowHandle()->format();
    if (requestedFormat.swapInterval() != tlwFormat.swapInterval()) {
        // Most platforms will pick up the changed swap interval on the next
        // makeCurrent or swapBuffers.
        tlwFormat.setSwapInterval(requestedFormat.swapInterval());
        tlw->windowHandle()->setFormat(tlwFormat);
    }
    if (requestedFormat.swapBehavior() != tlwFormat.swapBehavior()) {
        tlwFormat.setSwapBehavior(requestedFormat.swapBehavior());
        tlw->windowHandle()->setFormat(tlwFormat);
    }

    // The top-level window's surface is not good enough since it causes way too
    // much trouble with regards to the QSurfaceFormat for example. So just like
    // in QQuickWidget, use a dedicated QOffscreenSurface.
    surface = new QOffscreenSurface;
    surface->setFormat(ctx->format());
    surface->create();

    if (!ctx->makeCurrent(surface)) {
        qWarning("QOpenGLWidget: Failed to make context current");
        return;
    }

    paintDevice = new QOpenGLWidgetPaintDevice(q);
    paintDevice->setSize(q->size() * q->devicePixelRatio());
    paintDevice->setDevicePixelRatio(q->devicePixelRatio());

    context = ctx.take();
    initialized = true;

    q->initializeGL();
}

void QOpenGLWidgetPrivate::resolveSamples()
{
    Q_Q(QOpenGLWidget);
    if (resolvedFbo) {
        q->makeCurrent();
        QRect rect(QPoint(0, 0), fbo->size());
        QOpenGLFramebufferObject::blitFramebuffer(resolvedFbo, rect, fbo, rect);
    }
}

void QOpenGLWidgetPrivate::invokeUserPaint()
{
    Q_Q(QOpenGLWidget);
    QOpenGLFunctions *f = QOpenGLContext::currentContext()->functions();

    f->glViewport(0, 0, q->width() * q->devicePixelRatio(), q->height() * q->devicePixelRatio());
    q->paintGL();
    f->glFlush();
}

void QOpenGLWidgetPrivate::render()
{
    Q_Q(QOpenGLWidget);

    if (fakeHidden || !initialized)
        return;

    q->makeCurrent();
    invokeUserPaint();
}

extern Q_GUI_EXPORT QImage qt_gl_read_framebuffer(const QSize &size, bool alpha_format, bool include_alpha);

QImage QOpenGLWidgetPrivate::grabFramebuffer()
{
    Q_Q(QOpenGLWidget);
    if (!initialized)
        return QImage();

    render();
    resolveSamples();
    q->makeCurrent();
    QImage res = qt_gl_read_framebuffer(q->size() * q->devicePixelRatio(), false, false);

    return res;
}

void QOpenGLWidgetPrivate::initializeViewportFramebuffer()
{
    Q_Q(QOpenGLWidget);
    // Legacy behavior for compatibility with QGLWidget when used as a graphics view
    // viewport: enable clearing on each painter begin.
    q->setAutoFillBackground(true);
}

void QOpenGLWidgetPrivate::resizeViewportFramebuffer()
{
    Q_Q(QOpenGLWidget);
    if (!initialized)
        return;

    if (!fbo || q->size() * q->devicePixelRatio() != fbo->size())
        recreateFbo();
}

/*!
  Constructs a widget which is a child of \a parent, with widget flags set to \a f.
 */
QOpenGLWidget::QOpenGLWidget(QWidget *parent, Qt::WindowFlags f)
    : QWidget(*(new QOpenGLWidgetPrivate), parent, f)
{
    Q_D(QOpenGLWidget);
    if (QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::RasterGLSurface))
        d->setRenderToTexture();
    else
        qWarning("QOpenGLWidget is not supported on this platform.");
}

/*!
  Destroys the widget
 */
QOpenGLWidget::~QOpenGLWidget()
{
}

/*!
  Sets the requested surface \a format.

  When the format is not explicitly set via this function, the format returned by
  QSurfaceFormat::defaultFormat() will be used. This means that when having multiple
  OpenGL widgets, individual calls to this function can be replaced by one single call to
  QSurfaceFormat::setDefaultFormat() before creating the first widget.

  \note Requesting an alpha buffer via this function, or by setting
  Qt::WA_TranslucentBackground, will not lead to the desired results when the intention is
  to make other widgets beneath visible. Instead, use Qt::WA_AlwaysStackOnTop to enable
  semi-transparent QOpenGLWidget instances with other widgets visible underneath. Keep in
  mind however that this breaks the stacking order, so it will no longer be possible to
  have other widgets on top of the QOpenGLWidget. When the intention is to have a
  semi-transparent top-level window, Qt::WA_TranslucentBackground is sufficient.

  \sa format(), Qt::WA_AlwaysStackOnTop, QSurfaceFormat::setDefaultFormat()
 */
void QOpenGLWidget::setFormat(const QSurfaceFormat &format)
{
    Q_UNUSED(format);
    Q_D(QOpenGLWidget);
    if (d->initialized) {
        qWarning("QOpenGLWidget: Already initialized, setting the format has no effect");
        return;
    }

    d->requestedFormat = format;
}

/*!
    Returns the context and surface format used by this widget and its toplevel
    window.

    After the widget and its toplevel have both been created, resized and shown,
    this function will return the actual format of the context. This may differ
    from the requested format if the request could not be fulfilled by the
    platform. It is also possible to get larger color buffer sizes than
    requested.

    When the widget's window and the related OpenGL resources are not yet
    initialized, the return value is the format that has been set via
    setFormat().

    \sa setFormat(), context()
 */
QSurfaceFormat QOpenGLWidget::format() const
{
    Q_D(const QOpenGLWidget);
    return d->initialized ? d->context->format() : d->requestedFormat;
}

/*!
  \return \e true if the widget and OpenGL resources, like the context, have
  been successfully initialized. Note that the return value is always false
  until the widget is shown.
*/
bool QOpenGLWidget::isValid() const
{
    Q_D(const QOpenGLWidget);
    return d->initialized && d->context->isValid();
}

/*!
  Prepares for rendering OpenGL content for this widget by making the
  corresponding context current and binding the framebuffer object in that
  context.

  It is not necessary to call this function in most cases, because it
  is called automatically before invoking paintGL().

  \sa context(), paintGL(), doneCurrent()
 */
void QOpenGLWidget::makeCurrent()
{
    Q_D(QOpenGLWidget);
    if (!d->initialized) {
        qWarning("QOpenGLWidget: Cannot make uninitialized widget current");
        return;
    }

    d->context->makeCurrent(d->surface);

    if (d->fbo) // there may not be one if we are in reset()
        d->fbo->bind();
}

/*!
  Releases the context.

  It is not necessary to call this function in most cases, since the
  widget will make sure the context is bound and released properly
  when invoking paintGL().
 */
void QOpenGLWidget::doneCurrent()
{
    Q_D(QOpenGLWidget);
    if (!d->initialized)
        return;

    d->context->doneCurrent();
}

/*!
  \return The QOpenGLContext used by this widget or \c 0 if not yet initialized.

  \note The context and the framebuffer object used by the widget changes when
  reparenting the widget via setParent().

  \sa QOpenGLContext::setShareContext(), defaultFramebufferObject()
 */
QOpenGLContext *QOpenGLWidget::context() const
{
    Q_D(const QOpenGLWidget);
    return d->context;
}

/*!
  \return The framebuffer object handle or \c 0 if not yet initialized.

  \note The framebuffer object belongs to the context returned by context()
  and may not be accessible from other contexts.

  \note The context and the framebuffer object used by the widget changes when
  reparenting the widget via setParent(). In addition, the framebuffer object
  changes on each resize.

  \sa context()
 */
GLuint QOpenGLWidget::defaultFramebufferObject() const
{
    Q_D(const QOpenGLWidget);
    return d->fbo ? d->fbo->handle() : 0;
}

/*!
  This virtual function is called once before the first call to
  paintGL() or resizeGL(). Reimplement it in a subclass.

  This function should set up any required OpenGL resources and state.

  There is no need to call makeCurrent() because this has already been
  done when this function is called. Note however that the framebuffer
  is not yet available at this stage, so avoid issuing draw calls from
  here. Defer such calls to paintGL() instead.

  \sa paintGL(), resizeGL()
*/
void QOpenGLWidget::initializeGL()
{
}

/*!
  This virtual function is called whenever the widget has been
  resized. Reimplement it in a subclass. The new size is passed in
  \a w and \a h.

  There is no need to call makeCurrent() because this has already been
  done when this function is called. Additionally, the framebuffer is
  also bound.

  \sa initializeGL(), paintGL()
*/
void QOpenGLWidget::resizeGL(int w, int h)
{
    Q_UNUSED(w);
    Q_UNUSED(h);
}

/*!
  This virtual function is called whenever the widget needs to be
  painted. Reimplement it in a subclass.

  There is no need to call makeCurrent() because this has already
  been done when this function is called.

  Before invoking this function, the context and the framebuffer are
  bound, and the viewport is set up by a call to glViewport(). No
  other state is set and no clearing or drawing is performed by the
  framework.

  \sa initializeGL(), resizeGL()
*/
void QOpenGLWidget::paintGL()
{
}

/*!
  Handles resize events that are passed in the \a e event parameter.
  Calls the virtual function resizeGL().

  \note Avoid overriding this function in derived classes. If that is not
  feasible, make sure that QOpenGLWidget's implementation is invoked
  too. Otherwise the underlying framebuffer object and related resources will
  not get resized properly and will lead to incorrect rendering.
*/
void QOpenGLWidget::resizeEvent(QResizeEvent *e)
{
    Q_D(QOpenGLWidget);

    if (e->size().isEmpty()) {
        d->fakeHidden = true;
        return;
    }
    d->fakeHidden = false;

    d->initialize();
    if (!d->initialized)
        return;

    d->recreateFbo();
    resizeGL(width(), height());
    d->invokeUserPaint();
    d->resolveSamples();
}

/*!
  Handles paint events.

  Calling QWidget::update() will lead to sending a paint event \a e,
  and thus invoking this function. (NB this is asynchronous and will
  happen at some point after returning from update()). This function
  will then, after some preparation, call the virtual paintGL() to
  update the contents of the QOpenGLWidget's framebuffer. The widget's
  top-level window will then composite the framebuffer's texture with
  the rest of the window.
*/
void QOpenGLWidget::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e);
    Q_D(QOpenGLWidget);
    if (!d->initialized)
        return;

    if (updatesEnabled())
        d->render();
}

/*!
  Renders and returns a 32-bit RGB image of the framebuffer.

  \note This is a potentially expensive operation because it relies on glReadPixels()
  to read back the pixels. This may be slow and can stall the GPU pipeline.
*/
QImage QOpenGLWidget::grabFramebuffer()
{
    Q_D(QOpenGLWidget);
    return d->grabFramebuffer();
}

/*!
  \internal
*/
int QOpenGLWidget::metric(QPaintDevice::PaintDeviceMetric metric) const
{
    Q_D(const QOpenGLWidget);
    if (d->inBackingStorePaint)
        return QWidget::metric(metric);

    QWidget *tlw = window();
    QScreen *screen = tlw && tlw->windowHandle() ? tlw->windowHandle()->screen() : 0;
    if (!screen && QGuiApplication::primaryScreen())
        screen = QGuiApplication::primaryScreen();

    const float dpmx = qt_defaultDpiX() * 100. / 2.54;
    const float dpmy = qt_defaultDpiY() * 100. / 2.54;

    switch (metric) {
    case PdmWidth:
        return width();
    case PdmHeight:
        return height();
    case PdmDepth:
        return 32;
    case PdmWidthMM:
        if (screen)
            return width() * screen->physicalSize().width() / screen->geometry().width();
        else
            return width() * 1000 / dpmx;
    case PdmHeightMM:
        if (screen)
            return height() * screen->physicalSize().height() / screen->geometry().height();
        else
            return height() * 1000 / dpmy;
    case PdmNumColors:
        return 0;
    case PdmDpiX:
        if (screen)
            return qRound(screen->logicalDotsPerInchX());
        else
            return qRound(dpmx * 0.0254);
    case PdmDpiY:
        if (screen)
            return qRound(screen->logicalDotsPerInchY());
        else
            return qRound(dpmy * 0.0254);
    case PdmPhysicalDpiX:
        if (screen)
            return qRound(screen->physicalDotsPerInchX());
        else
            return qRound(dpmx * 0.0254);
    case PdmPhysicalDpiY:
        if (screen)
            return qRound(screen->physicalDotsPerInchY());
        else
            return qRound(dpmy * 0.0254);
    case PdmDevicePixelRatio:
        if (screen)
            return screen->devicePixelRatio();
        else
            return 1.0;
    default:
        qWarning("QOpenGLWidget::metric(): unknown metric %d", metric);
        return 0;
    }
}

/*!
  \internal
*/
QPaintDevice *QOpenGLWidget::redirected(QPoint *p) const
{
    Q_D(const QOpenGLWidget);
    if (d->inBackingStorePaint)
        return QWidget::redirected(p);

    return d->paintDevice;
}

/*!
  \internal
*/
QPaintEngine *QOpenGLWidget::paintEngine() const
{
    Q_D(const QOpenGLWidget);
    // QWidget needs to "punch a hole" into the backingstore. This needs the
    // normal paint engine and device, not the GL one. So in this mode, behave
    // like a normal widget.
    if (d->inBackingStorePaint)
        return QWidget::paintEngine();

    if (!d->initialized)
        return 0;

    return d->paintDevice->paintEngine();
}

/*!
  \internal
*/
bool QOpenGLWidget::event(QEvent *e)
{
    Q_D(QOpenGLWidget);
    switch (e->type()) {
    case QEvent::WindowChangeInternal:
        if (qGuiApp->testAttribute(Qt::AA_ShareOpenGLContexts))
            break;
        if (d->initialized)
            d->reset();
        // FALLTHROUGH
    case QEvent::Show: // reparenting may not lead to a resize so reinitalize on Show too
        if (!d->initialized && !size().isEmpty() && window() && window()->windowHandle()) {
            d->initialize();
            if (d->initialized)
                d->recreateFbo();
        }
        break;
    case QEvent::ScreenChangeInternal:
        if (d->initialized && d->paintDevice->devicePixelRatio() != devicePixelRatio())
            d->recreateFbo();
        break;
    default:
        break;
    }
    return QWidget::event(e);
}

QT_END_NAMESPACE
