/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#ifndef WeakBlock_h
#define WeakBlock_h

#include "HeapBlock.h"
#include "WeakHandleOwner.h"
#include "WeakImpl.h"
#include <wtf/DoublyLinkedList.h>
#include <wtf/StdLibExtras.h>

namespace JSC {

class DeadBlock;
class HeapRootVisitor;
class JSValue;
class WeakHandleOwner;

class WeakBlock : public HeapBlock<WeakBlock> {
public:
    friend class WTF::DoublyLinkedListNode<WeakBlock>;
    static const size_t blockSize = 4 * KB; // 5% of MarkedBlock size

    struct FreeCell {
        FreeCell* next;
    };

    struct SweepResult {
        SweepResult();
        bool isNull() const;

        bool blockIsFree;
        FreeCell* freeList;
    };

    static WeakBlock* create(DeadBlock*);

    static WeakImpl* asWeakImpl(FreeCell*);

    bool isEmpty();

    void sweep();
    SweepResult takeSweepResult();

    void visit(HeapRootVisitor&);
    void reap();

    void lastChanceToFinalize();

private:
    static FreeCell* asFreeCell(WeakImpl*);

    WeakBlock(Region*);
    WeakImpl* firstWeakImpl();
    void finalize(WeakImpl*);
    WeakImpl* weakImpls();
    size_t weakImplCount();
    void addToFreeList(FreeCell**, WeakImpl*);

    SweepResult m_sweepResult;
};

inline WeakBlock::SweepResult::SweepResult()
    : blockIsFree(true)
    , freeList(0)
{
    ASSERT(isNull());
}

inline bool WeakBlock::SweepResult::isNull() const
{
    return blockIsFree && !freeList; // This state is impossible, so we can use it to mean null.
}

inline WeakImpl* WeakBlock::asWeakImpl(FreeCell* freeCell)
{
    return reinterpret_cast_ptr<WeakImpl*>(freeCell);
}

inline WeakBlock::SweepResult WeakBlock::takeSweepResult()
{
    SweepResult tmp;
    std::swap(tmp, m_sweepResult);
    ASSERT(m_sweepResult.isNull());
    return tmp;
}

inline WeakBlock::FreeCell* WeakBlock::asFreeCell(WeakImpl* weakImpl)
{
    return reinterpret_cast_ptr<FreeCell*>(weakImpl);
}

inline WeakImpl* WeakBlock::weakImpls()
{
    return reinterpret_cast_ptr<WeakImpl*>(this) + ((sizeof(WeakBlock) + sizeof(WeakImpl) - 1) / sizeof(WeakImpl));
}

inline size_t WeakBlock::weakImplCount()
{
    return (blockSize / sizeof(WeakImpl)) - ((sizeof(WeakBlock) + sizeof(WeakImpl) - 1) / sizeof(WeakImpl));
}

inline void WeakBlock::addToFreeList(FreeCell** freeList, WeakImpl* weakImpl)
{
    ASSERT(weakImpl->state() == WeakImpl::Deallocated);
    FreeCell* freeCell = asFreeCell(weakImpl);
    ASSERT(!*freeList || ((char*)*freeList > (char*)this && (char*)*freeList < (char*)this + blockSize));
    ASSERT((char*)freeCell > (char*)this && (char*)freeCell < (char*)this + blockSize);
    freeCell->next = *freeList;
    *freeList = freeCell;
}

inline bool WeakBlock::isEmpty()
{
    return !m_sweepResult.isNull() && m_sweepResult.blockIsFree;
}

} // namespace JSC

#endif // WeakBlock_h
