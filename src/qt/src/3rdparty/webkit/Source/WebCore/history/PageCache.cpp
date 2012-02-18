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
#include "DeviceMotionController.h"
#include "DeviceOrientationController.h"
#include "Document.h"
#include "DocumentLoader.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "FrameLoaderStateMachine.h"
#include "HistoryItem.h"
#include "Logging.h"
#include "Page.h"
#include "Settings.h"
#include "SharedWorkerRepository.h"
#include "SystemTime.h"
#include <wtf/CurrentTime.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringConcatenate.h>

using namespace std;

namespace WebCore {

static const double autoreleaseInterval = 3;

#ifndef NDEBUG

static String& pageCacheLogPrefix(int indentLevel)
{
    static int previousIndent = -1;
    DEFINE_STATIC_LOCAL(String, prefix, ());
    
    if (indentLevel != previousIndent) {    
        previousIndent = indentLevel;
        prefix.truncate(0);
        for (int i = 0; i < previousIndent; ++i)
            prefix += "    ";
    }
    
    return prefix;
}

static void pageCacheLog(const String& prefix, const String& message)
{
    LOG(PageCache, "%s%s", prefix.utf8().data(), message.utf8().data());
}
    
#define PCLOG(...) pageCacheLog(pageCacheLogPrefix(indentLevel), makeString(__VA_ARGS__))
    
static bool logCanCacheFrameDecision(Frame* frame, int indentLevel)
{
    // Only bother logging for frames that have actually loaded and have content.
    if (frame->loader()->stateMachine()->creatingInitialEmptyDocument())
        return false;
    KURL currentURL = frame->loader()->documentLoader() ? frame->loader()->documentLoader()->url() : KURL();
    if (currentURL.isEmpty())
        return false;
    
    PCLOG("+---");
    KURL newURL = frame->loader()->provisionalDocumentLoader() ? frame->loader()->provisionalDocumentLoader()->url() : KURL();
    if (!newURL.isEmpty())
        PCLOG(" Determining if frame can be cached navigating from (", currentURL.string(), ") to (", newURL.string(), "):");
    else
        PCLOG(" Determining if subframe with URL (", currentURL.string(), ") can be cached:");
    
    bool cannotCache = false;
    
    do {
        if (!frame->loader()->documentLoader()) {
            PCLOG("   -There is no DocumentLoader object");
            cannotCache = true;
            break;
        }
        if (!frame->loader()->documentLoader()->mainDocumentError().isNull()) {
            PCLOG("   -Main document has an error");
            cannotCache = true;
        }
        if (frame->loader()->subframeLoader()->containsPlugins()) {
            PCLOG("   -Frame contains plugins");
            cannotCache = true;
        }
        if (frame->document()->url().protocolIs("https")) {
            PCLOG("   -Frame is HTTPS");
            cannotCache = true;
        }
        if (frame->domWindow() && frame->domWindow()->hasEventListeners(eventNames().unloadEvent)) {
            PCLOG("   -Frame has an unload event listener");
            cannotCache = true;
        }
#if ENABLE(DATABASE)
        if (frame->document()->hasOpenDatabases()) {
            PCLOG("   -Frame has open database handles");
            cannotCache = true;
        }
#endif
#if ENABLE(SHARED_WORKERS)
        if (SharedWorkerRepository::hasSharedWorkers(frame->document())) {
            PCLOG("   -Frame has associated SharedWorkers");
            cannotCache = true;
        }
#endif
        if (frame->document()->usingGeolocation()) {
            PCLOG("   -Frame uses Geolocation");
            cannotCache = true;
        }
        if (!frame->loader()->history()->currentItem()) {
            PCLOG("   -No current history item");
            cannotCache = true;
        }
        if (frame->loader()->quickRedirectComing()) {
            PCLOG("   -Quick redirect is coming");
            cannotCache = true;
        }
        if (frame->loader()->documentLoader()->isLoadingInAPISense()) {
            PCLOG("   -DocumentLoader is still loading in API sense");
            cannotCache = true;
        }
        if (frame->loader()->documentLoader()->isStopping()) {
            PCLOG("   -DocumentLoader is in the middle of stopping");
            cannotCache = true;
        }
        if (!frame->document()->canSuspendActiveDOMObjects()) {
            PCLOG("   -The document cannot suspect its active DOM Objects");
            cannotCache = true;
        }
#if ENABLE(OFFLINE_WEB_APPLICATIONS)
        if (!frame->loader()->documentLoader()->applicationCacheHost()->canCacheInPageCache()) {
            PCLOG("   -The DocumentLoader uses an application cache");
            cannotCache = true;
        }
#endif
        if (!frame->loader()->client()->canCachePage()) {
            PCLOG("   -The client says this frame cannot be cached");
            cannotCache = true; 
        }
    } while (false);
    
    for (Frame* child = frame->tree()->firstChild(); child; child = child->tree()->nextSibling())
        if (!logCanCacheFrameDecision(child, indentLevel + 1))
            cannotCache = true;
    
    PCLOG(cannotCache ? " Frame CANNOT be cached" : " Frame CAN be cached");
    PCLOG("+---");
    
    return !cannotCache;
}
    
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
    
    bool cannotCache = !logCanCacheFrameDecision(page->mainFrame(), 1);
    
    FrameLoadType loadType = page->mainFrame()->loader()->loadType();
    if (!page->backForward()->isActive()) {
        PCLOG("   -The back/forward list is disabled or has 0 capacity");
        cannotCache = true;
    }
    if (!page->settings()->usesPageCache()) {
        PCLOG("   -Page settings says b/f cache disabled");
        cannotCache = true;
    }
#if ENABLE(DEVICE_ORIENTATION)
    if (page->deviceMotionController() && page->deviceMotionController()->isActive()) {
        PCLOG("   -Page is using DeviceMotion");
        cannotCache = true;
    }
    if (page->deviceOrientationController() && page->deviceOrientationController()->isActive()) {
        PCLOG("   -Page is using DeviceOrientation");
        cannotCache = true;
    }
#endif
    if (loadType == FrameLoadTypeReload) {
        PCLOG("   -Load type is: Reload");
        cannotCache = true;
    }
    if (loadType == FrameLoadTypeReloadFromOrigin) {
        PCLOG("   -Load type is: Reload from origin");
        cannotCache = true;
    }
    if (loadType == FrameLoadTypeSame) {
        PCLOG("   -Load type is: Same");
        cannotCache = true;
    }
    
    PCLOG(cannotCache ? " Page CANNOT be cached\n--------" : " Page CAN be cached\n--------");
}

#endif 

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
    , m_autoreleaseTimer(this, &PageCache::releaseAutoreleasedPagesNowOrReschedule)
{
}
    
bool PageCache::canCachePageContainingThisFrame(Frame* frame)
{
    for (Frame* child = frame->tree()->firstChild(); child; child = child->tree()->nextSibling()) {
        if (!canCachePageContainingThisFrame(child))
            return false;
    }
    
    return frame->loader()->documentLoader()
        && frame->loader()->documentLoader()->mainDocumentError().isNull()
        // Do not cache error pages (these can be recognized as pages with substitute data or unreachable URLs).
        && !(frame->loader()->documentLoader()->substituteData().isValid() && !frame->loader()->documentLoader()->substituteData().failingURL().isEmpty())
        // FIXME: If we ever change this so that frames with plug-ins will be cached,
        // we need to make sure that we don't cache frames that have outstanding NPObjects
        // (objects created by the plug-in). Since there is no way to pause/resume a Netscape plug-in,
        // they would need to be destroyed and then recreated, and there is no way that we can recreate
        // the right NPObjects. See <rdar://problem/5197041> for more information.
        && !frame->loader()->subframeLoader()->containsPlugins()
        && !frame->document()->url().protocolIs("https")
        && (!frame->domWindow() || !frame->domWindow()->hasEventListeners(eventNames().unloadEvent))
#if ENABLE(DATABASE)
        && !frame->document()->hasOpenDatabases()
#endif
#if ENABLE(SHARED_WORKERS)
        && !SharedWorkerRepository::hasSharedWorkers(frame->document())
#endif
        && !frame->document()->usingGeolocation()
        && frame->loader()->history()->currentItem()
        && !frame->loader()->quickRedirectComing()
        && !frame->loader()->documentLoader()->isLoadingInAPISense()
        && !frame->loader()->documentLoader()->isStopping()
        && frame->document()->canSuspendActiveDOMObjects()
#if ENABLE(OFFLINE_WEB_APPLICATIONS)
        // FIXME: We should investigating caching frames that have an associated
        // application cache. <rdar://problem/5917899> tracks that work.
        && frame->loader()->documentLoader()->applicationCacheHost()->canCacheInPageCache()
#endif
        && frame->loader()->client()->canCachePage();
}
    
bool PageCache::canCache(Page* page)
{
    if (!page)
        return false;
    
#ifndef NDEBUG
    logCanCachePageDecision(page);
#endif
    
    // Cache the page, if possible.
    // Don't write to the cache if in the middle of a redirect, since we will want to
    // store the final page we end up on.
    // No point writing to the cache on a reload or loadSame, since we will just write
    // over it again when we leave that page.
    // FIXME: <rdar://problem/4886592> - We should work out the complexities of caching pages with frames as they
    // are the most interesting pages on the web, and often those that would benefit the most from caching!
    FrameLoadType loadType = page->mainFrame()->loader()->loadType();
    
    return canCachePageContainingThisFrame(page->mainFrame())
        && page->backForward()->isActive()
        && page->settings()->usesPageCache()
#if ENABLE(DEVICE_ORIENTATION)
        && !(page->deviceMotionController() && page->deviceMotionController()->isActive())
        && !(page->deviceOrientationController() && page->deviceOrientationController()->isActive())
#endif
        && loadType != FrameLoadTypeReload
        && loadType != FrameLoadTypeReloadFromOrigin
        && loadType != FrameLoadTypeSame;
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

int PageCache::autoreleasedPageCount() const
{
    return m_autoreleaseSet.size();
}

void PageCache::markPagesForVistedLinkStyleRecalc()
{
    for (HistoryItem* current = m_head; current; current = current->m_next)
        current->m_cachedPage->markForVistedLinkStyleRecalc();
}

void PageCache::add(PassRefPtr<HistoryItem> prpItem, Page* page)
{
    ASSERT(prpItem);
    ASSERT(page);
    ASSERT(canCache(page));
    
    HistoryItem* item = prpItem.releaseRef(); // Balanced in remove().

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
        // FIXME: 1800 should not be hardcoded, it should come from
        // WebKitBackForwardCacheExpirationIntervalKey in WebKit.
        // Or we should remove WebKitBackForwardCacheExpirationIntervalKey.
        if (currentTime() - cachedPage->timeStamp() <= 1800)
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

    autorelease(item->m_cachedPage.release());
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

void PageCache::releaseAutoreleasedPagesNowOrReschedule(Timer<PageCache>* timer)
{
    double loadDelta = currentTime() - FrameLoader::timeOfLastCompletedLoad();
    float userDelta = userIdleTime();
    
    // FIXME: <rdar://problem/5211190> This limit of 42 risks growing the page cache far beyond its nominal capacity.
    if ((userDelta < 0.5 || loadDelta < 1.25) && m_autoreleaseSet.size() < 42) {
        LOG(PageCache, "WebCorePageCache: Postponing releaseAutoreleasedPagesNowOrReschedule() - %f since last load, %f since last input, %i objects pending release", loadDelta, userDelta, m_autoreleaseSet.size());
        timer->startOneShot(autoreleaseInterval);
        return;
    }

    LOG(PageCache, "WebCorePageCache: Releasing page caches - %f seconds since last load, %f since last input, %i objects pending release", loadDelta, userDelta, m_autoreleaseSet.size());
    releaseAutoreleasedPagesNow();
}

void PageCache::releaseAutoreleasedPagesNow()
{
    m_autoreleaseTimer.stop();

    // Postpone dead pruning until all our resources have gone dead.
    memoryCache()->setPruneEnabled(false);

    CachedPageSet tmp;
    tmp.swap(m_autoreleaseSet);

    CachedPageSet::iterator end = tmp.end();
    for (CachedPageSet::iterator it = tmp.begin(); it != end; ++it)
        (*it)->destroy();

    // Now do the prune.
    memoryCache()->setPruneEnabled(true);
    memoryCache()->prune();
}

void PageCache::autorelease(PassRefPtr<CachedPage> page)
{
    ASSERT(page);
    ASSERT(!m_autoreleaseSet.contains(page.get()));
    m_autoreleaseSet.add(page);
    if (!m_autoreleaseTimer.isActive())
        m_autoreleaseTimer.startOneShot(autoreleaseInterval);
}

} // namespace WebCore
