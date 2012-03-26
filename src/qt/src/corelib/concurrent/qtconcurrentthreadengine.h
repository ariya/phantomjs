/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QTCONCURRENT_THREADENGINE_H
#define QTCONCURRENT_THREADENGINE_H

#include <QtCore/qglobal.h>

#ifndef QT_NO_CONCURRENT

#include <QtCore/qthreadpool.h>
#include <QtCore/qfuture.h>
#include <QtCore/qdebug.h>
#include <QtCore/qtconcurrentexception.h>
#include <QtCore/qwaitcondition.h>
#include <QtCore/qatomic.h>
#include <QtCore/qsemaphore.h>

QT_BEGIN_HEADER
QT_BEGIN_NAMESPACE

QT_MODULE(Core)

#ifndef qdoc

namespace QtConcurrent {

// The ThreadEngineBarrier counts worker threads, and allows one
// thread to wait for all others to finish. Tested for its use in
// QtConcurrent, requires more testing for use as a general class.
class ThreadEngineBarrier
{
private:
    // The thread count is maintained as an integer in the count atomic
    // variable. The count can be either positive or negative - a negative
    // count signals that a thread is waiting on the barrier.

    // BC note: inlined code from Qt < 4.6 will expect to find the QMutex 
    // and QAtomicInt here. ### Qt 5: remove.
    QMutex mutex;
    QAtomicInt count;

    QSemaphore semaphore;
public:
    ThreadEngineBarrier();
    void acquire();
    int release();
    void wait();
    int currentCount();
    bool releaseUnlessLast();
};

enum ThreadFunctionResult { ThrottleThread, ThreadFinished };

// The ThreadEngine controls the threads used in the computation.
// Can be run in three modes: single threaded, multi-threaded blocking
// and multi-threaded asynchronous.
// The code for the single threaded mode is
class Q_CORE_EXPORT ThreadEngineBase: public QRunnable
{
public:
    // Public API:
    ThreadEngineBase();
    virtual ~ThreadEngineBase();
    void startSingleThreaded();
    void startBlocking();
    void startThread();
    bool isCanceled();
    void waitForResume();
    bool isProgressReportingEnabled();
    void setProgressValue(int progress);
    void setProgressRange(int minimum, int maximum);
    void acquireBarrierSemaphore();

protected: // The user overrides these:
    virtual void start() {}
    virtual void finish() {}
    virtual ThreadFunctionResult threadFunction() { return ThreadFinished; }
    virtual bool shouldStartThread() { return futureInterface ? !futureInterface->isPaused() : true; }
    virtual bool shouldThrottleThread() { return futureInterface ? futureInterface->isPaused() : false; }
private:
    bool startThreadInternal();
    void startThreads();
    void threadExit();
    bool threadThrottleExit();
    void run();
    virtual void asynchronousFinish() = 0;
#ifndef QT_NO_EXCEPTIONS
    void handleException(const QtConcurrent::Exception &exception);
#endif
protected:
    QFutureInterfaceBase *futureInterface;
    QThreadPool *threadPool;
    ThreadEngineBarrier barrier;
    QtConcurrent::internal::ExceptionStore exceptionStore;
};


template <typename T>
class ThreadEngine : public virtual ThreadEngineBase
{
public:
    typedef T ResultType;

    virtual T *result() { return 0; }

    QFutureInterface<T> *futureInterfaceTyped()
    {
        return static_cast<QFutureInterface<T> *>(futureInterface);
    }

    // Runs the user algorithm using a single thread.
    T *startSingleThreaded()
    {
        ThreadEngineBase::startSingleThreaded();
        return result();
    }

    // Runs the user algorithm using multiple threads.
    // This function blocks until the algorithm is finished,
    // and then returns the result.
    T *startBlocking()
    {
        ThreadEngineBase::startBlocking();
        return result();
    }

    // Runs the user algorithm using multiple threads.
    // Does not block, returns a future.
    QFuture<T> startAsynchronously()
    {
        futureInterface = new QFutureInterface<T>();

        // reportStart() must be called before starting threads, otherwise the
        // user algorithm might finish while reportStart() is running, which
        // is very bad.
        futureInterface->reportStarted();
        QFuture<T> future = QFuture<T>(futureInterfaceTyped());
        start();

        acquireBarrierSemaphore();
        threadPool->start(this);
        return future;
    }

    void asynchronousFinish()
    {
        finish();
        futureInterfaceTyped()->reportFinished(result());
        delete futureInterfaceTyped();
        delete this;
    }


    void reportResult(const T *_result, int index = -1)
    {
        if (futureInterface)
            futureInterfaceTyped()->reportResult(_result, index);
    }

    void reportResults(const QVector<T> &_result, int index = -1, int count = -1)
    {
        if (futureInterface)
            futureInterfaceTyped()->reportResults(_result, index, count);
    }
};

// The ThreadEngineStarter class ecapsulates the return type
// from the thread engine.
// Depending on how the it is used, it will run
// the engine in either blocking mode or asynchronous mode.
template <typename T>
class ThreadEngineStarterBase
{
public:
    ThreadEngineStarterBase(ThreadEngine<T> *_threadEngine)
    : threadEngine(_threadEngine) { }

    inline ThreadEngineStarterBase(const ThreadEngineStarterBase &other)
    : threadEngine(other.threadEngine) { }

    QFuture<T> startAsynchronously()
    {
        return threadEngine->startAsynchronously();
    }

    operator QFuture<T>()
    {
        return startAsynchronously();
    }

protected:
    ThreadEngine<T> *threadEngine;
};


// We need to factor out the code that dereferences the T pointer,
// with a specialization where T is void. (code that dereferences a void *
// won't compile)
template <typename T>
class ThreadEngineStarter : public ThreadEngineStarterBase<T>
{
    typedef ThreadEngineStarterBase<T> Base;
    typedef ThreadEngine<T> TypedThreadEngine;
public:
    ThreadEngineStarter(TypedThreadEngine *eng)
        : Base(eng) { }

    T startBlocking()
    {
        T t = *this->threadEngine->startBlocking();
        delete this->threadEngine;
        return t;
    }
};

// Full template specialization where T is void.
template <>
class ThreadEngineStarter<void> : public ThreadEngineStarterBase<void>
{
public:
    ThreadEngineStarter<void>(ThreadEngine<void> *_threadEngine)
    :ThreadEngineStarterBase<void>(_threadEngine) {}

    void startBlocking()
    {
        this->threadEngine->startBlocking();
        delete this->threadEngine;
    }
};

template <typename ThreadEngine>
inline ThreadEngineStarter<typename ThreadEngine::ResultType> startThreadEngine(ThreadEngine *threadEngine)
{
    return ThreadEngineStarter<typename ThreadEngine::ResultType>(threadEngine);
}

} // namespace QtConcurrent

#endif //qdoc

QT_END_NAMESPACE
QT_END_HEADER

#endif // QT_NO_CONCURRENT

#endif
