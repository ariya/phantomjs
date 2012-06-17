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

#include "qthreadpool.h"
#include "qthreadpool_p.h"
#include "qelapsedtimer.h"

#ifndef QT_NO_THREAD

QT_BEGIN_NAMESPACE

inline bool operator<(int priority, const QPair<QRunnable *, int> &p)
{
    return p.second < priority;
}
inline bool operator<(const QPair<QRunnable *, int> &p, int priority)
{
    return priority < p.second;
}

Q_GLOBAL_STATIC(QThreadPool, theInstance)

/*
    QThread wrapper, provides synchronizitaion against a ThreadPool
*/
class QThreadPoolThread : public QThread
{
public:
    QThreadPoolThread(QThreadPoolPrivate *manager);
    void run();
    void registerTheadInactive();

    QThreadPoolPrivate *manager;
    QRunnable *runnable;
};

/*
    QThreadPool private class.
*/


/*!\internal

*/
QThreadPoolThread::QThreadPoolThread(QThreadPoolPrivate *manager)
    :manager(manager), runnable(0)
{ }

/* \internal

*/
void QThreadPoolThread::run()
{
    QMutexLocker locker(&manager->mutex);
    for(;;) {
        QRunnable *r = runnable;
        runnable = 0;

        do {
            if (r) {
                const bool autoDelete = r->autoDelete();


                // run the task
                locker.unlock();
#ifndef QT_NO_EXCEPTIONS
                try {
#endif
                    r->run();
#ifndef QT_NO_EXCEPTIONS
                } catch (...) {
                    qWarning("Qt Concurrent has caught an exception thrown from a worker thread.\n"
                             "This is not supported, exceptions thrown in worker threads must be\n"
                             "caught before control returns to Qt Concurrent.");
                    registerTheadInactive();
                    throw;
                }
#endif
                locker.relock();

                if (autoDelete && !--r->ref)
                    delete r;
            }

            // if too many threads are active, expire this thread
            if (manager->tooManyThreadsActive())
                break;

            r = !manager->queue.isEmpty() ? manager->queue.takeFirst().first : 0;
        } while (r != 0);

        if (manager->isExiting) {
            registerTheadInactive();
            break;
        }

        // if too many threads are active, expire this thread
        bool expired = manager->tooManyThreadsActive();
        if (!expired) {
            ++manager->waitingThreads;
            registerTheadInactive();
            // wait for work, exiting after the expiry timeout is reached
            expired = !manager->runnableReady.wait(locker.mutex(), manager->expiryTimeout);
            ++manager->activeThreads;
    
            if (expired)
                --manager->waitingThreads;
        }
        if (expired) {
            manager->expiredThreads.enqueue(this);
            registerTheadInactive();
            break;
        }
    }
}

void QThreadPoolThread::registerTheadInactive()
{
    if (--manager->activeThreads == 0)
        manager->noActiveThreads.wakeAll();
}


/* \internal

*/
QThreadPoolPrivate:: QThreadPoolPrivate()
    : isExiting(false),
      expiryTimeout(30000),
      maxThreadCount(qAbs(QThread::idealThreadCount())),
      reservedThreads(0),
      waitingThreads(0),
      activeThreads(0)
{ }

bool QThreadPoolPrivate::tryStart(QRunnable *task)
{
    if (allThreads.isEmpty()) {
        // always create at least one thread
        startThread(task);
        return true;
    }

    // can't do anything if we're over the limit
    if (activeThreadCount() >= maxThreadCount)
        return false;

    if (waitingThreads > 0) {
        // recycle an available thread
        --waitingThreads;
        enqueueTask(task);
        return true;
    }

    if (!expiredThreads.isEmpty()) {
        // restart an expired thread
        QThreadPoolThread *thread = expiredThreads.dequeue();
        Q_ASSERT(thread->runnable == 0);

        ++activeThreads;

        if (task->autoDelete())
            ++task->ref;
        thread->runnable = task;
        thread->start();
        return true;
    }

    // start a new thread
    startThread(task);
    return true;
}

void QThreadPoolPrivate::enqueueTask(QRunnable *runnable, int priority)
{
    if (runnable->autoDelete())
        ++runnable->ref;

    // put it on the queue
    QList<QPair<QRunnable *, int> >::iterator at =
        qUpperBound(queue.begin(), queue.end(), priority);
    queue.insert(at, qMakePair(runnable, priority));
    runnableReady.wakeOne();
}

int QThreadPoolPrivate::activeThreadCount() const
{
    // To improve scalability this function is called without holding 
    // the mutex lock -- keep it thread-safe.
    return (allThreads.count()
            - expiredThreads.count()
            - waitingThreads
            + reservedThreads);
}

void QThreadPoolPrivate::tryToStartMoreThreads()
{
    // try to push tasks on the queue to any available threads
    while (!queue.isEmpty() && tryStart(queue.first().first))
        queue.removeFirst();
}

bool QThreadPoolPrivate::tooManyThreadsActive() const
{
    const int activeThreadCount = this->activeThreadCount();
    return activeThreadCount > maxThreadCount && (activeThreadCount - reservedThreads) > 1;
}

/*! \internal

*/
void QThreadPoolPrivate::startThread(QRunnable *runnable)
{
    QScopedPointer <QThreadPoolThread> thread(new QThreadPoolThread(this));
    thread->setObjectName(QLatin1String("Thread (pooled)"));
    allThreads.insert(thread.data());
    ++activeThreads;

    if (runnable->autoDelete())
        ++runnable->ref;
    thread->runnable = runnable;
    thread.take()->start();
}

/*! \internal
    Makes all threads exit, waits for each tread to exit and deletes it.
*/
void QThreadPoolPrivate::reset()
{
    QMutexLocker locker(&mutex);
    isExiting = true;
    runnableReady.wakeAll();

    do {
        // make a copy of the set so that we can iterate without the lock
        QSet<QThreadPoolThread *> allThreadsCopy = allThreads;
        allThreads.clear();
        locker.unlock();

        foreach (QThreadPoolThread *thread, allThreadsCopy) {
            thread->wait();
            delete thread;
        }

        locker.relock();
        // repeat until all newly arrived threads have also completed
    } while (!allThreads.isEmpty());

    waitingThreads = 0;
    expiredThreads.clear();

    isExiting = false;
}

bool QThreadPoolPrivate::waitForDone(int msecs)
{
    QMutexLocker locker(&mutex);
    if (msecs < 0) {
        while (!(queue.isEmpty() && activeThreads == 0))
            noActiveThreads.wait(locker.mutex());
    } else {
        QElapsedTimer timer;
        timer.start();
        int t;
        while (!(queue.isEmpty() && activeThreads == 0) && 
               ((t = msecs - timer.elapsed()) > 0))
            noActiveThreads.wait(locker.mutex(), t);
    }
    return queue.isEmpty() && activeThreads == 0;
}

/*! \internal
    Pulls a runnable from the front queue and runs it in the current thread. Blocks
    until the runnable has completed. Returns true if a runnable was found.
*/
bool QThreadPoolPrivate::startFrontRunnable()
{
    QMutexLocker locker(&mutex);
    if (queue.isEmpty())
        return false;

    QRunnable *runnable = queue.takeFirst().first;
    const bool autoDelete = runnable->autoDelete();
    bool del = autoDelete && !--runnable->ref;

    locker.unlock();
    runnable->run();
    locker.relock();

    if (del) {
        delete runnable;
    }

    return true;
}

/*! \internal
    Seaches for \a runnable in the queue, removes it from the queue and
    runs it if found. This functon does not return until the runnable
    has completed.
*/
void QThreadPoolPrivate::stealRunnable(QRunnable *runnable)
{
    if (runnable == 0 || queue.isEmpty())
        return;
    bool found = false;
    {
        QMutexLocker locker(&mutex);
        QList<QPair<QRunnable *, int> >::iterator it = queue.begin();
        QList<QPair<QRunnable *, int> >::iterator end = queue.end();

        while (it != end) {
            if (it->first == runnable) {
                found = true;
                queue.erase(it);
                break;
            }
            ++it;
        }
    }

    if (!found)
        return;

    const bool autoDelete = runnable->autoDelete();
    bool del = autoDelete && !--runnable->ref;

    runnable->run();

    if (del) {
        delete runnable;
    }
}

/*!
    \class QThreadPool
    \brief The QThreadPool class manages a collection of QThreads.
    \since 4.4
    \threadsafe

    \ingroup thread

    QThreadPool manages and recyles individual QThread objects to help reduce
    thread creation costs in programs that use threads. Each Qt application
    has one global QThreadPool object, which can be accessed by calling
    globalInstance().

    To use one of the QThreadPool threads, subclass QRunnable and implement
    the run() virtual function. Then create an object of that class and pass
    it to QThreadPool::start().

    \snippet doc/src/snippets/code/src_corelib_concurrent_qthreadpool.cpp 0

    QThreadPool deletes the QRunnable automatically by default. Use 
    QRunnable::setAutoDelete() to change the auto-deletion flag.

    QThreadPool supports executing the same QRunnable more than once
    by calling tryStart(this) from within QRunnable::run(). 
    If autoDelete is enabled the QRunnable will be deleted when
    the last thread exits the run function. Calling start()
    multiple times with the same QRunnable when autoDelete is enabled
    creates a race condition and is not recommended.

    Threads that are unused for a certain amount of time will expire. The
    default expiry timeout is 30000 milliseconds (30 seconds). This can be
    changed using setExpiryTimeout(). Setting a negative expiry timeout
    disables the expiry mechanism.

    Call maxThreadCount() to query the maximum number of threads to be used.
    If needed, you can change the limit with setMaxThreadCount(). The default
    maxThreadCount() is QThread::idealThreadCount(). The activeThreadCount()
    function returns the number of threads currently doing work.

    The reserveThread() function reserves a thread for external
    use. Use releaseThread() when your are done with the thread, so
    that it may be reused.  Essentially, these functions temporarily
    increase or reduce the active thread count and are useful when
    implementing time-consuming operations that are not visible to the
    QThreadPool.

    Note that QThreadPool is a low-level class for managing threads, see
    QtConcurrent::run() or the other
    \l {Concurrent Programming}{Qt Concurrent} APIs for higher
    level alternatives.

    \sa QRunnable
*/

/*!
    Constructs a thread pool with the given \a parent.
*/
QThreadPool::QThreadPool(QObject *parent)
    : QObject(*new QThreadPoolPrivate, parent)
{ }

/*!
    Destroys the QThreadPool.
    This function will block until all runnables have been completed.
*/
QThreadPool::~QThreadPool()
{
    d_func()->waitForDone();
    d_func()->reset();
}

/*!
    Returns the global QThreadPool instance.
*/
QThreadPool *QThreadPool::globalInstance()
{
    return theInstance();
}

/*!
    Reserves a thread and uses it to run \a runnable, unless this thread will
    make the current thread count exceed maxThreadCount().  In that case,
    \a runnable is added to a run queue instead. The \a priority argument can
    be used to control the run queue's order of execution.

    Note that the thread pool takes ownership of the \a runnable if
    \l{QRunnable::autoDelete()}{runnable->autoDelete()} returns true,
    and the \a runnable will be deleted automatically by the thread
    pool after the \l{QRunnable::run()}{runnable->run()} returns. If
    \l{QRunnable::autoDelete()}{runnable->autoDelete()} returns false,
    ownership of \a runnable remains with the caller. Note that
    changing the auto-deletion on \a runnable after calling this
    functions results in undefined behavior.
*/
void QThreadPool::start(QRunnable *runnable, int priority)
{
    if (!runnable)
        return;

    Q_D(QThreadPool);
    QMutexLocker locker(&d->mutex);
    if (!d->tryStart(runnable))
        d->enqueueTask(runnable, priority);
}

/*!
    Attempts to reserve a thread to run \a runnable.

    If no threads are available at the time of calling, then this function
    does nothing and returns false.  Otherwise, \a runnable is run immediately
    using one available thread and this function returns true.

    Note that the thread pool takes ownership of the \a runnable if
    \l{QRunnable::autoDelete()}{runnable->autoDelete()} returns true,
    and the \a runnable will be deleted automatically by the thread
    pool after the \l{QRunnable::run()}{runnable->run()} returns. If
    \l{QRunnable::autoDelete()}{runnable->autoDelete()} returns false,
    ownership of \a runnable remains with the caller. Note that
    changing the auto-deletion on \a runnable after calling this
    function results in undefined behavior.
*/
bool QThreadPool::tryStart(QRunnable *runnable)
{
    if (!runnable)
        return false;

    Q_D(QThreadPool);

    // To improve scalability perform a check on the thread count
    // before locking the mutex.
    if (d->allThreads.isEmpty() == false && d->activeThreadCount() >= d->maxThreadCount)
        return false;

    QMutexLocker locker(&d->mutex);
    return d->tryStart(runnable);
}

/*! \property QThreadPool::expiryTimeout

    Threads that are unused for \a expiryTimeout milliseconds are considered
    to have expired and will exit. Such threads will be restarted as needed.
    The default \a expiryTimeout is 30000 milliseconds (30 seconds). If
    \a expiryTimeout is negative, newly created threads will not expire, e.g.,
    they will not exit until the thread pool is destroyed.

    Note that setting \a expiryTimeout has no effect on already running
    threads. Only newly created threads will use the new \a expiryTimeout.
    We recommend setting the \a expiryTimeout immediately after creating the
    thread pool, but before calling start().
*/

int QThreadPool::expiryTimeout() const
{
    Q_D(const QThreadPool);
    return d->expiryTimeout;
}

void QThreadPool::setExpiryTimeout(int expiryTimeout)
{
    Q_D(QThreadPool);
    if (d->expiryTimeout == expiryTimeout)
        return;
    d->expiryTimeout = expiryTimeout;
}

/*! \property QThreadPool::maxThreadCount

    This property represents the maximum number of threads used by the thread
    pool.

    \note The thread pool will always use at least 1 thread, even if
    \a maxThreadCount limit is zero or negative.

    The default \a maxThreadCount is QThread::idealThreadCount().
*/

int QThreadPool::maxThreadCount() const
{
    Q_D(const QThreadPool);
    return d->maxThreadCount;
}

void QThreadPool::setMaxThreadCount(int maxThreadCount)
{
    Q_D(QThreadPool);
    QMutexLocker locker(&d->mutex);

    if (maxThreadCount == d->maxThreadCount)
        return;

    d->maxThreadCount = maxThreadCount;
    d->tryToStartMoreThreads();
}

/*! \property QThreadPool::activeThreadCount

    This property represents the number of active threads in the thread pool.

    \note It is possible for this function to return a value that is greater
    than maxThreadCount(). See reserveThread() for more details.

    \sa reserveThread(), releaseThread()
*/

int QThreadPool::activeThreadCount() const
{
    Q_D(const QThreadPool);
    return d->activeThreadCount();
}

/*!
    Reserves one thread, disregarding activeThreadCount() and maxThreadCount().

    Once you are done with the thread, call releaseThread() to allow it to be
    reused.

    \note This function will always increase the number of active threads.
    This means that by using this function, it is possible for
    activeThreadCount() to return a value greater than maxThreadCount() .

    \sa releaseThread()
 */
void QThreadPool::reserveThread()
{
    Q_D(QThreadPool);
    QMutexLocker locker(&d->mutex);
    ++d->reservedThreads;
}

/*!
    Releases a thread previously reserved by a call to reserveThread().

    \note Calling this function without previously reserving a thread
    temporarily increases maxThreadCount(). This is useful when a
    thread goes to sleep waiting for more work, allowing other threads
    to continue. Be sure to call reserveThread() when done waiting, so
    that the thread pool can correctly maintain the
    activeThreadCount().

    \sa reserveThread()
*/
void QThreadPool::releaseThread()
{
    Q_D(QThreadPool);
    QMutexLocker locker(&d->mutex);
    --d->reservedThreads;
    d->tryToStartMoreThreads();
}

/*!
    Waits for each thread to exit and removes all threads from the thread pool.
*/
void QThreadPool::waitForDone()
{
    Q_D(QThreadPool);
    d->waitForDone();
    d->reset();
}

/*!
    \overload waitForDone()
    \since 4.8

    Waits up to \a msecs milliseconds for all threads to exit and removes all 
    threads from the thread pool. Returns true if all threads were removed; 
    otherwise it returns false.
*/
bool QThreadPool::waitForDone(int msecs)
{
    Q_D(QThreadPool);
    bool rc = d->waitForDone(msecs);
    if (rc)
      d->reset();
    return rc;
}

QT_END_NAMESPACE

#endif
