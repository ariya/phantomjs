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

#include "config.h"
#include "WeakSet.h"

#include "Heap.h"
#include "VM.h"

namespace JSC {

WeakSet::~WeakSet()
{
    WeakBlock* next = 0;
    for (WeakBlock* block = m_blocks.head(); block; block = next) {
        next = block->next();
        heap()->blockAllocator().deallocate(WeakBlock::destroy(block));
    }
    m_blocks.clear();
}

void WeakSet::sweep()
{
    for (WeakBlock* block = m_blocks.head(); block; block = block->next())
        block->sweep();

    resetAllocator();
}

WeakBlock::FreeCell* WeakSet::findAllocator()
{
    if (WeakBlock::FreeCell* allocator = tryFindAllocator())
        return allocator;

    return addAllocator();
}

WeakBlock::FreeCell* WeakSet::tryFindAllocator()
{
    while (m_nextAllocator) {
        WeakBlock* block = m_nextAllocator;
        m_nextAllocator = m_nextAllocator->next();

        WeakBlock::SweepResult sweepResult = block->takeSweepResult();
        if (sweepResult.freeList)
            return sweepResult.freeList;
    }

    return 0;
}

WeakBlock::FreeCell* WeakSet::addAllocator()
{
    WeakBlock* block = WeakBlock::create(heap()->blockAllocator().allocate<WeakBlock>());
    heap()->didAllocate(WeakBlock::blockSize);
    m_blocks.append(block);
    WeakBlock::SweepResult sweepResult = block->takeSweepResult();
    ASSERT(!sweepResult.isNull() && sweepResult.freeList);
    return sweepResult.freeList;
}

void WeakSet::removeAllocator(WeakBlock* block)
{
    m_blocks.remove(block);
    heap()->blockAllocator().deallocate(WeakBlock::destroy(block));
}

} // namespace JSC
