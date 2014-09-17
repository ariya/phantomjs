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

#ifndef QEVENTDISPATCHER_SYMBIAN_P_H
#define QEVENTDISPATCHER_SYMBIAN_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qhash.h>
#include <qset.h>
#include <qshareddata.h>
#include <qabstracteventdispatcher.h>
#include <private/qabstracteventdispatcher_p.h>
#include <qthread.h>
#include <qmutex.h>
#include <qwaitcondition.h>
#include <qsocketnotifier.h>
#include <qdatetime.h>
#include <qelapsedtimer.h>

#include <e32base.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>

QT_BEGIN_NAMESPACE


class QEventDispatcherSymbian;
class QTimerActiveObject;

class Q_CORE_EXPORT QActiveObject : public CActive
{
public:
    QActiveObject(TInt priority, QEventDispatcherSymbian *dispatcher);
    ~QActiveObject();

    bool maybeDeferSocketEvent();
    bool maybeQueueForLater();
    void reactivateAndComplete();
protected:
    QEventDispatcherSymbian *m_dispatcher;
    QThreadData *m_threadData;

private:
    bool m_hasAlreadyRun : 1;
    bool m_hasRunAgain : 1;
    int m_iterationCount;
};

class QWakeUpActiveObject : public CActive
{
public:
    QWakeUpActiveObject(QEventDispatcherSymbian *dispatcher);
    ~QWakeUpActiveObject();

    void Complete();

protected:
    void DoCancel();
    void RunL();

private:
    QEventDispatcherSymbian *m_dispatcher;
    TThreadId m_hostThreadId;
};

struct SymbianTimerInfo : public QSharedData
{
    SymbianTimerInfo();
    ~SymbianTimerInfo();

    int timerId;
    int interval;
    int msLeft;
    bool inTimerEvent;
    QObject *receiver;
    QTimerActiveObject *timerAO;
    QEventDispatcherSymbian *dispatcher;
};

typedef QExplicitlySharedDataPointer<SymbianTimerInfo> SymbianTimerInfoPtr;

// This is a bit of a proxy class. See comments in SetActive and Start for details.
class QTimerActiveObject : public QActiveObject
{
public:
    QTimerActiveObject(QEventDispatcherSymbian *dispatcher, SymbianTimerInfo *timerInfo);
    ~QTimerActiveObject();

    void Start();

protected:
    void DoCancel();
    void RunL();

private:
    void Run();
    void StartTimer();

private:
    SymbianTimerInfo *m_timerInfo;
    QElapsedTimer m_timeoutTimer;
    int m_expectedTimeSinceLastEvent;
    RTimer m_rTimer;
};

class QCompleteDeferredAOs : public CActive
{
public:
    QCompleteDeferredAOs(QEventDispatcherSymbian *dispatcher);
    ~QCompleteDeferredAOs();

    void complete();

protected:
    void DoCancel();
    void RunL();

private:
    QEventDispatcherSymbian *m_dispatcher;
};

class QSocketActiveObject : public QActiveObject
{
public:
    QSocketActiveObject(QEventDispatcherSymbian *dispatcher, QSocketNotifier *notifier);
    ~QSocketActiveObject();

    void deleteLater();

protected:
    void DoCancel();
    void RunL();
    void run();

private:
    QSocketNotifier *m_notifier;
    bool m_inSocketEvent;
    bool m_deleteLater;

    friend class QEventDispatcherSymbian;
};

class QSelectThread : public QThread
{
    Q_DECLARE_PRIVATE(QThread)

public:
    QSelectThread();
    ~QSelectThread();

    void requestSocketEvents ( QSocketNotifier *notifier, TRequestStatus *status );
    void cancelSocketEvents ( QSocketNotifier *notifier );
    void restart();
    void stop();

protected:
    void run();

private:
    int updateSocketSet(QSocketNotifier::Type type, fd_set *fds);
    void updateActivatedNotifiers(QSocketNotifier::Type type, fd_set *fds);

private:
    int m_pipeEnds[2];
    QHash<QSocketNotifier *, TRequestStatus *> m_AOStatuses;
    QMutex m_mutex;
    QWaitCondition m_waitCond;
    bool m_quit;
};

class Q_CORE_EXPORT CQtActiveScheduler : public CActiveScheduler
{
public: // from CActiveScheduler
    virtual void Error(TInt aError) const;
};

class Q_CORE_EXPORT QEventDispatcherSymbian : public QAbstractEventDispatcher
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QAbstractEventDispatcher)

public:
    QEventDispatcherSymbian(QObject *parent = 0);
    ~QEventDispatcherSymbian();

    void flush();
    bool hasPendingEvents();
    void interrupt();
    bool processEvents ( QEventLoop::ProcessEventsFlags flags );
    void registerSocketNotifier ( QSocketNotifier * notifier );
    void registerTimer ( int timerId, int interval, QObject * object );
    QList<TimerInfo> registeredTimers ( QObject * object ) const;
    void unregisterSocketNotifier ( QSocketNotifier * notifier );
    bool unregisterTimer ( int timerId );
    bool unregisterTimers ( QObject * object );
    void wakeUp();

    void startingUp();
    void closingDown();

    void timerFired(int timerId, QTimerActiveObject *ao);
    void wakeUpWasCalled(QWakeUpActiveObject *ao);
    void reactivateSocketNotifier(QSocketNotifier *notifier);

    void addDeferredActiveObject(QActiveObject *object);
    void removeDeferredActiveObject(QActiveObject *object);
    void queueDeferredActiveObjectsCompletion();
    // Can be overridden to activate local active objects too, but do call baseclass!
    virtual void reactivateDeferredActiveObjects();
    inline int iterationCount() const { return m_iterationCount; }

    void addDeferredSocketActiveObject(QActiveObject *object);
    inline bool areSocketEventsBlocked() const { return m_noSocketEvents; }

    static void RequestComplete(TRequestStatus *&status, TInt reason);
    static void RequestComplete(RThread &threadHandle, TRequestStatus *&status, TInt reason);

    void activeObjectError(int error);

private:
    bool sendPostedEvents();
    bool sendDeferredSocketEvents();

    QSelectThread& selectThread();
private:
    QSelectThread *m_selectThread;

    CQtActiveScheduler *m_activeScheduler;

    QHash<int, SymbianTimerInfoPtr> m_timerList;
    QHash<QSocketNotifier *, QSocketActiveObject *> m_notifiers;

    QWakeUpActiveObject *m_wakeUpAO;
    QCompleteDeferredAOs *m_completeDeferredAOs;

    volatile bool m_interrupt;
    QAtomicInt m_wakeUpDone;

    unsigned char m_iterationCount;
    bool m_insideTimerEvent;
    bool m_noSocketEvents;
    //deferred until socket events are enabled
    QList<QActiveObject *> m_deferredSocketEvents;

    QList<QActiveObject *> m_deferredActiveObjects;

    int m_delay;
    int m_avgEventTime;
    QElapsedTimer m_lastIdleRequestTimer;
    int m_oomErrorCount;
    QElapsedTimer m_oomErrorTimer;
};

#ifdef QT_DEBUG
# define VERIFY_PENDING_REQUEST_STATUS \
    Q_ASSERT(status->Int() == KRequestPending);
#else
# define VERIFY_PENDING_REQUEST_STATUS
#endif

// Convenience functions for doing some sanity checking on our own complete code.
// Unless QT_DEBUG is defined, it is exactly equivalent to the Symbian version.
inline void QEventDispatcherSymbian::RequestComplete(TRequestStatus *&status, TInt reason)
{
    VERIFY_PENDING_REQUEST_STATUS
    User::RequestComplete(status, reason);
}
inline void QEventDispatcherSymbian::RequestComplete(RThread &threadHandle, TRequestStatus *&status, TInt reason)
{
    VERIFY_PENDING_REQUEST_STATUS
    threadHandle.RequestComplete(status, reason);
}

#undef VERIFY_PENDING_REQUEST_STATUS

QT_END_NAMESPACE

#endif // QEVENTDISPATCHER_SYMBIAN_P_H
