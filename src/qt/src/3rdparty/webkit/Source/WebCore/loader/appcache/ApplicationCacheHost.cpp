/*
 * Copyright (C) 2008, 2009 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "ApplicationCacheHost.h"

#if ENABLE(OFFLINE_WEB_APPLICATIONS)

#include "ApplicationCache.h"
#include "ApplicationCacheGroup.h"
#include "ApplicationCacheResource.h"
#include "DocumentLoader.h"
#include "DOMApplicationCache.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "MainResourceLoader.h"
#include "ProgressEvent.h"
#include "ResourceLoader.h"
#include "ResourceRequest.h"
#include "Settings.h"

namespace WebCore {

ApplicationCacheHost::ApplicationCacheHost(DocumentLoader* documentLoader)
    : m_domApplicationCache(0)
    , m_documentLoader(documentLoader)
    , m_defersEvents(true)
    , m_candidateApplicationCacheGroup(0)
{
    ASSERT(m_documentLoader);
}

ApplicationCacheHost::~ApplicationCacheHost()
{
    ASSERT(!m_applicationCache || !m_candidateApplicationCacheGroup || m_applicationCache->group() == m_candidateApplicationCacheGroup);

    if (m_applicationCache)
        m_applicationCache->group()->disassociateDocumentLoader(m_documentLoader);
    else if (m_candidateApplicationCacheGroup)
        m_candidateApplicationCacheGroup->disassociateDocumentLoader(m_documentLoader);
}

void ApplicationCacheHost::selectCacheWithoutManifest()
{
    ApplicationCacheGroup::selectCacheWithoutManifestURL(m_documentLoader->frame());
}

void ApplicationCacheHost::selectCacheWithManifest(const KURL& manifestURL)
{
    ApplicationCacheGroup::selectCache(m_documentLoader->frame(), manifestURL);
}

void ApplicationCacheHost::maybeLoadMainResource(ResourceRequest& request, SubstituteData& substituteData)
{
    // Check if this request should be loaded from the application cache
    if (!substituteData.isValid() && isApplicationCacheEnabled()) {
        ASSERT(!m_mainResourceApplicationCache);

        m_mainResourceApplicationCache = ApplicationCacheGroup::cacheForMainRequest(request, m_documentLoader);

        if (m_mainResourceApplicationCache) {
            // Get the resource from the application cache. By definition, cacheForMainRequest() returns a cache that contains the resource.
            ApplicationCacheResource* resource = m_mainResourceApplicationCache->resourceForRequest(request);
            substituteData = SubstituteData(resource->data(), 
                                            resource->response().mimeType(),
                                            resource->response().textEncodingName(), KURL());
        }
    }
}

void ApplicationCacheHost::maybeLoadMainResourceForRedirect(ResourceRequest& request, SubstituteData& substituteData)
{
    ASSERT(status() == UNCACHED);
    maybeLoadMainResource(request, substituteData);
}

bool ApplicationCacheHost::maybeLoadFallbackForMainResponse(const ResourceRequest& request, const ResourceResponse& r)
{
    if (r.httpStatusCode() / 100 == 4 || r.httpStatusCode() / 100 == 5) {
        ASSERT(!m_mainResourceApplicationCache);
        if (isApplicationCacheEnabled()) {
            m_mainResourceApplicationCache = ApplicationCacheGroup::fallbackCacheForMainRequest(request, documentLoader());

            if (scheduleLoadFallbackResourceFromApplicationCache(documentLoader()->mainResourceLoader(), m_mainResourceApplicationCache.get()))
                return true;
        }
    }
    return false;
}

bool ApplicationCacheHost::maybeLoadFallbackForMainError(const ResourceRequest& request, const ResourceError& error)
{
    if (!error.isCancellation()) {
        ASSERT(!m_mainResourceApplicationCache);
        if (isApplicationCacheEnabled()) {
            m_mainResourceApplicationCache = ApplicationCacheGroup::fallbackCacheForMainRequest(request, m_documentLoader);

            if (scheduleLoadFallbackResourceFromApplicationCache(documentLoader()->mainResourceLoader(), m_mainResourceApplicationCache.get()))
                return true;
        }
    }
    return false;
}

void ApplicationCacheHost::mainResourceDataReceived(const char*, int, long long, bool)
{
    // This method is here to facilitate alternate implemetations of this interface by the host browser.
}

void ApplicationCacheHost::failedLoadingMainResource()
{
    ApplicationCacheGroup* group = m_candidateApplicationCacheGroup;
    if (!group && m_applicationCache) {
        if (mainResourceApplicationCache()) {
            // Even when the main resource is being loaded from an application cache, loading can fail if aborted.
            return;
        }
        group = m_applicationCache->group();
    }
    
    if (group)
        group->failedLoadingMainResource(m_documentLoader);
}

void ApplicationCacheHost::finishedLoadingMainResource()
{
    ApplicationCacheGroup* group = candidateApplicationCacheGroup();
    if (!group && applicationCache() && !mainResourceApplicationCache())
        group = applicationCache()->group();
    
    if (group)
        group->finishedLoadingMainResource(m_documentLoader);
}

bool ApplicationCacheHost::maybeLoadResource(ResourceLoader* loader, ResourceRequest& request, const KURL& originalURL)
{
    if (!isApplicationCacheEnabled())
        return false;
    
    if (request.url() != originalURL)
        return false;

    ApplicationCacheResource* resource;
    if (!shouldLoadResourceFromApplicationCache(request, resource))
        return false;
    
    m_documentLoader->m_pendingSubstituteResources.set(loader, resource);
    m_documentLoader->deliverSubstituteResourcesAfterDelay();
        
    return true;
}

bool ApplicationCacheHost::maybeLoadFallbackForRedirect(ResourceLoader* resourceLoader, ResourceRequest& request, const ResourceResponse& redirectResponse)
{
    if (!redirectResponse.isNull() && !protocolHostAndPortAreEqual(request.url(), redirectResponse.url()))
        if (scheduleLoadFallbackResourceFromApplicationCache(resourceLoader))
            return true;
    return false;
}

bool ApplicationCacheHost::maybeLoadFallbackForResponse(ResourceLoader* resourceLoader, const ResourceResponse& response)
{
    if (response.httpStatusCode() / 100 == 4 || response.httpStatusCode() / 100 == 5)
        if (scheduleLoadFallbackResourceFromApplicationCache(resourceLoader))
            return true;
    return false;
}

bool ApplicationCacheHost::maybeLoadFallbackForError(ResourceLoader* resourceLoader, const ResourceError& error)
{
    if (!error.isCancellation())
        if (scheduleLoadFallbackResourceFromApplicationCache(resourceLoader))
            return true;
    return false;
}

bool ApplicationCacheHost::maybeLoadSynchronously(ResourceRequest& request, ResourceError& error, ResourceResponse& response, Vector<char>& data)
{
    ApplicationCacheResource* resource;
    if (shouldLoadResourceFromApplicationCache(request, resource)) {
        if (resource) {
            response = resource->response();
            data.append(resource->data()->data(), resource->data()->size());
        } else {
            error = documentLoader()->frameLoader()->client()->cannotShowURLError(request);
        }
        return true;
    }
    return false;
}

void ApplicationCacheHost::maybeLoadFallbackSynchronously(const ResourceRequest& request, ResourceError& error, ResourceResponse& response, Vector<char>& data)
{
    // If normal loading results in a redirect to a resource with another origin (indicative of a captive portal), or a 4xx or 5xx status code or equivalent,
    // or if there were network errors (but not if the user canceled the download), then instead get, from the cache, the resource of the fallback entry
    // corresponding to the matched namespace.
    if ((!error.isNull() && !error.isCancellation())
         || response.httpStatusCode() / 100 == 4 || response.httpStatusCode() / 100 == 5
         || !protocolHostAndPortAreEqual(request.url(), response.url())) {
        ApplicationCacheResource* resource;
        if (getApplicationCacheFallbackResource(request, resource)) {
            response = resource->response();
            data.clear();
            data.append(resource->data()->data(), resource->data()->size());
        }
    }
}

bool ApplicationCacheHost::canCacheInPageCache() const 
{
    return !applicationCache() && !candidateApplicationCacheGroup();
}

void ApplicationCacheHost::setDOMApplicationCache(DOMApplicationCache* domApplicationCache)
{
    ASSERT(!m_domApplicationCache || !domApplicationCache);
    m_domApplicationCache = domApplicationCache;
}

void ApplicationCacheHost::notifyDOMApplicationCache(EventID id, int total, int done)
{
    if (m_defersEvents) {
        // Event dispatching is deferred until document.onload has fired.
        m_deferredEvents.append(DeferredEvent(id, total, done));
        return;
    }
    dispatchDOMEvent(id, total, done);
}

void ApplicationCacheHost::stopLoadingInFrame(Frame* frame)
{
    ASSERT(!m_applicationCache || !m_candidateApplicationCacheGroup || m_applicationCache->group() == m_candidateApplicationCacheGroup);

    if (m_candidateApplicationCacheGroup)
        m_candidateApplicationCacheGroup->stopLoadingInFrame(frame);
    else if (m_applicationCache)
        m_applicationCache->group()->stopLoadingInFrame(frame);
}

void ApplicationCacheHost::stopDeferringEvents()
{
    RefPtr<DocumentLoader> protect(documentLoader());
    for (unsigned i = 0; i < m_deferredEvents.size(); ++i) {
        const DeferredEvent& deferred = m_deferredEvents[i];
        dispatchDOMEvent(deferred.eventID, deferred.progressTotal, deferred.progressDone);
    }
    m_deferredEvents.clear();
    m_defersEvents = false;
}

#if ENABLE(INSPECTOR)
void ApplicationCacheHost::fillResourceList(ResourceInfoList* resources)
{
    ApplicationCache* cache = applicationCache();
    if (!cache || !cache->isComplete())
        return;
     
    ApplicationCache::ResourceMap::const_iterator end = cache->end();
    for (ApplicationCache::ResourceMap::const_iterator it = cache->begin(); it != end; ++it) {
        RefPtr<ApplicationCacheResource> resource = it->second;
        unsigned type = resource->type();
        bool isMaster   = type & ApplicationCacheResource::Master;
        bool isManifest = type & ApplicationCacheResource::Manifest;
        bool isExplicit = type & ApplicationCacheResource::Explicit;
        bool isForeign  = type & ApplicationCacheResource::Foreign;
        bool isFallback = type & ApplicationCacheResource::Fallback;
        resources->append(ResourceInfo(resource->url(), isMaster, isManifest, isFallback, isForeign, isExplicit, resource->estimatedSizeInStorage()));
    }
}
 
ApplicationCacheHost::CacheInfo ApplicationCacheHost::applicationCacheInfo()
{
    ApplicationCache* cache = applicationCache();
    if (!cache || !cache->isComplete())
        return CacheInfo(KURL(), 0, 0, 0);
  
    // FIXME: Add "Creation Time" and "Update Time" to Application Caches.
    return CacheInfo(cache->manifestResource()->url(), 0, 0, cache->estimatedSizeInStorage());
}
#endif

void ApplicationCacheHost::dispatchDOMEvent(EventID id, int total, int done)
{
    if (m_domApplicationCache) {
        const AtomicString& eventType = DOMApplicationCache::toEventType(id);
        ExceptionCode ec = 0;
        RefPtr<Event> event;
        if (id == PROGRESS_EVENT)
            event = ProgressEvent::create(eventType, true, done, total);
        else
            event = Event::create(eventType, false, false);
        m_domApplicationCache->dispatchEvent(event, ec);
        ASSERT(!ec);
    }
}

void ApplicationCacheHost::setCandidateApplicationCacheGroup(ApplicationCacheGroup* group)
{
    ASSERT(!m_applicationCache);
    m_candidateApplicationCacheGroup = group;
}
    
void ApplicationCacheHost::setApplicationCache(PassRefPtr<ApplicationCache> applicationCache)
{
    if (m_candidateApplicationCacheGroup) {
        ASSERT(!m_applicationCache);
        m_candidateApplicationCacheGroup = 0;
    }

    m_applicationCache = applicationCache;
}

bool ApplicationCacheHost::shouldLoadResourceFromApplicationCache(const ResourceRequest& request, ApplicationCacheResource*& resource)
{
    ApplicationCache* cache = applicationCache();
    if (!cache || !cache->isComplete())
        return false;

    // If the resource is not to be fetched using the HTTP GET mechanism or equivalent, or if its URL has a different
    // <scheme> component than the application cache's manifest, then fetch the resource normally.
    if (!ApplicationCache::requestIsHTTPOrHTTPSGet(request) || !equalIgnoringCase(request.url().protocol(), cache->manifestResource()->url().protocol()))
        return false;

    // If the resource's URL is an master entry, the manifest, an explicit entry, or a fallback entry
    // in the application cache, then get the resource from the cache (instead of fetching it).
    resource = cache->resourceForURL(request.url());

    // Resources that match fallback namespaces or online whitelist entries are fetched from the network,
    // unless they are also cached.
    if (!resource && (cache->allowsAllNetworkRequests() || cache->urlMatchesFallbackNamespace(request.url()) || cache->isURLInOnlineWhitelist(request.url())))
        return false;

    // Resources that are not present in the manifest will always fail to load (at least, after the
    // cache has been primed the first time), making the testing of offline applications simpler.
    return true;
}

bool ApplicationCacheHost::getApplicationCacheFallbackResource(const ResourceRequest& request, ApplicationCacheResource*& resource, ApplicationCache* cache)
{
    if (!cache) {
        cache = applicationCache();
        if (!cache)
            return false;
    }
    if (!cache->isComplete())
        return false;
    
    // If the resource is not a HTTP/HTTPS GET, then abort
    if (!ApplicationCache::requestIsHTTPOrHTTPSGet(request))
        return false;

    KURL fallbackURL;
    if (cache->isURLInOnlineWhitelist(request.url()))
        return false;
    if (!cache->urlMatchesFallbackNamespace(request.url(), &fallbackURL))
        return false;

    resource = cache->resourceForURL(fallbackURL);
    ASSERT(resource);

    return true;
}

bool ApplicationCacheHost::scheduleLoadFallbackResourceFromApplicationCache(ResourceLoader* loader, ApplicationCache* cache)
{
    if (!isApplicationCacheEnabled())
        return false;

    ApplicationCacheResource* resource;
    if (!getApplicationCacheFallbackResource(loader->request(), resource, cache))
        return false;

    m_documentLoader->m_pendingSubstituteResources.set(loader, resource);
    m_documentLoader->deliverSubstituteResourcesAfterDelay();
    
    loader->handle()->cancel();

    return true;
}

ApplicationCacheHost::Status ApplicationCacheHost::status() const
{
    ApplicationCache* cache = applicationCache();    
    if (!cache)
        return UNCACHED;

    switch (cache->group()->updateStatus()) {
        case ApplicationCacheGroup::Checking:
            return CHECKING;
        case ApplicationCacheGroup::Downloading:
            return DOWNLOADING;
        case ApplicationCacheGroup::Idle: {
            if (cache->group()->isObsolete())
                return OBSOLETE;
            if (cache != cache->group()->newestCache())
                return UPDATEREADY;
            return IDLE;
        }
    }

    ASSERT_NOT_REACHED();
    return UNCACHED;
}

bool ApplicationCacheHost::update()
{
    ApplicationCache* cache = applicationCache();
    if (!cache)
        return false;
    cache->group()->update(m_documentLoader->frame(), ApplicationCacheUpdateWithoutBrowsingContext);
    return true;
}

bool ApplicationCacheHost::swapCache()
{
    ApplicationCache* cache = applicationCache();
    if (!cache)
        return false;

    // If the group of application caches to which cache belongs has the lifecycle status obsolete, unassociate document from cache.
    if (cache->group()->isObsolete()) {
        cache->group()->disassociateDocumentLoader(m_documentLoader);
        return true;
    }

    // If there is no newer cache, raise an INVALID_STATE_ERR exception.
    ApplicationCache* newestCache = cache->group()->newestCache();
    if (cache == newestCache)
        return false;
    
    ASSERT(cache->group() == newestCache->group());
    setApplicationCache(newestCache);
    
    return true;
}

bool ApplicationCacheHost::isApplicationCacheEnabled()
{
    return m_documentLoader->frame()->settings()
           && m_documentLoader->frame()->settings()->offlineWebApplicationCacheEnabled();
}

}  // namespace WebCore

#endif  // ENABLE(OFFLINE_WEB_APPLICATIONS)
