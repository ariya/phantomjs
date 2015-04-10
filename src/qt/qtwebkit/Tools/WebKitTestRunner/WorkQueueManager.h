/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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

#ifndef WorkQueueManager_h
#define WorkQueueManager_h

#include <wtf/Deque.h>
#include <wtf/OwnPtr.h>
#include <wtf/text/WTFString.h>

namespace WTR {

class WorkQueueManager {
    WTF_MAKE_NONCOPYABLE(WorkQueueManager);
public:
    WorkQueueManager();
    ~WorkQueueManager();

    bool isWorkQueueEmpty() const { return m_workQueue.isEmpty(); }
    void clearWorkQueue();
    bool processWorkQueue(); // Returns 'true' if queue is processed (no new loading is started), returns 'false' otherwise.

    void queueLoad(const String& url, const String& target);
    void queueLoadHTMLString(const String& content, const String& baseURL, const String& unreachableURL);
    void queueBackNavigation(unsigned howFarBackward);
    void queueForwardNavigation(unsigned howFarForward);
    void queueReload();
    void queueLoadingScript(const String& script);
    void queueNonLoadingScript(const String& script);

private:    
    typedef Deque<OwnPtr<class WorkQueueItem> > WorkQueue;

    void enqueue(WorkQueueItem*); // Adopts pointer.

    WorkQueue m_workQueue;
    bool m_processing;
};

} // namespace WTR

#endif // WorkQueueManager_h
