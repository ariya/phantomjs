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

#include "qplatformwindow.h"
#include "qplatformwindow_p.h"
#include "qplatformscreen.h"

#include <private/qguiapplication_p.h>
#include <qpa/qwindowsysteminterface.h>
#include <QtGui/qwindow.h>
#include <QtGui/qscreen.h>
#include <private/qwindow_p.h>

QT_BEGIN_NAMESPACE

/*!
    Constructs a platform window with the given top level window.
*/

QPlatformWindow::QPlatformWindow(QWindow *window)
    : QPlatformSurface(window)
    , d_ptr(new QPlatformWindowPrivate)
{
    Q_D(QPlatformWindow);
    d->rect = window->geometry();
}

/*!
    Virtual destructor does not delete its top level window.
*/
QPlatformWindow::~QPlatformWindow()
{
}

/*!
    Returns the window which belongs to the QPlatformWindow
*/
QWindow *QPlatformWindow::window() const
{
    return static_cast<QWindow *>(m_surface);
}

/*!
    Returns the parent platform window (or 0 if orphan).
*/
QPlatformWindow *QPlatformWindow::parent() const
{
    return window()->parent() ? window()->parent()->handle() : 0;
}

/*!
    Returns the platform screen handle corresponding to this platform window.
*/
QPlatformScreen *QPlatformWindow::screen() const
{
    return window()->screen()->handle();
}

/*!
    Returns the actual surface format of the window.
*/
QSurfaceFormat QPlatformWindow::format() const
{
    return QSurfaceFormat();
}

/*!
    This function is called by Qt whenever a window is moved or the window is resized. The resize
    can happen programatically(from ie. user application) or by the window manager. This means that
    there is no need to call this function specifically from the window manager callback, instead
    call QWindowSystemInterface::handleGeometryChange(QWindow *w, const QRect &newRect);

    The position(x, y) part of the rect might be inclusive or exclusive of the window frame
    as returned by frameMargins(). You can detect this in the plugin by checking
    qt_window_private(window())->positionPolicy.
*/
void QPlatformWindow::setGeometry(const QRect &rect)
{
    Q_D(QPlatformWindow);
    d->rect = rect;
}

/*!
    Returnes the current geometry of a window
*/
QRect QPlatformWindow::geometry() const
{
    Q_D(const QPlatformWindow);
    return d->rect;
}

/*!
    Returns the geometry of a window in 'normal' state
    (neither maximized, fullscreen nor minimized) for saving geometries to
    application settings.

    \since 5.3
*/
QRect QPlatformWindow::normalGeometry() const
{
    return QRect();
}

QMargins QPlatformWindow::frameMargins() const
{
    return QMargins();
}

/*!
    Reimplemented in subclasses to show the surface
    if \a visible is \c true, and hide it if \a visible is \c false.

    The default implementation sends a synchronous expose event.
*/
void QPlatformWindow::setVisible(bool visible)
{
    Q_UNUSED(visible);
    QRect rect(QPoint(), geometry().size());
    QWindowSystemInterface::handleExposeEvent(window(), rect);
    QWindowSystemInterface::flushWindowSystemEvents();
}

/*!
    Requests setting the window flags of this surface
    to \a flags.
*/
void QPlatformWindow::setWindowFlags(Qt::WindowFlags flags)
{
    Q_UNUSED(flags);
}

/*!
    Returns if this window is exposed in the windowing system.

    An exposeEvent() is sent every time this value changes.
 */

bool QPlatformWindow::isExposed() const
{
    return window()->isVisible();
}

/*!
    Returns \c true if the window should appear active from a style perspective.

    This function can make platform-specific isActive checks, such as checking
    if the QWindow is embedded in an active native window.
*/
bool QPlatformWindow::isActive() const
{
    return false;
}

/*!
    Returns \c true if the window is a descendant of an embedded non-Qt window.
    Example of an embedded non-Qt window is the parent window of an in-process QAxServer.

    If \a parentWindow is nonzero, only check if the window is embedded in the
    specified \a parentWindow.
*/
bool QPlatformWindow::isEmbedded(const QPlatformWindow *parentWindow) const
{
    Q_UNUSED(parentWindow);
    return false;
}

/*!
    Translates the window coordinate \a pos to global screen
    coordinates using native methods. This is required for embedded windows,
    where the topmost QWindow coordinates are not global screen coordinates.

    Returns \a pos if there is no platform specific implementation.
*/
QPoint QPlatformWindow::mapToGlobal(const QPoint &pos) const
{
    const QPlatformWindow *p = this;
    QPoint result = pos;
    while (p) {
        result += p->geometry().topLeft();
        p = p->parent();
    }
    return result;
}

/*!
    Translates the global screen coordinate \a pos to window
    coordinates using native methods. This is required for embedded windows,
    where the topmost QWindow coordinates are not global screen coordinates.

    Returns \a pos if there is no platform specific implementation.
*/
QPoint QPlatformWindow::mapFromGlobal(const QPoint &pos) const
{
    const QPlatformWindow *p = this;
    QPoint result = pos;
    while (p) {
        result -= p->geometry().topLeft();
        p = p->parent();
    }
    return result;
}

/*!
    Requests setting the window state of this surface
    to \a type.

    Qt::WindowActive can be ignored.
*/
void QPlatformWindow::setWindowState(Qt::WindowState)
{
}

/*!
  Reimplement in subclasses to return a handle to the native window
*/
WId QPlatformWindow::winId() const
{
    // Return anything but 0. Returning 0 would cause havoc with QWidgets on
    // very basic platform plugins that do not reimplement this function,
    // because the top-level widget's internalWinId() would always be 0 which
    // would mean top-levels are never treated as native.
    return WId(1);
}

/*!
    This function is called to enable native child window in QPA. It is common not to support this
    feature in Window systems, but can be faked. When this function is called all geometry of this
    platform window will be relative to the parent.
*/
//jl: It would be useful to have a property on the platform window which indicated if the sub-class
// supported the setParent. If not, then geometry would be in screen coordinates.
void QPlatformWindow::setParent(const QPlatformWindow *parent)
{
    Q_UNUSED(parent);
    qWarning("This plugin does not support setParent!");
}

/*!
  Reimplement to set the window title to \a title.

  The implementation might want to append the application display name to
  the window title, like Windows and Linux do.

  \sa QGuiApplication::applicationDisplayName()
*/
void QPlatformWindow::setWindowTitle(const QString &title) { Q_UNUSED(title); }

/*!
  Reimplement to set the window file path to \a filePath
*/
void QPlatformWindow::setWindowFilePath(const QString &filePath) { Q_UNUSED(filePath); }

/*!
  Reimplement to set the window icon to \a icon
*/
void QPlatformWindow::setWindowIcon(const QIcon &icon) { Q_UNUSED(icon); }

/*!
  Reimplement to be able to let Qt raise windows to the top of the desktop
*/
void QPlatformWindow::raise() { qWarning("This plugin does not support raise()"); }

/*!
  Reimplement to be able to let Qt lower windows to the bottom of the desktop
*/
void QPlatformWindow::lower() { qWarning("This plugin does not support lower()"); }

/*!
  Reimplement to propagate the size hints of the QWindow.

  The size hints include QWindow::minimumSize(), QWindow::maximumSize(),
  QWindow::sizeIncrement(), and QWindow::baseSize().
*/
void QPlatformWindow::propagateSizeHints() {qWarning("This plugin does not support propagateSizeHints()"); }

/*!
  Reimplement to be able to let Qt set the opacity level of a window
*/
void QPlatformWindow::setOpacity(qreal level)
{
    Q_UNUSED(level);
    qWarning("This plugin does not support setting window opacity");
}

/*!
  Reimplement to  be able to let Qt set the mask of a window
*/

void QPlatformWindow::setMask(const QRegion &region)
{
    Q_UNUSED(region);
    qWarning("This plugin does not support setting window masks");
}

/*!
  Reimplement to let Qt be able to request activation/focus for a window

  Some window systems will probably not have callbacks for this functionality,
  and then calling QWindowSystemInterface::handleWindowActivated(QWindow *w)
  would be sufficient.

  If the window system has some event handling/callbacks then call
  QWindowSystemInterface::handleWindowActivated(QWindow *w) when the window system
  gives the notification.

  Default implementation calls QWindowSystem::handleWindowActivated(QWindow *w)
*/
void QPlatformWindow::requestActivateWindow()
{
    QWindowSystemInterface::handleWindowActivated(window());
}

/*!
  Handle changes to the orientation of the platform window's contents.

  This is a hint to the window manager in case it needs to display
  additional content like popups, dialogs, status bars, or similar
  in relation to the window.

  \sa QWindow::reportContentOrientationChange()
*/
void QPlatformWindow::handleContentOrientationChange(Qt::ScreenOrientation orientation)
{
    Q_UNUSED(orientation);
}

/*!
    Reimplement this function in subclass to return the device pixel ratio
    for the window. This is the ratio between physical pixels
    and device-independent pixels.

    \sa QPlatformWindow::devicePixelRatio();
*/
qreal QPlatformWindow::devicePixelRatio() const
{
    return 1.0;
}

bool QPlatformWindow::setKeyboardGrabEnabled(bool grab)
{
    Q_UNUSED(grab);
    qWarning("This plugin does not support grabbing the keyboard");
    return false;
}

bool QPlatformWindow::setMouseGrabEnabled(bool grab)
{
    Q_UNUSED(grab);
    qWarning("This plugin does not support grabbing the mouse");
    return false;
}

/*!
    Reimplement to be able to let Qt indicate that the window has been
    modified. Return true if the native window supports setting the modified
    flag, false otherwise.
*/
bool QPlatformWindow::setWindowModified(bool modified)
{
    Q_UNUSED(modified);
    return false;
}

/*!
    Reimplement this method to be able to do any platform specific event
    handling. All events for window() are passed to this function before being
    sent to QWindow::event().

    The default implementation is empty and does nothing with \a event.
*/
void QPlatformWindow::windowEvent(QEvent *event)
{
    Q_UNUSED(event);
}

/*!
    Reimplement this method to start a system size grip drag
    operation if the system supports it and return true to indicate
    success.
    It is called from the mouse press event handler of the size grip.

    The default implementation is empty and does nothing with \a pos
    and \a corner.
*/

bool QPlatformWindow::startSystemResize(const QPoint &pos, Qt::Corner corner)
{
    Q_UNUSED(pos)
    Q_UNUSED(corner)
    return false;
}

/*!
    Reimplement this method to set whether frame strut events
    should be sent to \a enabled.

    \sa frameStrutEventsEnabled
*/

void QPlatformWindow::setFrameStrutEventsEnabled(bool enabled)
{
    Q_UNUSED(enabled) // Do not warn as widgets enable it by default causing warnings with XCB.
}

/*!
    Reimplement this method to return whether
    frame strut events are enabled.
*/

bool QPlatformWindow::frameStrutEventsEnabled() const
{
    return false;
}

/*!
    Call this method to put together a window title composed of
    \a title
    \a separator
    the application display name

    If the display name isn't set, and the title is empty, the raw app name is used.
*/
QString QPlatformWindow::formatWindowTitle(const QString &title, const QString &separator)
{
    QString fullTitle = title;
    if (QGuiApplicationPrivate::displayName) {
        // Append display name, if set.
        if (!fullTitle.isEmpty())
            fullTitle += separator;
        fullTitle += *QGuiApplicationPrivate::displayName;
    } else if (fullTitle.isEmpty()) {
        // Don't let the window title be completely empty, use the app name as fallback.
        fullTitle = QCoreApplication::applicationName();
    }
    return fullTitle;
}

/*!
    Reimplement this method to set whether the window demands attention
    (for example, by flashing the taskbar icon) depending on \a enabled.

    \sa isAlertState()
    \since 5.1
*/

void QPlatformWindow::setAlertState(bool enable)
{
    Q_UNUSED(enable)
}

/*!
    Reimplement this method return whether the window is in
    an alert state.

    \sa setAlertState()
    \since 5.1
*/

bool QPlatformWindow::isAlertState() const
{
    return false;
}

// Return the effective screen for the initial geometry of a window. In a
// multimonitor-setup, try to find the right screen by checking the transient
// parent or the mouse cursor for parentless windows (cf QTBUG-34204,
// QDialog::adjustPosition()).
static inline const QScreen *effectiveScreen(const QWindow *window)
{
    if (!window)
        return QGuiApplication::primaryScreen();
    const QScreen *screen = window->screen();
    if (!screen)
        return QGuiApplication::primaryScreen();
    const QList<QScreen *> siblings = screen->virtualSiblings();
    if (siblings.size() > 1) {
        const QPoint referencePoint = window->transientParent() ? window->transientParent()->geometry().center() : QCursor::pos();
        foreach (const QScreen *sibling, siblings)
            if (sibling->geometry().contains(referencePoint))
                return sibling;
    }
    return screen;
}

/*!
    Invalidates the window's surface by releasing its surface buffers.

    Many platforms do not support releasing the surface memory,
    and the default implementation does nothing.

    The platform window is expected to recreate the surface again if
    it is needed. For instance, if an OpenGL context is made current
    on this window.
 */
void QPlatformWindow::invalidateSurface()
{
}

/*!
    Helper function to get initial geometry on windowing systems which do not
    do smart positioning and also do not provide a means of centering a
    transient window w.r.t. its parent. For example this is useful on Windows
    and MacOS but not X11, because an X11 window manager typically tries to
    layout new windows to optimize usage of the available desktop space.
    However if the given window already has geometry which the application has
    initialized, it takes priority.
*/
QRect QPlatformWindow::initialGeometry(const QWindow *w,
    const QRect &initialGeometry, int defaultWidth, int defaultHeight)
{
    QRect rect(initialGeometry);
    if (rect.width() == 0) {
        const int minWidth = w->minimumWidth();
        rect.setWidth(minWidth > 0 ? minWidth : defaultWidth);
    }
    if (rect.height() == 0) {
        const int minHeight = w->minimumHeight();
        rect.setHeight(minHeight > 0 ? minHeight : defaultHeight);
    }
    if (w->isTopLevel() && qt_window_private(const_cast<QWindow*>(w))->positionAutomatic
        && w->type() != Qt::Popup) {
        if (const QScreen *screen = effectiveScreen(w)) {
            const QRect availableGeometry = screen->availableGeometry();
            // Center unless the geometry ( + unknown window frame) is too large for the screen).
            if (rect.height() < (availableGeometry.height() * 8) / 9
                && rect.width() < (availableGeometry.width() * 8) / 9) {
                const QWindow *tp = w->transientParent();
                if (tp) {
                    // A transient window should be centered w.r.t. its transient parent.
                    rect.moveCenter(tp->geometry().center());
                } else {
                    // Center the window on the screen.  (Only applicable on platforms
                    // which do not provide a better way.)
                    rect.moveCenter(availableGeometry.center());
                }
            }
        }
    }
    return rect;
}

/*!
    \class QPlatformWindow
    \since 4.8
    \internal
    \preliminary
    \ingroup qpa

    \brief The QPlatformWindow class provides an abstraction for top-level windows.

    The QPlatformWindow abstraction is used by QWindow for all its top level windows. It is being
    created by calling the createPlatformWindow function in the loaded QPlatformIntegration
    instance.

    QPlatformWindow is used to signal to the windowing system, how Qt perceives its frame.
    However, it is not concerned with how Qt renders into the window it represents.

    Visible QWindows will always have a QPlatformWindow. However, it is not necessary for
    all windows to have a QBackingStore. This is the case for QOpenGLWidget. And could be the case for
    windows where some  3.party renders into it.

    The platform specific window handle can be retrieved by the winId function.

    QPlatformWindow is also the way QPA defines how native child windows should be supported
    through the setParent function.

    \section1 Implementation Aspects

    \list 1
        \li Mouse grab: Qt expects windows to automatically grab the mouse if the user presses
            a button until the button is released.
            Automatic grab should be released if some window is explicitly grabbed.
        \li Enter/Leave events: If there is a window explicitly grabbing mouse events
            (\c{setMouseGrabEnabled()}), enter and leave events should only be sent to the
            grabbing window when mouse cursor passes over the grabbing window boundary.
            Other windows will not receive enter or leave events while the grab is active.
            While an automatic mouse grab caused by a mouse button press is active, no window
            will receive enter or leave events. When the last mouse button is released, the
            autograbbing window will receive leave event if mouse cursor is no longer within
            the window boundary.
            When any grab starts, the window under cursor will receive a leave event unless
            it is the grabbing window.
            When any grab ends, the window under cursor will receive an enter event unless it
            was the grabbing window.
        \li Window positioning: When calling \c{QWindow::setFramePosition()}, the flag
            \c{QWindowPrivate::positionPolicy} is set to \c{QWindowPrivate::WindowFrameInclusive}.
            This means the position includes the window frame, whose size is at this point
            unknown and the geometry's topleft point is the position of the window frame.
    \endlist

    Apart from the auto-tests (\c{tests/auto/gui/kernel/qwindow},
    \c{tests/auto/gui/kernel/qguiapplication} and \c{tests/auto/widgets/kernel/qwidget}),
    there are a number of manual tests and examples that can help testing a platform plugin:

    \list 1
        \li \c{examples/qpa/windows}: Basic \c{QWindow} creation.
        \li \c{examples/opengl/hellowindow}: Basic Open GL windows.
        \li \c{tests/manual/windowflags}: Tests setting the window flags.
        \li \c{tests/manual/windowgeometry} Tests setting the window geometry.
        \li \c{tests/manual/windowmodality} Tests setting the window modality.
        \li \c{tests/manual/widgetgrab} Tests mouse grab and dialogs.
    \endlist

    \sa QBackingStore, QWindow
*/

QT_END_NAMESPACE
