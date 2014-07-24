/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
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
#include "PageCache.h"

#include "ApplicationCacheHost.h"
#include "BackForwardController.h"
#include "MemoryCache.h"
#include "CachedPage.h"
#include "DOMWindow.h"
#include "DatabaseManager.h"
#include "DeviceMotionController.h"
#include "DeviceOrientationController.h"
#include "Document.h"
#include "DocumentLoader.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "FrameLoaderStateMachine.h"
#include "FrameView.h"
#include "HistogramSupport.h"
#include "HistoryController.h"
#include "HistoryItem.h"
#include "Logging.h"
#include "Page.h"
#include "Settings.h"
#include "SharedWorkerRepository.h"
#include <wtf/CurrentTime.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringConcatenate.h>

#if ENABLE(PROXIMITY_EVENTS)
#include "DeviceProximityController.h"
#endif

using namespace std;

namespace WebCore {

#if !defined(NDEBUG)

#define PCLOG(...) LOG(PageCache, "%*s%s", indentLevel*4, "", makeString(__VA_ARGS__).utf8().data())
    
// Used in histograms, please only add at the end, and do not remove elements (renaming e.g. to "FooEnumUnused1" is fine).
// This is because statistics may be gathered from histograms between versions over time, and re-using values causes collisions.
enum ReasonFrameCannotBeInPageCache {
    NoDocumentLoader = 0,
    MainDocumentError,
    IsErrorPage,
    HasPlugins,
    IsHttpsAndCacheControlled,
    HasUnloadListener,
    HasDatabaseHandles,
    HasSharedWorkers,
    NoHistoryItem,
    QuickRedirectComing,
    IsLoadingInAPISense,
    IsStopping,
    CannotSuspendActiveDOMObjects,
    DocumentLoaderUsesApplicationCache,
    ClientDeniesCaching,
    NumberOfReasonsFramesCannotBeInPageCache,
};
COMPILE_ASSERT(NumberOfReasonsFramesCannotBeInPageCache <= sizeof(unsigned)*8, ReasonFrameCannotBeInPageCacheDoesNotFitInBitmap);

static unsigned logCanCacheFrameDecision(Frame* frame, int indentLevel)
{
    PCLOG("+---");
    if (!frame->loader()->documentLoader()) {
        PCLOG("   -There is no DocumentLoader object");
        return 1 << NoDocumentLoader;
    }

    KURL currentURL = frame->loader()->documentLoader()->url();
    KURL newURL = frame->loader()->provisionalDocumentLoader() ? frame->loader()->provisionalDocumentLoader()->url() : KURL();
    if (!newURL.isEmpty())
        PCLOG(" Determining if frame can be cached navigating from (", currentURL.string(), ") to (", newURL.string(), "):");
    else
        PCLOG(" Determining if subframe with URL (", currentURL.string(), ") can be cached:");
     
    unsigned rejectReasons = 0;
    if (!frame->loader()->documentLoader()->mainDocumentError().isNull()) {
        PCLOG("   -Main document has an error");
        rejectReasons |= 1 << MainDocumentError;
    }
    if (frame->loader()->documentLoader()->substituteData().isValid() && frame->loader()->documentLoader()->substituteData().failingURL().isEmpty()) {
        PCLOG("   -Frame is an error page");
        rejectReasons |= 1 << IsErrorPage;
    }
    if (frame->loader()->subframeLoader()->containsPlugins() && !frame->page()->settings()->pageCacheSupportsPlugins()) {
        PCLOG("   -Frame contains plugins");
        rejectReasons |= 1 << HasPlugins;
    }
    if (frame->document()->url().protocolIs("https")
        && (frame->loader()->documentLoader()->response().cacheControlContainsNoCache()
            || frame->loader()->documentLoader()->response().cacheControlContainsNoStore())) {
        PCLOG("   -Frame is HTTPS, and cache control prohibits caching or storing");
        rejectReasons |= 1 << IsHttpsAndCacheControlled;
    }
    if (frame->document()->domWindow() && frame->document()->domWindow()->hasEventListeners(eventNames().unloadEvent)) {
        PCLOG("   -Frame has an unload event listener");
        rejectReasons |= 1 << HasUnloadListener;
    }
#if ENABLE(SQL_DATABASE)
    if (DatabaseManager::manager().hasOpenDatabases(frame->document())) {
        PCLOG("   -Frame has open database handles");
        rejectReasons |= 1 << HasDatabaseHandles;
    }
#endif
#if ENABLE(SHARED_WORKERS)
    if (SharedWorkerRepository::hasSharedWorkers(frame->document())) {
        PCLOG("   -Frame has associated SharedWorkers");
        rejectReasons |= 1 << HasSharedWorkers;
    }
#endif
    if (!frame->loader()->history()->currentItem()) {
        PCLOG("   -No current history item");
        rejectReasons |= 1 << NoHistoryItem;
    }
    if (frame->loader()->quickRedirectComing()) {
        PCLOG("   -Quick redirect is coming");
        rejectReasons |= 1 << QuickRedirectComing;
    }
    if (frame->loader()->documentLoader()->isLoadingInAPISense()) {
        PCLOG("   -DocumentLoader is still loading in API sense");
        rejectReasons |= 1 << IsLoadingInAPISense;
    }
    if (frame->loader()->documentLoader()->isStopping()) {
        PCLOG("   -DocumentLoader is in the middle of stopping");
        rejectReasons |= 1 << IsStopping;
    }
    if (!frame->document()->canSuspendActiveDOMObjects()) {
        PCLOG("   -The document cannot suspect its active DOM Objects");
        rejectReasons |= 1 << CannotSuspendActiveDOMObjects;
    }
    if (!frame->loader()->documentLoader()->applicationCacheHost()->canCacheInPageCache()) {
        PCLOG("   -The DocumentLoader uses an application cache");
        rejectReasons |= 1 << DocumentLoaderUsesApplicationCache;
    }
    if (!frame->loader()->client()->canCachePage()) {
        PCLOG("   -The client says this frame cannot be cached");
        rejectReasons |= 1 << ClientDeniesCaching;
    }

    HistogramSupport::histogramEnumeration("PageCache.FrameCacheable", !rejectReasons, 2);
    int reasonCount = 0;
    for (int i = 0; i < NumberOfReasonsFramesCannotBeInPageCache; ++i) {
        if (rejectReasons & (1 << i)) {
            ++reasonCount;
            HistogramSupport::histogramEnumeration("PageCache.FrameRejectReason", i, NumberOfReasonsFramesCannotBeInPageCache);
        }
    }
    HistogramSupport::histogramEnumeration("PageCache.FrameRejectReasonCount", reasonCount, 1 + NumberOfReasonsFramesCannotBeInPageCache);

    for (Frame* child = frame->tree()->firstChild(); child; child = child->tree()->nextSibling())
        rejectReasons |= logCanCacheFrameDecision(child, indentLevel + 1);
    
    PCLOG(rejectReasons ? " Frame CANNOT be cached" : " Frame CAN be cached");
    PCLOG("+---");
    
    return rejectReasons;
}

// Used in histograms, please only add at the end, and do not remove elements (renaming e.g. to "FooEnumUnused1" is fine).
// This is because statistics may be gathered from histograms between versions over time, and re-using values causes collisions.
enum ReasonPageCannotBeInPageCache {
    FrameCannotBeInPageCache = 0,
    DisabledBackForwardList,
    DisabledPageCache,
    UsesDeviceMotion,
    UsesDeviceOrientation,
    IsReload,
    IsReloadFromOrigin,
    IsSameLoad,
    NumberOfReasonsPagesCannotBeInPageCache,
};
COMPILE_ASSERT(NumberOfReasonsPagesCannotBeInPageCache <= sizeof(unsigned)*8, ReasonPageCannotBeInPageCacheDoesNotFitInBitmap);

static void logCanCachePageDecision(Page* page)
{
    // Only bother logging for main frames that have actually loaded and have content.
    if (page->mainFrame()->loader()->stateMachine()->creatingInitialEmptyDocument())
        return;
    KURL currentURL = page->mainFrame()->loader()->documentLoader() ? page->mainFrame()->loader()->documentLoader()->url() : KURL();
    if (currentURL.isEmpty())
        return;
    
    int indentLevel = 0;    
    PCLOG("--------\n Determining if page can be cached:");
    
    unsigned rejectReasons = 0;
    unsigned frameRejectReasons = logCanCacheFrameDecision(page->mainFrame(), indentLevel+1);
    if (frameRejectReasons)
        rejectReasons |= 1 << FrameCannotBeInPageCache;
    
    if (!page->backForward()->isActive()) {
        PCLOG("   -The back/forward list is disabled or has 0 capacity");
        rejectReasons |= 1 << DisabledBackForwardList;
    }
    if (!page->settings()->usesPageCache()) {
        PCLOG("   -Page settings says b/f cache disabled");
        rejectReasons |= 1 << DisabledPageCache;
    }
#if ENABLE(DEVICE_ORIENTATION)
    if (DeviceMotionController::isActiveAt(page)) {
        PCLOG("   -Page is using DeviceMotion");
        rejectReasons |= 1 << UsesDeviceMotion;
    }
    if (DeviceOrientationController::isActiveAt(page)) {
        PCLOG("   -Page is using DeviceOrientation");
        rejectReasons |= 1 << UsesDeviceOrientation;
    }
#endif
#if ENABLE(PROXIMITY_EVENTS)
    if (DeviceProximityController::isActiveAt(page)) {
        PCLOG("   -Page is using DeviceProximity");
        rejectReasons |= 1 << UsesDeviceMotion;
    }
#endif
    FrameLoadType loadType = page->mainFrame()->loader()->loadType();
    if (loadType == FrameLoadTypeReload) {
        PCLOG("   -Load type is: Reload");
        rejectReasons |= 1 << IsReload;
    }
    if (loadType == FrameLoadTypeReloadFromOrigin) {
        PCLOG("   -Load type is: Reload from origin");
        rejectReasons |= 1 << IsReloadFromOrigin;
    }
    if (loadType == FrameLoadTypeSame) {
        PCLOG("   -Load type is: Same");
        rejectReasons |= 1 << IsSameLoad;
    }
    
    PCLOG(rejectReasons ? " Page CANNOT be cached\n--------" : " Page CAN be cached\n--------");

    HistogramSupport::histogramEnumeration("PageCache.PageCacheable", !rejectReasons, 2);
    int reasonCount = 0;
    for (int i = 0; i < NumberOfReasonsPagesCannotBeInPageCache; ++i) {
        if (rejectReasons & (1 << i)) {
            ++reasonCount;
            HistogramSupport::histogramEnumeration("PageCache.PageRejectReason", i, NumberOfReasonsPagesCannotBeInPageCache);
        }
    }
    HistogramSupport::histogramEnumeration("PageCache.PageRejectReasonCount", reasonCount, 1 + NumberOfReasonsPagesCannotBeInPageCache);
    const bool settingsDisabledPageCache = rejectReasons & (1 << DisabledPageCache);
    HistogramSupport::histogramEnumeration("PageCache.PageRejectReasonCountExcludingSettings", reasonCount - settingsDisabledPageCache, NumberOfReasonsPagesCannotBeInPageCache);

    // Report also on the frame reasons by page; this is distinct from the per frame statistics since it coalesces the
    // causes from all subframes together.
    HistogramSupport::histogramEnumeration("PageCache.FrameCacheableByPage", !frameRejectReasons, 2);
    int frameReasonCount = 0;
    for (int i = 0; i <= NumberOfReasonsFramesCannotBeInPageCache; ++i) {
        if (frameRejectReasons & (1 << i)) {
            ++frameReasonCount;
            HistogramSupport::histogramEnumeration("PageCache.FrameRejectReasonByPage", i, NumberOfReasonsFramesCannotBeInPageCache);
        }
    }

    HistogramSupport::histogramEnumeration("PageCache.FrameRejectReasonCountByPage", frameReasonCount, 1 + NumberOfReasonsFramesCannotBeInPageCache);
}

#endif // !defined(NDEBUG)

PageCache* pageCache()
{
    static PageCache* staticPageCache = new PageCache;
    return staticPageCache;
}

PageCache::PageCache()
    : m_capacity(0)
    , m_size(0)
    , m_head(0)
    , m_tail(0)
#if USE(ACCELERATED_COMPOSITING)
    , m_shouldClearBackingStores(false)
#endif
{
}
    
bool PageCache::canCachePageContainingThisFrame(Frame* frame)
{
    for (Frame* child = frame->tree()->firstChild(); child; child = child->tree()->nextSibling()) {
        if (!canCachePageContainingThisFrame(child))
            return false;
    }
    
    FrameLoader* frameLoader = frame->loader();
    DocumentLoader* documentLoader = frameLoader->documentLoader();
    Document* document = frame->document();
    
    return documentLoader
        && documentLoader->mainDocumentError().isNull()
        // Do not cache error pages (these can be recognized as pages with substitute data or unreachable URLs).
        && !(documentLoader->substituteData().isValid() && !documentLoader->substituteData().failingURL().isEmpty())
        && (!frameLoader->subframeLoader()->containsPlugins() || frame->page()->settings()->pageCacheSupportsPlugins())
        && (!document->url().protocolIs("https") || (!documentLoader->response().cacheControlContainsNoCache() && !documentLoader->response().cacheControlContainsNoStore()))
        && (!document->domWindow() || !document->domWindow()->hasEventListeners(eventNames().unloadEvent))
#if ENABLE(SQL_DATABASE)
        && !DatabaseManager::manager().hasOpenDatabases(document)
#endif
#if ENABLE(SHARED_WORKERS)
        && !SharedWorkerRepository::hasSharedWorkers(document)
#endif
        && frameLoader->history()->currentItem()
        && !frameLoader->quickRedirectComing()
        && !documentLoader->isLoadingInAPISense()
        && !documentLoader->isStopping()
        && document->canSuspendActiveDOMObjects()
        // FIXME: We should investigating caching frames that have an associated
        // application cache. <rdar://problem/5917899> tracks that work.
        && documentLoader->applicationCacheHost()->canCacheInPageCache()
        && frameLoader->client()->canCachePage();
}
    
bool PageCache::canCache(Page* page) const
{
    if (!page)
        return false;
    
#if !defined(NDEBUG)
    logCanCachePageDecision(page);
#endif
    
    // Cache the page, if possible.
    // Don't write to the cache if in the middle of a redirect, since we will want to
    // store the final page we end up on.
    // No point writing to the cache on a reload or loadSame, since we will just write
    // over it again when we leave that page.
    FrameLoadType loadType = page->mainFrame()->loader()->loadType();
    
    return m_capacity > 0
        && canCachePageContainingThisFrame(page->mainFrame())
        && page->backForward()->isActive()
        && page->settings()->usesPageCache()
#if ENABLE(DEVICE_ORIENTATION)
        && !DeviceMotionController::isActiveAt(page)
        && !DeviceOrientationController::isActiveAt(page)
#endif
#if ENABLE(PROXIMITY_EVENTS)
        && !DeviceProximityController::isActiveAt(page)
#endif
        && (loadType == FrameLoadTypeStandard
            || loadType == FrameLoadTypeBack
            || loadType == FrameLoadTypeForward
            || loadType == FrameLoadTypeIndexedBackForward);
}

void PageCache::setCapacity(int capacity)
{
    ASSERT(capacity >= 0);
    m_capacity = max(capacity, 0);

    prune();
}

int PageCache::frameCount() const
{
    int frameCount = 0;
    for (HistoryItem* current = m_head; current; current = current->m_next) {
        ++frameCount;
        ASSERT(current->m_cachedPage);
        frameCount += current->m_cachedPage ? current->m_cachedPage->cachedMainFrame()->descendantFrameCount() : 0;
    }
    
    return frameCount;
}

void PageCache::markPagesForVistedLinkStyleRecalc()
{
    for (HistoryItem* current = m_head; current; current = current->m_next)
        current->m_cachedPage->markForVistedLinkStyleRecalc();
}

void PageCache::markPagesForFullStyleRecalc(Page* page)
{
    Frame* mainFrame = page->mainFrame();

    for (HistoryItem* current = m_head; current; current = current->m_next) {
        CachedPage* cachedPage = current->m_cachedPage.get();
        if (cachedPage->cachedMainFrame()->view()->frame() == mainFrame)
            cachedPage->markForFullStyleRecalc();
    }
}


#if USE(ACCELERATED_COMPOSITING)
void PageCache::markPagesForDeviceScaleChanged(Page* page)
{
    Frame* mainFrame = page->mainFrame();

    for (HistoryItem* current = m_head; current; current = current->m_next) {
        CachedPage* cachedPage = current->m_cachedPage.get();
        if (cachedPage->cachedMainFrame()->view()->frame() == mainFrame)
            cachedPage->markForDeviceScaleChanged();
    }
}
#endif

#if ENABLE(VIDEO_TRACK)
void PageCache::markPagesForCaptionPreferencesChanged()
{
    for (HistoryItem* current = m_head; current; current = current->m_next)
        current->m_cachedPage->markForCaptionPreferencesChanged();
}
#endif

void PageCache::add(PassRefPtr<HistoryItem> prpItem, Page* page)
{
    ASSERT(prpItem);
    ASSERT(page);
    ASSERT(canCache(page));
    
    HistoryItem* item = prpItem.leakRef(); // Balanced in remove().

    // Remove stale cache entry if necessary.
    if (item->m_cachedPage)
        remove(item);

    item->m_cachedPage = CachedPage::create(page);
    addToLRUList(item);
    ++m_size;
    
    prune();
}

CachedPage* PageCache::get(HistoryItem* item)
{
    if (!item)
        return 0;

    if (CachedPage* cachedPage = item->m_cachedPage.get()) {
        if (!cachedPage->hasExpired())
            return cachedPage;
        
        LOG(PageCache, "Not restoring page for %s from back/forward cache because cache entry has expired", item->url().string().ascii().data());
        pageCache()->remove(item);
    }
    return 0;
}

void PageCache::remove(HistoryItem* item)
{
    // Safely ignore attempts to remove items not in the cache.
    if (!item || !item->m_cachedPage)
        return;

    item->m_cachedPage.clear();
    removeFromLRUList(item);
    --m_size;

    item->deref(); // Balanced in add().
}

void PageCache::prune()
{
    while (m_size > m_capacity) {
        ASSERT(m_tail && m_tail->m_cachedPage);
        remove(m_tail);
    }
}

void PageCache::addToLRUList(HistoryItem* item)
{
    item->m_next = m_head;
    item->m_prev = 0;

    if (m_head) {
        ASSERT(m_tail);
        m_head->m_prev = item;
    } else {
        ASSERT(!m_tail);
        m_tail = item;
    }

    m_head = item;
}

void PageCache::removeFromLRUList(HistoryItem* item)
{
    if (!item->m_next) {
        ASSERT(item == m_tail);
        m_tail = item->m_prev;
    } else {
        ASSERT(item != m_tail);
        item->m_next->m_prev = item->m_prev;
    }

    if (!item->m_prev) {
        ASSERT(item == m_head);
        m_head = item->m_next;
    } else {
        ASSERT(item != m_head);
        item->m_prev->m_next = item->m_next;
    }
}

} // namespace WebCore
