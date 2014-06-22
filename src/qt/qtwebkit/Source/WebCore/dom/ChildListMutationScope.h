/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ChildListMutationScope_h
#define ChildListMutationScope_h

#include "Document.h"
#include "MutationObserver.h"
#include "Node.h"
#include <wtf/HashMap.h>
#include <wtf/Noncopyable.h>
#include <wtf/OwnPtr.h>
#include <wtf/RefCounted.h>

namespace WebCore {

class MutationObserverInterestGroup;

// ChildListMutationAccumulator is not meant to be used directly; ChildListMutationScope is the public interface.
class ChildListMutationAccumulator : public RefCounted<ChildListMutationAccumulator> {
public:
    static PassRefPtr<ChildListMutationAccumulator> getOrCreate(Node*);
    ~ChildListMutationAccumulator();

    void childAdded(PassRefPtr<Node>);
    void willRemoveChild(PassRefPtr<Node>);

    bool hasObservers() const { return m_observers; }

private:
    ChildListMutationAccumulator(PassRefPtr<Node>, PassOwnPtr<MutationObserverInterestGroup>);

    void enqueueMutationRecord();
    bool isEmpty();
    bool isAddedNodeInOrder(Node*);
    bool isRemovedNodeInOrder(Node*);

    RefPtr<Node> m_target;

    Vector<RefPtr<Node> > m_removedNodes;
    Vector<RefPtr<Node> > m_addedNodes;
    RefPtr<Node> m_previousSibling;
    RefPtr<Node> m_nextSibling;
    Node* m_lastAdded;

    OwnPtr<MutationObserverInterestGroup> m_observers;
};

class ChildListMutationScope {
    WTF_MAKE_NONCOPYABLE(ChildListMutationScope);
public:
    explicit ChildListMutationScope(Node* target)
    {
        if (target->document()->hasMutationObserversOfType(MutationObserver::ChildList))
            m_accumulator = ChildListMutationAccumulator::getOrCreate(target);
    }

    void childAdded(Node* child)
    {
        if (m_accumulator && m_accumulator->hasObservers())
            m_accumulator->childAdded(child);
    }

    void willRemoveChild(Node* child)
    {
        if (m_accumulator && m_accumulator->hasObservers())
            m_accumulator->willRemoveChild(child);
    }

private:
    RefPtr<ChildListMutationAccumulator> m_accumulator;
};

} // namespace WebCore

#endif // ChildListMutationScope_h
