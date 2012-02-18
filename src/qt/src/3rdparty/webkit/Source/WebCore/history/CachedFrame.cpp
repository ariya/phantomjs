/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */
 
#include "config.h"
#include "CachedPage.h"

#include "CachedFramePlatformData.h"
#include "DocumentLoader.h"
#include "ExceptionCode.h"
#include "EventNames.h"
#include "FocusController.h"
#include "Frame.h"
#include "FrameLoaderClient.h"
#include "FrameView.h"
#include "HistoryItem.h"
#include "Logging.h"
#include "PageTransitionEvent.h"
#include <wtf/text/CString.h>
#include <wtf/RefCountedLeakCounter.h>

#if ENABLE(SVG)
#include "SVGDocumentExtensions.h"
#endif

#if ENABLE(TOUCH_EVENTS)
#include "Chrome.h"
#include "ChromeClient.h"
#include "Page.h"
#endif

namespace WebCore {

#ifndef NDEBUG
static WTF::RefCountedLeakCounter& cachedFrameCounter()
{
    DEFINE_STATIC_LOCAL(WTF::RefCountedLeakCounter, counter, ("CachedFrame"));
    return counter;
}
#endif

CachedFrameBase::CachedFrameBase(Frame* frame)
    : m_document(frame->document())
    , m_documentLoader(frame->loader()->documentLoader())
    , m_view(frame->view())
    , m_mousePressNode(frame->eventHandler()->mousePressNode())
    , m_url(frame->document()->url())
    , m_isMainFrame(!frame->tree()->parent())
{
}

CachedFrameBase::~CachedFrameBase()
{
#ifndef NDEBUG
    cachedFrameCounter().decrement();
#endif
    // CachedFrames should always have had destroy() called by their parent CachedPage
    ASSERT(!m_document);
}

void CachedFrameBase::restore()
{
    ASSERT(m_document->view() == m_view);

    if (m_isMainFrame)
        m_view->setParentVisible(true);

    Frame* frame = m_view->frame();
    m_cachedFrameScriptData->restore(frame);

#if ENABLE(SVG)
    if (m_document->svgExtensions())
        m_document->accessSVGExtensions()->unpauseAnimations();
#endif

    frame->animation()->resumeAnimationsForDocument(m_document.get());
    frame->eventHandler()->setMousePressNode(m_mousePressNode.get());
    m_document->resumeActiveDOMObjects();
    m_document->resumeScriptedAnimationControllerCallbacks();

    // It is necessary to update any platform script objects after restoring the
    // cached page.
    frame->script()->updatePlatformScriptObjects();

    frame->loader()->client()->didRestoreFromPageCache();

    // Reconstruct the FrameTree
    for (unsigned i = 0; i < m_childFrames.size(); ++i)
        frame->tree()->appendChild(m_childFrames[i]->view()->frame());

    // Open the child CachedFrames in their respective FrameLoaders.
    for (unsigned i = 0; i < m_childFrames.size(); ++i)
        m_childFrames[i]->open();

    m_document->enqueuePageshowEvent(PageshowEventPersisted);
    
    HistoryItem* historyItem = frame->loader()->history()->currentItem();
    m_document->enqueuePopstateEvent(historyItem && historyItem->stateObject() ? historyItem->stateObject() : SerializedScriptValue::nullValue());
    
#if ENABLE(TOUCH_EVENTS)
    if (m_document->hasListenerType(Document::TOUCH_LISTENER))
        m_document->page()->chrome()->client()->needTouchEvents(true);
#endif

    m_document->documentDidBecomeActive();
}

CachedFrame::CachedFrame(Frame* frame)
    : CachedFrameBase(frame)
{
#ifndef NDEBUG
    cachedFrameCounter().increment();
#endif
    ASSERT(m_document);
    ASSERT(m_documentLoader);
    ASSERT(m_view);

    if (frame->page()->focusController()->focusedFrame() == frame)
        frame->page()->focusController()->setFocusedFrame(frame->page()->mainFrame());

    // Custom scrollbar renderers will get reattached when the document comes out of the page cache
    m_view->detachCustomScrollbars();

    m_document->documentWillBecomeInactive();
    frame->clearTimers();
    m_document->setInPageCache(true);
    frame->loader()->stopLoading(UnloadEventPolicyUnloadAndPageHide);

    // Create the CachedFrames for all Frames in the FrameTree.
    for (Frame* child = frame->tree()->firstChild(); child; child = child->tree()->nextSibling())
        m_childFrames.append(CachedFrame::create(child));

    // Active DOM objects must be suspended before we cache the frame script data,
    // but after we've fired the pagehide event, in case that creates more objects.
    // Suspending must also happen after we've recursed over child frames, in case
    // those create more objects.
    // FIXME: It's still possible to have objects created after suspending in some cases, see http://webkit.org/b/53733 for more details.
    m_document->suspendScriptedAnimationControllerCallbacks();
    m_document->suspendActiveDOMObjects(ActiveDOMObject::DocumentWillBecomeInactive);
    m_cachedFrameScriptData = adoptPtr(new ScriptCachedFrameData(frame));

    frame->loader()->client()->savePlatformDataToCachedFrame(this);

    // Deconstruct the FrameTree, to restore it later.
    // We do this for two reasons:
    // 1 - We reuse the main frame, so when it navigates to a new page load it needs to start with a blank FrameTree.
    // 2 - It's much easier to destroy a CachedFrame while it resides in the PageCache if it is disconnected from its parent.
    for (unsigned i = 0; i < m_childFrames.size(); ++i)
        frame->tree()->removeChild(m_childFrames[i]->view()->frame());

    if (!m_isMainFrame)
        frame->page()->decrementFrameCount();

    frame->loader()->client()->didSaveToPageCache();

#ifndef NDEBUG
    if (m_isMainFrame)
        LOG(PageCache, "Finished creating CachedFrame for main frame url '%s' and DocumentLoader %p\n", m_url.string().utf8().data(), m_documentLoader.get());
    else
        LOG(PageCache, "Finished creating CachedFrame for child frame with url '%s' and DocumentLoader %p\n", m_url.string().utf8().data(), m_documentLoader.get());
#endif

#if ENABLE(TOUCH_EVENTS)
    if (m_document->hasListenerType(Document::TOUCH_LISTENER))
        m_document->page()->chrome()->client()->needTouchEvents(false);
#endif
}

void CachedFrame::open()
{
    ASSERT(m_view);
    m_view->frame()->loader()->open(*this);

    if (!m_isMainFrame)
        m_view->frame()->page()->incrementFrameCount();
}

void CachedFrame::clear()
{
    if (!m_document)
        return;

    // clear() should only be called for Frames representing documents that are no longer in the page cache.
    // This means the CachedFrame has been:
    // 1 - Successfully restore()'d by going back/forward.
    // 2 - destroy()'ed because the PageCache is pruning or the WebView was closed.
    ASSERT(!m_document->inPageCache());
    ASSERT(m_view);
    ASSERT(m_document->frame() == m_view->frame());

    for (int i = m_childFrames.size() - 1; i >= 0; --i)
        m_childFrames[i]->clear();

    m_document = 0;
    m_view = 0;
    m_mousePressNode = 0;
    m_url = KURL();

    m_cachedFramePlatformData.clear();
    m_cachedFrameScriptData.clear();
}

void CachedFrame::destroy()
{
    if (!m_document)
        return;
    
    // Only CachedFrames that are still in the PageCache should be destroyed in this manner
    ASSERT(m_document->inPageCache());
    ASSERT(m_view);
    ASSERT(m_document->frame() == m_view->frame());

    if (!m_isMainFrame) {
        m_view->frame()->detachFromPage();
        m_view->frame()->loader()->detachViewsAndDocumentLoader();
    }
    
    for (int i = m_childFrames.size() - 1; i >= 0; --i)
        m_childFrames[i]->destroy();

    if (m_cachedFramePlatformData)
        m_cachedFramePlatformData->clear();

    Frame::clearTimers(m_view.get(), m_document.get());

    // FIXME: Why do we need to call removeAllEventListeners here? When the document is in page cache, this method won't work
    // fully anyway, because the document won't be able to access its DOMWindow object (due to being frameless).
    m_document->removeAllEventListeners();

    m_document->setInPageCache(false);
    // FIXME: We don't call willRemove here. Why is that OK?
    m_document->detach();
    m_view->clearFrame();

    clear();
}

void CachedFrame::setCachedFramePlatformData(PassOwnPtr<CachedFramePlatformData> data)
{
    m_cachedFramePlatformData = data;
}

CachedFramePlatformData* CachedFrame::cachedFramePlatformData()
{
    return m_cachedFramePlatformData.get();
}

int CachedFrame::descendantFrameCount() const
{
    int count = m_childFrames.size();
    for (size_t i = 0; i < m_childFrames.size(); ++i)
        count += m_childFrames[i]->descendantFrameCount();
    
    return count;
}

} // namespace WebCore
