/*
    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller (mueller@kde.org)
    Copyright (C) 2002 Waldo Bastian (bastian@kde.org)
    Copyright (C) 2004, 2005, 2006, 2008 Apple Inc. All rights reserved.
    Copyright (C) 2009 Torch Mobile Inc. http://www.torchmobile.com/

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

    This class provides all functionality needed for loading images, style sheets and html
    pages from the web. It has a memory cache for these objects.
*/

#include "config.h"
#include "CachedResourceLoader.h"

#include "CachedCSSStyleSheet.h"
#include "CachedFont.h"
#include "CachedImage.h"
#include "CachedResourceRequest.h"
#include "CachedScript.h"
#include "CachedXSLStyleSheet.h"
#include "Console.h"
#include "ContentSecurityPolicy.h"
#include "DOMWindow.h"
#include "Document.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "HTMLElement.h"
#include "Logging.h"
#include "MemoryCache.h"
#include "PingLoader.h"
#include "ResourceLoadScheduler.h"
#include "SecurityOrigin.h"
#include "Settings.h"
#include <wtf/UnusedParam.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringConcatenate.h>

#define PRELOAD_DEBUG 0

namespace WebCore {

static CachedResource* createResource(CachedResource::Type type, const KURL& url, const String& charset)
{
    switch (type) {
    case CachedResource::ImageResource:
        return new CachedImage(url.string());
    case CachedResource::CSSStyleSheet:
        return new CachedCSSStyleSheet(url.string(), charset);
    case CachedResource::Script:
        return new CachedScript(url.string(), charset);
    case CachedResource::FontResource:
        return new CachedFont(url.string());
#if ENABLE(XSLT)
    case CachedResource::XSLStyleSheet:
        return new CachedXSLStyleSheet(url.string());
#endif
#if ENABLE(LINK_PREFETCH)
    case CachedResource::LinkResource:
        return new CachedResource(url.string(), CachedResource::LinkResource);
#endif
    }
    ASSERT_NOT_REACHED();
    return 0;
}

CachedResourceLoader::CachedResourceLoader(Document* document)
    : m_document(document)
    , m_requestCount(0)
    , m_loadDoneActionTimer(this, &CachedResourceLoader::loadDoneActionTimerFired)
    , m_autoLoadImages(true)
    , m_loadFinishing(false)
    , m_allowStaleResources(false)
{
}

CachedResourceLoader::~CachedResourceLoader()
{
    m_document = 0;

    cancelRequests();
    clearPreloads();
    DocumentResourceMap::iterator end = m_documentResources.end();
    for (DocumentResourceMap::iterator it = m_documentResources.begin(); it != end; ++it)
        it->second->setOwningCachedResourceLoader(0);

    // Make sure no requests still point to this CachedResourceLoader
    ASSERT(m_requestCount == 0);
}

CachedResource* CachedResourceLoader::cachedResource(const String& resourceURL) const 
{
    KURL url = m_document->completeURL(resourceURL);
    return cachedResource(url); 
}

CachedResource* CachedResourceLoader::cachedResource(const KURL& resourceURL) const
{
    KURL url = MemoryCache::removeFragmentIdentifierIfNeeded(resourceURL);
    return m_documentResources.get(url).get(); 
}

Frame* CachedResourceLoader::frame() const
{
    return m_document ? m_document->frame() : 0;
}

CachedImage* CachedResourceLoader::requestImage(const String& url)
{
    if (Frame* f = frame()) {
        Settings* settings = f->settings();
        if (!f->loader()->client()->allowImages(!settings || settings->areImagesEnabled()))
            return 0;

        if (f->loader()->pageDismissalEventBeingDispatched()) {
            KURL completeURL = m_document->completeURL(url);
            if (completeURL.isValid() && canRequest(CachedResource::ImageResource, completeURL))
                PingLoader::loadImage(f, completeURL);
            return 0;
        }
    }
    CachedImage* resource = static_cast<CachedImage*>(requestResource(CachedResource::ImageResource, url, String()));
    if (autoLoadImages() && resource && resource->stillNeedsLoad()) {
        resource->setLoading(true);
        load(resource, true);
    }
    return resource;
}

CachedFont* CachedResourceLoader::requestFont(const String& url)
{
    return static_cast<CachedFont*>(requestResource(CachedResource::FontResource, url, String()));
}

CachedCSSStyleSheet* CachedResourceLoader::requestCSSStyleSheet(const String& url, const String& charset, ResourceLoadPriority priority)
{
    return static_cast<CachedCSSStyleSheet*>(requestResource(CachedResource::CSSStyleSheet, url, charset, priority));
}

CachedCSSStyleSheet* CachedResourceLoader::requestUserCSSStyleSheet(const String& requestURL, const String& charset)
{
    KURL url = MemoryCache::removeFragmentIdentifierIfNeeded(KURL(KURL(), requestURL));

    if (CachedResource* existing = memoryCache()->resourceForURL(url)) {
        if (existing->type() == CachedResource::CSSStyleSheet)
            return static_cast<CachedCSSStyleSheet*>(existing);
        memoryCache()->remove(existing);
    }
    CachedCSSStyleSheet* userSheet = new CachedCSSStyleSheet(url, charset);
    
    bool inCache = memoryCache()->add(userSheet);
    if (!inCache)
        userSheet->setInCache(true);
    
    userSheet->load(this, /*incremental*/ false, SkipSecurityCheck, /*sendResourceLoadCallbacks*/ false);

    if (!inCache)
        userSheet->setInCache(false);
    
    return userSheet;
}

CachedScript* CachedResourceLoader::requestScript(const String& url, const String& charset)
{
    return static_cast<CachedScript*>(requestResource(CachedResource::Script, url, charset));
}

#if ENABLE(XSLT)
CachedXSLStyleSheet* CachedResourceLoader::requestXSLStyleSheet(const String& url)
{
    return static_cast<CachedXSLStyleSheet*>(requestResource(CachedResource::XSLStyleSheet, url, String()));
}
#endif

#if ENABLE(LINK_PREFETCH)
CachedResource* CachedResourceLoader::requestLinkResource(const String& url, ResourceLoadPriority priority)
{
    ASSERT(frame());
    return requestResource(CachedResource::LinkResource, url, String(), priority);
}
#endif

bool CachedResourceLoader::canRequest(CachedResource::Type type, const KURL& url, bool forPreload)
{
    if (!document()->securityOrigin()->canDisplay(url)) {
        if (!forPreload)
            FrameLoader::reportLocalLoadFailed(document()->frame(), url.string());
        LOG(ResourceLoading, "CachedResourceLoader::requestResource URL was not allowed by SecurityOrigin::canDisplay");
        return 0;
    }

    // Some types of resources can be loaded only from the same origin.  Other
    // types of resources, like Images, Scripts, and CSS, can be loaded from
    // any URL.
    switch (type) {
    case CachedResource::ImageResource:
    case CachedResource::CSSStyleSheet:
    case CachedResource::Script:
    case CachedResource::FontResource:
#if ENABLE(LINK_PREFETCH)
    case CachedResource::LinkResource:
#endif
        // These types of resources can be loaded from any origin.
        // FIXME: Are we sure about CachedResource::FontResource?
        break;
#if ENABLE(XSLT)
    case CachedResource::XSLStyleSheet:
        if (!m_document->securityOrigin()->canRequest(url)) {
            printAccessDeniedMessage(url);
            return false;
        }
        break;
#endif
    }

    // Given that the load is allowed by the same-origin policy, we should
    // check whether the load passes the mixed-content policy.
    //
    // Note: Currently, we always allow mixed content, but we generate a
    //       callback to the FrameLoaderClient in case the embedder wants to
    //       update any security indicators.
    // 
    // FIXME: Should we consider forPreload here?
    //
    switch (type) {
    case CachedResource::Script:
#if ENABLE(XSLT)
    case CachedResource::XSLStyleSheet:
#endif
    case CachedResource::CSSStyleSheet:
        // These resource can inject script into the current document (Script,
        // XSL) or exfiltrate the content of the current document (CSS).
        if (Frame* f = frame())
            if (!f->loader()->checkIfRunInsecureContent(m_document->securityOrigin(), url))
                return false;
        break;
    case CachedResource::ImageResource:
    case CachedResource::FontResource: {
        // These resources can corrupt only the frame's pixels.
        if (Frame* f = frame()) {
            Frame* top = f->tree()->top();
            if (!top->loader()->checkIfDisplayInsecureContent(top->document()->securityOrigin(), url))
                return false;
        }
        break;
    }
#if ENABLE(LINK_PREFETCH)
    case CachedResource::LinkResource:
        // Prefetch cannot affect the current document.
        break;
#endif
    }
    // FIXME: Consider letting the embedder block mixed content loads.

    switch (type) {
    case CachedResource::Script:
        if (!m_document->contentSecurityPolicy()->allowScriptFromSource(url))
            return false;
        break;
#if ENABLE(XSLT)
    case CachedResource::XSLStyleSheet:
#endif
    case CachedResource::CSSStyleSheet:
        if (!m_document->contentSecurityPolicy()->allowStyleFromSource(url))
            return false;
        break;
    case CachedResource::ImageResource:
        if (!m_document->contentSecurityPolicy()->allowImageFromSource(url))
            return false;
        break;
    case CachedResource::FontResource: {
        if (!m_document->contentSecurityPolicy()->allowFontFromSource(url))
            return false;
        break;
    }
#if ENABLE(LINK_PREFETCH)
    case CachedResource::LinkResource:
        break;
#endif
    }

    return true;
}

CachedResource* CachedResourceLoader::requestResource(CachedResource::Type type, const String& resourceURL, const String& charset, ResourceLoadPriority priority, bool forPreload)
{
    KURL url = m_document->completeURL(resourceURL);
    
    LOG(ResourceLoading, "CachedResourceLoader::requestResource '%s', charset '%s', priority=%d, forPreload=%u", url.string().latin1().data(), charset.latin1().data(), priority, forPreload);
    
    // If only the fragment identifiers differ, it is the same resource.
    url = MemoryCache::removeFragmentIdentifierIfNeeded(url);

    if (!url.isValid())
        return 0;

    if (!canRequest(type, url, forPreload))
        return 0;

    if (memoryCache()->disabled()) {
        DocumentResourceMap::iterator it = m_documentResources.find(url.string());
        if (it != m_documentResources.end()) {
            it->second->setOwningCachedResourceLoader(0);
            m_documentResources.remove(it);
        }
    }

    // See if we can use an existing resource from the cache.
    CachedResource* resource = memoryCache()->resourceForURL(url);

    switch (determineRevalidationPolicy(type, forPreload, resource)) {
    case Load:
        resource = loadResource(type, url, charset, priority);
        break;
    case Reload:
        memoryCache()->remove(resource);
        resource = loadResource(type, url, charset, priority);
        break;
    case Revalidate:
        resource = revalidateResource(resource, priority);
        break;
    case Use:
        memoryCache()->resourceAccessed(resource);
        notifyLoadedFromMemoryCache(resource);
        break;
    }

    if (!resource)
        return 0;

    ASSERT(resource->url() == url.string());
    m_documentResources.set(resource->url(), resource);
    
    return resource;
}
    
CachedResource* CachedResourceLoader::revalidateResource(CachedResource* resource, ResourceLoadPriority priority)
{
    ASSERT(resource);
    ASSERT(resource->inCache());
    ASSERT(!memoryCache()->disabled());
    ASSERT(resource->canUseCacheValidator());
    ASSERT(!resource->resourceToRevalidate());
    
    // Copy the URL out of the resource to be revalidated in case it gets deleted by the remove() call below.
    String url = resource->url();
    CachedResource* newResource = createResource(resource->type(), KURL(ParsedURLString, url), resource->encoding());
    
    LOG(ResourceLoading, "Resource %p created to revalidate %p", newResource, resource);
    newResource->setResourceToRevalidate(resource);
    
    memoryCache()->remove(resource);
    memoryCache()->add(newResource);
    
    newResource->setLoadPriority(priority);
    newResource->load(this);
    
    m_validatedURLs.add(url);
    return newResource;
}

CachedResource* CachedResourceLoader::loadResource(CachedResource::Type type, const KURL& url, const String& charset, ResourceLoadPriority priority)
{
    ASSERT(!memoryCache()->resourceForURL(url));
    
    LOG(ResourceLoading, "Loading CachedResource for '%s'.", url.string().latin1().data());
    
    CachedResource* resource = createResource(type, url, charset);
    
    bool inCache = memoryCache()->add(resource);
    
    // Pretend the resource is in the cache, to prevent it from being deleted during the load() call.
    // FIXME: CachedResource should just use normal refcounting instead.
    if (!inCache)
        resource->setInCache(true);
    
    resource->setLoadPriority(priority);
    resource->load(this);
    
    if (!inCache) {
        resource->setOwningCachedResourceLoader(this);
        resource->setInCache(false);
    }

    // We don't support immediate loads, but we do support immediate failure.
    if (resource->errorOccurred()) {
        if (inCache)
            memoryCache()->remove(resource);
        else
            delete resource;
        return 0;
    }

    m_validatedURLs.add(url.string());
    return resource;
}

CachedResourceLoader::RevalidationPolicy CachedResourceLoader::determineRevalidationPolicy(CachedResource::Type type, bool forPreload, CachedResource* existingResource) const
{
    if (!existingResource)
        return Load;
    
    // We already have a preload going for this URL.
    if (forPreload && existingResource->isPreloaded())
        return Use;
    
    // If the same URL has been loaded as a different type, we need to reload.
    if (existingResource->type() != type) {
        LOG(ResourceLoading, "CachedResourceLoader::determineRevalidationPolicy reloading due to type mismatch.");
        return Reload;
    }
    
    // Don't reload resources while pasting.
    if (m_allowStaleResources)
        return Use;
    
    // Alwaus use preloads.
    if (existingResource->isPreloaded())
        return Use;
    
    // CachePolicyHistoryBuffer uses the cache no matter what.
    if (cachePolicy() == CachePolicyHistoryBuffer)
        return Use;

    // Don't reuse resources with Cache-control: no-store.
    if (existingResource->response().cacheControlContainsNoStore()) {
        LOG(ResourceLoading, "CachedResourceLoader::determineRevalidationPolicy reloading due to Cache-control: no-store.");
        return Reload;
    }

    // Avoid loading the same resource multiple times for a single document, even if the cache policies would tell us to.
    if (m_validatedURLs.contains(existingResource->url()))
        return Use;
    
    // CachePolicyReload always reloads
    if (cachePolicy() == CachePolicyReload) {
        LOG(ResourceLoading, "CachedResourceLoader::determineRevalidationPolicy reloading due to CachePolicyReload.");
        return Reload;
    }
    
    // We'll try to reload the resource if it failed last time.
    if (existingResource->errorOccurred()) {
        LOG(ResourceLoading, "CachedResourceLoader::determineRevalidationPolicye reloading due to resource being in the error state");
        return Reload;
    }
    
    // For resources that are not yet loaded we ignore the cache policy.
    if (existingResource->isLoading())
        return Use;

    // Check if the cache headers requires us to revalidate (cache expiration for example).
    if (existingResource->mustRevalidateDueToCacheHeaders(cachePolicy())) {
        // See if the resource has usable ETag or Last-modified headers.
        if (existingResource->canUseCacheValidator())
            return Revalidate;
        
        // No, must reload.
        LOG(ResourceLoading, "CachedResourceLoader::determineRevalidationPolicy reloading due to missing cache validators.");            
        return Reload;
    }

    return Use;
}

void CachedResourceLoader::printAccessDeniedMessage(const KURL& url) const
{
    if (url.isNull())
        return;

    if (!frame())
        return;

    Settings* settings = frame()->settings();
    if (!settings || settings->privateBrowsingEnabled())
        return;

    String message = m_document->url().isNull() ?
        makeString("Unsafe attempt to load URL ", url.string(), '.') :
        makeString("Unsafe attempt to load URL ", url.string(), " from frame with URL ", m_document->url().string(), ". Domains, protocols and ports must match.\n");

    // FIXME: provide a real line number and source URL.
    frame()->domWindow()->console()->addMessage(OtherMessageSource, LogMessageType, ErrorMessageLevel, message, 1, String());
}

void CachedResourceLoader::setAutoLoadImages(bool enable)
{
    if (enable == m_autoLoadImages)
        return;

    m_autoLoadImages = enable;

    if (!m_autoLoadImages)
        return;

    DocumentResourceMap::iterator end = m_documentResources.end();
    for (DocumentResourceMap::iterator it = m_documentResources.begin(); it != end; ++it) {
        CachedResource* resource = it->second.get();
        if (resource->type() == CachedResource::ImageResource) {
            CachedImage* image = const_cast<CachedImage*>(static_cast<const CachedImage*>(resource));

            if (image->stillNeedsLoad())
                load(image, true);
        }
    }
}

CachePolicy CachedResourceLoader::cachePolicy() const
{
    return frame() ? frame()->loader()->subresourceCachePolicy() : CachePolicyVerify;
}

void CachedResourceLoader::removeCachedResource(CachedResource* resource) const
{
#ifndef NDEBUG
    DocumentResourceMap::iterator it = m_documentResources.find(resource->url());
    if (it != m_documentResources.end())
        ASSERT(it->second.get() == resource);
#endif
    m_documentResources.remove(resource->url());
}

void CachedResourceLoader::load(CachedResource* resource, bool incremental, SecurityCheckPolicy securityCheck, bool sendResourceLoadCallbacks)
{
    incrementRequestCount(resource);

    RefPtr<CachedResourceRequest> request = CachedResourceRequest::load(this, resource, incremental, securityCheck, sendResourceLoadCallbacks);
    if (request)
        m_requests.add(request);
}

void CachedResourceLoader::loadDone(CachedResourceRequest* request)
{
    m_loadFinishing = false;
    RefPtr<CachedResourceRequest> protect(request);
    if (request)
        m_requests.remove(request);
    if (frame())
        frame()->loader()->loadDone();

    if (!request) {
        // If the request passed to this function is null, loadDone finished synchronously from when
        // the load was started, so we want to kick off our next set of loads (via checkForPendingPreloads
        // and servePendingRequests) asynchronously.
        m_loadDoneActionTimer.startOneShot(0);
        return;
    }

    performPostLoadActions();
}

void CachedResourceLoader::loadDoneActionTimerFired(Timer<CachedResourceLoader>*)
{
    performPostLoadActions();
}

void CachedResourceLoader::performPostLoadActions()
{
    checkForPendingPreloads();
    resourceLoadScheduler()->servePendingRequests();
}

void CachedResourceLoader::cancelRequests()
{
    clearPendingPreloads();
    Vector<CachedResourceRequest*, 256> requestsToCancel;
    RequestSet::iterator end = m_requests.end();
    for (RequestSet::iterator i = m_requests.begin(); i != end; ++i)
        requestsToCancel.append((*i).get());

    for (unsigned i = 0; i < requestsToCancel.size(); ++i)
        requestsToCancel[i]->didFail(true);
}

void CachedResourceLoader::notifyLoadedFromMemoryCache(CachedResource* resource)
{
    if (!resource || !frame() || resource->status() != CachedResource::Cached)
        return;

    // FIXME: If the WebKit client changes or cancels the request, WebCore does not respect this and continues the load.
    frame()->loader()->loadedResourceFromMemoryCache(resource);
}

void CachedResourceLoader::incrementRequestCount(const CachedResource* res)
{
    if (res->isLinkResource())
        return;

    ++m_requestCount;
}

void CachedResourceLoader::decrementRequestCount(const CachedResource* res)
{
    if (res->isLinkResource())
        return;

    --m_requestCount;
    ASSERT(m_requestCount > -1);
}

int CachedResourceLoader::requestCount()
{
    if (m_loadFinishing)
         return m_requestCount + 1;
    return m_requestCount;
}
    
void CachedResourceLoader::preload(CachedResource::Type type, const String& url, const String& charset, bool referencedFromBody)
{
    // FIXME: Rip this out when we are sure it is no longer necessary (even for mobile).
    UNUSED_PARAM(referencedFromBody);

    bool hasRendering = m_document->body() && m_document->body()->renderer();
    bool canBlockParser = type == CachedResource::Script || type == CachedResource::CSSStyleSheet;
    if (!hasRendering && !canBlockParser) {
        // Don't preload subresources that can't block the parser before we have something to draw.
        // This helps prevent preloads from delaying first display when bandwidth is limited.
        PendingPreload pendingPreload = { type, url, charset };
        m_pendingPreloads.append(pendingPreload);
        return;
    }
    requestPreload(type, url, charset);
}

void CachedResourceLoader::checkForPendingPreloads() 
{
    if (m_pendingPreloads.isEmpty() || !m_document->body() || !m_document->body()->renderer())
        return;
    while (!m_pendingPreloads.isEmpty()) {
        PendingPreload preload = m_pendingPreloads.takeFirst();
        // Don't request preload if the resource already loaded normally (this will result in double load if the page is being reloaded with cached results ignored).
        if (!cachedResource(m_document->completeURL(preload.m_url)))
            requestPreload(preload.m_type, preload.m_url, preload.m_charset);
    }
    m_pendingPreloads.clear();
}

void CachedResourceLoader::requestPreload(CachedResource::Type type, const String& url, const String& charset)
{
    String encoding;
    if (type == CachedResource::Script || type == CachedResource::CSSStyleSheet)
        encoding = charset.isEmpty() ? m_document->charset() : charset;

    CachedResource* resource = requestResource(type, url, encoding, ResourceLoadPriorityUnresolved, true);
    if (!resource || (m_preloads && m_preloads->contains(resource)))
        return;
    resource->increasePreloadCount();

    if (!m_preloads)
        m_preloads = adoptPtr(new ListHashSet<CachedResource*>);
    m_preloads->add(resource);

#if PRELOAD_DEBUG
    printf("PRELOADING %s\n",  resource->url().latin1().data());
#endif
}

void CachedResourceLoader::clearPreloads()
{
#if PRELOAD_DEBUG
    printPreloadStats();
#endif
    if (!m_preloads)
        return;

    ListHashSet<CachedResource*>::iterator end = m_preloads->end();
    for (ListHashSet<CachedResource*>::iterator it = m_preloads->begin(); it != end; ++it) {
        CachedResource* res = *it;
        res->decreasePreloadCount();
        if (res->canDelete() && !res->inCache())
            delete res;
        else if (res->preloadResult() == CachedResource::PreloadNotReferenced)
            memoryCache()->remove(res);
    }
    m_preloads.clear();
}

void CachedResourceLoader::clearPendingPreloads()
{
    m_pendingPreloads.clear();
}

#if PRELOAD_DEBUG
void CachedResourceLoader::printPreloadStats()
{
    unsigned scripts = 0;
    unsigned scriptMisses = 0;
    unsigned stylesheets = 0;
    unsigned stylesheetMisses = 0;
    unsigned images = 0;
    unsigned imageMisses = 0;
    ListHashSet<CachedResource*>::iterator end = m_preloads.end();
    for (ListHashSet<CachedResource*>::iterator it = m_preloads.begin(); it != end; ++it) {
        CachedResource* res = *it;
        if (res->preloadResult() == CachedResource::PreloadNotReferenced)
            printf("!! UNREFERENCED PRELOAD %s\n", res->url().latin1().data());
        else if (res->preloadResult() == CachedResource::PreloadReferencedWhileComplete)
            printf("HIT COMPLETE PRELOAD %s\n", res->url().latin1().data());
        else if (res->preloadResult() == CachedResource::PreloadReferencedWhileLoading)
            printf("HIT LOADING PRELOAD %s\n", res->url().latin1().data());
        
        if (res->type() == CachedResource::Script) {
            scripts++;
            if (res->preloadResult() < CachedResource::PreloadReferencedWhileLoading)
                scriptMisses++;
        } else if (res->type() == CachedResource::CSSStyleSheet) {
            stylesheets++;
            if (res->preloadResult() < CachedResource::PreloadReferencedWhileLoading)
                stylesheetMisses++;
        } else {
            images++;
            if (res->preloadResult() < CachedResource::PreloadReferencedWhileLoading)
                imageMisses++;
        }
        
        if (res->errorOccurred())
            memoryCache()->remove(res);
        
        res->decreasePreloadCount();
    }
    m_preloads.clear();
    
    if (scripts)
        printf("SCRIPTS: %d (%d hits, hit rate %d%%)\n", scripts, scripts - scriptMisses, (scripts - scriptMisses) * 100 / scripts);
    if (stylesheets)
        printf("STYLESHEETS: %d (%d hits, hit rate %d%%)\n", stylesheets, stylesheets - stylesheetMisses, (stylesheets - stylesheetMisses) * 100 / stylesheets);
    if (images)
        printf("IMAGES:  %d (%d hits, hit rate %d%%)\n", images, images - imageMisses, (images - imageMisses) * 100 / images);
}
#endif
    
}
