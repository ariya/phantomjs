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

#include "qevent.h"
#include "qcursor.h"
#include "private/qguiapplication_p.h"
#include "qpa/qplatformintegration.h"
#include "qpa/qplatformdrag.h"
#include "private/qevent_p.h"
#include "qdebug.h"
#include "qmimedata.h"
#include "private/qdnd_p.h"
#include "qevent_p.h"
#include "qmath.h"


QT_BEGIN_NAMESPACE

/*!
    \class QEnterEvent
    \ingroup events
    \inmodule QtGui

    \brief The QEnterEvent class contains parameters that describe an enter event.

    Enter events occur when the mouse cursor enters a window or a widget.

    \since 5.0
*/

/*!
    Constructs an enter event object.

    The points \a localPos, \a windowPos and \a screenPos specify the
    mouse cursor's position relative to the receiving widget or item,
    window, and screen, respectively.
*/

QEnterEvent::QEnterEvent(const QPointF &localPos, const QPointF &windowPos, const QPointF &screenPos)
    : QEvent(QEvent::Enter)
    , l(localPos)
    , w(windowPos)
    , s(screenPos)
{
}

/*!
    \internal
*/
QEnterEvent::~QEnterEvent()
{
}

/*!
    \class QInputEvent
    \ingroup events
    \inmodule QtGui

    \brief The QInputEvent class is the base class for events that
    describe user input.
*/

/*!
  \internal
*/
QInputEvent::QInputEvent(Type type, Qt::KeyboardModifiers modifiers)
    : QEvent(type), modState(modifiers), ts(0)
{}

/*!
  \internal
*/
QInputEvent::~QInputEvent()
{
}

/*!
    \fn Qt::KeyboardModifiers QInputEvent::modifiers() const

    Returns the keyboard modifier flags that existed immediately
    before the event occurred.

    \sa QGuiApplication::keyboardModifiers()
*/

/*! \fn void QInputEvent::setModifiers(Qt::KeyboardModifiers modifiers)

    \internal

    Sets the keyboard modifiers flags for this event.
*/

/*!
    \fn ulong QInputEvent::timestamp() const

    Returns the window system's timestamp for this event.
*/

/*! \fn void QInputEvent::setTimestamp(ulong atimestamp)

    \internal

    Sets the timestamp for this event.
*/

/*!
    \class QMouseEvent
    \ingroup events
    \inmodule QtGui

    \brief The QMouseEvent class contains parameters that describe a mouse event.

    Mouse events occur when a mouse button is pressed or released
    inside a widget, or when the mouse cursor is moved.

    Mouse move events will occur only when a mouse button is pressed
    down, unless mouse tracking has been enabled with
    QWidget::setMouseTracking().

    Qt automatically grabs the mouse when a mouse button is pressed
    inside a widget; the widget will continue to receive mouse events
    until the last mouse button is released.

    A mouse event contains a special accept flag that indicates
    whether the receiver wants the event. You should call ignore() if
    the mouse event is not handled by your widget. A mouse event is
    propagated up the parent widget chain until a widget accepts it
    with accept(), or an event filter consumes it.

    \note If a mouse event is propagated to a \l{QWidget}{widget} for
    which Qt::WA_NoMousePropagation has been set, that mouse event
    will not be propagated further up the parent widget chain.

    The state of the keyboard modifier keys can be found by calling the
    \l{QInputEvent::modifiers()}{modifiers()} function, inherited from
    QInputEvent.

    The functions pos(), x(), and y() give the cursor position
    relative to the widget that receives the mouse event. If you
    move the widget as a result of the mouse event, use the global
    position returned by globalPos() to avoid a shaking motion.

    The QWidget::setEnabled() function can be used to enable or
    disable mouse and keyboard events for a widget.

    Reimplement the QWidget event handlers, QWidget::mousePressEvent(),
    QWidget::mouseReleaseEvent(), QWidget::mouseDoubleClickEvent(),
    and QWidget::mouseMoveEvent() to receive mouse events in your own
    widgets.

    \sa QWidget::setMouseTracking(), QWidget::grabMouse(),
    QCursor::pos()
*/

/*!
    Constructs a mouse event object.

    The \a type parameter must be one of QEvent::MouseButtonPress,
    QEvent::MouseButtonRelease, QEvent::MouseButtonDblClick,
    or QEvent::MouseMove.

    The \a localPos is the mouse cursor's position relative to the
    receiving widget or item. The window position is set to the same value
    as \a localPos.
    The \a button that caused the event is given as a value from
    the Qt::MouseButton enum. If the event \a type is
    \l MouseMove, the appropriate button for this event is Qt::NoButton.
    The mouse and keyboard states at the time of the event are specified by
    \a buttons and \a modifiers.

    The screenPos() is initialized to QCursor::pos(), which may not
    be appropriate. Use the other constructor to specify the global
    position explicitly.
*/
QMouseEvent::QMouseEvent(Type type, const QPointF &localPos, Qt::MouseButton button,
                         Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers)
    : QInputEvent(type, modifiers), l(localPos), w(localPos), b(button), mouseState(buttons), caps(0)
{
    s = QCursor::pos();
}


/*!
    Constructs a mouse event object.

    The \a type parameter must be QEvent::MouseButtonPress,
    QEvent::MouseButtonRelease, QEvent::MouseButtonDblClick,
    or QEvent::MouseMove.

    The \a localPos is the mouse cursor's position relative to the
    receiving widget or item. The cursor's position in screen coordinates is
    specified by \a screenPos. The window position is set to the same value
    as \a localPos. The \a button that caused the event is
    given as a value from the \l Qt::MouseButton enum. If the event \a
    type is \l MouseMove, the appropriate button for this event is
    Qt::NoButton. \a buttons is the state of all buttons at the
    time of the event, \a modifiers the state of all keyboard
    modifiers.

*/
QMouseEvent::QMouseEvent(Type type, const QPointF &localPos, const QPointF &screenPos,
                         Qt::MouseButton button, Qt::MouseButtons buttons,
                         Qt::KeyboardModifiers modifiers)
    : QInputEvent(type, modifiers), l(localPos), w(localPos), s(screenPos), b(button), mouseState(buttons), caps(0)
{}

/*!
    Constructs a mouse event object.

    The \a type parameter must be QEvent::MouseButtonPress,
    QEvent::MouseButtonRelease, QEvent::MouseButtonDblClick,
    or QEvent::MouseMove.

    The points \a localPos, \a windowPos and \a screenPos specify the
    mouse cursor's position relative to the receiving widget or item,
    window, and screen, respectively.

    The \a button that caused the event is
    given as a value from the \l Qt::MouseButton enum. If the event \a
    type is \l MouseMove, the appropriate button for this event is
    Qt::NoButton. \a buttons is the state of all buttons at the
    time of the event, \a modifiers the state of all keyboard
    modifiers.

*/
QMouseEvent::QMouseEvent(Type type, const QPointF &localPos, const QPointF &windowPos, const QPointF &screenPos,
                         Qt::MouseButton button, Qt::MouseButtons buttons,
                         Qt::KeyboardModifiers modifiers)
    : QInputEvent(type, modifiers), l(localPos), w(windowPos), s(screenPos), b(button), mouseState(buttons), caps(0)
{}

/*!
    \internal
*/
QMouseEvent::~QMouseEvent()
{
}

/*!
  \since 5.3

  Returns information about the mouse event source.

  The mouse event source can be used to distinguish between genuine
  and artificial mouse events. The latter are events that are
  synthesized from touch events by the operating system or Qt itself.

  \note Many platforms provide no such information. On such platforms
  \l Qt::MouseEventNotSynthesized is returned always.

  \sa Qt::MouseEventSource
 */
Qt::MouseEventSource QMouseEvent::source() const
{
    return QGuiApplicationPrivate::mouseEventSource(this);
}

/*!
     \since 5.3

     Returns the mouse event flags.

     The mouse event flags provide additional information about a mouse event.

     \sa Qt::MouseEventFlag
 */
Qt::MouseEventFlags QMouseEvent::flags() const
{
    return QGuiApplicationPrivate::mouseEventFlags(this);
}

/*!
    \fn QPointF QMouseEvent::localPos() const

    \since 5.0

    Returns the position of the mouse cursor as a QPointF, relative to the
    widget or item that received the event.

    If you move the widget as a result of the mouse event, use the
    screen position returned by screenPos() to avoid a shaking
    motion.

    \sa x(), y(), windowPos(), screenPos()
*/

/*!
    \fn QPointF QMouseEvent::windowPos() const

    \since 5.0

    Returns the position of the mouse cursor as a QPointF, relative to the
    window that received the event.

    If you move the widget as a result of the mouse event, use the
    global position returned by globalPos() to avoid a shaking
    motion.

    \sa x(), y(), pos(), localPos(), screenPos()
*/

/*!
    \fn QPointF QMouseEvent::screenPos() const

    \since 5.0

    Returns the position of the mouse cursor as a QPointF, relative to the
    screen that received the event.

    \sa x(), y(), pos(), localPos(), windowPos()
*/

/*!
    \fn QPoint QMouseEvent::pos() const

    Returns the position of the mouse cursor, relative to the widget
    that received the event.

    If you move the widget as a result of the mouse event, use the
    global position returned by globalPos() to avoid a shaking
    motion.

    \sa x(), y(), globalPos()
*/

/*!
    \fn QPoint QMouseEvent::globalPos() const

    Returns the global position of the mouse cursor \e{at the time
    of the event}. This is important on asynchronous window systems
    like X11. Whenever you move your widgets around in response to
    mouse events, globalPos() may differ a lot from the current
    pointer position QCursor::pos(), and from
    QWidget::mapToGlobal(pos()).

    \sa globalX(), globalY()
*/

/*!
    \fn int QMouseEvent::x() const

    Returns the x position of the mouse cursor, relative to the
    widget that received the event.

    \sa y(), pos()
*/

/*!
    \fn int QMouseEvent::y() const

    Returns the y position of the mouse cursor, relative to the
    widget that received the event.

    \sa x(), pos()
*/

/*!
    \fn int QMouseEvent::globalX() const

    Returns the global x position of the mouse cursor at the time of
    the event.

    \sa globalY(), globalPos()
*/

/*!
    \fn int QMouseEvent::globalY() const

    Returns the global y position of the mouse cursor at the time of
    the event.

    \sa globalX(), globalPos()
*/

/*!
    \fn Qt::MouseButton QMouseEvent::button() const

    Returns the button that caused the event.

    Note that the returned value is always Qt::NoButton for mouse
    move events.

    \sa buttons(), Qt::MouseButton
*/

/*!
    \fn Qt::MouseButton QMouseEvent::buttons() const

    Returns the button state when the event was generated. The button
    state is a combination of Qt::LeftButton, Qt::RightButton,
    Qt::MidButton using the OR operator. For mouse move events,
    this is all buttons that are pressed down. For mouse press and
    double click events this includes the button that caused the
    event. For mouse release events this excludes the button that
    caused the event.

    \sa button(), Qt::MouseButton
*/

/*!
    \fn QPointF QMouseEvent::posF() const
    \obsolete

    Use localPos() instead.
*/

/*!
    \class QHoverEvent
    \ingroup events
    \inmodule QtGui

    \brief The QHoverEvent class contains parameters that describe a mouse event.

    Mouse events occur when a mouse cursor is moved into, out of, or within a
    widget, and if the widget has the Qt::WA_Hover attribute.

    The function pos() gives the current cursor position, while oldPos() gives
    the old mouse position.

    There are a few similarities between the events QEvent::HoverEnter
    and QEvent::HoverLeave, and the events QEvent::Enter and QEvent::Leave.
    However, they are slightly different because we do an update() in the event
    handler of HoverEnter and HoverLeave.

    QEvent::HoverMove is also slightly different from QEvent::MouseMove. Let us
    consider a top-level window A containing a child B which in turn contains a
    child C (all with mouse tracking enabled):

    \image hoverevents.png

    Now, if you move the cursor from the top to the bottom in the middle of A,
    you will get the following QEvent::MouseMove events:

    \list 1
        \li A::MouseMove
        \li B::MouseMove
        \li C::MouseMove
    \endlist

    You will get the same events for QEvent::HoverMove, except that the event
    always propagates to the top-level regardless whether the event is accepted
    or not. It will only stop propagating with the Qt::WA_NoMousePropagation
    attribute.

    In this case the events will occur in the following way:

    \list 1
        \li A::HoverMove
        \li A::HoverMove, B::HoverMove
        \li A::HoverMove, B::HoverMove, C::HoverMove
    \endlist

*/

/*!
    \fn QPoint QHoverEvent::pos() const

    Returns the position of the mouse cursor, relative to the widget
    that received the event.

    On QEvent::HoverLeave events, this position will always be
    QPoint(-1, -1).

    \sa oldPos()
*/

/*!
    \fn QPoint QHoverEvent::oldPos() const

    Returns the previous position of the mouse cursor, relative to the widget
    that received the event. If there is no previous position, oldPos() will
    return the same position as pos().

    On QEvent::HoverEnter events, this position will always be
    QPoint(-1, -1).

    \sa pos()
*/

/*!
    \fn const QPointF &QHoverEvent::posF() const

    Returns the position of the mouse cursor, relative to the widget
    that received the event.

    On QEvent::HoverLeave events, this position will always be
    QPointF(-1, -1).

    \sa oldPosF()
*/

/*!
    \fn const QPointF &QHoverEvent::oldPosF() const

    Returns the previous position of the mouse cursor, relative to the widget
    that received the event. If there is no previous position, oldPosF() will
    return the same position as posF().

    On QEvent::HoverEnter events, this position will always be
    QPointF(-1, -1).

    \sa posF()
*/

/*!
    Constructs a hover event object.

    The \a type parameter must be QEvent::HoverEnter,
    QEvent::HoverLeave, or QEvent::HoverMove.

    The \a pos is the current mouse cursor's position relative to the
    receiving widget, while \a oldPos is its previous such position.
    \a modifiers hold the state of all keyboard modifiers at the time
    of the event.
*/
QHoverEvent::QHoverEvent(Type type, const QPointF &pos, const QPointF &oldPos, Qt::KeyboardModifiers modifiers)
    : QInputEvent(type, modifiers), p(pos), op(oldPos)
{
}

/*!
    \internal
*/
QHoverEvent::~QHoverEvent()
{
}


/*!
    \class QWheelEvent
    \brief The QWheelEvent class contains parameters that describe a wheel event.
    \inmodule QtGui

    \ingroup events

    Wheel events are sent to the widget under the mouse cursor, but
    if that widget does not handle the event they are sent to the
    focus widget. Wheel events are generated for both mouse wheels
    and trackpad scroll gestures. There are two ways to read the
    wheel event delta: angleDelta() returns the delta in wheel
    degrees. This value is always provided. pixelDelta() returns
    the delta in screen pixels and is available on platforms that
    have high-resolution trackpads, such as Mac OS X.

    The functions pos() and globalPos() return the mouse cursor's
    location at the time of the event.

    A wheel event contains a special accept flag that indicates
    whether the receiver wants the event. You should call ignore() if
    you do not handle the wheel event; this ensures that it will be
    sent to the parent widget.

    The QWidget::setEnabled() function can be used to enable or
    disable mouse and keyboard events for a widget.

    The event handler QWidget::wheelEvent() receives wheel events.

    \sa QMouseEvent, QWidget::grabMouse()
*/

/*!
    \fn Qt::MouseButtons QWheelEvent::buttons() const

    Returns the mouse state when the event occurred.
*/

/*!
    \fn Qt::Orientation QWheelEvent::orientation() const
    \obsolete

    Returns the wheel's orientation.

    Use angleDelta() instead.
*/

/*!
    \obsolete
    Constructs a wheel event object.

    Use the constructor taking \e angleDelta and \e pixelDelta QPoints instead.

    The position, \a pos, is the location of the mouse cursor within
    the widget. The globalPos() is initialized to QCursor::pos()
    which is usually, but not always, correct.
    Use the other constructor if you need to specify the global
    position explicitly.

    The \a buttons describe the state of the mouse buttons at the time
    of the event, \a delta contains the rotation distance,
    \a modifiers holds the keyboard modifier flags at the time of the
    event, and \a orient holds the wheel's orientation.

    \sa pos(), pixelDelta(), angleDelta()
*/
#ifndef QT_NO_WHEELEVENT
QWheelEvent::QWheelEvent(const QPointF &pos, int delta,
                         Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers,
                         Qt::Orientation orient)
    : QInputEvent(Wheel, modifiers), p(pos), qt4D(delta), qt4O(orient), mouseState(buttons)
{
    g = QCursor::pos();
}

/*!
  \internal
*/
QWheelEvent::~QWheelEvent()
{
}

/*!
    \obsolete
    Constructs a wheel event object.

    Use the constructor taking \e angleDelta and \e pixelDelta QPoints instead.

    The \a pos provides the location of the mouse cursor
    within the widget. The position in global coordinates is specified
    by \a globalPos. \a delta contains the rotation distance, \a modifiers
    holds the keyboard modifier flags at the time of the event, and
    \a orient holds the wheel's orientation.


    \sa pos(), pixelDelta(), angleDelta()
*/
QWheelEvent::QWheelEvent(const QPointF &pos, const QPointF& globalPos, int delta,
                         Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers,
                         Qt::Orientation orient)
    : QInputEvent(Wheel, modifiers), p(pos), g(globalPos), qt4D(delta), qt4O(orient), mouseState(buttons)
{}

/*!
    Constructs a wheel event object.

    The \a pos provides the location of the mouse cursor
    within the window. The position in global coordinates is specified
    by \a globalPos.

    \a pixelDelta contains the scrolling distance in pixels on screen, while
    \a angleDelta contains the wheel rotation distance. \a pixelDelta is
    optional and can be null.

    The mouse and keyboard states at the time of the event are specified by
    \a buttons and \a modifiers.

    For backwards compatibility, the event can also hold monodirectional wheel
    event data: \a qt4Delta specifies the rotation, and \a qt4Orientation the
    direction.

    The phase() is initialized to Qt::ScrollUpdate. Use the other constructor
    to specify the phase explicitly.

    \sa posF(), globalPosF(), angleDelta(), pixelDelta()
*/

QWheelEvent::QWheelEvent(const QPointF &pos, const QPointF& globalPos,
            QPoint pixelDelta, QPoint angleDelta, int qt4Delta, Qt::Orientation qt4Orientation,
            Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers)
    : QInputEvent(Wheel, modifiers), p(pos), g(globalPos), pixelD(pixelDelta),
      angleD(angleDelta), qt4D(qt4Delta), qt4O(qt4Orientation), mouseState(buttons), ph(Qt::ScrollUpdate)
{}

/*!
    Constructs a wheel event object.

    The \a pos provides the location of the mouse cursor
    within the window. The position in global coordinates is specified
    by \a globalPos.

    \a pixelDelta contains the scrolling distance in pixels on screen, while
    \a angleDelta contains the wheel rotation distance. \a pixelDelta is
    optional and can be null.

    The mouse and keyboard states at the time of the event are specified by
    \a buttons and \a modifiers.

    For backwards compatibility, the event can also hold monodirectional wheel
    event data: \a qt4Delta specifies the rotation, and \a qt4Orientation the
    direction.

    The scrolling phase of the event is specified by \a phase.

    \sa posF(), globalPosF(), angleDelta(), pixelDelta(), phase()
*/

QWheelEvent::QWheelEvent(const QPointF &pos, const QPointF& globalPos,
            QPoint pixelDelta, QPoint angleDelta, int qt4Delta, Qt::Orientation qt4Orientation,
            Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers, Qt::ScrollPhase phase)
    : QInputEvent(Wheel, modifiers), p(pos), g(globalPos), pixelD(pixelDelta),
      angleD(angleDelta), qt4D(qt4Delta), qt4O(qt4Orientation), mouseState(buttons), ph(phase)
{}

#endif // QT_NO_WHEELEVENT

/*!
    \fn QPoint QWheelEvent::pixelDelta() const

    Returns the scrolling distance in pixels on screen. This value is
    provided on platforms that support high-resolution pixel-based
    delta values, such as Mac OS X. The value should be used directly
    to scroll content on screen.

    Example:

    \snippet code/src_gui_kernel_qevent.cpp 0

    \note On platforms that support scrolling \l{phase()}{phases}, the delta may be null when:
    \list
    \li scrolling is about to begin, but the distance did not yet change (Qt::ScrollBegin),
    \li or scrolling has ended and the distance did not change anymore (Qt::ScrollEnd).
    \endlist
*/

/*!
    \fn QPoint QWheelEvent::angleDelta() const

    Returns the distance that the wheel is rotated, in eighths of a
    degree. A positive value indicates that the wheel was rotated
    forwards away from the user; a negative value indicates that the
    wheel was rotated backwards toward the user.

    Most mouse types work in steps of 15 degrees, in which case the
    delta value is a multiple of 120; i.e., 120 units * 1/8 = 15 degrees.

    However, some mice have finer-resolution wheels and send delta values
    that are less than 120 units (less than 15 degrees). To support this
    possibility, you can either cumulatively add the delta values from events
    until the value of 120 is reached, then scroll the widget, or you can
    partially scroll the widget in response to each wheel event.

    Example:

    \snippet code/src_gui_kernel_qevent.cpp 0

    \note On platforms that support scrolling \l{phase()}{phases}, the delta may be null when:
    \list
    \li scrolling is about to begin, but the distance did not yet change (Qt::ScrollBegin),
    \li or scrolling has ended and the distance did not change anymore (Qt::ScrollEnd).
    \endlist
*/

/*!
    \fn int QWheelEvent::delta() const
    \obsolete

    This function has been deprecated, use pixelDelta() or angleDelta() instead.
*/

/*!
    \fn QPoint QWheelEvent::pos() const

    Returns the position of the mouse cursor relative to the widget
    that received the event.

    If you move your widgets around in response to mouse events,
    use globalPos() instead of this function.

    \sa x(), y(), globalPos()
*/

/*!
    \fn int QWheelEvent::x() const

    Returns the x position of the mouse cursor, relative to the
    widget that received the event.

    \sa y(), pos()
*/

/*!
    \fn int QWheelEvent::y() const

    Returns the y position of the mouse cursor, relative to the
    widget that received the event.

    \sa x(), pos()
*/


/*!
    \fn QPoint QWheelEvent::globalPos() const

    Returns the global position of the mouse pointer \e{at the time
    of the event}. This is important on asynchronous window systems
    such as X11; whenever you move your widgets around in response to
    mouse events, globalPos() can differ a lot from the current
    cursor position returned by QCursor::pos().

    \sa globalX(), globalY()
*/

/*!
    \fn int QWheelEvent::globalX() const

    Returns the global x position of the mouse cursor at the time of
    the event.

    \sa globalY(), globalPos()
*/

/*!
    \fn int QWheelEvent::globalY() const

    Returns the global y position of the mouse cursor at the time of
    the event.

    \sa globalX(), globalPos()
*/

/*!
    \fn const QPointF &QWheelEvent::posF() const

    Returns the position of the mouse cursor relative to the widget
    that received the event.

    If you move your widgets around in response to mouse events,
    use globalPosF() instead of this function.

    \sa globalPosF()
*/

/*!
    \fn const QPointF &QWheelEvent::globalPosF() const

    Returns the global position of the mouse pointer \e{at the time
    of the event}. This is important on asynchronous window systems
    such as X11; whenever you move your widgets around in response to
    mouse events, globalPosF() can differ a lot from the current
    cursor position returned by QCursor::pos().

    \sa posF()
*/

/*!
    \fn Qt::ScrollPhase QWheelEvent::phase() const
    \since 5.2

    Returns the scrolling phase of this wheel event.

    \note The Qt::ScrollBegin and Qt::ScrollEnd phases are currently
    supported only on Mac OS X.
*/


/*!
    \class QKeyEvent
    \brief The QKeyEvent class describes a key event.

    \ingroup events
    \inmodule QtGui

    Key events are sent to the widget with keyboard input focus
    when keys are pressed or released.

    A key event contains a special accept flag that indicates whether
    the receiver will handle the key event. You should call ignore()
    if the key press or release event is not handled by your widget.
    A key event is propagated up the parent widget chain until a
    widget accepts it with accept() or an event filter consumes it.
    Key events for multimedia keys are ignored by default. You should
    call accept() if your widget handles those events.

    The QWidget::setEnable() function can be used to enable or disable
    mouse and keyboard events for a widget.

    The event handlers QWidget::keyPressEvent(), QWidget::keyReleaseEvent(),
    QGraphicsItem::keyPressEvent() and QGraphicsItem::keyReleaseEvent()
    receive key events.

    \sa QFocusEvent, QWidget::grabKeyboard()
*/

/*!
    Constructs a key event object.

    The \a type parameter must be QEvent::KeyPress, QEvent::KeyRelease,
    or QEvent::ShortcutOverride.

    Int \a key is the code for the Qt::Key that the event loop should listen
    for. If \a key is 0, the event is not a result of a known key; for
    example, it may be the result of a compose sequence or keyboard macro.
    The \a modifiers holds the keyboard modifiers, and the given \a text
    is the Unicode text that the key generated. If \a autorep is true,
    isAutoRepeat() will be true. \a count is the number of keys involved
    in the event.
*/
QKeyEvent::QKeyEvent(Type type, int key, Qt::KeyboardModifiers modifiers, const QString& text,
                     bool autorep, ushort count)
    : QInputEvent(type, modifiers), txt(text), k(key),
      nScanCode(0), nVirtualKey(0), nModifiers(0),
      c(count), autor(autorep)
{
}

/*!
    Constructs a key event object.

    The \a type parameter must be QEvent::KeyPress, QEvent::KeyRelease,
    or QEvent::ShortcutOverride.

    Int \a key is the code for the Qt::Key that the event loop should listen
    for. If \a key is 0, the event is not a result of a known key; for
    example, it may be the result of a compose sequence or keyboard macro.
    The \a modifiers holds the keyboard modifiers, and the given \a text
    is the Unicode text that the key generated. If \a autorep is true,
    isAutoRepeat() will be true. \a count is the number of keys involved
    in the event.

    In addition to the normal key event data, also contains \a nativeScanCode,
    \a nativeVirtualKey and \a nativeModifiers. This extra data is used by the
    shortcut system, to determine which shortcuts to trigger.
*/
QKeyEvent::QKeyEvent(Type type, int key, Qt::KeyboardModifiers modifiers,
                     quint32 nativeScanCode, quint32 nativeVirtualKey, quint32 nativeModifiers,
                     const QString &text, bool autorep, ushort count)
    : QInputEvent(type, modifiers), txt(text), k(key),
      nScanCode(nativeScanCode), nVirtualKey(nativeVirtualKey), nModifiers(nativeModifiers),
      c(count), autor(autorep)
{
}


/*!
  \internal
*/
QKeyEvent::~QKeyEvent()
{
}

/*!
    \fn QKeyEvent *QKeyEvent::createExtendedKeyEvent(Type type, int key, Qt::KeyboardModifiers modifiers, quint32 nativeScanCode, quint32 nativeVirtualKey, quint32 nativeModifiers, const QString& text, bool autorep, ushort count)
    \internal
*/

/*!
    \fn bool QKeyEvent::hasExtendedInfo() const
    \internal
*/

/*!
  \fn quint32 QKeyEvent::nativeScanCode() const
  \since 4.2

  Returns the native scan code of the key event.  If the key event
  does not contain this data 0 is returned.

  Note: The native scan code may be 0, even if the key event contains
  extended information.

  Note: On Mac OS/X, this function is not useful, because there is no
  way to get the scan code from Carbon or Cocoa. The function always
  returns 1 (or 0 in the case explained above).
*/

/*!
    \fn quint32 QKeyEvent::nativeVirtualKey() const
    \since 4.2

    Returns the native virtual key, or key sym of the key event.
    If the key event does not contain this data 0 is returned.

    Note: The native virtual key may be 0, even if the key event contains extended information.
*/

/*!
    \fn quint32 QKeyEvent::nativeModifiers() const
    \since 4.2

    Returns the native modifiers of a key event.
    If the key event does not contain this data 0 is returned.

    Note: The native modifiers may be 0, even if the key event contains extended information.
*/

/*!
    \fn int QKeyEvent::key() const

    Returns the code of the key that was pressed or released.

    See \l Qt::Key for the list of keyboard codes. These codes are
    independent of the underlying window system. Note that this
    function does not distinguish between capital and non-capital
    letters, use the text() function (returning the Unicode text the
    key generated) for this purpose.

    A value of either 0 or Qt::Key_unknown means that the event is not
    the result of a known key; for example, it may be the result of
    a compose sequence, a keyboard macro, or due to key event
    compression.

    \sa Qt::WA_KeyCompression
*/

/*!
    \fn QString QKeyEvent::text() const

    Returns the Unicode text that this key generated.

    Return values when modifier keys such as
    Shift, Control, Alt, and Meta are pressed
    differ among platforms and could return an empty string.

    \note \l key() will always return a valid value,
    independent of modifier keys.

    \sa Qt::WA_KeyCompression
*/

/*!
    Returns the keyboard modifier flags that existed immediately
    after the event occurred.

    \warning This function cannot always be trusted. The user can
    confuse it by pressing both \uicontrol{Shift} keys simultaneously and
    releasing one of them, for example.

    \sa QGuiApplication::keyboardModifiers()
*/

Qt::KeyboardModifiers QKeyEvent::modifiers() const
{
    if (key() == Qt::Key_Shift)
        return Qt::KeyboardModifiers(QInputEvent::modifiers()^Qt::ShiftModifier);
    if (key() == Qt::Key_Control)
        return Qt::KeyboardModifiers(QInputEvent::modifiers()^Qt::ControlModifier);
    if (key() == Qt::Key_Alt)
        return Qt::KeyboardModifiers(QInputEvent::modifiers()^Qt::AltModifier);
    if (key() == Qt::Key_Meta)
        return Qt::KeyboardModifiers(QInputEvent::modifiers()^Qt::MetaModifier);
    if (key() == Qt::Key_AltGr)
        return Qt::KeyboardModifiers(QInputEvent::modifiers()^Qt::GroupSwitchModifier);
    return QInputEvent::modifiers();
}

#ifndef QT_NO_SHORTCUT
/*!
    \fn bool QKeyEvent::matches(QKeySequence::StandardKey key) const
    \since 4.2

    Returns \c true if the key event matches the given standard \a key;
    otherwise returns \c false.
*/
bool QKeyEvent::matches(QKeySequence::StandardKey matchKey) const
{
    //The keypad and group switch modifier should not make a difference
    uint searchkey = (modifiers() | key()) & ~(Qt::KeypadModifier | Qt::GroupSwitchModifier);

    const QList<QKeySequence> bindings = QKeySequence::keyBindings(matchKey);
    return bindings.contains(QKeySequence(searchkey));
}
#endif // QT_NO_SHORTCUT


/*!
    \fn bool QKeyEvent::isAutoRepeat() const

    Returns \c true if this event comes from an auto-repeating key;
    returns \c false if it comes from an initial key press.

    Note that if the event is a multiple-key compressed event that is
    partly due to auto-repeat, this function could return either true
    or false indeterminately.
*/

/*!
    \fn int QKeyEvent::count() const

    Returns the number of keys involved in this event. If text()
    is not empty, this is simply the length of the string.

    \sa Qt::WA_KeyCompression
*/

/*!
    \class QFocusEvent
    \brief The QFocusEvent class contains event parameters for widget focus
    events.
    \inmodule QtGui

    \ingroup events

    Focus events are sent to widgets when the keyboard input focus
    changes. Focus events occur due to mouse actions, key presses
    (such as \uicontrol{Tab} or \uicontrol{Backtab}), the window system, popup
    menus, keyboard shortcuts, or other application-specific reasons.
    The reason for a particular focus event is returned by reason()
    in the appropriate event handler.

    The event handlers QWidget::focusInEvent(),
    QWidget::focusOutEvent(), QGraphicsItem::focusInEvent and
    QGraphicsItem::focusOutEvent() receive focus events.

    \sa QWidget::setFocus(), QWidget::setFocusPolicy(), {Keyboard Focus in Widgets}
*/

/*!
    Constructs a focus event object.

    The \a type parameter must be either QEvent::FocusIn or
    QEvent::FocusOut. The \a reason describes the cause of the change
    in focus.
*/
QFocusEvent::QFocusEvent(Type type, Qt::FocusReason reason)
    : QEvent(type), m_reason(reason)
{}

/*!
    \internal
*/
QFocusEvent::~QFocusEvent()
{
}

/*!
    Returns the reason for this focus event.
 */
Qt::FocusReason QFocusEvent::reason() const
{
    return m_reason;
}

/*!
    \fn bool QFocusEvent::gotFocus() const

    Returns \c true if type() is QEvent::FocusIn; otherwise returns
    false.
*/

/*!
    \fn bool QFocusEvent::lostFocus() const

    Returns \c true if type() is QEvent::FocusOut; otherwise returns
    false.
*/


/*!
    \class QPaintEvent
    \brief The QPaintEvent class contains event parameters for paint events.
    \inmodule QtGui

    \ingroup events

    Paint events are sent to widgets that need to update themselves,
    for instance when part of a widget is exposed because a covering
    widget was moved.

    The event contains a region() that needs to be updated, and a
    rect() that is the bounding rectangle of that region. Both are
    provided because many widgets can't make much use of region(),
    and rect() can be much faster than region().boundingRect().

    \section1 Automatic Clipping

    Painting is clipped to region() during the processing of a paint
    event. This clipping is performed by Qt's paint system and is
    independent of any clipping that may be applied to a QPainter used to
    draw on the paint device.

    As a result, the value returned by QPainter::clipRegion() on
    a newly-constructed QPainter will not reflect the clip region that is
    used by the paint system.

    \sa QPainter, QWidget::update(), QWidget::repaint(),
        QWidget::paintEvent()
*/

/*!
    Constructs a paint event object with the region that needs to
    be updated. The region is specified by \a paintRegion.
*/
QPaintEvent::QPaintEvent(const QRegion& paintRegion)
    : QEvent(Paint), m_rect(paintRegion.boundingRect()), m_region(paintRegion), m_erased(false)
{}

/*!
    Constructs a paint event object with the rectangle that needs
    to be updated. The region is specified by \a paintRect.
*/
QPaintEvent::QPaintEvent(const QRect &paintRect)
    : QEvent(Paint), m_rect(paintRect),m_region(paintRect), m_erased(false)
{}


/*!
  \internal
*/
QPaintEvent::~QPaintEvent()
{
}

/*!
    \fn const QRect &QPaintEvent::rect() const

    Returns the rectangle that needs to be updated.

    \sa region(), QPainter::setClipRect()
*/

/*!
    \fn const QRegion &QPaintEvent::region() const

    Returns the region that needs to be updated.

    \sa rect(), QPainter::setClipRegion()
*/


/*!
    \class QMoveEvent
    \brief The QMoveEvent class contains event parameters for move events.
    \inmodule QtGui

    \ingroup events

    Move events are sent to widgets that have been moved to a new
    position relative to their parent.

    The event handler QWidget::moveEvent() receives move events.

    \sa QWidget::move(), QWidget::setGeometry()
*/

/*!
    Constructs a move event with the new and old widget positions,
    \a pos and \a oldPos respectively.
*/
QMoveEvent::QMoveEvent(const QPoint &pos, const QPoint &oldPos)
    : QEvent(Move), p(pos), oldp(oldPos)
{}

/*!
  \internal
*/
QMoveEvent::~QMoveEvent()
{
}

/*!
    \fn const QPoint &QMoveEvent::pos() const

    Returns the new position of the widget. This excludes the window
    frame for top level widgets.
*/

/*!
    \fn const QPoint &QMoveEvent::oldPos() const

    Returns the old position of the widget.
*/

/*!
    \class QExposeEvent
    \since 5.0
    \brief The QExposeEvent class contains event parameters for expose events.
    \inmodule QtGui

    \ingroup events

    Expose events are sent to windows when an area of the window is invalidated
    or window visibility in the windowing system changes.

    The event handler QWindow::exposeEvent() receives expose events.
*/

/*!
    Constructs an expose event for the given \a exposeRegion.
*/
QExposeEvent::QExposeEvent(const QRegion &exposeRegion)
    : QEvent(Expose)
    , rgn(exposeRegion)
{
}

/*!
  \internal
*/
QExposeEvent::~QExposeEvent()
{
}

/*!
    \fn const QRegion &QExposeEvent::region() const

    Returns the window area that has been exposed.
*/

/*!
    \class QResizeEvent
    \brief The QResizeEvent class contains event parameters for resize events.
    \inmodule QtGui

    \ingroup events

    Resize events are sent to widgets that have been resized.

    The event handler QWidget::resizeEvent() receives resize events.

    \sa QWidget::resize(), QWidget::setGeometry()
*/

/*!
    Constructs a resize event with the new and old widget sizes, \a
    size and \a oldSize respectively.
*/
QResizeEvent::QResizeEvent(const QSize &size, const QSize &oldSize)
    : QEvent(Resize), s(size), olds(oldSize)
{}

/*!
  \internal
*/
QResizeEvent::~QResizeEvent()
{
}

/*!
    \fn const QSize &QResizeEvent::size() const

    Returns the new size of the widget. This is the same as
    QWidget::size().
*/

/*!
    \fn const QSize &QResizeEvent::oldSize() const

    Returns the old size of the widget.
*/


/*!
    \class QCloseEvent
    \brief The QCloseEvent class contains parameters that describe a close event.

    \ingroup events
    \inmodule QtGui

    Close events are sent to widgets that the user wants to close,
    usually by choosing "Close" from the window menu, or by clicking
    the \uicontrol{X} title bar button. They are also sent when you call
    QWidget::close() to close a widget programmatically.

    Close events contain a flag that indicates whether the receiver
    wants the widget to be closed or not. When a widget accepts the
    close event, it is hidden (and destroyed if it was created with
    the Qt::WA_DeleteOnClose flag). If it refuses to accept the close
    event nothing happens. (Under X11 it is possible that the window
    manager will forcibly close the window; but at the time of writing
    we are not aware of any window manager that does this.)

    The event handler QWidget::closeEvent() receives close events. The
    default implementation of this event handler accepts the close
    event. If you do not want your widget to be hidden, or want some
    special handing, you should reimplement the event handler and
    ignore() the event.

    The \l{mainwindows/application#close event handler}{closeEvent() in the
    Application example} shows a close event handler that
    asks whether to save a document before closing.

    If you want the widget to be deleted when it is closed, create it
    with the Qt::WA_DeleteOnClose flag. This is very useful for
    independent top-level windows in a multi-window application.

    \l{QObject}s emits the \l{QObject::destroyed()}{destroyed()}
    signal when they are deleted.

    If the last top-level window is closed, the
    QGuiApplication::lastWindowClosed() signal is emitted.

    The isAccepted() function returns \c true if the event's receiver has
    agreed to close the widget; call accept() to agree to close the
    widget and call ignore() if the receiver of this event does not
    want the widget to be closed.

    \sa QWidget::close(), QWidget::hide(), QObject::destroyed(),
        QCoreApplication::exec(), QCoreApplication::quit(),
        QGuiApplication::lastWindowClosed()
*/

/*!
    Constructs a close event object.

    \sa accept()
*/
QCloseEvent::QCloseEvent()
    : QEvent(Close)
{}

/*! \internal
*/
QCloseEvent::~QCloseEvent()
{
}

/*!
   \class QIconDragEvent
   \brief The QIconDragEvent class indicates that a main icon drag has begun.
    \inmodule QtGui

   \ingroup events

   Icon drag events are sent to widgets when the main icon of a window
   has been dragged away. On Mac OS X, this happens when the proxy
   icon of a window is dragged off the title bar.

   It is normal to begin using drag and drop in response to this
   event.

   \sa {Drag and Drop}, QMimeData, QDrag
*/

/*!
    Constructs an icon drag event object with the accept flag set to
    false.

    \sa accept()
*/
QIconDragEvent::QIconDragEvent()
    : QEvent(IconDrag)
{ ignore(); }

/*! \internal */
QIconDragEvent::~QIconDragEvent()
{
}

/*!
    \class QContextMenuEvent
    \brief The QContextMenuEvent class contains parameters that describe a context menu event.
    \inmodule QtGui

    \ingroup events

    Context menu events are sent to widgets when a user performs
    an action associated with opening a context menu.
    The actions required to open context menus vary between platforms;
    for example, on Windows, pressing the menu button or clicking the
    right mouse button will cause this event to be sent.

    When this event occurs it is customary to show a QMenu with a
    context menu, if this is relevant to the context.

    Context menu events contain a special accept flag that indicates
    whether the receiver accepted the event. If the event handler does
    not accept the event then, if possible, whatever triggered the event will be
    handled as a regular input event.
*/

#ifndef QT_NO_CONTEXTMENU
/*!
    Constructs a context menu event object with the accept parameter
    flag set to false.

    The \a reason parameter must be QContextMenuEvent::Mouse or
    QContextMenuEvent::Keyboard.

    The \a pos parameter specifies the mouse position relative to the
    receiving widget. \a globalPos is the mouse position in absolute
    coordinates.
*/
QContextMenuEvent::QContextMenuEvent(Reason reason, const QPoint &pos, const QPoint &globalPos)
    : QInputEvent(ContextMenu), p(pos), gp(globalPos), reas(reason)
{}

/*!
    Constructs a context menu event object with the accept parameter
    flag set to false.

    The \a reason parameter must be QContextMenuEvent::Mouse or
    QContextMenuEvent::Keyboard.

    The \a pos parameter specifies the mouse position relative to the
    receiving widget. \a globalPos is the mouse position in absolute
    coordinates. The \a modifiers holds the keyboard modifiers.
*/
QContextMenuEvent::QContextMenuEvent(Reason reason, const QPoint &pos, const QPoint &globalPos,
                                     Qt::KeyboardModifiers modifiers)
    : QInputEvent(ContextMenu, modifiers), p(pos), gp(globalPos), reas(reason)
{}


/*! \internal */
QContextMenuEvent::~QContextMenuEvent()
{
}
/*!
    Constructs a context menu event object with the accept parameter
    flag set to false.

    The \a reason parameter must be QContextMenuEvent::Mouse or
    QContextMenuEvent::Keyboard.

    The \a pos parameter specifies the mouse position relative to the
    receiving widget.

    The globalPos() is initialized to QCursor::pos(), which may not be
    appropriate. Use the other constructor to specify the global
    position explicitly.
*/
QContextMenuEvent::QContextMenuEvent(Reason reason, const QPoint &pos)
    : QInputEvent(ContextMenu), p(pos), reas(reason)
{
    gp = QCursor::pos();
}

/*!
    \fn const QPoint &QContextMenuEvent::pos() const

    Returns the position of the mouse pointer relative to the widget
    that received the event.

    \sa x(), y(), globalPos()
*/

/*!
    \fn int QContextMenuEvent::x() const

    Returns the x position of the mouse pointer, relative to the
    widget that received the event.

    \sa y(), pos()
*/

/*!
    \fn int QContextMenuEvent::y() const

    Returns the y position of the mouse pointer, relative to the
    widget that received the event.

    \sa x(), pos()
*/

/*!
    \fn const QPoint &QContextMenuEvent::globalPos() const

    Returns the global position of the mouse pointer at the time of
    the event.

    \sa x(), y(), pos()
*/

/*!
    \fn int QContextMenuEvent::globalX() const

    Returns the global x position of the mouse pointer at the time of
    the event.

    \sa globalY(), globalPos()
*/

/*!
    \fn int QContextMenuEvent::globalY() const

    Returns the global y position of the mouse pointer at the time of
    the event.

    \sa globalX(), globalPos()
*/
#endif // QT_NO_CONTEXTMENU

/*!
    \enum QContextMenuEvent::Reason

    This enum describes the reason why the event was sent.

    \value Mouse The mouse caused the event to be sent. Normally this
    means the right mouse button was clicked, but this is platform
    dependent.

    \value Keyboard The keyboard caused this event to be sent. On
    Windows, this means the menu button was pressed.

    \value Other The event was sent by some other means (i.e. not by
    the mouse or keyboard).
*/


/*!
    \fn QContextMenuEvent::Reason QContextMenuEvent::reason() const

    Returns the reason for this context event.
*/


/*!
    \class QInputMethodEvent
    \brief The QInputMethodEvent class provides parameters for input method events.
    \inmodule QtGui

    \ingroup events

    Input method events are sent to widgets when an input method is
    used to enter text into a widget. Input methods are widely used
    to enter text for languages with non-Latin alphabets.

    Note that when creating custom text editing widgets, the
    Qt::WA_InputMethodEnabled window attribute must be set explicitly
    (using the QWidget::setAttribute() function) in order to receive
    input method events.

    The events are of interest to authors of keyboard entry widgets
    who want to be able to correctly handle languages with complex
    character input. Text input in such languages is usually a three
    step process:

    \list 1
    \li \b{Starting to Compose}

       When the user presses the first key on a keyboard, an input
       context is created. This input context will contain a string
       of the typed characters.

    \li \b{Composing}

       With every new key pressed, the input method will try to create a
       matching string for the text typed so far called preedit
       string. While the input context is active, the user can only move
       the cursor inside the string belonging to this input context.

    \li \b{Completing}

       At some point, the user will activate a user interface component
       (perhaps using a particular key) where they can choose from a
       number of strings matching the text they have typed so far. The
       user can either confirm their choice cancel the input; in either
       case the input context will be closed.
    \endlist

    QInputMethodEvent models these three stages, and transfers the
    information needed to correctly render the intermediate result. A
    QInputMethodEvent has two main parameters: preeditString() and
    commitString(). The preeditString() parameter gives the currently
    active preedit string. The commitString() parameter gives a text
    that should get added to (or replace parts of) the text of the
    editor widget. It usually is a result of the input operations and
    has to be inserted to the widgets text directly before the preedit
    string.

    If the commitString() should replace parts of the of the text in
    the editor, replacementLength() will contain the number of
    characters to be replaced. replacementStart() contains the position
    at which characters are to be replaced relative from the start of
    the preedit string.

    A number of attributes control the visual appearance of the
    preedit string (the visual appearance of text outside the preedit
    string is controlled by the widget only). The AttributeType enum
    describes the different attributes that can be set.

    A class implementing QWidget::inputMethodEvent() or
    QGraphicsItem::inputMethodEvent() should at least understand and
    honor the \l TextFormat and \l Cursor attributes.

    Since input methods need to be able to query certain properties
    from the widget or graphics item, subclasses must also implement
    QWidget::inputMethodQuery() and QGraphicsItem::inputMethodQuery(),
    respectively.

    When receiving an input method event, the text widget has to performs the
    following steps:

    \list 1
    \li If the widget has selected text, the selected text should get
       removed.

    \li Remove the text starting at replacementStart() with length
       replacementLength() and replace it by the commitString(). If
       replacementLength() is 0, replacementStart() gives the insertion
       position for the commitString().

       When doing replacement the area of the preedit
       string is ignored, thus a replacement starting at -1 with a length
       of 2 will remove the last character before the preedit string and
       the first character afterwards, and insert the commit string
       directly before the preedit string.

       If the widget implements undo/redo, this operation gets added to
       the undo stack.

    \li If there is no current preedit string, insert the
       preeditString() at the current cursor position; otherwise replace
       the previous preeditString with the one received from this event.

       If the widget implements undo/redo, the preeditString() should not
       influence the undo/redo stack in any way.

       The widget should examine the list of attributes to apply to the
       preedit string. It has to understand at least the TextFormat and
       Cursor attributes and render them as specified.
    \endlist

    \sa QInputMethod
*/

/*!
    \enum QInputMethodEvent::AttributeType

    \value TextFormat
    A QTextCharFormat for the part of the preedit string specified by
    start and length. value contains a QVariant of type QTextFormat
    specifying rendering of this part of the preedit string. There
    should be at most one format for every part of the preedit
    string. If several are specified for any character in the string the
    behaviour is undefined. A conforming implementation has to at least
    honor the backgroundColor, textColor and fontUnderline properties
    of the format.

    \value Cursor If set, a cursor should be shown inside the preedit
    string at position start. The length variable determines whether
    the cursor is visible or not. If the length is 0 the cursor is
    invisible. If value is a QVariant of type QColor this color will
    be used for rendering the cursor, otherwise the color of the
    surrounding text will be used. There should be at most one Cursor
    attribute per event. If several are specified the behaviour is
    undefined.

    \value Language
    The variant contains a QLocale object specifying the language of a
    certain part of the preedit string. There should be at most one
    language set for every part of the preedit string. If several are
    specified for any character in the string the behavior is undefined.

    \value Ruby
    The ruby text for a part of the preedit string. There should be at
    most one ruby text set for every part of the preedit string. If
    several are specified for any character in the string the behaviour
    is undefined.

    \value Selection
    If set, the edit cursor should be moved to the specified position
    in the editor text contents. In contrast with \c Cursor, this
    attribute does not work on the preedit text, but on the surrounding
    text. The cursor will be moved after the commit string has been
    committed, and the preedit string will be located at the new edit
    position.
    The start position specifies the new position and the length
    variable can be used to set a selection starting from that point.
    The value is unused.

    \sa Attribute
*/

/*!
    \class QInputMethodEvent::Attribute
    \inmodule QtGui
    \brief The QInputMethodEvent::Attribute class stores an input method attribute.
*/

/*!
    \fn QInputMethodEvent::Attribute::Attribute(AttributeType type, int start, int length, QVariant value)

    Constructs an input method attribute. \a type specifies the type
    of attribute, \a start and \a length the position of the
    attribute, and \a value the value of the attribute.
*/

/*!
    Constructs an event of type QEvent::InputMethod. The
    attributes(), preeditString(), commitString(), replacementStart(),
    and replacementLength() are initialized to default values.

    \sa setCommitString()
*/
QInputMethodEvent::QInputMethodEvent()
    : QEvent(QEvent::InputMethod), replace_from(0), replace_length(0)
{
}

/*!
    Constructs an event of type QEvent::InputMethod. The
    preedit text is set to \a preeditText, the attributes to
    \a attributes.

    The commitString(), replacementStart(), and replacementLength()
    values can be set using setCommitString().

    \sa preeditString(), attributes()
*/
QInputMethodEvent::QInputMethodEvent(const QString &preeditText, const QList<Attribute> &attributes)
    : QEvent(QEvent::InputMethod), preedit(preeditText), attrs(attributes),
      replace_from(0), replace_length(0)
{
}

/*!
    Constructs a copy of \a other.
*/
QInputMethodEvent::QInputMethodEvent(const QInputMethodEvent &other)
    : QEvent(QEvent::InputMethod), preedit(other.preedit), attrs(other.attrs),
      commit(other.commit), replace_from(other.replace_from), replace_length(other.replace_length)
{
}

/*!
    Sets the commit string to \a commitString.

    The commit string is the text that should get added to (or
    replace parts of) the text of the editor widget. It usually is a
    result of the input operations and has to be inserted to the
    widgets text directly before the preedit string.

    If the commit string should replace parts of the of the text in
    the editor, \a replaceLength specifies the number of
    characters to be replaced. \a replaceFrom specifies the position
    at which characters are to be replaced relative from the start of
    the preedit string.

    \sa commitString(), replacementStart(), replacementLength()
*/
void QInputMethodEvent::setCommitString(const QString &commitString, int replaceFrom, int replaceLength)
{
    commit = commitString;
    replace_from = replaceFrom;
    replace_length = replaceLength;
}

/*!
    \fn const QList<Attribute> &QInputMethodEvent::attributes() const

    Returns the list of attributes passed to the QInputMethodEvent
    constructor. The attributes control the visual appearance of the
    preedit string (the visual appearance of text outside the preedit
    string is controlled by the widget only).

    \sa preeditString(), Attribute
*/

/*!
    \fn const QString &QInputMethodEvent::preeditString() const

    Returns the preedit text, i.e. the text before the user started
    editing it.

    \sa commitString(), attributes()
*/

/*!
    \fn const QString &QInputMethodEvent::commitString() const

    Returns the text that should get added to (or replace parts of)
    the text of the editor widget. It usually is a result of the
    input operations and has to be inserted to the widgets text
    directly before the preedit string.

    \sa setCommitString(), preeditString(), replacementStart(), replacementLength()
*/

/*!
    \fn int QInputMethodEvent::replacementStart() const

    Returns the position at which characters are to be replaced relative
    from the start of the preedit string.

    \sa replacementLength(), setCommitString()
*/

/*!
    \fn int QInputMethodEvent::replacementLength() const

    Returns the number of characters to be replaced in the preedit
    string.

    \sa replacementStart(), setCommitString()
*/

/*!
    \class QInputMethodQueryEvent
    \since 5.0
    \inmodule QtGui

    \brief The QInputMethodQueryEvent class provides an event sent by the input context to input objects.

    It is used by the
    input method to query a set of properties of the object to be
    able to support complex input method operations as support for
    surrounding text and reconversions.

    queries() specifies which properties are queried.

    The object should call setValue() on the event to fill in the requested
    data before calling accept().
*/

/*!
    \fn Qt::InputMethodQueries QInputMethodQueryEvent::queries() const

    Returns the properties queried by the event.
 */

/*!
    Constructs a query event for properties given by \a queries.
 */
QInputMethodQueryEvent::QInputMethodQueryEvent(Qt::InputMethodQueries queries)
    : QEvent(InputMethodQuery),
      m_queries(queries)
{
}

/*!
    \internal
 */
QInputMethodQueryEvent::~QInputMethodQueryEvent()
{
}

/*!
    Sets property \a query to \a value.
 */
void QInputMethodQueryEvent::setValue(Qt::InputMethodQuery query, const QVariant &value)
{
    for (int i = 0; i < m_values.size(); ++i) {
        if (m_values.at(i).query == query) {
            m_values[i].value = value;
            return;
        }
    }
    QueryPair pair = { query, value };
    m_values.append(pair);
}

/*!
    Returns value of the property \a query.
 */
QVariant QInputMethodQueryEvent::value(Qt::InputMethodQuery query) const
{
    for (int i = 0; i < m_values.size(); ++i)
        if (m_values.at(i).query == query)
            return m_values.at(i).value;
    return QVariant();
}

#ifndef QT_NO_TABLETEVENT

/*!
    \class QTabletEvent
    \brief The QTabletEvent class contains parameters that describe a Tablet event.
    \inmodule QtGui

    \ingroup events

    Tablet Events are generated from a Wacom tablet. Most of the time you will
    want to deal with events from the tablet as if they were events from a
    mouse; for example, you would retrieve the cursor position with x(), y(),
    pos(), globalX(), globalY(), and globalPos(). In some situations you may
    wish to retrieve the extra information provided by the tablet device
    driver; for example, you might want to do subpixeling with higher
    resolution coordinates or you may want to adjust color brightness based on
    pressure.  QTabletEvent allows you to read the pressure(), the xTilt(), and
    yTilt(), as well as the type of device being used with device() (see
    \l{TabletDevice}). It can also give you the minimum and maximum values for
    each device's pressure and high resolution coordinates.

    A tablet event contains a special accept flag that indicates whether the
    receiver wants the event. You should call QTabletEvent::accept() if you
    handle the tablet event; otherwise it will be sent to the parent widget.
    The exception are TabletEnterProximity and TabletLeaveProximity events,
    these are only sent to QApplication and don't check whether or not they are
    accepted.

    The QWidget::setEnabled() function can be used to enable or
    disable mouse and keyboard events for a widget.

    The event handler QWidget::tabletEvent() receives all three types of
    tablet events. Qt will first send a tabletEvent then, if it is not
    accepted, it will send a mouse event. This allows applications that
    don't utilize tablets to use a tablet like a mouse, while also
    enabling those who want to use both tablets and mouses differently.

    \section1 Notes for X11 Users

    Qt uses the following hard-coded names to identify tablet
    devices from the xorg.conf file on X11 (apart from IRIX):
    'stylus', 'pen', and 'eraser'. If the devices have other names,
    they will not be picked up Qt.
*/

/*!
    \enum QTabletEvent::TabletDevice

    This enum defines what type of device is generating the event.

    \value NoDevice    No device, or an unknown device.
    \value Puck    A Puck (a device that is similar to a flat mouse with
    a transparent circle with cross-hairs).
    \value Stylus  A Stylus.
    \value Airbrush An airbrush
    \value FourDMouse A 4D Mouse.
    \value RotationStylus A special stylus that also knows about rotation
           (a 6D stylus). \since 4.1
    \omitvalue XFreeEraser
*/

/*!
    \enum QTabletEvent::PointerType

    This enum defines what type of point is generating the event.

    \value UnknownPointer    An unknown device.
    \value Pen    Tip end of a stylus-like device (the narrow end of the pen).
    \value Cursor  Any puck-like device.
    \value Eraser  Eraser end of a stylus-like device (the broad end of the pen).

    \sa pointerType()
*/

/*!
  Construct a tablet event of the given \a type.

  The \a pos parameter indicates where the event occurred in the
  widget; \a globalPos is the corresponding position in absolute
  coordinates.

  \a pressure contains the pressure exerted on the \a device.

  \a pointerType describes the type of pen that is being used.

  \a xTilt and \a yTilt contain the device's degree of tilt from the
  x and y axes respectively.

  \a keyState specifies which keyboard modifiers are pressed (e.g.,
  \uicontrol{Ctrl}).

  The \a uniqueID parameter contains the unique ID for the current device.

  The \a z parameter contains the coordinate of the device on the tablet, this
  is usually given by a wheel on 4D mouse. If the device does not support a
  Z-axis, pass zero here.

  The \a tangentialPressure parameter contins the tangential pressure of an air
  brush. If the device does not support tangential pressure, pass 0 here.

  \a rotation contains the device's rotation in degrees. 4D mice support
  rotation. If the device does not support rotation, pass 0 here.

  \sa pos(), globalPos(), device(), pressure(), xTilt(), yTilt(), uniqueId(), rotation(),
      tangentialPressure(), z()
*/

QTabletEvent::QTabletEvent(Type type, const QPointF &pos, const QPointF &globalPos,
                           int device, int pointerType,
                           qreal pressure, int xTilt, int yTilt, qreal tangentialPressure,
                           qreal rotation, int z, Qt::KeyboardModifiers keyState, qint64 uniqueID)
    : QInputEvent(type, keyState),
      mPos(pos),
      mGPos(globalPos),
      mDev(device),
      mPointerType(pointerType),
      mXT(xTilt),
      mYT(yTilt),
      mZ(z),
      mPress(pressure),
      mTangential(tangentialPressure),
      mRot(rotation),
      mUnique(uniqueID),
      mExtra(0)
{
}

/*!
    \internal
*/
QTabletEvent::~QTabletEvent()
{
}

/*!
    \fn TabletDevices QTabletEvent::device() const

    Returns the type of device that generated the event.

    \sa TabletDevice
*/

/*!
    \fn PointerType QTabletEvent::pointerType() const

    Returns the type of point that generated the event.
*/

/*!
    \fn qreal QTabletEvent::tangentialPressure() const

    Returns the tangential pressure for the device.  This is typically given by a finger
    wheel on an airbrush tool.  The range is from -1.0 to 1.0. 0.0 indicates a
    neutral position.  Current airbrushes can only move in the positive
    direction from the neutrual position. If the device does not support
    tangential pressure, this value is always 0.0.

    \sa pressure()
*/

/*!
    \fn qreal QTabletEvent::rotation() const

    Returns the rotation of the current device in degress. This is usually
    given by a 4D Mouse. If the device doesn't support rotation this value is
    always 0.0.

*/

/*!
    \fn qreal QTabletEvent::pressure() const

    Returns the pressure for the device. 0.0 indicates that the stylus is not
    on the tablet, 1.0 indicates the maximum amount of pressure for the stylus.

    \sa tangentialPressure()
*/

/*!
    \fn int QTabletEvent::xTilt() const

    Returns the angle between the device (a pen, for example) and the
    perpendicular in the direction of the x axis.
    Positive values are towards the tablet's physical right. The angle
    is in the range -60 to +60 degrees.

    \image qtabletevent-tilt.png

    \sa yTilt()
*/

/*!
    \fn int QTabletEvent::yTilt() const

    Returns the angle between the device (a pen, for example) and the
    perpendicular in the direction of the y axis.
    Positive values are towards the bottom of the tablet. The angle is
    within the range -60 to +60 degrees.

    \sa xTilt()
*/

/*!
    \fn QPoint QTabletEvent::pos() const

    Returns the position of the device, relative to the widget that
    received the event.

    If you move widgets around in response to mouse events, use
    globalPos() instead of this function.

    \sa x(), y(), globalPos()
*/

/*!
    \fn int QTabletEvent::x() const

    Returns the x position of the device, relative to the widget that
    received the event.

    \sa y(), pos()
*/

/*!
    \fn int QTabletEvent::y() const

    Returns the y position of the device, relative to the widget that
    received the event.

    \sa x(), pos()
*/

/*!
    \fn int QTabletEvent::z() const

    Returns the z position of the device. Typically this is represented by a
    wheel on a 4D Mouse. If the device does not support a Z-axis, this value is
    always zero. This is \b not the same as pressure.

    \sa pressure()
*/

/*!
    \fn QPoint QTabletEvent::globalPos() const

    Returns the global position of the device \e{at the time of the
    event}. This is important on asynchronous windows systems like X11;
    whenever you move your widgets around in response to mouse events,
    globalPos() can differ significantly from the current position
    QCursor::pos().

    \sa globalX(), globalY(), hiResGlobalPos()
*/

/*!
    \fn int QTabletEvent::globalX() const

    Returns the global x position of the mouse pointer at the time of
    the event.

    \sa globalY(), globalPos(), hiResGlobalX()
*/

/*!
    \fn int QTabletEvent::globalY() const

    Returns the global y position of the tablet device at the time of
    the event.

    \sa globalX(), globalPos(), hiResGlobalY()
*/

/*!
    \fn qint64 QTabletEvent::uniqueId() const

    Returns a unique ID for the current device, making it possible
    to differentiate between multiple devices being used at the same
    time on the tablet.

    Support of this feature is dependent on the tablet.

    Values for the same device may vary from OS to OS.

    Later versions of the Wacom driver for Linux will now report
    the ID information. If you have a tablet that supports unique ID
    and are not getting the information on Linux, consider upgrading
    your driver.

    As of Qt 4.2, the unique ID is the same regardless of the orientation
    of the pen. Earlier versions would report a different value when using
    the eraser-end versus the pen-end of the stylus on some OS's.

    \sa pointerType()
*/

/*!
    \fn const QPointF &QTabletEvent::hiResGlobalPos() const

    The high precision coordinates delivered from the tablet expressed.
    Sub pixeling information is in the fractional part of the QPointF.

    \sa globalPos(), hiResGlobalX(), hiResGlobalY()
*/

/*!
    \fn qreal &QTabletEvent::hiResGlobalX() const

    The high precision x position of the tablet device.
*/

/*!
    \fn qreal &QTabletEvent::hiResGlobalY() const

    The high precision y position of the tablet device.
*/

/*!
    \fn const QPointF &QTabletEvent::posF() const

    Returns the position of the device, relative to the widget that
    received the event.

    If you move widgets around in response to mouse events, use
    globalPosF() instead of this function.

    \sa globalPosF()
*/

/*!
    \fn const QPointF &QTabletEvent::globalPosF() const

    Returns the global position of the device \e{at the time of the
    event}. This is important on asynchronous windows systems like X11;
    whenever you move your widgets around in response to mouse events,
    globalPosF() can differ significantly from the current position
    QCursor::pos().

    \sa posF()
*/

#endif // QT_NO_TABLETEVENT

#ifndef QT_NO_GESTURES
/*!
    \class QNativeGestureEvent
    \since 5.2
    \brief The QNativeGestureEvent class contains parameters that describe a gesture event.
    \inmodule QtGui
    \ingroup events

    Native gesture events are generated by the operating system, typically by
    interpreting touch events. Gesture events are high-level events such
    as zoom or rotate.

    \table
    \header
        \li Event Type
        \li Description
        \li Touch equence
    \row
        \li Qt::ZoomNativeGesture
        \li Magnification delta in percent.
        \li OS X: Two-finger pinch.
    \row
        \li Qt::SmartZoomNativeGesture
        \li Boolean magnification state.
        \li OS X: Two-finger douple tap (trackpad) / One-finger douple tap (magic mouse).
    \row
        \li Qt::RotateNativeGesture
        \li Rotation delta in degrees.
        \li OS X: Two-finger rotate.
    \endtable


    In addition, BeginNativeGesture and EndNativeGesture are sent before and after
    gesture event streams:

        BeginNativeGesture
        ZoomNativeGesture
        ZoomNativeGesture
        ZoomNativeGesture
        EndNativeGesture

    \sa Qt::NativeGestureType, QGestureEvent
*/

/*!
    Constructs a native gesture event of type \a type.

    The points \a localPos, \a windowPos and \a screenPos specify the
    gesture position relative to the receiving widget or item,
    window, and screen, respectively.

    \a realValue is the OS X event parameter, \a sequenceId and \a intValue are the Windows event parameters.
*/
QNativeGestureEvent::QNativeGestureEvent(Qt::NativeGestureType type, const QPointF &localPos, const QPointF &windowPos,
                                         const QPointF &screenPos, qreal realValue, ulong sequenceId, quint64 intValue)
    : QInputEvent(QEvent::NativeGesture), mGestureType(type),
      mLocalPos(localPos), mWindowPos(windowPos), mScreenPos(screenPos), mRealValue(realValue),
      mSequenceId(sequenceId), mIntValue(intValue)
{ }

/*!
    \fn QNativeGestureEvent::gestureType() const
    \since 5.2

    Returns the gesture type.
*/

/*!
    \fn QNativeGestureEvent::value() const
    \since 5.2

    Returns the gesture value. The value should be interpreted based on the
    gesture type. For example, a Zoom gesture provides a scale factor while a Rotate
    gesture provides a rotation delta.

    \sa QNativeGestureEvent, gestureType()
*/

/*!
    \fn QPoint QNativeGestureEvent::globalPos() const
    \since 5.2

    Returns the position of the gesture as a QPointF in screen coordinates
*/

/*!
    \fn QPoint QNativeGestureEvent::pos() const
    \since 5.2

    Returns the position of the mouse cursor, relative to the widget
    or item that received the event.
*/

/*!
    \fn QPointF QNativeGestureEvent::localPos() const
    \since 5.2

    Returns the position of the gesture as a QPointF, relative to the
    widget or item that received the event.
*/

/*!
    \fn QPointF QNativeGestureEvent::screenPos() const
    \since 5.2

    Returns the position of the gesture as a QPointF in screen coordinates.
*/

/*!
    \fn QPointF QNativeGestureEvent::windowPos() const
    \since 5.2

    Returns the position of the gesture as a QPointF, relative to the
    window that received the event.
*/
#endif // QT_NO_GESTURES

#ifndef QT_NO_DRAGANDDROP
/*!
    Creates a QDragMoveEvent of the required \a type indicating
    that the mouse is at position \a pos given within a widget.

    The mouse and keyboard states are specified by \a buttons and
    \a modifiers, and the \a actions describe the types of drag
    and drop operation that are possible.
    The drag data is passed as MIME-encoded information in \a data.

    \warning Do not attempt to create a QDragMoveEvent yourself.
    These objects rely on Qt's internal state.
*/
QDragMoveEvent::QDragMoveEvent(const QPoint& pos, Qt::DropActions actions, const QMimeData *data,
                               Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers, Type type)
    : QDropEvent(pos, actions, data, buttons, modifiers, type)
    , rect(pos, QSize(1, 1))
{}

/*!
    Destroys the event.
*/
QDragMoveEvent::~QDragMoveEvent()
{
}

/*!
    \fn void QDragMoveEvent::accept(const QRect &rectangle)

    The same as accept(), but also notifies that future moves will
    also be acceptable if they remain within the \a rectangle
    given on the widget. This can improve performance, but may
    also be ignored by the underlying system.

    If the rectangle is empty, drag move events will be sent
    continuously. This is useful if the source is scrolling in a
    timer event.
*/

/*!
    \fn void QDragMoveEvent::accept()

    \overload

    Calls QDropEvent::accept().
*/

/*!
    \fn void QDragMoveEvent::ignore()

    \overload

    Calls QDropEvent::ignore().
*/

/*!
    \fn void QDragMoveEvent::ignore(const QRect &rectangle)

    The opposite of the accept(const QRect&) function.
    Moves within the \a rectangle are not acceptable, and will be
    ignored.
*/

/*!
    \fn QRect QDragMoveEvent::answerRect() const

    Returns the rectangle in the widget where the drop will occur if accepted.
    You can use this information to restrict drops to certain places on the
    widget.
*/


/*!
    \class QDropEvent
    \ingroup events
    \ingroup draganddrop
    \inmodule QtGui

    \brief The QDropEvent class provides an event which is sent when a
    drag and drop action is completed.

    When a widget \l{QWidget::setAcceptDrops()}{accepts drop events}, it will
    receive this event if it has accepted the most recent QDragEnterEvent or
    QDragMoveEvent sent to it.

    The drop event contains a proposed action, available from proposedAction(), for
    the widget to either accept or ignore. If the action can be handled by the
    widget, you should call the acceptProposedAction() function. Since the
    proposed action can be a combination of \l Qt::DropAction values, it may be
    useful to either select one of these values as a default action or ask
    the user to select their preferred action.

    If the proposed drop action is not suitable, perhaps because your custom
    widget does not support that action, you can replace it with any of the
    \l{possibleActions()}{possible drop actions} by calling setDropAction()
    with your preferred action. If you set a value that is not present in the
    bitwise OR combination of values returned by possibleActions(), the default
    copy action will be used. Once a replacement drop action has been set, call
    accept() instead of acceptProposedAction() to complete the drop operation.

    The mimeData() function provides the data dropped on the widget in a QMimeData
    object. This contains information about the MIME type of the data in addition to
    the data itself.

    \sa QMimeData, QDrag, {Drag and Drop}
*/

/*!
    \fn const QMimeData *QDropEvent::mimeData() const

    Returns the data that was dropped on the widget and its associated MIME
    type information.
*/

/*!
    Constructs a drop event of a certain \a type corresponding to a
    drop at the point specified by \a pos in the destination widget's
    coordinate system.

    The \a actions indicate which types of drag and drop operation can
    be performed, and the drag data is stored as MIME-encoded data in \a data.

    The states of the mouse buttons and keyboard modifiers at the time of
    the drop are specified by \a buttons and \a modifiers.
*/ // ### pos is in which coordinate system?
QDropEvent::QDropEvent(const QPointF& pos, Qt::DropActions actions, const QMimeData *data,
                       Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers, Type type)
    : QEvent(type), p(pos), mouseState(buttons),
      modState(modifiers), act(actions),
      mdata(data)
{
    default_action = QGuiApplicationPrivate::platformIntegration()->drag()->defaultAction(act, modifiers);
    drop_action = default_action;
    ignore();
}

/*! \internal */
QDropEvent::~QDropEvent()
{
}


/*!
    If the source of the drag operation is a widget in this
    application, this function returns that source; otherwise it
    returns 0. The source of the operation is the first parameter to
    the QDrag object used instantiate the drag.

    This is useful if your widget needs special behavior when dragging
    to itself.

    \sa QDrag::QDrag()
*/
QObject* QDropEvent::source() const
{
    if (const QDragManager *manager = QDragManager::self())
        return manager->source();
    return 0;
}


void QDropEvent::setDropAction(Qt::DropAction action)
{
    if (!(action & act) && action != Qt::IgnoreAction)
        action = default_action;
    drop_action = action;
}

/*!
    \fn QPoint QDropEvent::pos() const

    Returns the position where the drop was made.
*/

/*!
    \fn const QPointF& QDropEvent::posF() const

    Returns the position where the drop was made.
*/

/*!
    \fn Qt::MouseButtons QDropEvent::mouseButtons() const

    Returns the mouse buttons that are pressed..
*/

/*!
    \fn Qt::KeyboardModifiers QDropEvent::keyboardModifiers() const

    Returns the modifier keys that are pressed.
*/

/*!
    \fn void QDropEvent::setDropAction(Qt::DropAction action)

    Sets the \a action to be performed on the data by the target.
    Use this to override the \l{proposedAction()}{proposed action}
    with one of the \l{possibleActions()}{possible actions}.

    If you set a drop action that is not one of the possible actions, the
    drag and drop operation will default to a copy operation.

    Once you have supplied a replacement drop action, call accept()
    instead of acceptProposedAction().

    \sa dropAction()
*/

/*!
    \fn Qt::DropAction QDropEvent::dropAction() const

    Returns the action to be performed on the data by the target. This may be
    different from the action supplied in proposedAction() if you have called
    setDropAction() to explicitly choose a drop action.

    \sa setDropAction()
*/

/*!
    \fn Qt::DropActions QDropEvent::possibleActions() const

    Returns an OR-combination of possible drop actions.

    \sa dropAction()
*/

/*!
    \fn Qt::DropAction QDropEvent::proposedAction() const

    Returns the proposed drop action.

    \sa dropAction()
*/

/*!
    \fn void QDropEvent::acceptProposedAction()

    Sets the drop action to be the proposed action.

    \sa setDropAction(), proposedAction(), {QEvent::accept()}{accept()}
*/

/*!
    \class QDragEnterEvent
    \brief The QDragEnterEvent class provides an event which is sent
    to a widget when a drag and drop action enters it.

    \ingroup events
    \ingroup draganddrop
    \inmodule QtGui

    A widget must accept this event in order to receive the \l
    {QDragMoveEvent}{drag move events} that are sent while the drag
    and drop action is in progress. The drag enter event is always
    immediately followed by a drag move event.

    QDragEnterEvent inherits most of its functionality from
    QDragMoveEvent, which in turn inherits most of its functionality
    from QDropEvent.

    \sa QDragLeaveEvent, QDragMoveEvent, QDropEvent
*/

/*!
    Constructs a QDragEnterEvent that represents a drag entering a
    widget at the given \a point with mouse and keyboard states specified by
    \a buttons and \a modifiers.

    The drag data is passed as MIME-encoded information in \a data, and the
    specified \a actions describe the possible types of drag and drop
    operation that can be performed.

    \warning Do not create a QDragEnterEvent yourself since these
    objects rely on Qt's internal state.
*/
QDragEnterEvent::QDragEnterEvent(const QPoint& point, Qt::DropActions actions, const QMimeData *data,
                                 Qt::MouseButtons buttons, Qt::KeyboardModifiers modifiers)
    : QDragMoveEvent(point, actions, data, buttons, modifiers, DragEnter)
{}

/*! \internal
*/
QDragEnterEvent::~QDragEnterEvent()
{
}

/*!
    \class QDragMoveEvent
    \brief The QDragMoveEvent class provides an event which is sent while a drag and drop action is in progress.

    \ingroup events
    \ingroup draganddrop
    \inmodule QtGui

    A widget will receive drag move events repeatedly while the drag
    is within its boundaries, if it accepts
    \l{QWidget::setAcceptDrops()}{drop events} and \l
    {QWidget::dragEnterEvent()}{enter events}. The widget should
    examine the event to see what kind of \l{mimeData()}{data} it
    provides, and call the accept() function to accept the drop if appropriate.

    The rectangle supplied by the answerRect() function can be used to restrict
    drops to certain parts of the widget. For example, we can check whether the
    rectangle intersects with the geometry of a certain child widget and only
    call \l{QDropEvent::acceptProposedAction()}{acceptProposedAction()} if that
    is the case.

    Note that this class inherits most of its functionality from
    QDropEvent.

    \sa QDragEnterEvent, QDragLeaveEvent, QDropEvent
*/

/*!
    \class QDragLeaveEvent
    \brief The QDragLeaveEvent class provides an event that is sent to a widget when a drag and drop action leaves it.

    \ingroup events
    \ingroup draganddrop
    \inmodule QtGui

    This event is always preceded by a QDragEnterEvent and a series
    of \l{QDragMoveEvent}s. It is not sent if a QDropEvent is sent
    instead.

    \sa QDragEnterEvent, QDragMoveEvent, QDropEvent
*/

/*!
    Constructs a QDragLeaveEvent.

    \warning Do not create a QDragLeaveEvent yourself since these
    objects rely on Qt's internal state.
*/
QDragLeaveEvent::QDragLeaveEvent()
    : QEvent(DragLeave)
{}

/*! \internal
*/
QDragLeaveEvent::~QDragLeaveEvent()
{
}
#endif // QT_NO_DRAGANDDROP

/*!
    \class QHelpEvent
    \brief The QHelpEvent class provides an event that is used to request helpful information
    about a particular point in a widget.

    \ingroup events
    \ingroup helpsystem
    \inmodule QtGui

    This event can be intercepted in applications to provide tooltips
    or "What's This?" help for custom widgets. The type() can be
    either QEvent::ToolTip or QEvent::WhatsThis.

    \sa QToolTip, QWhatsThis, QStatusTipEvent, QWhatsThisClickedEvent
*/

/*!
    Constructs a help event with the given \a type corresponding to the
    widget-relative position specified by \a pos and the global position
    specified by \a globalPos.

    \a type must be either QEvent::ToolTip or QEvent::WhatsThis.

    \sa pos(), globalPos()
*/
QHelpEvent::QHelpEvent(Type type, const QPoint &pos, const QPoint &globalPos)
    : QEvent(type), p(pos), gp(globalPos)
{}

/*!
    \fn int QHelpEvent::x() const

    Same as pos().x().

    \sa y(), pos(), globalPos()
*/

/*!
    \fn int QHelpEvent::y() const

    Same as pos().y().

    \sa x(), pos(), globalPos()
*/

/*!
    \fn int QHelpEvent::globalX() const

    Same as globalPos().x().

    \sa x(), globalY(), globalPos()
*/

/*!
    \fn int QHelpEvent::globalY() const

    Same as globalPos().y().

    \sa y(), globalX(), globalPos()
*/

/*!
    \fn const QPoint &QHelpEvent::pos()  const

    Returns the mouse cursor position when the event was generated,
    relative to the widget to which the event is dispatched.

    \sa globalPos(), x(), y()
*/

/*!
    \fn const QPoint &QHelpEvent::globalPos() const

    Returns the mouse cursor position when the event was generated
    in global coordinates.

    \sa pos(), globalX(), globalY()
*/

/*! \internal
*/
QHelpEvent::~QHelpEvent()
{
}

#ifndef QT_NO_STATUSTIP

/*!
    \class QStatusTipEvent
    \brief The QStatusTipEvent class provides an event that is used to show messages in a status bar.

    \ingroup events
    \ingroup helpsystem
    \inmodule QtGui

    Status tips can be set on a widget using the
    QWidget::setStatusTip() function.  They are shown in the status
    bar when the mouse cursor enters the widget. For example:

    \table 100%
    \row
    \li
    \snippet qstatustipevent/main.cpp 1
    \dots
    \snippet qstatustipevent/main.cpp 3
    \li
    \image qstatustipevent-widget.png Widget with status tip.
    \endtable

    Status tips can also be set on actions using the
    QAction::setStatusTip() function:

    \table 100%
    \row
    \li
    \snippet qstatustipevent/main.cpp 0
    \snippet qstatustipevent/main.cpp 2
    \dots
    \snippet qstatustipevent/main.cpp 3
    \li
    \image qstatustipevent-action.png Action with status tip.
    \endtable

    Finally, status tips are supported for the item view classes
    through the Qt::StatusTipRole enum value.

    \sa QStatusBar, QHelpEvent, QWhatsThisClickedEvent
*/

/*!
    Constructs a status tip event with the text specified by \a tip.

    \sa tip()
*/
QStatusTipEvent::QStatusTipEvent(const QString &tip)
    : QEvent(StatusTip), s(tip)
{}

/*! \internal
*/
QStatusTipEvent::~QStatusTipEvent()
{
}

/*!
    \fn QString QStatusTipEvent::tip() const

    Returns the message to show in the status bar.

    \sa QStatusBar::showMessage()
*/

#endif // QT_NO_STATUSTIP

#ifndef QT_NO_WHATSTHIS

/*!
    \class QWhatsThisClickedEvent
    \brief The QWhatsThisClickedEvent class provides an event that
    can be used to handle hyperlinks in a "What's This?" text.

    \ingroup events
    \ingroup helpsystem
    \inmodule QtGui

    \sa QWhatsThis, QHelpEvent, QStatusTipEvent
*/

/*!
    Constructs an event containing a URL specified by \a href when a link
    is clicked in a "What's This?" message.

    \sa href()
*/
QWhatsThisClickedEvent::QWhatsThisClickedEvent(const QString &href)
    : QEvent(WhatsThisClicked), s(href)
{}

/*! \internal
*/
QWhatsThisClickedEvent::~QWhatsThisClickedEvent()
{
}

/*!
    \fn QString QWhatsThisClickedEvent::href() const

    Returns the URL that was clicked by the user in the "What's
    This?" text.
*/

#endif // QT_NO_WHATSTHIS

#ifndef QT_NO_ACTION

/*!
    \class QActionEvent
    \brief The QActionEvent class provides an event that is generated
    when a QAction is added, removed, or changed.

    \ingroup events
    \inmodule QtGui

    Actions can be added to widgets using QWidget::addAction(). This
    generates an \l ActionAdded event, which you can handle to provide
    custom behavior. For example, QToolBar reimplements
    QWidget::actionEvent() to create \l{QToolButton}s for the
    actions.

    \sa QAction, QWidget::addAction(), QWidget::removeAction(), QWidget::actions()
*/

/*!
    Constructs an action event. The \a type can be \l ActionChanged,
    \l ActionAdded, or \l ActionRemoved.

    \a action is the action that is changed, added, or removed. If \a
    type is ActionAdded, the action is to be inserted before the
    action \a before. If \a before is 0, the action is appended.
*/
QActionEvent::QActionEvent(int type, QAction *action, QAction *before)
    : QEvent(static_cast<QEvent::Type>(type)), act(action), bef(before)
{}

/*! \internal
*/
QActionEvent::~QActionEvent()
{
}

/*!
    \fn QAction *QActionEvent::action() const

    Returns the action that is changed, added, or removed.

    \sa before()
*/

/*!
    \fn QAction *QActionEvent::before() const

    If type() is \l ActionAdded, returns the action that should
    appear before action(). If this function returns 0, the action
    should be appended to already existing actions on the same
    widget.

    \sa action(), QWidget::actions()
*/

#endif // QT_NO_ACTION

/*!
    \class QHideEvent
    \brief The QHideEvent class provides an event which is sent after a widget is hidden.

    \ingroup events
    \inmodule QtGui

    This event is sent just before QWidget::hide() returns, and also
    when a top-level window has been hidden (iconified) by the user.

    If spontaneous() is true, the event originated outside the
    application. In this case, the user hid the window using the
    window manager controls, either by iconifying the window or by
    switching to another virtual desktop where the window isn't
    visible. The window will become hidden but not withdrawn. If the
    window was iconified, QWidget::isMinimized() returns \c true.

    \sa QShowEvent
*/

/*!
    Constructs a QHideEvent.
*/
QHideEvent::QHideEvent()
    : QEvent(Hide)
{}

/*! \internal
*/
QHideEvent::~QHideEvent()
{
}

/*!
    \class QShowEvent
    \brief The QShowEvent class provides an event that is sent when a widget is shown.

    \ingroup events
    \inmodule QtGui

    There are two kinds of show events: show events caused by the
    window system (spontaneous), and internal show events. Spontaneous (QEvent::spontaneous())
    show events are sent just after the window system shows the
    window; they are also sent when a top-level window is redisplayed
    after being iconified. Internal show events are delivered just
    before the widget becomes visible.

    \sa QHideEvent
*/

/*!
    Constructs a QShowEvent.
*/
QShowEvent::QShowEvent()
    : QEvent(Show)
{}

/*! \internal
*/
QShowEvent::~QShowEvent()
{
}

/*!
    \class QFileOpenEvent
    \brief The QFileOpenEvent class provides an event that will be
    sent when there is a request to open a file or a URL.

    \ingroup events
    \inmodule QtGui

    File open events will be sent to the QApplication::instance()
    when the operating system requests that a file or URL should be opened.
    This is a high-level event that can be caused by different user actions
    depending on the user's desktop environment; for example, double
    clicking on an file icon in the Finder on Mac OS X.

    This event is only used to notify the application of a request.
    It may be safely ignored.

    \note This class is currently supported for Mac OS X only.
*/

/*!
    \internal

    Constructs a file open event for the given \a file.
*/
QFileOpenEvent::QFileOpenEvent(const QString &file)
    : QEvent(FileOpen), f(file), m_url(QUrl::fromLocalFile(file))
{
}

/*!
    \internal

    Constructs a file open event for the given \a url.
*/
QFileOpenEvent::QFileOpenEvent(const QUrl &url)
    : QEvent(FileOpen), f(url.toLocalFile()), m_url(url)
{
}


/*! \internal
*/
QFileOpenEvent::~QFileOpenEvent()
{
}

/*!
    \fn QString QFileOpenEvent::file() const

    Returns the file that is being opened.
*/

/*!
    \fn QUrl QFileOpenEvent::url() const

    Returns the url that is being opened.

    \since 4.6
*/

/*!
    \fn bool QFileOpenEvent::openFile(QFile &file, QIODevice::OpenMode flags) const

    Opens a QFile on the \a file referenced by this event in the mode specified
    by \a flags. Returns \c true if successful; otherwise returns \c false.

    This is necessary as some files cannot be opened by name, but require specific
    information stored in this event.

    \since 4.8
*/
bool QFileOpenEvent::openFile(QFile &file, QIODevice::OpenMode flags) const
{
    file.setFileName(f);
    return file.open(flags);
}

#ifndef QT_NO_TOOLBAR
/*!
    \internal
    \class QToolBarChangeEvent
    \brief The QToolBarChangeEvent class provides an event that is
    sent whenever a the toolbar button is clicked on Mac OS X.

    \ingroup events
    \inmodule QtGui

    The QToolBarChangeEvent is sent when the toolbar button is clicked. On Mac
    OS X, this is the long oblong button on the right side of the window
    title bar. The default implementation is to toggle the appearance (hidden or
    shown) of the associated toolbars for the window.
*/

/*!
    \internal

    Construct a QToolBarChangeEvent given the current button state in \a state.
*/
QToolBarChangeEvent::QToolBarChangeEvent(bool t)
    : QEvent(ToolBarChange), tog(t)
{}

/*! \internal
*/
QToolBarChangeEvent::~QToolBarChangeEvent()
{
}

/*!
    \fn bool QToolBarChangeEvent::toggle() const
    \internal
*/

/*
    \fn Qt::ButtonState QToolBarChangeEvent::state() const

    Returns the keyboard modifier flags at the time of the event.

    The returned value is a selection of the following values,
    combined using the OR operator:
    Qt::ShiftButton, Qt::ControlButton, Qt::MetaButton, and Qt::AltButton.
*/

#endif // QT_NO_TOOLBAR

#ifndef QT_NO_SHORTCUT

/*!
    Constructs a shortcut event for the given \a key press,
    associated with the QShortcut ID \a id.

    \a ambiguous specifies whether there is more than one QShortcut
    for the same key sequence.
*/
QShortcutEvent::QShortcutEvent(const QKeySequence &key, int id, bool ambiguous)
    : QEvent(Shortcut), sequence(key), ambig(ambiguous), sid(id)
{
}

/*!
    Destroys the event object.
*/
QShortcutEvent::~QShortcutEvent()
{
}

#endif // QT_NO_SHORTCUT

#ifndef QT_NO_DEBUG_STREAM

static inline void formatTouchEvent(QDebug d, const char *name, const QTouchEvent &t)
{
    d << "QTouchEvent(" << name << " states: " <<  t.touchPointStates();
    const QList<QTouchEvent::TouchPoint> points = t.touchPoints();
    const int size = points.size();
    d << ", " << size << " points: ";
    for (int i = 0; i < size; ++i) {
        if (i)
            d << ", ";
        d << points.at(i).pos() << ' ' << points.at(i).rect();
        switch (points.at(i).state()) {
        case Qt::TouchPointPressed:
            d << " pressed";
            break;
        case Qt::TouchPointReleased:
            d << " released";
            break;
        case Qt::TouchPointMoved:
            d << " moved";
            break;
        case Qt::TouchPointStationary:
            d << " stationary";
            break;
        }
    }
    d << ')';
}

QDebug operator<<(QDebug dbg, const QEvent *e) {
    // More useful event output could be added here
    if (!e)
        return dbg << "QEvent(this = 0x0)";
    const char *n = 0;
    switch (e->type()) {
    case QEvent::Timer:
        n = "Timer";
        break;
    case QEvent::MouseButtonPress:
    case QEvent::MouseMove:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    {
        const QMouseEvent *me = static_cast<const QMouseEvent*>(e);
        switch(me->type()) {
        case QEvent::MouseButtonPress:
            n = "MouseButtonPress";
            break;
        case QEvent::MouseMove:
            n = "MouseMove";
            break;
        case QEvent::MouseButtonRelease:
            n = "MouseButtonRelease";
            break;
        case QEvent::MouseButtonDblClick:
        default:
            n = "MouseButtonDblClick";
            break;
        }
        QDebug nsp = dbg.nospace();
        nsp << "QMouseEvent("  << n
                      << ", " << me->button()
                      << ", " << hex << (int)me->buttons()
                      << ", " << hex << (int)me->modifiers() << dec;
        if (const Qt::MouseEventSource source = me->source())
            nsp << ", source = " << source;
        nsp << ')';
    }
    return dbg.space();

#ifndef QT_NO_TOOLTIP
    case QEvent::ToolTip:
        n = "ToolTip";
        break;
#endif
    case QEvent::WindowActivate:
        n = "WindowActivate";
        break;
    case QEvent::WindowDeactivate:
        n = "WindowDeactivate";
        break;
    case QEvent::ActivationChange:
        n = "ActivationChange";
        break;
#ifndef QT_NO_WHEELEVENT
    case QEvent::Wheel:
        dbg.nospace() << "QWheelEvent("
                      << static_cast<const QWheelEvent *>(e)->pixelDelta()
                      << static_cast<const QWheelEvent *>(e)->angleDelta()
                      << ')';
        return dbg.space();
#endif
    case QEvent::KeyPress:
    case QEvent::KeyRelease:
    case QEvent::ShortcutOverride:
        {
            const QKeyEvent *ke = static_cast<const QKeyEvent*>(e);
            switch(ke->type()) {
            case QEvent::ShortcutOverride:
                n = "ShortcutOverride";
                break;
            case QEvent::KeyRelease:
                n = "KeyRelease";
                break;
            case QEvent::KeyPress:
            default:
                n = "KeyPress";
                break;
            }
            dbg.nospace() << "QKeyEvent("  << n
                          << ", " << hex << ke->key()
                          << ", " << hex << (int)ke->modifiers()
                          << ", \"" << ke->text()
                          << "\", " << ke->isAutoRepeat()
                          << ", " << ke->count()
                          << ')';
        }
        return dbg.space();
    case QEvent::FocusIn:
        n = "FocusIn";
        break;
    case QEvent::FocusOut:
        n = "FocusOut";
        break;
    case QEvent::Enter:
        n = "Enter";
        break;
    case QEvent::Leave:
        n = "Leave";
        break;
    case QEvent::PaletteChange:
        n = "PaletteChange";
        break;
    case QEvent::PolishRequest:
        n = "PolishRequest";
        break;
    case QEvent::Polish:
        n = "Polish";
        break;
    case QEvent::UpdateRequest:
        n = "UpdateRequest";
        break;
    case QEvent::Paint:
        n = "Paint";
        break;
    case QEvent::Move:
        n = "Move";
        break;
    case QEvent::Resize:
        n = "Resize";
        break;
    case QEvent::Create:
        n = "Create";
        break;
    case QEvent::Destroy:
        n = "Destroy";
        break;
    case QEvent::Close:
        n = "Close";
        break;
    case QEvent::Quit:
        n = "Quit";
        break;
    case QEvent::FileOpen:
        n = "FileOpen";
        break;
    case QEvent::Show:
        n = "Show";
        break;
    case QEvent::ShowToParent:
        n = "ShowToParent";
        break;
    case QEvent::Hide:
        n = "Hide";
        break;
    case QEvent::HideToParent:
        n = "HideToParent";
        break;
    case QEvent::None:
        n = "None";
        break;
    case QEvent::ParentChange:
        n = "ParentChange";
        break;
    case QEvent::ParentAboutToChange:
        n = "ParentAboutToChange";
        break;
    case QEvent::HoverEnter:
        n = "HoverEnter";
        break;
    case QEvent::HoverMove:
        n = "HoverMove";
        break;
    case QEvent::HoverLeave:
        n = "HoverLeave";
        break;
    case QEvent::ZOrderChange:
        n = "ZOrderChange";
        break;
    case QEvent::StyleChange:
        n = "StyleChange";
        break;
    case QEvent::DragEnter:
        n = "DragEnter";
        break;
    case QEvent::DragMove:
        n = "DragMove";
        break;
    case QEvent::DragLeave:
        n = "DragLeave";
        break;
    case QEvent::Drop:
        n = "Drop";
        break;
    case QEvent::GraphicsSceneMouseMove:
        n = "GraphicsSceneMouseMove";
        break;
    case QEvent::GraphicsSceneMousePress:
        n = "GraphicsSceneMousePress";
        break;
    case QEvent::GraphicsSceneMouseRelease:
        n = "GraphicsSceneMouseRelease";
        break;
    case QEvent::GraphicsSceneMouseDoubleClick:
        n = "GraphicsSceneMouseDoubleClick";
        break;
    case QEvent::GraphicsSceneContextMenu:
        n = "GraphicsSceneContextMenu";
        break;
    case QEvent::GraphicsSceneHoverEnter:
        n = "GraphicsSceneHoverEnter";
        break;
    case QEvent::GraphicsSceneHoverMove:
        n = "GraphicsSceneHoverMove";
        break;
    case QEvent::GraphicsSceneHoverLeave:
        n = "GraphicsSceneHoverLeave";
        break;
    case QEvent::GraphicsSceneHelp:
        n = "GraphicsSceneHelp";
        break;
    case QEvent::GraphicsSceneDragEnter:
        n = "GraphicsSceneDragEnter";
        break;
    case QEvent::GraphicsSceneDragMove:
        n = "GraphicsSceneDragMove";
        break;
    case QEvent::GraphicsSceneDragLeave:
        n = "GraphicsSceneDragLeave";
        break;
    case QEvent::GraphicsSceneDrop:
        n = "GraphicsSceneDrop";
        break;
    case QEvent::GraphicsSceneWheel:
        n = "GraphicsSceneWheel";
        break;
    case QEvent::GraphicsSceneResize:
        n = "GraphicsSceneResize";
        break;
    case QEvent::GraphicsSceneMove:
        n = "GraphicsSceneMove";
        break;
    case QEvent::CursorChange:
        n = "CursorChange";
        break;
    case QEvent::ToolTipChange:
        n = "ToolTipChange";
        break;
    case QEvent::StatusTip:
        n = "StatusTip";
        break;
    case QEvent::WhatsThis:
        n = "WhatsThis";
        break;
    case QEvent::FontChange:
        n = "FontChange";
        break;
    case QEvent::Style:
        n = "Style";
        break;
    case QEvent::KeyboardLayoutChange:
        n = "KeyboardLayoutChange";
        break;
    case QEvent::DynamicPropertyChange:
        n = "DynamicPropertyChange";
        break;
    case QEvent::GrabMouse:
        n = "GrabMouse";
        break;
    case QEvent::UngrabMouse:
        n = "UngrabMouse";
        break;
    case QEvent::GrabKeyboard:
        n = "GrabKeyboard";
        break;
    case QEvent::UngrabKeyboard:
        n = "UngrabKeyboard";
        break;
    case QEvent::TouchBegin:
        n = "TouchBegin";
    case QEvent::TouchUpdate:
        n = n ? n : "TouchUpdate";
    case QEvent::TouchEnd:
        n = n ? n : "TouchEnd";
        formatTouchEvent(dbg.nospace(), n, *static_cast<const QTouchEvent*>(e));
        return dbg.nospace();
    case QEvent::ChildAdded: n = n ? n : "ChildAdded";
    case QEvent::ChildPolished: n = n ? n : "ChildPolished";
    case QEvent::ChildRemoved: n = n ? n : "ChildRemoved";
        dbg.nospace() << "QChildEvent(" << n << ", " << (static_cast<const QChildEvent*>(e))->child();
        return dbg.space();
#ifndef QT_NO_GESTURES
    case QEvent::Gesture:
        n = "Gesture";
        break;
#endif
    default:
        dbg.nospace() << "QEvent(" << (const void *)e << ", type = " << e->type() << ')';
        return dbg.space();
    }

    dbg.nospace() << 'Q' << n << "Event(" << (const void *)e << ')';
    return dbg.space();
}
#endif

/*!
    \class QShortcutEvent
    \brief The QShortcutEvent class provides an event which is generated when
    the user presses a key combination.

    \ingroup events
    \inmodule QtGui

    Normally you don't need to use this class directly; QShortcut
    provides a higher-level interface to handle shortcut keys.

    \sa QShortcut
*/

/*!
    \fn const QKeySequence &QShortcutEvent::key() const

    Returns the key sequence that triggered the event.
*/

/*!
    \fn int QShortcutEvent::shortcutId() const

    Returns the ID of the QShortcut object for which this event was
    generated.

    \sa QShortcut::id()
*/

/*!
    \fn bool QShortcutEvent::isAmbiguous() const

    Returns \c true if the key sequence that triggered the event is
    ambiguous.

    \sa QShortcut::activatedAmbiguously()
*/

/*!
    \class QWindowStateChangeEvent
    \ingroup events
    \inmodule QtGui

    \brief The QWindowStateChangeEvent class provides the window state before a
    window state change.
*/

/*! \fn Qt::WindowStates QWindowStateChangeEvent::oldState() const

    Returns the state of the window before the change.
*/

/*! \internal
 */
QWindowStateChangeEvent::QWindowStateChangeEvent(Qt::WindowStates s, bool isOverride)
    : QEvent(WindowStateChange), ostate(s), m_override(isOverride)
{
}

/*! \internal
 */
bool QWindowStateChangeEvent::isOverride() const
{
    return m_override;
}

/*! \internal
*/
QWindowStateChangeEvent::~QWindowStateChangeEvent()
{
}


/*!
    \class QTouchEvent
    \brief The QTouchEvent class contains parameters that describe a touch event.
    \since 4.6
    \ingroup events
    \ingroup touch
    \inmodule QtGui

    \section1 Enabling Touch Events

    Touch events occur when pressing, releasing, or moving one or more touch points on a touch
    device (such as a touch-screen or track-pad). To receive touch events, widgets have to have the
    Qt::WA_AcceptTouchEvents attribute set and graphics items need to have the
    \l{QGraphicsItem::setAcceptTouchEvents()}{acceptTouchEvents} attribute set to true.

    When using QAbstractScrollArea based widgets, you should enable the Qt::WA_AcceptTouchEvents
    attribute on the scroll area's \l{QAbstractScrollArea::viewport()}{viewport}.

    Similarly to QMouseEvent, Qt automatically grabs each touch point on the first press inside a
    widget, and the widget will receive all updates for the touch point until it is released.
    Note that it is possible for a widget to receive events for numerous touch points, and that
    multiple widgets may be receiving touch events at the same time.

    \section1 Event Handling

    All touch events are of type QEvent::TouchBegin, QEvent::TouchUpdate, QEvent::TouchEnd or
    QEvent::TouchCancel. Reimplement QWidget::event() or QAbstractScrollArea::viewportEvent() for
    widgets and QGraphicsItem::sceneEvent() for items in a graphics view to receive touch events.

    Unlike widgets, QWindows receive touch events always, there is no need to opt in. When working
    directly with a QWindow, it is enough to reimplement QWindow::touchEvent().

    The QEvent::TouchUpdate and QEvent::TouchEnd events are sent to the widget or item that
    accepted the QEvent::TouchBegin event. If the QEvent::TouchBegin event is not accepted and not
    filtered by an event filter, then no further touch events are sent until the next
    QEvent::TouchBegin.

    Some systems may send an event of type QEvent::TouchCancel. Upon receiving this event
    applications are requested to ignore the entire active touch sequence. For example in a
    composited system the compositor may decide to treat certain gestures as system-wide
    gestures. Whenever such a decision is made (the gesture is recognized), the clients will be
    notified with a QEvent::TouchCancel event so they can update their state accordingly.

    The touchPoints() function returns a list of all touch points contained in the event. Note that
    this list may be empty, for example in case of a QEvent::TouchCancel event. Information about
    each touch point can be retrieved using the QTouchEvent::TouchPoint class. The
    Qt::TouchPointState enum describes the different states that a touch point may have.

    \note The list of touchPoints() will never be partial: A touch event will always contain a touch
    point for each existing physical touch contacts targetting the window or widget to which the
    event is sent. For instance, assuming that all touches target the same window or widget, an
    event with a condition of touchPoints().count()==2 is guaranteed to imply that the number of
    fingers touching the touchscreen or touchpad is exactly two.

    \section1 Event Delivery and Propagation

    By default, QGuiApplication translates the first touch point in a QTouchEvent into
    a QMouseEvent. This makes it possible to enable touch events on existing widgets that do not
    normally handle QTouchEvent. See below for information on some special considerations needed
    when doing this.

    QEvent::TouchBegin is the first touch event sent to a widget. The QEvent::TouchBegin event
    contains a special accept flag that indicates whether the receiver wants the event. By default,
    the event is accepted. You should call ignore() if the touch event is not handled by your
    widget. The QEvent::TouchBegin event is propagated up the parent widget chain until a widget
    accepts it with accept(), or an event filter consumes it. For QGraphicsItems, the
    QEvent::TouchBegin event is propagated to items under the mouse (similar to mouse event
    propagation for QGraphicsItems).

    \section1 Touch Point Grouping

    As mentioned above, it is possible that several widgets can be receiving QTouchEvents at the
    same time. However, Qt makes sure to never send duplicate QEvent::TouchBegin events to the same
    widget, which could theoretically happen during propagation if, for example, the user touched 2
    separate widgets in a QGroupBox and both widgets ignored the QEvent::TouchBegin event.

    To avoid this, Qt will group new touch points together using the following rules:

    \list

    \li When the first touch point is detected, the destination widget is determined firstly by the
    location on screen and secondly by the propagation rules.

    \li When additional touch points are detected, Qt first looks to see if there are any active
    touch points on any ancestor or descendent of the widget under the new touch point. If there
    are, the new touch point is grouped with the first, and the new touch point will be sent in a
    single QTouchEvent to the widget that handled the first touch point. (The widget under the new
    touch point will not receive an event).

    \endlist

    This makes it possible for sibling widgets to handle touch events independently while making
    sure that the sequence of QTouchEvents is always correct.

    \section1 Mouse Events and Touch Event synthesizing

    QTouchEvent delivery is independent from that of QMouseEvent. The application flags
    Qt::AA_SynthesizeTouchForUnhandledMouseEvents and Qt::AA_SynthesizeMouseForUnhandledTouchEvents
    can be used to enable or disable automatic synthesizing of touch events to mouse events and
    mouse events to touch events.

    \section1 Caveats

    \list

    \li As mentioned above, enabling touch events means multiple widgets can be receiving touch
    events simultaneously. Combined with the default QWidget::event() handling for QTouchEvents,
    this gives you great flexibility in designing touch user interfaces. Be aware of the
    implications. For example, it is possible that the user is moving a QSlider with one finger and
    pressing a QPushButton with another. The signals emitted by these widgets will be
    interleaved.

    \li Recursion into the event loop using one of the exec() methods (e.g., QDialog::exec() or
    QMenu::exec()) in a QTouchEvent event handler is not supported. Since there are multiple event
    recipients, recursion may cause problems, including but not limited to lost events
    and unexpected infinite recursion.

    \li QTouchEvents are not affected by a \l{QWidget::grabMouse()}{mouse grab} or an
    \l{QApplication::activePopupWidget()}{active pop-up widget}. The behavior of QTouchEvents is
    undefined when opening a pop-up or grabbing the mouse while there are more than one active touch
    points.

    \endlist

    \sa QTouchEvent::TouchPoint, Qt::TouchPointState, Qt::WA_AcceptTouchEvents,
    QGraphicsItem::acceptTouchEvents()
*/

/*! \enum QTouchEvent::DeviceType
    \obsolete

    This enum represents the type of device that generated a QTouchEvent.

    This enum has been deprecated. Use QTouchDevice::DeviceType instead.
    \omitvalue TouchPad
    \omitvalue TouchScreen

    \sa QTouchDevice::DeviceType, QTouchDevice::type(), QTouchEvent::device()
*/

/*!
    Constructs a QTouchEvent with the given \a eventType, \a device, and
    \a touchPoints. The \a touchPointStates and \a modifiers
    are the current touch point states and keyboard modifiers at the time of
    the event.
*/
QTouchEvent::QTouchEvent(QEvent::Type eventType,
                         QTouchDevice *device,
                         Qt::KeyboardModifiers modifiers,
                         Qt::TouchPointStates touchPointStates,
                         const QList<QTouchEvent::TouchPoint> &touchPoints)
    : QInputEvent(eventType, modifiers),
      _window(0),
      _target(0),
      _device(device),
      _touchPointStates(touchPointStates),
      _touchPoints(touchPoints)
{ }

/*!
    Destroys the QTouchEvent.
*/
QTouchEvent::~QTouchEvent()
{ }

/*! \fn QWindow *QTouchEvent::window() const

    Returns the window on which the event occurred. Useful for doing
    global-local mapping on data like rawScreenPositions() which,
    for performance reasons, only stores the global positions in the
    touch event.
*/

/*! \fn QObject *QTouchEvent::target() const

    Returns the target object within the window on which the event occurred.
    This is typically a QWidget or a QQuickItem. May be 0 when no specific target is available.
*/

/*! \fn QTouchEvent::DeviceType QTouchEvent::deviceType() const
    \obsolete

    Returns the touch device Type, which is of type \l {QTouchEvent::DeviceType} {DeviceType}.

    This function has been deprecated. Use QTouchDevice::type() instead.

    \sa QTouchDevice::type(), QTouchEvent::device()
*/

/*! \fn Qt::TouchPointStates QTouchEvent::touchPointStates() const

    Returns a bitwise OR of all the touch point states for this event.
*/

/*! \fn const QList<QTouchEvent::TouchPoint> &QTouchEvent::touchPoints() const

    Returns the list of touch points contained in the touch event.
*/

/*! \fn QTouchDevice* QTouchEvent::device() const

    Returns the touch device from which this touch event originates.
*/

/*! \fn void QTouchEvent::setWindow(QWindow *window)

    \internal

    Sets the window for this event.
*/

/*! \fn void QTouchEvent::setTarget(QObject *target)

    \internal

    Sets the target within the window (typically a widget) for this event.
*/

/*! \fn void QTouchEvent::setTouchPointStates(Qt::TouchPointStates touchPointStates)

    \internal

    Sets a bitwise OR of all the touch point states for this event.
*/

/*! \fn void QTouchEvent::setTouchPoints(const QList<QTouchEvent::TouchPoint> &touchPoints)

    \internal

    Sets the list of touch points for this event.
*/

/*! \fn void QTouchEvent::setDevice(QTouchDevice *adevice)

    \internal

    Sets the device to \a adevice.
*/

/*! \class QTouchEvent::TouchPoint
    \brief The TouchPoint class provides information about a touch point in a QTouchEvent.
    \since 4.6
    \inmodule QtGui
*/

/*! \enum TouchPoint::InfoFlag

    The values of this enum describe additional information about a touch point.

    \value Pen Indicates that the contact has been made by a designated pointing device (e.g. a pen) instead of a finger.
*/

/*!
    \internal

    Constructs a QTouchEvent::TouchPoint for use in a QTouchEvent.
*/
QTouchEvent::TouchPoint::TouchPoint(int id)
    : d(new QTouchEventTouchPointPrivate(id))
{ }

/*!
    \fn TouchPoint::TouchPoint(const TouchPoint &other)
    \internal

    Constructs a copy of \a other.
*/
QTouchEvent::TouchPoint::TouchPoint(const QTouchEvent::TouchPoint &other)
    : d(other.d)
{
    d->ref.ref();
}

/*!
    \internal

    Destroys the QTouchEvent::TouchPoint.
*/
QTouchEvent::TouchPoint::~TouchPoint()
{
    if (d && !d->ref.deref())
        delete d;
}

/*!
    Returns the id number of this touch point.

    Do not assume that id numbers start at zero or that they are sequential.
    Such an assumption is often false due to the way the underlying drivers work.
*/
int QTouchEvent::TouchPoint::id() const
{
    return d->id;
}

/*!
    Returns the current state of this touch point.
*/
Qt::TouchPointState QTouchEvent::TouchPoint::state() const
{
    return Qt::TouchPointState(int(d->state));
}

/*!
    Returns the position of this touch point, relative to the widget
    or QGraphicsItem that received the event.

    \sa startPos(), lastPos(), screenPos(), scenePos(), normalizedPos()
*/
QPointF QTouchEvent::TouchPoint::pos() const
{
    return d->rect.center();
}

/*!
    Returns the scene position of this touch point.

    The scene position is the position in QGraphicsScene coordinates
    if the QTouchEvent is handled by a QGraphicsItem::touchEvent()
    reimplementation, and identical to the screen position for
    widgets.

    \sa startScenePos(), lastScenePos(), pos()
*/
QPointF QTouchEvent::TouchPoint::scenePos() const
{
    return d->sceneRect.center();
}

/*!
    Returns the screen position of this touch point.

    \sa startScreenPos(), lastScreenPos(), pos()
*/
QPointF QTouchEvent::TouchPoint::screenPos() const
{
    return d->screenRect.center();
}

/*!
    Returns the normalized position of this touch point.

    The coordinates are normalized to the size of the touch device,
    i.e. (0,0) is the top-left corner and (1,1) is the bottom-right corner.

    \sa startNormalizedPos(), lastNormalizedPos(), pos()
*/
QPointF QTouchEvent::TouchPoint::normalizedPos() const
{
    return d->normalizedPos;
}

/*!
    Returns the starting position of this touch point, relative to the
    widget or QGraphicsItem that received the event.

    \sa pos(), lastPos()
*/
QPointF QTouchEvent::TouchPoint::startPos() const
{
    return d->startPos;
}

/*!
    Returns the starting scene position of this touch point.

    The scene position is the position in QGraphicsScene coordinates
    if the QTouchEvent is handled by a QGraphicsItem::touchEvent()
    reimplementation, and identical to the screen position for
    widgets.

    \sa scenePos(), lastScenePos()
*/
QPointF QTouchEvent::TouchPoint::startScenePos() const
{
    return d->startScenePos;
}

/*!
    Returns the starting screen position of this touch point.

    \sa screenPos(), lastScreenPos()
*/
QPointF QTouchEvent::TouchPoint::startScreenPos() const
{
    return d->startScreenPos;
}

/*!
    Returns the normalized starting position of this touch point.

    The coordinates are normalized to the size of the touch device,
    i.e. (0,0) is the top-left corner and (1,1) is the bottom-right corner.

    \sa normalizedPos(), lastNormalizedPos()
*/
QPointF QTouchEvent::TouchPoint::startNormalizedPos() const
{
    return d->startNormalizedPos;
}

/*!
    Returns the position of this touch point from the previous touch
    event, relative to the widget or QGraphicsItem that received the event.

    \sa pos(), startPos()
*/
QPointF QTouchEvent::TouchPoint::lastPos() const
{
    return d->lastPos;
}

/*!
    Returns the scene position of this touch point from the previous
    touch event.

    The scene position is the position in QGraphicsScene coordinates
    if the QTouchEvent is handled by a QGraphicsItem::touchEvent()
    reimplementation, and identical to the screen position for
    widgets.

    \sa scenePos(), startScenePos()
*/
QPointF QTouchEvent::TouchPoint::lastScenePos() const
{
    return d->lastScenePos;
}

/*!
    Returns the screen position of this touch point from the previous
    touch event.

    \sa screenPos(), startScreenPos()
*/
QPointF QTouchEvent::TouchPoint::lastScreenPos() const
{
    return d->lastScreenPos;
}

/*!
    Returns the normalized position of this touch point from the
    previous touch event.

    The coordinates are normalized to the size of the touch device,
    i.e. (0,0) is the top-left corner and (1,1) is the bottom-right corner.

    \sa normalizedPos(), startNormalizedPos()
*/
QPointF QTouchEvent::TouchPoint::lastNormalizedPos() const
{
    return d->lastNormalizedPos;
}

/*!
    Returns the rect for this touch point, relative to the widget
    or QGraphicsItem that received the event. The rect is centered
    around the point returned by pos().

    \note This function returns an empty rect if the device does not report touch point sizes.
*/
QRectF QTouchEvent::TouchPoint::rect() const
{
    return d->rect;
}

/*!
    Returns the rect for this touch point in scene coordinates.

    \note This function returns an empty rect if the device does not report touch point sizes.

    \sa scenePos(), rect()
*/
QRectF QTouchEvent::TouchPoint::sceneRect() const
{
    return d->sceneRect;
}

/*!
    Returns the rect for this touch point in screen coordinates.

    \note This function returns an empty rect if the device does not report touch point sizes.

    \sa screenPos(), rect()
*/
QRectF QTouchEvent::TouchPoint::screenRect() const
{
    return d->screenRect;
}

/*!
    Returns the pressure of this touch point. The return value is in
    the range 0.0 to 1.0.
*/
qreal QTouchEvent::TouchPoint::pressure() const
{
    return d->pressure;
}

/*!
    Returns a velocity vector for this touch point.
    The vector is in the screen's coordinate system, using pixels per seconds for the magnitude.

    \note The returned vector is only valid if the touch device's capabilities include QTouchDevice::Velocity.

    \sa QTouchDevice::capabilities(), device()
*/
QVector2D QTouchEvent::TouchPoint::velocity() const
{
    return d->velocity;
}

/*!
  Returns additional information about the touch point.

  \sa QTouchEvent::TouchPoint::InfoFlags
  */
QTouchEvent::TouchPoint::InfoFlags QTouchEvent::TouchPoint::flags() const
{
    return d->flags;
}

/*!
  \since 5.0
  Returns the raw, unfiltered positions for the touch point. The positions are in native screen coordinates.
  To get local coordinates you can use mapFromGlobal() of the QWindow returned by QTouchEvent::window().

  \note Returns an empty vector if the touch device's capabilities do not include QTouchDevice::RawPositions.

  \note Native screen coordinates refer to the native orientation of the screen which, in case of
  mobile devices, is typically portrait. This means that on systems capable of screen orientation
  changes the positions in this list will not reflect the current orientation (unlike pos(),
  screenPos(), etc.) and will always be reported in the native orientation.

  \sa QTouchDevice::capabilities(), device(), window()
  */
QVector<QPointF> QTouchEvent::TouchPoint::rawScreenPositions() const
{
    return d->rawScreenPositions;
}

/*! \internal */
void QTouchEvent::TouchPoint::setId(int id)
{
    if (d->ref.load() != 1)
        d = d->detach();
    d->id = id;
}

/*! \internal */
void QTouchEvent::TouchPoint::setState(Qt::TouchPointStates state)
{
    if (d->ref.load() != 1)
        d = d->detach();
    d->state = state;
}

/*! \internal */
void QTouchEvent::TouchPoint::setPos(const QPointF &pos)
{
    if (d->ref.load() != 1)
        d = d->detach();
    d->rect.moveCenter(pos);
}

/*! \internal */
void QTouchEvent::TouchPoint::setScenePos(const QPointF &scenePos)
{
    if (d->ref.load() != 1)
        d = d->detach();
    d->sceneRect.moveCenter(scenePos);
}

/*! \internal */
void QTouchEvent::TouchPoint::setScreenPos(const QPointF &screenPos)
{
    if (d->ref.load() != 1)
        d = d->detach();
    d->screenRect.moveCenter(screenPos);
}

/*! \internal */
void QTouchEvent::TouchPoint::setNormalizedPos(const QPointF &normalizedPos)
{
    if (d->ref.load() != 1)
        d = d->detach();
    d->normalizedPos = normalizedPos;
}

/*! \internal */
void QTouchEvent::TouchPoint::setStartPos(const QPointF &startPos)
{
    if (d->ref.load() != 1)
        d = d->detach();
    d->startPos = startPos;
}

/*! \internal */
void QTouchEvent::TouchPoint::setStartScenePos(const QPointF &startScenePos)
{
    if (d->ref.load() != 1)
        d = d->detach();
    d->startScenePos = startScenePos;
}

/*! \internal */
void QTouchEvent::TouchPoint::setStartScreenPos(const QPointF &startScreenPos)
{
    if (d->ref.load() != 1)
        d = d->detach();
    d->startScreenPos = startScreenPos;
}

/*! \internal */
void QTouchEvent::TouchPoint::setStartNormalizedPos(const QPointF &startNormalizedPos)
{
    if (d->ref.load() != 1)
        d = d->detach();
    d->startNormalizedPos = startNormalizedPos;
}

/*! \internal */
void QTouchEvent::TouchPoint::setLastPos(const QPointF &lastPos)
{
    if (d->ref.load() != 1)
        d = d->detach();
    d->lastPos = lastPos;
}

/*! \internal */
void QTouchEvent::TouchPoint::setLastScenePos(const QPointF &lastScenePos)
{
    if (d->ref.load() != 1)
        d = d->detach();
    d->lastScenePos = lastScenePos;
}

/*! \internal */
void QTouchEvent::TouchPoint::setLastScreenPos(const QPointF &lastScreenPos)
{
    if (d->ref.load() != 1)
        d = d->detach();
    d->lastScreenPos = lastScreenPos;
}

/*! \internal */
void QTouchEvent::TouchPoint::setLastNormalizedPos(const QPointF &lastNormalizedPos)
{
    if (d->ref.load() != 1)
        d = d->detach();
    d->lastNormalizedPos = lastNormalizedPos;
}

/*! \internal */
void QTouchEvent::TouchPoint::setRect(const QRectF &rect)
{
    if (d->ref.load() != 1)
        d = d->detach();
    d->rect = rect;
}

/*! \internal */
void QTouchEvent::TouchPoint::setSceneRect(const QRectF &sceneRect)
{
    if (d->ref.load() != 1)
        d = d->detach();
    d->sceneRect = sceneRect;
}

/*! \internal */
void QTouchEvent::TouchPoint::setScreenRect(const QRectF &screenRect)
{
    if (d->ref.load() != 1)
        d = d->detach();
    d->screenRect = screenRect;
}

/*! \internal */
void QTouchEvent::TouchPoint::setPressure(qreal pressure)
{
    if (d->ref.load() != 1)
        d = d->detach();
    d->pressure = pressure;
}

/*! \internal */
void QTouchEvent::TouchPoint::setVelocity(const QVector2D &v)
{
    if (d->ref.load() != 1)
        d = d->detach();
    d->velocity = v;
}

/*! \internal */
void QTouchEvent::TouchPoint::setRawScreenPositions(const QVector<QPointF> &positions)
{
    if (d->ref.load() != 1)
        d = d->detach();
    d->rawScreenPositions = positions;
}

/*!
    \internal
*/
void QTouchEvent::TouchPoint::setFlags(InfoFlags flags)
{
    if (d->ref.load() != 1)
        d = d->detach();
    d->flags = flags;
}

/*!
    \fn TouchPoint &TouchPoint::operator=(const TouchPoint &other)
    \internal
 */

/*!
    \fn void QTouchEvent::TouchPoint::swap(TouchPoint &other);
    \internal
*/

/*!
    \class QScrollPrepareEvent
    \since 4.8
    \ingroup events
    \inmodule QtGui

    \brief The QScrollPrepareEvent class is sent in preparation of scrolling.

    The scroll prepare event is sent before scrolling (usually by QScroller) is started.
    The object receiving this event should set viewportSize, maxContentPos and contentPos.
    It also should accept this event to indicate that scrolling should be started.

    It is not guaranteed that a QScrollEvent will be sent after an acceepted
    QScrollPrepareEvent, e.g. in a case where the maximum content position is (0,0).

    \sa QScrollEvent, QScroller
*/

/*!
    Creates new QScrollPrepareEvent
    The \a startPos is the position of a touch or mouse event that started the scrolling.
*/
QScrollPrepareEvent::QScrollPrepareEvent(const QPointF &startPos)
    : QEvent(QEvent::ScrollPrepare), m_target(0), m_startPos(startPos)
{
    Q_UNUSED(m_target);
}

/*!
    Destroys QScrollEvent.
*/
QScrollPrepareEvent::~QScrollPrepareEvent()
{
}

/*!
    Returns the position of the touch or mouse event that started the scrolling.
*/
QPointF QScrollPrepareEvent::startPos() const
{
    return m_startPos;
}

/*!
    Returns size of the area that is to be scrolled as set by setViewportSize

    \sa setViewportSize()
*/
QSizeF QScrollPrepareEvent::viewportSize() const
{
    return m_viewportSize;
}

/*!
    Returns the range of coordinates for the content as set by setContentPosRange().
*/
QRectF QScrollPrepareEvent::contentPosRange() const
{
    return m_contentPosRange;
}

/*!
    Returns the current position of the content as set by setContentPos.
*/
QPointF QScrollPrepareEvent::contentPos() const
{
    return m_contentPos;
}


/*!
    Sets the size of the area that is to be scrolled to \a size.

    \sa viewportSize()
*/
void QScrollPrepareEvent::setViewportSize(const QSizeF &size)
{
    m_viewportSize = size;
}

/*!
    Sets the range of content coordinates to \a rect.

    \sa contentPosRange()
*/
void QScrollPrepareEvent::setContentPosRange(const QRectF &rect)
{
    m_contentPosRange = rect;
}

/*!
    Sets the current content position to \a pos.

    \sa contentPos()
*/
void QScrollPrepareEvent::setContentPos(const QPointF &pos)
{
    m_contentPos = pos;
}


/*!
    \class QScrollEvent
    \since 4.8
    \ingroup events
    \inmodule QtGui

    \brief The QScrollEvent class is sent when scrolling.

    The scroll event is sent to indicate that the receiver should be scrolled.
    Usually the receiver should be something visual like QWidget or QGraphicsObject.

    Some care should be taken that no conflicting QScrollEvents are sent from two
    sources. Using QScroller::scrollTo is save however.

    \sa QScrollPrepareEvent, QScroller
*/

/*!
    \enum QScrollEvent::ScrollState

    This enum describes the states a scroll event can have.

    \value ScrollStarted Set for the first scroll event of a scroll activity.

    \value ScrollUpdated Set for all but the first and the last scroll event of a scroll activity.

    \value ScrollFinished Set for the last scroll event of a scroll activity.

    \sa QScrollEvent::scrollState()
*/

/*!
    Creates a new QScrollEvent
    \a contentPos is the new content position, \a overshootDistance is the
    new overshoot distance while \a scrollState indicates if this scroll
    event is the first one, the last one or some event in between.
*/
QScrollEvent::QScrollEvent(const QPointF &contentPos, const QPointF &overshootDistance, ScrollState scrollState)
    : QEvent(QEvent::Scroll), m_contentPos(contentPos), m_overshoot(overshootDistance), m_state(scrollState)
{
}

/*!
    Destroys QScrollEvent.
*/
QScrollEvent::~QScrollEvent()
{
}

/*!
    Returns the new scroll position.
*/
QPointF QScrollEvent::contentPos() const
{
    return m_contentPos;
}

/*!
    Returns the new overshoot distance.
    See QScroller for an explanation of the term overshoot.

    \sa QScroller
*/
QPointF QScrollEvent::overshootDistance() const
{
    return m_overshoot;
}

/*!
    Returns the current scroll state as a combination of ScrollStateFlag values.
    ScrollStarted (or ScrollFinished) will be set, if this scroll event is the first (or last) event in a scrolling activity.
    Please note that both values can be set at the same time, if the activity consists of a single QScrollEvent.
    All other scroll events in between will have their state set to ScrollUpdated.

    A widget could for example revert selections when scrolling is started and stopped.
*/
QScrollEvent::ScrollState QScrollEvent::scrollState() const
{
    return m_state;
}

/*!
    Creates a new QScreenOrientationChangeEvent
    \a orientation is the new orientation of the screen.
*/
QScreenOrientationChangeEvent::QScreenOrientationChangeEvent(QScreen *screen, Qt::ScreenOrientation screenOrientation)
    : QEvent(QEvent::OrientationChange), m_screen(screen), m_orientation(screenOrientation)
{
}

/*!
    Destroys QScreenOrientationChangeEvent.
*/
QScreenOrientationChangeEvent::~QScreenOrientationChangeEvent()
{
}

/*!
    Returns the screen whose orientation changed.
*/
QScreen *QScreenOrientationChangeEvent::screen() const
{
    return m_screen;
}

/*!
    Returns the orientation of the screen.
*/
Qt::ScreenOrientation QScreenOrientationChangeEvent::orientation() const
{
    return m_orientation;
}

/*!
    Creates a new QApplicationStateChangeEvent.
    \a applicationState is the new state.
*/
QApplicationStateChangeEvent::QApplicationStateChangeEvent(Qt::ApplicationState applicationState)
    : QEvent(QEvent::ApplicationStateChange), m_applicationState(applicationState)
{
}

/*!
    Returns the state of the application.
*/
Qt::ApplicationState QApplicationStateChangeEvent::applicationState() const
{
    return m_applicationState;
}

QT_END_NAMESPACE
