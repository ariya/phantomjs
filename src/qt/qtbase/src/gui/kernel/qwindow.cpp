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

#include "qwindow.h"

#include <qpa/qplatformwindow.h>
#include <qpa/qplatformintegration.h>
#include "qsurfaceformat.h"
#ifndef QT_NO_OPENGL
#include <qpa/qplatformopenglcontext.h>
#include "qopenglcontext.h"
#endif
#include "qscreen.h"

#include "qwindow_p.h"
#include "qguiapplication_p.h"
#ifndef QT_NO_ACCESSIBILITY
#  include "qaccessible.h"
#endif

#include <private/qevent_p.h>

#include <QtCore/QTimer>
#include <QtCore/QDebug>

#include <QStyleHints>
#include <qpa/qplatformcursor.h>

QT_BEGIN_NAMESPACE

/*!
    \class QWindow
    \inmodule QtGui
    \since 5.0
    \brief The QWindow class represents a window in the underlying windowing system.

    A window that is supplied a parent becomes a native child window of
    their parent window.

    An application will typically use QWidget or QQuickView for its UI, and not
    QWindow directly. Still, it is possible to render directly to a QWindow
    with QBackingStore or QOpenGLContext, when wanting to keep dependencies to
    a minimum or when wanting to use OpenGL directly. The
    \l{Raster Window Example} and \l{OpenGL Window Example}
    are useful reference examples for how to render to a QWindow using
    either approach.

    \section1 Resource management

    Windows can potentially use a lot of memory. A usual measurement is
    width times height times color depth. A window might also include multiple
    buffers to support double and triple buffering, as well as depth and stencil
    buffers. To release a window's memory resources, call the destroy() function.

    \section1 Content orientation

    QWindow has reportContentOrientationChange() that can be used to specify
    the layout of the window contents in relation to the screen. The content
    orientation is simply a hint to the windowing system about which
    orientation the window contents are in.  It's useful when you wish to keep
    the same window size, but rotate the contents instead, especially when
    doing rotation animations between different orientations. The windowing
    system might use this value to determine the layout of system popups or
    dialogs.

    \section1 Visibility and Windowing system exposure.

    By default, the window is not visible, and you must call setVisible(true),
    or show() or similar to make it visible. To make a window hidden again,
    call setVisible(false) or hide(). The visible property describes the state
    the application wants the window to be in. Depending on the underlying
    system, a visible window might still not be shown on the screen. It could,
    for instance, be covered by other opaque windows or moved outside the
    physical area of the screen. On windowing systems that have exposure
    notifications, the isExposed() accessor describes whether the window should
    be treated as directly visible on screen. The exposeEvent() function is
    called whenever the windows exposure in the windowing system changes.  On
    windowing systems that do not make this information visible to the
    application, isExposed() will simply return the same value as isVisible().

    QWindow::Visibility queried through visibility() is a convenience API
    combining the functions of visible() and windowState().

    \section1 Rendering

    There are two Qt APIs that can be used to render content into a window,
    QBackingStore for rendering with a QPainter and flushing the contents
    to a window with type QSurface::RasterSurface, and QOpenGLContext for
    rendering with OpenGL to a window with type QSurface::OpenGLSurface.

    The application can start rendering as soon as isExposed() returns \c true,
    and can keep rendering until it isExposed() returns \c false. To find out when
    isExposed() changes, reimplement exposeEvent(). The window will always get
    a resize event before the first expose event.

    \section1 Initial geometry

    If the window's width and height are left uninitialized, the window will
    get a reasonable default geometry from the platform window. If the position
    is left uninitialized, then the platform window will allow the windowing
    system to position the window. For example on X11, the window manager
    usually does some kind of smart positioning to try to avoid having new
    windows completely obscure existing windows. However setGeometry()
    initializes both the position and the size, so if you want a fixed size but
    an automatic position, you should call resize() or setWidth() and
    setHeight() instead.
*/

/*!
    Creates a window as a top level on the \a targetScreen.

    The window is not shown until setVisible(true), show(), or similar is called.

    \sa setScreen()
*/
QWindow::QWindow(QScreen *targetScreen)
    : QObject(*new QWindowPrivate(), 0)
    , QSurface(QSurface::Window)
{
    Q_D(QWindow);
    d->screen = targetScreen;
    if (!d->screen)
        d->screen = QGuiApplication::primaryScreen();
    d->init();
}

/*!
    Creates a window as a child of the given \a parent window.

    The window will be embedded inside the parent window, its coordinates
    relative to the parent.

    The screen is inherited from the parent.

    \sa setParent()
*/
QWindow::QWindow(QWindow *parent)
    : QObject(*new QWindowPrivate(), parent)
    , QSurface(QSurface::Window)
{
    Q_D(QWindow);
    d->parentWindow = parent;
    if (parent)
        d->screen = parent->screen();
    if (!d->screen)
        d->screen = QGuiApplication::primaryScreen();
    d->init();
}

/*!
    Creates a window as a child of the given \a parent window with the \a dd
    private implementation.

    The window will be embedded inside the parent window, its coordinates
    relative to the parent.

    The screen is inherited from the parent.

    \internal
    \sa setParent()
*/
QWindow::QWindow(QWindowPrivate &dd, QWindow *parent)
    : QObject(dd, parent)
    , QSurface(QSurface::Window)
{
    Q_D(QWindow);
    d->parentWindow = parent;
    if (parent)
        d->screen = parent->screen();
    if (!d->screen)
        d->screen = QGuiApplication::primaryScreen();
    d->init();
}

/*!
    Destroys the window.
*/
QWindow::~QWindow()
{
    if (QGuiApplicationPrivate::focus_window == this)
        QGuiApplicationPrivate::focus_window = 0;
    if (QGuiApplicationPrivate::currentMouseWindow == this)
        QGuiApplicationPrivate::currentMouseWindow = 0;
    if (QGuiApplicationPrivate::tabletPressTarget == this)
        QGuiApplicationPrivate::tabletPressTarget = 0;
    QGuiApplicationPrivate::window_list.removeAll(this);
    destroy();
}

void QWindowPrivate::init()
{
    Q_Q(QWindow);

    // If your application aborts here, you are probably creating a QWindow
    // before the screen list is populated.
    if (!screen) {
        qFatal("Cannot create window: no screens available");
        exit(1);
    }
    QObject::connect(screen, SIGNAL(destroyed(QObject*)), q, SLOT(screenDestroyed(QObject*)));
    QGuiApplicationPrivate::window_list.prepend(q);
}

/*!
    \enum QWindow::Visibility
    \since 5.1

    This enum describes what part of the screen the window occupies or should
    occupy.

    \value Windowed The window occupies part of the screen, but not necessarily
    the entire screen. This state will occur only on windowing systems which
    support showing multiple windows simultaneously. In this state it is
    possible for the user to move and resize the window manually, if
    WindowFlags permit it and if it is supported by the windowing system.

    \value Minimized The window is reduced to an entry or icon on the task bar,
    dock, task list or desktop, depending on how the windowing system handles
    minimized windows.

    \value Maximized The window occupies one entire screen, and the titlebar is
    still visible. On most windowing systems this is the state achieved by
    clicking the maximize button on the toolbar.

    \value FullScreen The window occupies one entire screen, is not resizable,
    and there is no titlebar. On some platforms which do not support showing
    multiple simultaneous windows, this can be the usual visibility when the
    window is not hidden.

    \value AutomaticVisibility This means to give the window a default visible
    state, which might be fullscreen or windowed depending on the platform.
    It can be given as a parameter to setVisibility but will never be
    read back from the visibility accessor.

    \value Hidden The window is not visible in any way, however it may remember
    a latent visibility which can be restored by setting AutomaticVisibility.
*/

/*!
    \property QWindow::visibility
    \brief the screen-occupation state of the window
    \since 5.1

    Visibility is whether the window should appear in the windowing system as
    normal, minimized, maximized, fullscreen or hidden.

    To set the visibility to AutomaticVisibility means to give the window
    a default visible state, which might be fullscreen or windowed depending on
    the platform.
    When reading the visibility property you will always get the actual state,
    never AutomaticVisibility.
*/
QWindow::Visibility QWindow::visibility() const
{
    Q_D(const QWindow);
    return d->visibility;
}

void QWindow::setVisibility(Visibility v)
{
    switch (v) {
    case Hidden:
        hide();
        break;
    case AutomaticVisibility:
        show();
        break;
    case Windowed:
        showNormal();
        break;
    case Minimized:
        showMinimized();
        break;
    case Maximized:
        showMaximized();
        break;
    case FullScreen:
        showFullScreen();
        break;
    default:
        Q_ASSERT(false);
        break;
    }
}

void QWindowPrivate::updateVisibility()
{
    Q_Q(QWindow);

    QWindow::Visibility old = visibility;

    if (visible) {
        switch (windowState) {
        case Qt::WindowMinimized:
            visibility = QWindow::Minimized;
            break;
        case Qt::WindowMaximized:
            visibility = QWindow::Maximized;
            break;
        case Qt::WindowFullScreen:
            visibility = QWindow::FullScreen;
            break;
        case Qt::WindowNoState:
            visibility = QWindow::Windowed;
            break;
        default:
            Q_ASSERT(false);
            break;
        }
    } else {
        visibility = QWindow::Hidden;
    }

    if (visibility != old)
        emit q->visibilityChanged(visibility);
}

void QWindowPrivate::setScreen(QScreen *newScreen, bool recreate)
{
    Q_Q(QWindow);
    if (newScreen != screen) {
        const bool shouldRecreate = recreate && platformWindow != 0
            && !(screen && screen->virtualSiblings().contains(newScreen));
        if (shouldRecreate)
            q->destroy();
        if (screen)
            q->disconnect(screen, SIGNAL(destroyed(QObject*)), q, SLOT(screenDestroyed(QObject*)));
        screen = newScreen;
        if (newScreen) {
            q->connect(screen, SIGNAL(destroyed(QObject*)), q, SLOT(screenDestroyed(QObject*)));
            if (shouldRecreate)
                q->create();
        }
        emit q->screenChanged(newScreen);
    }
}

void QWindowPrivate::clearFocusObject()
{
}

/*!
    Sets the \a surfaceType of the window.

    Specifies whether the window is meant for raster rendering with
    QBackingStore, or OpenGL rendering with QOpenGLContext.

    The surfaceType will be used when the native surface is created
    in the create() function. Calling this function after the native
    surface has been created requires calling destroy() and create()
    to release the old native surface and create a new one.

    \sa QBackingStore, QOpenGLContext, create(), destroy()
*/
void QWindow::setSurfaceType(SurfaceType surfaceType)
{
    Q_D(QWindow);
    d->surfaceType = surfaceType;
}

/*!
    Returns the surface type of the window.

    \sa setSurfaceType()
*/
QWindow::SurfaceType QWindow::surfaceType() const
{
    Q_D(const QWindow);
    return d->surfaceType;
}

/*!
    \property QWindow::visible
    \brief whether the window is visible or not

    This property controls the visibility of the window in the windowing system.

    By default, the window is not visible, you must call setVisible(true), or
    show() or similar to make it visible.

    \sa show()
*/
void QWindow::setVisible(bool visible)
{
    Q_D(QWindow);

    if (d->visible == visible)
        return;
    d->visible = visible;
    emit visibleChanged(visible);
    d->updateVisibility();

    if (!d->platformWindow)
        create();

    if (visible) {
        // remove posted quit events when showing a new window
        QCoreApplication::removePostedEvents(qApp, QEvent::Quit);

        if (type() == Qt::Window) {
            QString &firstWindowTitle = QGuiApplicationPrivate::instance()->firstWindowTitle;
            if (!firstWindowTitle.isEmpty()) {
                setTitle(firstWindowTitle);
                firstWindowTitle = QString();
            }
        }

        QShowEvent showEvent;
        QGuiApplication::sendEvent(this, &showEvent);
    }

    if (isModal()) {
        if (visible)
            QGuiApplicationPrivate::showModalWindow(this);
        else
            QGuiApplicationPrivate::hideModalWindow(this);
    }

#ifndef QT_NO_CURSOR
    if (visible && d->hasCursor)
        d->applyCursor();
#endif
    d->platformWindow->setVisible(visible);

    if (!visible) {
        QHideEvent hideEvent;
        QGuiApplication::sendEvent(this, &hideEvent);
    }
}

bool QWindow::isVisible() const
{
    Q_D(const QWindow);

    return d->visible;
}

/*!
    Allocates the platform resources associated with the window.

    It is at this point that the surface format set using setFormat() gets resolved
    into an actual native surface. However, the window remains hidden until setVisible() is called.

    Note that it is not usually necessary to call this function directly, as it will be implicitly
    called by show(), setVisible(), and other functions that require access to the platform
    resources.

    Call destroy() to free the platform resources if necessary.

    \sa destroy()
*/
void QWindow::create()
{
    Q_D(QWindow);
    if (!d->platformWindow) {
        d->platformWindow = QGuiApplicationPrivate::platformIntegration()->createPlatformWindow(this);
        QObjectList childObjects = children();
        for (int i = 0; i < childObjects.size(); i ++) {
            QObject *object = childObjects.at(i);
            if(object->isWindowType()) {
                QWindow *window = static_cast<QWindow *>(object);
                if (window->d_func()->platformWindow)
                    window->d_func()->platformWindow->setParent(d->platformWindow);
            }
        }
    }
}

/*!
    Returns the window's platform id.

    For platforms where this id might be useful, the value returned
    will uniquely represent the window inside the corresponding screen.

    \sa screen()
*/
WId QWindow::winId() const
{
    Q_D(const QWindow);

    if (type() == Qt::ForeignWindow)
        return WId(property("_q_foreignWinId").value<WId>());

    if(!d->platformWindow)
        const_cast<QWindow *>(this)->create();

    return d->platformWindow->winId();
}

/*!
    Returns the parent window, if any.

    A window without a parent is known as a top level window.
*/
QWindow *QWindow::parent() const
{
    Q_D(const QWindow);
    return d->parentWindow;
}

/*!
    Sets the \a parent Window. This will lead to the windowing system managing
    the clip of the window, so it will be clipped to the \a parent window.

    Setting \a parent to be 0 will make the window become a top level window.

    If \a parent is a window created by fromWinId(), then the current window
    will be embedded inside \a parent, if the platform supports it. Window
    embedding is currently supported only by the X11 platform plugin.
*/
void QWindow::setParent(QWindow *parent)
{
    Q_D(QWindow);

    QObject::setParent(parent);

    if (d->platformWindow) {
        if (parent && parent->d_func()->platformWindow) {
            d->platformWindow->setParent(parent->d_func()->platformWindow);
        } else {
            d->platformWindow->setParent(0);
        }
    }

    d->parentWindow = parent;

    QGuiApplicationPrivate::updateBlockedStatus(this);
}

/*!
    Returns whether the window is top level, i.e. has no parent window.
*/
bool QWindow::isTopLevel() const
{
    Q_D(const QWindow);
    return d->parentWindow == 0;
}

/*!
    Returns whether the window is modal.

    A modal window prevents other windows from getting any input.

    \sa QWindow::modality
*/
bool QWindow::isModal() const
{
    Q_D(const QWindow);
    return d->modality != Qt::NonModal;
}

/*! \property QWindow::modality
    \brief the modality of the window

    A modal window prevents other windows from receiving input events. Qt
    supports two types of modality: Qt::WindowModal and Qt::ApplicationModal.

    By default, this property is Qt::NonModal

    \sa Qt::WindowModality
*/

Qt::WindowModality QWindow::modality() const
{
    Q_D(const QWindow);
    return d->modality;
}

void QWindow::setModality(Qt::WindowModality modality)
{
    Q_D(QWindow);
    if (d->modality == modality)
        return;
    d->modality = modality;
    emit modalityChanged(modality);
}

/*! \fn void QWindow::modalityChanged(Qt::WindowModality modality)

    This signal is emitted when the Qwindow::modality property changes to \a modality.
*/

/*!
    Sets the window's surface \a format.

    The format determines properties such as color depth, alpha, depth and
    stencil buffer size, etc. For example, to give a window a transparent
    background (provided that the window system supports compositing, and
    provided that other content in the window does not make it opaque again):

    \code
    QSurfaceFormat format;
    format.setAlphaBufferSize(8);
    window.setFormat(format);
    \endcode

    The surface format will be resolved in the create() function. Calling
    this function after create() has been called will not re-resolve the
    surface format of the native surface.

    \sa create(), destroy()
*/
void QWindow::setFormat(const QSurfaceFormat &format)
{
    Q_D(QWindow);
    d->requestedFormat = format;
}

/*!
    Returns the requested surface format of this window.

    If the requested format was not supported by the platform implementation,
    the requestedFormat will differ from the actual window format.

    This is the value set with setFormat().

    \sa setFormat(), format()
 */
QSurfaceFormat QWindow::requestedFormat() const
{
    Q_D(const QWindow);
    return d->requestedFormat;
}

/*!
    Returns the actual format of this window.

    After the window has been created, this function will return the actual surface format
    of the window. It might differ from the requested format if the requested format could
    not be fulfilled by the platform. It might also be a superset, for example certain
    buffer sizes may be larger than requested.

    \note Depending on the platform, certain values in this surface format may still
    contain the requested values, that is, the values that have been passed to
    setFormat(). Typical examples are the OpenGL version, profile and options. These may
    not get updated during create() since these are context specific and a single window
    may be used together with multiple contexts over its lifetime. Use the
    QOpenGLContext's format() instead to query such values.

    \sa create(), requestedFormat(), QOpenGLContext::format()
*/
QSurfaceFormat QWindow::format() const
{
    Q_D(const QWindow);
    if (d->platformWindow)
        return d->platformWindow->format();
    return d->requestedFormat;
}

/*!
    \property QWindow::flags
    \brief the window flags of the window

    The window flags control the window's appearance in the windowing system,
    whether it's a dialog, popup, or a regular window, and whether it should
    have a title bar, etc.

    The actual window flags might differ from the flags set with setFlags()
    if the requested flags could not be fulfilled.
*/
void QWindow::setFlags(Qt::WindowFlags flags)
{
    Q_D(QWindow);
    if (d->platformWindow)
        d->platformWindow->setWindowFlags(flags);
    d->windowFlags = flags;
}

Qt::WindowFlags QWindow::flags() const
{
    Q_D(const QWindow);
    return d->windowFlags;
}

/*!
    Returns the type of the window.

    This returns the part of the window flags that represents
    whether the window is a dialog, tooltip, popup, regular window, etc.

    \sa flags(), setFlags()
*/
Qt::WindowType QWindow::type() const
{
    Q_D(const QWindow);
    return static_cast<Qt::WindowType>(int(d->windowFlags & Qt::WindowType_Mask));
}

/*!
    \property QWindow::title
    \brief the window's title in the windowing system

    The window title might appear in the title area of the window decorations,
    depending on the windowing system and the window flags. It might also
    be used by the windowing system to identify the window in other contexts,
    such as in the task switcher.

    \sa flags()
*/
void QWindow::setTitle(const QString &title)
{
    Q_D(QWindow);
    bool changed = false;
    if (d->windowTitle != title) {
        d->windowTitle = title;
        changed = true;
    }
    if (d->platformWindow)
        d->platformWindow->setWindowTitle(title);
    if (changed)
        emit windowTitleChanged(title);
}

QString QWindow::title() const
{
    Q_D(const QWindow);
    return d->windowTitle;
}

/*!
    \brief set the file name this window is representing.

    The windowing system might use \a filePath to display the
    path of the document this window is representing in the tile bar.

*/
void QWindow::setFilePath(const QString &filePath)
{
    Q_D(QWindow);
    d->windowFilePath = filePath;
    if (d->platformWindow)
        d->platformWindow->setWindowFilePath(filePath);
}

/*!
    \brief the file name this window is representing.

    \sa setFilePath()
*/
QString QWindow::filePath() const
{
    Q_D(const QWindow);
    return d->windowFilePath;
}

/*!
    \brief Sets the window's \a icon in the windowing system

    The window icon might be used by the windowing system for example to
    decorate the window, and/or in the task switcher.
*/
void QWindow::setIcon(const QIcon &icon)
{
    Q_D(QWindow);
    d->windowIcon = icon;
    if (d->platformWindow)
        d->platformWindow->setWindowIcon(icon);
    QEvent e(QEvent::WindowIconChange);
    QCoreApplication::sendEvent(this, &e);
}

/*!
    \brief Sets the window's icon in the windowing system

    \sa setIcon()
*/
QIcon QWindow::icon() const
{
    Q_D(const QWindow);
    if (d->windowIcon.isNull())
        return QGuiApplication::windowIcon();
    return d->windowIcon;
}

/*!
    Raise the window in the windowing system.

    Requests that the window be raised to appear above other windows.
*/
void QWindow::raise()
{
    Q_D(QWindow);
    if (d->platformWindow)
        d->platformWindow->raise();
}

/*!
    Lower the window in the windowing system.

    Requests that the window be lowered to appear below other windows.
*/
void QWindow::lower()
{
    Q_D(QWindow);
    if (d->platformWindow)
        d->platformWindow->lower();
}

/*!
    \property QWindow::opacity
    \brief The opacity of the window in the windowing system.
    \since 5.1

    If the windowing system supports window opacity, this can be used to fade the
    window in and out, or to make it semitransparent.

    A value of 1.0 or above is treated as fully opaque, whereas a value of 0.0 or below
    is treated as fully transparent. Values inbetween represent varying levels of
    translucency between the two extremes.

    The default value is 1.0.
*/
void QWindow::setOpacity(qreal level)
{
    Q_D(QWindow);
    if (level == d->opacity)
        return;
    d->opacity = level;
    if (d->platformWindow) {
        d->platformWindow->setOpacity(level);
        emit opacityChanged(level);
    }
}

qreal QWindow::opacity() const
{
    Q_D(const QWindow);
    return d->opacity;
}

/*!
    Sets the mask of the window.

    The mask is a hint to the windowing system that the application does not
    want to receive mouse or touch input outside the given \a region.

    The window manager may or may not choose to display any areas of the window
    not included in the mask, thus it is the application's responsibility to
    clear to transparent the areas that are not part of the mask.

    Setting the mask before the window has been created has no effect.
*/
void QWindow::setMask(const QRegion &region)
{
    Q_D(QWindow);
    if (!d->platformWindow)
        return;
    d->platformWindow->setMask(region);
    d->mask = region;
}

/*!
    Returns the mask set on the window.

    The mask is a hint to the windowing system that the application does not
    want to receive mouse or touch input outside the given region.
*/
QRegion QWindow::mask() const
{
    Q_D(const QWindow);
    return d->mask;
}

/*!
    Requests the window to be activated, i.e. receive keyboard focus.

    \sa isActive(), QGuiApplication::focusWindow()
*/
void QWindow::requestActivate()
{
    Q_D(QWindow);
    if (flags() & Qt::WindowDoesNotAcceptFocus) {
        qWarning() << "requestActivate() called for " << this << " which has Qt::WindowDoesNotAcceptFocus set.";
        return;
    }
    if (d->platformWindow)
        d->platformWindow->requestActivateWindow();
}

/*!
    Returns if this window is exposed in the windowing system.

    When the window is not exposed, it is shown by the application
    but it is still not showing in the windowing system, so the application
    should minimize rendering and other graphical activities.

    An exposeEvent() is sent every time this value changes.

    \sa exposeEvent()
*/
bool QWindow::isExposed() const
{
    Q_D(const QWindow);
    return d->exposed;
}

/*!
    \property QWindow::active
    \brief the active status of the window
    \since 5.1

    \sa requestActivate()
*/

/*!
    Returns \c true if the window should appear active from a style perspective.

    This is the case for the window that has input focus as well as windows
    that are in the same parent / transient parent chain as the focus window.

    To get the window that currently has focus, use QGuiApplication::focusWindow().
*/
bool QWindow::isActive() const
{
    Q_D(const QWindow);
    if (!d->platformWindow)
        return false;

    QWindow *focus = QGuiApplication::focusWindow();

    // Means the whole application lost the focus
    if (!focus)
        return false;

    if (focus == this)
        return true;

    if (!parent() && !transientParent()) {
        return isAncestorOf(focus);
    } else {
        return (parent() && parent()->isActive()) || (transientParent() && transientParent()->isActive());
    }
}

/*!
    \property QWindow::contentOrientation
    \brief the orientation of the window's contents

    This is a hint to the window manager in case it needs to display
    additional content like popups, dialogs, status bars, or similar
    in relation to the window.

    The recommended orientation is QScreen::orientation() but
    an application doesn't have to support all possible orientations,
    and thus can opt to ignore the current screen orientation.

    The difference between the window and the content orientation
    determines how much to rotate the content by. QScreen::angleBetween(),
    QScreen::transformBetween(), and QScreen::mapBetween() can be used
    to compute the necessary transform.

    The default value is Qt::PrimaryOrientation
*/
void QWindow::reportContentOrientationChange(Qt::ScreenOrientation orientation)
{
    Q_D(QWindow);
    if (d->contentOrientation == orientation)
        return;
    if (d->platformWindow)
        d->platformWindow->handleContentOrientationChange(orientation);
    d->contentOrientation = orientation;
    emit contentOrientationChanged(orientation);
}

Qt::ScreenOrientation QWindow::contentOrientation() const
{
    Q_D(const QWindow);
    return d->contentOrientation;
}

/*!
    Returns the ratio between physical pixels and device-independent pixels
    for the window. This value is dependent on the screen the window is on,
    and may change when the window is moved.

    Common values are 1.0 on normal displays and 2.0 on Apple "retina" displays.

    \sa QScreen::devicePixelRatio(), QGuiApplication::devicePixelRatio()
*/
qreal QWindow::devicePixelRatio() const
{
    Q_D(const QWindow);

    // If there is no platform window, do the second best thing and
    // return the app global devicePixelRatio. This is the highest
    // devicePixelRatio found on the system screens, and will be
    // correct for single-display systems (a very common case).
    if (!d->platformWindow)
        return qApp->devicePixelRatio();
    return d->platformWindow->devicePixelRatio();
}

/*!
    \brief set the screen-occupation state of the window

    The window \a state represents whether the window appears in the
    windowing system as maximized, minimized, fullscreen, or normal.

    The enum value Qt::WindowActive is not an accepted parameter.

    \sa showNormal(), showFullScreen(), showMinimized(), showMaximized()
*/
void QWindow::setWindowState(Qt::WindowState state)
{
    if (state == Qt::WindowActive) {
        qWarning() << "QWindow::setWindowState does not accept Qt::WindowActive";
        return;
    }

    Q_D(QWindow);
    if (d->platformWindow)
        d->platformWindow->setWindowState(state);
    d->windowState = state;
    emit windowStateChanged(d->windowState);
    d->updateVisibility();
}

/*!
    \brief the screen-occupation state of the window

    \sa setWindowState()
*/
Qt::WindowState QWindow::windowState() const
{
    Q_D(const QWindow);
    return d->windowState;
}

/*!
    \fn QWindow::windowStateChanged(Qt::WindowState windowState)

    This signal is emitted when the \a windowState changes, either
    by being set explicitly with setWindowState(), or automatically when
    the user clicks one of the titlebar buttons or by other means.
*/

/*!
    Sets the transient \a parent

    This is a hint to the window manager that this window is a dialog or pop-up
    on behalf of the given window.

    \sa transientParent(), parent()
*/
void QWindow::setTransientParent(QWindow *parent)
{
    Q_D(QWindow);
    d->transientParent = parent;

    QGuiApplicationPrivate::updateBlockedStatus(this);
}

/*!
    Returns the transient parent of the window.

    \sa setTransientParent(), parent()
*/
QWindow *QWindow::transientParent() const
{
    Q_D(const QWindow);
    return d->transientParent.data();
}

/*!
    \enum QWindow::AncestorMode

    This enum is used to control whether or not transient parents
    should be considered ancestors.

    \value ExcludeTransients Transient parents are not considered ancestors.
    \value IncludeTransients Transient parents are considered ancestors.
*/

/*!
    Returns \c true if the window is an ancestor of the given \a child. If \a mode
    is IncludeTransients, then transient parents are also considered ancestors.
*/
bool QWindow::isAncestorOf(const QWindow *child, AncestorMode mode) const
{
    if (child->parent() == this || (mode == IncludeTransients && child->transientParent() == this))
        return true;

    return (child->parent() && isAncestorOf(child->parent(), mode))
        || (mode == IncludeTransients && child->transientParent() && isAncestorOf(child->transientParent(), mode));
}

/*!
    Returns the minimum size of the window.

    \sa setMinimumSize()
*/
QSize QWindow::minimumSize() const
{
    Q_D(const QWindow);
    return d->minimumSize;
}

/*!
    Returns the maximum size of the window.

    \sa setMaximumSize()
*/
QSize QWindow::maximumSize() const
{
    Q_D(const QWindow);
    return d->maximumSize;
}

/*!
    Returns the base size of the window.

    \sa setBaseSize()
*/
QSize QWindow::baseSize() const
{
    Q_D(const QWindow);
    return d->baseSize;
}

/*!
    Returns the size increment of the window.

    \sa setSizeIncrement()
*/
QSize QWindow::sizeIncrement() const
{
    Q_D(const QWindow);
    return d->sizeIncrement;
}

/*!
    Sets the minimum size of the window.

    This is a hint to the window manager to prevent resizing below the specified \a size.

    \sa setMaximumSize(), minimumSize()
*/
void QWindow::setMinimumSize(const QSize &size)
{
    Q_D(QWindow);
    QSize adjustedSize = QSize(qBound(0, size.width(), QWINDOWSIZE_MAX), qBound(0, size.height(), QWINDOWSIZE_MAX));
    if (d->minimumSize == adjustedSize)
        return;
    QSize oldSize = d->minimumSize;
    d->minimumSize = adjustedSize;
    if (d->platformWindow && isTopLevel())
        d->platformWindow->propagateSizeHints();
    if (d->minimumSize.width() != oldSize.width())
        emit minimumWidthChanged(d->minimumSize.width());
    if (d->minimumSize.height() != oldSize.height())
        emit minimumHeightChanged(d->minimumSize.height());
}

/*!
    \property QWindow::x
    \brief the x position of the window's geometry
*/
void QWindow::setX(int arg)
{
    if (x() != arg)
        setGeometry(QRect(arg, y(), width(), height()));
}

/*!
    \property QWindow::y
    \brief the y position of the window's geometry
*/
void QWindow::setY(int arg)
{
    if (y() != arg)
        setGeometry(QRect(x(), arg, width(), height()));
}

/*!
    \property QWindow::width
    \brief the width of the window's geometry
*/
void QWindow::setWidth(int arg)
{
    if (width() != arg)
        resize(arg, height());
}

/*!
    \property QWindow::height
    \brief the height of the window's geometry
*/
void QWindow::setHeight(int arg)
{
    if (height() != arg)
        resize(width(), arg);
}

/*!
    \property QWindow::minimumWidth
    \brief the minimum width of the window's geometry
*/
void QWindow::setMinimumWidth(int w)
{
    setMinimumSize(QSize(w, minimumHeight()));
}

/*!
    \property QWindow::minimumHeight
    \brief the minimum height of the window's geometry
*/
void QWindow::setMinimumHeight(int h)
{
    setMinimumSize(QSize(minimumWidth(), h));
}

/*!
    Sets the maximum size of the window.

    This is a hint to the window manager to prevent resizing above the specified \a size.

    \sa setMinimumSize(), maximumSize()
*/
void QWindow::setMaximumSize(const QSize &size)
{
    Q_D(QWindow);
    QSize adjustedSize = QSize(qBound(0, size.width(), QWINDOWSIZE_MAX), qBound(0, size.height(), QWINDOWSIZE_MAX));
    if (d->maximumSize == adjustedSize)
        return;
    QSize oldSize = d->maximumSize;
    d->maximumSize = adjustedSize;
    if (d->platformWindow && isTopLevel())
        d->platformWindow->propagateSizeHints();
    if (d->maximumSize.width() != oldSize.width())
        emit maximumWidthChanged(d->maximumSize.width());
    if (d->maximumSize.height() != oldSize.height())
        emit maximumHeightChanged(d->maximumSize.height());
}

/*!
    \property QWindow::maximumWidth
    \brief the maximum width of the window's geometry
*/
void QWindow::setMaximumWidth(int w)
{
    setMaximumSize(QSize(w, maximumHeight()));
}

/*!
    \property QWindow::maximumHeight
    \brief the maximum height of the window's geometry
*/
void QWindow::setMaximumHeight(int h)
{
    setMaximumSize(QSize(maximumWidth(), h));
}

/*!
    Sets the base \a size of the window.

    The base size is used to calculate a proper window size if the
    window defines sizeIncrement().

    \sa setMinimumSize(), setMaximumSize(), setSizeIncrement(), baseSize()
*/
void QWindow::setBaseSize(const QSize &size)
{
    Q_D(QWindow);
    if (d->baseSize == size)
        return;
    d->baseSize = size;
    if (d->platformWindow && isTopLevel())
        d->platformWindow->propagateSizeHints();
}

/*!
    Sets the size increment (\a size) of the window.

    When the user resizes the window, the size will move in steps of
    sizeIncrement().width() pixels horizontally and
    sizeIncrement().height() pixels vertically, with baseSize() as the
    basis.

    By default, this property contains a size with zero width and height.

    The windowing system might not support size increments.

    \sa setBaseSize(), setMinimumSize(), setMaximumSize()
*/
void QWindow::setSizeIncrement(const QSize &size)
{
    Q_D(QWindow);
    if (d->sizeIncrement == size)
        return;
    d->sizeIncrement = size;
    if (d->platformWindow && isTopLevel())
        d->platformWindow->propagateSizeHints();
}

/*!
    Sets the geometry of the window, excluding its window frame, to a
    rectangle constructed from \a posx, \a posy, \a w and \a h.

    \sa geometry()
*/
void QWindow::setGeometry(int posx, int posy, int w, int h)
{
    setGeometry(QRect(posx, posy, w, h));
}

/*!
    \brief Sets the geometry of the window, excluding its window frame, to \a rect.

    \sa geometry()
*/
void QWindow::setGeometry(const QRect &rect)
{
    Q_D(QWindow);
    d->positionAutomatic = false;
    if (rect == geometry())
        return;
    QRect oldRect = geometry();

    d->positionPolicy = QWindowPrivate::WindowFrameExclusive;
    if (d->platformWindow) {
        d->platformWindow->setGeometry(rect);
    } else {
        d->geometry = rect;

        if (rect.x() != oldRect.x())
            emit xChanged(rect.x());
        if (rect.y() != oldRect.y())
            emit yChanged(rect.y());
        if (rect.width() != oldRect.width())
            emit widthChanged(rect.width());
        if (rect.height() != oldRect.height())
            emit heightChanged(rect.height());
    }
}

/*!
    Returns the geometry of the window, excluding its window frame.

    \sa frameMargins(), frameGeometry()
*/
QRect QWindow::geometry() const
{
    Q_D(const QWindow);
    if (d->platformWindow)
        return d->platformWindow->geometry();
    return d->geometry;
}

/*!
    Returns the window frame margins surrounding the window.

    \sa geometry(), frameGeometry()
*/
QMargins QWindow::frameMargins() const
{
    Q_D(const QWindow);
    if (d->platformWindow)
        return d->platformWindow->frameMargins();
    return QMargins();
}

/*!
    Returns the geometry of the window, including its window frame.

    \sa geometry(), frameMargins()
*/
QRect QWindow::frameGeometry() const
{
    Q_D(const QWindow);
    if (d->platformWindow) {
        QMargins m = frameMargins();
        return d->platformWindow->geometry().adjusted(-m.left(), -m.top(), m.right(), m.bottom());
    }
    return d->geometry;
}

/*!
    Returns the top left position of the window, including its window frame.

    This returns the same value as frameGeometry().topLeft().

    \sa geometry(), frameGeometry()
*/
QPoint QWindow::framePosition() const
{
    Q_D(const QWindow);
    if (d->platformWindow) {
        QMargins margins = frameMargins();
        return d->platformWindow->geometry().topLeft() - QPoint(margins.left(), margins.top());
    }
    return d->geometry.topLeft();
}

/*!
    Sets the upper left position of the window (\a point) including its window frame.

    \sa setGeometry(), frameGeometry()
*/
void QWindow::setFramePosition(const QPoint &point)
{
    Q_D(QWindow);
    d->positionPolicy = QWindowPrivate::WindowFrameInclusive;
    if (d->platformWindow) {
        d->platformWindow->setGeometry(QRect(point, size()));
    } else {
        d->positionAutomatic = false;
        d->geometry.moveTopLeft(point);
    }
}

/*!
    \brief set the position of the window on the desktop to \a pt

    \sa position()
*/
void QWindow::setPosition(const QPoint &pt)
{
    setGeometry(QRect(pt, size()));
}

/*!
    \brief set the position of the window on the desktop to \a posx, \a posy

    \sa position()
*/
void QWindow::setPosition(int posx, int posy)
{
    setPosition(QPoint(posx, posy));
}

/*!
    \fn QPoint QWindow::position() const
    \brief Returns the position of the window on the desktop excluding any window frame

    \sa setPosition()
*/

/*!
    \fn QSize QWindow::size() const
    \brief Returns the size of the window excluding any window frame

    \sa resize()
*/

/*!
    set the size of the window, excluding any window frame, to a QSize
    constructed from width \a w and height \a h

    \sa size(), geometry()
*/
void QWindow::resize(int w, int h)
{
    resize(QSize(w, h));
}

/*!
    \brief set the size of the window, excluding any window frame, to \a newSize

    \sa size(), geometry()
*/
void QWindow::resize(const QSize &newSize)
{
    Q_D(QWindow);
    if (d->platformWindow) {
        d->platformWindow->setGeometry(QRect(position(), newSize));
    } else {
        const QSize oldSize = d->geometry.size();
        d->geometry.setSize(newSize);
        if (newSize.width() != oldSize.width())
            emit widthChanged(newSize.width());
        if (newSize.height() != oldSize.height())
            emit heightChanged(newSize.height());
    }
}

/*!
    Releases the native platform resources associated with this window.

    \sa create()
*/
void QWindow::destroy()
{
    Q_D(QWindow);
    QObjectList childrenWindows = children();
    for (int i = 0; i < childrenWindows.size(); i++) {
        QObject *object = childrenWindows.at(i);
        if (object->isWindowType()) {
            QWindow *w = static_cast<QWindow*>(object);
            QGuiApplicationPrivate::window_list.removeAll(w);
            w->destroy();
        }
    }
    setVisible(false);
    delete d->platformWindow;
    d->resizeEventPending = true;
    d->receivedExpose = false;
    d->exposed = false;
    d->platformWindow = 0;
}

/*!
    Returns the platform window corresponding to the window.

    \internal
*/
QPlatformWindow *QWindow::handle() const
{
    Q_D(const QWindow);
    return d->platformWindow;
}

/*!
    Returns the platform surface corresponding to the window.

    \internal
*/
QPlatformSurface *QWindow::surfaceHandle() const
{
    Q_D(const QWindow);
    return d->platformWindow;
}

/*!
    Sets whether keyboard grab should be enabled or not (\a grab).

    If the return value is true, the window receives all key events until
    setKeyboardGrabEnabled(false) is called; other windows get no key events at
    all. Mouse events are not affected. Use setMouseGrabEnabled() if you want
    to grab that.

    \sa setMouseGrabEnabled()
*/
bool QWindow::setKeyboardGrabEnabled(bool grab)
{
    Q_D(QWindow);
    if (grab && QGuiApplicationPrivate::noGrab)
        return false;
    if (d->platformWindow)
        return d->platformWindow->setKeyboardGrabEnabled(grab);
    return false;
}

/*!
    Sets whether mouse grab should be enabled or not (\a grab).

    If the return value is true, the window receives all mouse events until setMouseGrabEnabled(false) is
    called; other windows get no mouse events at all. Keyboard events are not affected.
    Use setKeyboardGrabEnabled() if you want to grab that.

    \sa setKeyboardGrabEnabled()
*/
bool QWindow::setMouseGrabEnabled(bool grab)
{
    Q_D(QWindow);
    if (grab && QGuiApplicationPrivate::noGrab)
        return false;
    if (d->platformWindow)
        return d->platformWindow->setMouseGrabEnabled(grab);
    return false;
}

/*!
    Returns the screen on which the window is shown.

    The value returned will not change when the window is moved
    between virtual screens (as returned by QScreen::virtualSiblings()).

    \sa setScreen(), QScreen::virtualSiblings()
*/
QScreen *QWindow::screen() const
{
    Q_D(const QWindow);
    return d->screen;
}

/*!
    Sets the screen on which the window should be shown.

    If the window has been created, it will be recreated on the \a newScreen.

    Note that if the screen is part of a virtual desktop of multiple screens,
    the window can appear on any of the screens returned by QScreen::virtualSiblings().

    \sa screen(), QScreen::virtualSiblings()
*/
void QWindow::setScreen(QScreen *newScreen)
{
    Q_D(QWindow);
    if (!newScreen)
        newScreen = QGuiApplication::primaryScreen();
    d->setScreen(newScreen, true /* recreate */);
}

void QWindow::screenDestroyed(QObject *object)
{
    Q_D(QWindow);
    if (object == static_cast<QObject *>(d->screen)) {
        const bool wasVisible = isVisible();
        setScreen(0);
        // destroy() might have hidden our window, show it again.
        // This might not be the best behavior if the new screen isn't a virtual sibling
        // of the old one. This can be removed once platform plugins have the power to
        // update the QScreen of its QWindows itself.
        if (wasVisible && d->platformWindow)
            setVisible(true);
    }
}

/*!
    \fn QWindow::screenChanged(QScreen *screen)

    This signal is emitted when a window's \a screen changes, either
    by being set explicitly with setScreen(), or automatically when
    the window's screen is removed.
*/

/*!
  Returns the accessibility interface for the object that the window represents
  \internal
  \sa QAccessible
  */
QAccessibleInterface *QWindow::accessibleRoot() const
{
    return 0;
}

/*!
    \fn QWindow::focusObjectChanged(QObject *object)

    This signal is emitted when the final receiver of events tied to focus
    is changed to \a object.

    \sa focusObject()
*/

/*!
    Returns the QObject that will be the final receiver of events tied focus, such
    as key events.
*/
QObject *QWindow::focusObject() const
{
    return const_cast<QWindow *>(this);
}

/*!
    Shows the window.

    This is equivalent to calling showFullScreen(), showMaximized(), or showNormal(),
    depending on the platform's default behavior for the window type and flags.

    \sa showFullScreen(), showMaximized(), showNormal(), hide(), QStyleHints::showIsFullScreen(), flags()
*/
void QWindow::show()
{
    Qt::WindowState defaultState = QGuiApplicationPrivate::platformIntegration()->defaultWindowState(d_func()->windowFlags);
    if (defaultState == Qt::WindowFullScreen)
        showFullScreen();
    else if (defaultState == Qt::WindowMaximized)
        showMaximized();
    else
        showNormal();
}

/*!
    Hides the window.

    Equivalent to calling setVisible(false).

    \sa show(), setVisible()
*/
void QWindow::hide()
{
    setVisible(false);
}

/*!
    Shows the window as minimized.

    Equivalent to calling setWindowState(Qt::WindowMinimized) and then
    setVisible(true).

    \sa setWindowState(), setVisible()
*/
void QWindow::showMinimized()
{
    setWindowState(Qt::WindowMinimized);
    setVisible(true);
}

/*!
    Shows the window as maximized.

    Equivalent to calling setWindowState(Qt::WindowMaximized) and then
    setVisible(true).

    \sa setWindowState(), setVisible()
*/
void QWindow::showMaximized()
{
    setWindowState(Qt::WindowMaximized);
    setVisible(true);
}

/*!
    Shows the window as fullscreen.

    Equivalent to calling setWindowState(Qt::WindowFullScreen) and then
    setVisible(true).

    \sa setWindowState(), setVisible()
*/
void QWindow::showFullScreen()
{
    setWindowState(Qt::WindowFullScreen);
    setVisible(true);
#if !defined Q_OS_QNX // On QNX this window will be activated anyway from libscreen
                      // activating it here before libscreen activates it causes problems
    requestActivate();
#endif
}

/*!
    Shows the window as normal, i.e. neither maximized, minimized, nor fullscreen.

    Equivalent to calling setWindowState(Qt::WindowNoState) and then
    setVisible(true).

    \sa setWindowState(), setVisible()
*/
void QWindow::showNormal()
{
    setWindowState(Qt::WindowNoState);
    setVisible(true);
}

/*!
    Close the window.

    This closes the window, effectively calling destroy(), and potentially
    quitting the application. Returns \c true on success, false if it has a parent
    window (in which case the top level window should be closed instead).

    \sa destroy(), QGuiApplication::quitOnLastWindowClosed()
*/
bool QWindow::close()
{
    Q_D(QWindow);

    // Do not close non top level windows
    if (parent())
        return false;

    if (QGuiApplicationPrivate::focus_window == this)
        QGuiApplicationPrivate::focus_window = 0;
    if (QGuiApplicationPrivate::currentMouseWindow == this)
        QGuiApplicationPrivate::currentMouseWindow = 0;

    QGuiApplicationPrivate::window_list.removeAll(this);
    destroy();
    d->maybeQuitOnLastWindowClosed();
    return true;
}

/*!
    The expose event (\a ev) is sent by the window system whenever the window's
    exposure on screen changes.

    The application can start rendering into the window with QBackingStore
    and QOpenGLContext as soon as it gets an exposeEvent() such that
    isExposed() is true.

    If the window is moved off screen, is made totally obscured by another
    window, iconified or similar, this function might be called and the
    value of isExposed() might change to false. When this happens,
    an application should stop its rendering as it is no longer visible
    to the user.

    A resize event will always be sent before the expose event the first time
    a window is shown.

    \sa isExposed()
*/
void QWindow::exposeEvent(QExposeEvent *ev)
{
    ev->ignore();
}

/*!
    Override this to handle window move events (\a ev).
*/
void QWindow::moveEvent(QMoveEvent *ev)
{
    ev->ignore();
}

/*!
    Override this to handle resize events (\a ev).

    The resize event is called whenever the window is resized in the windowing system,
    either directly through the windowing system acknowledging a setGeometry() or resize() request,
    or indirectly through the user resizing the window manually.
*/
void QWindow::resizeEvent(QResizeEvent *ev)
{
    ev->ignore();
}

/*!
    Override this to handle show events (\a ev).

    The function is called when the window has requested becoming visible.

    If the window is successfully shown by the windowing system, this will
    be followed by a resize and an expose event.
*/
void QWindow::showEvent(QShowEvent *ev)
{
    ev->ignore();
}

/*!
    Override this to handle hide events (\a ev).

    The function is called when the window has requested being hidden in the
    windowing system.
*/
void QWindow::hideEvent(QHideEvent *ev)
{
    ev->ignore();
}

/*!
    Override this to handle any event (\a ev) sent to the window.
    Return \c true if the event was recognized and processed.

    Remember to call the base class version if you wish for mouse events,
    key events, resize events, etc to be dispatched as usual.
*/
bool QWindow::event(QEvent *ev)
{
    switch (ev->type()) {
    case QEvent::MouseMove:
        mouseMoveEvent(static_cast<QMouseEvent*>(ev));
        break;

    case QEvent::MouseButtonPress:
        mousePressEvent(static_cast<QMouseEvent*>(ev));
        break;

    case QEvent::MouseButtonRelease:
        mouseReleaseEvent(static_cast<QMouseEvent*>(ev));
        break;

    case QEvent::MouseButtonDblClick:
        mouseDoubleClickEvent(static_cast<QMouseEvent*>(ev));
        break;

    case QEvent::TouchBegin:
    case QEvent::TouchUpdate:
    case QEvent::TouchEnd:
    case QEvent::TouchCancel:
        touchEvent(static_cast<QTouchEvent *>(ev));
        break;

    case QEvent::Move:
        moveEvent(static_cast<QMoveEvent*>(ev));
        break;

    case QEvent::Resize:
        resizeEvent(static_cast<QResizeEvent*>(ev));
        break;

    case QEvent::KeyPress:
        keyPressEvent(static_cast<QKeyEvent *>(ev));
        break;

    case QEvent::KeyRelease:
        keyReleaseEvent(static_cast<QKeyEvent *>(ev));
        break;

    case QEvent::FocusIn: {
        focusInEvent(static_cast<QFocusEvent *>(ev));
#ifndef QT_NO_ACCESSIBILITY
        QAccessible::State state;
        state.active = true;
        QAccessibleStateChangeEvent event(this, state);
        QAccessible::updateAccessibility(&event);
#endif
        break; }

    case QEvent::FocusOut: {
        focusOutEvent(static_cast<QFocusEvent *>(ev));
#ifndef QT_NO_ACCESSIBILITY
        QAccessible::State state;
        state.active = true;
        QAccessibleStateChangeEvent event(this, state);
        QAccessible::updateAccessibility(&event);
#endif
        break; }

#ifndef QT_NO_WHEELEVENT
    case QEvent::Wheel:
        wheelEvent(static_cast<QWheelEvent*>(ev));
        break;
#endif

    case QEvent::Close: {
        Q_D(QWindow);
        bool wasVisible = isVisible();
        if (ev->isAccepted()) {
            destroy();
            if (wasVisible)
                d->maybeQuitOnLastWindowClosed();
        }
        break; }

    case QEvent::Expose:
        exposeEvent(static_cast<QExposeEvent *>(ev));
        break;

    case QEvent::Show:
        showEvent(static_cast<QShowEvent *>(ev));
        break;

    case QEvent::Hide:
        hideEvent(static_cast<QHideEvent *>(ev));
        break;

    case QEvent::ApplicationWindowIconChange:
        setIcon(icon());
        break;

    case QEvent::WindowStateChange: {
        Q_D(QWindow);
        emit windowStateChanged(d->windowState);
        d->updateVisibility();
        break;
    }

#ifndef QT_NO_TABLETEVENT
    case QEvent::TabletPress:
    case QEvent::TabletMove:
    case QEvent::TabletRelease:
        tabletEvent(static_cast<QTabletEvent *>(ev));
        break;
#endif

    default:
        return QObject::event(ev);
    }
    return true;
}

/*!
    Override this to handle key press events (\a ev).

    \sa keyReleaseEvent()
*/
void QWindow::keyPressEvent(QKeyEvent *ev)
{
    ev->ignore();
}

/*!
    Override this to handle key release events (\a ev).

    \sa keyPressEvent()
*/
void QWindow::keyReleaseEvent(QKeyEvent *ev)
{
    ev->ignore();
}

/*!
    Override this to handle focus in events (\a ev).

    Focus in events are sent when the window receives keyboard focus.

    \sa focusOutEvent()
*/
void QWindow::focusInEvent(QFocusEvent *ev)
{
    ev->ignore();
}

/*!
    Override this to handle focus out events (\a ev).

    Focus out events are sent when the window loses keyboard focus.

    \sa focusInEvent()
*/
void QWindow::focusOutEvent(QFocusEvent *ev)
{
    ev->ignore();
}

/*!
    Override this to handle mouse press events (\a ev).

    \sa mouseReleaseEvent()
*/
void QWindow::mousePressEvent(QMouseEvent *ev)
{
    ev->ignore();
}

/*!
    Override this to handle mouse release events (\a ev).

    \sa mousePressEvent()
*/
void QWindow::mouseReleaseEvent(QMouseEvent *ev)
{
    ev->ignore();
}

/*!
    Override this to handle mouse double click events (\a ev).

    \sa mousePressEvent(), QStyleHints::mouseDoubleClickInterval()
*/
void QWindow::mouseDoubleClickEvent(QMouseEvent *ev)
{
    ev->ignore();
}

/*!
    Override this to handle mouse move events (\a ev).
*/
void QWindow::mouseMoveEvent(QMouseEvent *ev)
{
    ev->ignore();
}

#ifndef QT_NO_WHEELEVENT
/*!
    Override this to handle mouse wheel or other wheel events (\a ev).
*/
void QWindow::wheelEvent(QWheelEvent *ev)
{
    ev->ignore();
}
#endif //QT_NO_WHEELEVENT

/*!
    Override this to handle touch events (\a ev).
*/
void QWindow::touchEvent(QTouchEvent *ev)
{
    ev->ignore();
}

#ifndef QT_NO_TABLETEVENT
/*!
    Override this to handle tablet press, move, and release events (\a ev).

    Proximity enter and leave events are not sent to windows, they are
    delivered to the application instance.
*/
void QWindow::tabletEvent(QTabletEvent *ev)
{
    ev->ignore();
}
#endif

/*!
    Override this to handle platform dependent events.
    Will be given \a eventType, \a message and \a result.

    This might make your application non-portable.

    Should return true only if the event was handled.
*/
bool QWindow::nativeEvent(const QByteArray &eventType, void *message, long *result)
{
    Q_UNUSED(eventType);
    Q_UNUSED(message);
    Q_UNUSED(result);
    return false;
}

/*!
    \fn QPoint QWindow::mapToGlobal(const QPoint &pos) const

    Translates the window coordinate \a pos to global screen
    coordinates. For example, \c{mapToGlobal(QPoint(0,0))} would give
    the global coordinates of the top-left pixel of the window.

    \sa mapFromGlobal()
*/
QPoint QWindow::mapToGlobal(const QPoint &pos) const
{
    Q_D(const QWindow);
    if (d->platformWindow && d->platformWindow->isEmbedded(0))
        return d->platformWindow->mapToGlobal(pos);
    else
        return pos + d_func()->globalPosition();
}


/*!
    \fn QPoint QWindow::mapFromGlobal(const QPoint &pos) const

    Translates the global screen coordinate \a pos to window
    coordinates.

    \sa mapToGlobal()
*/
QPoint QWindow::mapFromGlobal(const QPoint &pos) const
{
    Q_D(const QWindow);
    if (d->platformWindow && d->platformWindow->isEmbedded(0))
        return d->platformWindow->mapFromGlobal(pos);
    else
        return pos - d_func()->globalPosition();
}


Q_GUI_EXPORT QWindowPrivate *qt_window_private(QWindow *window)
{
    return window->d_func();
}

void QWindowPrivate::maybeQuitOnLastWindowClosed()
{
    Q_Q(QWindow);

    // Attempt to close the application only if this has WA_QuitOnClose set and a non-visible parent
    bool quitOnClose = QGuiApplication::quitOnLastWindowClosed() && !q->parent();

    if (quitOnClose) {
        QWindowList list = QGuiApplication::topLevelWindows();
        bool lastWindowClosed = true;
        for (int i = 0; i < list.size(); ++i) {
            QWindow *w = list.at(i);
            if (!w->isVisible() || w->transientParent())
                continue;
            lastWindowClosed = false;
            break;
        }
        if (lastWindowClosed) {
            QGuiApplicationPrivate::emitLastWindowClosed();
            QCoreApplicationPrivate *applicationPrivate = static_cast<QCoreApplicationPrivate*>(QObjectPrivate::get(QCoreApplication::instance()));
            applicationPrivate->maybeQuit();
        }
    }

}

QWindow *QWindowPrivate::topLevelWindow() const
{
    Q_Q(const QWindow);

    QWindow *window = const_cast<QWindow *>(q);

    while (window) {
        QWindow *parent = window->parent();
        if (!parent)
            parent = window->transientParent();

        if (!parent)
            break;

        window = parent;
    }

    return window;
}

/*!
    Creates a local representation of a window created by another process or by
    using native libraries below Qt.

    Given the handle \a id to a native window, this method creates a QWindow
    object which can be used to represent the window when invoking methods like
    setParent() and setTransientParent().
    This can be used, on platforms which support it, to embed a window inside a
    container or to make a window stick on top of a window created by another
    process.

    \sa setParent()
    \sa setTransientParent()
*/
QWindow *QWindow::fromWinId(WId id)
{
    if (!QGuiApplicationPrivate::platformIntegration()->hasCapability(QPlatformIntegration::ForeignWindows)) {
        qWarning() << "QWindow::fromWinId(): platform plugin does not support foreign windows.";
        return 0;
    }

    QWindow *window = new QWindow;
    window->setFlags(Qt::ForeignWindow);
    window->setProperty("_q_foreignWinId", QVariant::fromValue(id));
    window->create();
    return window;
}

/*!
    Causes an alert to be shown for \a msec miliseconds. If \a msec is \c 0 (the
    default), then the alert is shown indefinitely until the window becomes
    active again.

    In alert state, the window indicates that it demands attention, for example by
    flashing or bouncing the taskbar entry.

    \since 5.1
*/

void QWindow::alert(int msec)
{
    Q_D(QWindow);
    if (!d->platformWindow || d->platformWindow->isAlertState())
        return;
    d->platformWindow->setAlertState(true);
    if (d->platformWindow->isAlertState() && msec)
        QTimer::singleShot(msec, this, SLOT(_q_clearAlert()));
}

void QWindowPrivate::_q_clearAlert()
{
    if (platformWindow && platformWindow->isAlertState())
        platformWindow->setAlertState(false);
}

#ifndef QT_NO_CURSOR
/*!
    \brief set the cursor shape for this window

    The mouse \a cursor will assume this shape when it is over this
    window, unless an override cursor is set.
    See the \l{Qt::CursorShape}{list of predefined cursor objects} for a
    range of useful shapes.

    By default, the cursor has the Qt::ArrowCursor shape.

    Some underlying window implementations will reset the cursor if it
    leaves a window even if the mouse is grabbed. If you want to have
    a cursor set for all windows, even when outside the window, consider
    QGuiApplication::setOverrideCursor().

    \sa QGuiApplication::setOverrideCursor()
*/
void QWindow::setCursor(const QCursor &cursor)
{
    Q_D(QWindow);
    d->setCursor(&cursor);
}

/*!
  \brief Restores the default arrow cursor for this window.
 */
void QWindow::unsetCursor()
{
    Q_D(QWindow);
    d->setCursor(0);
}

/*!
    \brief the cursor shape for this window

    \sa setCursor(), unsetCursor()
*/
QCursor QWindow::cursor() const
{
    Q_D(const QWindow);
    return d->cursor;
}

void QWindowPrivate::setCursor(const QCursor *newCursor)
{

    Q_Q(QWindow);
    if (newCursor) {
        const Qt::CursorShape newShape = newCursor->shape();
        if (newShape <= Qt::LastCursor && hasCursor && newShape == cursor.shape())
            return; // Unchanged and no bitmap/custom cursor.
        cursor = *newCursor;
        hasCursor = true;
    } else {
        if (!hasCursor)
            return;
        cursor = QCursor(Qt::ArrowCursor);
        hasCursor = false;
    }
    // Only attempt to set cursor and emit signal if there is an actual platform cursor
    if (screen->handle()->cursor()) {
        applyCursor();
        QEvent event(QEvent::CursorChange);
        QGuiApplication::sendEvent(q, &event);
    }
}

void QWindowPrivate::applyCursor()
{
    Q_Q(QWindow);
    if (platformWindow) {
        if (QPlatformCursor *platformCursor = screen->handle()->cursor()) {
            QCursor *c = QGuiApplication::overrideCursor();
            if (!c && hasCursor)
                c = &cursor;
            platformCursor->changeCursor(c, q);
        }
    }
}
#endif // QT_NO_CURSOR

QT_END_NAMESPACE

#include "moc_qwindow.cpp"
