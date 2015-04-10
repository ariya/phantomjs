/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef HandleSet_h
#define HandleSet_h

#include "Handle.h"
#include "HandleBlock.h"
#include <wtf/DoublyLinkedList.h>
#include <wtf/HashCountedSet.h>
#include <wtf/SentinelLinkedList.h>
#include <wtf/SinglyLinkedList.h>

namespace JSC {

class HandleBlock;
class HandleSet;
class HeapRootVisitor;
class VM;
class JSValue;
class SlotVisitor;

class HandleNode {
public:
    HandleNode(WTF::SentinelTag);
    HandleNode();
    
    HandleSlot slot();
    HandleSet* handleSet();

    void setPrev(HandleNode*);
    HandleNode* prev();

    void setNext(HandleNode*);
    HandleNode* next();

private:
    JSValue m_value;
    HandleNode* m_prev;
    HandleNode* m_next;
};

class HandleSet {
    friend class HandleBlock;
public:
    static HandleSet* heapFor(HandleSlot);

    HandleSet(VM*);
    ~HandleSet();

    VM* vm();

    HandleSlot allocate();
    void deallocate(HandleSlot);

    void visitStrongHandles(HeapRootVisitor&);

    JS_EXPORT_PRIVATE void writeBarrier(HandleSlot, const JSValue&);

    unsigned protectedGlobalObjectCount();

    template<typename Functor> void forEachStrongHandle(Functor&, const HashCountedSet<JSCell*>& skipSet);

private:
    typedef HandleNode Node;
    static HandleSlot toHandle(Node*);
    static Node* toNode(HandleSlot);

    JS_EXPORT_PRIVATE void grow();
    
#if ENABLE(GC_VALIDATION) || !ASSERT_DISABLED
    bool isLiveNode(Node*);
#endif

    VM* m_vm;
    DoublyLinkedList<HandleBlock> m_blockList;

    SentinelLinkedList<Node> m_strongList;
    SentinelLinkedList<Node> m_immediateList;
    SinglyLinkedList<Node> m_freeList;
    Node* m_nextToFinalize;
};

inline HandleSet* HandleSet::heapFor(HandleSlot handle)
{
    return toNode(handle)->handleSet();
}

inline VM* HandleSet::vm()
{
    return m_vm;
}

inline HandleSlot HandleSet::toHandle(HandleSet::Node* node)
{
    return reinterpret_cast<HandleSlot>(node);
}

inline HandleSet::Node* HandleSet::toNode(HandleSlot handle)
{
    return reinterpret_cast<HandleSet::Node*>(handle);
}

inline HandleSlot HandleSet::allocate()
{
    // Forbid assignment to handles during the finalization phase, since it would violate many GC invariants.
    // File a bug with stack trace if you hit this.
    RELEASE_ASSERT(!m_nextToFinalize);

    if (m_freeList.isEmpty())
        grow();

    HandleSet::Node* node = m_freeList.pop();
    new (NotNull, node) HandleSet::Node();
    m_immediateList.push(node);
    return toHandle(node);
}

inline void HandleSet::deallocate(HandleSlot handle)
{
    HandleSet::Node* node = toNode(handle);
    if (node == m_nextToFinalize) {
        ASSERT(m_nextToFinalize->next());
        m_nextToFinalize = m_nextToFinalize->next();
    }

    SentinelLinkedList<HandleSet::Node>::remove(node);
    m_freeList.push(node);
}

inline HandleNode::HandleNode()
    : m_prev(0)
    , m_next(0)
{
}

inline HandleNode::HandleNode(WTF::SentinelTag)
    : m_prev(0)
    , m_next(0)
{
}

inline HandleSlot HandleNode::slot()
{
    return &m_value;
}

inline HandleSet* HandleNode::handleSet()
{
    return HandleBlock::blockFor(this)->handleSet();
}

inline void HandleNode::setPrev(HandleNode* prev)
{
    m_prev = prev;
}

inline HandleNode* HandleNode::prev()
{
    return m_prev;
}

inline void HandleNode::setNext(HandleNode* next)
{
    m_next = next;
}

inline HandleNode* HandleNode::next()
{
    return m_next;
}

template<typename Functor> void HandleSet::forEachStrongHandle(Functor& functor, const HashCountedSet<JSCell*>& skipSet)
{
    HandleSet::Node* end = m_strongList.end();
    for (HandleSet::Node* node = m_strongList.begin(); node != end; node = node->next()) {
        JSValue value = *node->slot();
        if (!value || !value.isCell())
            continue;
        if (skipSet.contains(value.asCell()))
            continue;
        functor(value.asCell());
    }
}

}

#endif
