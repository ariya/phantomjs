/*
 * Copyright (C) 2010 Google Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(BLOB) || ENABLE(FILE_SYSTEM)

#include "FileThread.h"

#include "AutodrainedPool.h"
#include "Logging.h"

namespace WebCore {

FileThread::FileThread()
    : m_threadID(0)
{
    m_selfRef = this;
}

FileThread::~FileThread()
{
    ASSERT(m_queue.killed());
}

bool FileThread::start()
{
    MutexLocker lock(m_threadCreationMutex);
    if (m_threadID)
        return true;
    m_threadID = createThread(FileThread::fileThreadStart, this, "WebCore: File");
    return m_threadID;
}

void FileThread::stop()
{
    m_queue.kill();
}

void FileThread::postTask(PassOwnPtr<Task> task)
{
    m_queue.append(task);
}

class SameInstancePredicate {
public:
    SameInstancePredicate(const void* instance) : m_instance(instance) { }
    bool operator()(FileThread::Task* task) const { return task->instance() == m_instance; }
private:
    const void* m_instance;
};

void FileThread::unscheduleTasks(const void* instance)
{
    SameInstancePredicate predicate(instance);
    m_queue.removeIf(predicate);
}

void* FileThread::fileThreadStart(void* arg)
{
    FileThread* fileThread = static_cast<FileThread*>(arg);
    return fileThread->runLoop();
}

void* FileThread::runLoop()
{
    {
        // Wait for FileThread::start() to complete to have m_threadID
        // established before starting the main loop.
        MutexLocker lock(m_threadCreationMutex);
        LOG(FileAPI, "Started FileThread %p", this);
    }

    AutodrainedPool pool;
    while (OwnPtr<Task> task = m_queue.waitForMessage()) {
        task->performTask();
        pool.cycle();
    }

    LOG(FileAPI, "About to detach thread %i and clear the ref to FileThread %p, which currently has %i ref(s)", m_threadID, this, refCount());

    detachThread(m_threadID);

    // Clear the self refptr, possibly resulting in deletion
    m_selfRef = 0;

    return 0;
}

} // namespace WebCore

#endif // ENABLE(BLOB) || ENABLE(FILE_SYSTEM)
