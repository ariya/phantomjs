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

#if ENABLE(THREADING_GENERIC)

#include "ParallelJobs.h"
#include <wtf/NumberOfCores.h>

namespace WTF {

Vector< RefPtr<ParallelEnvironment::ThreadPrivate> >* ParallelEnvironment::s_threadPool = 0;

ParallelEnvironment::ParallelEnvironment(ThreadFunction threadFunction, size_t sizeOfParameter, int requestedJobNumber) :
    m_threadFunction(threadFunction),
    m_sizeOfParameter(sizeOfParameter)
{
    ASSERT_ARG(requestedJobNumber, requestedJobNumber >= 1);

    int maxNumberOfCores = numberOfProcessorCores();

    if (!requestedJobNumber || requestedJobNumber > maxNumberOfCores)
        requestedJobNumber = static_cast<unsigned>(maxNumberOfCores);

    if (!s_threadPool)
        s_threadPool = new Vector< RefPtr<ThreadPrivate> >();

    // The main thread should be also a worker.
    int maxNumberOfNewThreads = requestedJobNumber - 1;

    for (int i = 0; i < maxNumberOfCores && m_threads.size() < static_cast<unsigned>(maxNumberOfNewThreads); ++i) {
        if (s_threadPool->size() < static_cast<unsigned>(i) + 1U)
            s_threadPool->append(ThreadPrivate::create());

        if ((*s_threadPool)[i]->tryLockFor(this))
            m_threads.append((*s_threadPool)[i]);
    }

    m_numberOfJobs = m_threads.size() + 1;
}

void ParallelEnvironment::execute(void* parameters)
{
    unsigned char* currentParameter = static_cast<unsigned char*>(parameters);
    size_t i;
    for (i = 0; i < m_threads.size(); ++i) {
        m_threads[i]->execute(m_threadFunction, currentParameter);
        currentParameter += m_sizeOfParameter;
    }

    // The work for the main thread.
    (*m_threadFunction)(currentParameter);

    // Wait until all jobs are done.
    for (i = 0; i < m_threads.size(); ++i)
        m_threads[i]->waitForFinish();
}

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

void ParallelEnvironment::ThreadPrivate::workerThread(void* threadData)
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
}

} // namespace WTF
#endif // ENABLE(THREADING_GENERIC)
