/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "qcoreevent.h"
#include "qcoreapplication.h"
#include "qcoreapplication_p.h"

#include "qmutex.h"
#include "qset.h"

QT_BEGIN_NAMESPACE

/*!
    \class QEvent
    \inmodule QtCore
    \brief The QEvent class is the base class of all
    event classes. Event objects contain event parameters.

    \ingroup events

    Qt's main event loop (QCoreApplication::exec()) fetches native
    window system events from the event queue, translates them into
    QEvents, and sends the translated events to \l{QObject}s.

    In general, events come from the underlying window system
    (spontaneous() returns \c true), but it is also possible to manually
    send events using QCoreApplication::sendEvent() and
    QCoreApplication::postEvent() (spontaneous() returns \c false).

    QObjects receive events by having their QObject::event() function
    called. The function can be reimplemented in subclasses to
    customize event handling and add additional event types;
    QWidget::event() is a notable example. By default, events are
    dispatched to event handlers like QObject::timerEvent() and
    QWidget::mouseMoveEvent(). QObject::installEventFilter() allows an
    object to intercept events destined for another object.

    The basic QEvent contains only an event type parameter and an
    "accept" flag.  The accept flag set with accept(), and cleared
    with ignore(). It is set by default, but don't rely on this as
    subclasses may choose to clear it in their constructor.

    Subclasses of QEvent contain additional parameters that describe
    the particular event.

    \sa QObject::event(), QObject::installEventFilter(),
        QCoreApplication::sendEvent(),
        QCoreApplication::postEvent(), QCoreApplication::processEvents()
*/


/*!
    \enum QEvent::Type

    This enum type defines the valid event types in Qt. The event
    types and the specialized classes for each type are as follows:

    \value None                             Not an event.
    \value ActionAdded                      A new action has been added (QActionEvent).
    \value ActionChanged                    An action has been changed (QActionEvent).
    \value ActionRemoved                    An action has been removed (QActionEvent).
    \value ActivationChange                 A widget's top-level window activation state has changed.
    \value ApplicationActivate              This enum has been deprecated. Use ApplicationStateChange instead.
    \value ApplicationActivated             This enum has been deprecated. Use ApplicationStateChange instead.
    \value ApplicationDeactivate            This enum has been deprecated. Use ApplicationStateChange instead.
    \value ApplicationFontChange            The default application font has changed.
    \value ApplicationLayoutDirectionChange The default application layout direction has changed.
    \value ApplicationPaletteChange         The default application palette has changed.
    \value ApplicationStateChange           The state of the application has changed.
    \value ApplicationWindowIconChange      The application's icon has changed.
    \value ChildAdded                       An object gets a child (QChildEvent).
    \value ChildPolished                    A widget child gets polished (QChildEvent).
    \value ChildRemoved                     An object loses a child (QChildEvent).
    \value Clipboard                        The clipboard contents have changed (QClipboardEvent).
    \value Close                            Widget was closed (QCloseEvent).
    \value CloseSoftwareInputPanel          A widget wants to close the software input panel (SIP).
    \value ContentsRectChange               The margins of the widget's content rect changed.
    \value ContextMenu                      Context popup menu (QContextMenuEvent).
    \value CursorChange                     The widget's cursor has changed.
    \value DeferredDelete                   The object will be deleted after it has cleaned up (QDeferredDeleteEvent).
    \value DragEnter                        The cursor enters a widget during a drag and drop operation (QDragEnterEvent).
    \value DragLeave                        The cursor leaves a widget during a drag and drop operation (QDragLeaveEvent).
    \value DragMove                         A drag and drop operation is in progress (QDragMoveEvent).
    \value Drop                             A drag and drop operation is completed (QDropEvent).
    \value DynamicPropertyChange            A dynamic property was added, changed, or removed from the object.
    \value EnabledChange                    Widget's enabled state has changed.
    \value Enter                            Mouse enters widget's boundaries (QEnterEvent).
    \value EnterEditFocus                   An editor widget gains focus for editing. QT_KEYPAD_NAVIGATION must be defined.
    \value EnterWhatsThisMode               Send to toplevel widgets when the application enters "What's This?" mode.
    \value Expose                           Sent to a window when its on-screen contents are invalidated and need to be flushed from the backing store.
    \value FileOpen                         File open request (QFileOpenEvent).
    \value FocusIn                          Widget or Window gains keyboard focus (QFocusEvent).
    \value FocusOut                         Widget or Window loses keyboard focus (QFocusEvent).
    \value FocusAboutToChange               Widget or Window focus is about to change (QFocusEvent)
    \value FontChange                       Widget's font has changed.
    \value Gesture                          A gesture was triggered (QGestureEvent).
    \value GestureOverride                  A gesture override was triggered (QGestureEvent).
    \value GrabKeyboard                     Item gains keyboard grab (QGraphicsItem only).
    \value GrabMouse                        Item gains mouse grab (QGraphicsItem only).
    \value GraphicsSceneContextMenu         Context popup menu over a graphics scene (QGraphicsSceneContextMenuEvent).
    \value GraphicsSceneDragEnter           The cursor enters a graphics scene during a drag and drop operation (QGraphicsSceneDragDropEvent).
    \value GraphicsSceneDragLeave           The cursor leaves a graphics scene during a drag and drop operation (QGraphicsSceneDragDropEvent).
    \value GraphicsSceneDragMove            A drag and drop operation is in progress over a scene (QGraphicsSceneDragDropEvent).
    \value GraphicsSceneDrop                A drag and drop operation is completed over a scene (QGraphicsSceneDragDropEvent).
    \value GraphicsSceneHelp                The user requests help for a graphics scene (QHelpEvent).
    \value GraphicsSceneHoverEnter          The mouse cursor enters a hover item in a graphics scene (QGraphicsSceneHoverEvent).
    \value GraphicsSceneHoverLeave          The mouse cursor leaves a hover item in a graphics scene (QGraphicsSceneHoverEvent).
    \value GraphicsSceneHoverMove           The mouse cursor moves inside a hover item in a graphics scene (QGraphicsSceneHoverEvent).
    \value GraphicsSceneMouseDoubleClick    Mouse press again (double click) in a graphics scene (QGraphicsSceneMouseEvent).
    \value GraphicsSceneMouseMove           Move mouse in a graphics scene (QGraphicsSceneMouseEvent).
    \value GraphicsSceneMousePress          Mouse press in a graphics scene (QGraphicsSceneMouseEvent).
    \value GraphicsSceneMouseRelease        Mouse release in a graphics scene (QGraphicsSceneMouseEvent).
    \value GraphicsSceneMove                Widget was moved (QGraphicsSceneMoveEvent).
    \value GraphicsSceneResize              Widget was resized (QGraphicsSceneResizeEvent).
    \value GraphicsSceneWheel               Mouse wheel rolled in a graphics scene (QGraphicsSceneWheelEvent).
    \value Hide                             Widget was hidden (QHideEvent).
    \value HideToParent                     A child widget has been hidden.
    \value HoverEnter                       The mouse cursor enters a hover widget (QHoverEvent).
    \value HoverLeave                       The mouse cursor leaves a hover widget (QHoverEvent).
    \value HoverMove                        The mouse cursor moves inside a hover widget (QHoverEvent).
    \value IconDrag                         The main icon of a window has been dragged away (QIconDragEvent).
    \value IconTextChange                   Widget's icon text has been changed.
    \value InputMethod                      An input method is being used (QInputMethodEvent).
    \value InputMethodQuery                 A input method query event (QInputMethodQueryEvent)
    \value KeyboardLayoutChange             The keyboard layout has changed.
    \value KeyPress                         Key press (QKeyEvent).
    \value KeyRelease                       Key release (QKeyEvent).
    \value LanguageChange                   The application translation changed.
    \value LayoutDirectionChange            The direction of layouts changed.
    \value LayoutRequest                    Widget layout needs to be redone.
    \value Leave                            Mouse leaves widget's boundaries.
    \value LeaveEditFocus                   An editor widget loses focus for editing. QT_KEYPAD_NAVIGATION must be defined.
    \value LeaveWhatsThisMode               Send to toplevel widgets when the application leaves "What's This?" mode.
    \value LocaleChange                     The system locale has changed.
    \value NonClientAreaMouseButtonDblClick A mouse double click occurred outside the client area.
    \value NonClientAreaMouseButtonPress    A mouse button press occurred outside the client area.
    \value NonClientAreaMouseButtonRelease  A mouse button release occurred outside the client area.
    \value NonClientAreaMouseMove           A mouse move occurred outside the client area.
    \value MacSizeChange                    The user changed his widget sizes (Mac OS X only).
    \value MetaCall                         An asynchronous method invocation via QMetaObject::invokeMethod().
    \value ModifiedChange                   Widgets modification state has been changed.
    \value MouseButtonDblClick              Mouse press again (QMouseEvent).
    \value MouseButtonPress                 Mouse press (QMouseEvent).
    \value MouseButtonRelease               Mouse release (QMouseEvent).
    \value MouseMove                        Mouse move (QMouseEvent).
    \value MouseTrackingChange              The mouse tracking state has changed.
    \value Move                             Widget's position changed (QMoveEvent).
    \value OrientationChange                The screens orientation has changes (QScreenOrientationChangeEvent)
    \value Paint                            Screen update necessary (QPaintEvent).
    \value PaletteChange                    Palette of the widget changed.
    \value ParentAboutToChange              The widget parent is about to change.
    \value ParentChange                     The widget parent has changed.
    \value PlatformPanel                    A platform specific panel has been requested.
    \value Polish                           The widget is polished.
    \value PolishRequest                    The widget should be polished.
    \value QueryWhatsThis                   The widget should accept the event if it has "What's This?" help.
    \value RequestSoftwareInputPanel        A widget wants to open a software input panel (SIP).
    \value Resize                           Widget's size changed (QResizeEvent).
    \value ScrollPrepare                    The object needs to fill in its geometry information (QScrollPrepareEvent).
    \value Scroll                           The object needs to scroll to the supplied position (QScrollEvent).
    \value Shortcut                         Key press in child for shortcut key handling (QShortcutEvent).
    \value ShortcutOverride                 Key press in child, for overriding shortcut key handling (QKeyEvent).
    \value Show                             Widget was shown on screen (QShowEvent).
    \value ShowToParent                     A child widget has been shown.
    \value SockAct                          Socket activated, used to implement QSocketNotifier.
    \omitvalue SockClose
    \value StateMachineSignal               A signal delivered to a state machine (QStateMachine::SignalEvent).
    \value StateMachineWrapped              The event is a wrapper for, i.e., contains, another event (QStateMachine::WrappedEvent).
    \value StatusTip                        A status tip is requested (QStatusTipEvent).
    \value StyleChange                      Widget's style has been changed.
    \value TabletMove                       Wacom tablet move (QTabletEvent).
    \value TabletPress                      Wacom tablet press (QTabletEvent).
    \value TabletRelease                    Wacom tablet release (QTabletEvent).
    \value OkRequest                        Ok button in decoration pressed. Supported only for Windows CE.
    \value TabletEnterProximity             Wacom tablet enter proximity event (QTabletEvent), sent to QApplication.
    \value TabletLeaveProximity             Wacom tablet leave proximity event (QTabletEvent), sent to QApplication.
    \omitvalue ThemeChange
    \value ThreadChange                     The object is moved to another thread. This is the last event sent to this object in the previous thread. See QObject::moveToThread().
    \value Timer                            Regular timer events (QTimerEvent).
    \value ToolBarChange                    The toolbar button is toggled on Mac OS X.
    \value ToolTip                          A tooltip was requested (QHelpEvent).
    \value ToolTipChange                    The widget's tooltip has changed.
    \value TouchBegin                       Beginning of a sequence of touch-screen or track-pad events (QTouchEvent).
    \value TouchCancel                      Cancellation of touch-event sequence (QTouchEvent).
    \value TouchEnd                         End of touch-event sequence (QTouchEvent).
    \value TouchUpdate                      Touch-screen event (QTouchEvent).
    \value UngrabKeyboard                   Item loses keyboard grab (QGraphicsItem only).
    \value UngrabMouse                      Item loses mouse grab (QGraphicsItem only).
    \value UpdateLater                      The widget should be queued to be repainted at a later time.
    \value UpdateRequest                    The widget should be repainted.
    \value WhatsThis                        The widget should reveal "What's This?" help (QHelpEvent).
    \value WhatsThisClicked                 A link in a widget's "What's This?" help was clicked.
    \value Wheel                            Mouse wheel rolled (QWheelEvent).
    \value WinEventAct                      A Windows-specific activation event has occurred.
    \value WindowActivate                   Window was activated.
    \value WindowBlocked                    The window is blocked by a modal dialog.
    \value WindowDeactivate                 Window was deactivated.
    \value WindowIconChange                 The window's icon has changed.
    \value WindowStateChange                The \l{QWindow::windowState()}{window's state} (minimized, maximized or full-screen) has changed (QWindowStateChangeEvent).
    \value WindowTitleChange                The window title has changed.
    \value WindowUnblocked                  The window is unblocked after a modal dialog exited.
    \value WinIdChange                      The window system identifer for this native widget has changed.
    \value ZOrderChange                     The widget's z-order has changed. This event is never sent to top level windows.

    User events should have values between \c User and \c{MaxUser}:

    \value User                             User-defined event.
    \value MaxUser                          Last user event ID.

    For convenience, you can use the registerEventType() function to
    register and reserve a custom event type for your
    application. Doing so will allow you to avoid accidentally
    re-using a custom event type already in use elsewhere in your
    application.

    \omitvalue AcceptDropsChange
    \omitvalue ActivateControl
    \omitvalue Create
    \omitvalue DeactivateControl
    \omitvalue Destroy
    \omitvalue DragResponse
    \omitvalue EmbeddingControl
    \omitvalue HelpRequest
    \omitvalue Quit
    \omitvalue ShowWindowRequest
    \omitvalue Speech
    \omitvalue Style
    \omitvalue StyleAnimationUpdate
    \omitvalue ZeroTimerEvent
    \omitvalue ApplicationActivate
    \omitvalue ApplicationActivated
    \omitvalue ApplicationDeactivate
    \omitvalue ApplicationDeactivated
    \omitvalue MacGLWindowChange
    \omitvalue MacGLClearDrawable
    \omitvalue NetworkReplyUpdated
    \omitvalue FutureCallOut
    \omitvalue NativeGesture
    \omitvalue WindowChangeInternal
*/

/*!
    Contructs an event object of type \a type.
*/
QEvent::QEvent(Type type)
    : d(0), t(type), posted(false), spont(false), m_accept(true)
{}

/*!
    \internal
    Attempts to copy the \a other event.

    Copying events is a bad idea, yet some Qt 4 code does it (notably,
    QApplication and the state machine).
 */
QEvent::QEvent(const QEvent &other)
    : d(other.d), t(other.t), posted(other.posted), spont(other.spont),
      m_accept(other.m_accept)
{
    // if QEventPrivate becomes available, make sure to implement a
    // virtual QEventPrivate *clone() const; function so we can copy here
    Q_ASSERT_X(!d, "QEvent", "Impossible, this can't happen: QEventPrivate isn't defined anywhere");
}

/*!
    \internal
    Attempts to copy the \a other event.

    Copying events is a bad idea, yet some Qt 4 code does it (notably,
    QApplication and the state machine).
 */
QEvent &QEvent::operator=(const QEvent &other)
{
    // if QEventPrivate becomes available, make sure to implement a
    // virtual QEventPrivate *clone() const; function so we can copy here
    Q_ASSERT_X(!other.d, "QEvent", "Impossible, this can't happen: QEventPrivate isn't defined anywhere");

    t = other.t;
    posted = other.posted;
    spont = other.spont;
    m_accept = other.m_accept;
    return *this;
}

/*!
    Destroys the event. If it was \l{QCoreApplication::postEvent()}{posted},
    it will be removed from the list of events to be posted.
*/

QEvent::~QEvent()
{
    if (posted && QCoreApplication::instance())
        QCoreApplicationPrivate::removePostedEvent(this);
    Q_ASSERT_X(!d, "QEvent", "Impossible, this can't happen: QEventPrivate isn't defined anywhere");
}


/*!
    \property  QEvent::accepted
    the accept flag of the event object

    Setting the accept parameter indicates that the event receiver
    wants the event. Unwanted events might be propagated to the parent
    widget. By default, isAccepted() is set to true, but don't rely on
    this as subclasses may choose to clear it in their constructor.

    For convenience, the accept flag can also be set with accept(),
    and cleared with ignore().
*/

/*!
    \fn void QEvent::accept()

    Sets the accept flag of the event object, the equivalent of
    calling setAccepted(true).

    Setting the accept parameter indicates that the event receiver
    wants the event. Unwanted events might be propagated to the parent
    widget.

    \sa ignore()
*/


/*!
    \fn void QEvent::ignore()

    Clears the accept flag parameter of the event object, the
    equivalent of calling setAccepted(false).

    Clearing the accept parameter indicates that the event receiver
    does not want the event. Unwanted events might be propagated to the
    parent widget.

    \sa accept()
*/


/*!
    \fn QEvent::Type QEvent::type() const

    Returns the event type.
*/

/*!
    \fn bool QEvent::spontaneous() const

    Returns \c true if the event originated outside the application (a
    system event); otherwise returns \c false.

    The return value of this function is not defined for paint events.
*/

class QEventUserEventRegistration
{
public:
    QMutex mutex;
    QSet<int> set;
};
Q_GLOBAL_STATIC(QEventUserEventRegistration, userEventRegistrationHelper)

/*!
    \since 4.4
    \threadsafe

    Registers and returns a custom event type. The \a hint provided
    will be used if it is available, otherwise it will return a value
    between QEvent::User and QEvent::MaxUser that has not yet been
    registered. The \a hint is ignored if its value is not between
    QEvent::User and QEvent::MaxUser.

    Returns -1 if all available values are already taken or the
    program is shutting down.
*/
int QEvent::registerEventType(int hint)
{
    QEventUserEventRegistration *userEventRegistration
        = userEventRegistrationHelper();
    if (!userEventRegistration)
        return -1;

    QMutexLocker locker(&userEventRegistration->mutex);

    // if the type hint hasn't been registered yet, take it
    if (hint >= QEvent::User && hint <= QEvent::MaxUser && !userEventRegistration->set.contains(hint)) {
        userEventRegistration->set.insert(hint);
        return hint;
    }

    // find a free event type, starting at MaxUser and decreasing
    int id = QEvent::MaxUser;
    while (userEventRegistration->set.contains(id) && id >= QEvent::User)
        --id;
    if (id >= QEvent::User) {
        userEventRegistration->set.insert(id);
        return id;
    }
    return -1;
}

/*!
    \class QTimerEvent
    \inmodule QtCore
    \brief The QTimerEvent class contains parameters that describe a
    timer event.

    \ingroup events

    Timer events are sent at regular intervals to objects that have
    started one or more timers. Each timer has a unique identifier. A
    timer is started with QObject::startTimer().

    The QTimer class provides a high-level programming interface that
    uses signals instead of events. It also provides single-shot timers.

    The event handler QObject::timerEvent() receives timer events.

    \sa QTimer, QObject::timerEvent(), QObject::startTimer(),
    QObject::killTimer()
*/

/*!
    Constructs a timer event object with the timer identifier set to
    \a timerId.
*/
QTimerEvent::QTimerEvent(int timerId)
    : QEvent(Timer), id(timerId)
{}

/*!
    \internal
*/
QTimerEvent::~QTimerEvent()
{
}

/*!
    \fn int QTimerEvent::timerId() const

    Returns the unique timer identifier, which is the same identifier
    as returned from QObject::startTimer().
*/

/*!
    \class QChildEvent
    \inmodule QtCore
    \brief The QChildEvent class contains event parameters for child object
    events.

    \ingroup events

    Child events are sent immediately to objects when children are
    added or removed.

    In both cases you can only rely on the child being a QObject (or,
    if QObject::isWidgetType() returns \c true, a QWidget). This is
    because in the QEvent::ChildAdded case the child is not yet fully
    constructed; in the QEvent::ChildRemoved case it might have
    already been destructed.

    The handler for these events is QObject::childEvent().
*/

/*!
    Constructs a child event object of a particular \a type for the
    \a child.

    \a type can be QEvent::ChildAdded, QEvent::ChildRemoved,
    QEvent::ChildPolished, or QEvent::ChildRemoved.

    \sa child()
*/
QChildEvent::QChildEvent(Type type, QObject *child)
    : QEvent(type), c(child)
{}

/*!
    \internal
*/
QChildEvent::~QChildEvent()
{
}

/*!
    \fn QObject *QChildEvent::child() const

    Returns the child object that was added or removed.
*/

/*!
    \fn bool QChildEvent::added() const

    Returns \c true if type() is QEvent::ChildAdded; otherwise returns
    false.
*/

/*!
    \fn bool QChildEvent::removed() const

    Returns \c true if type() is QEvent::ChildRemoved; otherwise returns
    false.
*/

/*!
    \fn bool QChildEvent::polished() const

    Returns \c true if type() is QEvent::ChildPolished; otherwise returns
    false.
*/


/*!
    \class QDynamicPropertyChangeEvent
    \inmodule QtCore
    \since 4.2
    \brief The QDynamicPropertyChangeEvent class contains event parameters for dynamic
    property change events.

    \ingroup events

    Dynamic property change events are sent to objects when properties are
    dynamically added, changed or removed using QObject::setProperty().
*/

/*!
    Constructs a dynamic property change event object with the property name set to
    \a name.
*/
QDynamicPropertyChangeEvent::QDynamicPropertyChangeEvent(const QByteArray &name)
    : QEvent(QEvent::DynamicPropertyChange), n(name)
{
}

/*!
    \internal
*/
QDynamicPropertyChangeEvent::~QDynamicPropertyChangeEvent()
{
}

/*!
    \fn QByteArray QDynamicPropertyChangeEvent::propertyName() const

    Returns the name of the dynamic property that was added, changed or
    removed.

    \sa QObject::setProperty(), QObject::dynamicPropertyNames()
*/

/*!
    Constructs a deferred delete event with an initial loopLevel() of zero.
*/
QDeferredDeleteEvent::QDeferredDeleteEvent()
    : QEvent(QEvent::DeferredDelete)
    , level(0)
{ }

/*!
    \internal
*/
QDeferredDeleteEvent::~QDeferredDeleteEvent()
{ }

/*! \fn int QDeferredDeleteEvent::loopLevel() const

    Returns the loop-level in which the event was posted. The
    loop-level is set by QCoreApplication::postEvent().

    \sa QObject::deleteLater()
*/

QT_END_NAMESPACE
