// Copyright (c) 2005, 2007, Google Inc.
// All rights reserved.
// Copyright (C) 2005, 2006, 2007, 2008, 2009, 2011 Apple Inc. All rights reserved.
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
// Author: Sanjay Ghemawat <opensource@google.com>
//
// A malloc that uses a per-thread cache to satisfy small malloc requests.
// (The time for malloc/free of a small object drops from 300 ns to 50 ns.)
//
// See doc/tcmalloc.html for a high-level
// description of how this malloc works.
//
// SYNCHRONIZATION
//  1. The thread-specific lists are accessed without acquiring any locks.
//     This is safe because each such list is only accessed by one thread.
//  2. We have a lock per central free-list, and hold it while manipulating
//     the central free list for a particular size.
//  3. The central page allocator is protected by "pageheap_lock".
//  4. The pagemap (which maps from page-number to descriptor),
//     can be read without holding any locks, and written while holding
//     the "pageheap_lock".
//  5. To improve performance, a subset of the information one can get
//     from the pagemap is cached in a data structure, pagemap_cache_,
//     that atomically reads and writes its entries.  This cache can be
//     read and written without locking.
//
//     This multi-threaded access to the pagemap is safe for fairly
//     subtle reasons.  We basically assume that when an object X is
//     allocated by thread A and deallocated by thread B, there must
//     have been appropriate synchronization in the handoff of object
//     X from thread A to thread B.  The same logic applies to pagemap_cache_.
//
// THE PAGEID-TO-SIZECLASS CACHE
// Hot PageID-to-sizeclass mappings are held by pagemap_cache_.  If this cache
// returns 0 for a particular PageID then that means "no information," not that
// the sizeclass is 0.  The cache may have stale information for pages that do
// not hold the beginning of any free()'able object.  Staleness is eliminated
// in Populate() for pages with sizeclass > 0 objects, and in do_malloc() and
// do_memalign() for all other relevant pages.
//
// TODO: Bias reclamation to larger addresses
// TODO: implement mallinfo/mallopt
// TODO: Better testing
//
// 9/28/2003 (new page-level allocator replaces ptmalloc2):
// * malloc/free of small objects goes from ~300 ns to ~50 ns.
// * allocation of a reasonably complicated struct
//   goes from about 1100 ns to about 300 ns.

#include "config.h"
#include "FastMalloc.h"

#include "Assertions.h"
#include <limits>
#if ENABLE(WTF_MULTIPLE_THREADS)
#include <pthread.h>
#endif
#include <wtf/StdLibExtras.h>

#ifndef NO_TCMALLOC_SAMPLES
#ifdef WTF_CHANGES
#define NO_TCMALLOC_SAMPLES
#endif
#endif

#if !(defined(USE_SYSTEM_MALLOC) && USE_SYSTEM_MALLOC) && defined(NDEBUG)
#define FORCE_SYSTEM_MALLOC 0
#else
#define FORCE_SYSTEM_MALLOC 1
#endif

// Use a background thread to periodically scavenge memory to release back to the system
#define USE_BACKGROUND_THREAD_TO_SCAVENGE_MEMORY 1

#ifndef NDEBUG
namespace WTF {

#if ENABLE(WTF_MULTIPLE_THREADS)
static pthread_key_t isForbiddenKey;
static pthread_once_t isForbiddenKeyOnce = PTHREAD_ONCE_INIT;
static void initializeIsForbiddenKey()
{
  pthread_key_create(&isForbiddenKey, 0);
}

#if !ASSERT_DISABLED
static bool isForbidden()
{
    pthread_once(&isForbiddenKeyOnce, initializeIsForbiddenKey);
    return !!pthread_getspecific(isForbiddenKey);
}
#endif

void fastMallocForbid()
{
    pthread_once(&isForbiddenKeyOnce, initializeIsForbiddenKey);
    pthread_setspecific(isForbiddenKey, &isForbiddenKey);
}

void fastMallocAllow()
{
    pthread_once(&isForbiddenKeyOnce, initializeIsForbiddenKey);
    pthread_setspecific(isForbiddenKey, 0);
}

#else

static bool staticIsForbidden;
static bool isForbidden()
{
    return staticIsForbidden;
}

void fastMallocForbid()
{
    staticIsForbidden = true;
}

void fastMallocAllow()
{
    staticIsForbidden = false;
}
#endif // ENABLE(WTF_MULTIPLE_THREADS)

} // namespace WTF
#endif // NDEBUG

#include <string.h>

namespace WTF {


namespace Internal {
#if !ENABLE(WTF_MALLOC_VALIDATION)
void fastMallocMatchFailed(void*);
#else
COMPILE_ASSERT(((sizeof(ValidationHeader) % sizeof(AllocAlignmentInteger)) == 0), ValidationHeader_must_produce_correct_alignment);
#endif
void fastMallocMatchFailed(void*)
{
    CRASH();
}

} // namespace Internal


void* fastZeroedMalloc(size_t n) 
{
    void* result = fastMalloc(n);
    memset(result, 0, n);
    return result;
}

char* fastStrDup(const char* src)
{
    size_t len = strlen(src) + 1;
    char* dup = static_cast<char*>(fastMalloc(len));
    memcpy(dup, src, len);
    return dup;
}

TryMallocReturnValue tryFastZeroedMalloc(size_t n) 
{
    void* result;
    if (!tryFastMalloc(n).getValue(result))
        return 0;
    memset(result, 0, n);
    return result;
}

} // namespace WTF

#if FORCE_SYSTEM_MALLOC

#if PLATFORM(BREWMP)
#include "brew/SystemMallocBrew.h"
#endif

#if OS(DARWIN)
#include <malloc/malloc.h>
#elif OS(WINDOWS)
#include <malloc.h>
#endif

namespace WTF {

TryMallocReturnValue tryFastMalloc(size_t n) 
{
    ASSERT(!isForbidden());

#if ENABLE(WTF_MALLOC_VALIDATION)
    if (std::numeric_limits<size_t>::max() - Internal::ValidationBufferSize <= n)  // If overflow would occur...
        return 0;

    void* result = malloc(n + Internal::ValidationBufferSize);
    if (!result)
        return 0;
    Internal::ValidationHeader* header = static_cast<Internal::ValidationHeader*>(result);
    header->m_size = n;
    header->m_type = Internal::AllocTypeMalloc;
    header->m_prefix = static_cast<unsigned>(Internal::ValidationPrefix);
    result = header + 1;
    *Internal::fastMallocValidationSuffix(result) = Internal::ValidationSuffix;
    fastMallocValidate(result);
    return result;
#else
    return malloc(n);
#endif
}

void* fastMalloc(size_t n) 
{
    ASSERT(!isForbidden());

#if ENABLE(WTF_MALLOC_VALIDATION)
    TryMallocReturnValue returnValue = tryFastMalloc(n);
    void* result;
    if (!returnValue.getValue(result))
        CRASH();
#else
    void* result = malloc(n);
#endif

    if (!result) {
#if PLATFORM(BREWMP)
        // The behavior of malloc(0) is implementation defined.
        // To make sure that fastMalloc never returns 0, retry with fastMalloc(1).
        if (!n)
            return fastMalloc(1);
#endif
        CRASH();
    }

    return result;
}

TryMallocReturnValue tryFastCalloc(size_t n_elements, size_t element_size)
{
    ASSERT(!isForbidden());

#if ENABLE(WTF_MALLOC_VALIDATION)
    size_t totalBytes = n_elements * element_size;
    if (n_elements > 1 && element_size && (totalBytes / element_size) != n_elements)
        return 0;

    TryMallocReturnValue returnValue = tryFastMalloc(totalBytes);
    void* result;
    if (!returnValue.getValue(result))
        return 0;
    memset(result, 0, totalBytes);
    fastMallocValidate(result);
    return result;
#else
    return calloc(n_elements, element_size);
#endif
}

void* fastCalloc(size_t n_elements, size_t element_size)
{
    ASSERT(!isForbidden());

#if ENABLE(WTF_MALLOC_VALIDATION)
    TryMallocReturnValue returnValue = tryFastCalloc(n_elements, element_size);
    void* result;
    if (!returnValue.getValue(result))
        CRASH();
#else
    void* result = calloc(n_elements, element_size);
#endif

    if (!result) {
#if PLATFORM(BREWMP)
        // If either n_elements or element_size is 0, the behavior of calloc is implementation defined.
        // To make sure that fastCalloc never returns 0, retry with fastCalloc(1, 1).
        if (!n_elements || !element_size)
            return fastCalloc(1, 1);
#endif
        CRASH();
    }

    return result;
}

void fastFree(void* p)
{
    ASSERT(!isForbidden());

#if ENABLE(WTF_MALLOC_VALIDATION)
    if (!p)
        return;
    
    fastMallocMatchValidateFree(p, Internal::AllocTypeMalloc);
    Internal::ValidationHeader* header = Internal::fastMallocValidationHeader(p);
    memset(p, 0xCC, header->m_size);
    free(header);
#else
    free(p);
#endif
}

TryMallocReturnValue tryFastRealloc(void* p, size_t n)
{
    ASSERT(!isForbidden());

#if ENABLE(WTF_MALLOC_VALIDATION)
    if (p) {
        if (std::numeric_limits<size_t>::max() - Internal::ValidationBufferSize <= n)  // If overflow would occur...
            return 0;
        fastMallocValidate(p);
        Internal::ValidationHeader* result = static_cast<Internal::ValidationHeader*>(realloc(Internal::fastMallocValidationHeader(p), n + Internal::ValidationBufferSize));
        if (!result)
            return 0;
        result->m_size = n;
        result = result + 1;
        *fastMallocValidationSuffix(result) = Internal::ValidationSuffix;
        fastMallocValidate(result);
        return result;
    } else {
        return fastMalloc(n);
    }
#else
    return realloc(p, n);
#endif
}

void* fastRealloc(void* p, size_t n)
{
    ASSERT(!isForbidden());

#if ENABLE(WTF_MALLOC_VALIDATION)
    TryMallocReturnValue returnValue = tryFastRealloc(p, n);
    void* result;
    if (!returnValue.getValue(result))
        CRASH();
#else
    void* result = realloc(p, n);
#endif

    if (!result)
        CRASH();
    return result;
}

void releaseFastMallocFreeMemory() { }
    
FastMallocStatistics fastMallocStatistics()
{
    FastMallocStatistics statistics = { 0, 0, 0 };
    return statistics;
}

size_t fastMallocSize(const void* p)
{
#if ENABLE(WTF_MALLOC_VALIDATION)
    return Internal::fastMallocValidationHeader(const_cast<void*>(p))->m_size;
#elif OS(DARWIN)
    return malloc_size(p);
#elif OS(WINDOWS) && !PLATFORM(BREWMP)
    // Brew MP uses its own memory allocator, so _msize does not work on the Brew MP simulator.
    return _msize(const_cast<void*>(p));
#else
    return 1;
#endif
}

} // namespace WTF

#if OS(DARWIN)
// This symbol is present in the JavaScriptCore exports file even when FastMalloc is disabled.
// It will never be used in this case, so it's type and value are less interesting than its presence.
extern "C" const int jscore_fastmalloc_introspection = 0;
#endif

#else // FORCE_SYSTEM_MALLOC

#if HAVE(STDINT_H)
#include <stdint.h>
#elif HAVE(INTTYPES_H)
#include <inttypes.h>
#else
#include <sys/types.h>
#endif

#include "AlwaysInline.h"
#include "Assertions.h"
#include "TCPackedCache.h"
#include "TCPageMap.h"
#include "TCSpinLock.h"
#include "TCSystemAlloc.h"
#include <algorithm>
#include <limits>
#include <pthread.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#if HAVE(ERRNO_H)
#include <errno.h>
#endif
#if OS(UNIX)
#include <unistd.h>
#endif
#if OS(WINDOWS)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#ifdef WTF_CHANGES

#if OS(DARWIN)
#include "MallocZoneSupport.h"
#include <wtf/HashSet.h>
#include <wtf/Vector.h>
#endif

#if HAVE(HEADER_DETECTION_H)
#include "HeaderDetection.h"
#endif

#if HAVE(DISPATCH_H)
#include <dispatch/dispatch.h>
#endif

#if HAVE(PTHREAD_MACHDEP_H)
#include <System/pthread_machdep.h>

#if defined(__PTK_FRAMEWORK_JAVASCRIPTCORE_KEY0)
#define WTF_USE_PTHREAD_GETSPECIFIC_DIRECT 1
#endif
#endif

#ifndef PRIuS
#define PRIuS "zu"
#endif

// Calling pthread_getspecific through a global function pointer is faster than a normal
// call to the function on Mac OS X, and it's used in performance-critical code. So we
// use a function pointer. But that's not necessarily faster on other platforms, and we had
// problems with this technique on Windows, so we'll do this only on Mac OS X.
#if OS(DARWIN)
#if !USE(PTHREAD_GETSPECIFIC_DIRECT)
static void* (*pthread_getspecific_function_pointer)(pthread_key_t) = pthread_getspecific;
#define pthread_getspecific(key) pthread_getspecific_function_pointer(key)
#else
#define pthread_getspecific(key) _pthread_getspecific_direct(key)
#define pthread_setspecific(key, val) _pthread_setspecific_direct(key, (val))
#endif
#endif

#define DEFINE_VARIABLE(type, name, value, meaning) \
  namespace FLAG__namespace_do_not_use_directly_use_DECLARE_##type##_instead {  \
  type FLAGS_##name(value);                                \
  char FLAGS_no##name;                                                        \
  }                                                                           \
  using FLAG__namespace_do_not_use_directly_use_DECLARE_##type##_instead::FLAGS_##name
  
#define DEFINE_int64(name, value, meaning) \
  DEFINE_VARIABLE(int64_t, name, value, meaning)
  
#define DEFINE_double(name, value, meaning) \
  DEFINE_VARIABLE(double, name, value, meaning)

namespace WTF {

#define malloc fastMalloc
#define calloc fastCalloc
#define free fastFree
#define realloc fastRealloc

#define MESSAGE LOG_ERROR
#define CHECK_CONDITION ASSERT

#if OS(DARWIN)
struct Span;
class TCMalloc_Central_FreeListPadded;
class TCMalloc_PageHeap;
class TCMalloc_ThreadCache;
template <typename T> class PageHeapAllocator;

class FastMallocZone {
public:
    static void init();

    static kern_return_t enumerate(task_t, void*, unsigned typeMmask, vm_address_t zoneAddress, memory_reader_t, vm_range_recorder_t);
    static size_t goodSize(malloc_zone_t*, size_t size) { return size; }
    static boolean_t check(malloc_zone_t*) { return true; }
    static void  print(malloc_zone_t*, boolean_t) { }
    static void log(malloc_zone_t*, void*) { }
    static void forceLock(malloc_zone_t*) { }
    static void forceUnlock(malloc_zone_t*) { }
    static void statistics(malloc_zone_t*, malloc_statistics_t* stats) { memset(stats, 0, sizeof(malloc_statistics_t)); }

private:
    FastMallocZone(TCMalloc_PageHeap*, TCMalloc_ThreadCache**, TCMalloc_Central_FreeListPadded*, PageHeapAllocator<Span>*, PageHeapAllocator<TCMalloc_ThreadCache>*);
    static size_t size(malloc_zone_t*, const void*);
    static void* zoneMalloc(malloc_zone_t*, size_t);
    static void* zoneCalloc(malloc_zone_t*, size_t numItems, size_t size);
    static void zoneFree(malloc_zone_t*, void*);
    static void* zoneRealloc(malloc_zone_t*, void*, size_t);
    static void* zoneValloc(malloc_zone_t*, size_t) { LOG_ERROR("valloc is not supported"); return 0; }
    static void zoneDestroy(malloc_zone_t*) { }

    malloc_zone_t m_zone;
    TCMalloc_PageHeap* m_pageHeap;
    TCMalloc_ThreadCache** m_threadHeaps;
    TCMalloc_Central_FreeListPadded* m_centralCaches;
    PageHeapAllocator<Span>* m_spanAllocator;
    PageHeapAllocator<TCMalloc_ThreadCache>* m_pageHeapAllocator;
};

#endif

#endif

#ifndef WTF_CHANGES
// This #ifdef should almost never be set.  Set NO_TCMALLOC_SAMPLES if
// you're porting to a system where you really can't get a stacktrace.
#ifdef NO_TCMALLOC_SAMPLES
// We use #define so code compiles even if you #include stacktrace.h somehow.
# define GetStackTrace(stack, depth, skip)  (0)
#else
# include <google/stacktrace.h>
#endif
#endif

// Even if we have support for thread-local storage in the compiler
// and linker, the OS may not support it.  We need to check that at
// runtime.  Right now, we have to keep a manual set of "bad" OSes.
#if defined(HAVE_TLS)
  static bool kernel_supports_tls = false;      // be conservative
  static inline bool KernelSupportsTLS() {
    return kernel_supports_tls;
  }
# if !HAVE_DECL_UNAME   // if too old for uname, probably too old for TLS
    static void CheckIfKernelSupportsTLS() {
      kernel_supports_tls = false;
    }
# else
#   include <sys/utsname.h>    // DECL_UNAME checked for <sys/utsname.h> too
    static void CheckIfKernelSupportsTLS() {
      struct utsname buf;
      if (uname(&buf) != 0) {   // should be impossible
        MESSAGE("uname failed assuming no TLS support (errno=%d)\n", errno);
        kernel_supports_tls = false;
      } else if (strcasecmp(buf.sysname, "linux") == 0) {
        // The linux case: the first kernel to support TLS was 2.6.0
        if (buf.release[0] < '2' && buf.release[1] == '.')    // 0.x or 1.x
          kernel_supports_tls = false;
        else if (buf.release[0] == '2' && buf.release[1] == '.' &&
                 buf.release[2] >= '0' && buf.release[2] < '6' &&
                 buf.release[3] == '.')                       // 2.0 - 2.5
          kernel_supports_tls = false;
        else
          kernel_supports_tls = true;
      } else {        // some other kernel, we'll be optimisitic
        kernel_supports_tls = true;
      }
      // TODO(csilvers): VLOG(1) the tls status once we support RAW_VLOG
    }
#  endif  // HAVE_DECL_UNAME
#endif    // HAVE_TLS

// __THROW is defined in glibc systems.  It means, counter-intuitively,
// "This function will never throw an exception."  It's an optional
// optimization tool, but we may need to use it to match glibc prototypes.
#ifndef __THROW    // I guess we're not on a glibc system
# define __THROW   // __THROW is just an optimization, so ok to make it ""
#endif

//-------------------------------------------------------------------
// Configuration
//-------------------------------------------------------------------

// Not all possible combinations of the following parameters make
// sense.  In particular, if kMaxSize increases, you may have to
// increase kNumClasses as well.
static const size_t kPageShift  = 12;
static const size_t kPageSize   = 1 << kPageShift;
static const size_t kMaxSize    = 8u * kPageSize;
static const size_t kAlignShift = 3;
static const size_t kAlignment  = 1 << kAlignShift;
static const size_t kNumClasses = 68;

// Allocates a big block of memory for the pagemap once we reach more than
// 128MB
static const size_t kPageMapBigAllocationThreshold = 128 << 20;

// Minimum number of pages to fetch from system at a time.  Must be
// significantly bigger than kPageSize to amortize system-call
// overhead, and also to reduce external fragementation.  Also, we
// should keep this value big because various incarnations of Linux
// have small limits on the number of mmap() regions per
// address-space.
static const size_t kMinSystemAlloc = 1 << (20 - kPageShift);

// Number of objects to move between a per-thread list and a central
// list in one shot.  We want this to be not too small so we can
// amortize the lock overhead for accessing the central list.  Making
// it too big may temporarily cause unnecessary memory wastage in the
// per-thread free list until the scavenger cleans up the list.
static int num_objects_to_move[kNumClasses];

// Maximum length we allow a per-thread free-list to have before we
// move objects from it into the corresponding central free-list.  We
// want this big to avoid locking the central free-list too often.  It
// should not hurt to make this list somewhat big because the
// scavenging code will shrink it down when its contents are not in use.
static const int kMaxFreeListLength = 256;

// Lower and upper bounds on the per-thread cache sizes
static const size_t kMinThreadCacheSize = kMaxSize * 2;
static const size_t kMaxThreadCacheSize = 2 << 20;

// Default bound on the total amount of thread caches
static const size_t kDefaultOverallThreadCacheSize = 16 << 20;

// For all span-lengths < kMaxPages we keep an exact-size list.
// REQUIRED: kMaxPages >= kMinSystemAlloc;
static const size_t kMaxPages = kMinSystemAlloc;

/* The smallest prime > 2^n */
static int primes_list[] = {
    // Small values might cause high rates of sampling
    // and hence commented out.
    // 2, 5, 11, 17, 37, 67, 131, 257,
    // 521, 1031, 2053, 4099, 8209, 16411,
    32771, 65537, 131101, 262147, 524309, 1048583,
    2097169, 4194319, 8388617, 16777259, 33554467 };

// Twice the approximate gap between sampling actions.
// I.e., we take one sample approximately once every
//      tcmalloc_sample_parameter/2
// bytes of allocation, i.e., ~ once every 128KB.
// Must be a prime number.
#ifdef NO_TCMALLOC_SAMPLES
DEFINE_int64(tcmalloc_sample_parameter, 0,
             "Unused: code is compiled with NO_TCMALLOC_SAMPLES");
static size_t sample_period = 0;
#else
DEFINE_int64(tcmalloc_sample_parameter, 262147,
         "Twice the approximate gap between sampling actions."
         " Must be a prime number. Otherwise will be rounded up to a "
         " larger prime number");
static size_t sample_period = 262147;
#endif

// Protects sample_period above
static SpinLock sample_period_lock = SPINLOCK_INITIALIZER;

// Parameters for controlling how fast memory is returned to the OS.

DEFINE_double(tcmalloc_release_rate, 1,
              "Rate at which we release unused memory to the system.  "
              "Zero means we never release memory back to the system.  "
              "Increase this flag to return memory faster; decrease it "
              "to return memory slower.  Reasonable rates are in the "
              "range [0,10]");

//-------------------------------------------------------------------
// Mapping from size to size_class and vice versa
//-------------------------------------------------------------------

// Sizes <= 1024 have an alignment >= 8.  So for such sizes we have an
// array indexed by ceil(size/8).  Sizes > 1024 have an alignment >= 128.
// So for these larger sizes we have an array indexed by ceil(size/128).
//
// We flatten both logical arrays into one physical array and use
// arithmetic to compute an appropriate index.  The constants used by
// ClassIndex() were selected to make the flattening work.
//
// Examples:
//   Size       Expression                      Index
//   -------------------------------------------------------
//   0          (0 + 7) / 8                     0
//   1          (1 + 7) / 8                     1
//   ...
//   1024       (1024 + 7) / 8                  128
//   1025       (1025 + 127 + (120<<7)) / 128   129
//   ...
//   32768      (32768 + 127 + (120<<7)) / 128  376
static const size_t kMaxSmallSize = 1024;
static const int shift_amount[2] = { 3, 7 };  // For divides by 8 or 128
static const int add_amount[2] = { 7, 127 + (120 << 7) };
static unsigned char class_array[377];

// Compute index of the class_array[] entry for a given size
static inline int ClassIndex(size_t s) {
  const int i = (s > kMaxSmallSize);
  return static_cast<int>((s + add_amount[i]) >> shift_amount[i]);
}

// Mapping from size class to max size storable in that class
static size_t class_to_size[kNumClasses];

// Mapping from size class to number of pages to allocate at a time
static size_t class_to_pages[kNumClasses];

// TransferCache is used to cache transfers of num_objects_to_move[size_class]
// back and forth between thread caches and the central cache for a given size
// class.
struct TCEntry {
  void *head;  // Head of chain of objects.
  void *tail;  // Tail of chain of objects.
};
// A central cache freelist can have anywhere from 0 to kNumTransferEntries
// slots to put link list chains into.  To keep memory usage bounded the total
// number of TCEntries across size classes is fixed.  Currently each size
// class is initially given one TCEntry which also means that the maximum any
// one class can have is kNumClasses.
static const int kNumTransferEntries = kNumClasses;

// Note: the following only works for "n"s that fit in 32-bits, but
// that is fine since we only use it for small sizes.
static inline int LgFloor(size_t n) {
  int log = 0;
  for (int i = 4; i >= 0; --i) {
    int shift = (1 << i);
    size_t x = n >> shift;
    if (x != 0) {
      n = x;
      log += shift;
    }
  }
  ASSERT(n == 1);
  return log;
}

// Some very basic linked list functions for dealing with using void * as
// storage.

static inline void *SLL_Next(void *t) {
  return *(reinterpret_cast<void**>(t));
}

static inline void SLL_SetNext(void *t, void *n) {
  *(reinterpret_cast<void**>(t)) = n;
}

static inline void SLL_Push(void **list, void *element) {
  SLL_SetNext(element, *list);
  *list = element;
}

static inline void *SLL_Pop(void **list) {
  void *result = *list;
  *list = SLL_Next(*list);
  return result;
}


// Remove N elements from a linked list to which head points.  head will be
// modified to point to the new head.  start and end will point to the first
// and last nodes of the range.  Note that end will point to NULL after this
// function is called.
static inline void SLL_PopRange(void **head, int N, void **start, void **end) {
  if (N == 0) {
    *start = NULL;
    *end = NULL;
    return;
  }

  void *tmp = *head;
  for (int i = 1; i < N; ++i) {
    tmp = SLL_Next(tmp);
  }

  *start = *head;
  *end = tmp;
  *head = SLL_Next(tmp);
  // Unlink range from list.
  SLL_SetNext(tmp, NULL);
}

static inline void SLL_PushRange(void **head, void *start, void *end) {
  if (!start) return;
  SLL_SetNext(end, *head);
  *head = start;
}

static inline size_t SLL_Size(void *head) {
  int count = 0;
  while (head) {
    count++;
    head = SLL_Next(head);
  }
  return count;
}

// Setup helper functions.

static ALWAYS_INLINE size_t SizeClass(size_t size) {
  return class_array[ClassIndex(size)];
}

// Get the byte-size for a specified class
static ALWAYS_INLINE size_t ByteSizeForClass(size_t cl) {
  return class_to_size[cl];
}
static int NumMoveSize(size_t size) {
  if (size == 0) return 0;
  // Use approx 64k transfers between thread and central caches.
  int num = static_cast<int>(64.0 * 1024.0 / size);
  if (num < 2) num = 2;
  // Clamp well below kMaxFreeListLength to avoid ping pong between central
  // and thread caches.
  if (num > static_cast<int>(0.8 * kMaxFreeListLength))
    num = static_cast<int>(0.8 * kMaxFreeListLength);

  // Also, avoid bringing in too many objects into small object free
  // lists.  There are lots of such lists, and if we allow each one to
  // fetch too many at a time, we end up having to scavenge too often
  // (especially when there are lots of threads and each thread gets a
  // small allowance for its thread cache).
  //
  // TODO: Make thread cache free list sizes dynamic so that we do not
  // have to equally divide a fixed resource amongst lots of threads.
  if (num > 32) num = 32;

  return num;
}

// Initialize the mapping arrays
static void InitSizeClasses() {
  // Do some sanity checking on add_amount[]/shift_amount[]/class_array[]
  if (ClassIndex(0) < 0) {
    MESSAGE("Invalid class index %d for size 0\n", ClassIndex(0));
    CRASH();
  }
  if (static_cast<size_t>(ClassIndex(kMaxSize)) >= sizeof(class_array)) {
    MESSAGE("Invalid class index %d for kMaxSize\n", ClassIndex(kMaxSize));
    CRASH();
  }

  // Compute the size classes we want to use
  size_t sc = 1;   // Next size class to assign
  unsigned char alignshift = kAlignShift;
  int last_lg = -1;
  for (size_t size = kAlignment; size <= kMaxSize; size += (1 << alignshift)) {
    int lg = LgFloor(size);
    if (lg > last_lg) {
      // Increase alignment every so often.
      //
      // Since we double the alignment every time size doubles and
      // size >= 128, this means that space wasted due to alignment is
      // at most 16/128 i.e., 12.5%.  Plus we cap the alignment at 256
      // bytes, so the space wasted as a percentage starts falling for
      // sizes > 2K.
      if ((lg >= 7) && (alignshift < 8)) {
        alignshift++;
      }
      last_lg = lg;
    }

    // Allocate enough pages so leftover is less than 1/8 of total.
    // This bounds wasted space to at most 12.5%.
    size_t psize = kPageSize;
    while ((psize % size) > (psize >> 3)) {
      psize += kPageSize;
    }
    const size_t my_pages = psize >> kPageShift;

    if (sc > 1 && my_pages == class_to_pages[sc-1]) {
      // See if we can merge this into the previous class without
      // increasing the fragmentation of the previous class.
      const size_t my_objects = (my_pages << kPageShift) / size;
      const size_t prev_objects = (class_to_pages[sc-1] << kPageShift)
                                  / class_to_size[sc-1];
      if (my_objects == prev_objects) {
        // Adjust last class to include this size
        class_to_size[sc-1] = size;
        continue;
      }
    }

    // Add new class
    class_to_pages[sc] = my_pages;
    class_to_size[sc] = size;
    sc++;
  }
  if (sc != kNumClasses) {
    MESSAGE("wrong number of size classes: found %" PRIuS " instead of %d\n",
            sc, int(kNumClasses));
    CRASH();
  }

  // Initialize the mapping arrays
  int next_size = 0;
  for (unsigned char c = 1; c < kNumClasses; c++) {
    const size_t max_size_in_class = class_to_size[c];
    for (size_t s = next_size; s <= max_size_in_class; s += kAlignment) {
      class_array[ClassIndex(s)] = c;
    }
    next_size = static_cast<int>(max_size_in_class + kAlignment);
  }

  // Double-check sizes just to be safe
  for (size_t size = 0; size <= kMaxSize; size++) {
    const size_t sc = SizeClass(size);
    if (sc == 0) {
      MESSAGE("Bad size class %" PRIuS " for %" PRIuS "\n", sc, size);
      CRASH();
    }
    if (sc > 1 && size <= class_to_size[sc-1]) {
      MESSAGE("Allocating unnecessarily large class %" PRIuS " for %" PRIuS
              "\n", sc, size);
      CRASH();
    }
    if (sc >= kNumClasses) {
      MESSAGE("Bad size class %" PRIuS " for %" PRIuS "\n", sc, size);
      CRASH();
    }
    const size_t s = class_to_size[sc];
    if (size > s) {
     MESSAGE("Bad size %" PRIuS " for %" PRIuS " (sc = %" PRIuS ")\n", s, size, sc);
      CRASH();
    }
    if (s == 0) {
      MESSAGE("Bad size %" PRIuS " for %" PRIuS " (sc = %" PRIuS ")\n", s, size, sc);
      CRASH();
    }
  }

  // Initialize the num_objects_to_move array.
  for (size_t cl = 1; cl  < kNumClasses; ++cl) {
    num_objects_to_move[cl] = NumMoveSize(ByteSizeForClass(cl));
  }

#ifndef WTF_CHANGES
  if (false) {
    // Dump class sizes and maximum external wastage per size class
    for (size_t cl = 1; cl  < kNumClasses; ++cl) {
      const int alloc_size = class_to_pages[cl] << kPageShift;
      const int alloc_objs = alloc_size / class_to_size[cl];
      const int min_used = (class_to_size[cl-1] + 1) * alloc_objs;
      const int max_waste = alloc_size - min_used;
      MESSAGE("SC %3d [ %8d .. %8d ] from %8d ; %2.0f%% maxwaste\n",
              int(cl),
              int(class_to_size[cl-1] + 1),
              int(class_to_size[cl]),
              int(class_to_pages[cl] << kPageShift),
              max_waste * 100.0 / alloc_size
              );
    }
  }
#endif
}

// -------------------------------------------------------------------------
// Simple allocator for objects of a specified type.  External locking
// is required before accessing one of these objects.
// -------------------------------------------------------------------------

// Metadata allocator -- keeps stats about how many bytes allocated
static uint64_t metadata_system_bytes = 0;
static void* MetaDataAlloc(size_t bytes) {
  void* result = TCMalloc_SystemAlloc(bytes, 0);
  if (result != NULL) {
    metadata_system_bytes += bytes;
  }
  return result;
}

template <class T>
class PageHeapAllocator {
 private:
  // How much to allocate from system at a time
  static const size_t kAllocIncrement = 32 << 10;

  // Aligned size of T
  static const size_t kAlignedSize
  = (((sizeof(T) + kAlignment - 1) / kAlignment) * kAlignment);

  // Free area from which to carve new objects
  char* free_area_;
  size_t free_avail_;

  // Linked list of all regions allocated by this allocator
  void* allocated_regions_;

  // Free list of already carved objects
  void* free_list_;

  // Number of allocated but unfreed objects
  int inuse_;

 public:
  void Init() {
    ASSERT(kAlignedSize <= kAllocIncrement);
    inuse_ = 0;
    allocated_regions_ = 0;
    free_area_ = NULL;
    free_avail_ = 0;
    free_list_ = NULL;
  }

  T* New() {
    // Consult free list
    void* result;
    if (free_list_ != NULL) {
      result = free_list_;
      free_list_ = *(reinterpret_cast<void**>(result));
    } else {
      if (free_avail_ < kAlignedSize) {
        // Need more room
        char* new_allocation = reinterpret_cast<char*>(MetaDataAlloc(kAllocIncrement));
        if (!new_allocation)
          CRASH();

        *reinterpret_cast_ptr<void**>(new_allocation) = allocated_regions_;
        allocated_regions_ = new_allocation;
        free_area_ = new_allocation + kAlignedSize;
        free_avail_ = kAllocIncrement - kAlignedSize;
      }
      result = free_area_;
      free_area_ += kAlignedSize;
      free_avail_ -= kAlignedSize;
    }
    inuse_++;
    return reinterpret_cast<T*>(result);
  }

  void Delete(T* p) {
    *(reinterpret_cast<void**>(p)) = free_list_;
    free_list_ = p;
    inuse_--;
  }

  int inuse() const { return inuse_; }

#if defined(WTF_CHANGES) && OS(DARWIN)
  template <class Recorder>
  void recordAdministrativeRegions(Recorder& recorder, const RemoteMemoryReader& reader)
  {
      for (void* adminAllocation = allocated_regions_; adminAllocation; adminAllocation = reader.nextEntryInLinkedList(reinterpret_cast<void**>(adminAllocation)))
          recorder.recordRegion(reinterpret_cast<vm_address_t>(adminAllocation), kAllocIncrement);
  }
#endif
};

// -------------------------------------------------------------------------
// Span - a contiguous run of pages
// -------------------------------------------------------------------------

// Type that can hold a page number
typedef uintptr_t PageID;

// Type that can hold the length of a run of pages
typedef uintptr_t Length;

static const Length kMaxValidPages = (~static_cast<Length>(0)) >> kPageShift;

// Convert byte size into pages.  This won't overflow, but may return
// an unreasonably large value if bytes is huge enough.
static inline Length pages(size_t bytes) {
  return (bytes >> kPageShift) +
      ((bytes & (kPageSize - 1)) > 0 ? 1 : 0);
}

// Convert a user size into the number of bytes that will actually be
// allocated
static size_t AllocationSize(size_t bytes) {
  if (bytes > kMaxSize) {
    // Large object: we allocate an integral number of pages
    ASSERT(bytes <= (kMaxValidPages << kPageShift));
    return pages(bytes) << kPageShift;
  } else {
    // Small object: find the size class to which it belongs
    return ByteSizeForClass(SizeClass(bytes));
  }
}

// Information kept for a span (a contiguous run of pages).
struct Span {
  PageID        start;          // Starting page number
  Length        length;         // Number of pages in span
  Span*         next;           // Used when in link list
  Span*         prev;           // Used when in link list
  void*         objects;        // Linked list of free objects
  unsigned int  free : 1;       // Is the span free
#ifndef NO_TCMALLOC_SAMPLES
  unsigned int  sample : 1;     // Sampled object?
#endif
  unsigned int  sizeclass : 8;  // Size-class for small objects (or 0)
  unsigned int  refcount : 11;  // Number of non-free objects
  bool decommitted : 1;

#undef SPAN_HISTORY
#ifdef SPAN_HISTORY
  // For debugging, we can keep a log events per span
  int nexthistory;
  char history[64];
  int value[64];
#endif
};

#define ASSERT_SPAN_COMMITTED(span) ASSERT(!span->decommitted)

#ifdef SPAN_HISTORY
void Event(Span* span, char op, int v = 0) {
  span->history[span->nexthistory] = op;
  span->value[span->nexthistory] = v;
  span->nexthistory++;
  if (span->nexthistory == sizeof(span->history)) span->nexthistory = 0;
}
#else
#define Event(s,o,v) ((void) 0)
#endif

// Allocator/deallocator for spans
static PageHeapAllocator<Span> span_allocator;
static Span* NewSpan(PageID p, Length len) {
  Span* result = span_allocator.New();
  memset(result, 0, sizeof(*result));
  result->start = p;
  result->length = len;
#ifdef SPAN_HISTORY
  result->nexthistory = 0;
#endif
  return result;
}

static inline void DeleteSpan(Span* span) {
#ifndef NDEBUG
  // In debug mode, trash the contents of deleted Spans
  memset(span, 0x3f, sizeof(*span));
#endif
  span_allocator.Delete(span);
}

// -------------------------------------------------------------------------
// Doubly linked list of spans.
// -------------------------------------------------------------------------

static inline void DLL_Init(Span* list) {
  list->next = list;
  list->prev = list;
}

static inline void DLL_Remove(Span* span) {
  span->prev->next = span->next;
  span->next->prev = span->prev;
  span->prev = NULL;
  span->next = NULL;
}

static ALWAYS_INLINE bool DLL_IsEmpty(const Span* list) {
  return list->next == list;
}

static int DLL_Length(const Span* list) {
  int result = 0;
  for (Span* s = list->next; s != list; s = s->next) {
    result++;
  }
  return result;
}

#if 0 /* Not needed at the moment -- causes compiler warnings if not used */
static void DLL_Print(const char* label, const Span* list) {
  MESSAGE("%-10s %p:", label, list);
  for (const Span* s = list->next; s != list; s = s->next) {
    MESSAGE(" <%p,%u,%u>", s, s->start, s->length);
  }
  MESSAGE("\n");
}
#endif

static inline void DLL_Prepend(Span* list, Span* span) {
  ASSERT(span->next == NULL);
  ASSERT(span->prev == NULL);
  span->next = list->next;
  span->prev = list;
  list->next->prev = span;
  list->next = span;
}

// -------------------------------------------------------------------------
// Stack traces kept for sampled allocations
//   The following state is protected by pageheap_lock_.
// -------------------------------------------------------------------------

// size/depth are made the same size as a pointer so that some generic
// code below can conveniently cast them back and forth to void*.
static const int kMaxStackDepth = 31;
struct StackTrace {
  uintptr_t size;          // Size of object
  uintptr_t depth;         // Number of PC values stored in array below
  void*     stack[kMaxStackDepth];
};
static PageHeapAllocator<StackTrace> stacktrace_allocator;
static Span sampled_objects;

// -------------------------------------------------------------------------
// Map from page-id to per-page data
// -------------------------------------------------------------------------

// We use PageMap2<> for 32-bit and PageMap3<> for 64-bit machines.
// We also use a simple one-level cache for hot PageID-to-sizeclass mappings,
// because sometimes the sizeclass is all the information we need.

// Selector class -- general selector uses 3-level map
template <int BITS> class MapSelector {
 public:
  typedef TCMalloc_PageMap3<BITS-kPageShift> Type;
  typedef PackedCache<BITS, uint64_t> CacheType;
};

#if defined(WTF_CHANGES)
#if CPU(X86_64)
// On all known X86-64 platforms, the upper 16 bits are always unused and therefore 
// can be excluded from the PageMap key.
// See http://en.wikipedia.org/wiki/X86-64#Virtual_address_space_details

static const size_t kBitsUnusedOn64Bit = 16;
#else
static const size_t kBitsUnusedOn64Bit = 0;
#endif

// A three-level map for 64-bit machines
template <> class MapSelector<64> {
 public:
  typedef TCMalloc_PageMap3<64 - kPageShift - kBitsUnusedOn64Bit> Type;
  typedef PackedCache<64, uint64_t> CacheType;
};
#endif

// A two-level map for 32-bit machines
template <> class MapSelector<32> {
 public:
  typedef TCMalloc_PageMap2<32 - kPageShift> Type;
  typedef PackedCache<32 - kPageShift, uint16_t> CacheType;
};

// -------------------------------------------------------------------------
// Page-level allocator
//  * Eager coalescing
//
// Heap for page-level allocation.  We allow allocating and freeing a
// contiguous runs of pages (called a "span").
// -------------------------------------------------------------------------

#if USE_BACKGROUND_THREAD_TO_SCAVENGE_MEMORY
// The page heap maintains a free list for spans that are no longer in use by
// the central cache or any thread caches. We use a background thread to
// periodically scan the free list and release a percentage of it back to the OS.

// If free_committed_pages_ exceeds kMinimumFreeCommittedPageCount, the
// background thread:
//     - wakes up
//     - pauses for kScavengeDelayInSeconds
//     - returns to the OS a percentage of the memory that remained unused during
//       that pause (kScavengePercentage * min_free_committed_pages_since_last_scavenge_)
// The goal of this strategy is to reduce memory pressure in a timely fashion
// while avoiding thrashing the OS allocator.

// Time delay before the page heap scavenger will consider returning pages to
// the OS.
static const int kScavengeDelayInSeconds = 2;

// Approximate percentage of free committed pages to return to the OS in one
// scavenge.
static const float kScavengePercentage = .5f;

// number of span lists to keep spans in when memory is returned.
static const int kMinSpanListsWithSpans = 32;

// Number of free committed pages that we want to keep around.  The minimum number of pages used when there
// is 1 span in each of the first kMinSpanListsWithSpans spanlists.  Currently 528 pages.
static const size_t kMinimumFreeCommittedPageCount = kMinSpanListsWithSpans * ((1.0f+kMinSpanListsWithSpans) / 2.0f);

#endif

static SpinLock pageheap_lock = SPINLOCK_INITIALIZER;

class TCMalloc_PageHeap {
 public:
  void init();

  // Allocate a run of "n" pages.  Returns zero if out of memory.
  Span* New(Length n);

  // Delete the span "[p, p+n-1]".
  // REQUIRES: span was returned by earlier call to New() and
  //           has not yet been deleted.
  void Delete(Span* span);

  // Mark an allocated span as being used for small objects of the
  // specified size-class.
  // REQUIRES: span was returned by an earlier call to New()
  //           and has not yet been deleted.
  void RegisterSizeClass(Span* span, size_t sc);

  // Split an allocated span into two spans: one of length "n" pages
  // followed by another span of length "span->length - n" pages.
  // Modifies "*span" to point to the first span of length "n" pages.
  // Returns a pointer to the second span.
  //
  // REQUIRES: "0 < n < span->length"
  // REQUIRES: !span->free
  // REQUIRES: span->sizeclass == 0
  Span* Split(Span* span, Length n);

  // Return the descriptor for the specified page.
  inline Span* GetDescriptor(PageID p) const {
    return reinterpret_cast<Span*>(pagemap_.get(p));
  }

#ifdef WTF_CHANGES
  inline Span* GetDescriptorEnsureSafe(PageID p)
  {
      pagemap_.Ensure(p, 1);
      return GetDescriptor(p);
  }
    
  size_t ReturnedBytes() const;
#endif

  // Dump state to stderr
#ifndef WTF_CHANGES
  void Dump(TCMalloc_Printer* out);
#endif

  // Return number of bytes allocated from system
  inline uint64_t SystemBytes() const { return system_bytes_; }

  // Return number of free bytes in heap
  uint64_t FreeBytes() const {
    return (static_cast<uint64_t>(free_pages_) << kPageShift);
  }

  bool Check();
  bool CheckList(Span* list, Length min_pages, Length max_pages);

  // Release all pages on the free list for reuse by the OS:
  void ReleaseFreePages();

  // Return 0 if we have no information, or else the correct sizeclass for p.
  // Reads and writes to pagemap_cache_ do not require locking.
  // The entries are 64 bits on 64-bit hardware and 16 bits on
  // 32-bit hardware, and we don't mind raciness as long as each read of
  // an entry yields a valid entry, not a partially updated entry.
  size_t GetSizeClassIfCached(PageID p) const {
    return pagemap_cache_.GetOrDefault(p, 0);
  }
  void CacheSizeClass(PageID p, size_t cl) const { pagemap_cache_.Put(p, cl); }

 private:
  // Pick the appropriate map and cache types based on pointer size
  typedef MapSelector<8*sizeof(uintptr_t)>::Type PageMap;
  typedef MapSelector<8*sizeof(uintptr_t)>::CacheType PageMapCache;
  PageMap pagemap_;
  mutable PageMapCache pagemap_cache_;

  // We segregate spans of a given size into two circular linked
  // lists: one for normal spans, and one for spans whose memory
  // has been returned to the system.
  struct SpanList {
    Span        normal;
    Span        returned;
  };

  // List of free spans of length >= kMaxPages
  SpanList large_;

  // Array mapping from span length to a doubly linked list of free spans
  SpanList free_[kMaxPages];

  // Number of pages kept in free lists
  uintptr_t free_pages_;

  // Bytes allocated from system
  uint64_t system_bytes_;

#if USE_BACKGROUND_THREAD_TO_SCAVENGE_MEMORY
  // Number of pages kept in free lists that are still committed.
  Length free_committed_pages_;

  // Minimum number of free committed pages since last scavenge. (Can be 0 if
  // we've committed new pages since the last scavenge.)
  Length min_free_committed_pages_since_last_scavenge_;
#endif

  bool GrowHeap(Length n);

  // REQUIRES   span->length >= n
  // Remove span from its free list, and move any leftover part of
  // span into appropriate free lists.  Also update "span" to have
  // length exactly "n" and mark it as non-free so it can be returned
  // to the client.
  //
  // "released" is true iff "span" was found on a "returned" list.
  void Carve(Span* span, Length n, bool released);

  void RecordSpan(Span* span) {
    pagemap_.set(span->start, span);
    if (span->length > 1) {
      pagemap_.set(span->start + span->length - 1, span);
    }
  }
  
    // Allocate a large span of length == n.  If successful, returns a
  // span of exactly the specified length.  Else, returns NULL.
  Span* AllocLarge(Length n);

#if !USE_BACKGROUND_THREAD_TO_SCAVENGE_MEMORY
  // Incrementally release some memory to the system.
  // IncrementalScavenge(n) is called whenever n pages are freed.
  void IncrementalScavenge(Length n);
#endif

  // Number of pages to deallocate before doing more scavenging
  int64_t scavenge_counter_;

  // Index of last free list we scavenged
  size_t scavenge_index_;
  
#if defined(WTF_CHANGES) && OS(DARWIN)
  friend class FastMallocZone;
#endif

#if USE_BACKGROUND_THREAD_TO_SCAVENGE_MEMORY
  void initializeScavenger();
  ALWAYS_INLINE void signalScavenger();
  void scavenge();
  ALWAYS_INLINE bool shouldScavenge() const;

#if HAVE(DISPATCH_H) || OS(WINDOWS)
  void periodicScavenge();
  ALWAYS_INLINE bool isScavengerSuspended();
  ALWAYS_INLINE void scheduleScavenger();
  ALWAYS_INLINE void rescheduleScavenger();
  ALWAYS_INLINE void suspendScavenger();
#endif

#if HAVE(DISPATCH_H)
  dispatch_queue_t m_scavengeQueue;
  dispatch_source_t m_scavengeTimer;
  bool m_scavengingSuspended;
#elif OS(WINDOWS)
  static void CALLBACK scavengerTimerFired(void*, BOOLEAN);
  HANDLE m_scavengeQueueTimer;
#else 
  static NO_RETURN_WITH_VALUE void* runScavengerThread(void*);
  NO_RETURN void scavengerThread();

  // Keeps track of whether the background thread is actively scavenging memory every kScavengeDelayInSeconds, or
  // it's blocked waiting for more pages to be deleted.
  bool m_scavengeThreadActive;

  pthread_mutex_t m_scavengeMutex;
  pthread_cond_t m_scavengeCondition;
#endif

#endif  // USE_BACKGROUND_THREAD_TO_SCAVENGE_MEMORY
};

void TCMalloc_PageHeap::init()
{
  pagemap_.init(MetaDataAlloc);
  pagemap_cache_ = PageMapCache(0);
  free_pages_ = 0;
  system_bytes_ = 0;

#if USE_BACKGROUND_THREAD_TO_SCAVENGE_MEMORY
  free_committed_pages_ = 0;
  min_free_committed_pages_since_last_scavenge_ = 0;
#endif  // USE_BACKGROUND_THREAD_TO_SCAVENGE_MEMORY

  scavenge_counter_ = 0;
  // Start scavenging at kMaxPages list
  scavenge_index_ = kMaxPages-1;
  COMPILE_ASSERT(kNumClasses <= (1 << PageMapCache::kValuebits), valuebits);
  DLL_Init(&large_.normal);
  DLL_Init(&large_.returned);
  for (size_t i = 0; i < kMaxPages; i++) {
    DLL_Init(&free_[i].normal);
    DLL_Init(&free_[i].returned);
  }

#if USE_BACKGROUND_THREAD_TO_SCAVENGE_MEMORY
  initializeScavenger();
#endif  // USE_BACKGROUND_THREAD_TO_SCAVENGE_MEMORY
}

#if USE_BACKGROUND_THREAD_TO_SCAVENGE_MEMORY

#if HAVE(DISPATCH_H)

void TCMalloc_PageHeap::initializeScavenger()
{
    m_scavengeQueue = dispatch_queue_create("com.apple.JavaScriptCore.FastMallocSavenger", NULL);
    m_scavengeTimer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, m_scavengeQueue);
    dispatch_time_t startTime = dispatch_time(DISPATCH_TIME_NOW, kScavengeDelayInSeconds * NSEC_PER_SEC);
    dispatch_source_set_timer(m_scavengeTimer, startTime, kScavengeDelayInSeconds * NSEC_PER_SEC, 1000 * NSEC_PER_USEC);
    dispatch_source_set_event_handler(m_scavengeTimer, ^{ periodicScavenge(); });
    m_scavengingSuspended = true;
}

ALWAYS_INLINE bool TCMalloc_PageHeap::isScavengerSuspended()
{
    ASSERT(pageheap_lock.IsHeld());
    return m_scavengingSuspended;
}

ALWAYS_INLINE void TCMalloc_PageHeap::scheduleScavenger()
{
    ASSERT(pageheap_lock.IsHeld());
    m_scavengingSuspended = false;
    dispatch_resume(m_scavengeTimer);
}

ALWAYS_INLINE void TCMalloc_PageHeap::rescheduleScavenger()
{
    // Nothing to do here for libdispatch.
}

ALWAYS_INLINE void TCMalloc_PageHeap::suspendScavenger()
{
    ASSERT(pageheap_lock.IsHeld());
    m_scavengingSuspended = true;
    dispatch_suspend(m_scavengeTimer);
}

#elif OS(WINDOWS)

void TCMalloc_PageHeap::scavengerTimerFired(void* context, BOOLEAN)
{
    static_cast<TCMalloc_PageHeap*>(context)->periodicScavenge();
}

void TCMalloc_PageHeap::initializeScavenger()
{
    m_scavengeQueueTimer = 0;
}

ALWAYS_INLINE bool TCMalloc_PageHeap::isScavengerSuspended()
{
    ASSERT(IsHeld(pageheap_lock));
    return !m_scavengeQueueTimer;
}

ALWAYS_INLINE void TCMalloc_PageHeap::scheduleScavenger()
{
    // We need to use WT_EXECUTEONLYONCE here and reschedule the timer, because
    // Windows will fire the timer event even when the function is already running.
    ASSERT(IsHeld(pageheap_lock));
    CreateTimerQueueTimer(&m_scavengeQueueTimer, 0, scavengerTimerFired, this, kScavengeDelayInSeconds * 1000, 0, WT_EXECUTEONLYONCE);
}

ALWAYS_INLINE void TCMalloc_PageHeap::rescheduleScavenger()
{
    // We must delete the timer and create it again, because it is not possible to retrigger a timer on Windows.
    suspendScavenger();
    scheduleScavenger();
}

ALWAYS_INLINE void TCMalloc_PageHeap::suspendScavenger()
{
    ASSERT(IsHeld(pageheap_lock));
    HANDLE scavengeQueueTimer = m_scavengeQueueTimer;
    m_scavengeQueueTimer = 0;
    DeleteTimerQueueTimer(0, scavengeQueueTimer, 0);
}

#else

void TCMalloc_PageHeap::initializeScavenger()
{
    // Create a non-recursive mutex.
#if !defined(PTHREAD_MUTEX_NORMAL) || PTHREAD_MUTEX_NORMAL == PTHREAD_MUTEX_DEFAULT
    pthread_mutex_init(&m_scavengeMutex, 0);
#else
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);

    pthread_mutex_init(&m_scavengeMutex, &attr);

    pthread_mutexattr_destroy(&attr);
#endif

    pthread_cond_init(&m_scavengeCondition, 0);
    m_scavengeThreadActive = true;
    pthread_t thread;
    pthread_create(&thread, 0, runScavengerThread, this);
}

void* TCMalloc_PageHeap::runScavengerThread(void* context)
{
    static_cast<TCMalloc_PageHeap*>(context)->scavengerThread();
#if (COMPILER(MSVC) || COMPILER(SUNCC))
    // Without this, Visual Studio and Sun Studio will complain that this method does not return a value.
    return 0;
#endif
}

ALWAYS_INLINE void TCMalloc_PageHeap::signalScavenger()
{
    // m_scavengeMutex should be held before accessing m_scavengeThreadActive.
    ASSERT(pthread_mutex_trylock(m_scavengeMutex));
    if (!m_scavengeThreadActive && shouldScavenge())
        pthread_cond_signal(&m_scavengeCondition);
}

#endif

void TCMalloc_PageHeap::scavenge()
{
    size_t pagesToRelease = min_free_committed_pages_since_last_scavenge_ * kScavengePercentage;
    size_t targetPageCount = std::max<size_t>(kMinimumFreeCommittedPageCount, free_committed_pages_ - pagesToRelease);

    while (free_committed_pages_ > targetPageCount) {
        for (int i = kMaxPages; i > 0 && free_committed_pages_ >= targetPageCount; i--) {
            SpanList* slist = (static_cast<size_t>(i) == kMaxPages) ? &large_ : &free_[i];
            // If the span size is bigger than kMinSpanListsWithSpans pages return all the spans in the list, else return all but 1 span.  
            // Return only 50% of a spanlist at a time so spans of size 1 are not the only ones left.
            size_t length = DLL_Length(&slist->normal);
            size_t numSpansToReturn = (i > kMinSpanListsWithSpans) ? length : length / 2;
            for (int j = 0; static_cast<size_t>(j) < numSpansToReturn && !DLL_IsEmpty(&slist->normal) && free_committed_pages_ > targetPageCount; j++) {
                Span* s = slist->normal.prev; 
                DLL_Remove(s);
                ASSERT(!s->decommitted);
                if (!s->decommitted) {
                    TCMalloc_SystemRelease(reinterpret_cast<void*>(s->start << kPageShift),
                                           static_cast<size_t>(s->length << kPageShift));
                    ASSERT(free_committed_pages_ >= s->length);
                    free_committed_pages_ -= s->length;
                    s->decommitted = true;
                }
                DLL_Prepend(&slist->returned, s);
            }
        }
    }

    min_free_committed_pages_since_last_scavenge_ = free_committed_pages_;
}

ALWAYS_INLINE bool TCMalloc_PageHeap::shouldScavenge() const 
{
    return free_committed_pages_ > kMinimumFreeCommittedPageCount; 
}

#endif  // USE_BACKGROUND_THREAD_TO_SCAVENGE_MEMORY

inline Span* TCMalloc_PageHeap::New(Length n) {
  ASSERT(Check());
  ASSERT(n > 0);

  // Find first size >= n that has a non-empty list
  for (Length s = n; s < kMaxPages; s++) {
    Span* ll = NULL;
    bool released = false;
    if (!DLL_IsEmpty(&free_[s].normal)) {
      // Found normal span
      ll = &free_[s].normal;
    } else if (!DLL_IsEmpty(&free_[s].returned)) {
      // Found returned span; reallocate it
      ll = &free_[s].returned;
      released = true;
    } else {
      // Keep looking in larger classes
      continue;
    }

    Span* result = ll->next;
    Carve(result, n, released);
#if USE_BACKGROUND_THREAD_TO_SCAVENGE_MEMORY
    // The newly allocated memory is from a span that's in the normal span list (already committed).  Update the
    // free committed pages count.
    ASSERT(free_committed_pages_ >= n);
    free_committed_pages_ -= n;
    if (free_committed_pages_ < min_free_committed_pages_since_last_scavenge_) 
      min_free_committed_pages_since_last_scavenge_ = free_committed_pages_;
#endif  // USE_BACKGROUND_THREAD_TO_SCAVENGE_MEMORY
    ASSERT(Check());
    free_pages_ -= n;
    return result;
  }

  Span* result = AllocLarge(n);
  if (result != NULL) {
      ASSERT_SPAN_COMMITTED(result);
      return result;
  }

  // Grow the heap and try again
  if (!GrowHeap(n)) {
    ASSERT(Check());
    return NULL;
  }

  return AllocLarge(n);
}

Span* TCMalloc_PageHeap::AllocLarge(Length n) {
  // find the best span (closest to n in size).
  // The following loops implements address-ordered best-fit.
  bool from_released = false;
  Span *best = NULL;

  // Search through normal list
  for (Span* span = large_.normal.next;
       span != &large_.normal;
       span = span->next) {
    if (span->length >= n) {
      if ((best == NULL)
          || (span->length < best->length)
          || ((span->length == best->length) && (span->start < best->start))) {
        best = span;
        from_released = false;
      }
    }
  }

  // Search through released list in case it has a better fit
  for (Span* span = large_.returned.next;
       span != &large_.returned;
       span = span->next) {
    if (span->length >= n) {
      if ((best == NULL)
          || (span->length < best->length)
          || ((span->length == best->length) && (span->start < best->start))) {
        best = span;
        from_released = true;
      }
    }
  }

  if (best != NULL) {
    Carve(best, n, from_released);
#if USE_BACKGROUND_THREAD_TO_SCAVENGE_MEMORY
    // The newly allocated memory is from a span that's in the normal span list (already committed).  Update the
    // free committed pages count.
    ASSERT(free_committed_pages_ >= n);
    free_committed_pages_ -= n;
    if (free_committed_pages_ < min_free_committed_pages_since_last_scavenge_)
      min_free_committed_pages_since_last_scavenge_ = free_committed_pages_;
#endif  // USE_BACKGROUND_THREAD_TO_SCAVENGE_MEMORY
    ASSERT(Check());
    free_pages_ -= n;
    return best;
  }
  return NULL;
}

Span* TCMalloc_PageHeap::Split(Span* span, Length n) {
  ASSERT(0 < n);
  ASSERT(n < span->length);
  ASSERT(!span->free);
  ASSERT(span->sizeclass == 0);
  Event(span, 'T', n);

  const Length extra = span->length - n;
  Span* leftover = NewSpan(span->start + n, extra);
  Event(leftover, 'U', extra);
  RecordSpan(leftover);
  pagemap_.set(span->start + n - 1, span); // Update map from pageid to span
  span->length = n;

  return leftover;
}

inline void TCMalloc_PageHeap::Carve(Span* span, Length n, bool released) {
  ASSERT(n > 0);
  DLL_Remove(span);
  span->free = 0;
  Event(span, 'A', n);

  if (released) {
    // If the span chosen to carve from is decommited, commit the entire span at once to avoid committing spans 1 page at a time.
    ASSERT(span->decommitted);
    TCMalloc_SystemCommit(reinterpret_cast<void*>(span->start << kPageShift), static_cast<size_t>(span->length << kPageShift));
    span->decommitted = false;
#if USE_BACKGROUND_THREAD_TO_SCAVENGE_MEMORY
    free_committed_pages_ += span->length;
#endif
  }
  
  const int extra = static_cast<int>(span->length - n);
  ASSERT(extra >= 0);
  if (extra > 0) {
    Span* leftover = NewSpan(span->start + n, extra);
    leftover->free = 1;
    leftover->decommitted = false;
    Event(leftover, 'S', extra);
    RecordSpan(leftover);

    // Place leftover span on appropriate free list
    SpanList* listpair = (static_cast<size_t>(extra) < kMaxPages) ? &free_[extra] : &large_;
    Span* dst = &listpair->normal;
    DLL_Prepend(dst, leftover);

    span->length = n;
    pagemap_.set(span->start + n - 1, span);
  }
}

static ALWAYS_INLINE void mergeDecommittedStates(Span* destination, Span* other)
{
    if (destination->decommitted && !other->decommitted) {
        TCMalloc_SystemRelease(reinterpret_cast<void*>(other->start << kPageShift),
                               static_cast<size_t>(other->length << kPageShift));
    } else if (other->decommitted && !destination->decommitted) {
        TCMalloc_SystemRelease(reinterpret_cast<void*>(destination->start << kPageShift),
                               static_cast<size_t>(destination->length << kPageShift));
        destination->decommitted = true;
    }
}

inline void TCMalloc_PageHeap::Delete(Span* span) {
  ASSERT(Check());
  ASSERT(!span->free);
  ASSERT(span->length > 0);
  ASSERT(GetDescriptor(span->start) == span);
  ASSERT(GetDescriptor(span->start + span->length - 1) == span);
  span->sizeclass = 0;
#ifndef NO_TCMALLOC_SAMPLES
  span->sample = 0;
#endif

  // Coalesce -- we guarantee that "p" != 0, so no bounds checking
  // necessary.  We do not bother resetting the stale pagemap
  // entries for the pieces we are merging together because we only
  // care about the pagemap entries for the boundaries.
#if USE_BACKGROUND_THREAD_TO_SCAVENGE_MEMORY
  // Track the total size of the neighboring free spans that are committed.
  Length neighboringCommittedSpansLength = 0;
#endif
  const PageID p = span->start;
  const Length n = span->length;
  Span* prev = GetDescriptor(p-1);
  if (prev != NULL && prev->free) {
    // Merge preceding span into this span
    ASSERT(prev->start + prev->length == p);
    const Length len = prev->length;
#if USE_BACKGROUND_THREAD_TO_SCAVENGE_MEMORY
    if (!prev->decommitted)
        neighboringCommittedSpansLength += len;
#endif
    mergeDecommittedStates(span, prev);
    DLL_Remove(prev);
    DeleteSpan(prev);
    span->start -= len;
    span->length += len;
    pagemap_.set(span->start, span);
    Event(span, 'L', len);
  }
  Span* next = GetDescriptor(p+n);
  if (next != NULL && next->free) {
    // Merge next span into this span
    ASSERT(next->start == p+n);
    const Length len = next->length;
#if USE_BACKGROUND_THREAD_TO_SCAVENGE_MEMORY
    if (!next->decommitted)
        neighboringCommittedSpansLength += len;
#endif
    mergeDecommittedStates(span, next);
    DLL_Remove(next);
    DeleteSpan(next);
    span->length += len;
    pagemap_.set(span->start + span->length - 1, span);
    Event(span, 'R', len);
  }

  Event(span, 'D', span->length);
  span->free = 1;
  if (span->decommitted) {
    if (span->length < kMaxPages)
      DLL_Prepend(&free_[span->length].returned, span);
    else
      DLL_Prepend(&large_.returned, span);
  } else {
    if (span->length < kMaxPages)
      DLL_Prepend(&free_[span->length].normal, span);
    else
      DLL_Prepend(&large_.normal, span);
  }
  free_pages_ += n;

#if USE_BACKGROUND_THREAD_TO_SCAVENGE_MEMORY
  if (span->decommitted) {
      // If the merged span is decommitted, that means we decommitted any neighboring spans that were
      // committed.  Update the free committed pages count.
      free_committed_pages_ -= neighboringCommittedSpansLength;
      if (free_committed_pages_ < min_free_committed_pages_since_last_scavenge_)
            min_free_committed_pages_since_last_scavenge_ = free_committed_pages_;
  } else {
      // If the merged span remains committed, add the deleted span's size to the free committed pages count.
      free_committed_pages_ += n;
  }

  // Make sure the scavenge thread becomes active if we have enough freed pages to release some back to the system.
  signalScavenger();
#else
  IncrementalScavenge(n);
#endif

  ASSERT(Check());
}

#if !USE_BACKGROUND_THREAD_TO_SCAVENGE_MEMORY
void TCMalloc_PageHeap::IncrementalScavenge(Length n) {
  // Fast path; not yet time to release memory
  scavenge_counter_ -= n;
  if (scavenge_counter_ >= 0) return;  // Not yet time to scavenge

  // If there is nothing to release, wait for so many pages before
  // scavenging again.  With 4K pages, this comes to 16MB of memory.
  static const size_t kDefaultReleaseDelay = 1 << 8;

  // Find index of free list to scavenge
  size_t index = scavenge_index_ + 1;
  for (size_t i = 0; i < kMaxPages+1; i++) {
    if (index > kMaxPages) index = 0;
    SpanList* slist = (index == kMaxPages) ? &large_ : &free_[index];
    if (!DLL_IsEmpty(&slist->normal)) {
      // Release the last span on the normal portion of this list
      Span* s = slist->normal.prev;
      DLL_Remove(s);
      TCMalloc_SystemRelease(reinterpret_cast<void*>(s->start << kPageShift),
                             static_cast<size_t>(s->length << kPageShift));
      s->decommitted = true;
      DLL_Prepend(&slist->returned, s);

      scavenge_counter_ = std::max<size_t>(64UL, std::min<size_t>(kDefaultReleaseDelay, kDefaultReleaseDelay - (free_pages_ / kDefaultReleaseDelay)));

      if (index == kMaxPages && !DLL_IsEmpty(&slist->normal))
        scavenge_index_ = index - 1;
      else
        scavenge_index_ = index;
      return;
    }
    index++;
  }

  // Nothing to scavenge, delay for a while
  scavenge_counter_ = kDefaultReleaseDelay;
}
#endif

void TCMalloc_PageHeap::RegisterSizeClass(Span* span, size_t sc) {
  // Associate span object with all interior pages as well
  ASSERT(!span->free);
  ASSERT(GetDescriptor(span->start) == span);
  ASSERT(GetDescriptor(span->start+span->length-1) == span);
  Event(span, 'C', sc);
  span->sizeclass = static_cast<unsigned int>(sc);
  for (Length i = 1; i < span->length-1; i++) {
    pagemap_.set(span->start+i, span);
  }
}
    
#ifdef WTF_CHANGES
size_t TCMalloc_PageHeap::ReturnedBytes() const {
    size_t result = 0;
    for (unsigned s = 0; s < kMaxPages; s++) {
        const int r_length = DLL_Length(&free_[s].returned);
        unsigned r_pages = s * r_length;
        result += r_pages << kPageShift;
    }
    
    for (Span* s = large_.returned.next; s != &large_.returned; s = s->next)
        result += s->length << kPageShift;
    return result;
}
#endif

#ifndef WTF_CHANGES
static double PagesToMB(uint64_t pages) {
  return (pages << kPageShift) / 1048576.0;
}

void TCMalloc_PageHeap::Dump(TCMalloc_Printer* out) {
  int nonempty_sizes = 0;
  for (int s = 0; s < kMaxPages; s++) {
    if (!DLL_IsEmpty(&free_[s].normal) || !DLL_IsEmpty(&free_[s].returned)) {
      nonempty_sizes++;
    }
  }
  out->printf("------------------------------------------------\n");
  out->printf("PageHeap: %d sizes; %6.1f MB free\n",
              nonempty_sizes, PagesToMB(free_pages_));
  out->printf("------------------------------------------------\n");
  uint64_t total_normal = 0;
  uint64_t total_returned = 0;
  for (int s = 0; s < kMaxPages; s++) {
    const int n_length = DLL_Length(&free_[s].normal);
    const int r_length = DLL_Length(&free_[s].returned);
    if (n_length + r_length > 0) {
      uint64_t n_pages = s * n_length;
      uint64_t r_pages = s * r_length;
      total_normal += n_pages;
      total_returned += r_pages;
      out->printf("%6u pages * %6u spans ~ %6.1f MB; %6.1f MB cum"
                  "; unmapped: %6.1f MB; %6.1f MB cum\n",
                  s,
                  (n_length + r_length),
                  PagesToMB(n_pages + r_pages),
                  PagesToMB(total_normal + total_returned),
                  PagesToMB(r_pages),
                  PagesToMB(total_returned));
    }
  }

  uint64_t n_pages = 0;
  uint64_t r_pages = 0;
  int n_spans = 0;
  int r_spans = 0;
  out->printf("Normal large spans:\n");
  for (Span* s = large_.normal.next; s != &large_.normal; s = s->next) {
    out->printf("   [ %6" PRIuS " pages ] %6.1f MB\n",
                s->length, PagesToMB(s->length));
    n_pages += s->length;
    n_spans++;
  }
  out->printf("Unmapped large spans:\n");
  for (Span* s = large_.returned.next; s != &large_.returned; s = s->next) {
    out->printf("   [ %6" PRIuS " pages ] %6.1f MB\n",
                s->length, PagesToMB(s->length));
    r_pages += s->length;
    r_spans++;
  }
  total_normal += n_pages;
  total_returned += r_pages;
  out->printf(">255   large * %6u spans ~ %6.1f MB; %6.1f MB cum"
              "; unmapped: %6.1f MB; %6.1f MB cum\n",
              (n_spans + r_spans),
              PagesToMB(n_pages + r_pages),
              PagesToMB(total_normal + total_returned),
              PagesToMB(r_pages),
              PagesToMB(total_returned));
}
#endif

bool TCMalloc_PageHeap::GrowHeap(Length n) {
  ASSERT(kMaxPages >= kMinSystemAlloc);
  if (n > kMaxValidPages) return false;
  Length ask = (n>kMinSystemAlloc) ? n : static_cast<Length>(kMinSystemAlloc);
  size_t actual_size;
  void* ptr = TCMalloc_SystemAlloc(ask << kPageShift, &actual_size, kPageSize);
  if (ptr == NULL) {
    if (n < ask) {
      // Try growing just "n" pages
      ask = n;
      ptr = TCMalloc_SystemAlloc(ask << kPageShift, &actual_size, kPageSize);
    }
    if (ptr == NULL) return false;
  }
  ask = actual_size >> kPageShift;

  uint64_t old_system_bytes = system_bytes_;
  system_bytes_ += (ask << kPageShift);
  const PageID p = reinterpret_cast<uintptr_t>(ptr) >> kPageShift;
  ASSERT(p > 0);

  // If we have already a lot of pages allocated, just pre allocate a bunch of
  // memory for the page map. This prevents fragmentation by pagemap metadata
  // when a program keeps allocating and freeing large blocks.

  if (old_system_bytes < kPageMapBigAllocationThreshold
      && system_bytes_ >= kPageMapBigAllocationThreshold) {
    pagemap_.PreallocateMoreMemory();
  }

  // Make sure pagemap_ has entries for all of the new pages.
  // Plus ensure one before and one after so coalescing code
  // does not need bounds-checking.
  if (pagemap_.Ensure(p-1, ask+2)) {
    // Pretend the new area is allocated and then Delete() it to
    // cause any necessary coalescing to occur.
    //
    // We do not adjust free_pages_ here since Delete() will do it for us.
    Span* span = NewSpan(p, ask);
    RecordSpan(span);
    Delete(span);
    ASSERT(Check());
    return true;
  } else {
    // We could not allocate memory within "pagemap_"
    // TODO: Once we can return memory to the system, return the new span
    return false;
  }
}

bool TCMalloc_PageHeap::Check() {
  ASSERT(free_[0].normal.next == &free_[0].normal);
  ASSERT(free_[0].returned.next == &free_[0].returned);
  CheckList(&large_.normal, kMaxPages, 1000000000);
  CheckList(&large_.returned, kMaxPages, 1000000000);
  for (Length s = 1; s < kMaxPages; s++) {
    CheckList(&free_[s].normal, s, s);
    CheckList(&free_[s].returned, s, s);
  }
  return true;
}

#if ASSERT_DISABLED
bool TCMalloc_PageHeap::CheckList(Span*, Length, Length) {
  return true;
}
#else
bool TCMalloc_PageHeap::CheckList(Span* list, Length min_pages, Length max_pages) {
  for (Span* s = list->next; s != list; s = s->next) {
    CHECK_CONDITION(s->free);
    CHECK_CONDITION(s->length >= min_pages);
    CHECK_CONDITION(s->length <= max_pages);
    CHECK_CONDITION(GetDescriptor(s->start) == s);
    CHECK_CONDITION(GetDescriptor(s->start+s->length-1) == s);
  }
  return true;
}
#endif

static void ReleaseFreeList(Span* list, Span* returned) {
  // Walk backwards through list so that when we push these
  // spans on the "returned" list, we preserve the order.
  while (!DLL_IsEmpty(list)) {
    Span* s = list->prev;
    DLL_Remove(s);
    DLL_Prepend(returned, s);
    TCMalloc_SystemRelease(reinterpret_cast<void*>(s->start << kPageShift),
                           static_cast<size_t>(s->length << kPageShift));
  }
}

void TCMalloc_PageHeap::ReleaseFreePages() {
  for (Length s = 0; s < kMaxPages; s++) {
    ReleaseFreeList(&free_[s].normal, &free_[s].returned);
  }
  ReleaseFreeList(&large_.normal, &large_.returned);
  ASSERT(Check());
}

//-------------------------------------------------------------------
// Free list
//-------------------------------------------------------------------

class TCMalloc_ThreadCache_FreeList {
 private:
  void*    list_;       // Linked list of nodes
  uint16_t length_;     // Current length
  uint16_t lowater_;    // Low water mark for list length

 public:
  void Init() {
    list_ = NULL;
    length_ = 0;
    lowater_ = 0;
  }

  // Return current length of list
  int length() const {
    return length_;
  }

  // Is list empty?
  bool empty() const {
    return list_ == NULL;
  }

  // Low-water mark management
  int lowwatermark() const { return lowater_; }
  void clear_lowwatermark() { lowater_ = length_; }

  ALWAYS_INLINE void Push(void* ptr) {
    SLL_Push(&list_, ptr);
    length_++;
  }

  void PushRange(int N, void *start, void *end) {
    SLL_PushRange(&list_, start, end);
    length_ = length_ + static_cast<uint16_t>(N);
  }

  void PopRange(int N, void **start, void **end) {
    SLL_PopRange(&list_, N, start, end);
    ASSERT(length_ >= N);
    length_ = length_ - static_cast<uint16_t>(N);
    if (length_ < lowater_) lowater_ = length_;
  }

  ALWAYS_INLINE void* Pop() {
    ASSERT(list_ != NULL);
    length_--;
    if (length_ < lowater_) lowater_ = length_;
    return SLL_Pop(&list_);
  }

#ifdef WTF_CHANGES
  template <class Finder, class Reader>
  void enumerateFreeObjects(Finder& finder, const Reader& reader)
  {
      for (void* nextObject = list_; nextObject; nextObject = reader.nextEntryInLinkedList(reinterpret_cast<void**>(nextObject)))
          finder.visit(nextObject);
  }
#endif
};

//-------------------------------------------------------------------
// Data kept per thread
//-------------------------------------------------------------------

class TCMalloc_ThreadCache {
 private:
  typedef TCMalloc_ThreadCache_FreeList FreeList;
#if OS(WINDOWS)
  typedef DWORD ThreadIdentifier;
#else
  typedef pthread_t ThreadIdentifier;
#endif

  size_t        size_;                  // Combined size of data
  ThreadIdentifier tid_;                // Which thread owns it
  bool          in_setspecific_;           // Called pthread_setspecific?
  FreeList      list_[kNumClasses];     // Array indexed by size-class

  // We sample allocations, biased by the size of the allocation
  uint32_t      rnd_;                   // Cheap random number generator
  size_t        bytes_until_sample_;    // Bytes until we sample next

  // Allocate a new heap. REQUIRES: pageheap_lock is held.
  static inline TCMalloc_ThreadCache* NewHeap(ThreadIdentifier tid);

  // Use only as pthread thread-specific destructor function.
  static void DestroyThreadCache(void* ptr);
 public:
  // All ThreadCache objects are kept in a linked list (for stats collection)
  TCMalloc_ThreadCache* next_;
  TCMalloc_ThreadCache* prev_;

  void Init(ThreadIdentifier tid);
  void Cleanup();

  // Accessors (mostly just for printing stats)
  int freelist_length(size_t cl) const { return list_[cl].length(); }

  // Total byte size in cache
  size_t Size() const { return size_; }

  ALWAYS_INLINE void* Allocate(size_t size);
  void Deallocate(void* ptr, size_t size_class);

  ALWAYS_INLINE void FetchFromCentralCache(size_t cl, size_t allocationSize);
  void ReleaseToCentralCache(size_t cl, int N);
  void Scavenge();
  void Print() const;

  // Record allocation of "k" bytes.  Return true iff allocation
  // should be sampled
  bool SampleAllocation(size_t k);

  // Pick next sampling point
  void PickNextSample(size_t k);

  static void                  InitModule();
  static void                  InitTSD();
  static TCMalloc_ThreadCache* GetThreadHeap();
  static TCMalloc_ThreadCache* GetCache();
  static TCMalloc_ThreadCache* GetCacheIfPresent();
  static TCMalloc_ThreadCache* CreateCacheIfNecessary();
  static void                  DeleteCache(TCMalloc_ThreadCache* heap);
  static void                  BecomeIdle();
  static void                  RecomputeThreadCacheSize();

#ifdef WTF_CHANGES
  template <class Finder, class Reader>
  void enumerateFreeObjects(Finder& finder, const Reader& reader)
  {
      for (unsigned sizeClass = 0; sizeClass < kNumClasses; sizeClass++)
          list_[sizeClass].enumerateFreeObjects(finder, reader);
  }
#endif
};

//-------------------------------------------------------------------
// Data kept per size-class in central cache
//-------------------------------------------------------------------

class TCMalloc_Central_FreeList {
 public:
  void Init(size_t cl);

  // These methods all do internal locking.

  // Insert the specified range into the central freelist.  N is the number of
  // elements in the range.
  void InsertRange(void *start, void *end, int N);

  // Returns the actual number of fetched elements into N.
  void RemoveRange(void **start, void **end, int *N);

  // Returns the number of free objects in cache.
  size_t length() {
    SpinLockHolder h(&lock_);
    return counter_;
  }

  // Returns the number of free objects in the transfer cache.
  int tc_length() {
    SpinLockHolder h(&lock_);
    return used_slots_ * num_objects_to_move[size_class_];
  }

#ifdef WTF_CHANGES
  template <class Finder, class Reader>
  void enumerateFreeObjects(Finder& finder, const Reader& reader, TCMalloc_Central_FreeList* remoteCentralFreeList)
  {
    for (Span* span = &empty_; span && span != &empty_; span = (span->next ? reader(span->next) : 0))
      ASSERT(!span->objects);

    ASSERT(!nonempty_.objects);
    static const ptrdiff_t nonemptyOffset = reinterpret_cast<const char*>(&nonempty_) - reinterpret_cast<const char*>(this);

    Span* remoteNonempty = reinterpret_cast<Span*>(reinterpret_cast<char*>(remoteCentralFreeList) + nonemptyOffset);
    Span* remoteSpan = nonempty_.next;

    for (Span* span = reader(remoteSpan); span && remoteSpan != remoteNonempty; remoteSpan = span->next, span = (span->next ? reader(span->next) : 0)) {
      for (void* nextObject = span->objects; nextObject; nextObject = reader.nextEntryInLinkedList(reinterpret_cast<void**>(nextObject)))
        finder.visit(nextObject);
    }
  }
#endif

 private:
  // REQUIRES: lock_ is held
  // Remove object from cache and return.
  // Return NULL if no free entries in cache.
  void* FetchFromSpans();

  // REQUIRES: lock_ is held
  // Remove object from cache and return.  Fetches
  // from pageheap if cache is empty.  Only returns
  // NULL on allocation failure.
  void* FetchFromSpansSafe();

  // REQUIRES: lock_ is held
  // Release a linked list of objects to spans.
  // May temporarily release lock_.
  void ReleaseListToSpans(void *start);

  // REQUIRES: lock_ is held
  // Release an object to spans.
  // May temporarily release lock_.
  ALWAYS_INLINE void ReleaseToSpans(void* object);

  // REQUIRES: lock_ is held
  // Populate cache by fetching from the page heap.
  // May temporarily release lock_.
  ALWAYS_INLINE void Populate();

  // REQUIRES: lock is held.
  // Tries to make room for a TCEntry.  If the cache is full it will try to
  // expand it at the cost of some other cache size.  Return false if there is
  // no space.
  bool MakeCacheSpace();

  // REQUIRES: lock_ for locked_size_class is held.
  // Picks a "random" size class to steal TCEntry slot from.  In reality it
  // just iterates over the sizeclasses but does so without taking a lock.
  // Returns true on success.
  // May temporarily lock a "random" size class.
  static ALWAYS_INLINE bool EvictRandomSizeClass(size_t locked_size_class, bool force);

  // REQUIRES: lock_ is *not* held.
  // Tries to shrink the Cache.  If force is true it will relase objects to
  // spans if it allows it to shrink the cache.  Return false if it failed to
  // shrink the cache.  Decrements cache_size_ on succeess.
  // May temporarily take lock_.  If it takes lock_, the locked_size_class
  // lock is released to the thread from holding two size class locks
  // concurrently which could lead to a deadlock.
  bool ShrinkCache(int locked_size_class, bool force);

  // This lock protects all the data members.  cached_entries and cache_size_
  // may be looked at without holding the lock.
  SpinLock lock_;

  // We keep linked lists of empty and non-empty spans.
  size_t   size_class_;     // My size class
  Span     empty_;          // Dummy header for list of empty spans
  Span     nonempty_;       // Dummy header for list of non-empty spans
  size_t   counter_;        // Number of free objects in cache entry

  // Here we reserve space for TCEntry cache slots.  Since one size class can
  // end up getting all the TCEntries quota in the system we just preallocate
  // sufficient number of entries here.
  TCEntry tc_slots_[kNumTransferEntries];

  // Number of currently used cached entries in tc_slots_.  This variable is
  // updated under a lock but can be read without one.
  int32_t used_slots_;
  // The current number of slots for this size class.  This is an
  // adaptive value that is increased if there is lots of traffic
  // on a given size class.
  int32_t cache_size_;
};

// Pad each CentralCache object to multiple of 64 bytes
class TCMalloc_Central_FreeListPadded : public TCMalloc_Central_FreeList {
 private:
  char pad_[(64 - (sizeof(TCMalloc_Central_FreeList) % 64)) % 64];
};

//-------------------------------------------------------------------
// Global variables
//-------------------------------------------------------------------

// Central cache -- a collection of free-lists, one per size-class.
// We have a separate lock per free-list to reduce contention.
static TCMalloc_Central_FreeListPadded central_cache[kNumClasses];

// Page-level allocator
static AllocAlignmentInteger pageheap_memory[(sizeof(TCMalloc_PageHeap) + sizeof(AllocAlignmentInteger) - 1) / sizeof(AllocAlignmentInteger)];
static bool phinited = false;

// Avoid extra level of indirection by making "pageheap" be just an alias
// of pageheap_memory.
typedef union {
    void* m_memory;
    TCMalloc_PageHeap* m_pageHeap;
} PageHeapUnion;

static inline TCMalloc_PageHeap* getPageHeap()
{
    PageHeapUnion u = { &pageheap_memory[0] };
    return u.m_pageHeap;
}

#define pageheap getPageHeap()

#if USE_BACKGROUND_THREAD_TO_SCAVENGE_MEMORY

#if HAVE(DISPATCH_H) || OS(WINDOWS)

void TCMalloc_PageHeap::periodicScavenge()
{
    SpinLockHolder h(&pageheap_lock);
    pageheap->scavenge();

    if (shouldScavenge()) {
        rescheduleScavenger();
        return;
    }

    suspendScavenger();
}

ALWAYS_INLINE void TCMalloc_PageHeap::signalScavenger()
{
    ASSERT(pageheap_lock.IsHeld());
    if (isScavengerSuspended() && shouldScavenge())
        scheduleScavenger();
}

#else

void TCMalloc_PageHeap::scavengerThread()
{
#if HAVE(PTHREAD_SETNAME_NP)
  pthread_setname_np("JavaScriptCore: FastMalloc scavenger");
#endif

  while (1) {
      if (!shouldScavenge()) {
          pthread_mutex_lock(&m_scavengeMutex);
          m_scavengeThreadActive = false;
          // Block until there are enough free committed pages to release back to the system.
          pthread_cond_wait(&m_scavengeCondition, &m_scavengeMutex);
          m_scavengeThreadActive = true;
          pthread_mutex_unlock(&m_scavengeMutex);
      }
      sleep(kScavengeDelayInSeconds);
      {
          SpinLockHolder h(&pageheap_lock);
          pageheap->scavenge();
      }
  }
}

#endif

#endif

// If TLS is available, we also store a copy
// of the per-thread object in a __thread variable
// since __thread variables are faster to read
// than pthread_getspecific().  We still need
// pthread_setspecific() because __thread
// variables provide no way to run cleanup
// code when a thread is destroyed.
#ifdef HAVE_TLS
static __thread TCMalloc_ThreadCache *threadlocal_heap;
#endif
// Thread-specific key.  Initialization here is somewhat tricky
// because some Linux startup code invokes malloc() before it
// is in a good enough state to handle pthread_keycreate().
// Therefore, we use TSD keys only after tsd_inited is set to true.
// Until then, we use a slow path to get the heap object.
static bool tsd_inited = false;
#if USE(PTHREAD_GETSPECIFIC_DIRECT)
static const pthread_key_t heap_key = __PTK_FRAMEWORK_JAVASCRIPTCORE_KEY0;
#else
static pthread_key_t heap_key;
#endif
#if OS(WINDOWS)
DWORD tlsIndex = TLS_OUT_OF_INDEXES;
#endif

static ALWAYS_INLINE void setThreadHeap(TCMalloc_ThreadCache* heap)
{
#if USE(PTHREAD_GETSPECIFIC_DIRECT)
    // Can't have two libraries both doing this in the same process,
    // so check and make this crash right away.
    if (pthread_getspecific(heap_key))
        CRASH();
#endif

    // Still do pthread_setspecific even if there's an alternate form
    // of thread-local storage in use, to benefit from the delete callback.
    pthread_setspecific(heap_key, heap);

#if OS(WINDOWS)
    TlsSetValue(tlsIndex, heap);
#endif
}

// Allocator for thread heaps
static PageHeapAllocator<TCMalloc_ThreadCache> threadheap_allocator;

// Linked list of heap objects.  Protected by pageheap_lock.
static TCMalloc_ThreadCache* thread_heaps = NULL;
static int thread_heap_count = 0;

// Overall thread cache size.  Protected by pageheap_lock.
static size_t overall_thread_cache_size = kDefaultOverallThreadCacheSize;

// Global per-thread cache size.  Writes are protected by
// pageheap_lock.  Reads are done without any locking, which should be
// fine as long as size_t can be written atomically and we don't place
// invariants between this variable and other pieces of state.
static volatile size_t per_thread_cache_size = kMaxThreadCacheSize;

//-------------------------------------------------------------------
// Central cache implementation
//-------------------------------------------------------------------

void TCMalloc_Central_FreeList::Init(size_t cl) {
  lock_.Init();
  size_class_ = cl;
  DLL_Init(&empty_);
  DLL_Init(&nonempty_);
  counter_ = 0;

  cache_size_ = 1;
  used_slots_ = 0;
  ASSERT(cache_size_ <= kNumTransferEntries);
}

void TCMalloc_Central_FreeList::ReleaseListToSpans(void* start) {
  while (start) {
    void *next = SLL_Next(start);
    ReleaseToSpans(start);
    start = next;
  }
}

ALWAYS_INLINE void TCMalloc_Central_FreeList::ReleaseToSpans(void* object) {
  const PageID p = reinterpret_cast<uintptr_t>(object) >> kPageShift;
  Span* span = pageheap->GetDescriptor(p);
  ASSERT(span != NULL);
  ASSERT(span->refcount > 0);

  // If span is empty, move it to non-empty list
  if (span->objects == NULL) {
    DLL_Remove(span);
    DLL_Prepend(&nonempty_, span);
    Event(span, 'N', 0);
  }

  // The following check is expensive, so it is disabled by default
  if (false) {
    // Check that object does not occur in list
    unsigned got = 0;
    for (void* p = span->objects; p != NULL; p = *((void**) p)) {
      ASSERT(p != object);
      got++;
    }
    ASSERT(got + span->refcount ==
           (span->length<<kPageShift)/ByteSizeForClass(span->sizeclass));
  }

  counter_++;
  span->refcount--;
  if (span->refcount == 0) {
    Event(span, '#', 0);
    counter_ -= (span->length<<kPageShift) / ByteSizeForClass(span->sizeclass);
    DLL_Remove(span);

    // Release central list lock while operating on pageheap
    lock_.Unlock();
    {
      SpinLockHolder h(&pageheap_lock);
      pageheap->Delete(span);
    }
    lock_.Lock();
  } else {
    *(reinterpret_cast<void**>(object)) = span->objects;
    span->objects = object;
  }
}

ALWAYS_INLINE bool TCMalloc_Central_FreeList::EvictRandomSizeClass(
    size_t locked_size_class, bool force) {
  static int race_counter = 0;
  int t = race_counter++;  // Updated without a lock, but who cares.
  if (t >= static_cast<int>(kNumClasses)) {
    while (t >= static_cast<int>(kNumClasses)) {
      t -= kNumClasses;
    }
    race_counter = t;
  }
  ASSERT(t >= 0);
  ASSERT(t < static_cast<int>(kNumClasses));
  if (t == static_cast<int>(locked_size_class)) return false;
  return central_cache[t].ShrinkCache(static_cast<int>(locked_size_class), force);
}

bool TCMalloc_Central_FreeList::MakeCacheSpace() {
  // Is there room in the cache?
  if (used_slots_ < cache_size_) return true;
  // Check if we can expand this cache?
  if (cache_size_ == kNumTransferEntries) return false;
  // Ok, we'll try to grab an entry from some other size class.
  if (EvictRandomSizeClass(size_class_, false) ||
      EvictRandomSizeClass(size_class_, true)) {
    // Succeeded in evicting, we're going to make our cache larger.
    cache_size_++;
    return true;
  }
  return false;
}


namespace {
class LockInverter {
 private:
  SpinLock *held_, *temp_;
 public:
  inline explicit LockInverter(SpinLock* held, SpinLock *temp)
    : held_(held), temp_(temp) { held_->Unlock(); temp_->Lock(); }
  inline ~LockInverter() { temp_->Unlock(); held_->Lock();  }
};
}

bool TCMalloc_Central_FreeList::ShrinkCache(int locked_size_class, bool force) {
  // Start with a quick check without taking a lock.
  if (cache_size_ == 0) return false;
  // We don't evict from a full cache unless we are 'forcing'.
  if (force == false && used_slots_ == cache_size_) return false;

  // Grab lock, but first release the other lock held by this thread.  We use
  // the lock inverter to ensure that we never hold two size class locks
  // concurrently.  That can create a deadlock because there is no well
  // defined nesting order.
  LockInverter li(&central_cache[locked_size_class].lock_, &lock_);
  ASSERT(used_slots_ <= cache_size_);
  ASSERT(0 <= cache_size_);
  if (cache_size_ == 0) return false;
  if (used_slots_ == cache_size_) {
    if (force == false) return false;
    // ReleaseListToSpans releases the lock, so we have to make all the
    // updates to the central list before calling it.
    cache_size_--;
    used_slots_--;
    ReleaseListToSpans(tc_slots_[used_slots_].head);
    return true;
  }
  cache_size_--;
  return true;
}

void TCMalloc_Central_FreeList::InsertRange(void *start, void *end, int N) {
  SpinLockHolder h(&lock_);
  if (N == num_objects_to_move[size_class_] &&
    MakeCacheSpace()) {
    int slot = used_slots_++;
    ASSERT(slot >=0);
    ASSERT(slot < kNumTransferEntries);
    TCEntry *entry = &tc_slots_[slot];
    entry->head = start;
    entry->tail = end;
    return;
  }
  ReleaseListToSpans(start);
}

void TCMalloc_Central_FreeList::RemoveRange(void **start, void **end, int *N) {
  int num = *N;
  ASSERT(num > 0);

  SpinLockHolder h(&lock_);
  if (num == num_objects_to_move[size_class_] && used_slots_ > 0) {
    int slot = --used_slots_;
    ASSERT(slot >= 0);
    TCEntry *entry = &tc_slots_[slot];
    *start = entry->head;
    *end = entry->tail;
    return;
  }

  // TODO: Prefetch multiple TCEntries?
  void *tail = FetchFromSpansSafe();
  if (!tail) {
    // We are completely out of memory.
    *start = *end = NULL;
    *N = 0;
    return;
  }

  SLL_SetNext(tail, NULL);
  void *head = tail;
  int count = 1;
  while (count < num) {
    void *t = FetchFromSpans();
    if (!t) break;
    SLL_Push(&head, t);
    count++;
  }
  *start = head;
  *end = tail;
  *N = count;
}


void* TCMalloc_Central_FreeList::FetchFromSpansSafe() {
  void *t = FetchFromSpans();
  if (!t) {
    Populate();
    t = FetchFromSpans();
  }
  return t;
}

void* TCMalloc_Central_FreeList::FetchFromSpans() {
  if (DLL_IsEmpty(&nonempty_)) return NULL;
  Span* span = nonempty_.next;

  ASSERT(span->objects != NULL);
  ASSERT_SPAN_COMMITTED(span);
  span->refcount++;
  void* result = span->objects;
  span->objects = *(reinterpret_cast<void**>(result));
  if (span->objects == NULL) {
    // Move to empty list
    DLL_Remove(span);
    DLL_Prepend(&empty_, span);
    Event(span, 'E', 0);
  }
  counter_--;
  return result;
}

// Fetch memory from the system and add to the central cache freelist.
ALWAYS_INLINE void TCMalloc_Central_FreeList::Populate() {
  // Release central list lock while operating on pageheap
  lock_.Unlock();
  const size_t npages = class_to_pages[size_class_];

  Span* span;
  {
    SpinLockHolder h(&pageheap_lock);
    span = pageheap->New(npages);
    if (span) pageheap->RegisterSizeClass(span, size_class_);
  }
  if (span == NULL) {
#if HAVE(ERRNO_H)
    MESSAGE("allocation failed: %d\n", errno);
#elif OS(WINDOWS)
    MESSAGE("allocation failed: %d\n", ::GetLastError());
#else
    MESSAGE("allocation failed\n");
#endif
    lock_.Lock();
    return;
  }
  ASSERT_SPAN_COMMITTED(span);
  ASSERT(span->length == npages);
  // Cache sizeclass info eagerly.  Locking is not necessary.
  // (Instead of being eager, we could just replace any stale info
  // about this span, but that seems to be no better in practice.)
  for (size_t i = 0; i < npages; i++) {
    pageheap->CacheSizeClass(span->start + i, size_class_);
  }

  // Split the block into pieces and add to the free-list
  // TODO: coloring of objects to avoid cache conflicts?
  void** tail = &span->objects;
  char* ptr = reinterpret_cast<char*>(span->start << kPageShift);
  char* limit = ptr + (npages << kPageShift);
  const size_t size = ByteSizeForClass(size_class_);
  int num = 0;
  char* nptr;
  while ((nptr = ptr + size) <= limit) {
    *tail = ptr;
    tail = reinterpret_cast_ptr<void**>(ptr);
    ptr = nptr;
    num++;
  }
  ASSERT(ptr <= limit);
  *tail = NULL;
  span->refcount = 0; // No sub-object in use yet

  // Add span to list of non-empty spans
  lock_.Lock();
  DLL_Prepend(&nonempty_, span);
  counter_ += num;
}

//-------------------------------------------------------------------
// TCMalloc_ThreadCache implementation
//-------------------------------------------------------------------

inline bool TCMalloc_ThreadCache::SampleAllocation(size_t k) {
  if (bytes_until_sample_ < k) {
    PickNextSample(k);
    return true;
  } else {
    bytes_until_sample_ -= k;
    return false;
  }
}

void TCMalloc_ThreadCache::Init(ThreadIdentifier tid) {
  size_ = 0;
  next_ = NULL;
  prev_ = NULL;
  tid_  = tid;
  in_setspecific_ = false;
  for (size_t cl = 0; cl < kNumClasses; ++cl) {
    list_[cl].Init();
  }

  // Initialize RNG -- run it for a bit to get to good values
  bytes_until_sample_ = 0;
  rnd_ = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(this));
  for (int i = 0; i < 100; i++) {
    PickNextSample(static_cast<size_t>(FLAGS_tcmalloc_sample_parameter * 2));
  }
}

void TCMalloc_ThreadCache::Cleanup() {
  // Put unused memory back into central cache
  for (size_t cl = 0; cl < kNumClasses; ++cl) {
    if (list_[cl].length() > 0) {
      ReleaseToCentralCache(cl, list_[cl].length());
    }
  }
}

ALWAYS_INLINE void* TCMalloc_ThreadCache::Allocate(size_t size) {
  ASSERT(size <= kMaxSize);
  const size_t cl = SizeClass(size);
  FreeList* list = &list_[cl];
  size_t allocationSize = ByteSizeForClass(cl);
  if (list->empty()) {
    FetchFromCentralCache(cl, allocationSize);
    if (list->empty()) return NULL;
  }
  size_ -= allocationSize;
  return list->Pop();
}

inline void TCMalloc_ThreadCache::Deallocate(void* ptr, size_t cl) {
  size_ += ByteSizeForClass(cl);
  FreeList* list = &list_[cl];
  list->Push(ptr);
  // If enough data is free, put back into central cache
  if (list->length() > kMaxFreeListLength) {
    ReleaseToCentralCache(cl, num_objects_to_move[cl]);
  }
  if (size_ >= per_thread_cache_size) Scavenge();
}

// Remove some objects of class "cl" from central cache and add to thread heap
ALWAYS_INLINE void TCMalloc_ThreadCache::FetchFromCentralCache(size_t cl, size_t allocationSize) {
  int fetch_count = num_objects_to_move[cl];
  void *start, *end;
  central_cache[cl].RemoveRange(&start, &end, &fetch_count);
  list_[cl].PushRange(fetch_count, start, end);
  size_ += allocationSize * fetch_count;
}

// Remove some objects of class "cl" from thread heap and add to central cache
inline void TCMalloc_ThreadCache::ReleaseToCentralCache(size_t cl, int N) {
  ASSERT(N > 0);
  FreeList* src = &list_[cl];
  if (N > src->length()) N = src->length();
  size_ -= N*ByteSizeForClass(cl);

  // We return prepackaged chains of the correct size to the central cache.
  // TODO: Use the same format internally in the thread caches?
  int batch_size = num_objects_to_move[cl];
  while (N > batch_size) {
    void *tail, *head;
    src->PopRange(batch_size, &head, &tail);
    central_cache[cl].InsertRange(head, tail, batch_size);
    N -= batch_size;
  }
  void *tail, *head;
  src->PopRange(N, &head, &tail);
  central_cache[cl].InsertRange(head, tail, N);
}

// Release idle memory to the central cache
inline void TCMalloc_ThreadCache::Scavenge() {
  // If the low-water mark for the free list is L, it means we would
  // not have had to allocate anything from the central cache even if
  // we had reduced the free list size by L.  We aim to get closer to
  // that situation by dropping L/2 nodes from the free list.  This
  // may not release much memory, but if so we will call scavenge again
  // pretty soon and the low-water marks will be high on that call.
  //int64 start = CycleClock::Now();

  for (size_t cl = 0; cl < kNumClasses; cl++) {
    FreeList* list = &list_[cl];
    const int lowmark = list->lowwatermark();
    if (lowmark > 0) {
      const int drop = (lowmark > 1) ? lowmark/2 : 1;
      ReleaseToCentralCache(cl, drop);
    }
    list->clear_lowwatermark();
  }

  //int64 finish = CycleClock::Now();
  //CycleTimer ct;
  //MESSAGE("GC: %.0f ns\n", ct.CyclesToUsec(finish-start)*1000.0);
}

void TCMalloc_ThreadCache::PickNextSample(size_t k) {
  // Make next "random" number
  // x^32+x^22+x^2+x^1+1 is a primitive polynomial for random numbers
  static const uint32_t kPoly = (1 << 22) | (1 << 2) | (1 << 1) | (1 << 0);
  uint32_t r = rnd_;
  rnd_ = (r << 1) ^ ((static_cast<int32_t>(r) >> 31) & kPoly);

  // Next point is "rnd_ % (sample_period)".  I.e., average
  // increment is "sample_period/2".
  const int flag_value = static_cast<int>(FLAGS_tcmalloc_sample_parameter);
  static int last_flag_value = -1;

  if (flag_value != last_flag_value) {
    SpinLockHolder h(&sample_period_lock);
    int i;
    for (i = 0; i < (static_cast<int>(sizeof(primes_list)/sizeof(primes_list[0])) - 1); i++) {
      if (primes_list[i] >= flag_value) {
        break;
      }
    }
    sample_period = primes_list[i];
    last_flag_value = flag_value;
  }

  bytes_until_sample_ += rnd_ % sample_period;

  if (k > (static_cast<size_t>(-1) >> 2)) {
    // If the user has asked for a huge allocation then it is possible
    // for the code below to loop infinitely.  Just return (note that
    // this throws off the sampling accuracy somewhat, but a user who
    // is allocating more than 1G of memory at a time can live with a
    // minor inaccuracy in profiling of small allocations, and also
    // would rather not wait for the loop below to terminate).
    return;
  }

  while (bytes_until_sample_ < k) {
    // Increase bytes_until_sample_ by enough average sampling periods
    // (sample_period >> 1) to allow us to sample past the current
    // allocation.
    bytes_until_sample_ += (sample_period >> 1);
  }

  bytes_until_sample_ -= k;
}

void TCMalloc_ThreadCache::InitModule() {
  // There is a slight potential race here because of double-checked
  // locking idiom.  However, as long as the program does a small
  // allocation before switching to multi-threaded mode, we will be
  // fine.  We increase the chances of doing such a small allocation
  // by doing one in the constructor of the module_enter_exit_hook
  // object declared below.
  SpinLockHolder h(&pageheap_lock);
  if (!phinited) {
#ifdef WTF_CHANGES
    InitTSD();
#endif
    InitSizeClasses();
    threadheap_allocator.Init();
    span_allocator.Init();
    span_allocator.New(); // Reduce cache conflicts
    span_allocator.New(); // Reduce cache conflicts
    stacktrace_allocator.Init();
    DLL_Init(&sampled_objects);
    for (size_t i = 0; i < kNumClasses; ++i) {
      central_cache[i].Init(i);
    }
    pageheap->init();
    phinited = 1;
#if defined(WTF_CHANGES) && OS(DARWIN)
    FastMallocZone::init();
#endif
  }
}

inline TCMalloc_ThreadCache* TCMalloc_ThreadCache::NewHeap(ThreadIdentifier tid) {
  // Create the heap and add it to the linked list
  TCMalloc_ThreadCache *heap = threadheap_allocator.New();
  heap->Init(tid);
  heap->next_ = thread_heaps;
  heap->prev_ = NULL;
  if (thread_heaps != NULL) thread_heaps->prev_ = heap;
  thread_heaps = heap;
  thread_heap_count++;
  RecomputeThreadCacheSize();
  return heap;
}

inline TCMalloc_ThreadCache* TCMalloc_ThreadCache::GetThreadHeap() {
#ifdef HAVE_TLS
    // __thread is faster, but only when the kernel supports it
  if (KernelSupportsTLS())
    return threadlocal_heap;
#elif OS(WINDOWS)
    return static_cast<TCMalloc_ThreadCache*>(TlsGetValue(tlsIndex));
#else
    return static_cast<TCMalloc_ThreadCache*>(pthread_getspecific(heap_key));
#endif
}

inline TCMalloc_ThreadCache* TCMalloc_ThreadCache::GetCache() {
  TCMalloc_ThreadCache* ptr = NULL;
  if (!tsd_inited) {
    InitModule();
  } else {
    ptr = GetThreadHeap();
  }
  if (ptr == NULL) ptr = CreateCacheIfNecessary();
  return ptr;
}

// In deletion paths, we do not try to create a thread-cache.  This is
// because we may be in the thread destruction code and may have
// already cleaned up the cache for this thread.
inline TCMalloc_ThreadCache* TCMalloc_ThreadCache::GetCacheIfPresent() {
  if (!tsd_inited) return NULL;
  void* const p = GetThreadHeap();
  return reinterpret_cast<TCMalloc_ThreadCache*>(p);
}

void TCMalloc_ThreadCache::InitTSD() {
  ASSERT(!tsd_inited);
#if USE(PTHREAD_GETSPECIFIC_DIRECT)
  pthread_key_init_np(heap_key, DestroyThreadCache);
#else
  pthread_key_create(&heap_key, DestroyThreadCache);
#endif
#if OS(WINDOWS)
  tlsIndex = TlsAlloc();
#endif
  tsd_inited = true;
    
#if !OS(WINDOWS)
  // We may have used a fake pthread_t for the main thread.  Fix it.
  pthread_t zero;
  memset(&zero, 0, sizeof(zero));
#endif
#ifndef WTF_CHANGES
  SpinLockHolder h(&pageheap_lock);
#else
  ASSERT(pageheap_lock.IsHeld());
#endif
  for (TCMalloc_ThreadCache* h = thread_heaps; h != NULL; h = h->next_) {
#if OS(WINDOWS)
    if (h->tid_ == 0) {
      h->tid_ = GetCurrentThreadId();
    }
#else
    if (pthread_equal(h->tid_, zero)) {
      h->tid_ = pthread_self();
    }
#endif
  }
}

TCMalloc_ThreadCache* TCMalloc_ThreadCache::CreateCacheIfNecessary() {
  // Initialize per-thread data if necessary
  TCMalloc_ThreadCache* heap = NULL;
  {
    SpinLockHolder h(&pageheap_lock);

#if OS(WINDOWS)
    DWORD me;
    if (!tsd_inited) {
      me = 0;
    } else {
      me = GetCurrentThreadId();
    }
#else
    // Early on in glibc's life, we cannot even call pthread_self()
    pthread_t me;
    if (!tsd_inited) {
      memset(&me, 0, sizeof(me));
    } else {
      me = pthread_self();
    }
#endif

    // This may be a recursive malloc call from pthread_setspecific()
    // In that case, the heap for this thread has already been created
    // and added to the linked list.  So we search for that first.
    for (TCMalloc_ThreadCache* h = thread_heaps; h != NULL; h = h->next_) {
#if OS(WINDOWS)
      if (h->tid_ == me) {
#else
      if (pthread_equal(h->tid_, me)) {
#endif
        heap = h;
        break;
      }
    }

    if (heap == NULL) heap = NewHeap(me);
  }

  // We call pthread_setspecific() outside the lock because it may
  // call malloc() recursively.  The recursive call will never get
  // here again because it will find the already allocated heap in the
  // linked list of heaps.
  if (!heap->in_setspecific_ && tsd_inited) {
    heap->in_setspecific_ = true;
    setThreadHeap(heap);
  }
  return heap;
}

void TCMalloc_ThreadCache::BecomeIdle() {
  if (!tsd_inited) return;              // No caches yet
  TCMalloc_ThreadCache* heap = GetThreadHeap();
  if (heap == NULL) return;             // No thread cache to remove
  if (heap->in_setspecific_) return;    // Do not disturb the active caller

  heap->in_setspecific_ = true;
  setThreadHeap(NULL);
#ifdef HAVE_TLS
  // Also update the copy in __thread
  threadlocal_heap = NULL;
#endif
  heap->in_setspecific_ = false;
  if (GetThreadHeap() == heap) {
    // Somehow heap got reinstated by a recursive call to malloc
    // from pthread_setspecific.  We give up in this case.
    return;
  }

  // We can now get rid of the heap
  DeleteCache(heap);
}

void TCMalloc_ThreadCache::DestroyThreadCache(void* ptr) {
  // Note that "ptr" cannot be NULL since pthread promises not
  // to invoke the destructor on NULL values, but for safety,
  // we check anyway.
  if (ptr == NULL) return;
#ifdef HAVE_TLS
  // Prevent fast path of GetThreadHeap() from returning heap.
  threadlocal_heap = NULL;
#endif
  DeleteCache(reinterpret_cast<TCMalloc_ThreadCache*>(ptr));
}

void TCMalloc_ThreadCache::DeleteCache(TCMalloc_ThreadCache* heap) {
  // Remove all memory from heap
  heap->Cleanup();

  // Remove from linked list
  SpinLockHolder h(&pageheap_lock);
  if (heap->next_ != NULL) heap->next_->prev_ = heap->prev_;
  if (heap->prev_ != NULL) heap->prev_->next_ = heap->next_;
  if (thread_heaps == heap) thread_heaps = heap->next_;
  thread_heap_count--;
  RecomputeThreadCacheSize();

  threadheap_allocator.Delete(heap);
}

void TCMalloc_ThreadCache::RecomputeThreadCacheSize() {
  // Divide available space across threads
  int n = thread_heap_count > 0 ? thread_heap_count : 1;
  size_t space = overall_thread_cache_size / n;

  // Limit to allowed range
  if (space < kMinThreadCacheSize) space = kMinThreadCacheSize;
  if (space > kMaxThreadCacheSize) space = kMaxThreadCacheSize;

  per_thread_cache_size = space;
}

void TCMalloc_ThreadCache::Print() const {
  for (size_t cl = 0; cl < kNumClasses; ++cl) {
    MESSAGE("      %5" PRIuS " : %4d len; %4d lo\n",
            ByteSizeForClass(cl),
            list_[cl].length(),
            list_[cl].lowwatermark());
  }
}

// Extract interesting stats
struct TCMallocStats {
  uint64_t system_bytes;        // Bytes alloced from system
  uint64_t thread_bytes;        // Bytes in thread caches
  uint64_t central_bytes;       // Bytes in central cache
  uint64_t transfer_bytes;      // Bytes in central transfer cache
  uint64_t pageheap_bytes;      // Bytes in page heap
  uint64_t metadata_bytes;      // Bytes alloced for metadata
};

#ifndef WTF_CHANGES
// Get stats into "r".  Also get per-size-class counts if class_count != NULL
static void ExtractStats(TCMallocStats* r, uint64_t* class_count) {
  r->central_bytes = 0;
  r->transfer_bytes = 0;
  for (int cl = 0; cl < kNumClasses; ++cl) {
    const int length = central_cache[cl].length();
    const int tc_length = central_cache[cl].tc_length();
    r->central_bytes += static_cast<uint64_t>(ByteSizeForClass(cl)) * length;
    r->transfer_bytes +=
      static_cast<uint64_t>(ByteSizeForClass(cl)) * tc_length;
    if (class_count) class_count[cl] = length + tc_length;
  }

  // Add stats from per-thread heaps
  r->thread_bytes = 0;
  { // scope
    SpinLockHolder h(&pageheap_lock);
    for (TCMalloc_ThreadCache* h = thread_heaps; h != NULL; h = h->next_) {
      r->thread_bytes += h->Size();
      if (class_count) {
        for (size_t cl = 0; cl < kNumClasses; ++cl) {
          class_count[cl] += h->freelist_length(cl);
        }
      }
    }
  }

  { //scope
    SpinLockHolder h(&pageheap_lock);
    r->system_bytes = pageheap->SystemBytes();
    r->metadata_bytes = metadata_system_bytes;
    r->pageheap_bytes = pageheap->FreeBytes();
  }
}
#endif

#ifndef WTF_CHANGES
// WRITE stats to "out"
static void DumpStats(TCMalloc_Printer* out, int level) {
  TCMallocStats stats;
  uint64_t class_count[kNumClasses];
  ExtractStats(&stats, (level >= 2 ? class_count : NULL));

  if (level >= 2) {
    out->printf("------------------------------------------------\n");
    uint64_t cumulative = 0;
    for (int cl = 0; cl < kNumClasses; ++cl) {
      if (class_count[cl] > 0) {
        uint64_t class_bytes = class_count[cl] * ByteSizeForClass(cl);
        cumulative += class_bytes;
        out->printf("class %3d [ %8" PRIuS " bytes ] : "
                "%8" PRIu64 " objs; %5.1f MB; %5.1f cum MB\n",
                cl, ByteSizeForClass(cl),
                class_count[cl],
                class_bytes / 1048576.0,
                cumulative / 1048576.0);
      }
    }

    SpinLockHolder h(&pageheap_lock);
    pageheap->Dump(out);
  }

  const uint64_t bytes_in_use = stats.system_bytes
                                - stats.pageheap_bytes
                                - stats.central_bytes
                                - stats.transfer_bytes
                                - stats.thread_bytes;

  out->printf("------------------------------------------------\n"
              "MALLOC: %12" PRIu64 " Heap size\n"
              "MALLOC: %12" PRIu64 " Bytes in use by application\n"
              "MALLOC: %12" PRIu64 " Bytes free in page heap\n"
              "MALLOC: %12" PRIu64 " Bytes free in central cache\n"
              "MALLOC: %12" PRIu64 " Bytes free in transfer cache\n"
              "MALLOC: %12" PRIu64 " Bytes free in thread caches\n"
              "MALLOC: %12" PRIu64 " Spans in use\n"
              "MALLOC: %12" PRIu64 " Thread heaps in use\n"
              "MALLOC: %12" PRIu64 " Metadata allocated\n"
              "------------------------------------------------\n",
              stats.system_bytes,
              bytes_in_use,
              stats.pageheap_bytes,
              stats.central_bytes,
              stats.transfer_bytes,
              stats.thread_bytes,
              uint64_t(span_allocator.inuse()),
              uint64_t(threadheap_allocator.inuse()),
              stats.metadata_bytes);
}

static void PrintStats(int level) {
  const int kBufferSize = 16 << 10;
  char* buffer = new char[kBufferSize];
  TCMalloc_Printer printer(buffer, kBufferSize);
  DumpStats(&printer, level);
  write(STDERR_FILENO, buffer, strlen(buffer));
  delete[] buffer;
}

static void** DumpStackTraces() {
  // Count how much space we need
  int needed_slots = 0;
  {
    SpinLockHolder h(&pageheap_lock);
    for (Span* s = sampled_objects.next; s != &sampled_objects; s = s->next) {
      StackTrace* stack = reinterpret_cast<StackTrace*>(s->objects);
      needed_slots += 3 + stack->depth;
    }
    needed_slots += 100;            // Slop in case sample grows
    needed_slots += needed_slots/8; // An extra 12.5% slop
  }

  void** result = new void*[needed_slots];
  if (result == NULL) {
    MESSAGE("tcmalloc: could not allocate %d slots for stack traces\n",
            needed_slots);
    return NULL;
  }

  SpinLockHolder h(&pageheap_lock);
  int used_slots = 0;
  for (Span* s = sampled_objects.next; s != &sampled_objects; s = s->next) {
    ASSERT(used_slots < needed_slots);  // Need to leave room for terminator
    StackTrace* stack = reinterpret_cast<StackTrace*>(s->objects);
    if (used_slots + 3 + stack->depth >= needed_slots) {
      // No more room
      break;
    }

    result[used_slots+0] = reinterpret_cast<void*>(static_cast<uintptr_t>(1));
    result[used_slots+1] = reinterpret_cast<void*>(stack->size);
    result[used_slots+2] = reinterpret_cast<void*>(stack->depth);
    for (int d = 0; d < stack->depth; d++) {
      result[used_slots+3+d] = stack->stack[d];
    }
    used_slots += 3 + stack->depth;
  }
  result[used_slots] = reinterpret_cast<void*>(static_cast<uintptr_t>(0));
  return result;
}
#endif

#ifndef WTF_CHANGES

// TCMalloc's support for extra malloc interfaces
class TCMallocImplementation : public MallocExtension {
 public:
  virtual void GetStats(char* buffer, int buffer_length) {
    ASSERT(buffer_length > 0);
    TCMalloc_Printer printer(buffer, buffer_length);

    // Print level one stats unless lots of space is available
    if (buffer_length < 10000) {
      DumpStats(&printer, 1);
    } else {
      DumpStats(&printer, 2);
    }
  }

  virtual void** ReadStackTraces() {
    return DumpStackTraces();
  }

  virtual bool GetNumericProperty(const char* name, size_t* value) {
    ASSERT(name != NULL);

    if (strcmp(name, "generic.current_allocated_bytes") == 0) {
      TCMallocStats stats;
      ExtractStats(&stats, NULL);
      *value = stats.system_bytes
               - stats.thread_bytes
               - stats.central_bytes
               - stats.pageheap_bytes;
      return true;
    }

    if (strcmp(name, "generic.heap_size") == 0) {
      TCMallocStats stats;
      ExtractStats(&stats, NULL);
      *value = stats.system_bytes;
      return true;
    }

    if (strcmp(name, "tcmalloc.slack_bytes") == 0) {
      // We assume that bytes in the page heap are not fragmented too
      // badly, and are therefore available for allocation.
      SpinLockHolder l(&pageheap_lock);
      *value = pageheap->FreeBytes();
      return true;
    }

    if (strcmp(name, "tcmalloc.max_total_thread_cache_bytes") == 0) {
      SpinLockHolder l(&pageheap_lock);
      *value = overall_thread_cache_size;
      return true;
    }

    if (strcmp(name, "tcmalloc.current_total_thread_cache_bytes") == 0) {
      TCMallocStats stats;
      ExtractStats(&stats, NULL);
      *value = stats.thread_bytes;
      return true;
    }

    return false;
  }

  virtual bool SetNumericProperty(const char* name, size_t value) {
    ASSERT(name != NULL);

    if (strcmp(name, "tcmalloc.max_total_thread_cache_bytes") == 0) {
      // Clip the value to a reasonable range
      if (value < kMinThreadCacheSize) value = kMinThreadCacheSize;
      if (value > (1<<30)) value = (1<<30);     // Limit to 1GB

      SpinLockHolder l(&pageheap_lock);
      overall_thread_cache_size = static_cast<size_t>(value);
      TCMalloc_ThreadCache::RecomputeThreadCacheSize();
      return true;
    }

    return false;
  }

  virtual void MarkThreadIdle() {
    TCMalloc_ThreadCache::BecomeIdle();
  }

  virtual void ReleaseFreeMemory() {
    SpinLockHolder h(&pageheap_lock);
    pageheap->ReleaseFreePages();
  }
};
#endif

// The constructor allocates an object to ensure that initialization
// runs before main(), and therefore we do not have a chance to become
// multi-threaded before initialization.  We also create the TSD key
// here.  Presumably by the time this constructor runs, glibc is in
// good enough shape to handle pthread_key_create().
//
// The constructor also takes the opportunity to tell STL to use
// tcmalloc.  We want to do this early, before construct time, so
// all user STL allocations go through tcmalloc (which works really
// well for STL).
//
// The destructor prints stats when the program exits.
class TCMallocGuard {
 public:

  TCMallocGuard() {
#ifdef HAVE_TLS    // this is true if the cc/ld/libc combo support TLS
    // Check whether the kernel also supports TLS (needs to happen at runtime)
    CheckIfKernelSupportsTLS();
#endif
#ifndef WTF_CHANGES
#ifdef WIN32                    // patch the windows VirtualAlloc, etc.
    PatchWindowsFunctions();    // defined in windows/patch_functions.cc
#endif
#endif
    free(malloc(1));
    TCMalloc_ThreadCache::InitTSD();
    free(malloc(1));
#ifndef WTF_CHANGES
    MallocExtension::Register(new TCMallocImplementation);
#endif
  }

#ifndef WTF_CHANGES
  ~TCMallocGuard() {
    const char* env = getenv("MALLOCSTATS");
    if (env != NULL) {
      int level = atoi(env);
      if (level < 1) level = 1;
      PrintStats(level);
    }
#ifdef WIN32
    UnpatchWindowsFunctions();
#endif
  }
#endif
};

#ifndef WTF_CHANGES
static TCMallocGuard module_enter_exit_hook;
#endif


//-------------------------------------------------------------------
// Helpers for the exported routines below
//-------------------------------------------------------------------

#ifndef WTF_CHANGES

static Span* DoSampledAllocation(size_t size) {

  // Grab the stack trace outside the heap lock
  StackTrace tmp;
  tmp.depth = GetStackTrace(tmp.stack, kMaxStackDepth, 1);
  tmp.size = size;

  SpinLockHolder h(&pageheap_lock);
  // Allocate span
  Span *span = pageheap->New(pages(size == 0 ? 1 : size));
  if (span == NULL) {
    return NULL;
  }

  // Allocate stack trace
  StackTrace *stack = stacktrace_allocator.New();
  if (stack == NULL) {
    // Sampling failed because of lack of memory
    return span;
  }

  *stack = tmp;
  span->sample = 1;
  span->objects = stack;
  DLL_Prepend(&sampled_objects, span);

  return span;
}
#endif

static inline bool CheckCachedSizeClass(void *ptr) {
  PageID p = reinterpret_cast<uintptr_t>(ptr) >> kPageShift;
  size_t cached_value = pageheap->GetSizeClassIfCached(p);
  return cached_value == 0 ||
      cached_value == pageheap->GetDescriptor(p)->sizeclass;
}

static inline void* CheckedMallocResult(void *result)
{
  ASSERT(result == 0 || CheckCachedSizeClass(result));
  return result;
}

static inline void* SpanToMallocResult(Span *span) {
  ASSERT_SPAN_COMMITTED(span);
  pageheap->CacheSizeClass(span->start, 0);
  return
      CheckedMallocResult(reinterpret_cast<void*>(span->start << kPageShift));
}

#ifdef WTF_CHANGES
template <bool crashOnFailure>
#endif
static ALWAYS_INLINE void* do_malloc(size_t size) {
  void* ret = NULL;

#ifdef WTF_CHANGES
    ASSERT(!isForbidden());
#endif

  // The following call forces module initialization
  TCMalloc_ThreadCache* heap = TCMalloc_ThreadCache::GetCache();
#ifndef WTF_CHANGES
  if ((FLAGS_tcmalloc_sample_parameter > 0) && heap->SampleAllocation(size)) {
    Span* span = DoSampledAllocation(size);
    if (span != NULL) {
      ret = SpanToMallocResult(span);
    }
  } else
#endif
  if (size > kMaxSize) {
    // Use page-level allocator
    SpinLockHolder h(&pageheap_lock);
    Span* span = pageheap->New(pages(size));
    if (span != NULL) {
      ret = SpanToMallocResult(span);
    }
  } else {
    // The common case, and also the simplest.  This just pops the
    // size-appropriate freelist, afer replenishing it if it's empty.
    ret = CheckedMallocResult(heap->Allocate(size));
  }
  if (!ret) {
#ifdef WTF_CHANGES
    if (crashOnFailure) // This branch should be optimized out by the compiler.
        CRASH();
#else
    errno = ENOMEM;
#endif
  }
  return ret;
}

static ALWAYS_INLINE void do_free(void* ptr) {
  if (ptr == NULL) return;
  ASSERT(pageheap != NULL);  // Should not call free() before malloc()
  const PageID p = reinterpret_cast<uintptr_t>(ptr) >> kPageShift;
  Span* span = NULL;
  size_t cl = pageheap->GetSizeClassIfCached(p);

  if (cl == 0) {
    span = pageheap->GetDescriptor(p);
    cl = span->sizeclass;
    pageheap->CacheSizeClass(p, cl);
  }
  if (cl != 0) {
#ifndef NO_TCMALLOC_SAMPLES
    ASSERT(!pageheap->GetDescriptor(p)->sample);
#endif
    TCMalloc_ThreadCache* heap = TCMalloc_ThreadCache::GetCacheIfPresent();
    if (heap != NULL) {
      heap->Deallocate(ptr, cl);
    } else {
      // Delete directly into central cache
      SLL_SetNext(ptr, NULL);
      central_cache[cl].InsertRange(ptr, ptr, 1);
    }
  } else {
    SpinLockHolder h(&pageheap_lock);
    ASSERT(reinterpret_cast<uintptr_t>(ptr) % kPageSize == 0);
    ASSERT(span != NULL && span->start == p);
#ifndef NO_TCMALLOC_SAMPLES
    if (span->sample) {
      DLL_Remove(span);
      stacktrace_allocator.Delete(reinterpret_cast<StackTrace*>(span->objects));
      span->objects = NULL;
    }
#endif
    pageheap->Delete(span);
  }
}

#ifndef WTF_CHANGES
// For use by exported routines below that want specific alignments
//
// Note: this code can be slow, and can significantly fragment memory.
// The expectation is that memalign/posix_memalign/valloc/pvalloc will
// not be invoked very often.  This requirement simplifies our
// implementation and allows us to tune for expected allocation
// patterns.
static void* do_memalign(size_t align, size_t size) {
  ASSERT((align & (align - 1)) == 0);
  ASSERT(align > 0);
  if (pageheap == NULL) TCMalloc_ThreadCache::InitModule();

  // Allocate at least one byte to avoid boundary conditions below
  if (size == 0) size = 1;

  if (size <= kMaxSize && align < kPageSize) {
    // Search through acceptable size classes looking for one with
    // enough alignment.  This depends on the fact that
    // InitSizeClasses() currently produces several size classes that
    // are aligned at powers of two.  We will waste time and space if
    // we miss in the size class array, but that is deemed acceptable
    // since memalign() should be used rarely.
    size_t cl = SizeClass(size);
    while (cl < kNumClasses && ((class_to_size[cl] & (align - 1)) != 0)) {
      cl++;
    }
    if (cl < kNumClasses) {
      TCMalloc_ThreadCache* heap = TCMalloc_ThreadCache::GetCache();
      return CheckedMallocResult(heap->Allocate(class_to_size[cl]));
    }
  }

  // We will allocate directly from the page heap
  SpinLockHolder h(&pageheap_lock);

  if (align <= kPageSize) {
    // Any page-level allocation will be fine
    // TODO: We could put the rest of this page in the appropriate
    // TODO: cache but it does not seem worth it.
    Span* span = pageheap->New(pages(size));
    return span == NULL ? NULL : SpanToMallocResult(span);
  }

  // Allocate extra pages and carve off an aligned portion
  const Length alloc = pages(size + align);
  Span* span = pageheap->New(alloc);
  if (span == NULL) return NULL;

  // Skip starting portion so that we end up aligned
  Length skip = 0;
  while ((((span->start+skip) << kPageShift) & (align - 1)) != 0) {
    skip++;
  }
  ASSERT(skip < alloc);
  if (skip > 0) {
    Span* rest = pageheap->Split(span, skip);
    pageheap->Delete(span);
    span = rest;
  }

  // Skip trailing portion that we do not need to return
  const Length needed = pages(size);
  ASSERT(span->length >= needed);
  if (span->length > needed) {
    Span* trailer = pageheap->Split(span, needed);
    pageheap->Delete(trailer);
  }
  return SpanToMallocResult(span);
}
#endif

// Helpers for use by exported routines below:

#ifndef WTF_CHANGES
static inline void do_malloc_stats() {
  PrintStats(1);
}
#endif

static inline int do_mallopt(int, int) {
  return 1;     // Indicates error
}

#ifdef HAVE_STRUCT_MALLINFO  // mallinfo isn't defined on freebsd, for instance
static inline struct mallinfo do_mallinfo() {
  TCMallocStats stats;
  ExtractStats(&stats, NULL);

  // Just some of the fields are filled in.
  struct mallinfo info;
  memset(&info, 0, sizeof(info));

  // Unfortunately, the struct contains "int" field, so some of the
  // size values will be truncated.
  info.arena     = static_cast<int>(stats.system_bytes);
  info.fsmblks   = static_cast<int>(stats.thread_bytes
                                    + stats.central_bytes
                                    + stats.transfer_bytes);
  info.fordblks  = static_cast<int>(stats.pageheap_bytes);
  info.uordblks  = static_cast<int>(stats.system_bytes
                                    - stats.thread_bytes
                                    - stats.central_bytes
                                    - stats.transfer_bytes
                                    - stats.pageheap_bytes);

  return info;
}
#endif

//-------------------------------------------------------------------
// Exported routines
//-------------------------------------------------------------------

// CAVEAT: The code structure below ensures that MallocHook methods are always
//         called from the stack frame of the invoked allocation function.
//         heap-checker.cc depends on this to start a stack trace from
//         the call to the (de)allocation function.

#ifndef WTF_CHANGES
extern "C" 
#else
#define do_malloc do_malloc<crashOnFailure>

template <bool crashOnFailure>
ALWAYS_INLINE void* malloc(size_t);

void* fastMalloc(size_t size)
{
    return malloc<true>(size);
}

TryMallocReturnValue tryFastMalloc(size_t size)
{
    return malloc<false>(size);
}

template <bool crashOnFailure>
ALWAYS_INLINE
#endif
void* malloc(size_t size) {
#if ENABLE(WTF_MALLOC_VALIDATION)
    if (std::numeric_limits<size_t>::max() - Internal::ValidationBufferSize <= size)  // If overflow would occur...
        return 0;
    void* result = do_malloc(size + Internal::ValidationBufferSize);
    if (!result)
        return 0;

    Internal::ValidationHeader* header = static_cast<Internal::ValidationHeader*>(result);
    header->m_size = size;
    header->m_type = Internal::AllocTypeMalloc;
    header->m_prefix = static_cast<unsigned>(Internal::ValidationPrefix);
    result = header + 1;
    *Internal::fastMallocValidationSuffix(result) = Internal::ValidationSuffix;
    fastMallocValidate(result);
#else
    void* result = do_malloc(size);
#endif

#ifndef WTF_CHANGES
  MallocHook::InvokeNewHook(result, size);
#endif
  return result;
}

#ifndef WTF_CHANGES
extern "C" 
#endif
void free(void* ptr) {
#ifndef WTF_CHANGES
  MallocHook::InvokeDeleteHook(ptr);
#endif

#if ENABLE(WTF_MALLOC_VALIDATION)
    if (!ptr)
        return;

    fastMallocValidate(ptr);
    Internal::ValidationHeader* header = Internal::fastMallocValidationHeader(ptr);
    memset(ptr, 0xCC, header->m_size);
    do_free(header);
#else
    do_free(ptr);
#endif
}

#ifndef WTF_CHANGES
extern "C" 
#else
template <bool crashOnFailure>
ALWAYS_INLINE void* calloc(size_t, size_t);

void* fastCalloc(size_t n, size_t elem_size)
{
    void* result = calloc<true>(n, elem_size);
#if ENABLE(WTF_MALLOC_VALIDATION)
    fastMallocValidate(result);
#endif
    return result;
}

TryMallocReturnValue tryFastCalloc(size_t n, size_t elem_size)
{
    void* result = calloc<false>(n, elem_size);
#if ENABLE(WTF_MALLOC_VALIDATION)
    fastMallocValidate(result);
#endif
    return result;
}

template <bool crashOnFailure>
ALWAYS_INLINE
#endif
void* calloc(size_t n, size_t elem_size) {
  size_t totalBytes = n * elem_size;
    
  // Protect against overflow
  if (n > 1 && elem_size && (totalBytes / elem_size) != n)
    return 0;

#if ENABLE(WTF_MALLOC_VALIDATION)
    void* result = malloc<crashOnFailure>(totalBytes);
    if (!result)
        return 0;

    memset(result, 0, totalBytes);
    fastMallocValidate(result);
#else
    void* result = do_malloc(totalBytes);
    if (result != NULL) {
        memset(result, 0, totalBytes);
    }
#endif

#ifndef WTF_CHANGES
  MallocHook::InvokeNewHook(result, totalBytes);
#endif
  return result;
}

// Since cfree isn't used anywhere, we don't compile it in.
#ifndef WTF_CHANGES
#ifndef WTF_CHANGES
extern "C" 
#endif
void cfree(void* ptr) {
#ifndef WTF_CHANGES
    MallocHook::InvokeDeleteHook(ptr);
#endif
  do_free(ptr);
}
#endif

#ifndef WTF_CHANGES
extern "C" 
#else
template <bool crashOnFailure>
ALWAYS_INLINE void* realloc(void*, size_t);

void* fastRealloc(void* old_ptr, size_t new_size)
{
#if ENABLE(WTF_MALLOC_VALIDATION)
    fastMallocValidate(old_ptr);
#endif
    void* result = realloc<true>(old_ptr, new_size);
#if ENABLE(WTF_MALLOC_VALIDATION)
    fastMallocValidate(result);
#endif
    return result;
}

TryMallocReturnValue tryFastRealloc(void* old_ptr, size_t new_size)
{
#if ENABLE(WTF_MALLOC_VALIDATION)
    fastMallocValidate(old_ptr);
#endif
    void* result = realloc<false>(old_ptr, new_size);
#if ENABLE(WTF_MALLOC_VALIDATION)
    fastMallocValidate(result);
#endif
    return result;
}

template <bool crashOnFailure>
ALWAYS_INLINE
#endif
void* realloc(void* old_ptr, size_t new_size) {
  if (old_ptr == NULL) {
#if ENABLE(WTF_MALLOC_VALIDATION)
    void* result = malloc<crashOnFailure>(new_size);
#else
    void* result = do_malloc(new_size);
#ifndef WTF_CHANGES
    MallocHook::InvokeNewHook(result, new_size);
#endif
#endif
    return result;
  }
  if (new_size == 0) {
#ifndef WTF_CHANGES
    MallocHook::InvokeDeleteHook(old_ptr);
#endif
    free(old_ptr);
    return NULL;
  }

#if ENABLE(WTF_MALLOC_VALIDATION)
    if (std::numeric_limits<size_t>::max() - Internal::ValidationBufferSize <= new_size)  // If overflow would occur...
        return 0;
    Internal::ValidationHeader* header = Internal::fastMallocValidationHeader(old_ptr);
    fastMallocValidate(old_ptr);
    old_ptr = header;
    header->m_size = new_size;
    new_size += Internal::ValidationBufferSize;
#endif

  // Get the size of the old entry
  const PageID p = reinterpret_cast<uintptr_t>(old_ptr) >> kPageShift;
  size_t cl = pageheap->GetSizeClassIfCached(p);
  Span *span = NULL;
  size_t old_size;
  if (cl == 0) {
    span = pageheap->GetDescriptor(p);
    cl = span->sizeclass;
    pageheap->CacheSizeClass(p, cl);
  }
  if (cl != 0) {
    old_size = ByteSizeForClass(cl);
  } else {
    ASSERT(span != NULL);
    old_size = span->length << kPageShift;
  }

  // Reallocate if the new size is larger than the old size,
  // or if the new size is significantly smaller than the old size.
  if ((new_size > old_size) || (AllocationSize(new_size) < old_size)) {
    // Need to reallocate
    void* new_ptr = do_malloc(new_size);
    if (new_ptr == NULL) {
      return NULL;
    }
#ifndef WTF_CHANGES
    MallocHook::InvokeNewHook(new_ptr, new_size);
#endif
    memcpy(new_ptr, old_ptr, ((old_size < new_size) ? old_size : new_size));
#ifndef WTF_CHANGES
    MallocHook::InvokeDeleteHook(old_ptr);
#endif
    // We could use a variant of do_free() that leverages the fact
    // that we already know the sizeclass of old_ptr.  The benefit
    // would be small, so don't bother.
    do_free(old_ptr);
#if ENABLE(WTF_MALLOC_VALIDATION)
    new_ptr = static_cast<Internal::ValidationHeader*>(new_ptr) + 1;
    *Internal::fastMallocValidationSuffix(new_ptr) = Internal::ValidationSuffix;
#endif
    return new_ptr;
  } else {
#if ENABLE(WTF_MALLOC_VALIDATION)
    old_ptr = static_cast<Internal::ValidationHeader*>(old_ptr) + 1; // Set old_ptr back to the user pointer.
    *Internal::fastMallocValidationSuffix(old_ptr) = Internal::ValidationSuffix;
#endif
    return old_ptr;
  }
}

#ifdef WTF_CHANGES
#undef do_malloc
#else

static SpinLock set_new_handler_lock = SPINLOCK_INITIALIZER;

static inline void* cpp_alloc(size_t size, bool nothrow) {
  for (;;) {
    void* p = do_malloc(size);
#ifdef PREANSINEW
    return p;
#else
    if (p == NULL) {  // allocation failed
      // Get the current new handler.  NB: this function is not
      // thread-safe.  We make a feeble stab at making it so here, but
      // this lock only protects against tcmalloc interfering with
      // itself, not with other libraries calling set_new_handler.
      std::new_handler nh;
      {
        SpinLockHolder h(&set_new_handler_lock);
        nh = std::set_new_handler(0);
        (void) std::set_new_handler(nh);
      }
      // If no new_handler is established, the allocation failed.
      if (!nh) {
        if (nothrow) return 0;
        throw std::bad_alloc();
      }
      // Otherwise, try the new_handler.  If it returns, retry the
      // allocation.  If it throws std::bad_alloc, fail the allocation.
      // if it throws something else, don't interfere.
      try {
        (*nh)();
      } catch (const std::bad_alloc&) {
        if (!nothrow) throw;
        return p;
      }
    } else {  // allocation success
      return p;
    }
#endif
  }
}

#if ENABLE(GLOBAL_FASTMALLOC_NEW)

void* operator new(size_t size) {
  void* p = cpp_alloc(size, false);
  // We keep this next instruction out of cpp_alloc for a reason: when
  // it's in, and new just calls cpp_alloc, the optimizer may fold the
  // new call into cpp_alloc, which messes up our whole section-based
  // stacktracing (see ATTRIBUTE_SECTION, above).  This ensures cpp_alloc
  // isn't the last thing this fn calls, and prevents the folding.
  MallocHook::InvokeNewHook(p, size);
  return p;
}

void* operator new(size_t size, const std::nothrow_t&) __THROW {
  void* p = cpp_alloc(size, true);
  MallocHook::InvokeNewHook(p, size);
  return p;
}

void operator delete(void* p) __THROW {
  MallocHook::InvokeDeleteHook(p);
  do_free(p);
}

void operator delete(void* p, const std::nothrow_t&) __THROW {
  MallocHook::InvokeDeleteHook(p);
  do_free(p);
}

void* operator new[](size_t size) {
  void* p = cpp_alloc(size, false);
  // We keep this next instruction out of cpp_alloc for a reason: when
  // it's in, and new just calls cpp_alloc, the optimizer may fold the
  // new call into cpp_alloc, which messes up our whole section-based
  // stacktracing (see ATTRIBUTE_SECTION, above).  This ensures cpp_alloc
  // isn't the last thing this fn calls, and prevents the folding.
  MallocHook::InvokeNewHook(p, size);
  return p;
}

void* operator new[](size_t size, const std::nothrow_t&) __THROW {
  void* p = cpp_alloc(size, true);
  MallocHook::InvokeNewHook(p, size);
  return p;
}

void operator delete[](void* p) __THROW {
  MallocHook::InvokeDeleteHook(p);
  do_free(p);
}

void operator delete[](void* p, const std::nothrow_t&) __THROW {
  MallocHook::InvokeDeleteHook(p);
  do_free(p);
}

#endif

extern "C" void* memalign(size_t align, size_t size) __THROW {
  void* result = do_memalign(align, size);
  MallocHook::InvokeNewHook(result, size);
  return result;
}

extern "C" int posix_memalign(void** result_ptr, size_t align, size_t size)
    __THROW {
  if (((align % sizeof(void*)) != 0) ||
      ((align & (align - 1)) != 0) ||
      (align == 0)) {
    return EINVAL;
  }

  void* result = do_memalign(align, size);
  MallocHook::InvokeNewHook(result, size);
  if (result == NULL) {
    return ENOMEM;
  } else {
    *result_ptr = result;
    return 0;
  }
}

static size_t pagesize = 0;

extern "C" void* valloc(size_t size) __THROW {
  // Allocate page-aligned object of length >= size bytes
  if (pagesize == 0) pagesize = getpagesize();
  void* result = do_memalign(pagesize, size);
  MallocHook::InvokeNewHook(result, size);
  return result;
}

extern "C" void* pvalloc(size_t size) __THROW {
  // Round up size to a multiple of pagesize
  if (pagesize == 0) pagesize = getpagesize();
  size = (size + pagesize - 1) & ~(pagesize - 1);
  void* result = do_memalign(pagesize, size);
  MallocHook::InvokeNewHook(result, size);
  return result;
}

extern "C" void malloc_stats(void) {
  do_malloc_stats();
}

extern "C" int mallopt(int cmd, int value) {
  return do_mallopt(cmd, value);
}

#ifdef HAVE_STRUCT_MALLINFO
extern "C" struct mallinfo mallinfo(void) {
  return do_mallinfo();
}
#endif

//-------------------------------------------------------------------
// Some library routines on RedHat 9 allocate memory using malloc()
// and free it using __libc_free() (or vice-versa).  Since we provide
// our own implementations of malloc/free, we need to make sure that
// the __libc_XXX variants (defined as part of glibc) also point to
// the same implementations.
//-------------------------------------------------------------------

#if defined(__GLIBC__)
extern "C" {
#if COMPILER(GCC) && !defined(__MACH__) && defined(HAVE___ATTRIBUTE__)
  // Potentially faster variants that use the gcc alias extension.
  // Mach-O (Darwin) does not support weak aliases, hence the __MACH__ check.
# define ALIAS(x) __attribute__ ((weak, alias (x)))
  void* __libc_malloc(size_t size)              ALIAS("malloc");
  void  __libc_free(void* ptr)                  ALIAS("free");
  void* __libc_realloc(void* ptr, size_t size)  ALIAS("realloc");
  void* __libc_calloc(size_t n, size_t size)    ALIAS("calloc");
  void  __libc_cfree(void* ptr)                 ALIAS("cfree");
  void* __libc_memalign(size_t align, size_t s) ALIAS("memalign");
  void* __libc_valloc(size_t size)              ALIAS("valloc");
  void* __libc_pvalloc(size_t size)             ALIAS("pvalloc");
  int __posix_memalign(void** r, size_t a, size_t s) ALIAS("posix_memalign");
# undef ALIAS
# else   /* not __GNUC__ */
  // Portable wrappers
  void* __libc_malloc(size_t size)              { return malloc(size);       }
  void  __libc_free(void* ptr)                  { free(ptr);                 }
  void* __libc_realloc(void* ptr, size_t size)  { return realloc(ptr, size); }
  void* __libc_calloc(size_t n, size_t size)    { return calloc(n, size);    }
  void  __libc_cfree(void* ptr)                 { cfree(ptr);                }
  void* __libc_memalign(size_t align, size_t s) { return memalign(align, s); }
  void* __libc_valloc(size_t size)              { return valloc(size);       }
  void* __libc_pvalloc(size_t size)             { return pvalloc(size);      }
  int __posix_memalign(void** r, size_t a, size_t s) {
    return posix_memalign(r, a, s);
  }
# endif  /* __GNUC__ */
}
#endif   /* __GLIBC__ */

// Override __libc_memalign in libc on linux boxes specially.
// They have a bug in libc that causes them to (very rarely) allocate
// with __libc_memalign() yet deallocate with free() and the
// definitions above don't catch it.
// This function is an exception to the rule of calling MallocHook method
// from the stack frame of the allocation function;
// heap-checker handles this special case explicitly.
static void *MemalignOverride(size_t align, size_t size, const void *caller)
    __THROW {
  void* result = do_memalign(align, size);
  MallocHook::InvokeNewHook(result, size);
  return result;
}
void *(*__memalign_hook)(size_t, size_t, const void *) = MemalignOverride;

#endif

#ifdef WTF_CHANGES
void releaseFastMallocFreeMemory()
{
    // Flush free pages in the current thread cache back to the page heap.
    // Low watermark mechanism in Scavenge() prevents full return on the first pass.
    // The second pass flushes everything.
    if (TCMalloc_ThreadCache* threadCache = TCMalloc_ThreadCache::GetCacheIfPresent()) {
        threadCache->Scavenge();
        threadCache->Scavenge();
    }

    SpinLockHolder h(&pageheap_lock);
    pageheap->ReleaseFreePages();
}
    
FastMallocStatistics fastMallocStatistics()
{
    FastMallocStatistics statistics;

    SpinLockHolder lockHolder(&pageheap_lock);
    statistics.reservedVMBytes = static_cast<size_t>(pageheap->SystemBytes());
    statistics.committedVMBytes = statistics.reservedVMBytes - pageheap->ReturnedBytes();

    statistics.freeListBytes = 0;
    for (unsigned cl = 0; cl < kNumClasses; ++cl) {
        const int length = central_cache[cl].length();
        const int tc_length = central_cache[cl].tc_length();

        statistics.freeListBytes += ByteSizeForClass(cl) * (length + tc_length);
    }
    for (TCMalloc_ThreadCache* threadCache = thread_heaps; threadCache ; threadCache = threadCache->next_)
        statistics.freeListBytes += threadCache->Size();

    return statistics;
}

size_t fastMallocSize(const void* ptr)
{
#if ENABLE(WTF_MALLOC_VALIDATION)
    return Internal::fastMallocValidationHeader(const_cast<void*>(ptr))->m_size;
#else
    const PageID p = reinterpret_cast<uintptr_t>(ptr) >> kPageShift;
    Span* span = pageheap->GetDescriptorEnsureSafe(p);

    if (!span || span->free)
        return 0;

    for (void* free = span->objects; free != NULL; free = *((void**) free)) {
        if (ptr == free)
            return 0;
    }

    if (size_t cl = span->sizeclass)
        return ByteSizeForClass(cl);

    return span->length << kPageShift;
#endif
}

#if OS(DARWIN)

class FreeObjectFinder {
    const RemoteMemoryReader& m_reader;
    HashSet<void*> m_freeObjects;

public:
    FreeObjectFinder(const RemoteMemoryReader& reader) : m_reader(reader) { }

    void visit(void* ptr) { m_freeObjects.add(ptr); }
    bool isFreeObject(void* ptr) const { return m_freeObjects.contains(ptr); }
    bool isFreeObject(vm_address_t ptr) const { return isFreeObject(reinterpret_cast<void*>(ptr)); }
    size_t freeObjectCount() const { return m_freeObjects.size(); }

    void findFreeObjects(TCMalloc_ThreadCache* threadCache)
    {
        for (; threadCache; threadCache = (threadCache->next_ ? m_reader(threadCache->next_) : 0))
            threadCache->enumerateFreeObjects(*this, m_reader);
    }

    void findFreeObjects(TCMalloc_Central_FreeListPadded* centralFreeList, size_t numSizes, TCMalloc_Central_FreeListPadded* remoteCentralFreeList)
    {
        for (unsigned i = 0; i < numSizes; i++)
            centralFreeList[i].enumerateFreeObjects(*this, m_reader, remoteCentralFreeList + i);
    }
};

class PageMapFreeObjectFinder {
    const RemoteMemoryReader& m_reader;
    FreeObjectFinder& m_freeObjectFinder;

public:
    PageMapFreeObjectFinder(const RemoteMemoryReader& reader, FreeObjectFinder& freeObjectFinder)
        : m_reader(reader)
        , m_freeObjectFinder(freeObjectFinder)
    { }

    int visit(void* ptr) const
    {
        if (!ptr)
            return 1;

        Span* span = m_reader(reinterpret_cast<Span*>(ptr));
        if (!span)
            return 1;

        if (span->free) {
            void* ptr = reinterpret_cast<void*>(span->start << kPageShift);
            m_freeObjectFinder.visit(ptr);
        } else if (span->sizeclass) {
            // Walk the free list of the small-object span, keeping track of each object seen
            for (void* nextObject = span->objects; nextObject; nextObject = m_reader.nextEntryInLinkedList(reinterpret_cast<void**>(nextObject)))
                m_freeObjectFinder.visit(nextObject);
        }
        return span->length;
    }
};

class PageMapMemoryUsageRecorder {
    task_t m_task;
    void* m_context;
    unsigned m_typeMask;
    vm_range_recorder_t* m_recorder;
    const RemoteMemoryReader& m_reader;
    const FreeObjectFinder& m_freeObjectFinder;

    HashSet<void*> m_seenPointers;
    Vector<Span*> m_coalescedSpans;

public:
    PageMapMemoryUsageRecorder(task_t task, void* context, unsigned typeMask, vm_range_recorder_t* recorder, const RemoteMemoryReader& reader, const FreeObjectFinder& freeObjectFinder)
        : m_task(task)
        , m_context(context)
        , m_typeMask(typeMask)
        , m_recorder(recorder)
        , m_reader(reader)
        , m_freeObjectFinder(freeObjectFinder)
    { }

    ~PageMapMemoryUsageRecorder()
    {
        ASSERT(!m_coalescedSpans.size());
    }

    void recordPendingRegions()
    {
        Span* lastSpan = m_coalescedSpans[m_coalescedSpans.size() - 1];
        vm_range_t ptrRange = { m_coalescedSpans[0]->start << kPageShift, 0 };
        ptrRange.size = (lastSpan->start << kPageShift) - ptrRange.address + (lastSpan->length * kPageSize);

        // Mark the memory region the spans represent as a candidate for containing pointers
        if (m_typeMask & MALLOC_PTR_REGION_RANGE_TYPE)
            (*m_recorder)(m_task, m_context, MALLOC_PTR_REGION_RANGE_TYPE, &ptrRange, 1);

        if (!(m_typeMask & MALLOC_PTR_IN_USE_RANGE_TYPE)) {
            m_coalescedSpans.clear();
            return;
        }

        Vector<vm_range_t, 1024> allocatedPointers;
        for (size_t i = 0; i < m_coalescedSpans.size(); ++i) {
            Span *theSpan = m_coalescedSpans[i];
            if (theSpan->free)
                continue;

            vm_address_t spanStartAddress = theSpan->start << kPageShift;
            vm_size_t spanSizeInBytes = theSpan->length * kPageSize;

            if (!theSpan->sizeclass) {
                // If it's an allocated large object span, mark it as in use
                if (!m_freeObjectFinder.isFreeObject(spanStartAddress))
                    allocatedPointers.append((vm_range_t){spanStartAddress, spanSizeInBytes});
            } else {
                const size_t objectSize = ByteSizeForClass(theSpan->sizeclass);

                // Mark each allocated small object within the span as in use
                const vm_address_t endOfSpan = spanStartAddress + spanSizeInBytes;
                for (vm_address_t object = spanStartAddress; object + objectSize <= endOfSpan; object += objectSize) {
                    if (!m_freeObjectFinder.isFreeObject(object))
                        allocatedPointers.append((vm_range_t){object, objectSize});
                }
            }
        }

        (*m_recorder)(m_task, m_context, MALLOC_PTR_IN_USE_RANGE_TYPE, allocatedPointers.data(), allocatedPointers.size());

        m_coalescedSpans.clear();
    }

    int visit(void* ptr)
    {
        if (!ptr)
            return 1;

        Span* span = m_reader(reinterpret_cast<Span*>(ptr));
        if (!span || !span->start)
            return 1;

        if (m_seenPointers.contains(ptr))
            return span->length;
        m_seenPointers.add(ptr);

        if (!m_coalescedSpans.size()) {
            m_coalescedSpans.append(span);
            return span->length;
        }

        Span* previousSpan = m_coalescedSpans[m_coalescedSpans.size() - 1];
        vm_address_t previousSpanStartAddress = previousSpan->start << kPageShift;
        vm_size_t previousSpanSizeInBytes = previousSpan->length * kPageSize;

        // If the new span is adjacent to the previous span, do nothing for now.
        vm_address_t spanStartAddress = span->start << kPageShift;
        if (spanStartAddress == previousSpanStartAddress + previousSpanSizeInBytes) {
            m_coalescedSpans.append(span);
            return span->length;
        }

        // New span is not adjacent to previous span, so record the spans coalesced so far.
        recordPendingRegions();
        m_coalescedSpans.append(span);

        return span->length;
    }
};

class AdminRegionRecorder {
    task_t m_task;
    void* m_context;
    unsigned m_typeMask;
    vm_range_recorder_t* m_recorder;
    const RemoteMemoryReader& m_reader;

    Vector<vm_range_t, 1024> m_pendingRegions;

public:
    AdminRegionRecorder(task_t task, void* context, unsigned typeMask, vm_range_recorder_t* recorder, const RemoteMemoryReader& reader)
        : m_task(task)
        , m_context(context)
        , m_typeMask(typeMask)
        , m_recorder(recorder)
        , m_reader(reader)
    { }

    void recordRegion(vm_address_t ptr, size_t size)
    {
        if (m_typeMask & MALLOC_ADMIN_REGION_RANGE_TYPE)
            m_pendingRegions.append((vm_range_t){ ptr, size });
    }

    void visit(void *ptr, size_t size)
    {
        recordRegion(reinterpret_cast<vm_address_t>(ptr), size);
    }

    void recordPendingRegions()
    {
        if (m_pendingRegions.size()) {
            (*m_recorder)(m_task, m_context, MALLOC_ADMIN_REGION_RANGE_TYPE, m_pendingRegions.data(), m_pendingRegions.size());
            m_pendingRegions.clear();
        }
    }

    ~AdminRegionRecorder()
    {
        ASSERT(!m_pendingRegions.size());
    }
};

kern_return_t FastMallocZone::enumerate(task_t task, void* context, unsigned typeMask, vm_address_t zoneAddress, memory_reader_t reader, vm_range_recorder_t recorder)
{
    RemoteMemoryReader memoryReader(task, reader);

    InitSizeClasses();

    FastMallocZone* mzone = memoryReader(reinterpret_cast<FastMallocZone*>(zoneAddress));
    TCMalloc_PageHeap* pageHeap = memoryReader(mzone->m_pageHeap);
    TCMalloc_ThreadCache** threadHeapsPointer = memoryReader(mzone->m_threadHeaps);
    TCMalloc_ThreadCache* threadHeaps = memoryReader(*threadHeapsPointer);

    TCMalloc_Central_FreeListPadded* centralCaches = memoryReader(mzone->m_centralCaches, sizeof(TCMalloc_Central_FreeListPadded) * kNumClasses);

    FreeObjectFinder finder(memoryReader);
    finder.findFreeObjects(threadHeaps);
    finder.findFreeObjects(centralCaches, kNumClasses, mzone->m_centralCaches);

    TCMalloc_PageHeap::PageMap* pageMap = &pageHeap->pagemap_;
    PageMapFreeObjectFinder pageMapFinder(memoryReader, finder);
    pageMap->visitValues(pageMapFinder, memoryReader);

    PageMapMemoryUsageRecorder usageRecorder(task, context, typeMask, recorder, memoryReader, finder);
    pageMap->visitValues(usageRecorder, memoryReader);
    usageRecorder.recordPendingRegions();

    AdminRegionRecorder adminRegionRecorder(task, context, typeMask, recorder, memoryReader);
    pageMap->visitAllocations(adminRegionRecorder, memoryReader);

    PageHeapAllocator<Span>* spanAllocator = memoryReader(mzone->m_spanAllocator);
    PageHeapAllocator<TCMalloc_ThreadCache>* pageHeapAllocator = memoryReader(mzone->m_pageHeapAllocator);

    spanAllocator->recordAdministrativeRegions(adminRegionRecorder, memoryReader);
    pageHeapAllocator->recordAdministrativeRegions(adminRegionRecorder, memoryReader);

    adminRegionRecorder.recordPendingRegions();

    return 0;
}

size_t FastMallocZone::size(malloc_zone_t*, const void*)
{
    return 0;
}

void* FastMallocZone::zoneMalloc(malloc_zone_t*, size_t)
{
    return 0;
}

void* FastMallocZone::zoneCalloc(malloc_zone_t*, size_t, size_t)
{
    return 0;
}

void FastMallocZone::zoneFree(malloc_zone_t*, void* ptr)
{
    // Due to <rdar://problem/5671357> zoneFree may be called by the system free even if the pointer
    // is not in this zone.  When this happens, the pointer being freed was not allocated by any
    // zone so we need to print a useful error for the application developer.
    malloc_printf("*** error for object %p: pointer being freed was not allocated\n", ptr);
}

void* FastMallocZone::zoneRealloc(malloc_zone_t*, void*, size_t)
{
    return 0;
}


#undef malloc
#undef free
#undef realloc
#undef calloc

extern "C" {
malloc_introspection_t jscore_fastmalloc_introspection = { &FastMallocZone::enumerate, &FastMallocZone::goodSize, &FastMallocZone::check, &FastMallocZone::print,
    &FastMallocZone::log, &FastMallocZone::forceLock, &FastMallocZone::forceUnlock, &FastMallocZone::statistics

#ifndef BUILDING_ON_LEOPARD
    , 0 // zone_locked will not be called on the zone unless it advertises itself as version five or higher.
#endif
#if !defined(BUILDING_ON_LEOPARD) && !defined(BUILDING_ON_SNOW_LEOPARD)
    , 0, 0, 0, 0 // These members will not be used unless the zone advertises itself as version seven or higher.
#endif

    };
}

FastMallocZone::FastMallocZone(TCMalloc_PageHeap* pageHeap, TCMalloc_ThreadCache** threadHeaps, TCMalloc_Central_FreeListPadded* centralCaches, PageHeapAllocator<Span>* spanAllocator, PageHeapAllocator<TCMalloc_ThreadCache>* pageHeapAllocator)
    : m_pageHeap(pageHeap)
    , m_threadHeaps(threadHeaps)
    , m_centralCaches(centralCaches)
    , m_spanAllocator(spanAllocator)
    , m_pageHeapAllocator(pageHeapAllocator)
{
    memset(&m_zone, 0, sizeof(m_zone));
    m_zone.version = 4;
    m_zone.zone_name = "JavaScriptCore FastMalloc";
    m_zone.size = &FastMallocZone::size;
    m_zone.malloc = &FastMallocZone::zoneMalloc;
    m_zone.calloc = &FastMallocZone::zoneCalloc;
    m_zone.realloc = &FastMallocZone::zoneRealloc;
    m_zone.free = &FastMallocZone::zoneFree;
    m_zone.valloc = &FastMallocZone::zoneValloc;
    m_zone.destroy = &FastMallocZone::zoneDestroy;
    m_zone.introspect = &jscore_fastmalloc_introspection;
    malloc_zone_register(&m_zone);
}


void FastMallocZone::init()
{
    static FastMallocZone zone(pageheap, &thread_heaps, static_cast<TCMalloc_Central_FreeListPadded*>(central_cache), &span_allocator, &threadheap_allocator);
}

#endif // OS(DARWIN)

} // namespace WTF
#endif // WTF_CHANGES

#endif // FORCE_SYSTEM_MALLOC
