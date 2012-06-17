/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qthread.h"
#include "qthreadstorage.h"
#include "qmutex.h"
#include "qmutexpool_p.h"
#include "qreadwritelock.h"
#include "qabstracteventdispatcher.h"

#include <qeventloop.h>
#include <qhash.h>

#include "qthread_p.h"
#include "private/qcoreapplication_p.h"

/*
#ifdef Q_OS_WIN32
# include "qt_windows.h"
#else
# include <unistd.h>
# include <netinet/in.h>
# include <sys/utsname.h>
# include <sys/socket.h>
*/
/*
#  elif defined(Q_OS_HPUX)
#   include <sys/pstat.h>
#  elif defined(Q_OS_FREEBSD) || defined(Q_OS_OPENBSD) || defined(Q_OS_MAC)
#   include <sys/sysctl.h>
#  endif
#endif
*/

QT_BEGIN_NAMESPACE

/*
  QThreadData
*/

QThreadData::QThreadData(int initialRefCount)
    : _ref(initialRefCount), thread(0), threadId(0),
      quitNow(false), loopLevel(0), eventDispatcher(0), canWait(true), isAdopted(false)
{
    // fprintf(stderr, "QThreadData %p created\n", this);
}

QThreadData::~QThreadData()
{
    Q_ASSERT(_ref == 0);

    // In the odd case that Qt is running on a secondary thread, the main
    // thread instance will have been dereffed asunder because of the deref in
    // QThreadData::current() and the deref in the pthread_destroy. To avoid
    // crashing during QCoreApplicationData's global static cleanup we need to
    // safeguard the main thread here.. This fix is a bit crude, but it solves
    // the problem...
    if (this->thread == QCoreApplicationPrivate::theMainThread) {
       QCoreApplicationPrivate::theMainThread = 0;
    }

    QThread *t = thread;
    thread = 0;
    delete t;

    for (int i = 0; i < postEventList.size(); ++i) {
        const QPostEvent &pe = postEventList.at(i);
        if (pe.event) {
            --pe.receiver->d_func()->postedEvents;
            pe.event->posted = false;
            delete pe.event;
        }
    }

    // fprintf(stderr, "QThreadData %p destroyed\n", this);
}

void QThreadData::ref()
{
#ifndef QT_NO_THREAD
    (void) _ref.ref();
    Q_ASSERT(_ref != 0);
#endif
}

void QThreadData::deref()
{
#ifndef QT_NO_THREAD
    if (!_ref.deref())
        delete this;
#endif
}

/*
  QAdoptedThread
*/

QAdoptedThread::QAdoptedThread(QThreadData *data)
    : QThread(*new QThreadPrivate(data))
{
    // thread should be running and not finished for the lifetime
    // of the application (even if QCoreApplication goes away)
#ifndef QT_NO_THREAD
    d_func()->running = true;
    d_func()->finished = false;
    init();
#endif

    // fprintf(stderr, "new QAdoptedThread = %p\n", this);
}

QAdoptedThread::~QAdoptedThread()
{
    // fprintf(stderr, "~QAdoptedThread = %p\n", this);
}

QThread *QAdoptedThread::createThreadForAdoption()
{
    QScopedPointer<QThread> t(new QAdoptedThread(0));
    t->moveToThread(t.data());
    return t.take();
}

void QAdoptedThread::run()
{
    // this function should never be called
    qFatal("QAdoptedThread::run(): Internal error, this implementation should never be called.");
}
#ifndef QT_NO_THREAD
/*
  QThreadPrivate
*/

QThreadPrivate::QThreadPrivate(QThreadData *d)
    : QObjectPrivate(), running(false), finished(false), terminated(false),
      isInFinish(false), exited(false), returnCode(-1),
      stackSize(0), priority(QThread::InheritPriority), data(d)
{
#if defined (Q_OS_UNIX)
    thread_id = 0;
#elif defined (Q_WS_WIN)
    handle = 0;
    id = 0;
    waiters = 0;
#endif
#if defined (Q_WS_WIN) || defined (Q_OS_SYMBIAN)
    terminationEnabled = true;
    terminatePending = false;
#endif

    if (!data)
        data = new QThreadData;
}

QThreadPrivate::~QThreadPrivate()
{
    data->deref();
}

/*!
    \class QThread
    \brief The QThread class provides platform-independent threads.

    \ingroup thread

    A QThread represents a separate thread of control within the
    program; it shares data with all the other threads within the
    process but executes independently in the way that a separate
    program does on a multitasking operating system. Instead of
    starting in \c main(), QThreads begin executing in run().  By
    default, run() starts the event loop by calling exec() (see
    below). To create your own threads, subclass QThread and
    reimplement run(). For example:

    \snippet doc/src/snippets/code/src_corelib_thread_qthread.cpp 0

    This will create a QTcpSocket in the thread and then execute the
    thread's event loop. Use the start() method to begin execution.
    Execution ends when you return from run(), just as an application
    does when it leaves main(). QThread will notifiy you via a signal
    when the thread is started(), finished(), and terminated(), or
    you can use isFinished() and isRunning() to query the state of
    the thread. Use wait() to block until the thread has finished
    execution.

    Each thread gets its own stack from the operating system. The
    operating system also determines the default size of the stack.
    You can use setStackSize() to set a custom stack size.

    Each QThread can have its own event loop. You can start the event
    loop by calling exec(); you can stop it by calling exit() or
    quit(). Having an event loop in a thread makes it possible to
    connect signals from other threads to slots in this thread, using
    a mechanism called \l{Qt::QueuedConnection}{queued
    connections}. It also makes it possible to use classes that
    require the event loop, such as QTimer and QTcpSocket, in the
    thread. Note, however, that it is not possible to use any widget
    classes in the thread.

    In extreme cases, you may want to forcibly terminate() an
    executing thread. However, doing so is dangerous and discouraged.
    Please read the documentation for terminate() and
    setTerminationEnabled() for detailed information.

    The static functions currentThreadId() and currentThread() return
    identifiers for the currently executing thread. The former
    returns a platform specific ID for the thread; the latter returns
    a QThread pointer.

    QThread also provides platform independent sleep functions in
    varying resolutions. Use sleep() for full second resolution,
    msleep() for millisecond resolution, and usleep() for microsecond
    resolution.

    \sa {Thread Support in Qt}, QThreadStorage, QMutex, QSemaphore, QWaitCondition,
        {Mandelbrot Example}, {Semaphores Example}, {Wait Conditions Example}
*/

/*!
    \fn Qt::HANDLE QThread::currentThreadId()

    Returns the thread handle of the currently executing thread.

    \warning The handle returned by this function is used for internal
    purposes and should not be used in any application code.

    \warning On Windows, the returned value is a pseudo-handle for the
    current thread. It can't be used for numerical comparison. i.e.,
    this function returns the DWORD (Windows-Thread ID) returned by
    the Win32 function getCurrentThreadId(), not the HANDLE
    (Windows-Thread HANDLE) returned by the Win32 function
    getCurrentThread().
*/

/*!
    \fn int QThread::idealThreadCount()

    Returns the ideal number of threads that can be run on the system. This is done querying
    the number of processor cores, both real and logical, in the system. This function returns -1
    if the number of processor cores could not be detected.
*/

/*!
    \fn void QThread::yieldCurrentThread()

    Yields execution of the current thread to another runnable thread,
    if any. Note that the operating system decides to which thread to
    switch.
*/

/*!
    \fn void QThread::start(Priority priority)

    Begins execution of the thread by calling run(), which should be
    reimplemented in a QThread subclass to contain your code. The
    operating system will schedule the thread according to the \a
    priority parameter. If the thread is already running, this
    function does nothing.

    The effect of the \a priority parameter is dependent on the
    operating system's scheduling policy. In particular, the \a priority
    will be ignored on systems that do not support thread priorities
    (such as on Linux, see http://linux.die.net/man/2/sched_setscheduler
    for more details).

    \sa run(), terminate()
*/

/*!
    \fn void QThread::started()

    This signal is emitted when the thread starts executing.

    \sa finished(), terminated()
*/

/*!
    \fn void QThread::finished()

    This signal is emitted when the thread has finished executing.

    \sa started(), terminated()
*/

/*!
    \fn void QThread::terminated()

    This signal is emitted when the thread is terminated.

    \sa started(), finished()
*/

/*!
    \enum QThread::Priority

    This enum type indicates how the operating system should schedule
    newly created threads.

    \value IdlePriority scheduled only when no other threads are
           running.

    \value LowestPriority scheduled less often than LowPriority.
    \value LowPriority scheduled less often than NormalPriority.

    \value NormalPriority the default priority of the operating
           system.

    \value HighPriority scheduled more often than NormalPriority.
    \value HighestPriority scheduled more often than HighPriority.

    \value TimeCriticalPriority scheduled as often as possible.

    \value InheritPriority use the same priority as the creating
           thread. This is the default.
*/

/*!
    Returns a pointer to a QThread which represents the currently
    executing thread.
*/
QThread *QThread::currentThread()
{
    QThreadData *data = QThreadData::current();
    Q_ASSERT(data != 0);
    return data->thread;
}

/*!
    Constructs a new thread with the given \a parent. The thread does
    not begin executing until start() is called.

    \sa start()
*/
QThread::QThread(QObject *parent)
    : QObject(*(new QThreadPrivate), parent)
{
    Q_D(QThread);
    // fprintf(stderr, "QThreadData %p created for thread %p\n", d->data, this);
    d->data->thread = this;
}

/*! \internal
 */
QThread::QThread(QThreadPrivate &dd, QObject *parent)
    : QObject(dd, parent)
{
    Q_D(QThread);
    // fprintf(stderr, "QThreadData %p taken from private data for thread %p\n", d->data, this);
    d->data->thread = this;
}

/*!
    Destroys the thread.

    Note that deleting a QThread object will not stop the execution
    of the thread it represents. Deleting a running QThread (i.e.
    isFinished() returns false) will probably result in a program
    crash. You can wait() on a thread to make sure that it has
    finished.
*/
QThread::~QThread()
{
    Q_D(QThread);
    {
        QMutexLocker locker(&d->mutex);
        if (d->isInFinish) {
            locker.unlock();
            wait();
            locker.relock();
        }
        if (d->running && !d->finished && !d->data->isAdopted)
            qWarning("QThread: Destroyed while thread is still running");

        d->data->thread = 0;
    }
}

/*!
    Returns true if the thread is finished; otherwise returns false.

    \sa isRunning()
*/
bool QThread::isFinished() const
{
    Q_D(const QThread);
    QMutexLocker locker(&d->mutex);
    return d->finished;
}

/*!
    Returns true if the thread is running; otherwise returns false.

    \sa isFinished()
*/
bool QThread::isRunning() const
{
    Q_D(const QThread);
    QMutexLocker locker(&d->mutex);
#ifdef Q_OS_SYMBIAN
    // app shutdown on Symbian can terminate threads and invalidate their stacks without notification,
    // check the thread is still alive.
    if (d->data->symbian_thread_handle.Handle() && d->data->symbian_thread_handle.ExitType() != EExitPending)
        return false;
#endif
    return d->running;
}

/*!
    Sets the maximum stack size for the thread to \a stackSize. If \a
    stackSize is greater than zero, the maximum stack size is set to
    \a stackSize bytes, otherwise the maximum stack size is
    automatically determined by the operating system.

    \warning Most operating systems place minimum and maximum limits
    on thread stack sizes. The thread will fail to start if the stack
    size is outside these limits.

    \sa stackSize()
*/
void QThread::setStackSize(uint stackSize)
{
    Q_D(QThread);
    QMutexLocker locker(&d->mutex);
    Q_ASSERT_X(!d->running, "QThread::setStackSize",
               "cannot change stack size while the thread is running");
    d->stackSize = stackSize;
}

/*!
    Returns the maximum stack size for the thread (if set with
    setStackSize()); otherwise returns zero.

    \sa setStackSize()
*/
uint QThread::stackSize() const
{
    Q_D(const QThread);
    QMutexLocker locker(&d->mutex);
    return d->stackSize;
}

/*!
    Enters the event loop and waits until exit() is called, returning the value
    that was passed to exit(). The value returned is 0 if exit() is called via
    quit().

    It is necessary to call this function to start event handling.

    \sa quit(), exit()
*/
int QThread::exec()
{
    Q_D(QThread);
    QMutexLocker locker(&d->mutex);
    d->data->quitNow = false;
    if (d->exited) {
        d->exited = false;
        return d->returnCode;
    }
    locker.unlock();

    QEventLoop eventLoop;
    int returnCode = eventLoop.exec();

    locker.relock();
    d->exited = false;
    d->returnCode = -1;
    return returnCode;
}

/*!
    Tells the thread's event loop to exit with a return code.

    After calling this function, the thread leaves the event loop and
    returns from the call to QEventLoop::exec(). The
    QEventLoop::exec() function returns \a returnCode.

    By convention, a \a returnCode of 0 means success, any non-zero value
    indicates an error.

    Note that unlike the C library function of the same name, this
    function \e does return to the caller -- it is event processing
    that stops. 
    
    No QEventLoops will be started anymore in this thread  until 
    QThread::exec() has been called again. If the eventloop in QThread::exec()
    is not running then the next call to QThread::exec() will also return
    immediately.

    \sa quit() QEventLoop
*/
void QThread::exit(int returnCode)
{
    Q_D(QThread);
    QMutexLocker locker(&d->mutex);
    d->exited = true;
    d->returnCode = returnCode;
    d->data->quitNow = true;
    for (int i = 0; i < d->data->eventLoops.size(); ++i) {
        QEventLoop *eventLoop = d->data->eventLoops.at(i);
        eventLoop->exit(returnCode);
    }
}

/*!
    Tells the thread's event loop to exit with return code 0 (success).
    Equivalent to calling QThread::exit(0).

    This function does nothing if the thread does not have an event
    loop.

    \sa exit() QEventLoop
*/
void QThread::quit()
{ exit(); }

/*!
    The starting point for the thread. After calling start(), the
    newly created thread calls this function. The default
    implementation simply calls exec().

    You can reimplemented this function to do other useful
    work. Returning from this method will end the execution of the
    thread.

    \sa start() wait()
*/
void QThread::run()
{
    (void) exec();
}

/*! \internal
    Initializes the QThread system.
*/
#if defined (Q_OS_WIN)
void qt_create_tls();
#endif

void QThread::initialize()
{
    if (qt_global_mutexpool)
        return;
    qt_global_mutexpool = QMutexPool::instance();

#if defined (Q_OS_WIN)
    qt_create_tls();
#endif
}


/*! \internal
    Cleans up the QThread system.
*/
void QThread::cleanup()
{
    qt_global_mutexpool = 0;
}

/*!
    \fn bool QThread::finished() const

    Use isFinished() instead.
*/

/*!
    \fn bool QThread::running() const

    Use isRunning() instead.
*/

/*! \fn void QThread::setPriority(Priority priority)
    \since 4.1

    This function sets the \a priority for a running thread. If the
    thread is not running, this function does nothing and returns
    immediately.  Use start() to start a thread with a specific
    priority.

    The \a priority argument can be any value in the \c
    QThread::Priority enum except for \c InheritPriorty.

    The effect of the \a priority parameter is dependent on the
    operating system's scheduling policy. In particular, the \a priority
    will be ignored on systems that do not support thread priorities
    (such as on Linux, see http://linux.die.net/man/2/sched_setscheduler
    for more details).

    \sa Priority priority() start()
*/

/*!
    \since 4.1

    Returns the priority for a running thread.  If the thread is not
    running, this function returns \c InheritPriority.

    \sa Priority setPriority() start()
*/
QThread::Priority QThread::priority() const
{
    Q_D(const QThread);
    QMutexLocker locker(&d->mutex);

    // mask off the high bits that are used for flags
    return Priority(d->priority & 0xffff);
}

/*!
    \fn void QThread::sleep(unsigned long secs)

    Forces the current thread to sleep for \a secs seconds.

    \sa msleep(), usleep()
*/

/*!
    \fn void QThread::msleep(unsigned long msecs)

    Causes the current thread to sleep for \a msecs milliseconds.

    \sa sleep(), usleep()
*/

/*!
    \fn void QThread::usleep(unsigned long usecs)

    Causes the current thread to sleep for \a usecs microseconds.

    \sa sleep(), msleep()
*/

/*!
    \fn void QThread::terminate()

    Terminates the execution of the thread. The thread may or may not
    be terminated immediately, depending on the operating systems
    scheduling policies. Use QThread::wait() after terminate() for
    synchronous termination.

    When the thread is terminated, all threads waiting for the thread
    to finish will be woken up.

    \warning This function is dangerous and its use is discouraged.
    The thread can be terminated at any point in its code path.
    Threads can be terminated while modifying data. There is no
    chance for the thread to clean up after itself, unlock any held
    mutexes, etc. In short, use this function only if absolutely
    necessary.

    Termination can be explicitly enabled or disabled by calling
    QThread::setTerminationEnabled(). Calling this function while
    termination is disabled results in the termination being
    deferred, until termination is re-enabled. See the documentation
    of QThread::setTerminationEnabled() for more information.

    \sa setTerminationEnabled()
*/

/*!
    \fn bool QThread::wait(unsigned long time)

    Blocks the thread until either of these conditions is met:

    \list
    \o The thread associated with this QThread object has finished
       execution (i.e. when it returns from \l{run()}). This function
       will return true if the thread has finished. It also returns
       true if the thread has not been started yet.
    \o \a time milliseconds has elapsed. If \a time is ULONG_MAX (the
        default), then the wait will never timeout (the thread must
        return from \l{run()}). This function will return false if the
        wait timed out.
    \endlist

    This provides similar functionality to the POSIX \c
    pthread_join() function.

    \sa sleep(), terminate()
*/

/*!
    \fn void QThread::setTerminationEnabled(bool enabled)

    Enables or disables termination of the current thread based on the
    \a enabled parameter. The thread must have been started by
    QThread.

    When \a enabled is false, termination is disabled.  Future calls
    to QThread::terminate() will return immediately without effect.
    Instead, the termination is deferred until termination is enabled.

    When \a enabled is true, termination is enabled.  Future calls to
    QThread::terminate() will terminate the thread normally.  If
    termination has been deferred (i.e. QThread::terminate() was
    called with termination disabled), this function will terminate
    the calling thread \e immediately.  Note that this function will
    not return in this case.

    \sa terminate()
*/

#else // QT_NO_THREAD

QThread::QThread(QObject *parent)
    : QObject(*(new QThreadPrivate), (QObject*)0){
    Q_D(QThread);
    d->data->thread = this;
}

QThread *QThread::currentThread()
{
    return QThreadData::current()->thread;
}

QThreadData* QThreadData::current()
{
    static QThreadData *data = 0; // reinterpret_cast<QThreadData *>(pthread_getspecific(current_thread_data_key));
    if (!data) {
        QScopedPointer<QThreadData> newdata(new QThreadData);
        newdata->thread = new QAdoptedThread(newdata.data());
        data = newdata.take();
        data->deref();
    }
    return data;
}

/*! \internal
 */
QThread::QThread(QThreadPrivate &dd, QObject *parent)
    : QObject(dd, parent)
{
    Q_D(QThread);
    // fprintf(stderr, "QThreadData %p taken from private data for thread %p\n", d->data, this);
    d->data->thread = this;
}

#endif // QT_NO_THREAD

QT_END_NAMESPACE
