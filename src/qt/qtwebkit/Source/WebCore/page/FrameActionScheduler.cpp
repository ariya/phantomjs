/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "FrameActionScheduler.h"

#include "Event.h"
#include "ExceptionCodePlaceholder.h"
#include "Node.h"
#include <wtf/Vector.h>

namespace WebCore {

class EventFrameAction : public FrameAction {
public:
    EventFrameAction(PassRefPtr<Event> event, PassRefPtr<Node> target)
        : m_event(event)
        , m_eventTarget(target)
    {
    }

    virtual void fire()
    {
        // Only dispatch events to nodes that are in the document
        if (m_eventTarget->inDocument())
            m_eventTarget->dispatchEvent(m_event, IGNORE_EXCEPTION);
    }

private:
    RefPtr<Event> m_event;
    RefPtr<Node> m_eventTarget;
};

FrameActionScheduler::FrameActionScheduler()
    : m_enqueueActions(0)
{
}

FrameActionScheduler::~FrameActionScheduler()
{
    clear();
}

bool FrameActionScheduler::isEmpty() const
{
    return m_scheduledActions.isEmpty();
}

void FrameActionScheduler::clear()
{
    m_scheduledActions.clear();
    m_enqueueActions = 0;
}

void FrameActionScheduler::pause()
{
    ASSERT(isEmpty() || m_enqueueActions);
    m_enqueueActions++;
}

void FrameActionScheduler::resume()
{
    m_enqueueActions--;
    if (!m_enqueueActions)
        dispatch();
    ASSERT(isEmpty() || m_enqueueActions);
}

void FrameActionScheduler::dispatch()
{
    Vector< OwnPtr<FrameAction> > snapshot;
    m_scheduledActions.swap(snapshot);
    
    for (Vector< OwnPtr<FrameAction> >::iterator i = snapshot.begin(); i != snapshot.end(); ++i)
        (*i)->fire();
}

void FrameActionScheduler::scheduleAction(PassOwnPtr<FrameAction> action)
{
    m_scheduledActions.append(action);
}

void FrameActionScheduler::scheduleEvent(PassRefPtr<Event> event, PassRefPtr<Node> eventTarget)
{
    scheduleAction(adoptPtr(new EventFrameAction(event, eventTarget)));
}


} // namespace WebCore
