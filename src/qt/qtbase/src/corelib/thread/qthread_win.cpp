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

//#define WINVER 0x0500
#if (_WIN32_WINNT < 0x0400) && !defined(Q_OS_WINRT)
#define _WIN32_WINNT 0x0400
#endif


#include "qthread.h"
#include "qthread_p.h"
#include "qthreadstorage.h"
#include "qmutex.h"

#include <qcoreapplication.h>
#include <qpointer.h>

#include <private/qcoreapplication_p.h>
#ifdef Q_OS_WINRT
#include <private/qeventdispatcher_winrt_p.h>
#else
#include <private/qeventdispatcher_win_p.h>
#endif

#include <qt_windows.h>

#ifdef Q_OS_WINRT
#include <qelapsedtimer.h>
#include <thread>
#endif

#ifndef Q_OS_WINCE
#ifndef _MT
#define _MT
#endif
#include <process.h>
#else
#include "qfunctions_wince.h"
#endif

#ifndef QT_NO_THREAD
QT_BEGIN_NAMESPACE

#ifndef Q_OS_WINRT
void qt_watch_adopted_thread(const HANDLE adoptedThreadHandle, QThread *qthread);
DWORD WINAPI qt_adopted_thread_watcher_function(LPVOID);

static DWORD qt_current_thread_data_tls_index = TLS_OUT_OF_INDEXES;
void qt_create_tls()
{
    if (qt_current_thread_data_tls_index != TLS_OUT_OF_INDEXES)
        return;
    static QMutex mutex;
    QMutexLocker locker(&mutex);
    qt_current_thread_data_tls_index = TlsAlloc();
}

static void qt_free_tls()
{
    if (qt_current_thread_data_tls_index != TLS_OUT_OF_INDEXES) {
        TlsFree(qt_current_thread_data_tls_index);
        qt_current_thread_data_tls_index = TLS_OUT_OF_INDEXES;
    }
}
Q_DESTRUCTOR_FUNCTION(qt_free_tls)
#else // !Q_OS_WINRT

__declspec(thread) static QThreadData* qt_current_thread_data_tls_index = 0;
void qt_create_tls()
{
}

static void qt_free_tls()
{
    if (qt_current_thread_data_tls_index) {
        qt_current_thread_data_tls_index->deref();
        qt_current_thread_data_tls_index = 0;
    }
}

QThreadData* TlsGetValue(QThreadData*& tls)
{
    Q_ASSERT(tls == qt_current_thread_data_tls_index);
    return tls;
}

void TlsSetValue(QThreadData*& tls, QThreadData* data)
{
    Q_ASSERT(tls == qt_current_thread_data_tls_index);
    if (tls)
        tls->deref();
    tls = data;
    if (tls)
        tls->ref();
}
Q_DESTRUCTOR_FUNCTION(qt_free_tls)
#endif // Q_OS_WINRT

/*
    QThreadData
*/
void QThreadData::clearCurrentThreadData()
{
    TlsSetValue(qt_current_thread_data_tls_index, 0);
}

QThreadData *QThreadData::current(bool createIfNecessary)
{
    qt_create_tls();
    QThreadData *threadData = reinterpret_cast<QThreadData *>(TlsGetValue(qt_current_thread_data_tls_index));
    if (!threadData && createIfNecessary) {
        threadData = new QThreadData;
        // This needs to be called prior to new AdoptedThread() to
        // avoid recursion.
        TlsSetValue(qt_current_thread_data_tls_index, threadData);
        QT_TRY {
            threadData->thread = new QAdoptedThread(threadData);
        } QT_CATCH(...) {
            TlsSetValue(qt_current_thread_data_tls_index, 0);
            threadData->deref();
            threadData = 0;
            QT_RETHROW;
        }
        threadData->deref();
        threadData->isAdopted = true;
        threadData->threadId = reinterpret_cast<Qt::HANDLE>(GetCurrentThreadId());

        if (!QCoreApplicationPrivate::theMainThread) {
            QCoreApplicationPrivate::theMainThread = threadData->thread;
#ifndef Q_OS_WINRT
            // TODO: is there a way to reflect the branch's behavior using
            // WinRT API?
        } else {
            HANDLE realHandle = INVALID_HANDLE_VALUE;
#if !defined(Q_OS_WINCE) || (defined(_WIN32_WCE) && (_WIN32_WCE>=0x600))
            DuplicateHandle(GetCurrentProcess(),
                    GetCurrentThread(),
                    GetCurrentProcess(),
                    &realHandle,
                    0,
                    FALSE,
                    DUPLICATE_SAME_ACCESS);
#else
                        realHandle = reinterpret_cast<HANDLE>(GetCurrentThreadId());
#endif
            qt_watch_adopted_thread(realHandle, threadData->thread);
#endif // !Q_OS_WINRT
        }
    }
    return threadData;
}

void QAdoptedThread::init()
{
#ifndef Q_OS_WINRT
    d_func()->handle = GetCurrentThread();
    d_func()->id = GetCurrentThreadId();
#else
    d_func()->handle = nullptr;
    d_func()->id = std::this_thread::get_id();
#endif
}

#ifndef Q_OS_WINRT
static QVector<HANDLE> qt_adopted_thread_handles;
static QVector<QThread *> qt_adopted_qthreads;
static QMutex qt_adopted_thread_watcher_mutex;
static DWORD qt_adopted_thread_watcher_id = 0;
static HANDLE qt_adopted_thread_wakeup = 0;

/*!
    \internal
    Adds an adopted thread to the list of threads that Qt watches to make sure
    the thread data is properly cleaned up. This function starts the watcher
    thread if necessary.
*/
void qt_watch_adopted_thread(const HANDLE adoptedThreadHandle, QThread *qthread)
{
    QMutexLocker lock(&qt_adopted_thread_watcher_mutex);

    if (GetCurrentThreadId() == qt_adopted_thread_watcher_id) {
#if !defined(Q_OS_WINCE) || (defined(_WIN32_WCE) && (_WIN32_WCE>=0x600))
        CloseHandle(adoptedThreadHandle);
#endif
        return;
    }

    qt_adopted_thread_handles.append(adoptedThreadHandle);
    qt_adopted_qthreads.append(qthread);

    // Start watcher thread if it is not already running.
    if (qt_adopted_thread_watcher_id == 0) {
        if (qt_adopted_thread_wakeup == 0) {
            qt_adopted_thread_wakeup = CreateEvent(0, false, false, 0);
            qt_adopted_thread_handles.prepend(qt_adopted_thread_wakeup);
        }

        CloseHandle(CreateThread(0, 0, qt_adopted_thread_watcher_function, 0, 0, &qt_adopted_thread_watcher_id));
    } else {
        SetEvent(qt_adopted_thread_wakeup);
    }
}

/*
    This function loops and waits for native adopted threads to finish.
    When this happens it derefs the QThreadData for the adopted thread
    to make sure it gets cleaned up properly.
*/
DWORD WINAPI qt_adopted_thread_watcher_function(LPVOID)
{
    forever {
        qt_adopted_thread_watcher_mutex.lock();

        if (qt_adopted_thread_handles.count() == 1) {
            qt_adopted_thread_watcher_id = 0;
            qt_adopted_thread_watcher_mutex.unlock();
            break;
        }

        QVector<HANDLE> handlesCopy = qt_adopted_thread_handles;
        qt_adopted_thread_watcher_mutex.unlock();

        DWORD ret = WAIT_TIMEOUT;
        int count;
        int offset;
        int loops = handlesCopy.size() / MAXIMUM_WAIT_OBJECTS;
        if (handlesCopy.size() % MAXIMUM_WAIT_OBJECTS)
            ++loops;
        if (loops == 1) {
            // no need to loop, no timeout
            offset = 0;
            count = handlesCopy.count();
            ret = WaitForMultipleObjects(handlesCopy.count(), handlesCopy.constData(), false, INFINITE);
        } else {
            int loop = 0;
            do {
                offset = loop * MAXIMUM_WAIT_OBJECTS;
                count = qMin(handlesCopy.count() - offset, MAXIMUM_WAIT_OBJECTS);
                ret = WaitForMultipleObjects(count, handlesCopy.constData() + offset, false, 100);
                loop = (loop + 1) % loops;
            } while (ret == WAIT_TIMEOUT);
        }

        if (ret == WAIT_FAILED || ret >= WAIT_OBJECT_0 + uint(count)) {
            qWarning("QThread internal error while waiting for adopted threads: %d", int(GetLastError()));
            continue;
        }

        const int handleIndex = offset + ret - WAIT_OBJECT_0;
        if (handleIndex == 0){
            // New handle to watch was added.
            continue;
        } else {
//             printf("(qt) - qt_adopted_thread_watcher_function... called\n");
            const int qthreadIndex = handleIndex - 1;

            qt_adopted_thread_watcher_mutex.lock();
            QThreadData *data = QThreadData::get2(qt_adopted_qthreads.at(qthreadIndex));
            qt_adopted_thread_watcher_mutex.unlock();
            if (data->isAdopted) {
                QThread *thread = data->thread;
                Q_ASSERT(thread);
                QThreadPrivate *thread_p = static_cast<QThreadPrivate *>(QObjectPrivate::get(thread));
                Q_UNUSED(thread_p)
                Q_ASSERT(!thread_p->finished);
                thread_p->finish(thread);
            }
            data->deref();

            QMutexLocker lock(&qt_adopted_thread_watcher_mutex);
#if !defined(Q_OS_WINCE) || (defined(_WIN32_WCE) && (_WIN32_WCE>=0x600))
            CloseHandle(qt_adopted_thread_handles.at(handleIndex));
#endif
            qt_adopted_thread_handles.remove(handleIndex);
            qt_adopted_qthreads.remove(qthreadIndex);
        }
    }

    QThreadData *threadData = reinterpret_cast<QThreadData *>(TlsGetValue(qt_current_thread_data_tls_index));
    if (threadData)
        threadData->deref();

    return 0;
}

#if !defined(QT_NO_DEBUG) && defined(Q_CC_MSVC) && !defined(Q_OS_WINCE)

#ifndef Q_OS_WIN64
#  define ULONG_PTR DWORD
#endif

typedef struct tagTHREADNAME_INFO
{
    DWORD dwType;      // must be 0x1000
    LPCSTR szName;     // pointer to name (in user addr space)
    HANDLE dwThreadID; // thread ID (-1=caller thread)
    DWORD dwFlags;     // reserved for future use, must be zero
} THREADNAME_INFO;

void qt_set_thread_name(HANDLE threadId, LPCSTR threadName)
{
    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = threadName;
    info.dwThreadID = threadId;
    info.dwFlags = 0;

    __try
    {
        RaiseException(0x406D1388, 0, sizeof(info)/sizeof(DWORD), (const ULONG_PTR*)&info);
    }
    __except (EXCEPTION_CONTINUE_EXECUTION)
    {
    }
}
#endif // !QT_NO_DEBUG && Q_CC_MSVC && !Q_OS_WINCE
#endif // !Q_OS_WINRT

/**************************************************************************
 ** QThreadPrivate
 *************************************************************************/

#endif // QT_NO_THREAD

void QThreadPrivate::createEventDispatcher(QThreadData *data)
{
#ifdef Q_OS_WINRT
    QEventDispatcherWinRT *theEventDispatcher = new QEventDispatcherWinRT;
#else
    QEventDispatcherWin32 *theEventDispatcher = new QEventDispatcherWin32;
#endif
    data->eventDispatcher.storeRelease(theEventDispatcher);
    theEventDispatcher->startingUp();
}

#ifndef QT_NO_THREAD

unsigned int __stdcall QT_ENSURE_STACK_ALIGNED_FOR_SSE QThreadPrivate::start(void *arg)
{
    QThread *thr = reinterpret_cast<QThread *>(arg);
    QThreadData *data = QThreadData::get2(thr);

    qt_create_tls();
    TlsSetValue(qt_current_thread_data_tls_index, data);
    data->threadId = reinterpret_cast<Qt::HANDLE>(GetCurrentThreadId());

    QThread::setTerminationEnabled(false);

    {
        QMutexLocker locker(&thr->d_func()->mutex);
        data->quitNow = thr->d_func()->exited;
    }

    if (data->eventDispatcher.load()) // custom event dispatcher set?
        data->eventDispatcher.load()->startingUp();
    else
        createEventDispatcher(data);

#if !defined(QT_NO_DEBUG) && defined(Q_CC_MSVC) && !defined(Q_OS_WINCE) && !defined(Q_OS_WINRT)
    // sets the name of the current thread.
    QByteArray objectName = thr->objectName().toLocal8Bit();
    qt_set_thread_name((HANDLE)-1,
                       objectName.isEmpty() ?
                       thr->metaObject()->className() : objectName.constData());
#endif

    emit thr->started(QThread::QPrivateSignal());
    QThread::setTerminationEnabled(true);
    thr->run();

    finish(arg);
    return 0;
}

void QThreadPrivate::finish(void *arg, bool lockAnyway)
{
    QThread *thr = reinterpret_cast<QThread *>(arg);
    QThreadPrivate *d = thr->d_func();

    QMutexLocker locker(lockAnyway ? &d->mutex : 0);
    d->isInFinish = true;
    d->priority = QThread::InheritPriority;
    void **tls_data = reinterpret_cast<void **>(&d->data->tls);
    locker.unlock();
    emit thr->finished(QThread::QPrivateSignal());
    QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
    QThreadStorageData::finish(tls_data);
    locker.relock();

    QAbstractEventDispatcher *eventDispatcher = d->data->eventDispatcher.load();
    if (eventDispatcher) {
        d->data->eventDispatcher = 0;
        locker.unlock();
        eventDispatcher->closingDown();
        delete eventDispatcher;
        locker.relock();
    }

    d->running = false;
    d->finished = true;
    d->isInFinish = false;
    d->interruptionRequested = false;

    if (!d->waiters) {
#ifndef Q_OS_WINRT
        CloseHandle(d->handle);
#else
        d->handle->detach();
        delete d->handle;
#endif
        d->handle = 0;
    }

#ifndef Q_OS_WINRT
    d->id = 0;
#else
    d->id = std::thread::id();
#endif

}

/**************************************************************************
 ** QThread
 *************************************************************************/

Qt::HANDLE QThread::currentThreadId() Q_DECL_NOTHROW
{
    return reinterpret_cast<Qt::HANDLE>(GetCurrentThreadId());
}

int QThread::idealThreadCount() Q_DECL_NOTHROW
{
    SYSTEM_INFO sysinfo;
#ifndef Q_OS_WINRT
    GetSystemInfo(&sysinfo);
#else
    GetNativeSystemInfo(&sysinfo);
#endif
    return sysinfo.dwNumberOfProcessors;
}

#ifndef Q_OS_WINRT
void QThread::yieldCurrentThread()
{
#ifndef Q_OS_WINCE
    SwitchToThread();
#else
    ::Sleep(0);
#endif
}

void QThread::sleep(unsigned long secs)
{
    ::Sleep(secs * 1000);
}

void QThread::msleep(unsigned long msecs)
{
    ::Sleep(msecs);
}

void QThread::usleep(unsigned long usecs)
{
    ::Sleep((usecs / 1000) + 1);
}
#else // !Q_OS_WINRT

void QThread::yieldCurrentThread()
{
    msleep(1);
}

void QThread::sleep(unsigned long secs)
{
    msleep(secs * 1000);
}

void QThread::msleep(unsigned long msecs)
{
    WaitForSingleObjectEx(GetCurrentThread(), msecs, FALSE);
}

void QThread::usleep(unsigned long usecs)
{
    msleep((usecs / 1000) + 1);
}
#endif // Q_OS_WINRT

void QThread::start(Priority priority)
{
    Q_D(QThread);
    QMutexLocker locker(&d->mutex);

    if (d->isInFinish) {
        locker.unlock();
        wait();
        locker.relock();
    }

    if (d->running)
        return;

    d->running = true;
    d->finished = false;
    d->exited = false;
    d->returnCode = 0;
    d->interruptionRequested = false;

#ifndef Q_OS_WINRT
    /*
      NOTE: we create the thread in the suspended state, set the
      priority and then resume the thread.

      since threads are created with normal priority by default, we
      could get into a case where a thread (with priority less than
      NormalPriority) tries to create a new thread (also with priority
      less than NormalPriority), but the newly created thread preempts
      its 'parent' and runs at normal priority.
    */
    d->handle = (Qt::HANDLE) _beginthreadex(NULL, d->stackSize, QThreadPrivate::start,
                                            this, CREATE_SUSPENDED, &(d->id));

    if (!d->handle) {
        qErrnoWarning(errno, "QThread::start: Failed to create thread");
        d->running = false;
        d->finished = true;
        return;
    }

    int prio;
    d->priority = priority;
    switch (d->priority) {
    case IdlePriority:
        prio = THREAD_PRIORITY_IDLE;
        break;

    case LowestPriority:
        prio = THREAD_PRIORITY_LOWEST;
        break;

    case LowPriority:
        prio = THREAD_PRIORITY_BELOW_NORMAL;
        break;

    case NormalPriority:
        prio = THREAD_PRIORITY_NORMAL;
        break;

    case HighPriority:
        prio = THREAD_PRIORITY_ABOVE_NORMAL;
        break;

    case HighestPriority:
        prio = THREAD_PRIORITY_HIGHEST;
        break;

    case TimeCriticalPriority:
        prio = THREAD_PRIORITY_TIME_CRITICAL;
        break;

    case InheritPriority:
    default:
        prio = GetThreadPriority(GetCurrentThread());
        break;
    }

    if (!SetThreadPriority(d->handle, prio)) {
        qErrnoWarning("QThread::start: Failed to set thread priority");
    }

    if (ResumeThread(d->handle) == (DWORD) -1) {
        qErrnoWarning("QThread::start: Failed to resume new thread");
    }
#else // !Q_OS_WINRT
    d->handle = new std::thread(QThreadPrivate::start, this);

    if (!d->handle) {
        qErrnoWarning(errno, "QThread::start: Failed to create thread");
        d->running = false;
        d->finished = true;
        return;
    }

    d->id = d->handle->get_id();

    if (priority != NormalPriority || priority != InheritPriority) {
        qWarning("QThread::start: Failed to set thread priority (not implemented)");
        d->priority = NormalPriority;
    }
#endif // Q_OS_WINRT
}

void QThread::terminate()
{
    Q_D(QThread);
    QMutexLocker locker(&d->mutex);
    if (!d->running)
        return;
    if (!d->terminationEnabled) {
        d->terminatePending = true;
        return;
    }

#ifndef Q_OS_WINRT
    TerminateThread(d->handle, 0);
#else // !Q_OS_WINRT
    qWarning("QThread::terminate: Terminate is not supported on WinRT");
#endif // Q_OS_WINRT
    QThreadPrivate::finish(this, false);
}

bool QThread::wait(unsigned long time)
{
    Q_D(QThread);
    QMutexLocker locker(&d->mutex);

#ifndef Q_OS_WINRT
    if (d->id == GetCurrentThreadId()) {
#else
    if (d->id == std::this_thread::get_id()) {
#endif
        qWarning("QThread::wait: Thread tried to wait on itself");
        return false;
    }
    if (d->finished || !d->running)
        return true;

    ++d->waiters;
    locker.mutex()->unlock();

    bool ret = false;
#ifndef Q_OS_WINRT
    switch (WaitForSingleObject(d->handle, time)) {
    case WAIT_OBJECT_0:
        ret = true;
        break;
    case WAIT_FAILED:
        qErrnoWarning("QThread::wait: Thread wait failure");
        break;
    case WAIT_ABANDONED:
    case WAIT_TIMEOUT:
    default:
        break;
    }
#else // !Q_OS_WINRT
    if (!d->finished) {
        QElapsedTimer timer;
        timer.start();
        while (timer.elapsed() < time && !d->finished)
            yieldCurrentThread();
    }
#endif // Q_OS_WINRT

    locker.mutex()->lock();
    --d->waiters;

    if (ret && !d->finished) {
        // thread was terminated by someone else

        QThreadPrivate::finish(this, false);
    }

    if (d->finished && !d->waiters) {
#ifndef Q_OS_WINRT
        CloseHandle(d->handle);
#else
        d->handle->detach();
        delete d->handle;
#endif
        d->handle = 0;
    }

    return ret;
}

void QThread::setTerminationEnabled(bool enabled)
{
    QThread *thr = currentThread();
    Q_ASSERT_X(thr != 0, "QThread::setTerminationEnabled()",
               "Current thread was not started with QThread.");
    QThreadPrivate *d = thr->d_func();
    QMutexLocker locker(&d->mutex);
    d->terminationEnabled = enabled;
    if (enabled && d->terminatePending) {
        QThreadPrivate::finish(thr, false);
        locker.unlock(); // don't leave the mutex locked!
#ifndef Q_OS_WINRT
        _endthreadex(0);
#endif
    }
}

// Caller must hold the mutex
void QThreadPrivate::setPriority(QThread::Priority threadPriority)
{
#ifndef Q_OS_WINRT
    // copied from start() with a few modifications:

    int prio;
    priority = threadPriority;
    switch (priority) {
    case QThread::IdlePriority:
        prio = THREAD_PRIORITY_IDLE;
        break;

    case QThread::LowestPriority:
        prio = THREAD_PRIORITY_LOWEST;
        break;

    case QThread::LowPriority:
        prio = THREAD_PRIORITY_BELOW_NORMAL;
        break;

    case QThread::NormalPriority:
        prio = THREAD_PRIORITY_NORMAL;
        break;

    case QThread::HighPriority:
        prio = THREAD_PRIORITY_ABOVE_NORMAL;
        break;

    case QThread::HighestPriority:
        prio = THREAD_PRIORITY_HIGHEST;
        break;

    case QThread::TimeCriticalPriority:
        prio = THREAD_PRIORITY_TIME_CRITICAL;
        break;

    case QThread::InheritPriority:
    default:
        qWarning("QThread::setPriority: Argument cannot be InheritPriority");
        return;
    }

    if (!SetThreadPriority(handle, prio)) {
        qErrnoWarning("QThread::setPriority: Failed to set thread priority");
    }
#else // !Q_OS_WINRT
    if (priority != threadPriority) {
        qWarning("QThread::setPriority: Failed to set thread priority (not implemented)");
        return;
    }
#endif // Q_OS_WINRT
}

QT_END_NAMESPACE
#endif // QT_NO_THREAD
