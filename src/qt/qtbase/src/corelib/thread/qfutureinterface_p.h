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

#ifndef QFUTUREINTERFACE_P_H
#define QFUTUREINTERFACE_P_H

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

#include <QtCore/qelapsedtimer.h>
#include <QtCore/qcoreevent.h>
#include <QtCore/qlist.h>
#include <QtCore/qwaitcondition.h>
#include <QtCore/qrunnable.h>
#include <QtCore/qthreadpool.h>

QT_BEGIN_NAMESPACE

class QFutureCallOutEvent : public QEvent
{
public:
    enum CallOutType {
        Started,
        Finished,
        Canceled,
        Paused,
        Resumed,
        Progress,
        ProgressRange,
        ResultsReady
    };

    QFutureCallOutEvent()
        : QEvent(QEvent::FutureCallOut), callOutType(CallOutType(0)), index1(-1), index2(-1)
    { }
    explicit QFutureCallOutEvent(CallOutType callOutType, int index1 = -1)
        : QEvent(QEvent::FutureCallOut), callOutType(callOutType), index1(index1), index2(-1)
    { }
    QFutureCallOutEvent(CallOutType callOutType, int index1, int index2)
        : QEvent(QEvent::FutureCallOut), callOutType(callOutType), index1(index1), index2(index2)
    { }

    QFutureCallOutEvent(CallOutType callOutType, int index1, const QString &text)
        : QEvent(QEvent::FutureCallOut),
          callOutType(callOutType),
          index1(index1),
          index2(-1),
          text(text)
    { }

    CallOutType callOutType;
    int index1;
    int index2;
    QString text;

    QFutureCallOutEvent *clone() const
    {
        return new QFutureCallOutEvent(callOutType, index1, index2, text);
    }

private:
    QFutureCallOutEvent(CallOutType callOutType,
                        int index1,
                        int index2,
                        const QString &text)
        : QEvent(QEvent::FutureCallOut),
          callOutType(callOutType),
          index1(index1),
          index2(index2),
          text(text)
    { }
};

class QFutureCallOutInterface
{
public:
    virtual ~QFutureCallOutInterface() {}
    virtual void postCallOutEvent(const QFutureCallOutEvent &) = 0;
    virtual void callOutInterfaceDisconnected() = 0;
};

class QFutureInterfaceBasePrivate
{
public:
    QFutureInterfaceBasePrivate(QFutureInterfaceBase::State initialState);

    // When the last QFuture<T> reference is removed, we need to make
    // sure that data stored in the ResultStore is cleaned out.
    // Since QFutureInterfaceBasePrivate can be shared between QFuture<T>
    // and QFuture<void> objects, we use a separate ref. counter
    // to keep track of QFuture<T> objects.
    class RefCount
    {
    public:
        inline RefCount(int r = 0, int rt = 0)
            : m_refCount(r), m_refCountT(rt) {}
        // Default ref counter for QFIBP
        inline bool ref() { return m_refCount.ref(); }
        inline bool deref() { return m_refCount.deref(); }
        inline int load() const { return m_refCount.load(); }
        // Ref counter for type T
        inline bool refT() { return m_refCountT.ref(); }
        inline bool derefT() { return m_refCountT.deref(); }
        inline int loadT() const { return m_refCountT.load(); }

    private:
        QAtomicInt m_refCount;
        QAtomicInt m_refCountT;
    };

    RefCount refCount;
    mutable QMutex m_mutex;
    QWaitCondition waitCondition;
    QList<QFutureCallOutInterface *> outputConnections;
    int m_progressValue;
    int m_progressMinimum;
    int m_progressMaximum;
    QFutureInterfaceBase::State state;
    QElapsedTimer progressTime;
    QWaitCondition pausedWaitCondition;
    QtPrivate::ResultStoreBase m_results;
    bool manualProgress;
    int m_expectedResultCount;
    QtPrivate::ExceptionStore m_exceptionStore;
    QString m_progressText;
    QRunnable *runnable;
    QThreadPool *m_pool;

    inline QThreadPool *pool() const
    { return m_pool ? m_pool : QThreadPool::globalInstance(); }

    // Internal functions that does not change the mutex state.
    // The mutex must be locked when calling these.
    int internal_resultCount() const;
    bool internal_isResultReadyAt(int index) const;
    bool internal_waitForNextResult();
    bool internal_updateProgress(int progress, const QString &progressText = QString());
    void internal_setThrottled(bool enable);
    void sendCallOut(const QFutureCallOutEvent &callOut);
    void sendCallOuts(const QFutureCallOutEvent &callOut1, const QFutureCallOutEvent &callOut2);
    void connectOutputInterface(QFutureCallOutInterface *iface);
    void disconnectOutputInterface(QFutureCallOutInterface *iface);

    void setState(QFutureInterfaceBase::State state);
};

QT_END_NAMESPACE

#endif
