/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
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

#if ENABLE(ASSEMBLER)

namespace JSC {

size_t ExecutableAllocator::pageSize = 0;

#if ENABLE(EXECUTABLE_ALLOCATOR_DEMAND)

void ExecutableAllocator::intializePageSize()
{
#if OS(SYMBIAN) && CPU(ARMV5_OR_LOWER)
    // The moving memory model (as used in ARMv5 and earlier platforms)
    // on Symbian OS limits the number of chunks for each process to 16. 
    // To mitigate this limitation increase the pagesize to allocate
    // fewer, larger chunks. Set the page size to 256 Kb to compensate
    // for moving memory model limitation
    ExecutableAllocator::pageSize = 256 * 1024;
#else
    ExecutableAllocator::pageSize = WTF::pageSize();
#endif
}

ExecutablePool::Allocation ExecutablePool::systemAlloc(size_t size)
{
    PageAllocation allocation = PageAllocation::allocate(size, OSAllocator::JSJITCodePages, EXECUTABLE_POOL_WRITABLE, true);
    if (!allocation)
        CRASH();
    return allocation;
}

void ExecutablePool::systemRelease(ExecutablePool::Allocation& allocation)
{
    allocation.deallocate();
}

bool ExecutableAllocator::isValid() const
{
    return true;
}
    
bool ExecutableAllocator::underMemoryPressure()
{
    return false;
}
    
size_t ExecutableAllocator::committedByteCount()
{
    return 0;
} 

#endif

#if ENABLE(ASSEMBLER_WX_EXCLUSIVE)

#if OS(WINDOWS) || OS(SYMBIAN)
#error "ASSEMBLER_WX_EXCLUSIVE not yet suported on this platform."
#endif

void ExecutableAllocator::reprotectRegion(void* start, size_t size, ProtectionSetting setting)
{
    if (!pageSize)
        intializePageSize();

    // Calculate the start of the page containing this region,
    // and account for this extra memory within size.
    intptr_t startPtr = reinterpret_cast<intptr_t>(start);
    intptr_t pageStartPtr = startPtr & ~(pageSize - 1);
    void* pageStart = reinterpret_cast<void*>(pageStartPtr);
    size += (startPtr - pageStartPtr);

    // Round size up
    size += (pageSize - 1);
    size &= ~(pageSize - 1);

    mprotect(pageStart, size, (setting == Writable) ? PROTECTION_FLAGS_RW : PROTECTION_FLAGS_RX);
}

#endif

#if CPU(ARM_TRADITIONAL) && OS(LINUX) && COMPILER(RVCT)

__asm void ExecutableAllocator::cacheFlush(void* code, size_t size)
{
    ARM
    push {r7}
    add r1, r1, r0
    mov r7, #0xf0000
    add r7, r7, #0x2
    mov r2, #0x0
    svc #0x0
    pop {r7}
    bx lr
}

#endif

}

#endif // HAVE(ASSEMBLER)
