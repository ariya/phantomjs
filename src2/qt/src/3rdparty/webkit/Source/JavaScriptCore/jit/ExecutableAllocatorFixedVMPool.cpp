/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"

#include "ExecutableAllocator.h"

#if ENABLE(EXECUTABLE_ALLOCATOR_FIXED)

#include <errno.h>

#include "TCSpinLock.h"
#include <sys/mman.h>
#include <unistd.h>
#include <wtf/AVLTree.h>
#include <wtf/PageReservation.h>
#include <wtf/VMTags.h>

#if OS(LINUX)
#include <stdio.h>
#endif

using namespace WTF;

namespace JSC {
    
#define TwoPow(n) (1ull << n)

class AllocationTableSizeClass {
public:
    AllocationTableSizeClass(size_t size, size_t blockSize, unsigned log2BlockSize)
        : m_blockSize(blockSize)
    {
        ASSERT(blockSize == TwoPow(log2BlockSize));

        // Calculate the number of blocks needed to hold size.
        size_t blockMask = blockSize - 1;
        m_blockCount = (size + blockMask) >> log2BlockSize;

        // Align to the smallest power of two >= m_blockCount.
        m_blockAlignment = 1;
        while (m_blockAlignment < m_blockCount)
            m_blockAlignment += m_blockAlignment;
    }

    size_t blockSize() const { return m_blockSize; }
    size_t blockCount() const { return m_blockCount; }
    size_t blockAlignment() const { return m_blockAlignment; }

    size_t size()
    {
        return m_blockSize * m_blockCount;
    }

private:
    size_t m_blockSize;
    size_t m_blockCount;
    size_t m_blockAlignment;
};

template<unsigned log2Entries>
class AllocationTableLeaf {
    typedef uint64_t BitField;

public:
    static const unsigned log2SubregionSize = 12; // 2^12 == pagesize
    static const unsigned log2RegionSize = log2SubregionSize + log2Entries;

    static const size_t subregionSize = TwoPow(log2SubregionSize);
    static const size_t regionSize = TwoPow(log2RegionSize);
    static const unsigned entries = TwoPow(log2Entries);
    COMPILE_ASSERT(entries <= (sizeof(BitField) * 8), AllocationTableLeaf_entries_fit_in_BitField);

    AllocationTableLeaf()
        : m_allocated(0)
    {
    }

    ~AllocationTableLeaf()
    {
        ASSERT(isEmpty());
    }

    size_t allocate(AllocationTableSizeClass& sizeClass)
    {
        ASSERT(sizeClass.blockSize() == subregionSize);
        ASSERT(!isFull());

        size_t alignment = sizeClass.blockAlignment();
        size_t count = sizeClass.blockCount();
        // Use this mask to check for spans of free blocks.
        BitField mask = ((1ull << count) - 1) << (alignment - count);

        // Step in units of alignment size.
        for (unsigned i = 0; i < entries; i += alignment) {
            if (!(m_allocated & mask)) {
                m_allocated |= mask;
                return (i + (alignment - count)) << log2SubregionSize;
            }
            mask <<= alignment;
        }
        return notFound;
    }

    void free(size_t location, AllocationTableSizeClass& sizeClass)
    {
        ASSERT(sizeClass.blockSize() == subregionSize);

        size_t entry = location >> log2SubregionSize;
        size_t count = sizeClass.blockCount();
        BitField mask = ((1ull << count) - 1) << entry;

        ASSERT((m_allocated & mask) == mask);
        m_allocated &= ~mask;
    }

    bool isEmpty()
    {
        return !m_allocated;
    }

    bool isFull()
    {
        return !~m_allocated;
    }

    static size_t size()
    {
        return regionSize;
    }

    static AllocationTableSizeClass classForSize(size_t size)
    {
        return AllocationTableSizeClass(size, subregionSize, log2SubregionSize);
    }

#ifndef NDEBUG
    void dump(size_t parentOffset = 0, unsigned indent = 0)
    {
        for (unsigned i = 0; i < indent; ++i)
            fprintf(stderr, "    ");
        fprintf(stderr, "%08x: [%016llx]\n", (int)parentOffset, m_allocated);
    }
#endif

private:
    BitField m_allocated;
};


template<class NextLevel>
class LazyAllocationTable {
public:
    static const unsigned log2RegionSize = NextLevel::log2RegionSize;
    static const unsigned entries = NextLevel::entries;

    LazyAllocationTable()
        : m_ptr(0)
    {
    }

    ~LazyAllocationTable()
    {
        ASSERT(isEmpty());
    }

    size_t allocate(AllocationTableSizeClass& sizeClass)
    {
        if (!m_ptr)
            m_ptr = new NextLevel();
        return m_ptr->allocate(sizeClass);
    }

    void free(size_t location, AllocationTableSizeClass& sizeClass)
    {
        ASSERT(m_ptr);
        m_ptr->free(location, sizeClass);
        if (m_ptr->isEmpty()) {
            delete m_ptr;
            m_ptr = 0;
        }
    }

    bool isEmpty()
    {
        return !m_ptr;
    }

    bool isFull()
    {
        return m_ptr && m_ptr->isFull();
    }

    static size_t size()
    {
        return NextLevel::size();
    }

#ifndef NDEBUG
    void dump(size_t parentOffset = 0, unsigned indent = 0)
    {
        ASSERT(m_ptr);
        m_ptr->dump(parentOffset, indent);
    }
#endif

    static AllocationTableSizeClass classForSize(size_t size)
    {
        return NextLevel::classForSize(size);
    }

private:
    NextLevel* m_ptr;
};

template<class NextLevel, unsigned log2Entries>
class AllocationTableDirectory {
    typedef uint64_t BitField;

public:
    static const unsigned log2SubregionSize = NextLevel::log2RegionSize;
    static const unsigned log2RegionSize = log2SubregionSize + log2Entries;

    static const size_t subregionSize = TwoPow(log2SubregionSize);
    static const size_t regionSize = TwoPow(log2RegionSize);
    static const unsigned entries = TwoPow(log2Entries);
    COMPILE_ASSERT(entries <= (sizeof(BitField) * 8), AllocationTableDirectory_entries_fit_in_BitField);

    AllocationTableDirectory()
        : m_full(0)
        , m_hasSuballocation(0)
    {
    }

    ~AllocationTableDirectory()
    {
        ASSERT(isEmpty());
    }

    size_t allocate(AllocationTableSizeClass& sizeClass)
    {
        ASSERT(sizeClass.blockSize() <= subregionSize);
        ASSERT(!isFull());

        if (sizeClass.blockSize() < subregionSize) {
            BitField bit = 1;
            for (unsigned i = 0; i < entries; ++i, bit += bit) {
                if (m_full & bit)
                    continue;
                size_t location = m_suballocations[i].allocate(sizeClass);
                if (location != notFound) {
                    // If this didn't already have a subregion, it does now!
                    m_hasSuballocation |= bit;
                    // Mirror the suballocation's full bit.
                    if (m_suballocations[i].isFull())
                        m_full |= bit;
                    return (i * subregionSize) | location;
                }
            }
            return notFound;
        }

        // A block is allocated if either it is fully allocated or contains suballocations.
        BitField allocated = m_full | m_hasSuballocation;

        size_t alignment = sizeClass.blockAlignment();
        size_t count = sizeClass.blockCount();
        // Use this mask to check for spans of free blocks.
        BitField mask = ((1ull << count) - 1) << (alignment - count);

        // Step in units of alignment size.
        for (unsigned i = 0; i < entries; i += alignment) {
            if (!(allocated & mask)) {
                m_full |= mask;
                return (i + (alignment - count)) << log2SubregionSize;
            }
            mask <<= alignment;
        }
        return notFound;
    }

    void free(size_t location, AllocationTableSizeClass& sizeClass)
    {
        ASSERT(sizeClass.blockSize() <= subregionSize);

        size_t entry = location >> log2SubregionSize;

        if (sizeClass.blockSize() < subregionSize) {
            BitField bit = 1ull << entry;
            m_suballocations[entry].free(location & (subregionSize - 1), sizeClass);
            // Check if the suballocation is now empty.
            if (m_suballocations[entry].isEmpty())
                m_hasSuballocation &= ~bit;
            // No need to check, it clearly isn't full any more!
            m_full &= ~bit;
        } else {
            size_t count = sizeClass.blockCount();
            BitField mask = ((1ull << count) - 1) << entry;
            ASSERT((m_full & mask) == mask);
            ASSERT(!(m_hasSuballocation & mask));
            m_full &= ~mask;
        }
    }

    bool isEmpty()
    {
        return !(m_full | m_hasSuballocation);
    }

    bool isFull()
    {   
        return !~m_full;
    }

    static size_t size()
    {
        return regionSize;
    }

    static AllocationTableSizeClass classForSize(size_t size)
    {
        if (size < subregionSize) {
            AllocationTableSizeClass sizeClass = NextLevel::classForSize(size);
            if (sizeClass.size() < NextLevel::size())
                return sizeClass;
        }
        return AllocationTableSizeClass(size, subregionSize, log2SubregionSize);
    }

#ifndef NDEBUG
    void dump(size_t parentOffset = 0, unsigned indent = 0)
    {
        for (unsigned i = 0; i < indent; ++i)
            fprintf(stderr, "    ");
        fprintf(stderr, "%08x: [", (int)parentOffset);
        for (unsigned i = 0; i < entries; ++i) {
            BitField bit = 1ull << i;
            char c = m_hasSuballocation & bit
                ? (m_full & bit ? 'N' : 'n')
                : (m_full & bit ? 'F' : '-');
            fprintf(stderr, "%c", c);
        }
        fprintf(stderr, "]\n");

        for (unsigned i = 0; i < entries; ++i) {
            BitField bit = 1ull << i;
            size_t offset = parentOffset | (subregionSize * i);
            if (m_hasSuballocation & bit)
                m_suballocations[i].dump(offset, indent + 1);
        }
    }
#endif

private:
    NextLevel m_suballocations[entries];
    // Subregions exist in one of four states:
    // (1) empty (both bits clear)
    // (2) fully allocated as a single allocation (m_full set)
    // (3) partially allocated through suballocations (m_hasSuballocation set)
    // (4) fully allocated through suballocations (both bits set)
    BitField m_full;
    BitField m_hasSuballocation;
};


typedef AllocationTableLeaf<6> PageTables256KB;
typedef AllocationTableDirectory<PageTables256KB, 6> PageTables16MB;
typedef AllocationTableDirectory<LazyAllocationTable<PageTables16MB>, 1> PageTables32MB;
typedef AllocationTableDirectory<LazyAllocationTable<PageTables16MB>, 6> PageTables1GB;

#if CPU(ARM)
typedef PageTables16MB FixedVMPoolPageTables;
#elif CPU(X86_64)
typedef PageTables1GB FixedVMPoolPageTables;
#else
typedef PageTables32MB FixedVMPoolPageTables;
#endif


class FixedVMPoolAllocator
{
public:
    FixedVMPoolAllocator()
    {
        ASSERT(PageTables256KB::size() == 256 * 1024);
        ASSERT(PageTables16MB::size() == 16 * 1024 * 1024);
        ASSERT(PageTables32MB::size() == 32 * 1024 * 1024);
        ASSERT(PageTables1GB::size() == 1024 * 1024 * 1024);

        m_reservation = PageReservation::reserve(FixedVMPoolPageTables::size(), OSAllocator::JSJITCodePages, EXECUTABLE_POOL_WRITABLE, true);
#if !ENABLE(INTERPRETER)
        if (!isValid())
            CRASH();
#endif
    }
 
    ExecutablePool::Allocation alloc(size_t requestedSize)
    {
        ASSERT(requestedSize);
        AllocationTableSizeClass sizeClass = classForSize(requestedSize);
        size_t size = sizeClass.size();
        ASSERT(size);

        if (size >= FixedVMPoolPageTables::size())
            CRASH();
        if (m_pages.isFull())
            CRASH();

        size_t offset = m_pages.allocate(sizeClass);
        if (offset == notFound)
            CRASH();

        void* pointer = offsetToPointer(offset);
        m_reservation.commit(pointer, size);
        return ExecutablePool::Allocation(pointer, size);
    }

    void free(ExecutablePool::Allocation allocation)
    {
        void* pointer = allocation.base();
        size_t size = allocation.size();
        ASSERT(size);

        m_reservation.decommit(pointer, size);

        AllocationTableSizeClass sizeClass = classForSize(size);
        ASSERT(sizeClass.size() == size);
        m_pages.free(pointerToOffset(pointer), sizeClass);
    }

    size_t allocated()
    {
        return m_reservation.committed();
    }

    bool isValid() const
    {
        return !!m_reservation;
    }

private:
    AllocationTableSizeClass classForSize(size_t size)
    {
        return FixedVMPoolPageTables::classForSize(size);
    }

    void* offsetToPointer(size_t offset)
    {
        return reinterpret_cast<void*>(reinterpret_cast<intptr_t>(m_reservation.base()) + offset);
    }

    size_t pointerToOffset(void* pointer)
    {
        return reinterpret_cast<intptr_t>(pointer) - reinterpret_cast<intptr_t>(m_reservation.base());
    }

    PageReservation m_reservation;
    FixedVMPoolPageTables m_pages;
};


static SpinLock spinlock = SPINLOCK_INITIALIZER;
static FixedVMPoolAllocator* allocator = 0;


size_t ExecutableAllocator::committedByteCount()
{
    SpinLockHolder lockHolder(&spinlock);
    return allocator ? allocator->allocated() : 0;
}   

void ExecutableAllocator::intializePageSize()
{
    ExecutableAllocator::pageSize = getpagesize();
}

bool ExecutableAllocator::isValid() const
{
    SpinLockHolder lock_holder(&spinlock);
    if (!allocator)
        allocator = new FixedVMPoolAllocator();
    return allocator->isValid();
}

bool ExecutableAllocator::underMemoryPressure()
{
    // Technically we should take the spin lock here, but we don't care if we get stale data.
    // This is only really a heuristic anyway.
    return allocator && (allocator->allocated() > (FixedVMPoolPageTables::size() / 2));
}

ExecutablePool::Allocation ExecutablePool::systemAlloc(size_t size)
{
    SpinLockHolder lock_holder(&spinlock);
    ASSERT(allocator);
    return allocator->alloc(size);
}

void ExecutablePool::systemRelease(ExecutablePool::Allocation& allocation) 
{
    SpinLockHolder lock_holder(&spinlock);
    ASSERT(allocator);
    allocator->free(allocation);
}

}


#endif // HAVE(ASSEMBLER)
