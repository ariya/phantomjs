/*
 * Copyright (C) 2010 Google Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 *
 */

#include "config.h"
#include "WorkerEventQueue.h"

#include "DOMWindow.h"
#include "Document.h"
#include "Event.h"
#include "EventNames.h"
#include "ScriptExecutionContext.h"

namespace WebCore {

PassOwnPtr<WorkerEventQueue> WorkerEventQueue::create(ScriptExecutionContext* context)
{
    return adoptPtr(new WorkerEventQueue(context));
}

WorkerEventQueue::WorkerEventQueue(ScriptExecutionContext* context)
    : m_scriptExecutionContext(context)
    , m_isClosed(false)
{
}

WorkerEventQueue::~WorkerEventQueue()
{
    close();
}

class WorkerEventQueue::EventDispatcherTask : public ScriptExecutionContext::Task {
public:
    static PassOwnPtr<EventDispatcherTask> create(PassRefPtr<Event> event, WorkerEventQueue* eventQueue)
    {
        return adoptPtr(new EventDispatcherTask(event, eventQueue));
    }

    virtual ~EventDispatcherTask()
    {
        if (m_event)
            m_eventQueue->removeEvent(m_event.get());
    }

    void dispatchEvent(ScriptExecutionContext*, PassRefPtr<Event> event)
    {
        event->target()->dispatchEvent(event);
    }

    virtual void performTask(ScriptExecutionContext* context)
    {
        if (m_isCancelled)
            return;
        m_eventQueue->removeEvent(m_event.get());
        dispatchEvent(context, m_event);
        m_event.clear();
    }

    void cancel()
    {
        m_isCancelled = true;
        m_event.clear();
    }

private:
    EventDispatcherTask(PassRefPtr<Event> event, WorkerEventQueue* eventQueue)
        : m_event(event)
        , m_eventQueue(eventQueue)
        , m_isCancelled(false)
    {
    }

    RefPtr<Event> m_event;
    WorkerEventQueue* m_eventQueue;
    bool m_isCancelled;
};

void WorkerEventQueue::removeEvent(Event* event)
{
    m_eventTaskMap.remove(event);
}

bool WorkerEventQueue::enqueueEvent(PassRefPtr<Event> prpEvent)
{
    if (m_isClosed)
        return false;
    RefPtr<Event> event = prpEvent;
    OwnPtr<EventDispatcherTask> task = EventDispatcherTask::create(event, this);
    m_eventTaskMap.add(event.release(), task.get());
    m_scriptExecutionContext->postTask(task.release());
    return true;
}

bool WorkerEventQueue::cancelEvent(Event* event)
{
    EventDispatcherTask* task = m_eventTaskMap.get(event);
    if (!task)
        return false;
    task->cancel();
    removeEvent(event);
    return true;
}

void WorkerEventQueue::close()
{
    m_isClosed = true;
    for (EventTaskMap::iterator it = m_eventTaskMap.begin(); it != m_eventTaskMap.end(); ++it) {
        EventDispatcherTask* task = it->value;
        task->cancel();
    }
    m_eventTaskMap.clear();
}

}
