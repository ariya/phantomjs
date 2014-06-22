/*
 * Copyright (C) 2007, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Justin Haygood (jhaygood@reaktix.com)
 * Copyright (C) 2011 Research In Motion Limited. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "Threading.h"

#if USE(PTHREADS)

#include "CurrentTime.h"
#include "DateMath.h"
#include "dtoa.h"
#include "dtoa/cached-powers.h"
#include "HashMap.h"
#include "RandomNumberSeed.h"
#include "StackStats.h"
#include "StdLibExtras.h"
#include "ThreadFunctionInvocation.h"
#include "ThreadIdentifierDataPthreads.h"
#include "ThreadSpecific.h"
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/WTFThreadData.h>
#include <errno.h>

#if !COMPILER(MSVC)
#include <limits.h>
#include <sched.h>
#include <sys/time.h>
#endif

#if OS(MAC_OS_X)
#include <objc/objc-auto.h>
#endif

namespace WTF {

class PthreadState {
    WTF_MAKE_FAST_ALLOCATED;
public:
    enum JoinableState {
        Joinable, // The default thread state. The thread can be joined on.

        Joined, // Somebody waited on this thread to exit and this thread finally exited. This state is here because there can be a 
                // period of time between when the thread exits (which causes pthread_join to return and the remainder of waitOnThreadCompletion to run) 
                // and when threadDidExit is called. We need threadDidExit to take charge and delete the thread data since there's 
                // nobody else to pick up the slack in this case (since waitOnThreadCompletion has already returned).

        Detached // The thread has been detached and can no longer be joined on. At this point, the thread must take care of cleaning up after itself.
    };

    // Currently all threads created by WTF start out as joinable. 
    PthreadState(pthread_t handle)
        : m_joinableState(Joinable)
        , m_didExit(false)
        , m_pthreadHandle(handle)
    {
    }

    JoinableState joinableState() { return m_joinableState; }
    pthread_t pthreadHandle() { return m_pthreadHandle; }
    void didBecomeDetached() { m_joinableState = Detached; }
    void didExit() { m_didExit = true; }
    void didJoin() { m_joinableState = Joined; }
    bool hasExited() { return m_didExit; }

private:
    JoinableState m_joinableState;
    bool m_didExit;
    pthread_t m_pthreadHandle;
};

typedef HashMap<ThreadIdentifier, OwnPtr<PthreadState> > ThreadMap;

static Mutex* atomicallyInitializedStaticMutex;

void unsafeThreadWasDetached(ThreadIdentifier);
void threadDidExit(ThreadIdentifier);
void threadWasJoined(ThreadIdentifier);

static Mutex& threadMapMutex()
{
    DEFINE_STATIC_LOCAL(Mutex, mutex, ());
    return mutex;
}

#if OS(QNX) && CPU(ARM_THUMB2)
static void enableIEEE754Denormal()
{
    // Clear the ARM_VFP_FPSCR_FZ flag in FPSCR.
    unsigned fpscr;
    asm volatile("vmrs %0, fpscr" : "=r"(fpscr));
    fpscr &= ~0x01000000u;
    asm volatile("vmsr fpscr, %0" : : "r"(fpscr));
}
#endif

void initializeThreading()
{
    if (atomicallyInitializedStaticMutex)
        return;

#if OS(QNX) && CPU(ARM_THUMB2)
    enableIEEE754Denormal();
#endif

    WTF::double_conversion::initialize();
    // StringImpl::empty() does not construct its static string in a threadsafe fashion,
    // so ensure it has been initialized from here.
    StringImpl::empty();
    atomicallyInitializedStaticMutex = new Mutex;
    threadMapMutex();
    initializeRandomNumberGenerator();
    ThreadIdentifierData::initializeOnce();
    StackStats::initialize();
    wtfThreadData();
    s_dtoaP5Mutex = new Mutex;
    initializeDates();
}

void lockAtomicallyInitializedStaticMutex()
{
    ASSERT(atomicallyInitializedStaticMutex);
    atomicallyInitializedStaticMutex->lock();
}

void unlockAtomicallyInitializedStaticMutex()
{
    atomicallyInitializedStaticMutex->unlock();
}

static ThreadMap& threadMap()
{
    DEFINE_STATIC_LOCAL(ThreadMap, map, ());
    return map;
}

static ThreadIdentifier identifierByPthreadHandle(const pthread_t& pthreadHandle)
{
    MutexLocker locker(threadMapMutex());

    ThreadMap::iterator i = threadMap().begin();
    for (; i != threadMap().end(); ++i) {
        if (pthread_equal(i->value->pthreadHandle(), pthreadHandle) && !i->value->hasExited())
            return i->key;
    }

    return 0;
}

static ThreadIdentifier establishIdentifierForPthreadHandle(const pthread_t& pthreadHandle)
{
    ASSERT(!identifierByPthreadHandle(pthreadHandle));
    MutexLocker locker(threadMapMutex());
    static ThreadIdentifier identifierCount = 1;
    threadMap().add(identifierCount, adoptPtr(new PthreadState(pthreadHandle)));
    return identifierCount++;
}

static pthread_t pthreadHandleForIdentifierWithLockAlreadyHeld(ThreadIdentifier id)
{
    return threadMap().get(id)->pthreadHandle();
}

static void* wtfThreadEntryPoint(void* param)
{
    // Balanced by .leakPtr() in createThreadInternal.
    OwnPtr<ThreadFunctionInvocation> invocation = adoptPtr(static_cast<ThreadFunctionInvocation*>(param));
    invocation->function(invocation->data);
    return 0;
}

ThreadIdentifier createThreadInternal(ThreadFunction entryPoint, void* data, const char*)
{
    OwnPtr<ThreadFunctionInvocation> invocation = adoptPtr(new ThreadFunctionInvocation(entryPoint, data));
    pthread_t threadHandle;
    if (pthread_create(&threadHandle, 0, wtfThreadEntryPoint, invocation.get())) {
        LOG_ERROR("Failed to create pthread at entry point %p with data %p", wtfThreadEntryPoint, invocation.get());
        return 0;
    }

    // Balanced by adoptPtr() in wtfThreadEntryPoint.
    ThreadFunctionInvocation* leakedInvocation = invocation.leakPtr();
    UNUSED_PARAM(leakedInvocation);

    return establishIdentifierForPthreadHandle(threadHandle);
}

void initializeCurrentThreadInternal(const char* threadName)
{
#if HAVE(PTHREAD_SETNAME_NP)
    pthread_setname_np(threadName);
#elif OS(QNX)
    pthread_setname_np(pthread_self(), threadName);
#else
    UNUSED_PARAM(threadName);
#endif

#if OS(MAC_OS_X)
    // All threads that potentially use APIs above the BSD layer must be registered with the Objective-C
    // garbage collector in case API implementations use garbage-collected memory.
    objc_registerThreadWithCollector();
#endif

#if OS(QNX) && CPU(ARM_THUMB2)
    enableIEEE754Denormal();
#endif

    ThreadIdentifier id = identifierByPthreadHandle(pthread_self());
    ASSERT(id);
    ThreadIdentifierData::initialize(id);
}

int waitForThreadCompletion(ThreadIdentifier threadID)
{
    pthread_t pthreadHandle;
    ASSERT(threadID);

    {
        // We don't want to lock across the call to join, since that can block our thread and cause deadlock.
        MutexLocker locker(threadMapMutex());
        pthreadHandle = pthreadHandleForIdentifierWithLockAlreadyHeld(threadID);
        ASSERT(pthreadHandle);
    }

    int joinResult = pthread_join(pthreadHandle, 0);

    if (joinResult == EDEADLK)
        LOG_ERROR("ThreadIdentifier %u was found to be deadlocked trying to quit", threadID);
    else if (joinResult)
        LOG_ERROR("ThreadIdentifier %u was unable to be joined.\n", threadID);

    MutexLocker locker(threadMapMutex());
    PthreadState* state = threadMap().get(threadID);
    ASSERT(state);
    ASSERT(state->joinableState() == PthreadState::Joinable);

    // The thread has already exited, so clean up after it.
    if (state->hasExited())
        threadMap().remove(threadID);
    // The thread hasn't exited yet, so don't clean anything up. Just signal that we've already joined on it so that it will clean up after itself.
    else
        state->didJoin();

    return joinResult;
}

void detachThread(ThreadIdentifier threadID)
{
    ASSERT(threadID);

    MutexLocker locker(threadMapMutex());
    pthread_t pthreadHandle = pthreadHandleForIdentifierWithLockAlreadyHeld(threadID);
    ASSERT(pthreadHandle);

    int detachResult = pthread_detach(pthreadHandle);
    if (detachResult)
        LOG_ERROR("ThreadIdentifier %u was unable to be detached\n", threadID);

    PthreadState* state = threadMap().get(threadID);
    ASSERT(state);
    if (state->hasExited())
        threadMap().remove(threadID);
    else
        threadMap().get(threadID)->didBecomeDetached();
}

void threadDidExit(ThreadIdentifier threadID)
{
    MutexLocker locker(threadMapMutex());
    PthreadState* state = threadMap().get(threadID);
    ASSERT(state);
    
    state->didExit();

    if (state->joinableState() != PthreadState::Joinable)
        threadMap().remove(threadID);
}

void yield()
{
    sched_yield();
}

ThreadIdentifier currentThread()
{
    ThreadIdentifier id = ThreadIdentifierData::identifier();
    if (id)
        return id;

    // Not a WTF-created thread, ThreadIdentifier is not established yet.
    id = establishIdentifierForPthreadHandle(pthread_self());
    ThreadIdentifierData::initialize(id);
    return id;
}

Mutex::Mutex()
{
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_NORMAL);

    int result = pthread_mutex_init(&m_mutex, &attr);
    ASSERT_UNUSED(result, !result);

    pthread_mutexattr_destroy(&attr);
}

Mutex::~Mutex()
{
    int result = pthread_mutex_destroy(&m_mutex);
    ASSERT_UNUSED(result, !result);
}

void Mutex::lock()
{
    int result = pthread_mutex_lock(&m_mutex);
    ASSERT_UNUSED(result, !result);
}

bool Mutex::tryLock()
{
    int result = pthread_mutex_trylock(&m_mutex);

    if (result == 0)
        return true;
    if (result == EBUSY)
        return false;

    ASSERT_NOT_REACHED();
    return false;
}

void Mutex::unlock()
{
    int result = pthread_mutex_unlock(&m_mutex);
    ASSERT_UNUSED(result, !result);
}

ThreadCondition::ThreadCondition()
{ 
    pthread_cond_init(&m_condition, NULL);
}

ThreadCondition::~ThreadCondition()
{
    pthread_cond_destroy(&m_condition);
}
    
void ThreadCondition::wait(Mutex& mutex)
{
    int result = pthread_cond_wait(&m_condition, &mutex.impl());
    ASSERT_UNUSED(result, !result);
}

bool ThreadCondition::timedWait(Mutex& mutex, double absoluteTime)
{
    if (absoluteTime < currentTime())
        return false;

    if (absoluteTime > INT_MAX) {
        wait(mutex);
        return true;
    }

    int timeSeconds = static_cast<int>(absoluteTime);
    int timeNanoseconds = static_cast<int>((absoluteTime - timeSeconds) * 1E9);

    timespec targetTime;
    targetTime.tv_sec = timeSeconds;
    targetTime.tv_nsec = timeNanoseconds;

    return pthread_cond_timedwait(&m_condition, &mutex.impl(), &targetTime) == 0;
}

void ThreadCondition::signal()
{
    int result = pthread_cond_signal(&m_condition);
    ASSERT_UNUSED(result, !result);
}

void ThreadCondition::broadcast()
{
    int result = pthread_cond_broadcast(&m_condition);
    ASSERT_UNUSED(result, !result);
}

} // namespace WTF

#endif // USE(PTHREADS)
