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

#ifndef ParallelJobsGeneric_h
#define ParallelJobsGeneric_h

#if ENABLE(THREADING_GENERIC)

#include <wtf/RefCounted.h>
#include <wtf/Threading.h>

namespace WTF {

static const unsigned int maxParallelThreads = 2;

class ParallelEnvironment {
    WTF_MAKE_FAST_ALLOCATED;
public:
    typedef void (*ThreadFunction)(void*);

    ParallelEnvironment(ThreadFunction threadFunction, size_t sizeOfParameter, unsigned int requestedJobNumber) :
        m_threadFunction(threadFunction),
        m_sizeOfParameter(sizeOfParameter)
    {
        if (!requestedJobNumber || requestedJobNumber > maxParallelThreads)
            requestedJobNumber = maxParallelThreads;

        if (!s_threadPool)
            s_threadPool = new Vector< RefPtr<ThreadPrivate> >();

        // The main thread should be also a worker.
        unsigned int maxNewThreads = requestedJobNumber - 1;

        for (unsigned int i = 0; i < maxParallelThreads && m_threads.size() < maxNewThreads; ++i) {
            if (s_threadPool->size() < i + 1)
                s_threadPool->append(ThreadPrivate::create());

            if ((*s_threadPool)[i]->tryLockFor(this))
                m_threads.append((*s_threadPool)[i]);
        }

        m_numberOfJobs = m_threads.size() + 1;
    }

    int numberOfJobs()
    {
        return m_numberOfJobs;
    }

    void execute(unsigned char* parameters)
    {
        size_t i;
        for (i = 0; i < m_threads.size(); ++i) {
            m_threads[i]->execute(m_threadFunction, parameters);
            parameters += m_sizeOfParameter;
        }

        // The work for the main thread
        (*m_threadFunction)(parameters);

        // Wait until all jobs are done.
        for (i = 0; i < m_threads.size(); ++i)
            m_threads[i]->waitForFinish();
    }

    class ThreadPrivate : public RefCounted<ThreadPrivate> {
    public:
        ThreadPrivate()
            : m_threadID(0)
            , m_running(false)
            , m_parent(0)
        {
        }

        bool tryLockFor(ParallelEnvironment*);

        void execute(ThreadFunction, void*);

        void waitForFinish();

        static PassRefPtr<ThreadPrivate> create()
        {
            return adoptRef(new ThreadPrivate());
        }

        static void* workerThread(void*);

    private:
        ThreadIdentifier m_threadID;
        bool m_running;
        ParallelEnvironment* m_parent;

        mutable Mutex m_mutex;
        ThreadCondition m_threadCondition;

        ThreadFunction m_threadFunction;
        void* m_parameters;
    };

private:
    ThreadFunction m_threadFunction;
    size_t m_sizeOfParameter;
    int m_numberOfJobs;

    Vector< RefPtr<ThreadPrivate> > m_threads;
    static Vector< RefPtr<ThreadPrivate> >* s_threadPool;
};

} // namespace WTF

#endif // ENABLE(THREADING_GENERIC)


#endif // ParallelJobsGeneric_h
