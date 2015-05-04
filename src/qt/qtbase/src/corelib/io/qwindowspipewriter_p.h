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

#ifndef QWINDOWSPIPEWRITER_P_H
#define QWINDOWSPIPEWRITER_P_H

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

#include <qdatetime.h>
#include <qthread.h>
#include <qmutex.h>
#include <qwaitcondition.h>
#include <qt_windows.h>

QT_BEGIN_NAMESPACE


#ifndef QT_NO_THREAD

#define SLEEPMIN 10
#define SLEEPMAX 500

class QIncrementalSleepTimer
{

public:
    QIncrementalSleepTimer(int msecs)
        : totalTimeOut(msecs)
        , nextSleep(qMin(SLEEPMIN, totalTimeOut))
    {
        if (totalTimeOut == -1)
            nextSleep = SLEEPMIN;
        timer.start();
    }

    int nextSleepTime()
    {
        int tmp = nextSleep;
        nextSleep = qMin(nextSleep * 2, qMin(SLEEPMAX, timeLeft()));
        return tmp;
    }

    int timeLeft() const
    {
        if (totalTimeOut == -1)
            return SLEEPMAX;
        return qMax(totalTimeOut - timer.elapsed(), 0);
    }

    bool hasTimedOut() const
    {
        if (totalTimeOut == -1)
            return false;
        return timer.elapsed() >= totalTimeOut;
    }

    void resetIncrements()
    {
        nextSleep = qMin(SLEEPMIN, timeLeft());
    }

private:
    QTime timer;
    int totalTimeOut;
    int nextSleep;
};

class Q_CORE_EXPORT QWindowsPipeWriter : public QThread
{
    Q_OBJECT

Q_SIGNALS:
    void canWrite();
    void bytesWritten(qint64 bytes);

public:
    explicit QWindowsPipeWriter(HANDLE writePipe, QObject * parent = 0);
    ~QWindowsPipeWriter();

    bool waitForWrite(int msecs);
    qint64 write(const char *data, qint64 maxlen);

    qint64 bytesToWrite() const
    {
        QMutexLocker locker(&lock);
        return data.size();
    }

    bool hadWritten() const
    {
        return hasWritten;
    }

protected:
   void run();

private:
    QByteArray data;
    QWaitCondition waitCondition;
    mutable QMutex lock;
    HANDLE writePipe;
    volatile bool quitNow;
    bool hasWritten;
};

#endif //QT_NO_THREAD

QT_END_NAMESPACE

#endif // QT_NO_PROCESS
