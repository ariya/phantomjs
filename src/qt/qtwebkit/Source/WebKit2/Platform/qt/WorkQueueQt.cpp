/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WorkQueue.h"

#include <QLocalSocket>
#include <QObject>
#include <QThread>
#include <QProcess>
#include <wtf/Threading.h>

class WorkQueue::WorkItemQt : public QObject {
    Q_OBJECT
public:
    WorkItemQt(WorkQueue* workQueue, const Function<void()>& function)
        : m_queue(workQueue)
        , m_source(0)
        , m_signal(0)
        , m_function(function)
    {
    }

    WorkItemQt(WorkQueue* workQueue, QObject* source, const char* signal, const Function<void()>& function)
        : m_queue(workQueue)
        , m_source(source)
        , m_signal(signal)
        , m_function(function)
    {
        connect(m_source, m_signal, SLOT(execute()), Qt::QueuedConnection);
    }

    ~WorkItemQt()
    {
        m_queue->deref();
    }

    Q_SLOT void execute() 
    { 
        m_function();
    }

    Q_SLOT void executeAndDelete()
    {
        execute();
        delete this;
    }

    virtual void timerEvent(QTimerEvent*)
    {
        executeAndDelete();
    }

    WorkQueue* m_queue;
    QObject* m_source;
    const char* m_signal;
    Function<void()> m_function;
};

QSocketNotifier* WorkQueue::registerSocketEventHandler(int socketDescriptor, QSocketNotifier::Type type, const Function<void()>& function)
{
    ASSERT(m_workThread);

    QSocketNotifier* notifier = new QSocketNotifier(socketDescriptor, type, 0);
    notifier->setEnabled(false);
    notifier->moveToThread(m_workThread);
    WorkQueue::WorkItemQt* itemQt = new WorkQueue::WorkItemQt(this, notifier, SIGNAL(activated(int)), function);
    itemQt->moveToThread(m_workThread);
    QMetaObject::invokeMethod(notifier, "setEnabled", Q_ARG(bool, true));
    return notifier;
}

void WorkQueue::platformInitialize(const char*)
{
    m_workThread = new QThread();
    m_workThread->start();
}

void WorkQueue::platformInvalidate()
{
    m_workThread->exit();
    m_workThread->wait();
    delete m_workThread;
}

void WorkQueue::dispatch(const Function<void()>& function)
{
    ref();
    WorkQueue::WorkItemQt* itemQt = new WorkQueue::WorkItemQt(this, function);
    itemQt->moveToThread(m_workThread);
    QMetaObject::invokeMethod(itemQt, "executeAndDelete", Qt::QueuedConnection);
}

void WorkQueue::dispatchAfterDelay(const Function<void()>& function, double delayInSecond)
{
    ref();
    WorkQueue::WorkItemQt* itemQt = new WorkQueue::WorkItemQt(this, function);
    itemQt->startTimer(static_cast<int>(delayInSecond * 1000));
    itemQt->moveToThread(m_workThread);
}

void WorkQueue::dispatchOnTermination(WebKit::PlatformProcessIdentifier process, const Function<void()>& function)
{
    WorkQueue::WorkItemQt* itemQt = new WorkQueue::WorkItemQt(this, process, SIGNAL(finished(int, QProcess::ExitStatus)), function);
    itemQt->moveToThread(m_workThread);
}

#include "WorkQueueQt.moc"
