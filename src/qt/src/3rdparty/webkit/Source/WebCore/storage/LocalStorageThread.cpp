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
#include "LocalStorageThread.h"

#if ENABLE(DOM_STORAGE)

#include "LocalStorageTask.h"
#include "StorageAreaSync.h"

namespace WebCore {

PassOwnPtr<LocalStorageThread> LocalStorageThread::create()
{
    return adoptPtr(new LocalStorageThread);
}

LocalStorageThread::LocalStorageThread()
    : m_threadID(0)
{
}

LocalStorageThread::~LocalStorageThread()
{
    ASSERT(isMainThread());
    ASSERT(!m_threadID);
}

bool LocalStorageThread::start()
{
    ASSERT(isMainThread());
    if (!m_threadID)
        m_threadID = createThread(LocalStorageThread::threadEntryPointCallback, this, "WebCore: LocalStorage");
    return m_threadID;
}

void* LocalStorageThread::threadEntryPointCallback(void* thread)
{
    return static_cast<LocalStorageThread*>(thread)->threadEntryPoint();
}

void* LocalStorageThread::threadEntryPoint()
{
    ASSERT(!isMainThread());
    while (OwnPtr<LocalStorageTask> task = m_queue.waitForMessage())
        task->performTask();

    return 0;
}

void LocalStorageThread::scheduleTask(PassOwnPtr<LocalStorageTask> task)
{
    ASSERT(isMainThread());
    ASSERT(!m_queue.killed() && m_threadID);
    m_queue.append(task);
}

void LocalStorageThread::terminate()
{
    ASSERT(isMainThread());
    ASSERT(!m_queue.killed() && m_threadID);
    // Even in weird, exceptional cases, don't wait on a nonexistent thread to terminate.
    if (!m_threadID)
        return;

    void* returnValue;
    m_queue.append(LocalStorageTask::createTerminate(this));
    waitForThreadCompletion(m_threadID, &returnValue);
    ASSERT(m_queue.killed());
    m_threadID = 0;
}

void LocalStorageThread::performTerminate()
{
    ASSERT(!isMainThread());
    m_queue.kill();
}

}

#endif // ENABLE(DOM_STORAGE)
