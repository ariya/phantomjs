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

#ifndef EventQueue_h
#define EventQueue_h

#include <wtf/HashSet.h>
#include <wtf/ListHashSet.h>
#include <wtf/OwnPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class Event;
class EventQueueTimer;
class Node;
class ScriptExecutionContext;

class EventQueue : public RefCounted<EventQueue> {
public:
    enum ScrollEventTargetType {
        ScrollEventDocumentTarget,
        ScrollEventElementTarget
    };

    static PassRefPtr<EventQueue> create(ScriptExecutionContext*);
    ~EventQueue();

    void enqueueEvent(PassRefPtr<Event>);
    void enqueueOrDispatchScrollEvent(PassRefPtr<Node>, ScrollEventTargetType);
    bool cancelEvent(Event*);
    void cancelQueuedEvents();

private:
    explicit EventQueue(ScriptExecutionContext*);

    void pendingEventTimerFired();
    void dispatchEvent(PassRefPtr<Event>);

    OwnPtr<EventQueueTimer> m_pendingEventTimer;
    ListHashSet<RefPtr<Event> > m_queuedEvents;
    HashSet<Node*> m_nodesWithQueuedScrollEvents;
    
    friend class EventQueueTimer;    
};

}

#endif // EventQueue_h
