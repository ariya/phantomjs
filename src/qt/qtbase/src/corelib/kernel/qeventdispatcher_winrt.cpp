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

#include "qeventdispatcher_winrt_p.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QThread>
#include <QtCore/QHash>
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
    WinRTTimerInfo() : timer(0) {}

    int id;
    int interval;
    Qt::TimerType timerType;
    quint64 timeout;                            // - when to actually fire
    QObject *obj;                               // - object to receive events
    bool inTimerEvent;
    ComPtr<IThreadPoolTimer> timer;
};

class QEventDispatcherWinRTPrivate : public QAbstractEventDispatcherPrivate
{
    Q_DECLARE_PUBLIC(QEventDispatcherWinRT)

public:
    QEventDispatcherWinRTPrivate();
    ~QEventDispatcherWinRTPrivate();

    void registerTimer(WinRTTimerInfo *t);
    void unregisterTimer(WinRTTimerInfo *t);
    void sendTimerEvent(int timerId);

private:
    static HRESULT timerExpiredCallback(IThreadPoolTimer *timer);

    QHash<int, WinRTTimerInfo*> timerDict;
    QHash<IThreadPoolTimer *, int> timerIds;

    ComPtr<IThreadPoolTimerStatics> timerFactory;
    ComPtr<ICoreDispatcher> coreDispatcher;

    bool interrupt;
};

QEventDispatcherWinRT::QEventDispatcherWinRT(QObject *parent)
    : QAbstractEventDispatcher(*new QEventDispatcherWinRTPrivate, parent)
{
    Q_D(QEventDispatcherWinRT);

    // Only look up the event dispatcher in the main thread
    if (QThread::currentThread() != QCoreApplicationPrivate::theMainThread)
        return;

    ComPtr<ICoreApplication> application;
    HRESULT hr = RoGetActivationFactory(HString::MakeReference(RuntimeClass_Windows_ApplicationModel_Core_CoreApplication).Get(),
                                        IID_PPV_ARGS(&application));
    if (SUCCEEDED(hr)) {
        ComPtr<ICoreApplicationView> view;
        hr = application->GetCurrentView(&view);
        if (SUCCEEDED(hr)) {
            ComPtr<ICoreWindow> window;
            hr = view->get_CoreWindow(&window);
            if (SUCCEEDED(hr)) {
                hr = window->get_Dispatcher(&d->coreDispatcher);
                if (SUCCEEDED(hr))
                    return;
            }
        }
    }
    qCritical("QEventDispatcherWinRT: Unable to capture the core dispatcher. %s",
              qPrintable(qt_error_string(hr)));
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

    bool didProcess = false;
    forever {
        // Process native events
        if (d->coreDispatcher)
            d->coreDispatcher->ProcessEvents(CoreProcessEventsOption_ProcessAllIfPresent);

        // Dispatch accumulated user events
        didProcess = sendPostedEvents(flags);
        if (didProcess)
            break;

        if (d->interrupt)
            break;

        // Short sleep if there is nothing to do
        if (flags & QEventLoop::WaitForMoreEvents) {
            emit aboutToBlock();
            WaitForSingleObjectEx(GetCurrentThread(), 1, FALSE);
            emit awake();
        } else {
            break;
        }
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

    if (timerId < 1 || interval < 0 || !object) {
        qWarning("QEventDispatcherWinRT::registerTimer: invalid arguments");
        return;
    } else if (object->thread() != thread() || thread() != QThread::currentThread()) {
        qWarning("QObject::startTimer: timers cannot be started from another thread");
        return;
    }

    Q_D(QEventDispatcherWinRT);
    WinRTTimerInfo *t = new WinRTTimerInfo();
    t->id = timerId;
    t->interval = interval;
    t->timerType = timerType;
    t->obj = object;
    t->inTimerEvent = false;

    d->registerTimer(t);
    d->timerDict.insert(t->id, t);
}

bool QEventDispatcherWinRT::unregisterTimer(int timerId)
{
    if (timerId < 1) {
        qWarning("QEventDispatcherWinRT::unregisterTimer: invalid argument");
        return false;
    }
    if (thread() != QThread::currentThread()) {
        qWarning("QObject::killTimer: timers cannot be stopped from another thread");
        return false;
    }

    Q_D(QEventDispatcherWinRT);

    WinRTTimerInfo *t = d->timerDict.value(timerId);
    if (!t)
        return false;

    d->unregisterTimer(t);
    return true;
}

bool QEventDispatcherWinRT::unregisterTimers(QObject *object)
{
    if (!object) {
        qWarning("QEventDispatcherWinRT::unregisterTimers: invalid argument");
        return false;
    }
    QThread *currentThread = QThread::currentThread();
    if (object->thread() != thread() || thread() != currentThread) {
        qWarning("QObject::killTimers: timers cannot be stopped from another thread");
        return false;
    }

    Q_D(QEventDispatcherWinRT);
    foreach (WinRTTimerInfo *t, d->timerDict) {
        if (t->obj == object)
            d->unregisterTimer(t);
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
    foreach (WinRTTimerInfo *t, d->timerDict)
        d->unregisterTimer(t);
    d->timerDict.clear();
    d->timerIds.clear();
}

bool QEventDispatcherWinRT::event(QEvent *e)
{
    Q_D(QEventDispatcherWinRT);
    if (e->type() == QEvent::ZeroTimerEvent) {
        QZeroTimerEvent *zte = static_cast<QZeroTimerEvent*>(e);
        WinRTTimerInfo *t = d->timerDict.value(zte->timerId());
        if (t) {
            t->inTimerEvent = true;

            QTimerEvent te(zte->timerId());
            QCoreApplication::sendEvent(t->obj, &te);

            t = d->timerDict.value(zte->timerId());
            if (t) {
                if (t->interval == 0 && t->inTimerEvent) {
                    // post the next zero timer event as long as the timer was not restarted
                    QCoreApplication::postEvent(this, new QZeroTimerEvent(zte->timerId()));
                }

                t->inTimerEvent = false;
            }
        }
        return true;
    } else if (e->type() == QEvent::Timer) {
        QTimerEvent *te = static_cast<QTimerEvent*>(e);
        d->sendTimerEvent(te->timerId());
    }
    return QAbstractEventDispatcher::event(e);
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

void QEventDispatcherWinRTPrivate::registerTimer(WinRTTimerInfo *t)
{
    Q_Q(QEventDispatcherWinRT);

    bool ok = false;
    uint interval = t->interval;
    if (interval == 0u) {
        // optimization for single-shot-zero-timer
        QCoreApplication::postEvent(q, new QZeroTimerEvent(t->id));
        ok = true;
    } else {
        TimeSpan period;
        period.Duration = interval * 10000; // TimeSpan is based on 100-nanosecond units
        ok = SUCCEEDED(timerFactory->CreatePeriodicTimer(
                           Callback<ITimerElapsedHandler>(&QEventDispatcherWinRTPrivate::timerExpiredCallback).Get(), period, &t->timer));
        if (ok)
            timerIds.insert(t->timer.Get(), t->id);
    }
    t->timeout = qt_msectime() + interval;
    if (!ok)
        qErrnoWarning("QEventDispatcherWinRT::registerTimer: Failed to create a timer");
}

void QEventDispatcherWinRTPrivate::unregisterTimer(WinRTTimerInfo *t)
{
    if (t->timer) {
        timerIds.remove(t->timer.Get());
        t->timer->Cancel();
    }
    timerDict.remove(t->id);
    delete t;
}

void QEventDispatcherWinRTPrivate::sendTimerEvent(int timerId)
{
    WinRTTimerInfo *t = timerDict.value(timerId);
    if (t && !t->inTimerEvent) {
        // send event, but don't allow it to recurse
        t->inTimerEvent = true;

        QTimerEvent e(t->id);
        QCoreApplication::sendEvent(t->obj, &e);

        // timer could have been removed
        t = timerDict.value(timerId);
        if (t)
            t->inTimerEvent = false;
    }
}

HRESULT QEventDispatcherWinRTPrivate::timerExpiredCallback(IThreadPoolTimer *timer)
{
    QThread *thread = QThread::currentThread();
    if (!thread)
        return E_FAIL;

    QAbstractEventDispatcher *eventDispatcher = thread->eventDispatcher();
    if (!eventDispatcher)
        return E_FAIL;

    QEventDispatcherWinRTPrivate *d = static_cast<QEventDispatcherWinRTPrivate *>(get(eventDispatcher));
    int timerId = d->timerIds.value(timer, -1);
    if (timerId < 0)
        return E_FAIL; // A callback was received after the timer was canceled

    QCoreApplication::postEvent(eventDispatcher, new QTimerEvent(timerId));
    return S_OK;
}

QT_END_NAMESPACE
