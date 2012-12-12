/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#include "qthread.h"
#include <private/qthread_p.h>
#include <private/qcoreapplication_p.h>

QT_BEGIN_NAMESPACE

// we allow for 2^24 = 8^8 = 16777216 simultaneously running timers
static const int TimerIdMask = 0x00ffffff;
static const int TimerSerialMask = ~TimerIdMask & ~0x80000000;
static const int TimerSerialCounter = TimerIdMask + 1;
static const int MaxTimerId = TimerIdMask;

static int FirstBucket[] = {
     1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16,
    17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32
};

enum {
    FirstBucketOffset  = 0,
    SecondBucketOffset = sizeof(FirstBucket) / sizeof(FirstBucket[0]),
    ThirdBucketOffset  = 0x100,
    FourthBucketOffset = 0x1000,
    FifthBucketOffset  = 0x10000,
    SixthBucketOffset  = 0x100000
};

enum {
    FirstBucketSize  = SecondBucketOffset,
    SecondBucketSize = ThirdBucketOffset - SecondBucketOffset,
    ThirdBucketSize  = FourthBucketOffset - ThirdBucketOffset,
    FourthBucketSize = FifthBucketOffset - FourthBucketOffset,
    FifthBucketSize  = SixthBucketOffset - FifthBucketOffset,
    SixthBucketSize  = MaxTimerId - SixthBucketOffset
};

static const int BucketSize[] = {
    FirstBucketSize, SecondBucketSize, ThirdBucketSize,
    FourthBucketSize, FifthBucketSize, SixthBucketSize
};
enum { NumberOfBuckets = sizeof(BucketSize) / sizeof(BucketSize[0]) };

static const int BucketOffset[] = {
    FirstBucketOffset, SecondBucketOffset, ThirdBucketOffset,
    FourthBucketOffset, FifthBucketOffset, SixthBucketOffset
};

static QBasicAtomicPointer<int> timerIds[] =
    { Q_BASIC_ATOMIC_INITIALIZER(FirstBucket),
      Q_BASIC_ATOMIC_INITIALIZER(0),
      Q_BASIC_ATOMIC_INITIALIZER(0),
      Q_BASIC_ATOMIC_INITIALIZER(0),
      Q_BASIC_ATOMIC_INITIALIZER(0),
      Q_BASIC_ATOMIC_INITIALIZER(0) };

static void timerIdsDestructorFunction()
{
    // start at one, the first bucket is pre-allocated
    for (int i = 1; i < NumberOfBuckets; ++i)
        delete [] static_cast<int *>(timerIds[i]);
}
Q_DESTRUCTOR_FUNCTION(timerIdsDestructorFunction)

static QBasicAtomicInt nextFreeTimerId = Q_BASIC_ATOMIC_INITIALIZER(1);

// avoid the ABA-problem by using 7 of the top 8 bits of the timerId as a serial number
static inline int prepareNewValueWithSerialNumber(int oldId, int newId)
{
    return (newId & TimerIdMask) | ((oldId + TimerSerialCounter) & TimerSerialMask);
}

namespace {
    template<bool> struct QStaticAssertType;
    template<> struct QStaticAssertType<true> { enum { Value = 1 }; };
}
#define q_static_assert(expr)     (void)QStaticAssertType<expr>::Value

static inline int bucketOffset(int timerId)
{
    q_static_assert(sizeof BucketSize == sizeof BucketOffset);
    q_static_assert(sizeof(timerIds) / sizeof(timerIds[0]) == NumberOfBuckets);

    for (int i = 0; i < NumberOfBuckets; ++i) {
        if (timerId < BucketSize[i])
            return i;
        timerId -= BucketSize[i];
    }
    qFatal("QAbstractEventDispatcher: INTERNAL ERROR, timer ID %d is too large", timerId);
    return -1;
}

static inline int bucketIndex(int bucket, int timerId)
{
    return timerId - BucketOffset[bucket];
}

static inline int *allocateBucket(int bucket)
{
    // allocate a new bucket
    const int size = BucketSize[bucket];
    const int offset = BucketOffset[bucket];
    int *b = new int[size];
    for (int i = 0; i != size; ++i)
        b[i] = offset + i + 1;
    return b;
}

void QAbstractEventDispatcherPrivate::init()
{
    Q_Q(QAbstractEventDispatcher);
    if (threadData->eventDispatcher != 0) {
        qWarning("QAbstractEventDispatcher: An event dispatcher has already been created for this thread");
    } else {
        threadData->eventDispatcher = q;
    }
}

// Timer IDs are implemented using a free-list;
// there's a vector initialized with:
//    X[i] = i + 1
// and nextFreeTimerId starts with 1.
//
// Allocating a timer ID involves taking the ID from
//    X[nextFreeTimerId]
// updating nextFreeTimerId to this value and returning the old value
//
// When the timer ID is allocated, its cell in the vector is unused (it's a
// free list). As an added protection, we use the cell to store an invalid
// (negative) value that we can later check for integrity.
//
// (continues below).
int QAbstractEventDispatcherPrivate::allocateTimerId()
{
    int timerId, newTimerId;
    int at, *b;
    do {
        timerId = nextFreeTimerId; //.loadAcquire(); // ### FIXME Proper memory ordering semantics

        // which bucket are we looking in?
        int which = timerId & TimerIdMask;
        int bucket = bucketOffset(which);
        at = bucketIndex(bucket, which);
        b = timerIds[bucket];

        if (!b) {
            // allocate a new bucket
            b = allocateBucket(bucket);
            if (!timerIds[bucket].testAndSetRelease(0, b)) {
                // another thread won the race to allocate the bucket
                delete [] b;
                b = timerIds[bucket];
            }
        }

        newTimerId = prepareNewValueWithSerialNumber(timerId, b[at]);
    } while (!nextFreeTimerId.testAndSetRelaxed(timerId, newTimerId));

    b[at] = -timerId;

    return timerId;
}

// Releasing a timer ID requires putting the current ID back in the vector;
// we do it by setting:
//    X[timerId] = nextFreeTimerId;
// then we update nextFreeTimerId to the timer we've just released
//
// The extra code in allocateTimerId and releaseTimerId are ABA prevention
// and bucket memory. The buckets are simply to make sure we allocate only
// the necessary number of timers. See above.
//
// ABA prevention simply adds a value to 7 of the top 8 bits when resetting
// nextFreeTimerId.
void QAbstractEventDispatcherPrivate::releaseTimerId(int timerId)
{
    int which = timerId & TimerIdMask;
    int bucket = bucketOffset(which);
    int at = bucketIndex(bucket, which);
    int *b = timerIds[bucket];

    Q_ASSERT_X(timerId == -b[at], "QAbstractEventDispatcher::releaseTimerId",
               "Internal error: timer ID not found");

    int freeId, newTimerId;
    do {
        freeId = nextFreeTimerId;//.loadAcquire(); // ### FIXME Proper memory ordering semantics
        b[at] = freeId & TimerIdMask;

        newTimerId = prepareNewValueWithSerialNumber(freeId, timerId);
    } while (!nextFreeTimerId.testAndSetRelease(freeId, newTimerId));
}

/*!
    \class QAbstractEventDispatcher
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
    subclass, you must create your instance \e before you create the
    QApplication object.

    The main event loop is started by calling
    QCoreApplication::exec(), and stopped by calling
    QCoreApplication::exit(). Local event loops can be created using
    QEventLoop.

    Programs that perform long operations can call processEvents()
    with a bitwise OR combination of various QEventLoop::ProcessEventsFlag
    values to control which events should be delivered.

    QAbstractEventDispatcher also allows the integration of an
    external event loop with the Qt event loop. For example, the
    \l{Motif Extension}
    includes a reimplementation of QAbstractEventDispatcher that merges Qt and
    Motif events together.

    \sa QEventLoop, QCoreApplication
*/

/*!
    Constructs a new event dispatcher with the given \a parent.
*/
QAbstractEventDispatcher::QAbstractEventDispatcher(QObject *parent)
    : QObject(*new QAbstractEventDispatcherPrivate, parent)
{
    Q_D(QAbstractEventDispatcher);
    d->init();
}

/*!
    \internal
*/
QAbstractEventDispatcher::QAbstractEventDispatcher(QAbstractEventDispatcherPrivate &dd,
                                                   QObject *parent)
    : QObject(dd, parent)
{
    Q_D(QAbstractEventDispatcher);
    d->init();
}

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

    \bold{Note:} If Qt is built without thread support, the \a thread
    argument is ignored.
 */
QAbstractEventDispatcher *QAbstractEventDispatcher::instance(QThread *thread)
{
    QThreadData *data = thread ? QThreadData::get2(thread) : QThreadData::current();
    return data->eventDispatcher;
}

/*!
    \fn bool QAbstractEventDispatcher::processEvents(QEventLoop::ProcessEventsFlags flags)

    Processes pending events that match \a flags until there are no
    more events to process. Returns true if an event was processed;
    otherwise returns false.

    This function is especially useful if you have a long running
    operation and want to show its progress without allowing user
    input; i.e. by using the QEventLoop::ExcludeUserInputEvents flag.

    If the QEventLoop::WaitForMoreEvents flag is set in \a flags, the
    behavior of this function is as follows:

    \list

    \i If events are available, this function returns after processing
    them.

    \i If no events are available, this function will wait until more
    are available and return after processing newly available events.

    \endlist

    If the QEventLoop::WaitForMoreEvents flag is not set in \a flags,
    and no events are available, this function will return
    immediately.

    \bold{Note:} This function does not process events continuously; it
    returns after all available events are processed.

    \sa hasPendingEvents()
*/

/*! \fn bool QAbstractEventDispatcher::hasPendingEvents()

    Returns true if there is an event waiting; otherwise returns
    false.
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
    \fn int QAbstractEventDispatcher::registerTimer(int interval, QObject *object)

    Registers a timer with the specified \a interval for the given \a object.
*/
int QAbstractEventDispatcher::registerTimer(int interval, QObject *object)
{
    int id = QAbstractEventDispatcherPrivate::allocateTimerId();
    registerTimer(id, interval, object);
    return id;
}

/*!
    \fn void QAbstractEventDispatcher::registerTimer(int timerId, int interval, QObject *object)

    Register a timer with the specified \a timerId and \a interval for
    the given \a object.
*/

/*!
    \fn bool QAbstractEventDispatcher::unregisterTimer(int timerId)

    Unregisters the timer with the given \a timerId.
    Returns true if successful; otherwise returns false.

    \sa registerTimer(), unregisterTimers()
*/

/*!
    \fn bool QAbstractEventDispatcher::unregisterTimers(QObject *object)

    Unregisters all the timers associated with the given \a object.
    Returns true if all timers were successful removed; otherwise returns false.

    \sa unregisterTimer(), registeredTimers()
*/

/*!
    \fn QList<TimerInfo> QAbstractEventDispatcher::registeredTimers(QObject *object) const

    Returns a list of registered timers for \a object. The timer ID
    is the first member in each pair; the interval is the second.
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
/*! \internal */
void QAbstractEventDispatcher::startingUp()
{ }

/*! \internal */
void QAbstractEventDispatcher::closingDown()
{ }

/*!
    \typedef QAbstractEventDispatcher::TimerInfo

    Typedef for QPair<int, int>. The first component of
    the pair is the timer ID; the second component is
    the interval.

    \sa registeredTimers()
*/

/*!
    \typedef QAbstractEventDispatcher::EventFilter

    Typedef for a function with the signature

    \snippet doc/src/snippets/code/src_corelib_kernel_qabstracteventdispatcher.cpp 0

    Note that the type of the \a message is platform dependent. The
    following table shows the \a {message}'s type on Windows, Mac, and
    X11. You can do a static cast to these types.

    \table
        \header
            \o Platform
            \o type
        \row
            \o Windows
            \o MSG
        \row
            \o X11
            \o XEvent
        \row
            \o Mac
            \o NSEvent
    \endtable

    

    \sa setEventFilter(), filterEvent()
*/

/*!
    Replaces the event filter function for this
    QAbstractEventDispatcher with \a filter and returns the replaced
    event filter function. Only the current event filter function is
    called. If you want to use both filter functions, save the
    replaced EventFilter in a place where yours can call it.

    The event filter function set here is called for all messages
    taken from the system event loop before the event is dispatched to
    the respective target, including the messages not meant for Qt
    objects.

    The event filter function should return true if the message should
    be filtered, (i.e. stopped). It should return false to allow
    processing the message to continue.

    By default, no event filter function is set (i.e., this function
    returns a null EventFilter the first time it is called).
*/
QAbstractEventDispatcher::EventFilter QAbstractEventDispatcher::setEventFilter(EventFilter filter)
{
    Q_D(QAbstractEventDispatcher);
    EventFilter oldFilter = d->event_filter;
    d->event_filter = filter;
    return oldFilter;
}

/*!
    Sends \a message through the event filter that was set by
    setEventFilter().  If no event filter has been set, this function
    returns false; otherwise, this function returns the result of the
    event filter function.

    Subclasses of QAbstractEventDispatcher \e must call this function
    for \e all messages received from the system to ensure
    compatibility with any extensions that may be used in the
    application.

    Note that the type of \a message is platform dependent. See 
    QAbstractEventDispatcher::EventFilter for details.

    \sa setEventFilter()
*/
bool QAbstractEventDispatcher::filterEvent(void *message)
{
    Q_D(QAbstractEventDispatcher);
    if (d->event_filter) {
        // Raise the loopLevel so that deleteLater() calls in or triggered
        // by event_filter() will be processed from the main event loop.
        QScopedLoopLevelCounter loopLevelCounter(d->threadData);
        return d->event_filter(message);
    }
    return false;
}

/*! \fn void QAbstractEventDispatcher::awake()

    This signal is emitted after the event loop returns from a
    function that could block.

    \sa wakeUp() aboutToBlock()
*/

/*! \fn void QAbstractEventDispatcher::aboutToBlock()

    This signal is emitted before the event loop calls a function that
    could block.

    \sa awake()
*/

QT_END_NAMESPACE
