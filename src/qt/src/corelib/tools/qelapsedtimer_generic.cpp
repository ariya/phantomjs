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

#include "qelapsedtimer.h"
#include "qdatetime.h"

QT_BEGIN_NAMESPACE

/*!
    Returns the clock type that this QElapsedTimer implementation uses.

    \sa isMonotonic()
*/
QElapsedTimer::ClockType QElapsedTimer::clockType()
{
    return SystemTime;
}

/*!
    Returns true if this is a monotonic clock, false otherwise. See the
    information on the different clock types to understand which ones are
    monotonic.

    \sa clockType(), QElapsedTimer::ClockType
*/
bool QElapsedTimer::isMonotonic()
{
    return false;
}

/*!
    Starts this timer. Once started, a timer value can be checked with elapsed() or msecsSinceReference().

    Normally, a timer is started just before a lengthy operation, such as:
    \snippet doc/src/snippets/qelapsedtimer/main.cpp 0

    Also, starting a timer makes it valid again.

    \sa restart(), invalidate(), elapsed()
*/
void QElapsedTimer::start()
{
    restart();
}

/*!
    Restarts the timer and returns the time elapsed since the previous start.
    This function is equivalent to obtaining the elapsed time with elapsed()
    and then starting the timer again with start(), but it does so in one
    single operation, avoiding the need to obtain the clock value twice.

    The following example illustrates how to use this function to calibrate a
    parameter to a slow operation (for example, an iteration count) so that
    this operation takes at least 250 milliseconds:

    \snippet doc/src/snippets/qelapsedtimer/main.cpp 3

    \sa start(), invalidate(), elapsed()
*/
qint64 QElapsedTimer::restart()
{
    qint64 old = t1;
    t1 = QDateTime::currentMSecsSinceEpoch();
    t2 = 0;
    return t1 - old;
}

/*! \since 4.8

    Returns the number of nanoseconds since this QElapsedTimer was last
    started. Calling this function in a QElapsedTimer that was invalidated
    will result in undefined results.

    On platforms that do not provide nanosecond resolution, the value returned
    will be the best estimate available.

    \sa start(), restart(), hasExpired(), invalidate()
*/
qint64 QElapsedTimer::nsecsElapsed() const
{
    return elapsed() * 1000000;
}

/*!
    Returns the number of milliseconds since this QElapsedTimer was last
    started. Calling this function in a QElapsedTimer that was invalidated
    will result in undefined results.

    \sa start(), restart(), hasExpired(), invalidate()
*/
qint64 QElapsedTimer::elapsed() const
{
    return QDateTime::currentMSecsSinceEpoch() - t1;
}

/*!
    Returns the number of milliseconds between last time this QElapsedTimer
    object was started and its reference clock's start.

    This number is usually arbitrary for all clocks except the
    QElapsedTimer::SystemTime clock. For that clock type, this number is the
    number of milliseconds since January 1st, 1970 at 0:00 UTC (that is, it
    is the Unix time expressed in milliseconds).

    \sa clockType(), elapsed()
*/
qint64 QElapsedTimer::msecsSinceReference() const
{
    return t1;
}

/*!
    Returns the number of milliseconds between this QElapsedTimer and \a
    other. If \a other was started before this object, the returned value
    will be positive. If it was started later, the returned value will be
    negative.

    The return value is undefined if this object or \a other were invalidated.

    \sa secsTo(), elapsed()
*/
qint64 QElapsedTimer::msecsTo(const QElapsedTimer &other) const
{
    qint64 diff = other.t1 - t1;
    return diff;
}

/*!
    Returns the number of seconds between this QElapsedTimer and \a other. If
    \a other was started before this object, the returned value will be
    positive. If it was started later, the returned value will be negative.

    The return value is undefined if this object or \a other were invalidated.

    \sa msecsTo(), elapsed()
*/
qint64 QElapsedTimer::secsTo(const QElapsedTimer &other) const
{
    return msecsTo(other) / 1000;
}

/*!
    \relates QElapsedTimer

    Returns true if \a v1 was started before \a v2, false otherwise.

    The returned value is undefined if one of the two parameters is invalid
    and the other isn't. However, two invalid timers are equal and thus this
    function will return false.
*/
bool operator<(const QElapsedTimer &v1, const QElapsedTimer &v2)
{
    return v1.t1 < v2.t1;
}

QT_END_NAMESPACE
