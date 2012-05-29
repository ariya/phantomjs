/*
 * Copyright (C) 2011 University of Szeged
 * Copyright (C) 2011 Gabor Loki <loki@webkit.org>
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY UNIVERSITY OF SZEGED ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL UNIVERSITY OF SZEGED OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(PARALLEL_JOBS) && ENABLE(THREADING_GENERIC)

#include "ParallelJobs.h"

namespace WTF {

Vector< RefPtr<ParallelEnvironment::ThreadPrivate> >* ParallelEnvironment::s_threadPool = 0;

bool ParallelEnvironment::ThreadPrivate::tryLockFor(ParallelEnvironment* parent)
{
    bool locked = m_mutex.tryLock();

    if (!locked)
        return false;

    if (m_parent) {
        m_mutex.unlock();
        return false;
    }

    if (!m_threadID)
        m_threadID = createThread(&ParallelEnvironment::ThreadPrivate::workerThread, this, "Parallel worker");

    if (m_threadID)
        m_parent = parent;

    m_mutex.unlock();
    return m_threadID;
}

void ParallelEnvironment::ThreadPrivate::execute(ThreadFunction threadFunction, void* parameters)
{
    MutexLocker lock(m_mutex);

    m_threadFunction = threadFunction;
    m_parameters = parameters;
    m_running = true;
    m_threadCondition.signal();
}

void ParallelEnvironment::ThreadPrivate::waitForFinish()
{
    MutexLocker lock(m_mutex);

    while (m_running)
        m_threadCondition.wait(m_mutex);
}

void* ParallelEnvironment::ThreadPrivate::workerThread(void* threadData)
{
    ThreadPrivate* sharedThread = reinterpret_cast<ThreadPrivate*>(threadData);
    MutexLocker lock(sharedThread->m_mutex);

    while (sharedThread->m_threadID) {
        if (sharedThread->m_running) {
            (*sharedThread->m_threadFunction)(sharedThread->m_parameters);
            sharedThread->m_running = false;
            sharedThread->m_parent = 0;
            sharedThread->m_threadCondition.signal();
        }

        sharedThread->m_threadCondition.wait(sharedThread->m_mutex);
    }
    return 0;
}

} // namespace WTF

#endif // ENABLE(PARALLEL_JOBS) && ENABLE(THREADING_GENERIC)
