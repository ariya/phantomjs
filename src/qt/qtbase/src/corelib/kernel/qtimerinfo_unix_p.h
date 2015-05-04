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

#ifndef QTIMERINFO_UNIX_P_H
#define QTIMERINFO_UNIX_P_H

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

// #define QTIMERINFO_DEBUG

#include "qabstracteventdispatcher.h"

#include <sys/time.h> // struct timeval

QT_BEGIN_NAMESPACE

// internal timer info
struct QTimerInfo {
    int id;           // - timer identifier
    int interval;     // - timer interval in milliseconds
    Qt::TimerType timerType; // - timer type
    timespec timeout;  // - when to actually fire
    QObject *obj;     // - object to receive event
    QTimerInfo **activateRef; // - ref from activateTimers

#ifdef QTIMERINFO_DEBUG
    timeval expected; // when timer is expected to fire
    float cumulativeError;
    uint count;
#endif
};

class Q_CORE_EXPORT QTimerInfoList : public QList<QTimerInfo*>
{
#if ((_POSIX_MONOTONIC_CLOCK-0 <= 0) && !defined(Q_OS_MAC)) || defined(QT_BOOTSTRAPPED)
    timespec previousTime;
    clock_t previousTicks;
    int ticksPerSecond;
    int msPerTick;

    bool timeChanged(timespec *delta);
    void timerRepair(const timespec &);
#endif

    // state variables used by activateTimers()
    QTimerInfo *firstTimerInfo;

public:
    QTimerInfoList();

    timespec currentTime;
    timespec updateCurrentTime();

    // must call updateCurrentTime() first!
    void repairTimersIfNeeded();

    bool timerWait(timespec &);
    void timerInsert(QTimerInfo *);

    int timerRemainingTime(int timerId);

    void registerTimer(int timerId, int interval, Qt::TimerType timerType, QObject *object);
    bool unregisterTimer(int timerId);
    bool unregisterTimers(QObject *object);
    QList<QAbstractEventDispatcher::TimerInfo> registeredTimers(QObject *object) const;

    int activateTimers();
};

QT_END_NAMESPACE

#endif // QTIMERINFO_UNIX_P_H
