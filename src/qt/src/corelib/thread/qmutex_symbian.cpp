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

#include "qplatformdefs.h"
#include "qmutex.h"

#ifndef QT_NO_THREAD
#include "qatomic.h"
#include "qelapsedtimer.h"
#include "qthread.h"
#include "qmutex_p.h"

QT_BEGIN_NAMESPACE


QMutexPrivate::QMutexPrivate(QMutex::RecursionMode mode)
    : QMutexData(mode), maximumSpinTime(MaximumSpinTimeThreshold), averageWaitTime(0), owner(0), count(0)
{
    int r = lock.CreateLocal(0);
    if (r != KErrNone)
        qWarning("QMutex: failed to create lock, error %d", r);
    qt_symbian_throwIfError(r);
}

QMutexPrivate::~QMutexPrivate()
{
    lock.Close();
}

bool QMutexPrivate::wait(int timeout)
{
    if (contenders.fetchAndAddAcquire(1) == 0) {
        // lock acquired without waiting
        return true;
    }
    int r = KErrTimedOut;
    if (timeout < 0) {
        lock.Wait();
        r = KErrNone;
    } else {
        // Symbian lock waits are specified in microseconds.
        // The wait is therefore chunked.
        // KErrNone indicates success, KErrGeneral and KErrArgument are real fails, anything else is a timeout
        do {
            int waitTime = qMin(KMaxTInt / 1000, timeout);
            timeout -= waitTime;
            // Symbian undocumented feature - 0us means no timeout! Use a minimum of 1
            r = lock.Wait(qMax(1, waitTime * 1000));
        } while (r != KErrNone && r != KErrGeneral && r != KErrArgument && timeout > 0);
    }
    bool returnValue = (r == KErrNone);
    contenders.deref();
    return returnValue;
}

void QMutexPrivate::wakeUp()
{
    lock.Signal();
}

QT_END_NAMESPACE

#endif // QT_NO_THREAD
