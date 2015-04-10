/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "RunLoop.h"

#include <wtf/StdLibExtras.h>
#include <wtf/ThreadSpecific.h>

namespace WebCore {

#if !PLATFORM(MAC)
void RunLoop::setUseApplicationRunLoopOnMainRunLoop()
{
}
#endif

static RunLoop* s_mainRunLoop;

// Helper class for ThreadSpecificData.
class RunLoop::Holder {
public:
    Holder()
        : m_runLoop(adoptRef(new RunLoop))
    {
    }

    RunLoop* runLoop() const { return m_runLoop.get(); }

private:
    RefPtr<RunLoop> m_runLoop;
};

void RunLoop::initializeMainRunLoop()
{
    if (s_mainRunLoop)
        return;
    s_mainRunLoop = RunLoop::current();
}

RunLoop* RunLoop::current()
{
    DEFINE_STATIC_LOCAL(WTF::ThreadSpecific<RunLoop::Holder>, runLoopHolder, ());
    return runLoopHolder->runLoop();
}

RunLoop* RunLoop::main()
{
    ASSERT(s_mainRunLoop);
    return s_mainRunLoop;
}

void RunLoop::performWork()
{
    // It is important to handle the functions in the queue one at a time because while inside one of these
    // functions we might re-enter RunLoop::performWork() and we need to be able to pick up where we left off.
    // See http://webkit.org/b/89590 for more discussion.

    // One possible scenario when handling the function queue is as follows:
    // - RunLoop::performWork() is invoked with 1 function on the queue
    // - Handling that function results in 1 more function being enqueued
    // - Handling that one results in yet another being enqueued
    // - And so on
    //
    // In this situation one invocation of performWork() never returns so all other event sources are blocked.
    // By only handling up to the number of functions that were in the queue when performWork() is called
    // we guarantee to occasionally return from the run loop so other event sources will be allowed to spin.

    Function<void()> function;
    size_t functionsToHandle = 0;

    {
        MutexLocker locker(m_functionQueueLock);
        functionsToHandle = m_functionQueue.size();

        if (m_functionQueue.isEmpty())
            return;

        function = m_functionQueue.takeFirst();
    }

    function();

    for (size_t functionsHandled = 1; functionsHandled < functionsToHandle; ++functionsHandled) {
        {
            MutexLocker locker(m_functionQueueLock);

            // Even if we start off with N functions to handle and we've only handled less than N functions, the queue
            // still might be empty because those functions might have been handled in an inner RunLoop::performWork().
            // In that case we should bail here.
            if (m_functionQueue.isEmpty())
                break;

            function = m_functionQueue.takeFirst();
        }
        
        function();
    }
}

void RunLoop::dispatch(const Function<void()>& function)
{
    MutexLocker locker(m_functionQueueLock);
    m_functionQueue.append(function);

    wakeUp();
}

} // namespace WebCore
