/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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
#ifndef SQLCallbackWrapper_h
#define SQLCallbackWrapper_h

#if ENABLE(SQL_DATABASE)

#include "ScriptExecutionContext.h"
#include <wtf/ThreadingPrimitives.h>

namespace WebCore {

// A helper class to safely dereference the callback objects held by
// SQLStatement and SQLTransaction on the proper thread. The 'wrapped'
// callback is dereferenced:
// - by destructing the enclosing wrapper - on any thread
// - by calling clear() - on any thread
// - by unwrapping and then dereferencing normally - on context thread only
template<typename T> class SQLCallbackWrapper {
public:
    SQLCallbackWrapper(PassRefPtr<T> callback, ScriptExecutionContext* scriptExecutionContext)
        : m_callback(callback)
        , m_scriptExecutionContext(m_callback ? scriptExecutionContext : 0)
    {
        ASSERT(!m_callback || (m_scriptExecutionContext.get() && m_scriptExecutionContext->isContextThread()));
    }

    ~SQLCallbackWrapper()
    {
        clear();
    }

    void clear()
    {
        ScriptExecutionContext* context;
        T* callback;
        {
            MutexLocker locker(m_mutex);
            if (!m_callback) {
                ASSERT(!m_scriptExecutionContext);
                return;
            }
            if (m_scriptExecutionContext->isContextThread()) {
                m_callback = 0;
                m_scriptExecutionContext = 0;
                return;
            }
            context = m_scriptExecutionContext.release().leakRef();
            callback = m_callback.release().leakRef();
        }
        context->postTask(SafeReleaseTask::create(callback));
    }

    PassRefPtr<T> unwrap()
    {
        MutexLocker locker(m_mutex);
        ASSERT(!m_callback || m_scriptExecutionContext->isContextThread());
        m_scriptExecutionContext = 0;
        return m_callback.release();
    }

    // Useful for optimizations only, please test the return value of unwrap to be sure.
    bool hasCallback() const { return m_callback; }

private:
    class SafeReleaseTask : public ScriptExecutionContext::Task {
    public:
        static PassOwnPtr<SafeReleaseTask> create(T* callbackToRelease)
        {
            return adoptPtr(new SafeReleaseTask(callbackToRelease));
        }

        virtual void performTask(ScriptExecutionContext* context)
        {
            ASSERT(m_callbackToRelease && context && context->isContextThread());
            m_callbackToRelease->deref();
            context->deref();
        }

        virtual bool isCleanupTask() const { return true; }

    private:
        explicit SafeReleaseTask(T* callbackToRelease)
            : m_callbackToRelease(callbackToRelease)
        {
        }

        T* m_callbackToRelease;
    };

    Mutex m_mutex;
    RefPtr<T> m_callback;
    RefPtr<ScriptExecutionContext> m_scriptExecutionContext;
};

} // namespace WebCore

#endif // ENABLE(SQL_DATABASE)

#endif // SQLCallbackWrapper_h
