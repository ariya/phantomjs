/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "qoffscreensurface.h"

#include "qguiapplication_p.h"
#include "qscreen.h"
#include "qplatformintegration.h"
#include "qplatformoffscreensurface.h"
#include "qwindow.h"
#include "qplatformwindow.h"

QT_BEGIN_NAMESPACE

/*!
    \class QOffscreenSurface
    \inmodule QtGui
    \since 5.1
    \brief The QOffscreenSurface class represents an offscreen surface in the underlying platform.

    QOffscreenSurface is intended to be used with QOpenGLContext to allow rendering with OpenGL in
    an arbitrary thread without the need to create a QWindow.

    Even though the surface is renderable, the surface's pixels are not accessible.
    QOffscreenSurface should only be used to create OpenGL resources such as textures
    or framebuffer objects.

    An application will typically use QOffscreenSurface to perform some time-consuming tasks in a
    separate thread in order to avoid stalling the main rendering thread. Resources created in the
    QOffscreenSurface's context can be shared with the main OpenGL context. Some common use cases
    are asynchronous texture uploads or rendering into a QOpenGLFramebufferObject.

    How the offscreen surface is implemented depends on the underlying platform, but it will
    typically use a pixel buffer (pbuffer). If the platform doesn't implement or support
    offscreen surfaces, QOffscreenSurface will use an invisible QWindow internally.

    \note In order to create an offscreen surface that is guaranteed to be compatible with
    a given context and window, make sure to set the format to the context's or the
    window's actual format, that is, the QSurfaceFormat returned from
    QOpenGLContext::format() or QWindow::format() \e{after the context or window has been
    created}. Passing the format returned from QWindow::requestedFormat() to setFormat()
    may result in an incompatible offscreen surface since the underlying windowing system
    interface may offer a different set of configurations for window and pbuffer surfaces.
*/
class Q_GUI_EXPORT QOffscreenSurfacePrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QOffscreenSurface)

public:
    QOffscreenSurfacePrivate()
        : QObjectPrivate()
        , surfaceType(QSurface::OpenGLSurface)
        , platformOffscreenSurface(0)
        , offscreenWindow(0)
        , screen(0)
        , size(1, 1)
    {
    }

    ~QOffscreenSurfacePrivate()
    {
    }

    QSurface::SurfaceType surfaceType;
    QPlatformOffscreenSurface *platformOffscreenSurface;
    QWindow *offscreenWindow;
    QSurfaceFormat requestedFormat;
    QScreen *screen;
    QSize size;
};


/*!
    Creates an offscreen surface for the \a targetScreen.

    The underlying platform surface is not created until create() is called.

    \sa setScreen(), create()
*/
QOffscreenSurface::QOffscreenSurface(QScreen *targetScreen)
    : QObject(*new QOffscreenSurfacePrivate(), 0)
    , QSurface(Offscreen)
{
    Q_D(QOffscreenSurface);
    d->screen = targetScreen;
    if (!d->screen)
        d->screen = QGuiApplication::primaryScreen();

    //if your applications aborts here, then chances are your creating a QOffscreenSurface before
    //the screen list is populated.
    Q_ASSERT(d->screen);

    connect(d->screen, SIGNAL(destroyed(QObject*)), this, SLOT(screenDestroyed(QObject*)));
}


/*!
    Destroys the offscreen surface.
*/
QOffscreenSurface::~QOffscreenSurface()
{
    destroy();
}

/*!
    Returns the surface type of the offscreen surface.

    The surface type of an offscreen surface is always QSurface::OpenGLSurface.
*/
QOffscreenSurface::SurfaceType QOffscreenSurface::surfaceType() const
{
    Q_D(const QOffscreenSurface);
    return d->surfaceType;
}

/*!
    Allocates the platform resources associated with the offscreen surface.

    It is at this point that the surface format set using setFormat() gets resolved
    into an actual native surface.

    Call destroy() to free the platform resources if necessary.

    \sa destroy()
*/
void QOffscreenSurface::create()
{
    Q_D(QOffscreenSurface);
    if (!d->platformOffscreenSurface && !d->offscreenWindow) {
        d->platformOffscreenSurface = QGuiApplicationPrivate::platformIntegration()->createPlatformOffscreenSurface(this);
        // No platform offscreen surface, fallback to an invisible window
        if (!d->platformOffscreenSurface) {
            if (QThread::currentThread() != qGuiApp->thread())
                qWarning("Attempting to create QWindow-based QOffscreenSurface outside the gui thread. Expect failures.");
            d->offscreenWindow = new QWindow(d->screen);
            d->offscreenWindow->setSurfaceType(QWindow::OpenGLSurface);
            d->offscreenWindow->setFormat(d->requestedFormat);
            d->offscreenWindow->setGeometry(0, 0, d->size.width(), d->size.height());
            d->offscreenWindow->create();
        }
    }
}

/*!
    Releases the native platform resources associated with this offscreen surface.

    \sa create()
*/
void QOffscreenSurface::destroy()
{
    Q_D(QOffscreenSurface);
    delete d->platformOffscreenSurface;
    d->platformOffscreenSurface = 0;
    if (d->offscreenWindow) {
        d->offscreenWindow->destroy();
        delete d->offscreenWindow;
        d->offscreenWindow = 0;
    }
}

/*!
    Returns \c true if this offscreen surface is valid; otherwise returns \c false.

    The offscreen surface is valid if the platform resources have been successfuly allocated.

    \sa create()
*/
bool QOffscreenSurface::isValid() const
{
    Q_D(const QOffscreenSurface);
    return (d->platformOffscreenSurface && d->platformOffscreenSurface->isValid())
            || (d->offscreenWindow && d->offscreenWindow->handle());
}

/*!
    Sets the offscreen surface \a format.

    The surface format will be resolved in the create() function. Calling
    this function after create() will not re-resolve the surface format of the native surface.

    \sa create(), destroy()
*/
void QOffscreenSurface::setFormat(const QSurfaceFormat &format)
{
    Q_D(QOffscreenSurface);
    d->requestedFormat = format;
}

/*!
    Returns the requested surfaceformat of this offscreen surface.

    If the requested format was not supported by the platform implementation,
    the requestedFormat will differ from the actual offscreen surface format.

    This is the value set with setFormat().

    \sa setFormat(), format()
 */
QSurfaceFormat QOffscreenSurface::requestedFormat() const
{
    Q_D(const QOffscreenSurface);
    return d->requestedFormat;
}

/*!
    Returns the actual format of this offscreen surface.

    After the offscreen surface has been created, this function will return the actual
    surface format of the surface. It might differ from the requested format if the requested
    format could not be fulfilled by the platform.

    \sa create(), requestedFormat()
*/
QSurfaceFormat QOffscreenSurface::format() const
{
    Q_D(const QOffscreenSurface);
    if (d->platformOffscreenSurface)
        return d->platformOffscreenSurface->format();
    if (d->offscreenWindow)
        return d->offscreenWindow->format();
    return d->requestedFormat;
}

/*!
    Returns the size of the offscreen surface.
*/
QSize QOffscreenSurface::size() const
{
    Q_D(const QOffscreenSurface);
    return d->size;
}

/*!
    Returns the screen to which the offscreen surface is connected.

    \sa setScreen()
*/
QScreen *QOffscreenSurface::screen() const
{
    Q_D(const QOffscreenSurface);
    return d->screen;
}

/*!
    Sets the screen to which the offscreen surface is connected.

    If the offscreen surface has been created, it will be recreated on the \a newScreen.

    \sa screen()
*/
void QOffscreenSurface::setScreen(QScreen *newScreen)
{
    Q_D(QOffscreenSurface);
    if (!newScreen)
        newScreen = QGuiApplication::primaryScreen();
    if (newScreen != d->screen) {
        const bool wasCreated = d->platformOffscreenSurface != 0 || d->offscreenWindow != 0;
        if (wasCreated)
            destroy();
        if (d->screen)
            disconnect(d->screen, SIGNAL(destroyed(QObject*)), this, SLOT(screenDestroyed(QObject*)));
        d->screen = newScreen;
        if (newScreen) {
            connect(d->screen, SIGNAL(destroyed(QObject*)), this, SLOT(screenDestroyed(QObject*)));
            if (wasCreated)
                create();
        }
        emit screenChanged(newScreen);
    }
}

/*!
    Called when the offscreen surface's screen is destroyed.

    \internal
*/
void QOffscreenSurface::screenDestroyed(QObject *object)
{
    Q_D(QOffscreenSurface);
    if (object == static_cast<QObject *>(d->screen))
        setScreen(0);
}

/*!
    \fn QOffscreenSurface::screenChanged(QScreen *screen)

    This signal is emitted when an offscreen surface's \a screen changes, either
    by being set explicitly with setScreen(), or automatically when
    the window's screen is removed.
*/

/*!
    Returns the platform offscreen surface corresponding to the offscreen surface.

    \internal
*/
QPlatformOffscreenSurface *QOffscreenSurface::handle() const
{
    Q_D(const QOffscreenSurface);
    return d->platformOffscreenSurface;
}

/*!
    Returns the platform surface corresponding to the offscreen surface.

    \internal
*/
QPlatformSurface *QOffscreenSurface::surfaceHandle() const
{
    Q_D(const QOffscreenSurface);
    if (d->offscreenWindow)
        return d->offscreenWindow->handle();

    return d->platformOffscreenSurface;
}

QT_END_NAMESPACE
