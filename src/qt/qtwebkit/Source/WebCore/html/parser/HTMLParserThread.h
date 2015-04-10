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

#ifndef HTMLParserThread_h
#define HTMLParserThread_h

#if ENABLE(THREADED_HTML_PARSER)

#include <wtf/Functional.h>
#include <wtf/MessageQueue.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/Threading.h>

namespace WebCore {

// FIXME:: Closure is the Chromium-name for Function<void()>, but we may want something else for WebCore.
typedef Function<void()> Closure;

class HTMLParserThread {
public:
    static PassOwnPtr<HTMLParserThread> create()
    {
        return adoptPtr(new HTMLParserThread());
    }
    ~HTMLParserThread();

    static HTMLParserThread* shared();

    bool start();
    void stop();

    void postTask(const Closure&);

    ThreadIdentifier threadId() const { return m_threadID; }

private:
    HTMLParserThread();

    static void threadStart(void*);
    void runLoop();

    Mutex m_threadCreationMutex;
    MessageQueue<Closure> m_queue;
    ThreadIdentifier m_threadID;
};

} // namespace WebCore

#endif // ENABLE(THREADED_HTML_PARSER)

#endif // HTMLParserThread_h
