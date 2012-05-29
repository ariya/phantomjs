/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#ifndef CrossThreadRefCounted_h
#define CrossThreadRefCounted_h

#include "PassRefPtr.h"
#include "RefCounted.h"
#include "Threading.h"

namespace WTF {

    // Used to allowing sharing data across classes and threads (like ThreadSafeRefCounted).
    //
    // Why not just use ThreadSafeRefCounted?
    // ThreadSafeRefCounted can have a significant perf impact when used in low level classes
    // (like UString) that get ref/deref'ed a lot. This class has the benefit of doing fast ref
    // counts like RefPtr whenever possible, but it has the downside that you need to copy it
    // to use it on another thread.
    //
    // Is this class threadsafe?
    // While each instance of the class is not threadsafe, the copied instance is threadsafe
    // with respect to the original and any other copies.  The underlying m_data is jointly
    // owned by the original instance and all copies.
    template<class T>
    class CrossThreadRefCounted {
        WTF_MAKE_NONCOPYABLE(CrossThreadRefCounted);
    public:
        static PassRefPtr<CrossThreadRefCounted<T> > create(T* data)
        {
            return adoptRef(new CrossThreadRefCounted<T>(data, 0));
        }

        // Used to make an instance that can be used on another thread.
        PassRefPtr<CrossThreadRefCounted<T> > crossThreadCopy();

        void ref();
        void deref();
        T* release();

        bool isShared() const
        {
            return !m_refCounter.hasOneRef() || (m_threadSafeRefCounter && !m_threadSafeRefCounter->hasOneRef());
        }

    private:
        CrossThreadRefCounted(T* data, ThreadSafeRefCountedBase* threadedCounter)
            : m_threadSafeRefCounter(threadedCounter)
            , m_data(data)
#ifndef NDEBUG
            , m_threadId(0)
#endif
        {
            // We use RefCountedBase in an unusual way here, so get rid of the requirement
            // that adoptRef be called on it.
            m_refCounter.relaxAdoptionRequirement();
        }

        ~CrossThreadRefCounted()
        {
            if (!m_threadSafeRefCounter)
                delete m_data;
        }

        void threadSafeDeref();

#ifndef NDEBUG
        bool isOwnedByCurrentThread() const { return !m_threadId || m_threadId == currentThread(); }
#endif

        RefCountedBase m_refCounter;
        ThreadSafeRefCountedBase* m_threadSafeRefCounter;
        T* m_data;
#ifndef NDEBUG
        ThreadIdentifier m_threadId;
#endif
    };

    template<class T>
    void CrossThreadRefCounted<T>::ref()
    {
        ASSERT(isOwnedByCurrentThread());
        m_refCounter.ref();
#ifndef NDEBUG
        // Store the threadId as soon as the ref count gets to 2.
        // The class gets created with a ref count of 1 and then passed
        // to another thread where to ref count get increased.  This
        // is a heuristic but it seems to always work and has helped
        // find some bugs.
        if (!m_threadId && m_refCounter.refCount() == 2)
            m_threadId = currentThread();
#endif
    }

    template<class T>
    void CrossThreadRefCounted<T>::deref()
    {
        ASSERT(isOwnedByCurrentThread());
        if (m_refCounter.derefBase()) {
            threadSafeDeref();
            delete this;
        } else {
#ifndef NDEBUG
            // Clear the threadId when the ref goes to 1 because it
            // is safe to be passed to another thread at this point.
            if (m_threadId && m_refCounter.refCount() == 1)
                m_threadId = 0;
#endif
        }
    }

    template<class T>
    T* CrossThreadRefCounted<T>::release()
    {
        ASSERT(!isShared());

        T* data = m_data;
        m_data = 0;
        return data;
    }

    template<class T>
    PassRefPtr<CrossThreadRefCounted<T> > CrossThreadRefCounted<T>::crossThreadCopy()
    {
        ASSERT(isOwnedByCurrentThread());
        if (m_threadSafeRefCounter)
            m_threadSafeRefCounter->ref();
        else
            m_threadSafeRefCounter = new ThreadSafeRefCountedBase(2);

        return adoptRef(new CrossThreadRefCounted<T>(m_data, m_threadSafeRefCounter));
    }


    template<class T>
    void CrossThreadRefCounted<T>::threadSafeDeref()
    {
        if (m_threadSafeRefCounter && m_threadSafeRefCounter->derefBase()) {
            delete m_threadSafeRefCounter;
            m_threadSafeRefCounter = 0;
        }
    }
} // namespace WTF

using WTF::CrossThreadRefCounted;

#endif // CrossThreadRefCounted_h
