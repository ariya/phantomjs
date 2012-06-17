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

#ifndef QTHREADPOOL_P_H
#define QTHREADPOOL_P_H

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
//

#include "QtCore/qmutex.h"
#include "QtCore/qwaitcondition.h"
#include "QtCore/qset.h"
#include "QtCore/qqueue.h"
#include "private/qobject_p.h"

#ifndef QT_NO_THREAD

QT_BEGIN_NAMESPACE

class QThreadPoolThread;
class QThreadPoolPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QThreadPool)
    friend class QThreadPoolThread;

public:
    QThreadPoolPrivate();

    bool tryStart(QRunnable *task);
    void enqueueTask(QRunnable *task, int priority = 0);
    int activeThreadCount() const;

    void tryToStartMoreThreads();
    bool tooManyThreadsActive() const;

    void startThread(QRunnable *runnable = 0);
    void reset();
    bool waitForDone(int msecs = -1);
    bool startFrontRunnable();
    void stealRunnable(QRunnable *);

    mutable QMutex mutex;
    QWaitCondition runnableReady;
    QSet<QThreadPoolThread *> allThreads;
    QQueue<QThreadPoolThread *> expiredThreads;
    QList<QPair<QRunnable *, int> > queue;
    QWaitCondition noActiveThreads;

    bool isExiting;
    int expiryTimeout;
    int maxThreadCount;
    int reservedThreads;
    int waitingThreads;
    int activeThreads;
};

QT_END_NAMESPACE

#endif // QT_NO_THREAD
#endif
