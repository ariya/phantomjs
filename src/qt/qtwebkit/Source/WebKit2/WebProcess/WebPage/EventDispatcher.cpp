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

#include "config.h"
#include "EventDispatcher.h"

#include "EventDispatcherMessages.h"
#include "WebEvent.h"
#include "WebEventConversion.h"
#include "WebPage.h"
#include "WebPageProxyMessages.h"
#include "WebProcess.h"
#include <WebCore/Page.h>
#include <WebCore/RunLoop.h>
#include <wtf/MainThread.h>

#if ENABLE(THREADED_SCROLLING)
#include <WebCore/ScrollingCoordinator.h>
#include <WebCore/ScrollingThread.h>
#include <WebCore/ScrollingTree.h>
#endif

using namespace WebCore;

namespace WebKit {

PassRefPtr<EventDispatcher> EventDispatcher::create()
{
    return adoptRef(new EventDispatcher);
}

EventDispatcher::EventDispatcher()
    : m_queue(WorkQueue::create("com.apple.WebKit.EventDispatcher"))
{
}

EventDispatcher::~EventDispatcher()
{
}

#if ENABLE(THREADED_SCROLLING)
void EventDispatcher::addScrollingTreeForPage(WebPage* webPage)
{
    MutexLocker locker(m_scrollingTreesMutex);

    ASSERT(webPage->corePage()->scrollingCoordinator());
    ASSERT(!m_scrollingTrees.contains(webPage->pageID()));
    m_scrollingTrees.set(webPage->pageID(), webPage->corePage()->scrollingCoordinator()->scrollingTree());
}

void EventDispatcher::removeScrollingTreeForPage(WebPage* webPage)
{
    MutexLocker locker(m_scrollingTreesMutex);
    ASSERT(m_scrollingTrees.contains(webPage->pageID()));

    m_scrollingTrees.remove(webPage->pageID());
}
#endif

void EventDispatcher::initializeConnection(CoreIPC::Connection* connection)
{
    connection->addWorkQueueMessageReceiver(Messages::EventDispatcher::messageReceiverName(), m_queue.get(), this);
}

void EventDispatcher::wheelEvent(uint64_t pageID, const WebWheelEvent& wheelEvent, bool canGoBack, bool canGoForward)
{
#if ENABLE(THREADED_SCROLLING)
    MutexLocker locker(m_scrollingTreesMutex);
    if (ScrollingTree* scrollingTree = m_scrollingTrees.get(pageID)) {
        PlatformWheelEvent platformWheelEvent = platform(wheelEvent);

        // FIXME: It's pretty horrible that we're updating the back/forward state here.
        // WebCore should always know the current state and know when it changes so the
        // scrolling tree can be notified.
        // We only need to do this at the beginning of the gesture.
        if (platformWheelEvent.phase() == PlatformWheelEventPhaseBegan)
            ScrollingThread::dispatch(bind(&ScrollingTree::updateBackForwardState, scrollingTree, canGoBack, canGoForward));

        ScrollingTree::EventResult result = scrollingTree->tryToHandleWheelEvent(platformWheelEvent);
        if (result == ScrollingTree::DidHandleEvent || result == ScrollingTree::DidNotHandleEvent) {
            sendDidReceiveEvent(pageID, wheelEvent, result == ScrollingTree::DidHandleEvent);
            return;
        }
    }
#else
    UNUSED_PARAM(canGoBack);
    UNUSED_PARAM(canGoForward);
#endif

    RunLoop::main()->dispatch(bind(&EventDispatcher::dispatchWheelEvent, this, pageID, wheelEvent));
}

#if ENABLE(GESTURE_EVENTS)
void EventDispatcher::gestureEvent(uint64_t pageID, const WebGestureEvent& gestureEvent)
{
    RunLoop::main()->dispatch(bind(&EventDispatcher::dispatchGestureEvent, this, pageID, gestureEvent));
}
#endif

void EventDispatcher::dispatchWheelEvent(uint64_t pageID, const WebWheelEvent& wheelEvent)
{
    ASSERT(isMainThread());

    WebPage* webPage = WebProcess::shared().webPage(pageID);
    if (!webPage)
        return;

    webPage->wheelEvent(wheelEvent);
}

#if ENABLE(GESTURE_EVENTS)
void EventDispatcher::dispatchGestureEvent(uint64_t pageID, const WebGestureEvent& gestureEvent)
{
    ASSERT(isMainThread());

    WebPage* webPage = WebProcess::shared().webPage(pageID);
    if (!webPage)
        return;

    webPage->gestureEvent(gestureEvent);
}
#endif

#if ENABLE(THREADED_SCROLLING)
void EventDispatcher::sendDidReceiveEvent(uint64_t pageID, const WebEvent& event, bool didHandleEvent)
{
    WebProcess::shared().parentProcessConnection()->send(Messages::WebPageProxy::DidReceiveEvent(static_cast<uint32_t>(event.type()), didHandleEvent), pageID);
}
#endif

} // namespace WebKit
