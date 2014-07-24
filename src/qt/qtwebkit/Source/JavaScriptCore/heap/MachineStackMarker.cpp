/*
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 *  Copyright (C) 2007 Eric Seidel <eric@webkit.org>
 *  Copyright (C) 2009 Acision BV. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "config.h"
#include "MachineStackMarker.h"

#include "ConservativeRoots.h"
#include "Heap.h"
#include "JSArray.h"
#include "VM.h"
#include <setjmp.h>
#include <stdlib.h>
#include <wtf/StdLibExtras.h>

#if OS(DARWIN)

#include <mach/mach_init.h>
#include <mach/mach_port.h>
#include <mach/task.h>
#include <mach/thread_act.h>
#include <mach/vm_map.h>

#elif OS(WINDOWS)

#include <windows.h>
#include <malloc.h>

#elif OS(UNIX)

#include <sys/mman.h>
#include <unistd.h>

#if OS(SOLARIS)
#include <thread.h>
#else
#include <pthread.h>
#endif

#if HAVE(PTHREAD_NP_H)
#include <pthread_np.h>
#endif

#if OS(QNX)
#include <fcntl.h>
#include <sys/procfs.h>
#include <stdio.h>
#include <errno.h>
#endif

#if USE(PTHREADS) && !OS(WINDOWS) && !OS(DARWIN)
#include <signal.h>
#endif

#endif

using namespace WTF;

namespace JSC {

static inline void swapIfBackwards(void*& begin, void*& end)
{
#if OS(WINCE)
    if (begin <= end)
        return;
    std::swap(begin, end);
#else
UNUSED_PARAM(begin);
UNUSED_PARAM(end);
#endif
}

#if OS(DARWIN)
typedef mach_port_t PlatformThread;
#elif OS(WINDOWS)
typedef HANDLE PlatformThread;
#elif USE(PTHREADS)
typedef pthread_t PlatformThread;
static const int SigThreadSuspendResume = SIGUSR2;

#if defined(SA_RESTART)
static void pthreadSignalHandlerSuspendResume(int)
{
    sigset_t signalSet;
    sigemptyset(&signalSet);
    sigaddset(&signalSet, SigThreadSuspendResume);
    sigsuspend(&signalSet);
}
#endif
#endif

class MachineThreads::Thread {
    WTF_MAKE_FAST_ALLOCATED;
public:
    Thread(const PlatformThread& platThread, void* base)
        : platformThread(platThread)
        , stackBase(base)
    {
#if USE(PTHREADS) && !OS(WINDOWS) && !OS(DARWIN) && defined(SA_RESTART)
        // if we have SA_RESTART, enable SIGUSR2 debugging mechanism
        struct sigaction action;
        action.sa_handler = pthreadSignalHandlerSuspendResume;
        sigemptyset(&action.sa_mask);
        action.sa_flags = SA_RESTART;
        sigaction(SigThreadSuspendResume, &action, 0);

        sigset_t mask;
        sigemptyset(&mask);
        sigaddset(&mask, SigThreadSuspendResume);
        pthread_sigmask(SIG_UNBLOCK, &mask, 0);
#endif
    }

    Thread* next;
    PlatformThread platformThread;
    void* stackBase;
};

MachineThreads::MachineThreads(Heap* heap)
    : m_registeredThreads(0)
    , m_threadSpecific(0)
#if !ASSERT_DISABLED
    , m_heap(heap)
#endif
{
    UNUSED_PARAM(heap);
}

MachineThreads::~MachineThreads()
{
    if (m_threadSpecific)
        threadSpecificKeyDelete(m_threadSpecific);

    MutexLocker registeredThreadsLock(m_registeredThreadsMutex);
    for (Thread* t = m_registeredThreads; t;) {
        Thread* next = t->next;
        delete t;
        t = next;
    }
}

static inline PlatformThread getCurrentPlatformThread()
{
#if OS(DARWIN)
    return pthread_mach_thread_np(pthread_self());
#elif OS(WINDOWS)
    return GetCurrentThread();
#elif USE(PTHREADS)
    return pthread_self();
#endif
}

static inline bool equalThread(const PlatformThread& first, const PlatformThread& second)
{
#if OS(DARWIN) || OS(WINDOWS)
    return first == second;
#elif USE(PTHREADS)
    return !!pthread_equal(first, second);
#else
#error Need a way to compare threads on this platform
#endif
}

void MachineThreads::makeUsableFromMultipleThreads()
{
    if (m_threadSpecific)
        return;

    threadSpecificKeyCreate(&m_threadSpecific, removeThread);
}

void MachineThreads::addCurrentThread()
{
    ASSERT(!m_heap->vm()->exclusiveThread || m_heap->vm()->exclusiveThread == currentThread());

    if (!m_threadSpecific || threadSpecificGet(m_threadSpecific))
        return;

    threadSpecificSet(m_threadSpecific, this);
    Thread* thread = new Thread(getCurrentPlatformThread(), wtfThreadData().stack().origin());

    MutexLocker lock(m_registeredThreadsMutex);

    thread->next = m_registeredThreads;
    m_registeredThreads = thread;
}

void MachineThreads::removeThread(void* p)
{
    if (p)
        static_cast<MachineThreads*>(p)->removeCurrentThread();
}

void MachineThreads::removeCurrentThread()
{
    PlatformThread currentPlatformThread = getCurrentPlatformThread();

    MutexLocker lock(m_registeredThreadsMutex);

    if (equalThread(currentPlatformThread, m_registeredThreads->platformThread)) {
        Thread* t = m_registeredThreads;
        m_registeredThreads = m_registeredThreads->next;
        delete t;
    } else {
        Thread* last = m_registeredThreads;
        Thread* t;
        for (t = m_registeredThreads->next; t; t = t->next) {
            if (equalThread(t->platformThread, currentPlatformThread)) {
                last->next = t->next;
                break;
            }
            last = t;
        }
        ASSERT(t); // If t is NULL, we never found ourselves in the list.
        delete t;
    }
}

#if COMPILER(GCC)
#define REGISTER_BUFFER_ALIGNMENT __attribute__ ((aligned (sizeof(void*))))
#else
#define REGISTER_BUFFER_ALIGNMENT
#endif

void MachineThreads::gatherFromCurrentThread(ConservativeRoots& conservativeRoots, void* stackCurrent)
{
    // setjmp forces volatile registers onto the stack
    jmp_buf registers REGISTER_BUFFER_ALIGNMENT;
#if COMPILER(MSVC)
#pragma warning(push)
#pragma warning(disable: 4611)
#endif
    setjmp(registers);
#if COMPILER(MSVC)
#pragma warning(pop)
#endif

    void* registersBegin = &registers;
    void* registersEnd = reinterpret_cast<void*>(roundUpToMultipleOf<sizeof(void*)>(reinterpret_cast<uintptr_t>(&registers + 1)));
    swapIfBackwards(registersBegin, registersEnd);
    conservativeRoots.add(registersBegin, registersEnd);

    void* stackBegin = stackCurrent;
    void* stackEnd = wtfThreadData().stack().origin();
    swapIfBackwards(stackBegin, stackEnd);
    conservativeRoots.add(stackBegin, stackEnd);
}

static inline void suspendThread(const PlatformThread& platformThread)
{
#if OS(DARWIN)
    thread_suspend(platformThread);
#elif OS(WINDOWS)
    SuspendThread(platformThread);
#elif USE(PTHREADS)
    pthread_kill(platformThread, SigThreadSuspendResume);
#else
#error Need a way to suspend threads on this platform
#endif
}

static inline void resumeThread(const PlatformThread& platformThread)
{
#if OS(DARWIN)
    thread_resume(platformThread);
#elif OS(WINDOWS)
    ResumeThread(platformThread);
#elif USE(PTHREADS)
    pthread_kill(platformThread, SigThreadSuspendResume);
#else
#error Need a way to resume threads on this platform
#endif
}

typedef unsigned long usword_t; // word size, assumed to be either 32 or 64 bit

#if OS(DARWIN)

#if CPU(X86)
typedef i386_thread_state_t PlatformThreadRegisters;
#elif CPU(X86_64)
typedef x86_thread_state64_t PlatformThreadRegisters;
#elif CPU(PPC)
typedef ppc_thread_state_t PlatformThreadRegisters;
#elif CPU(PPC64)
typedef ppc_thread_state64_t PlatformThreadRegisters;
#elif CPU(ARM)
typedef arm_thread_state_t PlatformThreadRegisters;
#else
#error Unknown Architecture
#endif

#elif OS(WINDOWS)
typedef CONTEXT PlatformThreadRegisters;
#elif OS(QNX)
typedef struct _debug_thread_info PlatformThreadRegisters;
#elif USE(PTHREADS)
typedef pthread_attr_t PlatformThreadRegisters;
#else
#error Need a thread register struct for this platform
#endif

static size_t getPlatformThreadRegisters(const PlatformThread& platformThread, PlatformThreadRegisters& regs)
{
#if OS(DARWIN)

#if CPU(X86)
    unsigned user_count = sizeof(regs)/sizeof(int);
    thread_state_flavor_t flavor = i386_THREAD_STATE;
#elif CPU(X86_64)
    unsigned user_count = x86_THREAD_STATE64_COUNT;
    thread_state_flavor_t flavor = x86_THREAD_STATE64;
#elif CPU(PPC) 
    unsigned user_count = PPC_THREAD_STATE_COUNT;
    thread_state_flavor_t flavor = PPC_THREAD_STATE;
#elif CPU(PPC64)
    unsigned user_count = PPC_THREAD_STATE64_COUNT;
    thread_state_flavor_t flavor = PPC_THREAD_STATE64;
#elif CPU(ARM)
    unsigned user_count = ARM_THREAD_STATE_COUNT;
    thread_state_flavor_t flavor = ARM_THREAD_STATE;
#else
#error Unknown Architecture
#endif

    kern_return_t result = thread_get_state(platformThread, flavor, (thread_state_t)&regs, &user_count);
    if (result != KERN_SUCCESS) {
        WTFReportFatalError(__FILE__, __LINE__, WTF_PRETTY_FUNCTION, 
                            "JavaScript garbage collection failed because thread_get_state returned an error (%d). This is probably the result of running inside Rosetta, which is not supported.", result);
        CRASH();
    }
    return user_count * sizeof(usword_t);
// end OS(DARWIN)

#elif OS(WINDOWS)
    regs.ContextFlags = CONTEXT_INTEGER | CONTEXT_CONTROL;
    GetThreadContext(platformThread, &regs);
    return sizeof(CONTEXT);
#elif OS(QNX)
    memset(&regs, 0, sizeof(regs));
    regs.tid = platformThread;
    // FIXME: If we find this hurts performance, we can consider caching the fd and keeping it open.
    int fd = open("/proc/self/as", O_RDONLY);
    if (fd == -1) {
        LOG_ERROR("Unable to open /proc/self/as (errno: %d)", errno);
        CRASH();
    }
    int rc = devctl(fd, DCMD_PROC_TIDSTATUS, &regs, sizeof(regs), 0);
    if (rc != EOK) {
        LOG_ERROR("devctl(DCMD_PROC_TIDSTATUS) failed (error: %d)", rc);
        CRASH();
    }
    close(fd);
    return sizeof(struct _debug_thread_info);
#elif USE(PTHREADS)
    pthread_attr_init(&regs);
#if HAVE(PTHREAD_NP_H) || OS(NETBSD)
    // e.g. on FreeBSD 5.4, neundorf@kde.org
    pthread_attr_get_np(platformThread, &regs);
#else
    // FIXME: this function is non-portable; other POSIX systems may have different np alternatives
    pthread_getattr_np(platformThread, &regs);
#endif
    return 0;
#else
#error Need a way to get thread registers on this platform
#endif
}

static inline void* otherThreadStackPointer(const PlatformThreadRegisters& regs)
{
#if OS(DARWIN)

#if __DARWIN_UNIX03

#if CPU(X86)
    return reinterpret_cast<void*>(regs.__esp);
#elif CPU(X86_64)
    return reinterpret_cast<void*>(regs.__rsp);
#elif CPU(PPC) || CPU(PPC64)
    return reinterpret_cast<void*>(regs.__r1);
#elif CPU(ARM)
    return reinterpret_cast<void*>(regs.__sp);
#else
#error Unknown Architecture
#endif

#else // !__DARWIN_UNIX03

#if CPU(X86)
    return reinterpret_cast<void*>(regs.esp);
#elif CPU(X86_64)
    return reinterpret_cast<void*>(regs.rsp);
#elif CPU(PPC) || CPU(PPC64)
    return reinterpret_cast<void*>(regs.r1);
#else
#error Unknown Architecture
#endif

#endif // __DARWIN_UNIX03

// end OS(DARWIN)
#elif OS(WINDOWS)

#if CPU(ARM)
    return reinterpret_cast<void*>((uintptr_t) regs.Sp);
#elif CPU(MIPS)
    return reinterpret_cast<void*>((uintptr_t) regs.IntSp);
#elif CPU(X86)
    return reinterpret_cast<void*>((uintptr_t) regs.Esp);
#elif CPU(X86_64)
    return reinterpret_cast<void*>((uintptr_t) regs.Rsp);
#else
#error Unknown Architecture
#endif

#elif OS(QNX)
    return reinterpret_cast<void*>((uintptr_t) regs.sp);

#elif USE(PTHREADS)
    void* stackBase = 0;
    size_t stackSize = 0;
    int rc = pthread_attr_getstack(&regs, &stackBase, &stackSize);
    (void)rc; // FIXME: Deal with error code somehow? Seems fatal.
    ASSERT(stackBase);
    return static_cast<char*>(stackBase) + stackSize;
#else
#error Need a way to get the stack pointer for another thread on this platform
#endif
}

static void freePlatformThreadRegisters(PlatformThreadRegisters& regs)
{
#if USE(PTHREADS) && !OS(WINDOWS) && !OS(DARWIN) && !OS(QNX)
    pthread_attr_destroy(&regs);
#else
    UNUSED_PARAM(regs);
#endif
}

void MachineThreads::gatherFromOtherThread(ConservativeRoots& conservativeRoots, Thread* thread)
{
    PlatformThreadRegisters regs;
    size_t regSize = getPlatformThreadRegisters(thread->platformThread, regs);

    conservativeRoots.add(static_cast<void*>(&regs), static_cast<void*>(reinterpret_cast<char*>(&regs) + regSize));

    void* stackPointer = otherThreadStackPointer(regs);
    void* stackBase = thread->stackBase;
    swapIfBackwards(stackPointer, stackBase);
    conservativeRoots.add(stackPointer, stackBase);

    freePlatformThreadRegisters(regs);
}

void MachineThreads::gatherConservativeRoots(ConservativeRoots& conservativeRoots, void* stackCurrent)
{
    gatherFromCurrentThread(conservativeRoots, stackCurrent);

    if (m_threadSpecific) {
        PlatformThread currentPlatformThread = getCurrentPlatformThread();

        MutexLocker lock(m_registeredThreadsMutex);

#ifndef NDEBUG
        // Forbid malloc during the gather phase. The gather phase suspends
        // threads, so a malloc during gather would risk a deadlock with a
        // thread that had been suspended while holding the malloc lock.
        fastMallocForbid();
#endif
        for (Thread* thread = m_registeredThreads; thread; thread = thread->next) {
            if (!equalThread(thread->platformThread, currentPlatformThread))
                suspendThread(thread->platformThread);
        }

        // It is safe to access the registeredThreads list, because we earlier asserted that locks are being held,
        // and since this is a shared heap, they are real locks.
        for (Thread* thread = m_registeredThreads; thread; thread = thread->next) {
            if (!equalThread(thread->platformThread, currentPlatformThread))
                gatherFromOtherThread(conservativeRoots, thread);
        }

        for (Thread* thread = m_registeredThreads; thread; thread = thread->next) {
            if (!equalThread(thread->platformThread, currentPlatformThread))
                resumeThread(thread->platformThread);
        }

#ifndef NDEBUG
        fastMallocAllow();
#endif
    }
}

} // namespace JSC
