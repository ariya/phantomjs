/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#ifndef JSC_Region_h
#define JSC_Region_h

#include "HeapBlock.h"
#include "SuperRegion.h"
#include <wtf/DoublyLinkedList.h>
#include <wtf/MetaAllocatorHandle.h>
#include <wtf/PageAllocationAligned.h>

#define HEAP_MEMORY_ID reinterpret_cast<void*>(static_cast<intptr_t>(-3))

#define ENABLE_SUPER_REGION 0

#ifndef ENABLE_SUPER_REGION
#if USE(JSVALUE64)
#define ENABLE_SUPER_REGION 1
#else
#define ENABLE_SUPER_REGION 0
#endif
#endif

namespace JSC {

class DeadBlock : public HeapBlock<DeadBlock> {
public:
    DeadBlock(Region*);
};

inline DeadBlock::DeadBlock(Region* region)
    : HeapBlock<DeadBlock>(region)
{
}

class Region : public DoublyLinkedListNode<Region> {
    friend CLASS_IF_GCC DoublyLinkedListNode<Region>;
    friend class BlockAllocator;
public:
    ~Region();
    static Region* create(SuperRegion*, size_t blockSize);
    static Region* createCustomSize(SuperRegion*, size_t blockSize, size_t blockAlignment);
    Region* reset(size_t blockSize);
    void destroy();

    size_t blockSize() const { return m_blockSize; }
    bool isFull() const { return m_blocksInUse == m_totalBlocks; }
    bool isEmpty() const { return !m_blocksInUse; }
    bool isCustomSize() const { return m_isCustomSize; }

    DeadBlock* allocate();
    void deallocate(void*);

    static const size_t s_regionSize = 64 * KB;
    static const size_t s_regionMask = ~(s_regionSize - 1);

protected:
    Region(size_t blockSize, size_t totalBlocks, bool isExcess);
    void initializeBlockList();

    bool m_isExcess;

private:
    void* base();
    size_t size();

    size_t m_totalBlocks;
    size_t m_blocksInUse;
    size_t m_blockSize;
    bool m_isCustomSize;
    Region* m_prev;
    Region* m_next;
    DoublyLinkedList<DeadBlock> m_deadBlocks;
};


class NormalRegion : public Region {
    friend class Region;
private:
    NormalRegion(PassRefPtr<WTF::MetaAllocatorHandle>, size_t blockSize, size_t totalBlocks);

    static NormalRegion* tryCreate(SuperRegion*, size_t blockSize);
    static NormalRegion* tryCreateCustomSize(SuperRegion*, size_t blockSize, size_t blockAlignment);

    void* base() { return m_allocation->start(); }
    size_t size() { return m_allocation->sizeInBytes(); }

    NormalRegion* reset(size_t blockSize);

    RefPtr<WTF::MetaAllocatorHandle> m_allocation;
};

class ExcessRegion : public Region {
    friend class Region;
private:
    ExcessRegion(PageAllocationAligned&, size_t blockSize, size_t totalBlocks);

    ~ExcessRegion();

    static ExcessRegion* create(size_t blockSize);
    static ExcessRegion* createCustomSize(size_t blockSize, size_t blockAlignment);

    void* base() { return m_allocation.base(); }
    size_t size() { return m_allocation.size(); }

    ExcessRegion* reset(size_t blockSize);

    PageAllocationAligned m_allocation;
};

inline NormalRegion::NormalRegion(PassRefPtr<WTF::MetaAllocatorHandle> allocation, size_t blockSize, size_t totalBlocks)
    : Region(blockSize, totalBlocks, false)
    , m_allocation(allocation)
{
    initializeBlockList();
}

inline NormalRegion* NormalRegion::tryCreate(SuperRegion* superRegion, size_t blockSize)
{
    RefPtr<WTF::MetaAllocatorHandle> allocation = superRegion->allocate(s_regionSize, HEAP_MEMORY_ID);
    if (!allocation)
        return 0;
    return new NormalRegion(allocation, blockSize, s_regionSize / blockSize);
}

inline NormalRegion* NormalRegion::tryCreateCustomSize(SuperRegion* superRegion, size_t blockSize, size_t blockAlignment)
{
    ASSERT_UNUSED(blockAlignment, blockAlignment <= s_regionSize);
    RefPtr<WTF::MetaAllocatorHandle> allocation = superRegion->allocate(blockSize, HEAP_MEMORY_ID);
    if (!allocation)
        return 0;
    return new NormalRegion(allocation, blockSize, 1);
}

inline NormalRegion* NormalRegion::reset(size_t blockSize)
{
    ASSERT(!m_isExcess);
    RefPtr<WTF::MetaAllocatorHandle> allocation = m_allocation.release();
    return new (NotNull, this) NormalRegion(allocation.release(), blockSize, s_regionSize / blockSize);
}

inline ExcessRegion::ExcessRegion(PageAllocationAligned& allocation, size_t blockSize, size_t totalBlocks)
    : Region(blockSize, totalBlocks, true)
    , m_allocation(allocation)
{
    initializeBlockList();
}

inline ExcessRegion::~ExcessRegion()
{
    m_allocation.deallocate();
}

inline ExcessRegion* ExcessRegion::create(size_t blockSize)
{
    PageAllocationAligned allocation = PageAllocationAligned::allocate(s_regionSize, s_regionSize, OSAllocator::JSGCHeapPages);
    ASSERT(static_cast<bool>(allocation));
    return new ExcessRegion(allocation, blockSize, s_regionSize / blockSize); 
}

inline ExcessRegion* ExcessRegion::createCustomSize(size_t blockSize, size_t blockAlignment)
{
    PageAllocationAligned allocation = PageAllocationAligned::allocate(blockSize, blockAlignment, OSAllocator::JSGCHeapPages);
    ASSERT(static_cast<bool>(allocation));
    return new ExcessRegion(allocation, blockSize, 1);
}

inline ExcessRegion* ExcessRegion::reset(size_t blockSize)
{
    ASSERT(m_isExcess);
    PageAllocationAligned allocation = m_allocation;
    return new (NotNull, this) ExcessRegion(allocation, blockSize, s_regionSize / blockSize);
}

inline Region::Region(size_t blockSize, size_t totalBlocks, bool isExcess)
    : DoublyLinkedListNode<Region>()
    , m_isExcess(isExcess)
    , m_totalBlocks(totalBlocks)
    , m_blocksInUse(0)
    , m_blockSize(blockSize)
    , m_isCustomSize(false)
    , m_prev(0)
    , m_next(0)
{
}

inline void Region::initializeBlockList()
{
    char* start = static_cast<char*>(base());
    char* current = start;
    for (size_t i = 0; i < m_totalBlocks; i++) {
        ASSERT(current < start + size());
        m_deadBlocks.append(new (NotNull, current) DeadBlock(this));
        current += m_blockSize;
    }
}

inline Region* Region::create(SuperRegion* superRegion, size_t blockSize)
{
#if ENABLE(SUPER_REGION)
    ASSERT(blockSize <= s_regionSize);
    ASSERT(!(s_regionSize % blockSize));
    Region* region = NormalRegion::tryCreate(superRegion, blockSize);
    if (LIKELY(!!region))
        return region;
#else
    UNUSED_PARAM(superRegion);
#endif
    return ExcessRegion::create(blockSize);
}

inline Region* Region::createCustomSize(SuperRegion* superRegion, size_t blockSize, size_t blockAlignment)
{
#if ENABLE(SUPER_REGION)
    Region* region = NormalRegion::tryCreateCustomSize(superRegion, blockSize, blockAlignment);
    if (UNLIKELY(!region))
        region = ExcessRegion::createCustomSize(blockSize, blockAlignment);
#else
    UNUSED_PARAM(superRegion);
    Region* region = ExcessRegion::createCustomSize(blockSize, blockAlignment);
#endif
    region->m_isCustomSize = true;
    return region;
}

inline Region::~Region()
{
    ASSERT(isEmpty());
}

inline void Region::destroy()
{
#if ENABLE(SUPER_REGION)
    if (UNLIKELY(m_isExcess))
        delete static_cast<ExcessRegion*>(this);
    else
        delete static_cast<NormalRegion*>(this);
#else
    delete static_cast<ExcessRegion*>(this);
#endif
}

inline Region* Region::reset(size_t blockSize)
{
#if ENABLE(SUPER_REGION)
    ASSERT(isEmpty());
    if (UNLIKELY(m_isExcess))
        return static_cast<ExcessRegion*>(this)->reset(blockSize);
    return static_cast<NormalRegion*>(this)->reset(blockSize);
#else
    return static_cast<ExcessRegion*>(this)->reset(blockSize);
#endif
}

inline DeadBlock* Region::allocate()
{
    ASSERT(!isFull());
    m_blocksInUse++;
    return m_deadBlocks.removeHead();
}

inline void Region::deallocate(void* base)
{
    ASSERT(base);
    ASSERT(m_blocksInUse);
    ASSERT(base >= this->base() && base < static_cast<char*>(this->base()) + size());
    DeadBlock* block = new (NotNull, base) DeadBlock(this);
    m_deadBlocks.push(block);
    m_blocksInUse--;
}

inline void* Region::base()
{
#if ENABLE(SUPER_REGION)
    if (UNLIKELY(m_isExcess))
        return static_cast<ExcessRegion*>(this)->ExcessRegion::base();
    return static_cast<NormalRegion*>(this)->NormalRegion::base();
#else
    return static_cast<ExcessRegion*>(this)->ExcessRegion::base();
#endif
}

inline size_t Region::size()
{
#if ENABLE(SUPER_REGION)
    if (UNLIKELY(m_isExcess))
        return static_cast<ExcessRegion*>(this)->ExcessRegion::size();
    return static_cast<NormalRegion*>(this)->NormalRegion::size();
#else
    return static_cast<ExcessRegion*>(this)->ExcessRegion::size();
#endif
}

} // namespace JSC

#endif // JSC_Region_h
