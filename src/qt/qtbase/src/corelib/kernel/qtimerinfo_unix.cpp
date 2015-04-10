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

#include <qelapsedtimer.h>
#include <qcoreapplication.h>

#include "private/qcore_unix_p.h"
#include "private/qtimerinfo_unix_p.h"
#include "private/qobject_p.h"
#include "private/qabstracteventdispatcher_p.h"

#ifdef QTIMERINFO_DEBUG
#  include <QDebug>
#  include <QThread>
#endif

#include <sys/times.h>

QT_BEGIN_NAMESPACE

Q_CORE_EXPORT bool qt_disable_lowpriority_timers=false;

/*
 * Internal functions for manipulating timer data structures.  The
 * timerBitVec array is used for keeping track of timer identifiers.
 */

QTimerInfoList::QTimerInfoList()
{
#if (_POSIX_MONOTONIC_CLOCK-0 <= 0) && !defined(Q_OS_MAC) && !defined(Q_OS_NACL)
    if (!QElapsedTimer::isMonotonic()) {
        // not using monotonic timers, initialize the timeChanged() machinery
        previousTime = qt_gettime();

        tms unused;
        previousTicks = times(&unused);

        ticksPerSecond = sysconf(_SC_CLK_TCK);
        msPerTick = 1000/ticksPerSecond;
    } else {
        // detected monotonic timers
        previousTime.tv_sec = previousTime.tv_nsec = 0;
        previousTicks = 0;
        ticksPerSecond = 0;
        msPerTick = 0;
    }
#endif

    firstTimerInfo = 0;
}

timespec QTimerInfoList::updateCurrentTime()
{
    return (currentTime = qt_gettime());
}

#if ((_POSIX_MONOTONIC_CLOCK-0 <= 0) && !defined(Q_OS_MAC) && !defined(Q_OS_INTEGRITY)) || defined(QT_BOOTSTRAPPED)

timespec qAbsTimespec(const timespec &t)
{
    timespec tmp = t;
    if (tmp.tv_sec < 0) {
        tmp.tv_sec = -tmp.tv_sec - 1;
        tmp.tv_nsec -= 1000000000;
    }
    if (tmp.tv_sec == 0 && tmp.tv_nsec < 0) {
        tmp.tv_nsec = -tmp.tv_nsec;
    }
    return normalizedTimespec(tmp);
}

/*
  Returns \c true if the real time clock has changed by more than 10%
  relative to the processor time since the last time this function was
  called. This presumably means that the system time has been changed.

  If /a delta is nonzero, delta is set to our best guess at how much the system clock was changed.
*/
bool QTimerInfoList::timeChanged(timespec *delta)
{
#ifdef Q_OS_NACL
    Q_UNUSED(delta)
    return false; // Calling "times" crashes.
#endif
    struct tms unused;
    clock_t currentTicks = times(&unused);

    clock_t elapsedTicks = currentTicks - previousTicks;
    timespec elapsedTime = currentTime - previousTime;

    timespec elapsedTimeTicks;
    elapsedTimeTicks.tv_sec = elapsedTicks / ticksPerSecond;
    elapsedTimeTicks.tv_nsec = (((elapsedTicks * 1000) / ticksPerSecond) % 1000) * 1000 * 1000;

    timespec dummy;
    if (!delta)
        delta = &dummy;
    *delta = elapsedTime - elapsedTimeTicks;

    previousTicks = currentTicks;
    previousTime = currentTime;

    // If tick drift is more than 10% off compared to realtime, we assume that the clock has
    // been set. Of course, we have to allow for the tick granularity as well.
    timespec tickGranularity;
    tickGranularity.tv_sec = 0;
    tickGranularity.tv_nsec = msPerTick * 1000 * 1000;
    return elapsedTimeTicks < ((qAbsTimespec(*delta) - tickGranularity) * 10);
}

/*
  repair broken timer
*/
void QTimerInfoList::timerRepair(const timespec &diff)
{
    // repair all timers
    for (int i = 0; i < size(); ++i) {
        QTimerInfo *t = at(i);
        t->timeout = t->timeout + diff;
    }
}

void QTimerInfoList::repairTimersIfNeeded()
{
    if (QElapsedTimer::isMonotonic())
        return;
    timespec delta;
    if (timeChanged(&delta))
        timerRepair(delta);
}

#else // !(_POSIX_MONOTONIC_CLOCK-0 <= 0) && !defined(QT_BOOTSTRAPPED)

void QTimerInfoList::repairTimersIfNeeded()
{
}

#endif

/*
  insert timer info into list
*/
void QTimerInfoList::timerInsert(QTimerInfo *ti)
{
    int index = size();
    while (index--) {
        const QTimerInfo * const t = at(index);
        if (!(ti->timeout < t->timeout))
            break;
    }
    insert(index+1, ti);
}

inline timespec &operator+=(timespec &t1, int ms)
{
    t1.tv_sec += ms / 1000;
    t1.tv_nsec += ms % 1000 * 1000 * 1000;
    return normalizedTimespec(t1);
}

inline timespec operator+(const timespec &t1, int ms)
{
    timespec t2 = t1;
    return t2 += ms;
}

static timespec roundToMillisecond(timespec val)
{
    // always round up
    // worst case scenario is that the first trigger of a 1-ms timer is 0.999 ms late

    int ns = val.tv_nsec % (1000 * 1000);
    val.tv_nsec += 1000 * 1000 - ns;
    return normalizedTimespec(val);
}

#ifdef QTIMERINFO_DEBUG
QDebug operator<<(QDebug s, timeval tv)
{
    s.nospace() << tv.tv_sec << "." << qSetFieldWidth(6) << qSetPadChar(QChar(48)) << tv.tv_usec << reset;
    return s.space();
}
QDebug operator<<(QDebug s, Qt::TimerType t)
{
    s << (t == Qt::PreciseTimer ? "P" :
          t == Qt::CoarseTimer ? "C" : "VC");
    return s;
}
#endif

static void calculateCoarseTimerTimeout(QTimerInfo *t, timespec currentTime)
{
    // The coarse timer works like this:
    //  - interval under 40 ms: round to even
    //  - between 40 and 99 ms: round to multiple of 4
    //  - otherwise: try to wake up at a multiple of 25 ms, with a maximum error of 5%
    //
    // We try to wake up at the following second-fraction, in order of preference:
    //    0 ms
    //  500 ms
    //  250 ms or 750 ms
    //  200, 400, 600, 800 ms
    //  other multiples of 100
    //  other multiples of 50
    //  other multiples of 25
    //
    // The objective is to make most timers wake up at the same time, thereby reducing CPU wakeups.

    uint interval = uint(t->interval);
    uint msec = uint(t->timeout.tv_nsec) / 1000 / 1000;
    Q_ASSERT(interval >= 20);

    // Calculate how much we can round and still keep within 5% error
    uint absMaxRounding = interval / 20;

    if (interval < 100 && interval != 25 && interval != 50 && interval != 75) {
        // special mode for timers of less than 100 ms
        if (interval < 50) {
            // round to even
            // round towards multiples of 50 ms
            bool roundUp = (msec % 50) >= 25;
            msec >>= 1;
            msec |= uint(roundUp);
            msec <<= 1;
        } else {
            // round to multiple of 4
            // round towards multiples of 100 ms
            bool roundUp = (msec % 100) >= 50;
            msec >>= 2;
            msec |= uint(roundUp);
            msec <<= 2;
        }
    } else {
        uint min = qMax<int>(0, msec - absMaxRounding);
        uint max = qMin(1000u, msec + absMaxRounding);

        // find the boundary that we want, according to the rules above
        // extra rules:
        // 1) whatever the interval, we'll take any round-to-the-second timeout
        if (min == 0) {
            msec = 0;
            goto recalculate;
        } else if (max == 1000) {
            msec = 1000;
            goto recalculate;
        }

        uint wantedBoundaryMultiple;

        // 2) if the interval is a multiple of 500 ms and > 5000 ms, we'll always round
        //    towards a round-to-the-second
        // 3) if the interval is a multiple of 500 ms, we'll round towards the nearest
        //    multiple of 500 ms
        if ((interval % 500) == 0) {
            if (interval >= 5000) {
                msec = msec >= 500 ? max : min;
                goto recalculate;
            } else {
                wantedBoundaryMultiple = 500;
            }
        } else if ((interval % 50) == 0) {
            // 4) same for multiples of 250, 200, 100, 50
            uint mult50 = interval / 50;
            if ((mult50 % 4) == 0) {
                // multiple of 200
                wantedBoundaryMultiple = 200;
            } else if ((mult50 % 2) == 0) {
                // multiple of 100
                wantedBoundaryMultiple = 100;
            } else if ((mult50 % 5) == 0) {
                // multiple of 250
                wantedBoundaryMultiple = 250;
            } else {
                // multiple of 50
                wantedBoundaryMultiple = 50;
            }
        } else {
            wantedBoundaryMultiple = 25;
        }

        uint base = msec / wantedBoundaryMultiple * wantedBoundaryMultiple;
        uint middlepoint = base + wantedBoundaryMultiple / 2;
        if (msec < middlepoint)
            msec = qMax(base, min);
        else
            msec = qMin(base + wantedBoundaryMultiple, max);
    }

recalculate:
    if (msec == 1000u) {
        ++t->timeout.tv_sec;
        t->timeout.tv_nsec = 0;
    } else {
        t->timeout.tv_nsec = msec * 1000 * 1000;
    }

    if (t->timeout < currentTime)
        t->timeout += interval;
}

static void calculateNextTimeout(QTimerInfo *t, timespec currentTime)
{
    switch (t->timerType) {
    case Qt::PreciseTimer:
    case Qt::CoarseTimer:
        t->timeout += t->interval;
        if (t->timeout < currentTime) {
            t->timeout = currentTime;
            t->timeout += t->interval;
        }
#ifdef QTIMERINFO_DEBUG
        t->expected += t->interval;
        if (t->expected < currentTime) {
            t->expected = currentTime;
            t->expected += t->interval;
        }
#endif
        if (t->timerType == Qt::CoarseTimer)
            calculateCoarseTimerTimeout(t, currentTime);
        return;

    case Qt::VeryCoarseTimer:
        // we don't need to take care of the microsecond component of t->interval
        t->timeout.tv_sec += t->interval;
        if (t->timeout.tv_sec <= currentTime.tv_sec)
            t->timeout.tv_sec = currentTime.tv_sec + t->interval;
#ifdef QTIMERINFO_DEBUG
        t->expected.tv_sec += t->interval;
        if (t->expected.tv_sec <= currentTime.tv_sec)
            t->expected.tv_sec = currentTime.tv_sec + t->interval;
#endif
        return;
    }

#ifdef QTIMERINFO_DEBUG
    if (t->timerType != Qt::PreciseTimer)
    qDebug() << "timer" << t->timerType << hex << t->id << dec << "interval" << t->interval
            << "originally expected at" << t->expected << "will fire at" << t->timeout
            << "or" << (t->timeout - t->expected) << "s late";
#endif
}

/*
  Returns the time to wait for the next timer, or null if no timers
  are waiting.
*/
bool QTimerInfoList::timerWait(timespec &tm)
{
    timespec currentTime = updateCurrentTime();
    repairTimersIfNeeded();

    // Find first waiting timer not already active
    QTimerInfo *t = 0;
    for (QTimerInfoList::const_iterator it = constBegin(); it != constEnd(); ++it) {
        if (!(*it)->activateRef) {
            t = *it;
            break;
        }
    }

    if (!t)
      return false;

    if (currentTime < t->timeout) {
        // time to wait
        tm = roundToMillisecond(t->timeout - currentTime);
    } else {
        // no time to wait
        tm.tv_sec  = 0;
        tm.tv_nsec = 0;
    }

    return true;
}

/*
  Returns the timer's remaining time in milliseconds with the given timerId, or
  null if there is nothing left. If the timer id is not found in the list, the
  returned value will be -1. If the timer is overdue, the returned value will be 0.
*/
int QTimerInfoList::timerRemainingTime(int timerId)
{
    timespec currentTime = updateCurrentTime();
    repairTimersIfNeeded();
    timespec tm = {0, 0};

    for (int i = 0; i < count(); ++i) {
        QTimerInfo *t = at(i);
        if (t->id == timerId) {
            if (currentTime < t->timeout) {
                // time to wait
                tm = roundToMillisecond(t->timeout - currentTime);
                return tm.tv_sec*1000 + tm.tv_nsec/1000/1000;
            } else {
                return 0;
            }
        }
    }

#ifndef QT_NO_DEBUG
    qWarning("QTimerInfoList::timerRemainingTime: timer id %i not found", timerId);
#endif

    return -1;
}

void QTimerInfoList::registerTimer(int timerId, int interval, Qt::TimerType timerType, QObject *object)
{
    QTimerInfo *t = new QTimerInfo;
    t->id = timerId;
    t->interval = interval;
    t->timerType = timerType;
    t->obj = object;
    t->activateRef = 0;

    timespec expected = updateCurrentTime() + interval;

    switch (timerType) {
    case Qt::PreciseTimer:
        // high precision timer is based on millisecond precision
        // so no adjustment is necessary
        t->timeout = expected;
        break;

    case Qt::CoarseTimer:
        // this timer has up to 5% coarseness
        // so our boundaries are 20 ms and 20 s
        // below 20 ms, 5% inaccuracy is below 1 ms, so we convert to high precision
        // above 20 s, 5% inaccuracy is above 1 s, so we convert to VeryCoarseTimer
        if (interval >= 20000) {
            t->timerType = Qt::VeryCoarseTimer;
            // fall through
        } else {
            t->timeout = expected;
            if (interval <= 20) {
                t->timerType = Qt::PreciseTimer;
                // no adjustment is necessary
            } else if (interval <= 20000) {
                calculateCoarseTimerTimeout(t, currentTime);
            }
            break;
        }
        // fall through
    case Qt::VeryCoarseTimer:
        // the very coarse timer is based on full second precision,
        // so we keep the interval in seconds (round to closest second)
        t->interval /= 500;
        t->interval += 1;
        t->interval >>= 1;
        t->timeout.tv_sec = currentTime.tv_sec + t->interval;
        t->timeout.tv_nsec = 0;

        // if we're past the half-second mark, increase the timeout again
        if (currentTime.tv_nsec > 500*1000*1000)
            ++t->timeout.tv_sec;
    }

    timerInsert(t);

#ifdef QTIMERINFO_DEBUG
    t->expected = expected;
    t->cumulativeError = 0;
    t->count = 0;
    if (t->timerType != Qt::PreciseTimer)
    qDebug() << "timer" << t->timerType << hex <<t->id << dec << "interval" << t->interval << "expected at"
            << t->expected << "will fire first at" << t->timeout;
#endif
}

bool QTimerInfoList::unregisterTimer(int timerId)
{
    // set timer inactive
    for (int i = 0; i < count(); ++i) {
        QTimerInfo *t = at(i);
        if (t->id == timerId) {
            // found it
            removeAt(i);
            if (t == firstTimerInfo)
                firstTimerInfo = 0;
            if (t->activateRef)
                *(t->activateRef) = 0;
            delete t;
            return true;
        }
    }
    // id not found
    return false;
}

bool QTimerInfoList::unregisterTimers(QObject *object)
{
    if (isEmpty())
        return false;
    for (int i = 0; i < count(); ++i) {
        QTimerInfo *t = at(i);
        if (t->obj == object) {
            // object found
            removeAt(i);
            if (t == firstTimerInfo)
                firstTimerInfo = 0;
            if (t->activateRef)
                *(t->activateRef) = 0;
            delete t;
            // move back one so that we don't skip the new current item
            --i;
        }
    }
    return true;
}

QList<QAbstractEventDispatcher::TimerInfo> QTimerInfoList::registeredTimers(QObject *object) const
{
    QList<QAbstractEventDispatcher::TimerInfo> list;
    for (int i = 0; i < count(); ++i) {
        const QTimerInfo * const t = at(i);
        if (t->obj == object) {
            list << QAbstractEventDispatcher::TimerInfo(t->id,
                                                        (t->timerType == Qt::VeryCoarseTimer
                                                         ? t->interval * 1000
                                                         : t->interval),
                                                        t->timerType);
        }
    }
    return list;
}

/*
    Activate pending timers, returning how many where activated.
*/
int QTimerInfoList::activateTimers()
{
    if (qt_disable_lowpriority_timers || isEmpty())
        return 0; // nothing to do

    int n_act = 0, maxCount = 0;
    firstTimerInfo = 0;

    timespec currentTime = updateCurrentTime();
    // qDebug() << "Thread" << QThread::currentThreadId() << "woken up at" << currentTime;
    repairTimersIfNeeded();


    // Find out how many timer have expired
    for (QTimerInfoList::const_iterator it = constBegin(); it != constEnd(); ++it) {
        if (currentTime < (*it)->timeout)
            break;
        maxCount++;
    }

    //fire the timers.
    while (maxCount--) {
        if (isEmpty())
            break;

        QTimerInfo *currentTimerInfo = first();
        if (currentTime < currentTimerInfo->timeout)
            break; // no timer has expired

        if (!firstTimerInfo) {
            firstTimerInfo = currentTimerInfo;
        } else if (firstTimerInfo == currentTimerInfo) {
            // avoid sending the same timer multiple times
            break;
        } else if (currentTimerInfo->interval <  firstTimerInfo->interval
                   || currentTimerInfo->interval == firstTimerInfo->interval) {
            firstTimerInfo = currentTimerInfo;
        }

        // remove from list
        removeFirst();

#ifdef QTIMERINFO_DEBUG
        float diff;
        if (currentTime < currentTimerInfo->expected) {
            // early
            timeval early = currentTimerInfo->expected - currentTime;
            diff = -(early.tv_sec + early.tv_usec / 1000000.0);
        } else {
            timeval late = currentTime - currentTimerInfo->expected;
            diff = late.tv_sec + late.tv_usec / 1000000.0;
        }
        currentTimerInfo->cumulativeError += diff;
        ++currentTimerInfo->count;
        if (currentTimerInfo->timerType != Qt::PreciseTimer)
        qDebug() << "timer" << currentTimerInfo->timerType << hex << currentTimerInfo->id << dec << "interval"
                << currentTimerInfo->interval << "firing at" << currentTime
                << "(orig" << currentTimerInfo->expected << "scheduled at" << currentTimerInfo->timeout
                << ") off by" << diff << "activation" << currentTimerInfo->count
                << "avg error" << (currentTimerInfo->cumulativeError / currentTimerInfo->count);
#endif

        // determine next timeout time
        calculateNextTimeout(currentTimerInfo, currentTime);

        // reinsert timer
        timerInsert(currentTimerInfo);
        if (currentTimerInfo->interval > 0)
            n_act++;

        if (!currentTimerInfo->activateRef) {
            // send event, but don't allow it to recurse
            currentTimerInfo->activateRef = &currentTimerInfo;

            QTimerEvent e(currentTimerInfo->id);
            QCoreApplication::sendEvent(currentTimerInfo->obj, &e);

            if (currentTimerInfo)
                currentTimerInfo->activateRef = 0;
        }
    }

    firstTimerInfo = 0;
    // qDebug() << "Thread" << QThread::currentThreadId() << "activated" << n_act << "timers";
    return n_act;
}

QT_END_NAMESPACE
