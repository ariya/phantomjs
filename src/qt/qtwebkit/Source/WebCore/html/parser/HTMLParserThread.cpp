/*
 * Copyright (C) 2013 Google Inc.  All rights reserved.
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

#if ENABLE(THREADED_HTML_PARSER)

#include "HTMLParserThread.h"

namespace WebCore {

HTMLParserThread::HTMLParserThread()
    : m_threadID(0)
{
}

HTMLParserThread::~HTMLParserThread()
{
    ASSERT(m_queue.killed());
}

bool HTMLParserThread::start()
{
    MutexLocker lock(m_threadCreationMutex);
    if (m_threadID)
        return true;
    m_threadID = createThread(HTMLParserThread::threadStart, this, "WebCore: HTMLParser");
    return m_threadID;
}

void HTMLParserThread::stop()
{
    m_queue.kill();
    waitForThreadCompletion(m_threadID);
}

HTMLParserThread* HTMLParserThread::shared()
{
    static HTMLParserThread* thread;
    if (!thread) {
        thread = HTMLParserThread::create().leakPtr();
        thread->start();
    }
    return thread;
}

void HTMLParserThread::postTask(const Closure& function)
{
    m_queue.append(adoptPtr(new Closure(function)));
}

void HTMLParserThread::threadStart(void* arg)
{
    HTMLParserThread* thread = static_cast<HTMLParserThread*>(arg);
    thread->runLoop();
}

void HTMLParserThread::runLoop()
{
    {
        // Wait for HTMLParserThread::start() to complete to have m_threadID
        // established before starting the main loop.
        MutexLocker lock(m_threadCreationMutex);
    }
    while (OwnPtr<Closure> function = m_queue.waitForMessage())
        (*function)();

    // stop() will wait to join the thread, so we do not detach here.
}

} // namespace WebCore

#endif // ENABLE(THREADED_HTML_PARSER)
