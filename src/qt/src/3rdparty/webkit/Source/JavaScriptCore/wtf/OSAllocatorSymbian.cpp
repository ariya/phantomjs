/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
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
#include "OSAllocator.h"

#include "PageAllocatorSymbian.h"

namespace WTF {

// Array to store code chunks used by JIT engine(s)
static RPointerArray<SymbianChunk> codeChunksContainer;

// The singleton data allocator (non code)
static PageAllocatorSymbian dataAllocator;

_LIT(KErrorStringInternalConsistency, "OSAllocator:ConsistencyError");
_LIT(KErrorStringChunkCreation, "OSAllocator:ChunkInitError");
_LIT(KErrorStringPageSize, "OSAllocator:WrongPageSize");

// Makes a new code chunk for a JIT engine with everything in committed state
static void* allocateCodeChunk(size_t bytes) 
{
    RChunk c; 
    TInt error = c.CreateLocalCode(bytes, bytes);
    __ASSERT_ALWAYS(error == KErrNone, User::Panic(KErrorStringChunkCreation, error));
   
    codeChunksContainer.Append(new SymbianChunk(c.Handle())); 
    return static_cast<void*>(c.Base()); 
}
   
// Frees the _entire_ code chunk in which this address resides. 
static bool deallocateCodeChunk(void* address) 
{     
    bool found = false; 
    for (int i = 0; i < codeChunksContainer.Count(); i++) { 
        SymbianChunk* p = codeChunksContainer[i];
        if (p && p->contains(address)) { 
            codeChunksContainer.Remove(i);
            delete p;
            found = true;
        }
    }
    return found; 
}

// Return the (singleton) object that manages all non-code VM operations
static PageAllocatorSymbian* dataAllocatorInstance() 
{
    return &dataAllocator; 
}

// Reserve memory and return the base address of the region
void* OSAllocator::reserveUncommitted(size_t reservationSize, Usage usage, bool , bool executable) 
{
    void* base = 0; 
    if (executable) 
        base = allocateCodeChunk(reservationSize);
    else
        base = dataAllocatorInstance()->reserve(reservationSize);
    return base; 
}

// Inverse operation of reserveUncommitted()
void OSAllocator::releaseDecommitted(void* parkedBase, size_t bytes) 
{
    if (dataAllocatorInstance()->contains(parkedBase)) 
        dataAllocatorInstance()->release(parkedBase, bytes);

    // NOOP for code chunks (JIT) because we released them in decommit()
}

// Commit what was previously reserved via reserveUncommitted()
void OSAllocator::commit(void* address, size_t bytes, bool, bool executable) 
{
    // For code chunks, we commit (early) in reserveUncommitted(), so NOOP
    // For data regions, do real work
    if (!executable) 
        dataAllocatorInstance()->commit(address, bytes);
}

void OSAllocator::decommit(void* address, size_t bytes) 
{ 
    if (dataAllocatorInstance()->contains(address))
        dataAllocatorInstance()->decommit(address, bytes);
    else
        deallocateCodeChunk(address); // for code chunk, decommit AND release    
}
    
void* OSAllocator::reserveAndCommit(size_t bytes, Usage usage, bool writable, bool executable)
{ 
    void* base = reserveUncommitted(bytes, usage, writable, executable);
    commit(base, bytes, writable, executable);
    return base;
}


// The PageAllocatorSymbian class helps map OSAllocator calls for reserve/commit/decommit
// to a single large Symbian chunk. Only works with multiples of page size, and as a corollary 
// all addresses accepted or returned by it are also page-sized aligned. 
// Design notes: 
// - We initialize a chunk up-front with a large reservation size
// - The entire reservation reserve is logically divided into pageSized blocks (4K on Symbian) 
// - The map maintains 1 bit for each of the 4K-sized region in our address space
// - OSAllocator::reserveUncommitted() requests lead to 1 or more bits being set in map 
//   to indicate internally reserved state. The VM address corresponding to the first bit is returned. 
// - OSAllocator::commit() actually calls RChunk.commit() and commits *all or part* of the region 
//   reserved via reserveUncommitted() previously. 
// - OSAllocator::decommit() calls RChunk.decommit() 
// - OSAllocator::releaseDecommitted() unparks all the bits in the map, but trusts that a previously
//   call to decommit() would have returned the memory to the OS 
PageAllocatorSymbian::PageAllocatorSymbian()
{        
    __ASSERT_ALWAYS(m_pageSize == WTF::pageSize(), User::Panic(KErrorStringPageSize, m_pageSize));
    
    RChunk chunk;
    TInt error = chunk.CreateDisconnectedLocal(0, 0, TInt(largeReservationSize));
    __ASSERT_ALWAYS(error == KErrNone, User::Panic(KErrorStringChunkCreation, error));
    
    m_chunk = new SymbianChunk(chunk.Handle()); // takes ownership of chunk
}

PageAllocatorSymbian::~PageAllocatorSymbian() 
{
    delete m_chunk;
}

// Reserves a region internally in the bitmap
void* PageAllocatorSymbian::reserve(size_t bytes) 
{ 
    // Find first available region
    const size_t nPages = bytes / m_pageSize;
    const int64_t startIdx = m_map.findRunOfZeros(nPages);

    // Pseudo OOM
    if (startIdx < 0)
        return 0;
    
    for (size_t i = startIdx; i < startIdx + nPages ; i++)
        m_map.set(i);
    
    return static_cast<void*>( m_chunk->m_base + (TUint)(m_pageSize * startIdx) ); 
}

// Reverses the effects of a reserve() call
void PageAllocatorSymbian::release(void* address, size_t bytes)
{
    const size_t startIdx = (static_cast<char*>(address) - m_chunk->m_base) / m_pageSize;
    const size_t nPages = bytes / m_pageSize;
    for (size_t i = startIdx; i < startIdx + nPages ; i++)
        m_map.clear(i);
}

// Actually commit memory from the OS, after a previous call to reserve()
bool PageAllocatorSymbian::commit(void* address, size_t bytes) 
{ 
    // sanity check that bits were previously set
    const size_t idx = (static_cast<char*>(address) - m_chunk->m_base) / m_pageSize;
    const size_t nPages = bytes / m_pageSize;
    __ASSERT_ALWAYS(m_map.get(idx), User::Panic(KErrorStringInternalConsistency, idx));
    __ASSERT_ALWAYS(m_map.get(idx+nPages-1), User::Panic(KErrorStringInternalConsistency, idx+nPages-1));
    
    TInt error = m_chunk->Commit(static_cast<char*>(address) - m_chunk->m_base, bytes);
    return (error == KErrNone);
}

// Inverse operation of commit(), a release() should follow later
bool PageAllocatorSymbian::decommit(void* address, size_t bytes) 
{ 
    TInt error = m_chunk->Decommit(static_cast<char*>(address) - m_chunk->m_base, bytes);
    return (error == KErrNone); 
}

bool PageAllocatorSymbian::contains(const void* address) const
{
    return m_chunk->contains(address);     
}

} // namespace WTF
