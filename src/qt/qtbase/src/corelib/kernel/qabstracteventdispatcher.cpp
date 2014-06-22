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

#include "qabstracteventdispatcher.h"
#include "qabstracteventdispatcher_p.h"
#include "qabstractnativeeventfilter.h"

#include "qthread.h"
#include <private/qthread_p.h>
#include <private/qcoreapplication_p.h>
#include <private/qfreelist_p.h>

QT_BEGIN_NAMESPACE

// we allow for 2^24 = 8^8 = 16777216 simultaneously running timers
struct QtTimerIdFreeListConstants : public QFreeListDefaultConstants
{
    enum
    {
        InitialNextValue = 1,
        BlockCount = 6
    };

    static const int Sizes[BlockCount];
};

enum {
    Offset0 = 0x00000000,
    Offset1 = 0x00000040,
    Offset2 = 0x00000100,
    Offset3 = 0x00001000,
    Offset4 = 0x00010000,
    Offset5 = 0x00100000,

    Size0 = Offset1  - Offset0,
    Size1 = Offset2  - Offset1,
    Size2 = Offset3  - Offset2,
    Size3 = Offset4  - Offset3,
    Size4 = Offset5  - Offset4,
    Size5 = QtTimerIdFreeListConstants::MaxIndex - Offset5
};

const int QtTimerIdFreeListConstants::Sizes[QtTimerIdFreeListConstants::BlockCount] = {
    Size0,
    Size1,
    Size2,
    Size3,
    Size4,
    Size5
};

typedef QFreeList<void, QtTimerIdFreeListConstants> QtTimerIdFreeList;
Q_GLOBAL_STATIC(QtTimerIdFreeList, timerIdFreeList)

int QAbstractEventDispatcherPrivate::allocateTimerId()
{
    return timerIdFreeList()->next();
}

void QAbstractEventDispatcherPrivate::releaseTimerId(int timerId)
{
    // this function may be called by a global destructor after
    // timerIdFreeList() has been destructed
    if (QtTimerIdFreeList *fl = timerIdFreeList())
        fl->release(timerId);
}

/*!
    \class QAbstractEventDispatcher
    \inmodule QtCore
    \brief The QAbstractEventDispatcher class provides an interface to manage Qt's event queue.

    \ingroup events

    An event dispatcher receives events from the window system and other
    sources. It then sends them to the QCoreApplication or QApplication
    instance for processing and delivery. QAbstractEventDispatcher provides
    fine-grained control over event delivery.

    For simple control of event processing use
    QCoreApplication::processEvents().

    For finer control of the application's event loop, call
    instance() and call functions on the QAbstractEventDispatcher
    object that is returned. If you want to use your own instance of
    QAbstractEventDispatcher or of a QAbstractEventDispatcher
    subclass, you must install it with QCoreApplication::setEventDispatcher()
    or QThread::setEventDispatcher() \e before a default event dispatcher has
    been installed.

    The main event loop is started by calling
    QCoreApplication::exec(), and stopped by calling
    QCoreApplication::exit(). Local event loops can be created using
    QEventLoop.

    Programs that perform long operations can call processEvents()
    with a bitwise OR combination of various QEventLoop::ProcessEventsFlag
    values to control which events should be delivered.

    QAbstractEventDispatcher also allows the integration of an
    external event loop with the Qt event loop.

    \sa QEventLoop, QCoreApplication, QThread
*/

/*!
    Constructs a new event dispatcher with the given \a parent.
*/
QAbstractEventDispatcher::QAbstractEventDispatcher(QObject *parent)
    : QObject(*new QAbstractEventDispatcherPrivate, parent) {}

/*!
    \internal
*/
QAbstractEventDispatcher::QAbstractEventDispatcher(QAbstractEventDispatcherPrivate &dd,
                                                   QObject *parent)
    : QObject(dd, parent) {}

/*!
    Destroys the event dispatcher.
*/
QAbstractEventDispatcher::~QAbstractEventDispatcher()
{ }

/*!
    Returns a pointer to the event dispatcher object for the specified
    \a thread. If \a thread is zero, the current thread is used. If no
    event dispatcher exists for the specified thread, this function
    returns 0.

    \b{Note:} If Qt is built without thread support, the \a thread
    argument is ignored.
 */
QAbstractEventDispatcher *QAbstractEventDispatcher::instance(QThread *thread)
{
    QThreadData *data = thread ? QThreadData::get2(thread) : QThreadData::current();
    return data->eventDispatcher.load();
}

/*!
    \fn bool QAbstractEventDispatcher::processEvents(QEventLoop::ProcessEventsFlags flags)

    Processes pending events that match \a flags until there are no
    more events to process. Returns \c true if an event was processed;
    otherwise returns \c false.

    This function is especially useful if you have a long running
    operation and want to show its progress without allowing user
    input; i.e. by using the QEventLoop::ExcludeUserInputEvents flag.

    If the QEventLoop::WaitForMoreEvents flag is set in \a flags, the
    behavior of this function is as follows:

    \list

    \li If events are available, this function returns after processing
    them.

    \li If no events are available, this function will wait until more
    are available and return after processing newly available events.

    \endlist

    If the QEventLoop::WaitForMoreEvents flag is not set in \a flags,
    and no events are available, this function will return
    immediately.

    \b{Note:} This function does not process events continuously; it
    returns after all available events are processed.

    \sa hasPendingEvents()
*/

/*! \fn bool QAbstractEventDispatcher::hasPendingEvents()
    \deprecated

    Returns \c true if there is an event waiting; otherwise returns false. This
    function is an implementation detail for
    QCoreApplication::hasPendingEvents() and must not be called directly.
*/

/*!
    \fn void QAbstractEventDispatcher::registerSocketNotifier(QSocketNotifier *notifier)

    Registers \a notifier with the event loop. Subclasses must
    implement this method to tie a socket notifier into another
    event loop.
*/

/*! \fn void QAbstractEventDispatcher::unregisterSocketNotifier(QSocketNotifier *notifier)

    Unregisters \a notifier from the event dispatcher. Subclasses must
    reimplement this method to tie a socket notifier into another
    event loop. Reimplementations must call the base
    implementation.
*/

/*!
    \obsolete

    \fn int QAbstractEventDispatcher::registerTimer(int interval, QObject *object)

    Registers a timer with the specified \a interval for the given \a object
    and returns the timer id.
*/

/*!
    \obsolete

    \fn void QAbstractEventDispatcher::registerTimer(int timerId, int interval, QObject *object)

    Register a timer with the specified \a timerId and \a interval for the
    given \a object.
*/

/*!
    Registers a timer with the specified \a interval and \a timerType for the
    given \a object and returns the timer id.
*/
int QAbstractEventDispatcher::registerTimer(int interval, Qt::TimerType timerType, QObject *object)
{
    int id = QAbstractEventDispatcherPrivate::allocateTimerId();
    registerTimer(id, interval, timerType, object);
    return id;
}

/*!
    \fn void QAbstractEventDispatcher::registerTimer(int timerId, int interval, Qt::TimerType timerType, QObject *object)

    Register a timer with the specified \a timerId, \a interval, and \a
    timerType for the given \a object.
*/

/*!
    \fn bool QAbstractEventDispatcher::unregisterTimer(int timerId)

    Unregisters the timer with the given \a timerId.
    Returns \c true if successful; otherwise returns \c false.

    \sa registerTimer(), unregisterTimers()
*/

/*!
    \fn bool QAbstractEventDispatcher::unregisterTimers(QObject *object)

    Unregisters all the timers associated with the given \a object.
    Returns \c true if all timers were successful removed; otherwise returns \c false.

    \sa unregisterTimer(), registeredTimers()
*/

/*!
    \fn QList<TimerInfo> QAbstractEventDispatcher::registeredTimers(QObject *object) const

    Returns a list of registered timers for \a object. The TimerInfo struct has
    \c timerId, \c interval, and \c timerType members.

    \sa Qt::TimerType
*/

/*!
    \fn int QAbstractEventDispatcher::remainingTime(int timerId)

    Returns the remaining time in milliseconds with the given \a timerId.
    If the timer is inactive, the returned value will be -1. If the timer is
    overdue, the returned value will be 0.

    \sa Qt::TimerType
*/

/*! \fn void QAbstractEventDispatcher::wakeUp()
    \threadsafe

    Wakes up the event loop.

    \sa awake()
*/

/*!
    \fn void QAbstractEventDispatcher::interrupt()

    Interrupts event dispatching; i.e. the event dispatcher will
    return from processEvents() as soon as possible.
*/

/*! \fn void QAbstractEventDispatcher::flush()

    Flushes the event queue. This normally returns almost
    immediately. Does nothing on platforms other than X11.
*/

// ### DOC: Are these called when the _application_ starts/stops or just
// when the current _event loop_ starts/stops?
/*!
    \internal
*/
void QAbstractEventDispatcher::startingUp()
{ }

/*!
    \internal
*/
void QAbstractEventDispatcher::closingDown()
{ }

/*!
    \class QAbstractEventDispatcher::TimerInfo
    \inmodule QtCore

    This struct represents information about a timer:
    \l{QAbstractEventDispatcher::TimerInfo::timerId}{timerId},
    \l{QAbstractEventDispatcher::TimerInfo::interval}{interval}, and
    \l{QAbstractEventDispatcher::TimerInfo::timerType}{timerType}.

    \sa registeredTimers()
*/
/*! \fn QAbstractEventDispatcher::TimerInfo::TimerInfo(int timerId, int interval, Qt::TimerType timerType)

    Constructs a TimerInfo struct with the given \a timerId, \a interval, and
    \a timerType.
*/
/*!
    \variable QAbstractEventDispatcher::TimerInfo::timerId

    The timer's unique id.
*/
/*!
    \variable QAbstractEventDispatcher::TimerInfo::interval

    The timer's interval.
*/
/*!
    \variable QAbstractEventDispatcher::TimerInfo::timerType

    The timer's type

    \sa Qt::TimerType
*/

/*!
    Installs an event filter \a filterObj for all native event filters
    received by the application.

    The event filter \a filterObj receives events via its nativeEventFilter()
    function, which is called for all events received by all threads.

    The nativeEventFilter() function should return true if the event should
    be filtered, (i.e. stopped). It should return false to allow
    normal Qt processing to continue: the native event can then be translated
    into a QEvent and handled by the standard Qt \l{QEvent} {event} filtering,
    e.g. QObject::installEventFilter().

    If multiple event filters are installed, the filter that was installed last
    is activated first.

    \note The filter function set here receives native messages,
    i.e. MSG or XEvent structs.

    For maximum portability, you should always try to use QEvents
    and QObject::installEventFilter() whenever possible.

    \sa QObject::installEventFilter()

    \since 5.0
*/
void QAbstractEventDispatcher::installNativeEventFilter(QAbstractNativeEventFilter *filterObj)
{
    Q_D(QAbstractEventDispatcher);

    // clean up unused items in the list
    d->eventFilters.removeAll(0);
    d->eventFilters.removeAll(filterObj);
    d->eventFilters.prepend(filterObj);
}

/*!
    Removes the event filter \a filter from this object. The
    request is ignored if such an event filter has not been installed.

    All event filters for this object are automatically removed when
    this object is destroyed.

    It is always safe to remove an event filter, even during event
    filter activation (i.e. from the nativeEventFilter() function).

    \sa installNativeEventFilter(), QAbstractNativeEventFilter
    \since 5.0
*/
void QAbstractEventDispatcher::removeNativeEventFilter(QAbstractNativeEventFilter *filter)
{
    Q_D(QAbstractEventDispatcher);
    for (int i = 0; i < d->eventFilters.count(); ++i) {
        if (d->eventFilters.at(i) == filter) {
            d->eventFilters[i] = 0;
            break;
        }
    }
}

/*!
    Sends \a message through the event filters that were set by
    installNativeEventFilter().  This function returns \c true as soon as an
    event filter returns \c true, and false otherwise to indicate that
    the processing of the event should continue.

    Subclasses of QAbstractEventDispatcher \e must call this function
    for \e all messages received from the system to ensure
    compatibility with any extensions that may be used in the
    application. The type of event \a eventType is specific to the platform
    plugin chosen at run-time, and can be used to cast message to the right type.
    The \a result pointer is only used on Windows, and corresponds to the LRESULT pointer.

    Note that the type of \a message is platform dependent. See
    QAbstractNativeEventFilter for details.

    \sa installNativeEventFilter(), QAbstractNativeEventFilter::nativeEventFilter()
    \since 5.0
*/
bool QAbstractEventDispatcher::filterNativeEvent(const QByteArray &eventType, void *message, long *result)
{
    Q_D(QAbstractEventDispatcher);
    if (!d->eventFilters.isEmpty()) {
        // Raise the loopLevel so that deleteLater() calls in or triggered
        // by event_filter() will be processed from the main event loop.
        QScopedLoopLevelCounter loopLevelCounter(d->threadData);
        for (int i = 0; i < d->eventFilters.size(); ++i) {
            QAbstractNativeEventFilter *filter = d->eventFilters.at(i);
            if (!filter)
                continue;
            if (filter->nativeEventFilter(eventType, message, result))
                return true;
        }
    }
    return false;
}

/*! \fn bool QAbstractEventDispatcher::filterEvent(void *message)
    \deprecated

    Calls filterNativeEvent() with an empty eventType and \a message.
    This function returns \c true as soon as an
    event filter returns \c true, and false otherwise to indicate that
    the processing of the event should continue.
*/

/*! \fn bool QAbstractEventDispatcher::registerEventNotifier(QWinEventNotifier *notifier);

  This pure virtual method exists on windows only and has to be reimplemented by a Windows specific
  event dispatcher implementation. \a notifier is the QWinEventNotifier instance to be registered.

  The method should return true if the registration of \a notifier was sucessful, otherwise false.

  QWinEventNotifier calls this method in it's constructor and there should never be a need to call this
  method directly.

  \sa QWinEventNotifier, unregisterEventNotifier()
*/

/*! \fn bool QAbstractEventDispatcher::unregisterEventNotifier(QWinEventNotifier *notifier);

  This pure virtual method exists on windows only and has to be reimplemented by a Windows specific
  event dispatcher implementation. \a notifier is the QWinEventNotifier instance to be unregistered.

  QWinEventNotifier calls this method in it's destructor and there should never be a need to call this
  method directly.

  \sa QWinEventNotifier, registerEventNotifier()
*/

/*! \fn void QAbstractEventDispatcher::awake()

    This signal is emitted after the event loop returns from a
    function that could block.

    \sa wakeUp(), aboutToBlock()
*/

/*! \fn void QAbstractEventDispatcher::aboutToBlock()

    This signal is emitted before the event loop calls a function that
    could block.

    \sa awake()
*/

QT_END_NAMESPACE
