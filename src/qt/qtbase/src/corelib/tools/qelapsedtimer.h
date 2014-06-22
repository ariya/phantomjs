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

#ifndef QELAPSEDTIMER_H
#define QELAPSEDTIMER_H

#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE


class Q_CORE_EXPORT QElapsedTimer
{
public:
    enum ClockType {
        SystemTime,
        MonotonicClock,
        TickCounter,
        MachAbsoluteTime,
        PerformanceCounter
    };
    static ClockType clockType() Q_DECL_NOTHROW;
    static bool isMonotonic() Q_DECL_NOTHROW;

    void start() Q_DECL_NOTHROW;
    qint64 restart() Q_DECL_NOTHROW;
    void invalidate() Q_DECL_NOTHROW;
    bool isValid() const Q_DECL_NOTHROW;

    qint64 nsecsElapsed() const Q_DECL_NOTHROW;
    qint64 elapsed() const Q_DECL_NOTHROW;
    bool hasExpired(qint64 timeout) const Q_DECL_NOTHROW;

    qint64 msecsSinceReference() const Q_DECL_NOTHROW;
    qint64 msecsTo(const QElapsedTimer &other) const Q_DECL_NOTHROW;
    qint64 secsTo(const QElapsedTimer &other) const Q_DECL_NOTHROW;

    bool operator==(const QElapsedTimer &other) const Q_DECL_NOTHROW
    { return t1 == other.t1 && t2 == other.t2; }
    bool operator!=(const QElapsedTimer &other) const Q_DECL_NOTHROW
    { return !(*this == other); }

    friend bool Q_CORE_EXPORT operator<(const QElapsedTimer &v1, const QElapsedTimer &v2) Q_DECL_NOTHROW;

private:
    qint64 t1;
    qint64 t2;
};

QT_END_NAMESPACE

#endif // QELAPSEDTIMER_H
