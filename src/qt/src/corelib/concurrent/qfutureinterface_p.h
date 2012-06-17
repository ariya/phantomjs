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
    QFutureCallOutEvent(CallOutType callOutType, int index1 = -1)
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

    QAtomicInt refCount;
    mutable QMutex m_mutex;
    QWaitCondition waitCondition;
    QList<QFutureCallOutInterface *> outputConnections;
    int m_progressValue;
    int m_progressMinimum;
    int m_progressMaximum;
    QFutureInterfaceBase::State state;
    QElapsedTimer progressTime;
    QWaitCondition pausedWaitCondition;
    int pendingResults;
    QtConcurrent::ResultStoreBase m_results;
    bool manualProgress;
    int m_expectedResultCount;
    QtConcurrent::internal::ExceptionStore m_exceptionStore;
    QString m_progressText;
    QRunnable *runnable;

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
