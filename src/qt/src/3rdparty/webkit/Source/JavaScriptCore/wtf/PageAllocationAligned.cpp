/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
#include "PageAllocationAligned.h"

namespace WTF {

PageAllocationAligned PageAllocationAligned::allocate(size_t size, size_t alignment, OSAllocator::Usage usage, bool writable, bool executable)
{
    ASSERT(isPageAligned(size));
    ASSERT(isPageAligned(alignment));
    ASSERT(isPowerOfTwo(alignment));
    ASSERT(size >= alignment);
    size_t alignmentMask = alignment - 1;

#if OS(DARWIN)
    int flags = VM_FLAGS_ANYWHERE;
    if (usage != OSAllocator::UnknownUsage)
        flags |= usage;
    int protection = PROT_READ;
    if (writable)
        protection |= PROT_WRITE;
    if (executable)
        protection |= PROT_EXEC;

    vm_address_t address = 0;
    vm_map(current_task(), &address, size, alignmentMask, flags, MEMORY_OBJECT_NULL, 0, FALSE, protection, PROT_READ | PROT_WRITE | PROT_EXEC, VM_INHERIT_DEFAULT);
    return PageAllocationAligned(reinterpret_cast<void*>(address), size);
#else
    size_t alignmentDelta = alignment - pageSize();

    // Resererve with suffcient additional VM to correctly align.
    size_t reservationSize = size + alignmentDelta;
    void* reservationBase = OSAllocator::reserveUncommitted(reservationSize, usage, writable, executable);

    // Select an aligned region within the reservation and commit.
    void* alignedBase = reinterpret_cast<uintptr_t>(reservationBase) & alignmentMask
        ? reinterpret_cast<void*>((reinterpret_cast<uintptr_t>(reservationBase) & ~alignmentMask) + alignment)
        : reservationBase;
    OSAllocator::commit(alignedBase, size, writable, executable);

    return PageAllocationAligned(alignedBase, size, reservationBase, reservationSize);
#endif
}

void PageAllocationAligned::deallocate()
{
    // Clear base & size before calling release; if this is *inside* allocation
    // then we won't be able to clear then after deallocating the memory.
    PageAllocationAligned tmp;
    std::swap(tmp, *this);

    ASSERT(tmp);
    ASSERT(!*this);

#if OS(DARWIN)
    vm_deallocate(current_task(), reinterpret_cast<vm_address_t>(tmp.base()), tmp.size());
#else
    ASSERT(tmp.m_reservation.contains(tmp.base(), tmp.size()));
    OSAllocator::decommitAndRelease(tmp.m_reservation.base(), tmp.m_reservation.size(), tmp.base(), tmp.size());
#endif
}

} // namespace WTF
