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
#include "RunLoop.h"

#include <QAbstractEventDispatcher>
#include <QCoreApplication>
#include <QMetaMethod>
#include <QMetaObject>
#include <QObject>
#include <QTimerEvent>

namespace WebCore {

class RunLoop::TimerObject : public QObject {
    Q_OBJECT
public:
    TimerObject(RunLoop* runLoop) : m_runLoop(runLoop)
    {
        int methodIndex = metaObject()->indexOfMethod("performWork()");
        m_method = metaObject()->method(methodIndex);
    }

    Q_SLOT void performWork() { m_runLoop->performWork(); }
    inline void wakeUp() { m_method.invoke(this, Qt::QueuedConnection); }

protected:
    virtual void timerEvent(QTimerEvent* event)
    {
        RunLoop::TimerBase::timerFired(m_runLoop, event->timerId());
    }

private:
    RunLoop* m_runLoop;
    QMetaMethod m_method;
};

static QEventLoop* currentEventLoop;

void RunLoop::run()
{
    static bool mainEventLoopIsRunning = false;
    if (!mainEventLoopIsRunning) {
        mainEventLoopIsRunning = true;
        QCoreApplication::exec();
        mainEventLoopIsRunning = false;
    } else {
        QEventLoop eventLoop;

        QEventLoop* previousEventLoop = currentEventLoop;
        currentEventLoop = &eventLoop;

        eventLoop.exec();

        currentEventLoop = previousEventLoop;
    }
}

void RunLoop::stop()
{
    if (currentEventLoop)
        currentEventLoop->exit();
    else
        QCoreApplication::exit();
}

RunLoop::RunLoop()
    : m_timerObject(new TimerObject(this))
{
}

RunLoop::~RunLoop()
{
    delete m_timerObject;
}

void RunLoop::wakeUp()
{
    m_timerObject->wakeUp();
}

// RunLoop::Timer

void RunLoop::TimerBase::timerFired(RunLoop* runLoop, int ID)
{
    TimerMap::iterator it = runLoop->m_activeTimers.find(ID);
    ASSERT(it != runLoop->m_activeTimers.end());
    TimerBase* timer = it->value;

    if (!timer->m_isRepeating) {
        // Stop the timer (calling stop would need another hash table lookup).
        runLoop->m_activeTimers.remove(it);
        runLoop->m_timerObject->killTimer(timer->m_ID);
        timer->m_ID = 0;
    }

    timer->fired();
}

RunLoop::TimerBase::TimerBase(RunLoop* runLoop)
    : m_runLoop(runLoop)
    , m_ID(0)
    , m_isRepeating(false)
{
}

RunLoop::TimerBase::~TimerBase()
{
    stop();
}

void RunLoop::TimerBase::start(double nextFireInterval, bool repeat)
{
    stop();
    int millis = static_cast<int>(nextFireInterval * 1000);
    m_isRepeating = repeat;
    m_ID = m_runLoop->m_timerObject->startTimer(millis);
    ASSERT(m_ID);
    m_runLoop->m_activeTimers.set(m_ID, this);
}

void RunLoop::TimerBase::stop()
{
    if (!m_ID)
        return;
    TimerMap::iterator it = m_runLoop->m_activeTimers.find(m_ID);
    if (it == m_runLoop->m_activeTimers.end())
        return;

    m_runLoop->m_activeTimers.remove(it);
    m_runLoop->m_timerObject->killTimer(m_ID);
    m_ID = 0;
}

bool RunLoop::TimerBase::isActive() const
{
    return m_ID;
}

#include "RunLoopQt.moc"

} // namespace WebCore
