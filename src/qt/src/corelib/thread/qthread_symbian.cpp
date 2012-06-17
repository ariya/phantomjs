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
#include "qplatformdefs.h"
#include <private/qcoreapplication_p.h>
#include <private/qeventdispatcher_symbian_p.h>
#include "qthreadstorage.h"
#include "qthread_p.h"
#include <private/qsystemerror_p.h>
#include <private/qcore_symbian_p.h>

#include <sched.h>
#include <hal.h>
#include <hal_data.h>
#include <e32math.h>

#include <QRegExp>
// This can be manually enabled if debugging thread problems
#ifdef QT_USE_RTTI_IN_THREAD_CLASSNAME
#include <typeinfo>
#endif

// You only find these enumerations on Symbian^3 onwards, so we need to provide our own
// to remain compatible with older releases. They won't be called by pre-Sym^3 SDKs.

// HALData::ENumCpus
#define QT_HALData_ENumCpus 119

QT_BEGIN_NAMESPACE

#ifndef QT_NO_THREAD

enum { ThreadPriorityResetFlag = 0x80000000 };

// Utility functions for getting, setting and clearing thread specific data.
static QThreadData *get_thread_data()
{
    return reinterpret_cast<QThreadData *>(Dll::Tls());
}

static void set_thread_data(QThreadData *data)
{
    qt_symbian_throwIfError(Dll::SetTls(data));
}

static void clear_thread_data()
{
    Dll::FreeTls();
}


static void init_symbian_thread_handle(RThread &thread)
{
    thread = RThread();
    TThreadId threadId = thread.Id();
    qt_symbian_throwIfError(thread.Open(threadId, EOwnerProcess));
}

QThreadData *QThreadData::current()
{
    QThreadData *data = get_thread_data();
    if (!data) {
        void *a;
        if (QInternal::activateCallbacks(QInternal::AdoptCurrentThread, &a)) {
            QThread *adopted = static_cast<QThread*>(a);
            Q_ASSERT(adopted);
            data = QThreadData::get2(adopted);
            set_thread_data(data);
            adopted->d_func()->running = true;
            adopted->d_func()->finished = false;
            static_cast<QAdoptedThread *>(adopted)->init();
        } else {
            data = new QThreadData;
            QT_TRY {
                set_thread_data(data);
                data->thread = new QAdoptedThread(data);
            } QT_CATCH(...) {
                clear_thread_data();
                data->deref();
                data = 0;
                QT_RETHROW;
            }
            data->deref();
        }
        data->isAdopted = true;
        data->threadId = QThread::currentThreadId();
        if (!QCoreApplicationPrivate::theMainThread)
            QCoreApplicationPrivate::theMainThread = data->thread;
    }
    return data;
}


class QCAdoptedThreadMonitor : public CActive
{
public:
    QCAdoptedThreadMonitor(QThread *thread)
    : CActive(EPriorityStandard), data(QThreadData::get2(thread))
    {
        CActiveScheduler::Add(this);
        data->symbian_thread_handle.Logon(iStatus);
        SetActive();
    }
    ~QCAdoptedThreadMonitor()
    {
        Cancel();
    }
    void DoCancel()
    {
        data->symbian_thread_handle.LogonCancel(iStatus);
    }
    void RunL();
private:
    QThreadData* data;
};

class QCAddAdoptedThread : public CActive
{
public:
    QCAddAdoptedThread()
    : CActive(EPriorityStandard)
    {
        CActiveScheduler::Add(this);
    }
    void ConstructL()
    {
        User::LeaveIfError(monitorThread.Open(RThread().Id()));
        start();
    }
    ~QCAddAdoptedThread()
    {
        Cancel();
        monitorThread.Close();
    }
    void DoCancel()
    {
        User::RequestComplete(stat, KErrCancel);
    }
    void start()
    {
        iStatus = KRequestPending;
        SetActive();
        stat = &iStatus;
    }
    void RunL()
    {
        if (iStatus.Int() != KErrNone)
            return;

        QMutexLocker adoptedThreadMonitorMutexlock(&adoptedThreadMonitorMutex);
        for (int i=threadsToAdd.size()-1; i>=0; i--) {
            // Create an active object to monitor the thread
            new (ELeave) QCAdoptedThreadMonitor(threadsToAdd[i]);
            count++;
            threadsToAdd.pop_back();
        }
        start();
    }
    static void add(QThread *thread)
    {
        QMutexLocker adoptedThreadMonitorMutexlock(&adoptedThreadMonitorMutex);
        if (!adoptedThreadAdder) {
            RThread monitorThread;
            qt_symbian_throwIfError(monitorThread.Create(KNullDesC(), &monitorThreadFunc, 1024, &User::Allocator(), 0));
            TRequestStatus started;
            monitorThread.Rendezvous(started);
            monitorThread.Resume();
            User::WaitForRequest(started);
            monitorThread.Close();
        }
        if (RThread().Id() == adoptedThreadAdder->monitorThread.Id())
            return;
        adoptedThreadAdder->threadsToAdd.push_back(thread);
        if (adoptedThreadAdder->stat) {
            adoptedThreadAdder->monitorThread.RequestComplete(adoptedThreadAdder->stat, KErrNone);
        }
    }
    static void monitorThreadFuncL()
    {
        CActiveScheduler* scheduler = new (ELeave) CActiveScheduler();
        CleanupStack::PushL(scheduler);
        CActiveScheduler::Install(scheduler);

        adoptedThreadAdder = new(ELeave) QCAddAdoptedThread();
        CleanupStack::PushL(adoptedThreadAdder);
        adoptedThreadAdder->ConstructL();
        QCAddAdoptedThread *adder = adoptedThreadAdder;

        RThread::Rendezvous(KErrNone);
        CActiveScheduler::Start();

        CleanupStack::PopAndDestroy(adder);
        CleanupStack::PopAndDestroy(scheduler);
    }
    static int monitorThreadFunc(void *)
    {
        _LIT(KMonitorThreadName, "adoptedMonitorThread");
        RThread::RenameMe(KMonitorThreadName());
        CTrapCleanup* cleanup = CTrapCleanup::New();
        TRAPD(ret, monitorThreadFuncL());
        delete cleanup;
        return ret;
    }
    static void threadDied()
    {
        QMutexLocker adoptedThreadMonitorMutexlock(&adoptedThreadMonitorMutex);
        if (adoptedThreadAdder) {
            adoptedThreadAdder->count--;
            if (adoptedThreadAdder->count <= 0 && adoptedThreadAdder->threadsToAdd.size() == 0) {
                CActiveScheduler::Stop();
                adoptedThreadAdder = 0;
            }
        }
    }

private:
    QVector<QThread*> threadsToAdd;
    RThread monitorThread;
    static QMutex adoptedThreadMonitorMutex;
    static QCAddAdoptedThread *adoptedThreadAdder;
    int count;
    TRequestStatus *stat;
};

QMutex QCAddAdoptedThread::adoptedThreadMonitorMutex;
QCAddAdoptedThread* QCAddAdoptedThread::adoptedThreadAdder = 0;

void QCAdoptedThreadMonitor::RunL()
{
    if (data->isAdopted) {
        QThread *thread = data->thread;
        Q_ASSERT(thread);
        QThreadPrivate *thread_p = static_cast<QThreadPrivate *>(QObjectPrivate::get(thread));
        Q_ASSERT(!thread_p->finished);
        thread_p->finish(thread);
    }
    data->deref();
    QCAddAdoptedThread::threadDied();
    delete this;
}

void QAdoptedThread::init()
{
    Q_D(QThread);
    d->thread_id = RThread().Id();  // type operator to TUint
    init_symbian_thread_handle(d->data->symbian_thread_handle);
    QCAddAdoptedThread::add(this);
}

/*
   QThreadPrivate
*/

#if defined(Q_C_CALLBACKS)
extern "C" {
#endif

typedef void*(*QtThreadCallback)(void*);

#if defined(Q_C_CALLBACKS)
}
#endif

#endif // QT_NO_THREAD

void QThreadPrivate::createEventDispatcher(QThreadData *data)
{
    data->eventDispatcher = new QEventDispatcherSymbian;
    data->eventDispatcher->startingUp();
}

#ifndef QT_NO_THREAD

void *QThreadPrivate::start(void *arg)
{
    QThread *thr = reinterpret_cast<QThread *>(arg);
    QThreadData *data = QThreadData::get2(thr);

    // do we need to reset the thread priority?
    if (int(thr->d_func()->priority) & ThreadPriorityResetFlag) {
        thr->setPriority(QThread::Priority(thr->d_func()->priority & ~ThreadPriorityResetFlag));
    }

    // On symbian, threads other than the main thread are non critical by default
    // This means a worker thread can crash without crashing the application - to
    // use this feature, we would need to use RThread::Logon in the main thread
    // to catch abnormal thread exit and emit the finished signal.
    // For the sake of cross platform consistency, we set the thread as process critical
    // - advanced users who want the symbian behaviour can change the critical
    // attribute of the thread again once the app gains control in run()
    User::SetCritical(User::EProcessCritical);

    data->threadId = QThread::currentThreadId();
    set_thread_data(data);

    CTrapCleanup *cleanup = CTrapCleanup::New();
    q_check_ptr(cleanup);

    {
        QMutexLocker locker(&thr->d_func()->mutex);
        data->quitNow = thr->d_func()->exited;
    }

    // ### TODO: allow the user to create a custom event dispatcher
    createEventDispatcher(data);

    TRAPD(err, {
        try {
            emit thr->started();
            thr->run();
        } catch (const std::exception& ex) {
            qWarning("QThreadPrivate::start: Thread exited on exception %s", ex.what());
            User::Leave(KErrGeneral);   // leave to force cleanup stack cleanup
        }
    });
    if (err)
        qWarning("QThreadPrivate::start: Thread exited on leave %d", err);

    // finish emits signals which should be wrapped in a trap for Symbian code, but otherwise ignore leaves and exceptions.
    TRAP(err, {
        try {
            QThreadPrivate::finish(arg);
        } catch (const std::exception& ex) {
            User::Leave(KErrGeneral);   // leave to force cleanup stack cleanup
        }
    });

    delete cleanup;

    return 0;
}

void QThreadPrivate::finish(void *arg, bool lockAnyway, bool closeNativeHandle)
{
    QThread *thr = reinterpret_cast<QThread *>(arg);
    QThreadPrivate *d = thr->d_func();

    QMutexLocker locker(lockAnyway ? &d->mutex : 0);

    d->isInFinish = true;
    d->priority = QThread::InheritPriority;
    bool terminated = d->terminated;
    void *data = &d->data->tls;
    locker.unlock();
    if (terminated)
        emit thr->terminated();
    emit thr->finished();
    QCoreApplication::sendPostedEvents(0, QEvent::DeferredDelete);
    QThreadStorageData::finish((void **)data);
    locker.relock();
    d->terminated = false;

    QAbstractEventDispatcher *eventDispatcher = d->data->eventDispatcher;
    if (eventDispatcher) {
        d->data->eventDispatcher = 0;
        locker.unlock();
        eventDispatcher->closingDown();
        delete eventDispatcher;
        locker.relock();
    }

    d->thread_id = 0;
    if (closeNativeHandle)
        d->data->symbian_thread_handle.Close();
    d->running = false;
    d->finished = true;

    d->isInFinish = false;
    d->thread_done.wakeAll();
}




/**************************************************************************
 ** QThread
 *************************************************************************/

Qt::HANDLE QThread::currentThreadId()
{
    return (Qt::HANDLE) (TUint) RThread().Id();
}

int QThread::idealThreadCount()
{
    int cores = 1;

    if (QSysInfo::symbianVersion() >= QSysInfo::SV_SF_3) {
        TInt inumcpus;
        TInt err;
        err = HAL::Get((HALData::TAttribute)QT_HALData_ENumCpus, inumcpus);
        if (err == KErrNone) {
            cores = qMax(inumcpus, 1);
        }
    }

    return cores;
}

void QThread::yieldCurrentThread()
{
    sched_yield();
}

/*  \internal
    helper function to do thread sleeps
*/
static void thread_sleep(unsigned long remaining, unsigned long scale)
{
    // maximum Symbian wait is 2^31 microseconds
    unsigned long maxWait = KMaxTInt / scale;
    do {
        unsigned long waitTime = qMin(maxWait, remaining);
        remaining -= waitTime;
        User::AfterHighRes(waitTime * scale);
    } while (remaining);
}

void QThread::sleep(unsigned long secs)
{
    thread_sleep(secs, 1000000ul);
}

void QThread::msleep(unsigned long msecs)
{
    thread_sleep(msecs, 1000ul);
}

void QThread::usleep(unsigned long usecs)
{
    thread_sleep(usecs, 1ul);
}

TThreadPriority calculateSymbianPriority(QThread::Priority priority)
    {
    // Both Qt & Symbian use limited enums; this matches the mapping previously done through conversion to Posix granularity
    TThreadPriority symPriority;
    switch (priority)
    {
        case QThread::IdlePriority:
            symPriority = EPriorityMuchLess;
            break;
        case QThread::LowestPriority:
        case QThread::LowPriority:
            symPriority = EPriorityLess;
            break;
        case QThread::NormalPriority:
            symPriority = EPriorityNormal;
            break;
        case QThread::HighPriority:
            symPriority = EPriorityMore;
            break;
        case QThread::HighestPriority:
        case QThread::TimeCriticalPriority:
            symPriority = EPriorityMuchMore;
            break;
        case QThread::InheritPriority:
        default:
            symPriority = RThread().Priority();
            break;
    }
    return symPriority;
    }

void QThread::start(Priority priority)
{
    Q_D(QThread);
    QMutexLocker locker(&d->mutex);

    if (d->isInFinish)
        d->thread_done.wait(locker.mutex());

    if (d->running)
        return;

    d->running = true;
    d->finished = false;
    d->terminated = false;
    d->returnCode = 0;
    d->exited = false;

    d->priority = priority;

    if (d->stackSize == 0)
        // The default stack size on Symbian is very small, making even basic
        // operations like file I/O fail, so we increase it by default.
        d->stackSize = 0x14000; // Maximum stack size on Symbian.

    int code = KErrAlreadyExists;
    QString className(QLatin1String(metaObject()->className()));
#ifdef QT_USE_RTTI_IN_THREAD_CLASSNAME
    // use RTTI, if enabled, to get a more accurate className. This must be manually enabled.
    const char* rttiName = typeid(*this).name();
    if (rttiName)
        className = QLatin1String(rttiName);
#endif
    QString threadNameBase = QString(QLatin1String("%1_%2_v=0x%3_")).arg(objectName()).arg(className).arg(*(uint*)this,8,16,QLatin1Char('0'));
    // Thread name can contain only characters allowed by User::ValidateName() otherwise RThread::Create fails.
    // Not allowed characters are:
    // - any character outside range 0x20 - 0x7e
    // - or asterisk, question mark or colon
    const QRegExp notAllowedChars(QLatin1String("[^\\x20-\\x7e]|\\*|\\?|\\:"));
    threadNameBase.replace(notAllowedChars, QLatin1String("_"));

    TPtrC threadNameBasePtr(qt_QString2TPtrC(threadNameBase));
    // max thread name length is KMaxKernelName
    TBuf<KMaxKernelName> name;
    threadNameBasePtr.Set(threadNameBasePtr.Left(qMin(threadNameBasePtr.Length(), name.MaxLength() - 8)));
    const int MaxRetries = 10;
    for (int i=0; i<MaxRetries && code == KErrAlreadyExists; i++) {
        // generate a thread name with a random component to avoid and resolve name collisions
        // a named thread can be opened from another process
        name.Zero();
        name.Append(threadNameBasePtr);
        name.AppendNumFixedWidth(Math::Random(), EHex, 8);
        code = d->data->symbian_thread_handle.Create(name, (TThreadFunction) QThreadPrivate::start, d->stackSize, NULL, this);
    }
    if (code == KErrNone) {
        d->thread_id = d->data->symbian_thread_handle.Id();
        TThreadPriority symPriority = calculateSymbianPriority(priority);
        d->data->symbian_thread_handle.SetPriority(symPriority);
        d->data->symbian_thread_handle.Resume();
    } else {
        qWarning("QThread::start: Thread creation error: %s", qPrintable(QSystemError(code, QSystemError::NativeError).toString()));

        d->running = false;
        d->finished = false;
        d->thread_id = 0;
        d->data->symbian_thread_handle.Close();
    }
}

void QThread::terminate()
{
    Q_D(QThread);
    QMutexLocker locker(&d->mutex);

    if (!d->thread_id)
        return;

    if (!d->running)
        return;
    if (!d->terminationEnabled) {
        d->terminatePending = true;
        return;
    }

    d->terminated = true;
    // "false, false" meaning:
    // 1. lockAnyway = false. Don't lock the mutex because it's already locked
    //    (see above).
    // 2. closeNativeSymbianHandle = false. We don't want to close the thread handle,
    //    because we need it here to terminate the thread.
    QThreadPrivate::finish(this, false, false);
    d->data->symbian_thread_handle.Terminate(KErrNone);
    d->data->symbian_thread_handle.Close();
}

bool QThread::wait(unsigned long time)
{
    Q_D(QThread);
    QMutexLocker locker(&d->mutex);

    if (d->thread_id == (TUint) RThread().Id()) {
        qWarning("QThread::wait: Thread tried to wait on itself");
        return false;
    }

    if (d->finished || !d->running)
        return true;

    while (d->running) {
        // Check if thread still exists. Needed because kernel will kill it without notification
        // before global statics are deleted at application exit.
        if (d->data->symbian_thread_handle.Handle()
            && d->data->symbian_thread_handle.ExitType() != EExitPending) {
            // Cannot call finish here as wait is typically called from another thread.
            // It won't be necessary anyway, as we should never get here under normal operations;
            // all QThreads are EProcessCritical and therefore cannot normally exit
            // undetected (i.e. panic) as long as all thread control is via QThread.
            return true;
        }
        if (!d->thread_done.wait(locker.mutex(), time))
            return false;
    }
    return true;
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
        d->terminated = true;
        // "false" meaning:
        // -  lockAnyway = false. Don't lock the mutex because it's already locked
        //    (see above).
        QThreadPrivate::finish(thr, false);
        locker.unlock(); // don't leave the mutex locked!
        User::Exit(0);  // may be some other cleanup required? what if AS or cleanup stack?
    }
}

void QThread::setPriority(Priority priority)
{
    Q_D(QThread);
    QMutexLocker locker(&d->mutex);
    if (!d->running) {
        qWarning("QThread::setPriority: Cannot set priority, thread is not running");
        return;
    }

    d->priority = priority;

    // copied from start() with a few modifications:
    TThreadPriority symPriority = calculateSymbianPriority(priority);
    d->data->symbian_thread_handle.SetPriority(symPriority);
}

#endif // QT_NO_THREAD

QT_END_NAMESPACE

