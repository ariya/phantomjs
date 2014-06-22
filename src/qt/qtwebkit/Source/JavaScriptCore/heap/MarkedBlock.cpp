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
#include "MarkedBlock.h"

#include "IncrementalSweeper.h"
#include "JSCell.h"
#include "JSDestructibleObject.h"
#include "Operations.h"

namespace JSC {

MarkedBlock* MarkedBlock::create(DeadBlock* block, MarkedAllocator* allocator, size_t cellSize, DestructorType destructorType)
{
    ASSERT(reinterpret_cast<size_t>(block) == (reinterpret_cast<size_t>(block) & blockMask));
    Region* region = block->region();
    return new (NotNull, block) MarkedBlock(region, allocator, cellSize, destructorType);
}

MarkedBlock::MarkedBlock(Region* region, MarkedAllocator* allocator, size_t cellSize, DestructorType destructorType)
    : HeapBlock<MarkedBlock>(region)
    , m_atomsPerCell((cellSize + atomSize - 1) / atomSize)
    , m_endAtom((allocator->cellSize() ? atomsPerBlock : region->blockSize() / atomSize) - m_atomsPerCell + 1)
    , m_destructorType(destructorType)
    , m_allocator(allocator)
    , m_state(New) // All cells start out unmarked.
    , m_weakSet(allocator->heap()->vm())
{
    ASSERT(allocator);
    HEAP_LOG_BLOCK_STATE_TRANSITION(this);
}

inline void MarkedBlock::callDestructor(JSCell* cell)
{
    // A previous eager sweep may already have run cell's destructor.
    if (cell->isZapped())
        return;

#if ENABLE(SIMPLE_HEAP_PROFILING)
    m_heap->m_destroyedTypeCounts.countVPtr(vptr);
#endif

    cell->methodTableForDestruction()->destroy(cell);
    cell->zap();
}

template<MarkedBlock::BlockState blockState, MarkedBlock::SweepMode sweepMode, MarkedBlock::DestructorType dtorType>
MarkedBlock::FreeList MarkedBlock::specializedSweep()
{
    ASSERT(blockState != Allocated && blockState != FreeListed);
    ASSERT(!(dtorType == MarkedBlock::None && sweepMode == SweepOnly));

    // This produces a free list that is ordered in reverse through the block.
    // This is fine, since the allocation code makes no assumptions about the
    // order of the free list.
    FreeCell* head = 0;
    size_t count = 0;
    for (size_t i = firstAtom(); i < m_endAtom; i += m_atomsPerCell) {
        if (blockState == Marked && (m_marks.get(i) || (m_newlyAllocated && m_newlyAllocated->get(i))))
            continue;

        JSCell* cell = reinterpret_cast_ptr<JSCell*>(&atoms()[i]);

        if (dtorType != MarkedBlock::None && blockState != New)
            callDestructor(cell);

        if (sweepMode == SweepToFreeList) {
            FreeCell* freeCell = reinterpret_cast<FreeCell*>(cell);
            freeCell->next = head;
            head = freeCell;
            ++count;
        }
    }

    // We only want to discard the newlyAllocated bits if we're creating a FreeList,
    // otherwise we would lose information on what's currently alive.
    if (sweepMode == SweepToFreeList && m_newlyAllocated)
        m_newlyAllocated.clear();

    m_state = ((sweepMode == SweepToFreeList) ? FreeListed : Marked);
    return FreeList(head, count * cellSize());
}

MarkedBlock::FreeList MarkedBlock::sweep(SweepMode sweepMode)
{
    HEAP_LOG_BLOCK_STATE_TRANSITION(this);

    m_weakSet.sweep();

    if (sweepMode == SweepOnly && m_destructorType == MarkedBlock::None)
        return FreeList();

    if (m_destructorType == MarkedBlock::ImmortalStructure)
        return sweepHelper<MarkedBlock::ImmortalStructure>(sweepMode);
    if (m_destructorType == MarkedBlock::Normal)
        return sweepHelper<MarkedBlock::Normal>(sweepMode);
    return sweepHelper<MarkedBlock::None>(sweepMode);
}

template<MarkedBlock::DestructorType dtorType>
MarkedBlock::FreeList MarkedBlock::sweepHelper(SweepMode sweepMode)
{
    switch (m_state) {
    case New:
        ASSERT(sweepMode == SweepToFreeList);
        return specializedSweep<New, SweepToFreeList, dtorType>();
    case FreeListed:
        // Happens when a block transitions to fully allocated.
        ASSERT(sweepMode == SweepToFreeList);
        return FreeList();
    case Allocated:
        RELEASE_ASSERT_NOT_REACHED();
        return FreeList();
    case Marked:
        return sweepMode == SweepToFreeList
            ? specializedSweep<Marked, SweepToFreeList, dtorType>()
            : specializedSweep<Marked, SweepOnly, dtorType>();
    }

    RELEASE_ASSERT_NOT_REACHED();
    return FreeList();
}

class SetNewlyAllocatedFunctor : public MarkedBlock::VoidFunctor {
public:
    SetNewlyAllocatedFunctor(MarkedBlock* block)
        : m_block(block)
    {
    }

    void operator()(JSCell* cell)
    {
        ASSERT(MarkedBlock::blockFor(cell) == m_block);
        m_block->setNewlyAllocated(cell);
    }

private:
    MarkedBlock* m_block;
};

void MarkedBlock::canonicalizeCellLivenessData(const FreeList& freeList)
{
    HEAP_LOG_BLOCK_STATE_TRANSITION(this);
    FreeCell* head = freeList.head;

    if (m_state == Marked) {
        // If the block is in the Marked state then we know that:
        // 1) It was not used for allocation during the previous allocation cycle.
        // 2) It may have dead objects, and we only know them to be dead by the
        //    fact that their mark bits are unset.
        // Hence if the block is Marked we need to leave it Marked.
        
        ASSERT(!head);
        return;
    }
   
    ASSERT(m_state == FreeListed);
    
    // Roll back to a coherent state for Heap introspection. Cells newly
    // allocated from our free list are not currently marked, so we need another
    // way to tell what's live vs dead. 
    
    ASSERT(!m_newlyAllocated);
    m_newlyAllocated = adoptPtr(new WTF::Bitmap<atomsPerBlock>());

    SetNewlyAllocatedFunctor functor(this);
    forEachCell(functor);

    FreeCell* next;
    for (FreeCell* current = head; current; current = next) {
        next = current->next;
        reinterpret_cast<JSCell*>(current)->zap();
        clearNewlyAllocated(current);
    }
    
    m_state = Marked;
}

} // namespace JSC
