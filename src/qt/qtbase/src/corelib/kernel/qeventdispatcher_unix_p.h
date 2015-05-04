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

#ifndef QEVENTDISPATCHER_UNIX_P_H
#define QEVENTDISPATCHER_UNIX_P_H

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

#include "QtCore/qabstracteventdispatcher.h"
#include "QtCore/qlist.h"
#include "private/qabstracteventdispatcher_p.h"
#include "private/qcore_unix_p.h"
#include "private/qpodlist_p.h"
#include "QtCore/qvarlengtharray.h"
#include "private/qtimerinfo_unix_p.h"

#if !defined(Q_OS_VXWORKS)
#  include <sys/time.h>
#  if (!defined(Q_OS_HPUX) || defined(__ia64)) && !defined(Q_OS_NACL)
#    include <sys/select.h>
#  endif
#endif

QT_BEGIN_NAMESPACE

struct QSockNot
{
    QSocketNotifier *obj;
    int fd;
    fd_set *queue;
};

class QSockNotType
{
public:
    QSockNotType();
    ~QSockNotType();

    typedef QPodList<QSockNot*, 32> List;

    List list;
    fd_set select_fds;
    fd_set enabled_fds;
    fd_set pending_fds;

};

class QEventDispatcherUNIXPrivate;

#ifdef Q_OS_QNX
#  define FINAL_EXCEPT_BLACKBERRY
#else
#  define FINAL_EXCEPT_BLACKBERRY Q_DECL_FINAL
#endif

class Q_CORE_EXPORT QEventDispatcherUNIX : public QAbstractEventDispatcher
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QEventDispatcherUNIX)

public:
    explicit QEventDispatcherUNIX(QObject *parent = 0);
    ~QEventDispatcherUNIX();

    bool processEvents(QEventLoop::ProcessEventsFlags flags);
    bool hasPendingEvents();

    void registerSocketNotifier(QSocketNotifier *notifier) FINAL_EXCEPT_BLACKBERRY;
    void unregisterSocketNotifier(QSocketNotifier *notifier) FINAL_EXCEPT_BLACKBERRY;

    void registerTimer(int timerId, int interval, Qt::TimerType timerType, QObject *object) Q_DECL_FINAL;
    bool unregisterTimer(int timerId) Q_DECL_FINAL;
    bool unregisterTimers(QObject *object) Q_DECL_FINAL;
    QList<TimerInfo> registeredTimers(QObject *object) const Q_DECL_FINAL;

    int remainingTime(int timerId) Q_DECL_FINAL;

    void wakeUp() FINAL_EXCEPT_BLACKBERRY;
    void interrupt() Q_DECL_FINAL;
    void flush();

protected:
    QEventDispatcherUNIX(QEventDispatcherUNIXPrivate &dd, QObject *parent = 0);

    void setSocketNotifierPending(QSocketNotifier *notifier);

    int activateTimers();
    int activateSocketNotifiers();

    virtual int select(int nfds,
                       fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
                       timespec *timeout);
};

class Q_CORE_EXPORT QEventDispatcherUNIXPrivate : public QAbstractEventDispatcherPrivate
{
    Q_DECLARE_PUBLIC(QEventDispatcherUNIX)

public:
    QEventDispatcherUNIXPrivate();
    ~QEventDispatcherUNIXPrivate();

    int doSelect(QEventLoop::ProcessEventsFlags flags, timespec *timeout);
    virtual int initThreadWakeUp() FINAL_EXCEPT_BLACKBERRY;
    virtual int processThreadWakeUp(int nsel) FINAL_EXCEPT_BLACKBERRY;

    bool mainThread;

    // note for eventfd(7) support:
    // if thread_pipe[1] is -1, then eventfd(7) is in use and is stored in thread_pipe[0]
    int thread_pipe[2];

    // highest fd for all socket notifiers
    int sn_highest;
    // 3 socket notifier types - read, write and exception
    QSockNotType sn_vec[3];

    QTimerInfoList timerList;

    // pending socket notifiers list
    QSockNotType::List sn_pending_list;

    QAtomicInt wakeUps;
    QAtomicInt interrupt; // bool
};

#undef FINAL_EXCEPT_BLACKBERRY

QT_END_NAMESPACE

#endif // QEVENTDISPATCHER_UNIX_P_H
