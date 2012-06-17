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

#ifndef QTHREAD_H
#define QTHREAD_H

#include <QtCore/qobject.h>

#include <limits.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Core)

class QThreadData;
class QThreadPrivate;

#ifndef QT_NO_THREAD
class Q_CORE_EXPORT QThread : public QObject
{
public:
    static Qt::HANDLE currentThreadId();
    static QThread *currentThread();
    static int idealThreadCount();
    static void yieldCurrentThread();

    explicit QThread(QObject *parent = 0);
    ~QThread();

    enum Priority {
        IdlePriority,

        LowestPriority,
        LowPriority,
        NormalPriority,
        HighPriority,
        HighestPriority,

        TimeCriticalPriority,

        InheritPriority
    };

    void setPriority(Priority priority);
    Priority priority() const;

    bool isFinished() const;
    bool isRunning() const;

    void setStackSize(uint stackSize);
    uint stackSize() const;

    void exit(int retcode = 0);

public Q_SLOTS:
    void start(Priority = InheritPriority);
    void terminate();
    void quit();

public:
    // default argument causes thread to block indefinately
    bool wait(unsigned long time = ULONG_MAX);

Q_SIGNALS:
    void started();
    void finished();
    void terminated();

protected:
    virtual void run();
    int exec();

    static void setTerminationEnabled(bool enabled = true);

    static void sleep(unsigned long);
    static void msleep(unsigned long);
    static void usleep(unsigned long);

#ifdef QT3_SUPPORT
public:
    inline QT3_SUPPORT bool finished() const { return isFinished(); }
    inline QT3_SUPPORT bool running() const { return isRunning(); }
#endif

protected:
    QThread(QThreadPrivate &dd, QObject *parent = 0);

private:
    Q_OBJECT
    Q_DECLARE_PRIVATE(QThread)

    static void initialize();
    static void cleanup();

    friend class QCoreApplication;
    friend class QThreadData;
};

#else // QT_NO_THREAD

class Q_CORE_EXPORT QThread : public QObject
{
public:
    static Qt::HANDLE currentThreadId() { return Qt::HANDLE(currentThread()); }
    static QThread* currentThread();
    
protected:
    QThread(QThreadPrivate &dd, QObject *parent = 0);

private:
    explicit QThread(QObject *parent = 0);
    static QThread *instance;

    friend class QCoreApplication;
    friend class QThreadData;
    friend class QAdoptedThread;
    Q_DECLARE_PRIVATE(QThread)
};

#endif // QT_NO_THREAD

QT_END_NAMESPACE

QT_END_HEADER

#endif // QTHREAD_H
