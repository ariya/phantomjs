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

#include "qeventdispatcher_winrt_p.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QThread>
#include <QtCore/QHash>
#include <QtCore/qfunctions_winrt.h>
#include <private/qabstracteventdispatcher_p.h>
#include <private/qcoreapplication_p.h>

#include <wrl.h>
#include <windows.foundation.h>
#include <windows.system.threading.h>
#include <windows.ui.core.h>
#include <windows.applicationmodel.core.h>
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::System::Threading;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::UI::Core;
using namespace ABI::Windows::ApplicationModel::Core;

QT_BEGIN_NAMESPACE

class QZeroTimerEvent : public QTimerEvent
{
public:
    explicit inline QZeroTimerEvent(int timerId)
        : QTimerEvent(timerId)
    { t = QEvent::ZeroTimerEvent; }
};

struct WinRTTimerInfo                           // internal timer info
{
    WinRTTimerInfo(int timerId, int timerInterval, Qt::TimerType timerType, QObject *object, QEventDispatcherWinRT *dispatcher)
        : isFinished(false), id(timerId), interval(timerInterval), timerType(timerType), obj(object), inTimerEvent(false), dispatcher(dispatcher)
    {
    }

    void cancel()
    {
        if (isFinished) {
            delete this;
            return;
        }
        isFinished = true;
        if (!timer)
            return;

        HRESULT hr = timer->Cancel();
        RETURN_VOID_IF_FAILED("Failed to cancel timer");
    }

    HRESULT timerExpired(IThreadPoolTimer *)
    {
        if (isFinished)
            return S_OK;
        if (dispatcher)
            QCoreApplication::postEvent(dispatcher, new QTimerEvent(id));
        return S_OK;
    }

    HRESULT timerDestroyed(IThreadPoolTimer *)
    {
        if (isFinished)
            delete this;
        else
            isFinished = true;
        return S_OK;
    }

    bool isFinished;
    int id;
    int interval;
    Qt::TimerType timerType;
    quint64 timeout;                            // - when to actually fire
    QObject *obj;                               // - object to receive events
    bool inTimerEvent;
    ComPtr<IThreadPoolTimer> timer;
    QPointer<QEventDispatcherWinRT> dispatcher;
};

class QEventDispatcherWinRTPrivate : public QAbstractEventDispatcherPrivate
{
    Q_DECLARE_PUBLIC(QEventDispatcherWinRT)

public:
    QEventDispatcherWinRTPrivate();
    ~QEventDispatcherWinRTPrivate();

private:
    QHash<int, WinRTTimerInfo*> timerDict;

    ComPtr<IThreadPoolTimerStatics> timerFactory;
    ComPtr<ICoreDispatcher> coreDispatcher;
    QPointer<QThread> thread;

    bool interrupt;

    void fetchCoreDispatcher()
    {
        ComPtr<ICoreImmersiveApplication> application;
        HRESULT hr = RoGetActivationFactory(HString::MakeReference(RuntimeClass_Windows_ApplicationModel_Core_CoreApplication).Get(),
                                            IID_PPV_ARGS(&application));
        RETURN_VOID_IF_FAILED("Failed to get the application factory");

        ComPtr<ICoreApplicationView> view;
        hr = application->get_MainView(&view);
        RETURN_VOID_IF_FAILED("Failed to get the main view");

        ComPtr<ICoreApplicationView2> view2;
        hr = view.As(&view2);
        RETURN_VOID_IF_FAILED("Failed to cast the main view");

        hr = view2->get_Dispatcher(&coreDispatcher);
        if (hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND)) // expected in thread pool cases
            return;
        RETURN_VOID_IF_FAILED("Failed to get core dispatcher");

        thread = QThread::currentThread();
    }
};

QEventDispatcherWinRT::QEventDispatcherWinRT(QObject *parent)
    : QAbstractEventDispatcher(*new QEventDispatcherWinRTPrivate, parent)
{
    Q_D(QEventDispatcherWinRT);

    // Special treatment for the WinMain thread, as it is created before the UI
    static bool firstThread = true;
    if (firstThread) {
        firstThread = false;
        return;
    }

    d->fetchCoreDispatcher();
}

QEventDispatcherWinRT::QEventDispatcherWinRT(QEventDispatcherWinRTPrivate &dd, QObject *parent)
    : QAbstractEventDispatcher(dd, parent)
{ }

QEventDispatcherWinRT::~QEventDispatcherWinRT()
{
}

bool QEventDispatcherWinRT::processEvents(QEventLoop::ProcessEventsFlags flags)
{
    Q_D(QEventDispatcherWinRT);

    if (d->thread && d->thread != QThread::currentThread())
        d->fetchCoreDispatcher();

    bool didProcess = false;
    forever {
        // Process native events
        if (d->coreDispatcher) {
            boolean hasThreadAccess;
            HRESULT hr = d->coreDispatcher->get_HasThreadAccess(&hasThreadAccess);
            if (SUCCEEDED(hr) && hasThreadAccess) {
                hr = d->coreDispatcher->ProcessEvents(CoreProcessEventsOption_ProcessAllIfPresent);
                if (FAILED(hr))
                    qErrnoWarning(hr, "Failed to process events");
            }
        }

        // Dispatch accumulated user events
        didProcess = sendPostedEvents(flags);
        if (didProcess)
            break;

        if (d->interrupt)
            break;

        // Short sleep if there is nothing to do
        if (!(flags & QEventLoop::WaitForMoreEvents))
            break;

        emit aboutToBlock();
        WaitForSingleObjectEx(GetCurrentThread(), 1, FALSE);
        emit awake();
    }
    d->interrupt = false;
    return didProcess;
}

bool QEventDispatcherWinRT::sendPostedEvents(QEventLoop::ProcessEventsFlags flags)
{
    Q_UNUSED(flags);
    if (hasPendingEvents()) {
        QCoreApplication::sendPostedEvents();
        return true;
    }
    return false;
}

bool QEventDispatcherWinRT::hasPendingEvents()
{
    return qGlobalPostedEventsCount();
}

void QEventDispatcherWinRT::registerSocketNotifier(QSocketNotifier *notifier)
{
    Q_UNUSED(notifier);
    Q_UNIMPLEMENTED();
}
void QEventDispatcherWinRT::unregisterSocketNotifier(QSocketNotifier *notifier)
{
    Q_UNUSED(notifier);
    Q_UNIMPLEMENTED();
}

void QEventDispatcherWinRT::registerTimer(int timerId, int interval, Qt::TimerType timerType, QObject *object)
{
    Q_UNUSED(timerType);

#ifndef QT_NO_DEBUG
    if (timerId < 1 || interval < 0 || !object) {
        qWarning("QEventDispatcherWinRT::registerTimer: invalid arguments");
        return;
    } else if (object->thread() != thread() || thread() != QThread::currentThread()) {
        qWarning("QEventDispatcherWinRT::registerTimer: timers cannot be started from another thread");
        return;
    }
#endif

    Q_D(QEventDispatcherWinRT);

    WinRTTimerInfo *t = new WinRTTimerInfo(timerId, interval, timerType, object, this);
    t->timeout = qt_msectime() + interval;
    d->timerDict.insert(t->id, t);

    // Don't use timer factory for zero-delay timers
    if (interval == 0u) {
        QCoreApplication::postEvent(this, new QZeroTimerEvent(timerId));
        return;
    }

    TimeSpan period;
    period.Duration = interval ? (interval * 10000) : 1; // TimeSpan is based on 100-nanosecond units
    HRESULT hr = d->timerFactory->CreatePeriodicTimerWithCompletion(
                Callback<ITimerElapsedHandler>(t, &WinRTTimerInfo::timerExpired).Get(), period,
                Callback<ITimerDestroyedHandler>(t, &WinRTTimerInfo::timerDestroyed).Get(), &t->timer);
    if (FAILED(hr)) {
        qErrnoWarning(hr, "Failed to create periodic timer");
        delete t;
        d->timerDict.remove(t->id);
        return;
    }
}

bool QEventDispatcherWinRT::unregisterTimer(int timerId)
{
#ifndef QT_NO_DEBUG
    if (timerId < 1) {
        qWarning("QEventDispatcherWinRT::unregisterTimer: invalid argument");
        return false;
    }
    if (thread() != QThread::currentThread()) {
        qWarning("QEventDispatcherWinRT::unregisterTimer: timers cannot be stopped from another thread");
        return false;
    }
#endif

    Q_D(QEventDispatcherWinRT);

    WinRTTimerInfo *t = d->timerDict.take(timerId);
    if (!t)
        return false;

    t->cancel();
    return true;
}

bool QEventDispatcherWinRT::unregisterTimers(QObject *object)
{
#ifndef QT_NO_DEBUG
    if (!object) {
        qWarning("QEventDispatcherWinRT::unregisterTimers: invalid argument");
        return false;
    }
    QThread *currentThread = QThread::currentThread();
    if (object->thread() != thread() || thread() != currentThread) {
        qWarning("QEventDispatcherWinRT::unregisterTimers: timers cannot be stopped from another thread");
        return false;
    }
#endif

    Q_D(QEventDispatcherWinRT);
    for (QHash<int, WinRTTimerInfo *>::iterator it = d->timerDict.begin(); it != d->timerDict.end();) {
        if (it.value()->obj == object) {
            it.value()->cancel();
            it = d->timerDict.erase(it);
            continue;
        }
        ++it;
    }
    return true;
}

QList<QAbstractEventDispatcher::TimerInfo> QEventDispatcherWinRT::registeredTimers(QObject *object) const
{
    if (!object) {
        qWarning("QEventDispatcherWinRT:registeredTimers: invalid argument");
        return QList<TimerInfo>();
    }

    Q_D(const QEventDispatcherWinRT);
    QList<TimerInfo> list;
    foreach (const WinRTTimerInfo *t, d->timerDict) {
        if (t->obj == object)
            list.append(TimerInfo(t->id, t->interval, t->timerType));
    }
    return list;
}

bool QEventDispatcherWinRT::registerEventNotifier(QWinEventNotifier *notifier)
{
    Q_UNUSED(notifier);
    Q_UNIMPLEMENTED();
    return false;
}

void QEventDispatcherWinRT::unregisterEventNotifier(QWinEventNotifier *notifier)
{
    Q_UNUSED(notifier);
    Q_UNIMPLEMENTED();
}

int QEventDispatcherWinRT::remainingTime(int timerId)
{
#ifndef QT_NO_DEBUG
    if (timerId < 1) {
        qWarning("QEventDispatcherWinRT::remainingTime: invalid argument");
        return -1;
    }
#endif

    Q_D(QEventDispatcherWinRT);
    if (WinRTTimerInfo *t = d->timerDict.value(timerId)) {
        const quint64 currentTime = qt_msectime();
        if (currentTime < t->timeout) {
            // time to wait
            return t->timeout - currentTime;
        } else {
            return 0;
        }
    }

#ifndef QT_NO_DEBUG
    qWarning("QEventDispatcherWinRT::remainingTime: timer id %d not found", timerId);
#endif
    return -1;
}

void QEventDispatcherWinRT::wakeUp()
{
}

void QEventDispatcherWinRT::interrupt()
{
    Q_D(QEventDispatcherWinRT);
    d->interrupt = true;
}

void QEventDispatcherWinRT::flush()
{
}

void QEventDispatcherWinRT::startingUp()
{
}

void QEventDispatcherWinRT::closingDown()
{
    Q_D(QEventDispatcherWinRT);
    d->timerDict.clear();
}

bool QEventDispatcherWinRT::event(QEvent *e)
{
    Q_D(QEventDispatcherWinRT);
    bool ret = false;
    switch (e->type()) {
    case QEvent::ZeroTimerEvent:
        ret = true;
        // fall through
    case QEvent::Timer: {
        QTimerEvent *timerEvent = static_cast<QTimerEvent *>(e);
        const int id = timerEvent->timerId();
        if (WinRTTimerInfo *t = d->timerDict.value(id)) {
            if (t->inTimerEvent) // but don't allow event to recurse
                break;
            t->inTimerEvent = true;

            QTimerEvent te(id);
            QCoreApplication::sendEvent(t->obj, &te);

            if (t = d->timerDict.value(id)) {
                if (t->interval == 0 && t->inTimerEvent) {
                    // post the next zero timer event as long as the timer was not restarted
                    QCoreApplication::postEvent(this, new QZeroTimerEvent(id));
                }
                t->inTimerEvent = false;
            }
        }
    }
    default:
        break;
    }
    return ret ? true : QAbstractEventDispatcher::event(e);
}

QEventDispatcherWinRTPrivate::QEventDispatcherWinRTPrivate()
    : interrupt(false)
{
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    HRESULT hr = GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_System_Threading_ThreadPoolTimer).Get(), &timerFactory);
    if (FAILED(hr))
        qWarning("QEventDispatcherWinRTPrivate::QEventDispatcherWinRTPrivate: Could not obtain timer factory: %lx", hr);
}

QEventDispatcherWinRTPrivate::~QEventDispatcherWinRTPrivate()
{
    CoUninitialize();
}

QT_END_NAMESPACE
