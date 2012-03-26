// Copyright (c) 2005, 2007, Google Inc.
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
// 
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// ---
// Author: Sanjay Ghemawat

#include "config.h"
#if !(defined(USE_SYSTEM_MALLOC) && USE_SYSTEM_MALLOC)
#include "TCSystemAlloc.h"

#include <algorithm>
#include "Assertions.h"
#include "TCSpinLock.h"
#include "UnusedParam.h"
#include "VMTags.h"

#if HAVE(STDINT_H)
#include <stdint.h>
#elif HAVE(INTTYPES_H)
#include <inttypes.h>
#else
#include <sys/types.h>
#endif

#if OS(WINDOWS)
#include "windows.h"
#else
#include <errno.h>
#include <unistd.h>
#include <sys/mman.h>
#endif

#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

using namespace std;

// Structure for discovering alignment
union MemoryAligner {
  void*  p;
  double d;
  size_t s;
};

static SpinLock spinlock = SPINLOCK_INITIALIZER;

// Page size is initialized on demand
static size_t pagesize = 0;

// Configuration parameters.
//
// if use_devmem is true, either use_sbrk or use_mmap must also be true.
// For 2.2 kernels, it looks like the sbrk address space (500MBish) and
// the mmap address space (1300MBish) are disjoint, so we need both allocators
// to get as much virtual memory as possible.
#ifndef WTF_CHANGES
static bool use_devmem = false;
#endif

#if HAVE(SBRK)
static bool use_sbrk = false;
#endif

#if HAVE(MMAP)
static bool use_mmap = true;
#endif 

#if HAVE(VIRTUALALLOC)
static bool use_VirtualAlloc = true;
#endif

// Flags to keep us from retrying allocators that failed.
static bool devmem_failure = false;
static bool sbrk_failure = false;
static bool mmap_failure = false;
static bool VirtualAlloc_failure = false;

#ifndef WTF_CHANGES
DEFINE_int32(malloc_devmem_start, 0,
             "Physical memory starting location in MB for /dev/mem allocation."
             "  Setting this to 0 disables /dev/mem allocation");
DEFINE_int32(malloc_devmem_limit, 0,
             "Physical memory limit location in MB for /dev/mem allocation."
             "  Setting this to 0 means no limit.");
#else
static const int32_t FLAGS_malloc_devmem_start = 0;
static const int32_t FLAGS_malloc_devmem_limit = 0;
#endif

#if HAVE(SBRK)

static void* TrySbrk(size_t size, size_t *actual_size, size_t alignment) {
  size = ((size + alignment - 1) / alignment) * alignment;
  
  // could theoretically return the "extra" bytes here, but this
  // is simple and correct.
  if (actual_size) 
    *actual_size = size;
    
  void* result = sbrk(size);
  if (result == reinterpret_cast<void*>(-1)) {
    sbrk_failure = true;
    return NULL;
  }

  // Is it aligned?
  uintptr_t ptr = reinterpret_cast<uintptr_t>(result);
  if ((ptr & (alignment-1)) == 0)  return result;

  // Try to get more memory for alignment
  size_t extra = alignment - (ptr & (alignment-1));
  void* r2 = sbrk(extra);
  if (reinterpret_cast<uintptr_t>(r2) == (ptr + size)) {
    // Contiguous with previous result
    return reinterpret_cast<void*>(ptr + extra);
  }

  // Give up and ask for "size + alignment - 1" bytes so
  // that we can find an aligned region within it.
  result = sbrk(size + alignment - 1);
  if (result == reinterpret_cast<void*>(-1)) {
    sbrk_failure = true;
    return NULL;
  }
  ptr = reinterpret_cast<uintptr_t>(result);
  if ((ptr & (alignment-1)) != 0) {
    ptr += alignment - (ptr & (alignment-1));
  }
  return reinterpret_cast<void*>(ptr);
}

#endif /* HAVE(SBRK) */

#if HAVE(MMAP)

static void* TryMmap(size_t size, size_t *actual_size, size_t alignment) {
  // Enforce page alignment
  if (pagesize == 0) pagesize = getpagesize();
  if (alignment < pagesize) alignment = pagesize;
  size = ((size + alignment - 1) / alignment) * alignment;
  
  // could theoretically return the "extra" bytes here, but this
  // is simple and correct.
  if (actual_size) 
    *actual_size = size;
    
  // Ask for extra memory if alignment > pagesize
  size_t extra = 0;
  if (alignment > pagesize) {
    extra = alignment - pagesize;
  }
  void* result = mmap(NULL, size + extra,
                      PROT_READ | PROT_WRITE,
                      MAP_PRIVATE|MAP_ANONYMOUS,
                      VM_TAG_FOR_TCMALLOC_MEMORY, 0);
  if (result == reinterpret_cast<void*>(MAP_FAILED)) {
    mmap_failure = true;
    return NULL;
  }

  // Adjust the return memory so it is aligned
  uintptr_t ptr = reinterpret_cast<uintptr_t>(result);
  size_t adjust = 0;
  if ((ptr & (alignment - 1)) != 0) {
    adjust = alignment - (ptr & (alignment - 1));
  }

  // Return the unused memory to the system
  if (adjust > 0) {
    munmap(reinterpret_cast<void*>(ptr), adjust);
  }
  if (adjust < extra) {
    munmap(reinterpret_cast<void*>(ptr + adjust + size), extra - adjust);
  }

  ptr += adjust;
  return reinterpret_cast<void*>(ptr);
}

#endif /* HAVE(MMAP) */

#if HAVE(VIRTUALALLOC)

static void* TryVirtualAlloc(size_t size, size_t *actual_size, size_t alignment) {
  // Enforce page alignment
  if (pagesize == 0) {
    SYSTEM_INFO system_info;
    GetSystemInfo(&system_info);
    pagesize = system_info.dwPageSize;
  }

  if (alignment < pagesize) alignment = pagesize;
  size = ((size + alignment - 1) / alignment) * alignment;

  // could theoretically return the "extra" bytes here, but this
  // is simple and correct.
  if (actual_size) 
    *actual_size = size;
    
  // Ask for extra memory if alignment > pagesize
  size_t extra = 0;
  if (alignment > pagesize) {
    extra = alignment - pagesize;
  }
  void* result = VirtualAlloc(NULL, size + extra,
                              MEM_RESERVE | MEM_COMMIT | MEM_TOP_DOWN, 
                              PAGE_READWRITE);

  if (result == NULL) {
    VirtualAlloc_failure = true;
    return NULL;
  }

  // Adjust the return memory so it is aligned
  uintptr_t ptr = reinterpret_cast<uintptr_t>(result);
  size_t adjust = 0;
  if ((ptr & (alignment - 1)) != 0) {
    adjust = alignment - (ptr & (alignment - 1));
  }

  // Return the unused memory to the system - we'd like to release but the best we can do
  // is decommit, since Windows only lets you free the whole allocation.
  if (adjust > 0) {
    VirtualFree(reinterpret_cast<void*>(ptr), adjust, MEM_DECOMMIT);
  }
  if (adjust < extra) {
    VirtualFree(reinterpret_cast<void*>(ptr + adjust + size), extra-adjust, MEM_DECOMMIT);
  }

  ptr += adjust;
  return reinterpret_cast<void*>(ptr);
}

#endif /* HAVE(MMAP) */

#ifndef WTF_CHANGES
static void* TryDevMem(size_t size, size_t *actual_size, size_t alignment) {
  static bool initialized = false;
  static off_t physmem_base;  // next physical memory address to allocate
  static off_t physmem_limit; // maximum physical address allowed
  static int physmem_fd;      // file descriptor for /dev/mem
  
  // Check if we should use /dev/mem allocation.  Note that it may take
  // a while to get this flag initialized, so meanwhile we fall back to
  // the next allocator.  (It looks like 7MB gets allocated before
  // this flag gets initialized -khr.)
  if (FLAGS_malloc_devmem_start == 0) {
    // NOTE: not a devmem_failure - we'd like TCMalloc_SystemAlloc to
    // try us again next time.
    return NULL;
  }
  
  if (!initialized) {
    physmem_fd = open("/dev/mem", O_RDWR);
    if (physmem_fd < 0) {
      devmem_failure = true;
      return NULL;
    }
    physmem_base = FLAGS_malloc_devmem_start*1024LL*1024LL;
    physmem_limit = FLAGS_malloc_devmem_limit*1024LL*1024LL;
    initialized = true;
  }
  
  // Enforce page alignment
  if (pagesize == 0) pagesize = getpagesize();
  if (alignment < pagesize) alignment = pagesize;
  size = ((size + alignment - 1) / alignment) * alignment;
    
  // could theoretically return the "extra" bytes here, but this
  // is simple and correct.
  if (actual_size)
    *actual_size = size;
    
  // Ask for extra memory if alignment > pagesize
  size_t extra = 0;
  if (alignment > pagesize) {
    extra = alignment - pagesize;
  }
  
  // check to see if we have any memory left
  if (physmem_limit != 0 && physmem_base + size + extra > physmem_limit) {
    devmem_failure = true;
    return NULL;
  }
  void *result = mmap(0, size + extra, PROT_READ | PROT_WRITE,
                      MAP_SHARED, physmem_fd, physmem_base);
  if (result == reinterpret_cast<void*>(MAP_FAILED)) {
    devmem_failure = true;
    return NULL;
  }
  uintptr_t ptr = reinterpret_cast<uintptr_t>(result);
  
  // Adjust the return memory so it is aligned
  size_t adjust = 0;
  if ((ptr & (alignment - 1)) != 0) {
    adjust = alignment - (ptr & (alignment - 1));
  }
  
  // Return the unused virtual memory to the system
  if (adjust > 0) {
    munmap(reinterpret_cast<void*>(ptr), adjust);
  }
  if (adjust < extra) {
    munmap(reinterpret_cast<void*>(ptr + adjust + size), extra - adjust);
  }
  
  ptr += adjust;
  physmem_base += adjust + size;
  
  return reinterpret_cast<void*>(ptr);
}
#endif

void* TCMalloc_SystemAlloc(size_t size, size_t *actual_size, size_t alignment) {
  // Discard requests that overflow
  if (size + alignment < size) return NULL;
    
  SpinLockHolder lock_holder(&spinlock);

  // Enforce minimum alignment
  if (alignment < sizeof(MemoryAligner)) alignment = sizeof(MemoryAligner);

  // Try twice, once avoiding allocators that failed before, and once
  // more trying all allocators even if they failed before.
  for (int i = 0; i < 2; i++) {

#ifndef WTF_CHANGES
    if (use_devmem && !devmem_failure) {
      void* result = TryDevMem(size, actual_size, alignment);
      if (result != NULL) return result;
    }
#endif
    
#if HAVE(SBRK)
    if (use_sbrk && !sbrk_failure) {
      void* result = TrySbrk(size, actual_size, alignment);
      if (result != NULL) return result;
    }
#endif

#if HAVE(MMAP)    
    if (use_mmap && !mmap_failure) {
      void* result = TryMmap(size, actual_size, alignment);
      if (result != NULL) return result;
    }
#endif

#if HAVE(VIRTUALALLOC)
    if (use_VirtualAlloc && !VirtualAlloc_failure) {
      void* result = TryVirtualAlloc(size, actual_size, alignment);
      if (result != NULL) return result;
    }
#endif

    // nothing worked - reset failure flags and try again
    devmem_failure = false;
    sbrk_failure = false;
    mmap_failure = false;
    VirtualAlloc_failure = false;
  }
  return NULL;
}

#if HAVE(MADV_FREE_REUSE)

void TCMalloc_SystemRelease(void* start, size_t length)
{
    while (madvise(start, length, MADV_FREE_REUSABLE) == -1 && errno == EAGAIN) { }
}

#elif HAVE(MADV_FREE) || HAVE(MADV_DONTNEED)

void TCMalloc_SystemRelease(void* start, size_t length)
{
    // MADV_FREE clears the modified bit on pages, which allows
    // them to be discarded immediately.
#if HAVE(MADV_FREE)
    const int advice = MADV_FREE;
#else
    const int advice = MADV_DONTNEED;
#endif
  if (FLAGS_malloc_devmem_start) {
    // It's not safe to use MADV_DONTNEED if we've been mapping
    // /dev/mem for heap memory
    return;
  }
  if (pagesize == 0) pagesize = getpagesize();
  const size_t pagemask = pagesize - 1;

  size_t new_start = reinterpret_cast<size_t>(start);
  size_t end = new_start + length;
  size_t new_end = end;

  // Round up the starting address and round down the ending address
  // to be page aligned:
  new_start = (new_start + pagesize - 1) & ~pagemask;
  new_end = new_end & ~pagemask;

  ASSERT((new_start & pagemask) == 0);
  ASSERT((new_end & pagemask) == 0);
  ASSERT(new_start >= reinterpret_cast<size_t>(start));
  ASSERT(new_end <= end);

  if (new_end > new_start) {
    // Note -- ignoring most return codes, because if this fails it
    // doesn't matter...
    while (madvise(reinterpret_cast<char*>(new_start), new_end - new_start,
                   advice) == -1 &&
           errno == EAGAIN) {
      // NOP
    }
  }
}

#elif HAVE(MMAP)

void TCMalloc_SystemRelease(void* start, size_t length)
{
  void* newAddress = mmap(start, length, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
  // If the mmap failed then that's ok, we just won't return the memory to the system.
  ASSERT_UNUSED(newAddress, newAddress == start || newAddress == reinterpret_cast<void*>(MAP_FAILED));
}

#elif HAVE(VIRTUALALLOC)

void TCMalloc_SystemRelease(void* start, size_t length)
{
    if (VirtualFree(start, length, MEM_DECOMMIT))
        return;

    // The decommit may fail if the memory region consists of allocations
    // from more than one call to VirtualAlloc.  In this case, fall back to
    // using VirtualQuery to retrieve the allocation boundaries and decommit
    // them each individually.

    char* ptr = static_cast<char*>(start);
    char* end = ptr + length;
    MEMORY_BASIC_INFORMATION info;
    while (ptr < end) {
        size_t resultSize = VirtualQuery(ptr, &info, sizeof(info));
        ASSERT_UNUSED(resultSize, resultSize == sizeof(info));

        size_t decommitSize = min<size_t>(info.RegionSize, end - ptr);
        BOOL success = VirtualFree(ptr, decommitSize, MEM_DECOMMIT);
        ASSERT_UNUSED(success, success);
        ptr += decommitSize;
    }
}

#else

// Platforms that don't support returning memory use an empty inline version of TCMalloc_SystemRelease
// declared in TCSystemAlloc.h

#endif

#if HAVE(MADV_FREE_REUSE)

void TCMalloc_SystemCommit(void* start, size_t length)
{
    while (madvise(start, length, MADV_FREE_REUSE) == -1 && errno == EAGAIN) { }
}

#elif HAVE(VIRTUALALLOC)

void TCMalloc_SystemCommit(void* start, size_t length)
{
    if (VirtualAlloc(start, length, MEM_COMMIT, PAGE_READWRITE) == start)
        return;

    // The commit may fail if the memory region consists of allocations
    // from more than one call to VirtualAlloc.  In this case, fall back to
    // using VirtualQuery to retrieve the allocation boundaries and commit them
    // each individually.

    char* ptr = static_cast<char*>(start);
    char* end = ptr + length;
    MEMORY_BASIC_INFORMATION info;
    while (ptr < end) {
        size_t resultSize = VirtualQuery(ptr, &info, sizeof(info));
        ASSERT_UNUSED(resultSize, resultSize == sizeof(info));

        size_t commitSize = min<size_t>(info.RegionSize, end - ptr);
        void* newAddress = VirtualAlloc(ptr, commitSize, MEM_COMMIT, PAGE_READWRITE);
        ASSERT_UNUSED(newAddress, newAddress == ptr);
        ptr += commitSize;
    }
}

#else

// Platforms that don't need to explicitly commit memory use an empty inline version of TCMalloc_SystemCommit
// declared in TCSystemAlloc.h

#endif

#endif // #if !(defined(USE_SYSTEM_MALLOC) && USE_SYSTEM_MALLOC)

