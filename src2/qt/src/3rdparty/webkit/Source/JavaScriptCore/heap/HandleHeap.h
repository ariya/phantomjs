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

#ifndef HandleHeap_h
#define HandleHeap_h

#include "BlockStack.h"
#include "Handle.h"
#include "SentinelLinkedList.h"
#include "SinglyLinkedList.h"

namespace JSC {

class HandleHeap;
class HeapRootVisitor;
class JSGlobalData;
class JSValue;
class MarkStack;
class TypeCounter;
typedef MarkStack SlotVisitor;

class WeakHandleOwner {
public:
    virtual ~WeakHandleOwner();
    virtual bool isReachableFromOpaqueRoots(Handle<Unknown>, void* context, SlotVisitor&);
    virtual void finalize(Handle<Unknown>, void* context);
};

class HandleHeap {
public:
    static HandleHeap* heapFor(HandleSlot);

    HandleHeap(JSGlobalData*);
    
    JSGlobalData* globalData();

    HandleSlot allocate();
    void deallocate(HandleSlot);

    void makeWeak(HandleSlot, WeakHandleOwner* = 0, void* context = 0);
    HandleSlot copyWeak(HandleSlot);

    void markStrongHandles(HeapRootVisitor&);
    void markWeakHandles(HeapRootVisitor&);
    void finalizeWeakHandles();

    void writeBarrier(HandleSlot, const JSValue&);

#if !ASSERT_DISABLED
    bool hasWeakOwner(HandleSlot, WeakHandleOwner*);
#endif

    unsigned protectedGlobalObjectCount();
    void protectedObjectTypeCounts(TypeCounter&);

private:
    class Node {
    public:
        Node(WTF::SentinelTag);
        Node(HandleHeap*);
        
        HandleSlot slot();
        HandleHeap* handleHeap();

        void makeWeak(WeakHandleOwner*, void* context);
        bool isWeak();
        
        WeakHandleOwner* weakOwner();
        void* weakOwnerContext();

        void setPrev(Node*);
        Node* prev();

        void setNext(Node*);
        Node* next();

    private:
        WeakHandleOwner* emptyWeakOwner();

        JSValue m_value;
        HandleHeap* m_handleHeap;
        WeakHandleOwner* m_weakOwner;
        void* m_weakOwnerContext;
        Node* m_prev;
        Node* m_next;
    };

    static HandleSlot toHandle(Node*);
    static Node* toNode(HandleSlot);

    void grow();
    
#if !ASSERT_DISABLED
    bool isValidWeakNode(Node*);
#endif

    JSGlobalData* m_globalData;
    BlockStack<Node> m_blockStack;

    SentinelLinkedList<Node> m_strongList;
    SentinelLinkedList<Node> m_weakList;
    SentinelLinkedList<Node> m_immediateList;
    SinglyLinkedList<Node> m_freeList;
    Node* m_nextToFinalize;
};

inline HandleHeap* HandleHeap::heapFor(HandleSlot handle)
{
    return toNode(handle)->handleHeap();
}

inline JSGlobalData* HandleHeap::globalData()
{
    return m_globalData;
}

inline HandleSlot HandleHeap::toHandle(Node* node)
{
    return reinterpret_cast<HandleSlot>(node);
}

inline HandleHeap::Node* HandleHeap::toNode(HandleSlot handle)
{
    return reinterpret_cast<Node*>(handle);
}

inline HandleSlot HandleHeap::allocate()
{
    if (m_freeList.isEmpty())
        grow();

    Node* node = m_freeList.pop();
    new (node) Node(this);
    m_immediateList.push(node);
    return toHandle(node);
}

inline void HandleHeap::deallocate(HandleSlot handle)
{
    Node* node = toNode(handle);
    if (node == m_nextToFinalize) {
        m_nextToFinalize = node->next();
        ASSERT(m_nextToFinalize->next());
    }

    SentinelLinkedList<Node>::remove(node);
    m_freeList.push(node);
}

inline HandleSlot HandleHeap::copyWeak(HandleSlot other)
{
    Node* node = toNode(allocate());
    node->makeWeak(toNode(other)->weakOwner(), toNode(other)->weakOwnerContext());
    writeBarrier(node->slot(), *other);
    *node->slot() = *other;
    return toHandle(node);
}

inline void HandleHeap::makeWeak(HandleSlot handle, WeakHandleOwner* weakOwner, void* context)
{
    Node* node = toNode(handle);
    node->makeWeak(weakOwner, context);

    SentinelLinkedList<Node>::remove(node);
    if (!*handle || !handle->isCell()) {
        m_immediateList.push(node);
        return;
    }

    m_weakList.push(node);
}

#if !ASSERT_DISABLED
inline bool HandleHeap::hasWeakOwner(HandleSlot handle, WeakHandleOwner* weakOwner)
{
    return toNode(handle)->weakOwner() == weakOwner;
}
#endif

inline HandleHeap::Node::Node(HandleHeap* handleHeap)
    : m_handleHeap(handleHeap)
    , m_weakOwner(0)
    , m_weakOwnerContext(0)
{
}

inline HandleHeap::Node::Node(WTF::SentinelTag)
    : m_handleHeap(0)
    , m_weakOwner(0)
    , m_weakOwnerContext(0)
{
}

inline HandleSlot HandleHeap::Node::slot()
{
    return &m_value;
}

inline HandleHeap* HandleHeap::Node::handleHeap()
{
    return m_handleHeap;
}

inline void HandleHeap::Node::makeWeak(WeakHandleOwner* weakOwner, void* context)
{
    m_weakOwner = weakOwner ? weakOwner : emptyWeakOwner();
    m_weakOwnerContext = context;
}

inline bool HandleHeap::Node::isWeak()
{
    return m_weakOwner; // True for emptyWeakOwner().
}

inline WeakHandleOwner* HandleHeap::Node::weakOwner()
{
    return m_weakOwner == emptyWeakOwner() ? 0 : m_weakOwner; // 0 for emptyWeakOwner().
}

inline void* HandleHeap::Node::weakOwnerContext()
{
    ASSERT(weakOwner());
    return m_weakOwnerContext;
}

inline void HandleHeap::Node::setPrev(Node* prev)
{
    m_prev = prev;
}

inline HandleHeap::Node* HandleHeap::Node::prev()
{
    return m_prev;
}

inline void HandleHeap::Node::setNext(Node* next)
{
    m_next = next;
}

inline HandleHeap::Node* HandleHeap::Node::next()
{
    return m_next;
}

// Sentinel to indicate that a node is weak, but its owner has no meaningful
// callbacks. This allows us to optimize by skipping such nodes.
inline WeakHandleOwner* HandleHeap::Node::emptyWeakOwner()
{
    return reinterpret_cast<WeakHandleOwner*>(-1);
}

}

#endif
