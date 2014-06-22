/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
#include "ScrollingThread.h"

#if ENABLE(THREADED_SCROLLING)

#include <wtf/MainThread.h>

namespace WebCore {

ScrollingThread::ScrollingThread()
    : m_threadIdentifier(0)
{
}

bool ScrollingThread::isCurrentThread()
{
    if (!shared().m_threadIdentifier)
        return false;

    return currentThread() == shared().m_threadIdentifier;
}

void ScrollingThread::dispatch(const Function<void()>& function)
{
    shared().createThreadIfNeeded();

    {
        MutexLocker locker(shared().m_functionsMutex);
        shared().m_functions.append(function);
    }

    shared().wakeUpRunLoop();
}

static void callFunctionOnMainThread(const Function<void()>* function)
{
    callOnMainThread(*function);
    delete function;
}

void ScrollingThread::dispatchBarrier(const Function<void()>& function)
{
    dispatch(bind(callFunctionOnMainThread, new Function<void()>(function)));
}

ScrollingThread& ScrollingThread::shared()
{
    DEFINE_STATIC_LOCAL(ScrollingThread, scrollingThread, ());
    return scrollingThread;
}

void ScrollingThread::createThreadIfNeeded()
{
    if (m_threadIdentifier)
        return;

    // Wait for the thread to initialize the run loop.
    {
        MutexLocker locker(m_initializeRunLoopConditionMutex);

        m_threadIdentifier = createThread(threadCallback, this, "WebCore: Scrolling");
        
#if PLATFORM(MAC)
        while (!m_threadRunLoop)
            m_initializeRunLoopCondition.wait(m_initializeRunLoopConditionMutex);
#endif
    }
}

void ScrollingThread::threadCallback(void* scrollingThread)
{
    static_cast<ScrollingThread*>(scrollingThread)->threadBody();
}

void ScrollingThread::threadBody()
{
    initializeRunLoop();
}

void ScrollingThread::dispatchFunctionsFromScrollingThread()
{
    ASSERT(isCurrentThread());

    Vector<Function<void()> > functions;
    
    {
        MutexLocker locker(m_functionsMutex);
        m_functions.swap(functions);
    }
    
    for (size_t i = 0; i < functions.size(); ++i)
        functions[i]();
}

} // namespace WebCore

#endif // ENABLE(THREADED_SCROLLING)
