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

#include "config.h"
#include "HandleHeap.h"

#include "JSObject.h"

namespace JSC {

WeakHandleOwner::~WeakHandleOwner()
{
}

bool WeakHandleOwner::isReachableFromOpaqueRoots(Handle<Unknown>, void*, SlotVisitor&)
{
    return false;
}

void WeakHandleOwner::finalize(Handle<Unknown>, void*)
{
}

HandleHeap::HandleHeap(JSGlobalData* globalData)
    : m_globalData(globalData)
    , m_nextToFinalize(0)
{
    grow();
}

void HandleHeap::grow()
{
    Node* block = m_blockStack.grow();
    for (int i = m_blockStack.blockLength - 1; i >= 0; --i) {
        Node* node = &block[i];
        new (node) Node(this);
        m_freeList.push(node);
    }
}

void HandleHeap::markStrongHandles(HeapRootVisitor& heapRootMarker)
{
    Node* end = m_strongList.end();
    for (Node* node = m_strongList.begin(); node != end; node = node->next())
        heapRootMarker.mark(node->slot());
}

void HandleHeap::markWeakHandles(HeapRootVisitor& heapRootVisitor)
{
    SlotVisitor& visitor = heapRootVisitor.visitor();

    Node* end = m_weakList.end();
    for (Node* node = m_weakList.begin(); node != end; node = node->next()) {
        ASSERT(isValidWeakNode(node));
        JSCell* cell = node->slot()->asCell();
        if (Heap::isMarked(cell))
            continue;

        WeakHandleOwner* weakOwner = node->weakOwner();
        if (!weakOwner)
            continue;

        if (!weakOwner->isReachableFromOpaqueRoots(Handle<Unknown>::wrapSlot(node->slot()), node->weakOwnerContext(), visitor))
            continue;

        heapRootVisitor.mark(node->slot());
    }
}

void HandleHeap::finalizeWeakHandles()
{
    Node* end = m_weakList.end();
    for (Node* node = m_weakList.begin(); node != end; node = m_nextToFinalize) {
        m_nextToFinalize = node->next();

        ASSERT(isValidWeakNode(node));
        JSCell* cell = node->slot()->asCell();
        if (Heap::isMarked(cell))
            continue;

        if (WeakHandleOwner* weakOwner = node->weakOwner()) {
            weakOwner->finalize(Handle<Unknown>::wrapSlot(node->slot()), node->weakOwnerContext());
            if (m_nextToFinalize != node->next()) // Owner deallocated node.
                continue;
        }

        *node->slot() = JSValue();
        SentinelLinkedList<Node>::remove(node);
        m_immediateList.push(node);
    }
    
    m_nextToFinalize = 0;
}

void HandleHeap::writeBarrier(HandleSlot slot, const JSValue& value)
{
    ASSERT(!m_nextToFinalize); // Forbid assignment to handles during the finalization phase, since it would violate many GC invariants.

    if (!value == !*slot && slot->isCell() == value.isCell())
        return;

    Node* node = toNode(slot);
    SentinelLinkedList<Node>::remove(node);
    if (!value || !value.isCell()) {
        m_immediateList.push(node);
        return;
    }

    if (node->isWeak()) {
        m_weakList.push(node);
        return;
    }

    m_strongList.push(node);
}

unsigned HandleHeap::protectedGlobalObjectCount()
{
    unsigned count = 0;
    Node* end = m_strongList.end();
    for (Node* node = m_strongList.begin(); node != end; node = node->next()) {
        JSValue value = *node->slot();
        if (value.isObject() && asObject(value.asCell())->isGlobalObject())
            count++;
    }
    return count;
}

#if !ASSERT_DISABLED
bool HandleHeap::isValidWeakNode(Node* node)
{
    if (!node->isWeak())
        return false;

    JSValue value = *node->slot();
    if (!value || !value.isCell())
        return false;

    JSCell* cell = value.asCell();
    if (!cell || !cell->structure())
        return false;

#if ENABLE(JSC_ZOMBIES)
    if (cell->isZombie())
        return false;
#endif

    return true;
}
#endif

} // namespace JSC
