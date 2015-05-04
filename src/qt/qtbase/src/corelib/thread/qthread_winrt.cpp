/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qthread.h"
#include "qthread_p.h"
#include "qthreadstorage.h"

#include <QtCore/QElapsedTimer>
#include <QtCore/QUuid>
#include <QtCore/qt_windows.h>
#include <QtCore/qfunctions_winrt.h>
#include <QtCore/private/qcoreapplication_p.h>
#include <QtCore/private/qeventdispatcher_winrt_p.h>

#include <wrl.h>
#include <windows.system.threading.h>
#include <windows.system.threading.core.h>
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::System::Threading;
using namespace ABI::Windows::System::Threading::Core;

#ifndef QT_NO_THREAD
QT_BEGIN_NAMESPACE

static WorkItemPriority nativePriority(QThread::Priority priority)
{
    switch (priority) {
    default:
    case QThread::NormalPriority:
        return WorkItemPriority_Normal;
    case QThread::IdlePriority:
    case QThread::LowestPriority:
    case QThread::LowPriority:
        return WorkItemPriority_Low;
    case QThread::HighPriority:
    case QThread::HighestPriority:
    case QThread::TimeCriticalPriority:
        return WorkItemPriority_High;
    }
}

class QWinRTThreadGlobal
{
public:
    QWinRTThreadGlobal()
    {
        HRESULT hr;

        hr = RoGetActivationFactory(
                    HString::MakeReference(RuntimeClass_Windows_System_Threading_Core_PreallocatedWorkItem).Get(),
                    IID_PPV_ARGS(&workItemFactory));
        Q_ASSERT_SUCCEEDED(hr);

        hr = RoGetActivationFactory(
                    HString::MakeReference(RuntimeClass_Windows_System_Threading_Core_SignalNotifier).Get(),
                    IID_PPV_ARGS(&notifierFactory));
        Q_ASSERT_SUCCEEDED(hr);

        QString eventName = QUuid::createUuid().toString();
        dispatchEvent = CreateEventEx(NULL, reinterpret_cast<LPCWSTR>(eventName.utf16()), 0, EVENT_ALL_ACCESS);

        hr = notifierFactory->AttachToEvent(
                 HStringReference(reinterpret_cast<LPCWSTR>(eventName.utf16())).Get(),
                 Callback<ISignalHandler>(this, &QWinRTThreadGlobal::dispatch).Get(), &notifier);
        Q_ASSERT_SUCCEEDED(hr);
        hr = notifier->Enable();
        Q_ASSERT_SUCCEEDED(hr);
    }

    ~QWinRTThreadGlobal()
    {
        CloseHandle(dispatchEvent);
    }

    void dispatch()
    {
        SetEvent(dispatchEvent);
    }

    void push(QThreadPrivate *d)
    {
        threads.append(d);
    }

private:
    HRESULT dispatch(ISignalNotifier *notifier, boolean timedOut)
    {
        Q_UNUSED(timedOut);
        notifier->Enable();
        if (threads.isEmpty())
            return S_OK;

        QThreadPrivate *thread = threads.takeFirst();
        ComPtr<IPreallocatedWorkItem> workItem;
        HRESULT hr = workItemFactory->CreateWorkItemWithPriority(
                    Callback<IWorkItemHandler>(thread, &QThreadPrivate::start).Get(),
                    nativePriority(thread->priority), &workItem);
        if (FAILED(hr)) {
            qErrnoWarning(hr, "Failed to create thread work item");
            thread->finish();
            return hr;
        }

        hr = workItem->RunAsync(&thread->handle);
        if (FAILED(hr)) {
            qErrnoWarning(hr, "Failed to run work item");
            thread->finish();
            return hr;
        }

        return S_OK;
    }

    HANDLE dispatchEvent;
    ComPtr<ISignalNotifier> notifier;
    ComPtr<ISignalNotifierStatics> notifierFactory;
    ComPtr<IPreallocatedWorkItemFactory> workItemFactory;

    QList<QThreadPrivate *> threads;
};
Q_GLOBAL_STATIC(QWinRTThreadGlobal, g)

/**************************************************************************
 ** QThreadData
 *************************************************************************/

__declspec(thread) static QThreadData *qt_current_thread_data = 0;

void QThreadData::clearCurrentThreadData()
{
    qt_current_thread_data = 0;
}

QThreadData *QThreadData::current(bool createIfNecessary)
{
    static bool winmainThread = true;
    QThreadData *threadData = qt_current_thread_data;
    if (!threadData && createIfNecessary) {
        threadData = new QThreadData;
        // This needs to be called prior to new AdoptedThread() to
        // avoid recursion.
        qt_current_thread_data = threadData;
        QT_TRY {
            threadData->thread = new QAdoptedThread(threadData);
        } QT_CATCH(...) {
            qt_current_thread_data = 0;
            threadData->deref();
            threadData = 0;
            QT_RETHROW;
        }
        threadData->deref();
        threadData->isAdopted = true;
        threadData->threadId = reinterpret_cast<Qt::HANDLE>(GetCurrentThreadId());

        if (!QCoreApplicationPrivate::theMainThread && !winmainThread)
            QCoreApplicationPrivate::theMainThread = threadData->thread;

        if (winmainThread) {
            g->dispatch();
            threadData->thread->d_func()->createEventDispatcher(threadData);
            winmainThread = false;
        }
    }

    return threadData;
}

void QAdoptedThread::init()
{
    Q_D(QThread);

    d->handle = Q_NULLPTR;
    d->id = 0;
}

/**************************************************************************
 ** QThreadPrivate
 *************************************************************************/

#endif // QT_NO_THREAD

void QThreadPrivate::createEventDispatcher(QThreadData *data)
{
    QEventDispatcherWinRT *eventDispatcher = new QEventDispatcherWinRT;
    data->eventDispatcher.storeRelease(eventDispatcher);
    eventDispatcher->startingUp();
}

#ifndef QT_NO_THREAD

HRESULT QThreadPrivate::start(IAsyncAction *)
{
    Q_Q(QThread);

    qt_current_thread_data = data;
    id = GetCurrentThreadId();
    data->threadId = reinterpret_cast<Qt::HANDLE>(id);
    QThread::setTerminationEnabled(false);

    {
        QMutexLocker locker(&mutex);
        data->quitNow = exited;
    }

    if (data->eventDispatcher.load())
        data->eventDispatcher.load()->startingUp();
    else
        createEventDispatcher(data);

    running = true;
    emit q->started(QThread::QPrivateSignal());

    QThread::setTerminationEnabled(true);

    q->run();

    finish();

    return S_OK;
}

void QThreadPrivate::finish(bool lockAnyway)
{
    Q_Q(QThread);

    QMutexLocker locker(lockAnyway ? &mutex : 0);
    isInFinish = true;
    priority = QThread::InheritPriority;
    void **tls_data = reinterpret_cast<void **>(&data->tls);
    locker.unlock();
    emit q->finished(QThread::QPrivateSignal());
    QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
    QThreadStorageData::finish(tls_data);
    locker.relock();

    QAbstractEventDispatcher *eventDispatcher = data->eventDispatcher.load();
    if (eventDispatcher) {
        data->eventDispatcher = 0;
        locker.unlock();
        eventDispatcher->closingDown();
        delete eventDispatcher;
        locker.relock();
    }

    running = false;
    finished = true;
    isInFinish = false;
    interruptionRequested = false;

    if (!waiters) {
        if (handle)
            handle->Release();
        handle = Q_NULLPTR;
    }

    id = 0;
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
    GetNativeSystemInfo(&sysinfo);
    return sysinfo.dwNumberOfProcessors;
}

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

    d->finished = false;
    d->exited = false;
    d->returnCode = 0;
    d->interruptionRequested = false;
    d->priority = priority == QThread::InheritPriority ? currentThread()->priority() : priority;
    g->push(d);
    g->dispatch();

    locker.unlock();
    while (!d->running && !d->finished) {
        QAbstractEventDispatcher *eventDispatcher = QThread::currentThread()->eventDispatcher();
        if (eventDispatcher)
            eventDispatcher->processEvents(QEventLoop::AllEvents);
        else
            yieldCurrentThread();
    }
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

    if (d->handle) {
        ComPtr<IAsyncInfo> info;
        HRESULT hr = d->handle->QueryInterface(IID_PPV_ARGS(&info));
        Q_ASSERT_SUCCEEDED(hr);
        hr = info->Cancel();
        if (FAILED(hr))
            qErrnoWarning(hr, "Failed to cancel thread action");
    }

    d->finish(false);
}

bool QThread::wait(unsigned long time)
{
    Q_D(QThread);
    QMutexLocker locker(&d->mutex);

    if (d->id == GetCurrentThreadId()) {
        qWarning("QThread::wait: Thread tried to wait on itself");
        return false;
    }
    if (d->finished || !d->running)
        return true;

    ++d->waiters;
    locker.mutex()->unlock();

    // Alternatively, we could check the handle
    bool ret = false;
    if (!d->finished) {
        QElapsedTimer timer;
        timer.start();
        while (timer.elapsed() < time && !d->finished)
            yieldCurrentThread();

        ret = d->finished;
    }

    locker.mutex()->lock();
    --d->waiters;

    if (ret && !d->finished) {
        // thread was terminated by someone else

        d->finish(false);
    }

    if (d->finished && !d->waiters) {
        if (d->handle)
            d->handle->Release();
        d->handle = Q_NULLPTR;
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
        d->finish(false);
        locker.unlock(); // don't leave the mutex locked!
    }
}

// Caller must hold the mutex
void QThreadPrivate::setPriority(QThread::Priority threadPriority)
{
    if (running)
        qWarning("WinRT threads can't change priority while running.");

    priority = threadPriority;
}

QT_END_NAMESPACE
#endif // QT_NO_THREAD
