/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qplatformwindowformat_qpa.h"

#include <QtCore/QDebug>

Q_GLOBAL_STATIC(QPlatformWindowFormat, q_platformwindow_default_format);

class QPlatformWindowFormatPrivate
{
public:
    QPlatformWindowFormatPrivate()
        : ref(1)
        , opts(QPlatformWindowFormat::DoubleBuffer | QPlatformWindowFormat::DepthBuffer
             | QPlatformWindowFormat::Rgba | QPlatformWindowFormat::DirectRendering
             | QPlatformWindowFormat::StencilBuffer | QPlatformWindowFormat::DeprecatedFunctions
             | QPlatformWindowFormat::HasWindowSurface)
        , depthSize(-1)
        , accumSize(-1)
        , stencilSize(-1)
        , redSize(-1)
        , greenSize(-1)
        , blueSize(-1)
        , alphaSize(-1)
        , numSamples(-1)
        , swapInterval(-1)
        , windowApi(QPlatformWindowFormat::Raster)
        , sharedContext(0)
    {
    }

    QPlatformWindowFormatPrivate(const QPlatformWindowFormatPrivate *other)
        : ref(1),
          opts(other->opts),
          depthSize(other->depthSize),
          accumSize(other->accumSize),
          stencilSize(other->stencilSize),
          redSize(other->redSize),
          greenSize(other->greenSize),
          blueSize(other->blueSize),
          alphaSize(other->alphaSize),
          numSamples(other->numSamples),
          swapInterval(other->swapInterval),
          windowApi(other->windowApi),
          sharedContext(other->sharedContext)
    {
    }
    QAtomicInt ref;
    QPlatformWindowFormat::FormatOptions opts;
    int depthSize;
    int accumSize;
    int stencilSize;
    int redSize;
    int greenSize;
    int blueSize;
    int alphaSize;
    int numSamples;
    int swapInterval;
    QPlatformWindowFormat::WindowApi windowApi;
    QPlatformGLContext *sharedContext;
};

/*!
    \class QPlatformWindowFormat
    \ingroup painting
    \since 4.8
    \brief The QPlatformWindowFormat class specifies the display format of an OpenGL
    rendering context and if possible attributes of the corresponding QPlatformWindow.

    QWidget has a setter and getter function for QPlatformWindowFormat. These functions can be used
    by the application programmer to signal what kind of format he wants to the window and glcontext
    should have. However, it is not always possible to fulfill these requirements. The application
    programmer should therefore check the resulting QPlatformWindowFormat from QPlatformGLContext
    to see the format that was actually created.

    A display format has several characteristics:
    \list
    \i \link setDoubleBuffer() Double or single buffering.\endlink
    \i \link setDepth() Depth buffer.\endlink
    \i \link setRgba() RGBA or color index mode.\endlink
    \i \link setAlpha() Alpha channel.\endlink
    \i \link setAccum() Accumulation buffer.\endlink
    \i \link setStencil() Stencil buffer.\endlink
    \i \link setStereo() Stereo buffers.\endlink
    \i \link setDirectRendering() Direct rendering.\endlink
    \i \link setSampleBuffers() Multisample buffers.\endlink
    \endlist

    You can also specify preferred bit depths for the color buffer,
    depth buffer, alpha buffer, accumulation buffer and the stencil
    buffer with the functions: setRedBufferSize(), setGreenBufferSize(),
    setBlueBufferSize(), setDepthBufferSize(), setAlphaBufferSize(),
    setAccumBufferSize() and setStencilBufferSize().

    Note that even if you specify that you prefer a 32 bit depth
    buffer (e.g. with setDepthBufferSize(32)), the format that is
    chosen may not have a 32 bit depth buffer, even if there is a
    format available with a 32 bit depth buffer. The main reason for
    this is how the system dependant picking algorithms work on the
    different platforms, and some format options may have higher
    precedence than others.

    You create and tell a QPlatformWindowFormat object what rendering options you
    want from an OpenGL rendering context.

    OpenGL drivers or accelerated hardware may or may not support
    advanced features such as alpha channel or stereographic viewing.
    If you request some features that the driver/hardware does not
    provide when you create a QGLWidget, you will get a rendering
    context with the nearest subset of features.

    There are different ways to define the display characteristics of
    a rendering context. One is to create a QPlatformWindowFormat and make it the
    default for the entire application:
    \snippet doc/src/snippets/code/src_opengl_qgl.cpp 0

    Or you can specify the desired format when creating an object of
    your QGLWidget subclass:
    \snippet doc/src/snippets/code/src_opengl_qgl.cpp 1

    After the widget has been created, you can find out which of the
    requested features the system was able to provide:
    \snippet doc/src/snippets/code/src_opengl_qgl.cpp 2

    \legalese
        OpenGL is a trademark of Silicon Graphics, Inc. in the
        United States and other countries.
    \endlegalese

    \sa QPlatformGLContext, QWidget
*/

/*!
    Constructs a QPlatformWindowFormat object with the following default settings:
    \list
    \i \link setDoubleBuffer() Double buffer:\endlink Enabled.
    \i \link setDepth() Depth buffer:\endlink Enabled.
    \i \link setRgba() RGBA:\endlink Enabled (i.e., color index disabled).
    \i \link setAlpha() Alpha channel:\endlink Disabled.
    \i \link setAccum() Accumulator buffer:\endlink Disabled.
    \i \link setStencil() Stencil buffer:\endlink Enabled.
    \i \link setStereo() Stereo:\endlink Disabled.
    \i \link setDirectRendering() Direct rendering:\endlink Enabled.
    \i \link setSampleBuffers() Multisample buffers:\endlink Disabled.
    \endlist
*/

QPlatformWindowFormat::QPlatformWindowFormat()
{
    d = new QPlatformWindowFormatPrivate;
}


/*!
    Creates a QPlatformWindowFormat object that is a copy of the current
    defaultFormat().

    If \a options is not 0, the default format is modified by the
    specified format options. The \a options parameter should be
    QGL::FormatOption values OR'ed together.

    This constructor makes it easy to specify a certain desired format
    in classes derived from QGLWidget, for example:
    \snippet doc/src/snippets/code/src_opengl_qgl.cpp 3

    Note that there are QGL::FormatOption values to turn format settings
    both on and off; e.g., QGL::DepthBuffer and QGL::NoDepthBuffer,
    QGL::DirectRendering and QGL::IndirectRendering, etc.

    \sa defaultFormat(), setOption()
*/

QPlatformWindowFormat::QPlatformWindowFormat(QPlatformWindowFormat::FormatOptions options)
{
    d = new QPlatformWindowFormatPrivate;
    QPlatformWindowFormat::FormatOptions newOpts = options;
    d->opts = defaultFormat().d->opts;
    d->opts |= (newOpts & 0xffff);
    d->opts &= ~(newOpts >> 16);
}

/*!
    \internal
*/
void QPlatformWindowFormat::detach()
{
    if (d->ref != 1) {
        QPlatformWindowFormatPrivate *newd = new QPlatformWindowFormatPrivate(d);
        if (!d->ref.deref())
            delete d;
        d = newd;
    }
}

/*!
    Constructs a copy of \a other.
*/

QPlatformWindowFormat::QPlatformWindowFormat(const QPlatformWindowFormat &other)
{
    d = other.d;
    d->ref.ref();
}

/*!
    Assigns \a other to this object.
*/

QPlatformWindowFormat &QPlatformWindowFormat::operator=(const QPlatformWindowFormat &other)
{
    if (d != other.d) {
        other.d->ref.ref();
        if (!d->ref.deref())
            delete d;
        d = other.d;
    }
    return *this;
}

/*!
    Destroys the QPlatformWindowFormat.
*/
QPlatformWindowFormat::~QPlatformWindowFormat()
{
    if (!d->ref.deref())
        delete d;
}

/*!
    \fn bool QPlatformWindowFormat::doubleBuffer() const

    Returns true if double buffering is enabled; otherwise returns
    false. Double buffering is enabled by default.

    \sa setDoubleBuffer()
*/

/*!
    If \a enable is true sets double buffering; otherwise sets single
    buffering.

    Double buffering is enabled by default.

    Double buffering is a technique where graphics are rendered on an
    off-screen buffer and not directly to the screen. When the drawing
    has been completed, the program calls a swapBuffers() function to
    exchange the screen contents with the buffer. The result is
    flicker-free drawing and often better performance.

    \sa doubleBuffer(), QGLContext::swapBuffers(),
    QGLWidget::swapBuffers()
*/

void QPlatformWindowFormat::setDoubleBuffer(bool enable)
{
    setOption(enable ? QPlatformWindowFormat::DoubleBuffer : QPlatformWindowFormat::SingleBuffer);
}


/*!
    \fn bool QPlatformWindowFormat::depth() const

    Returns true if the depth buffer is enabled; otherwise returns
    false. The depth buffer is enabled by default.

    \sa setDepth(), setDepthBufferSize()
*/

/*!
    If \a enable is true enables the depth buffer; otherwise disables
    the depth buffer.

    The depth buffer is enabled by default.

    The purpose of a depth buffer (or Z-buffering) is to remove hidden
    surfaces. Pixels are assigned Z values based on the distance to
    the viewer. A pixel with a high Z value is closer to the viewer
    than a pixel with a low Z value. This information is used to
    decide whether to draw a pixel or not.

    \sa depth(), setDepthBufferSize()
*/

void QPlatformWindowFormat::setDepth(bool enable)
{
    setOption(enable ? QPlatformWindowFormat::DepthBuffer : QPlatformWindowFormat::NoDepthBuffer);
}


/*!
    \fn bool QPlatformWindowFormat::rgba() const

    Returns true if RGBA color mode is set. Returns false if color
    index mode is set. The default color mode is RGBA.

    \sa setRgba()
*/

/*!
    If \a enable is true sets RGBA mode. If \a enable is false sets
    color index mode.

    The default color mode is RGBA.

    RGBA is the preferred mode for most OpenGL applications. In RGBA
    color mode you specify colors as red + green + blue + alpha
    quadruplets.

    In color index mode you specify an index into a color lookup
    table.

    \sa rgba()
*/

void QPlatformWindowFormat::setRgba(bool enable)
{
    setOption(enable ? QPlatformWindowFormat::Rgba : QPlatformWindowFormat::ColorIndex);
}


/*!
    \fn bool QPlatformWindowFormat::alpha() const

    Returns true if the alpha buffer in the framebuffer is enabled;
    otherwise returns false. The alpha buffer is disabled by default.

    \sa setAlpha(), setAlphaBufferSize()
*/

/*!
    If \a enable is true enables the alpha buffer; otherwise disables
    the alpha buffer.

    The alpha buffer is disabled by default.

    The alpha buffer is typically used for implementing transparency
    or translucency. The A in RGBA specifies the transparency of a
    pixel.

    \sa alpha(), setAlphaBufferSize()
*/

void QPlatformWindowFormat::setAlpha(bool enable)
{
    setOption(enable ? QPlatformWindowFormat::AlphaChannel : QPlatformWindowFormat::NoAlphaChannel);
}


/*!
    \fn bool QPlatformWindowFormat::accum() const

    Returns true if the accumulation buffer is enabled; otherwise
    returns false. The accumulation buffer is disabled by default.

    \sa setAccum(), setAccumBufferSize()
*/

/*!
    If \a enable is true enables the accumulation buffer; otherwise
    disables the accumulation buffer.

    The accumulation buffer is disabled by default.

    The accumulation buffer is used to create blur effects and
    multiple exposures.

    \sa accum(), setAccumBufferSize()
*/

void QPlatformWindowFormat::setAccum(bool enable)
{
    setOption(enable ? QPlatformWindowFormat::AccumBuffer : QPlatformWindowFormat::NoAccumBuffer);
}


/*!
    \fn bool QPlatformWindowFormat::stencil() const

    Returns true if the stencil buffer is enabled; otherwise returns
    false. The stencil buffer is enabled by default.

    \sa setStencil(), setStencilBufferSize()
*/

/*!
    If \a enable is true enables the stencil buffer; otherwise
    disables the stencil buffer.

    The stencil buffer is enabled by default.

    The stencil buffer masks certain parts of the drawing area so that
    masked parts are not drawn on.

    \sa stencil(), setStencilBufferSize()
*/

void QPlatformWindowFormat::setStencil(bool enable)
{
    setOption(enable ? QPlatformWindowFormat::StencilBuffer: QPlatformWindowFormat::NoStencilBuffer);
}


/*!
    \fn bool QPlatformWindowFormat::stereo() const

    Returns true if stereo buffering is enabled; otherwise returns
    false. Stereo buffering is disabled by default.

    \sa setStereo()
*/

/*!
    If \a enable is true enables stereo buffering; otherwise disables
    stereo buffering.

    Stereo buffering is disabled by default.

    Stereo buffering provides extra color buffers to generate left-eye
    and right-eye images.

    \sa stereo()
*/

void QPlatformWindowFormat::setStereo(bool enable)
{
    setOption(enable ? QPlatformWindowFormat::StereoBuffers : QPlatformWindowFormat::NoStereoBuffers);
}


/*!
    \fn bool QPlatformWindowFormat::directRendering() const

    Returns true if direct rendering is enabled; otherwise returns
    false.

    Direct rendering is enabled by default.

    \sa setDirectRendering()
*/

/*!
    If \a enable is true enables direct rendering; otherwise disables
    direct rendering.

    Direct rendering is enabled by default.

    Enabling this option will make OpenGL bypass the underlying window
    system and render directly from hardware to the screen, if this is
    supported by the system.

    \sa directRendering()
*/

void QPlatformWindowFormat::setDirectRendering(bool enable)
{
    setOption(enable ? QPlatformWindowFormat::DirectRendering : QPlatformWindowFormat::IndirectRendering);
}

/*!
    \fn bool QPlatformWindowFormat::sampleBuffers() const

    Returns true if multisample buffer support is enabled; otherwise
    returns false.

    The multisample buffer is disabled by default.

    \sa setSampleBuffers()
*/

/*!
    If \a enable is true, a GL context with multisample buffer support
    is picked; otherwise ignored.

    \sa sampleBuffers(), setSamples(), samples()
*/
void QPlatformWindowFormat::setSampleBuffers(bool enable)
{
    setOption(enable ? QPlatformWindowFormat::SampleBuffers : QPlatformWindowFormat::NoSampleBuffers);
}

/*!
    Returns the number of samples per pixel when multisampling is
    enabled. By default, the highest number of samples that is
    available is used.

    \sa setSampleBuffers(), sampleBuffers(), setSamples()
*/
int QPlatformWindowFormat::samples() const
{
   return d->numSamples;
}

/*!
    Set the preferred number of samples per pixel when multisampling
    is enabled to \a numSamples. By default, the highest number of
    samples available is used.

    \sa setSampleBuffers(), sampleBuffers(), samples()
*/
void QPlatformWindowFormat::setSamples(int numSamples)
{
    detach();
    if (numSamples < 0) {
        qWarning("QPlatformWindowFormat::setSamples: Cannot have negative number of samples per pixel %d", numSamples);
        return;
    }
    d->numSamples = numSamples;
    setSampleBuffers(numSamples > 0);
}

/*!
    \since 4.2

    Set the preferred swap interval. This can be used to sync the GL
    drawing into a system window to the vertical refresh of the screen.
    Setting an \a interval value of 0 will turn the vertical refresh syncing
    off, any value higher than 0 will turn the vertical syncing on.

    Under Windows and under X11, where the \c{WGL_EXT_swap_control}
    and \c{GLX_SGI_video_sync} extensions are used, the \a interval
    parameter can be used to set the minimum number of video frames
    that are displayed before a buffer swap will occur. In effect,
    setting the \a interval to 10, means there will be 10 vertical
    retraces between every buffer swap.

    Under Windows the \c{WGL_EXT_swap_control} extension has to be present,
    and under X11 the \c{GLX_SGI_video_sync} extension has to be present.
*/
void QPlatformWindowFormat::setSwapInterval(int interval)
{
    detach();
    d->swapInterval = interval;
}

/*!
    \since 4.2

    Returns the currently set swap interval. -1 is returned if setting
    the swap interval isn't supported in the system GL implementation.
*/
int QPlatformWindowFormat::swapInterval() const
{
    return d->swapInterval;
}

void QPlatformWindowFormat::setWindowApi(QPlatformWindowFormat::WindowApi api)
{
    detach();
    d->windowApi = api;
}

QPlatformWindowFormat::WindowApi QPlatformWindowFormat::windowApi() const
{
    return d->windowApi;
}

void QPlatformWindowFormat::setSharedContext(QPlatformGLContext *context)
{
    d->sharedContext = context;
}

QPlatformGLContext *QPlatformWindowFormat::sharedGLContext() const
{
    return d->sharedContext;
}

/*!
    \fn bool QPlatformWindowFormat::hasWindowSurface() const

    Returns true if the corresponding widget has an instance of QWindowSurface.

    Otherwise returns false.

    WindowSurface is enabled by default.
*/

/*!
    If \a enable is true a top level QWidget will create a QWindowSurface at creation;

    otherwise the QWidget will only have a QPlatformWindow.

    This is useful for QGLWidget where the QPlatformGLContext controls the surface.
*/

void QPlatformWindowFormat::setWindowSurface(bool enable)
{
    setOption(enable ? QPlatformWindowFormat::HasWindowSurface : QPlatformWindowFormat::NoWindowSurface);
}

/*!
    Sets the format option to \a opt.

    \sa testOption()
*/

void QPlatformWindowFormat::setOption(QPlatformWindowFormat::FormatOptions opt)
{
    detach();
    if (opt & 0xffff)
        d->opts |= opt;
    else
       d->opts &= ~(opt >> 16);
}



/*!
    Returns true if format option \a opt is set; otherwise returns false.

    \sa setOption()
*/

bool QPlatformWindowFormat::testOption(QPlatformWindowFormat::FormatOptions opt) const
{
    if (opt & 0xffff)
       return (d->opts & opt) != 0;
    else
       return (d->opts & (opt >> 16)) == 0;
}

/*!
    Set the minimum depth buffer size to \a size.

    \sa depthBufferSize(), setDepth(), depth()
*/
void QPlatformWindowFormat::setDepthBufferSize(int size)
{
    detach();
    if (size < 0) {
        qWarning("QPlatformWindowFormat::setDepthBufferSize: Cannot set negative depth buffer size %d", size);
        return;
    }
    d->depthSize = size;
    setDepth(size > 0);
}

/*!
    Returns the depth buffer size.

    \sa depth(), setDepth(), setDepthBufferSize()
*/
int QPlatformWindowFormat::depthBufferSize() const
{
   return d->depthSize;
}

/*!
    \since 4.2

    Set the preferred red buffer size to \a size.

    \sa setGreenBufferSize(), setBlueBufferSize(), setAlphaBufferSize()
*/
void QPlatformWindowFormat::setRedBufferSize(int size)
{
    detach();
    if (size < 0) {
        qWarning("QPlatformWindowFormat::setRedBufferSize: Cannot set negative red buffer size %d", size);
        return;
    }
    d->redSize = size;
}

/*!
    \since 4.2

    Returns the red buffer size.

    \sa setRedBufferSize()
*/
int QPlatformWindowFormat::redBufferSize() const
{
   return d->redSize;
}

/*!
    \since 4.2

    Set the preferred green buffer size to \a size.

    \sa setRedBufferSize(), setBlueBufferSize(), setAlphaBufferSize()
*/
void QPlatformWindowFormat::setGreenBufferSize(int size)
{
    detach();
    if (size < 0) {
        qWarning("QPlatformWindowFormat::setGreenBufferSize: Cannot set negative green buffer size %d", size);
        return;
    }
    d->greenSize = size;
}

/*!
    \since 4.2

    Returns the green buffer size.

    \sa setGreenBufferSize()
*/
int QPlatformWindowFormat::greenBufferSize() const
{
   return d->greenSize;
}

/*!
    \since 4.2

    Set the preferred blue buffer size to \a size.

    \sa setRedBufferSize(), setGreenBufferSize(), setAlphaBufferSize()
*/
void QPlatformWindowFormat::setBlueBufferSize(int size)
{
    detach();
    if (size < 0) {
        qWarning("QPlatformWindowFormat::setBlueBufferSize: Cannot set negative blue buffer size %d", size);
        return;
    }
    d->blueSize = size;
}

/*!
    \since 4.2

    Returns the blue buffer size.

    \sa setBlueBufferSize()
*/
int QPlatformWindowFormat::blueBufferSize() const
{
   return d->blueSize;
}

/*!
    Set the preferred alpha buffer size to \a size.
    This function implicitly enables the alpha channel.

    \sa setRedBufferSize(), setGreenBufferSize(), alphaBufferSize()
*/
void QPlatformWindowFormat::setAlphaBufferSize(int size)
{
    detach();
    if (size < 0) {
        qWarning("QPlatformWindowFormat::setAlphaBufferSize: Cannot set negative alpha buffer size %d", size);
        return;
    }
    d->alphaSize = size;
    setAlpha(size > 0);
}

/*!
    Returns the alpha buffer size.

    \sa alpha(), setAlpha(), setAlphaBufferSize()
*/
int QPlatformWindowFormat::alphaBufferSize() const
{
   return d->alphaSize;
}

/*!
    Set the preferred accumulation buffer size, where \a size is the
    bit depth for each RGBA component.

    \sa accum(), setAccum(), accumBufferSize()
*/
void QPlatformWindowFormat::setAccumBufferSize(int size)
{
    detach();
    if (size < 0) {
        qWarning("QPlatformWindowFormat::setAccumBufferSize: Cannot set negative accumulate buffer size %d", size);
        return;
    }
    d->accumSize = size;
    setAccum(size > 0);
}

/*!
    Returns the accumulation buffer size.

    \sa setAccumBufferSize(), accum(), setAccum()
*/
int QPlatformWindowFormat::accumBufferSize() const
{
   return d->accumSize;
}

/*!
    Set the preferred stencil buffer size to \a size.

    \sa stencilBufferSize(), setStencil(), stencil()
*/
void QPlatformWindowFormat::setStencilBufferSize(int size)
{
    detach();
    if (size < 0) {
        qWarning("QPlatformWindowFormat::setStencilBufferSize: Cannot set negative stencil buffer size %d", size);
        return;
    }
    d->stencilSize = size;
    setStencil(size > 0);
}

/*!
    Returns the stencil buffer size.

    \sa stencil(), setStencil(), setStencilBufferSize()
*/
int QPlatformWindowFormat::stencilBufferSize() const
{
   return d->stencilSize;
}

/*!
    Returns the default QPlatformWindowFormat for the application. All QGLWidget
    objects that are created use this format unless another format is
    specified, e.g. when they are constructed.

    If no special default format has been set using
    setDefaultFormat(), the default format is the same as that created
    with QPlatformWindowFormat().

    \sa setDefaultFormat()
*/

QPlatformWindowFormat QPlatformWindowFormat::defaultFormat()
{
    return *q_platformwindow_default_format();
}

/*!
    Sets a new default QPlatformWindowFormat for the application to \a f. For
    example, to set single buffering as the default instead of double
    buffering, your main() might contain code like this:
    \snippet doc/src/snippets/code/src_opengl_qgl.cpp 4

    \sa defaultFormat()
*/

void QPlatformWindowFormat::setDefaultFormat(const QPlatformWindowFormat &f)
{
    *q_platformwindow_default_format() = f;
}


/*
    Returns the default QPlatformWindowFormat for overlay contexts.

    The default overlay format is:
    \list
    \i \link setDoubleBuffer() Double buffer:\endlink Disabled.
    \i \link setDepth() Depth buffer:\endlink Disabled.
    \i \link setRgba() RGBA:\endlink Disabled (i.e., color index enabled).
    \i \link setAlpha() Alpha channel:\endlink Disabled.
    \i \link setAccum() Accumulator buffer:\endlink Disabled.
    \i \link setStencil() Stencil buffer:\endlink Disabled.
    \i \link setStereo() Stereo:\endlink Disabled.
    \i \link setDirectRendering() Direct rendering:\endlink Enabled.
    \i \link setSampleBuffers() Multisample buffers:\endlink Disabled.
    \endlist

    \sa setDefaultFormat()
*/

//QPlatformWindowFormat QPlatformWindowFormat::defaultOverlayFormat()
//{
//    return *defaultOverlayFormatInstance();
//}

///*!
//    Sets a new default QPlatformWindowFormat for overlay contexts to \a f. This
//    format is used whenever a QGLWidget is created with a format that
//    hasOverlay() enabled.

//    For example, to get a double buffered overlay context (if
//    available), use code like this:

//    \snippet doc/src/snippets/code/src_opengl_qgl.cpp 5

//    As usual, you can find out after widget creation whether the
//    underlying OpenGL system was able to provide the requested
//    specification:

//    \snippet doc/src/snippets/code/src_opengl_qgl.cpp 6

//    \sa defaultOverlayFormat()
//*/

//void QPlatformWindowFormat::setDefaultOverlayFormat(const QPlatformWindowFormat &f)
//{
//    QPlatformWindowFormat *defaultFormat = defaultOverlayFormatInstance();
//    *defaultFormat = f;
//    // Make sure the user doesn't request that the overlays themselves
//    // have overlays, since it is unlikely that the system supports
//    // infinitely many planes...
//    defaultFormat->setOverlay(false);
//}


/*!
    \since 4.8

    Returns true if all the options of the two QPlatformWindowFormat objects
    \a a and \a b are equal; otherwise returns false.

    \relates QPlatformWindowFormat
*/

bool operator==(const QPlatformWindowFormat& a, const QPlatformWindowFormat& b)
{
    return (a.d == b.d) || ((int) a.d->opts == (int) b.d->opts
        && a.d->alphaSize == b.d->alphaSize
        && a.d->accumSize == b.d->accumSize
        && a.d->stencilSize == b.d->stencilSize
        && a.d->depthSize == b.d->depthSize
        && a.d->redSize == b.d->redSize
        && a.d->greenSize == b.d->greenSize
        && a.d->blueSize == b.d->blueSize
        && a.d->numSamples == b.d->numSamples
        && a.d->swapInterval == b.d->swapInterval
        && a.d->windowApi == b.d->windowApi);
}


/*!
    \since 4.8

    Returns false if all the options of the two QPlatformWindowFormat objects
    \a a and \a b are equal; otherwise returns true.

    \relates QPlatformWindowFormat
*/

bool operator!=(const QPlatformWindowFormat& a, const QPlatformWindowFormat& b)
{
    return !(a == b);
}

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug dbg, const QPlatformWindowFormat &f)
{
    const QPlatformWindowFormatPrivate * const d = f.d;

    dbg.nospace() << "QGLFormat("
                  << "options " << d->opts
                  << ", depthBufferSize " << d->depthSize
                  << ", accumBufferSize " << d->accumSize
                  << ", stencilBufferSize " << d->stencilSize
                  << ", redBufferSize " << d->redSize
                  << ", greenBufferSize " << d->greenSize
                  << ", blueBufferSize " << d->blueSize
                  << ", alphaBufferSize " << d->alphaSize
                  << ", samples " << d->numSamples
                  << ", swapInterval " << d->swapInterval
                  << ')';

    return dbg.space();
}
#endif
