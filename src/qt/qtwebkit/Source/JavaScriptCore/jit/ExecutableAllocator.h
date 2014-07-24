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

#ifndef ExecutableAllocator_h
#define ExecutableAllocator_h
#include "JITCompilationEffort.h"
#include <stddef.h> // for ptrdiff_t
#include <limits>
#include <wtf/Assertions.h>
#include <wtf/MetaAllocatorHandle.h>
#include <wtf/MetaAllocator.h>
#include <wtf/PageAllocation.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>

#if OS(IOS)
#include <libkern/OSCacheControl.h>
#endif

#if OS(IOS) || OS(QNX)
#include <sys/mman.h>
#endif

#if CPU(MIPS) && OS(LINUX)
#include <sys/cachectl.h>
#endif

#if CPU(SH4) && OS(LINUX)
#include <asm/cachectl.h>
#include <asm/unistd.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif

#if OS(WINCE)
// From pkfuncs.h (private header file from the Platform Builder)
#define CACHE_SYNC_ALL 0x07F
extern "C" __declspec(dllimport) void CacheRangeFlush(LPVOID pAddr, DWORD dwLength, DWORD dwFlags);
#endif

#define JIT_ALLOCATOR_LARGE_ALLOC_SIZE (pageSize() * 4)

#if ENABLE(ASSEMBLER_WX_EXCLUSIVE)
#define PROTECTION_FLAGS_RW (PROT_READ | PROT_WRITE)
#define PROTECTION_FLAGS_RX (PROT_READ | PROT_EXEC)
#define EXECUTABLE_POOL_WRITABLE false
#else
#define EXECUTABLE_POOL_WRITABLE true
#endif

namespace JSC {

class VM;
void releaseExecutableMemory(VM&);

static const unsigned jitAllocationGranule = 32;

inline size_t roundUpAllocationSize(size_t request, size_t granularity)
{
    RELEASE_ASSERT((std::numeric_limits<size_t>::max() - granularity) > request);
    
    // Round up to next page boundary
    size_t size = request + (granularity - 1);
    size = size & ~(granularity - 1);
    ASSERT(size >= request);
    return size;
}

}

namespace JSC {

typedef WTF::MetaAllocatorHandle ExecutableMemoryHandle;

#if ENABLE(ASSEMBLER)

#if ENABLE(EXECUTABLE_ALLOCATOR_DEMAND)
class DemandExecutableAllocator;
#endif

#if ENABLE(EXECUTABLE_ALLOCATOR_FIXED)
#if CPU(ARM)
static const size_t fixedExecutableMemoryPoolSize = 16 * 1024 * 1024;
#elif CPU(X86_64) && !CPU(X32)
static const size_t fixedExecutableMemoryPoolSize = 1024 * 1024 * 1024;
#else
static const size_t fixedExecutableMemoryPoolSize = 32 * 1024 * 1024;
#endif

extern uintptr_t startOfFixedExecutableMemoryPool;
#endif

class ExecutableAllocator {
    enum ProtectionSetting { Writable, Executable };

public:
    ExecutableAllocator(VM&);
    ~ExecutableAllocator();
    
    static void initializeAllocator();

    bool isValid() const;

    static bool underMemoryPressure();
    
    static double memoryPressureMultiplier(size_t addedMemoryUsage);
    
#if ENABLE(META_ALLOCATOR_PROFILE)
    static void dumpProfile();
#else
    static void dumpProfile() { }
#endif

    PassRefPtr<ExecutableMemoryHandle> allocate(VM&, size_t sizeInBytes, void* ownerUID, JITCompilationEffort);

#if ENABLE(ASSEMBLER_WX_EXCLUSIVE)
    static void makeWritable(void* start, size_t size)
    {
        reprotectRegion(start, size, Writable);
    }

    static void makeExecutable(void* start, size_t size)
    {
        reprotectRegion(start, size, Executable);
    }
#else
    static void makeWritable(void*, size_t) {}
    static void makeExecutable(void*, size_t) {}
#endif

    static size_t committedByteCount();

private:

#if ENABLE(ASSEMBLER_WX_EXCLUSIVE)
    static void reprotectRegion(void*, size_t, ProtectionSetting);
#if ENABLE(EXECUTABLE_ALLOCATOR_DEMAND)
    // We create a MetaAllocator for each JS global object.
    OwnPtr<DemandExecutableAllocator> m_allocator;
    DemandExecutableAllocator* allocator() { return m_allocator.get(); }
#endif
#endif

};

#endif // ENABLE(JIT) && ENABLE(ASSEMBLER)

} // namespace JSC

#endif // !defined(ExecutableAllocator)
