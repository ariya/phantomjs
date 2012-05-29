/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Justin Haygood (jhaygood@reaktix.com)
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
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
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

#if !USE(PTHREADS)

#include "CurrentTime.h"
#include "HashMap.h"
#include "MainThread.h"
#include "RandomNumberSeed.h"
#include <wtf/StdLibExtras.h>

#include <glib.h>
#include <limits.h>

namespace WTF {

typedef HashMap<ThreadIdentifier, GThread*> ThreadMap;

static Mutex* atomicallyInitializedStaticMutex;

static Mutex& threadMapMutex()
{
    DEFINE_STATIC_LOCAL(Mutex, mutex, ());
    return mutex;
}

void initializeThreading()
{
    if (!g_thread_supported())
        g_thread_init(NULL);
    ASSERT(g_thread_supported());

    if (!atomicallyInitializedStaticMutex) {
        atomicallyInitializedStaticMutex = new Mutex;
        threadMapMutex();
        initializeRandomNumberGenerator();
    }
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

static ThreadIdentifier identifierByGthreadHandle(GThread*& thread)
{
    MutexLocker locker(threadMapMutex());

    ThreadMap::iterator i = threadMap().begin();
    for (; i != threadMap().end(); ++i) {
        if (i->second == thread)
            return i->first;
    }

    return 0;
}

static ThreadIdentifier establishIdentifierForThread(GThread*& thread)
{
    ASSERT(!identifierByGthreadHandle(thread));

    MutexLocker locker(threadMapMutex());

    static ThreadIdentifier identifierCount = 1;

    threadMap().add(identifierCount, thread);

    return identifierCount++;
}

static GThread* threadForIdentifier(ThreadIdentifier id)
{
    MutexLocker locker(threadMapMutex());

    return threadMap().get(id);
}

static void clearThreadForIdentifier(ThreadIdentifier id)
{
    MutexLocker locker(threadMapMutex());

    ASSERT(threadMap().contains(id));

    threadMap().remove(id);
}

ThreadIdentifier createThreadInternal(ThreadFunction entryPoint, void* data, const char*)
{
    GThread* thread;
    if (!(thread = g_thread_create(entryPoint, data, TRUE, 0))) {
        LOG_ERROR("Failed to create thread at entry point %p with data %p", entryPoint, data);
        return 0;
    }

    ThreadIdentifier threadID = establishIdentifierForThread(thread);
    return threadID;
}

void initializeCurrentThreadInternal(const char*)
{
}

int waitForThreadCompletion(ThreadIdentifier threadID, void** result)
{
    ASSERT(threadID);

    GThread* thread = threadForIdentifier(threadID);

    void* joinResult = g_thread_join(thread);
    if (result)
        *result = joinResult;

    clearThreadForIdentifier(threadID);
    return 0;
}

void detachThread(ThreadIdentifier)
{
}

ThreadIdentifier currentThread()
{
    GThread* currentThread = g_thread_self();
    if (ThreadIdentifier id = identifierByGthreadHandle(currentThread))
        return id;
    return establishIdentifierForThread(currentThread);
}

void yield()
{
    g_thread_yield();
}

Mutex::Mutex()
    : m_mutex(g_mutex_new())
{
}

Mutex::~Mutex()
{
}

void Mutex::lock()
{
    g_mutex_lock(m_mutex.get());
}

bool Mutex::tryLock()
{
    return g_mutex_trylock(m_mutex.get());
}

void Mutex::unlock()
{
    g_mutex_unlock(m_mutex.get());
}

ThreadCondition::ThreadCondition()
    : m_condition(g_cond_new())
{
}

ThreadCondition::~ThreadCondition()
{
}

void ThreadCondition::wait(Mutex& mutex)
{
    g_cond_wait(m_condition.get(), mutex.impl().get());
}

bool ThreadCondition::timedWait(Mutex& mutex, double absoluteTime)
{
    // Time is in the past - return right away.
    if (absoluteTime < currentTime())
        return false;
    
    // Time is too far in the future for g_cond_timed_wait - wait forever.
    if (absoluteTime > INT_MAX) {
        wait(mutex);
        return true;
    }

    int timeSeconds = static_cast<int>(absoluteTime);
    int timeMicroseconds = static_cast<int>((absoluteTime - timeSeconds) * 1000000.0);
    
    GTimeVal targetTime;
    targetTime.tv_sec = timeSeconds;
    targetTime.tv_usec = timeMicroseconds;

    return g_cond_timed_wait(m_condition.get(), mutex.impl().get(), &targetTime);
}

void ThreadCondition::signal()
{
    g_cond_signal(m_condition.get());
}

void ThreadCondition::broadcast()
{
    g_cond_broadcast(m_condition.get());
}


}

#endif // !USE(PTHREADS)
