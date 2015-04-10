/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef EventDispatcher_h
#define EventDispatcher_h

#include "Connection.h"
#include <wtf/HashMap.h>
#include <wtf/Noncopyable.h>
#include <wtf/RefPtr.h>
#include <wtf/ThreadingPrimitives.h>

namespace WebCore {
    class ScrollingTree;
}

namespace WebKit {

class WebEvent;
class WebPage;
class WebWheelEvent;

#if ENABLE(GESTURE_EVENTS)
class WebGestureEvent;
#endif

class EventDispatcher : public CoreIPC::Connection::WorkQueueMessageReceiver {
public:
    static PassRefPtr<EventDispatcher> create();
    ~EventDispatcher();

#if ENABLE(THREADED_SCROLLING)
    void addScrollingTreeForPage(WebPage*);
    void removeScrollingTreeForPage(WebPage*);
#endif

    void initializeConnection(CoreIPC::Connection*);

private:
    EventDispatcher();

    // CoreIPC::Connection::WorkQueueMessageReceiver.
    virtual void didReceiveMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&) OVERRIDE;

    // Message handlers
    void wheelEvent(uint64_t pageID, const WebWheelEvent&, bool canGoBack, bool canGoForward);
#if ENABLE(GESTURE_EVENTS)
    void gestureEvent(uint64_t pageID, const WebGestureEvent&);
#endif

    // This is called on the main thread.
    void dispatchWheelEvent(uint64_t pageID, const WebWheelEvent&);
#if ENABLE(GESTURE_EVENTS)
    void dispatchGestureEvent(uint64_t pageID, const WebGestureEvent&);
#endif

#if ENABLE(THREADED_SCROLLING)
    void sendDidReceiveEvent(uint64_t pageID, const WebEvent&, bool didHandleEvent);
#endif

    RefPtr<WorkQueue> m_queue;

#if ENABLE(THREADED_SCROLLING)
    Mutex m_scrollingTreesMutex;
    HashMap<uint64_t, RefPtr<WebCore::ScrollingTree> > m_scrollingTrees;
#endif
};

} // namespace WebKit

#endif // EventDispatcher_h
