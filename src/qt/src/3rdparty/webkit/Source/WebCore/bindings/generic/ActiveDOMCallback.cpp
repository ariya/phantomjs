/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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
#include "ActiveDOMCallback.h"

#include "ActiveDOMObject.h"
#include "ScriptExecutionContext.h"
#include <wtf/PassOwnPtr.h>
#include <wtf/ThreadingPrimitives.h>

namespace WebCore {

class ActiveDOMObjectCallbackImpl : public ActiveDOMObject {
public:
    ActiveDOMObjectCallbackImpl(ScriptExecutionContext* context)
        : ActiveDOMObject(context, this)
        , m_suspended(false)
        , m_stopped(false)
    {
    }

    virtual void contextDestroyed()
    {
        MutexLocker locker(m_mutex);
        ActiveDOMObject::contextDestroyed();
    }
    virtual bool canSuspend() const { return false; }
    virtual void suspend(ReasonForSuspension)
    {
        MutexLocker locker(m_mutex);
        m_suspended = true;
    }
    virtual void resume()
    {
        MutexLocker locker(m_mutex);
        m_suspended = false;
    }
    virtual void stop()
    {
        MutexLocker locker(m_mutex);
        m_stopped = true;
    }
    bool canInvokeCallback()
    {
        MutexLocker locker(m_mutex);
        return (!m_suspended && !m_stopped);
    }
    ScriptExecutionContext* scriptExecutionContext()
    {
        MutexLocker locker(m_mutex);
        return ActiveDOMObject::scriptExecutionContext();
    }
    Mutex& mutex() { return m_mutex; }

private:
    Mutex m_mutex;
    bool m_suspended;
    bool m_stopped;
};

static void destroyOnContextThread(PassOwnPtr<ActiveDOMObjectCallbackImpl>);

class DestroyOnContextThreadTask : public ScriptExecutionContext::Task {
public:
    static PassOwnPtr<DestroyOnContextThreadTask> create(PassOwnPtr<ActiveDOMObjectCallbackImpl> impl)
    {
        return adoptPtr(new DestroyOnContextThreadTask(impl));
    }

    virtual void performTask(ScriptExecutionContext*)
    {
        destroyOnContextThread(m_impl.release());
    }

private:
    DestroyOnContextThreadTask(PassOwnPtr<ActiveDOMObjectCallbackImpl> impl)
        : m_impl(impl)
    {
    }

    OwnPtr<ActiveDOMObjectCallbackImpl> m_impl;
};

static void destroyOnContextThread(PassOwnPtr<ActiveDOMObjectCallbackImpl> impl)
{
    OwnPtr<ActiveDOMObjectCallbackImpl> implOwnPtr = impl;

    ScriptExecutionContext* context = implOwnPtr->scriptExecutionContext();
    MutexLocker locker(implOwnPtr->mutex());
    if (context && !context->isContextThread())
        context->postTask(DestroyOnContextThreadTask::create(implOwnPtr.release()));
}

ActiveDOMCallback::ActiveDOMCallback(ScriptExecutionContext* context)
    : m_impl(adoptPtr(new ActiveDOMObjectCallbackImpl(context)))
{
}

ActiveDOMCallback::~ActiveDOMCallback()
{
    destroyOnContextThread(m_impl.release());
}

bool ActiveDOMCallback::canInvokeCallback() const
{
    return m_impl->canInvokeCallback();
}

ScriptExecutionContext* ActiveDOMCallback::scriptExecutionContext() const
{
    return m_impl->scriptExecutionContext();
}

} // namespace WebCore
