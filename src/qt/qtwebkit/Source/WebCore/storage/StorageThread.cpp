/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
#include "StorageThread.h"

#include "StorageAreaSync.h"
#include <wtf/AutodrainedPool.h>
#include <wtf/HashSet.h>
#include <wtf/MainThread.h>

namespace WebCore {

static HashSet<StorageThread*>& activeStorageThreads()
{
    ASSERT(isMainThread());
    DEFINE_STATIC_LOCAL(HashSet<StorageThread*>, threads, ());
    return threads;
}

PassOwnPtr<StorageThread> StorageThread::create()
{
    return adoptPtr(new StorageThread);
}

StorageThread::StorageThread()
    : m_threadID(0)
{
    ASSERT(isMainThread());
}

StorageThread::~StorageThread()
{
    ASSERT(isMainThread());
    ASSERT(!m_threadID);
}

bool StorageThread::start()
{
    ASSERT(isMainThread());
    if (!m_threadID)
        m_threadID = createThread(StorageThread::threadEntryPointCallback, this, "WebCore: LocalStorage");
    activeStorageThreads().add(this);
    return m_threadID;
}

void StorageThread::threadEntryPointCallback(void* thread)
{
    static_cast<StorageThread*>(thread)->threadEntryPoint();
}

void StorageThread::threadEntryPoint()
{
    ASSERT(!isMainThread());

    while (OwnPtr<Function<void ()> > function = m_queue.waitForMessage()) {
        AutodrainedPool pool;
        (*function)();
    }
}

void StorageThread::dispatch(const Function<void ()>& function)
{
    ASSERT(isMainThread());
    ASSERT(!m_queue.killed() && m_threadID);
    m_queue.append(adoptPtr(new Function<void ()>(function)));
}

void StorageThread::terminate()
{
    ASSERT(isMainThread());
    ASSERT(!m_queue.killed() && m_threadID);
    activeStorageThreads().remove(this);
    // Even in weird, exceptional cases, don't wait on a nonexistent thread to terminate.
    if (!m_threadID)
        return;

    m_queue.append(adoptPtr(new Function<void ()>((bind(&StorageThread::performTerminate, this)))));
    waitForThreadCompletion(m_threadID);
    ASSERT(m_queue.killed());
    m_threadID = 0;
}

void StorageThread::performTerminate()
{
    ASSERT(!isMainThread());
    m_queue.kill();
}

void StorageThread::releaseFastMallocFreeMemoryInAllThreads()
{
    HashSet<StorageThread*>& threads = activeStorageThreads();

    for (HashSet<StorageThread*>::iterator it = threads.begin(), end = threads.end(); it != end; ++it)
        (*it)->dispatch(bind(WTF::releaseFastMallocFreeMemory));
}

}
