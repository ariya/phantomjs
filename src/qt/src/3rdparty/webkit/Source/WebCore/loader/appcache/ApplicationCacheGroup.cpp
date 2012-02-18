/*
 * Copyright (C) 2008, 2009, 2010 Apple Inc. All Rights Reserved.
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
#include "ApplicationCacheGroup.h"

#if ENABLE(OFFLINE_WEB_APPLICATIONS)

#include "ApplicationCache.h"
#include "ApplicationCacheHost.h"
#include "ApplicationCacheResource.h"
#include "ApplicationCacheStorage.h"
#include "Chrome.h"
#include "ChromeClient.h"
#include "DocumentLoader.h"
#include "DOMApplicationCache.h"
#include "DOMWindow.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "InspectorInstrumentation.h"
#include "MainResourceLoader.h"
#include "ManifestParser.h"
#include "Page.h"
#include "SecurityOrigin.h"
#include "Settings.h"
#include <wtf/HashMap.h>

#if ENABLE(INSPECTOR)
#include "ProgressTracker.h"
#endif

namespace WebCore {

ApplicationCacheGroup::ApplicationCacheGroup(const KURL& manifestURL, bool isCopy)
    : m_manifestURL(manifestURL)
    , m_origin(SecurityOrigin::create(manifestURL))
    , m_updateStatus(Idle)
    , m_downloadingPendingMasterResourceLoadersCount(0)
    , m_progressTotal(0)
    , m_progressDone(0)
    , m_frame(0)
    , m_storageID(0)
    , m_isObsolete(false)
    , m_completionType(None)
    , m_isCopy(isCopy)
    , m_calledReachedMaxAppCacheSize(false)
    , m_loadedSize(0)
    , m_availableSpaceInQuota(ApplicationCacheStorage::unknownQuota())
    , m_originQuotaReached(false)
{
}

ApplicationCacheGroup::~ApplicationCacheGroup()
{
    if (m_isCopy) {
        ASSERT(m_newestCache);
        ASSERT(m_caches.size() == 1);
        ASSERT(m_caches.contains(m_newestCache.get()));
        ASSERT(!m_cacheBeingUpdated);
        ASSERT(m_associatedDocumentLoaders.isEmpty());
        ASSERT(m_pendingMasterResourceLoaders.isEmpty());
        ASSERT(m_newestCache->group() == this);
        
        return;
    }
               
    ASSERT(!m_newestCache);
    ASSERT(m_caches.isEmpty());
    
    stopLoading();
    
    cacheStorage().cacheGroupDestroyed(this);
}
    
ApplicationCache* ApplicationCacheGroup::cacheForMainRequest(const ResourceRequest& request, DocumentLoader*)
{
    if (!ApplicationCache::requestIsHTTPOrHTTPSGet(request))
        return 0;

    KURL url(request.url());
    if (url.hasFragmentIdentifier())
        url.removeFragmentIdentifier();

    if (ApplicationCacheGroup* group = cacheStorage().cacheGroupForURL(url)) {
        ASSERT(group->newestCache());
        ASSERT(!group->isObsolete());
        
        return group->newestCache();
    }
    
    return 0;
}
    
ApplicationCache* ApplicationCacheGroup::fallbackCacheForMainRequest(const ResourceRequest& request, DocumentLoader*)
{
    if (!ApplicationCache::requestIsHTTPOrHTTPSGet(request))
        return 0;

    KURL url(request.url());
    if (url.hasFragmentIdentifier())
        url.removeFragmentIdentifier();

    if (ApplicationCacheGroup* group = cacheStorage().fallbackCacheGroupForURL(url)) {
        ASSERT(group->newestCache());
        ASSERT(!group->isObsolete());

        return group->newestCache();
    }
    
    return 0;
}

void ApplicationCacheGroup::selectCache(Frame* frame, const KURL& passedManifestURL)
{
    ASSERT(frame && frame->page());
    
    if (!frame->settings()->offlineWebApplicationCacheEnabled())
        return;
    
    DocumentLoader* documentLoader = frame->loader()->documentLoader();
    ASSERT(!documentLoader->applicationCacheHost()->applicationCache());

    if (passedManifestURL.isNull()) {
        selectCacheWithoutManifestURL(frame);        
        return;
    }

    KURL manifestURL(passedManifestURL);
    if (manifestURL.hasFragmentIdentifier())
        manifestURL.removeFragmentIdentifier();

    ApplicationCache* mainResourceCache = documentLoader->applicationCacheHost()->mainResourceApplicationCache();
    
    if (mainResourceCache) {
        if (manifestURL == mainResourceCache->group()->m_manifestURL) {
            // The cache may have gotten obsoleted after we've loaded from it, but before we parsed the document and saw cache manifest.
            if (mainResourceCache->group()->isObsolete())
                return;
            mainResourceCache->group()->associateDocumentLoaderWithCache(documentLoader, mainResourceCache);
            mainResourceCache->group()->update(frame, ApplicationCacheUpdateWithBrowsingContext);
        } else {
            // The main resource was loaded from cache, so the cache must have an entry for it. Mark it as foreign.
            KURL resourceURL(documentLoader->responseURL());
            if (resourceURL.hasFragmentIdentifier())
                resourceURL.removeFragmentIdentifier();
            ApplicationCacheResource* resource = mainResourceCache->resourceForURL(resourceURL);
            bool inStorage = resource->storageID();
            resource->addType(ApplicationCacheResource::Foreign);
            if (inStorage)
                cacheStorage().storeUpdatedType(resource, mainResourceCache);

            // Restart the current navigation from the top of the navigation algorithm, undoing any changes that were made
            // as part of the initial load.
            // The navigation will not result in the same resource being loaded, because "foreign" entries are never picked during navigation.
            frame->navigationScheduler()->scheduleLocationChange(frame->document()->securityOrigin(), documentLoader->url(), frame->loader()->referrer(), true);
        }
        
        return;
    }
    
    // The resource was loaded from the network, check if it is a HTTP/HTTPS GET.    
    const ResourceRequest& request = frame->loader()->activeDocumentLoader()->request();

    if (!ApplicationCache::requestIsHTTPOrHTTPSGet(request))
        return;

    // Check that the resource URL has the same scheme/host/port as the manifest URL.
    if (!protocolHostAndPortAreEqual(manifestURL, request.url()))
        return;

    // Don't change anything on disk if private browsing is enabled.
    if (!frame->settings() || frame->settings()->privateBrowsingEnabled()) {
        postListenerTask(ApplicationCacheHost::CHECKING_EVENT, documentLoader);
        postListenerTask(ApplicationCacheHost::ERROR_EVENT, documentLoader);
        return;
    }

    ApplicationCacheGroup* group = cacheStorage().findOrCreateCacheGroup(manifestURL);

    documentLoader->applicationCacheHost()->setCandidateApplicationCacheGroup(group);
    group->m_pendingMasterResourceLoaders.add(documentLoader);
    group->m_downloadingPendingMasterResourceLoadersCount++;

    ASSERT(!group->m_cacheBeingUpdated || group->m_updateStatus != Idle);
    group->update(frame, ApplicationCacheUpdateWithBrowsingContext);
}

void ApplicationCacheGroup::selectCacheWithoutManifestURL(Frame* frame)
{
    if (!frame->settings()->offlineWebApplicationCacheEnabled())
        return;

    DocumentLoader* documentLoader = frame->loader()->documentLoader();
    ASSERT(!documentLoader->applicationCacheHost()->applicationCache());

    ApplicationCache* mainResourceCache = documentLoader->applicationCacheHost()->mainResourceApplicationCache();

    if (mainResourceCache) {
        mainResourceCache->group()->associateDocumentLoaderWithCache(documentLoader, mainResourceCache);
        mainResourceCache->group()->update(frame, ApplicationCacheUpdateWithBrowsingContext);
    }
}

void ApplicationCacheGroup::finishedLoadingMainResource(DocumentLoader* loader)
{
    ASSERT(m_pendingMasterResourceLoaders.contains(loader));
    ASSERT(m_completionType == None || m_pendingEntries.isEmpty());
    KURL url = loader->url();
    if (url.hasFragmentIdentifier())
        url.removeFragmentIdentifier();

    switch (m_completionType) {
    case None:
        // The main resource finished loading before the manifest was ready. It will be handled via dispatchMainResources() later.
        return;
    case NoUpdate:
        ASSERT(!m_cacheBeingUpdated);
        associateDocumentLoaderWithCache(loader, m_newestCache.get());

        if (ApplicationCacheResource* resource = m_newestCache->resourceForURL(url)) {
            if (!(resource->type() & ApplicationCacheResource::Master)) {
                resource->addType(ApplicationCacheResource::Master);
                ASSERT(!resource->storageID());
            }
        } else
            m_newestCache->addResource(ApplicationCacheResource::create(url, loader->response(), ApplicationCacheResource::Master, loader->mainResourceData()));

        break;
    case Failure:
        // Cache update has been a failure, so there is no reason to keep the document associated with the incomplete cache
        // (its main resource was not cached yet, so it is likely that the application changed significantly server-side).
        ASSERT(!m_cacheBeingUpdated); // Already cleared out by stopLoading().
        loader->applicationCacheHost()->setApplicationCache(0); // Will unset candidate, too.
        m_associatedDocumentLoaders.remove(loader);
        postListenerTask(ApplicationCacheHost::ERROR_EVENT, loader);
        break;
    case Completed:
        ASSERT(m_associatedDocumentLoaders.contains(loader));

        if (ApplicationCacheResource* resource = m_cacheBeingUpdated->resourceForURL(url)) {
            if (!(resource->type() & ApplicationCacheResource::Master)) {
                resource->addType(ApplicationCacheResource::Master);
                ASSERT(!resource->storageID());
            }
        } else
            m_cacheBeingUpdated->addResource(ApplicationCacheResource::create(url, loader->response(), ApplicationCacheResource::Master, loader->mainResourceData()));
        // The "cached" event will be posted to all associated documents once update is complete.
        break;
    }

    m_downloadingPendingMasterResourceLoadersCount--;
    checkIfLoadIsComplete();
}

void ApplicationCacheGroup::failedLoadingMainResource(DocumentLoader* loader)
{
    ASSERT(m_pendingMasterResourceLoaders.contains(loader));
    ASSERT(m_completionType == None || m_pendingEntries.isEmpty());

    switch (m_completionType) {
    case None:
        // The main resource finished loading before the manifest was ready. It will be handled via dispatchMainResources() later.
        return;
    case NoUpdate:
        ASSERT(!m_cacheBeingUpdated);

        // The manifest didn't change, and we have a relevant cache - but the main resource download failed mid-way, so it cannot be stored to the cache,
        // and the loader does not get associated to it. If there are other main resources being downloaded for this cache group, they may still succeed.
        postListenerTask(ApplicationCacheHost::ERROR_EVENT, loader);

        break;
    case Failure:
        // Cache update failed, too.
        ASSERT(!m_cacheBeingUpdated); // Already cleared out by stopLoading().
        ASSERT(!loader->applicationCacheHost()->applicationCache() || loader->applicationCacheHost()->applicationCache() == m_cacheBeingUpdated);

        loader->applicationCacheHost()->setApplicationCache(0); // Will unset candidate, too.
        m_associatedDocumentLoaders.remove(loader);
        postListenerTask(ApplicationCacheHost::ERROR_EVENT, loader);
        break;
    case Completed:
        // The cache manifest didn't list this main resource, and all cache entries were already updated successfully - but the main resource failed to load,
        // so it cannot be stored to the cache. If there are other main resources being downloaded for this cache group, they may still succeed.
        ASSERT(m_associatedDocumentLoaders.contains(loader));
        ASSERT(loader->applicationCacheHost()->applicationCache() == m_cacheBeingUpdated);
        ASSERT(!loader->applicationCacheHost()->candidateApplicationCacheGroup());
        m_associatedDocumentLoaders.remove(loader);
        loader->applicationCacheHost()->setApplicationCache(0);

        postListenerTask(ApplicationCacheHost::ERROR_EVENT, loader);

        break;
    }

    m_downloadingPendingMasterResourceLoadersCount--;
    checkIfLoadIsComplete();
}

void ApplicationCacheGroup::stopLoading()
{
    if (m_manifestHandle) {
        ASSERT(!m_currentHandle);

        m_manifestHandle->setClient(0);
        m_manifestHandle->cancel();
        m_manifestHandle = 0;
    }
    
    if (m_currentHandle) {
        ASSERT(!m_manifestHandle);
        ASSERT(m_cacheBeingUpdated);

        m_currentHandle->setClient(0);
        m_currentHandle->cancel();
        m_currentHandle = 0;
    }    
    
    m_cacheBeingUpdated = 0;
    m_pendingEntries.clear();
}    

void ApplicationCacheGroup::disassociateDocumentLoader(DocumentLoader* loader)
{
    HashSet<DocumentLoader*>::iterator it = m_associatedDocumentLoaders.find(loader);
    if (it != m_associatedDocumentLoaders.end())
        m_associatedDocumentLoaders.remove(it);

    m_pendingMasterResourceLoaders.remove(loader);

    loader->applicationCacheHost()->setApplicationCache(0); // Will set candidate to 0, too.

    if (!m_associatedDocumentLoaders.isEmpty() || !m_pendingMasterResourceLoaders.isEmpty())
        return;

    if (m_caches.isEmpty()) {
        // There is an initial cache attempt in progress.
        ASSERT(!m_newestCache);
        // Delete ourselves, causing the cache attempt to be stopped.
        delete this;
        return;
    }

    ASSERT(m_caches.contains(m_newestCache.get()));

    // Release our reference to the newest cache. This could cause us to be deleted.
    // Any ongoing updates will be stopped from destructor.
    m_newestCache.release();
}

void ApplicationCacheGroup::cacheDestroyed(ApplicationCache* cache)
{
    if (!m_caches.contains(cache))
        return;
    
    m_caches.remove(cache);

    if (m_caches.isEmpty()) {
        ASSERT(m_associatedDocumentLoaders.isEmpty());
        ASSERT(m_pendingMasterResourceLoaders.isEmpty());
        delete this;
    }
}

void ApplicationCacheGroup::stopLoadingInFrame(Frame* frame)
{
    if (frame != m_frame)
        return;

    stopLoading();
}

void ApplicationCacheGroup::setNewestCache(PassRefPtr<ApplicationCache> newestCache)
{
    m_newestCache = newestCache;

    m_caches.add(m_newestCache.get());
    m_newestCache->setGroup(this);
    InspectorInstrumentation::updateApplicationCacheStatus(m_frame);
}

void ApplicationCacheGroup::makeObsolete()
{
    if (isObsolete())
        return;

    m_isObsolete = true;
    cacheStorage().cacheGroupMadeObsolete(this);
    ASSERT(!m_storageID);
    InspectorInstrumentation::updateApplicationCacheStatus(m_frame);
}

void ApplicationCacheGroup::update(Frame* frame, ApplicationCacheUpdateOption updateOption)
{
    if (m_updateStatus == Checking || m_updateStatus == Downloading) {
        if (updateOption == ApplicationCacheUpdateWithBrowsingContext) {
            postListenerTask(ApplicationCacheHost::CHECKING_EVENT, frame->loader()->documentLoader());
            if (m_updateStatus == Downloading)
                postListenerTask(ApplicationCacheHost::DOWNLOADING_EVENT, frame->loader()->documentLoader());
        }
        return;
    }

    // Don't change anything on disk if private browsing is enabled.
    if (!frame->settings() || frame->settings()->privateBrowsingEnabled()) {
        ASSERT(m_pendingMasterResourceLoaders.isEmpty());
        ASSERT(m_pendingEntries.isEmpty());
        ASSERT(!m_cacheBeingUpdated);
        postListenerTask(ApplicationCacheHost::CHECKING_EVENT, frame->loader()->documentLoader());
        postListenerTask(ApplicationCacheHost::NOUPDATE_EVENT, frame->loader()->documentLoader());
        return;
    }

    ASSERT(!m_frame);
    m_frame = frame;

    setUpdateStatus(Checking);

    postListenerTask(ApplicationCacheHost::CHECKING_EVENT, m_associatedDocumentLoaders);
    if (!m_newestCache) {
        ASSERT(updateOption == ApplicationCacheUpdateWithBrowsingContext);
        postListenerTask(ApplicationCacheHost::CHECKING_EVENT, frame->loader()->documentLoader());
    }
    
    ASSERT(!m_manifestHandle);
    ASSERT(!m_manifestResource);
    ASSERT(m_completionType == None);

    // FIXME: Handle defer loading
    m_manifestHandle = createResourceHandle(m_manifestURL, m_newestCache ? m_newestCache->manifestResource() : 0);
}

PassRefPtr<ResourceHandle> ApplicationCacheGroup::createResourceHandle(const KURL& url, ApplicationCacheResource* newestCachedResource)
{
    ResourceRequest request(url);
    m_frame->loader()->applyUserAgent(request);
    request.setHTTPHeaderField("Cache-Control", "max-age=0");

    if (newestCachedResource) {
        const String& lastModified = newestCachedResource->response().httpHeaderField("Last-Modified");
        const String& eTag = newestCachedResource->response().httpHeaderField("ETag");
        if (!lastModified.isEmpty() || !eTag.isEmpty()) {
            if (!lastModified.isEmpty())
                request.setHTTPHeaderField("If-Modified-Since", lastModified);
            if (!eTag.isEmpty())
                request.setHTTPHeaderField("If-None-Match", eTag);
        }
    }

    RefPtr<ResourceHandle> handle = ResourceHandle::create(m_frame->loader()->networkingContext(), request, this, false, true);
#if ENABLE(INSPECTOR)
    // Because willSendRequest only gets called during redirects, we initialize
    // the identifier and the first willSendRequest here.
    m_currentResourceIdentifier = m_frame->page()->progress()->createUniqueIdentifier();
    ResourceResponse redirectResponse = ResourceResponse();
    InspectorInstrumentation::willSendRequest(m_frame, m_currentResourceIdentifier, m_frame->loader()->documentLoader(), request, redirectResponse);
#endif
    return handle;
}

void ApplicationCacheGroup::didReceiveResponse(ResourceHandle* handle, const ResourceResponse& response)
{
#if ENABLE(INSPECTOR)
    DocumentLoader* loader = (handle == m_manifestHandle) ? 0 : m_frame->loader()->documentLoader();
    InspectorInstrumentationCookie cookie = InspectorInstrumentation::willReceiveResourceResponse(m_frame, m_currentResourceIdentifier, response);
    InspectorInstrumentation::didReceiveResourceResponse(cookie, m_currentResourceIdentifier, loader, response);
#endif

    if (handle == m_manifestHandle) {
        didReceiveManifestResponse(response);
        return;
    }
    
    ASSERT(handle == m_currentHandle);

    KURL url(handle->firstRequest().url());
    if (url.hasFragmentIdentifier())
        url.removeFragmentIdentifier();
    
    ASSERT(!m_currentResource);
    ASSERT(m_pendingEntries.contains(url));
    
    unsigned type = m_pendingEntries.get(url);
    
    // If this is an initial cache attempt, we should not get master resources delivered here.
    if (!m_newestCache)
        ASSERT(!(type & ApplicationCacheResource::Master));

    if (m_newestCache && response.httpStatusCode() == 304) { // Not modified.
        ApplicationCacheResource* newestCachedResource = m_newestCache->resourceForURL(url);
        if (newestCachedResource) {
            m_cacheBeingUpdated->addResource(ApplicationCacheResource::create(url, newestCachedResource->response(), type, newestCachedResource->data(), newestCachedResource->path()));
            m_pendingEntries.remove(m_currentHandle->firstRequest().url());
            m_currentHandle->cancel();
            m_currentHandle = 0;
            // Load the next resource, if any.
            startLoadingEntry();
            return;
        }
        // The server could return 304 for an unconditional request - in this case, we handle the response as a normal error.
    }

    if (response.httpStatusCode() / 100 != 2 || response.url() != m_currentHandle->firstRequest().url()) {
        if ((type & ApplicationCacheResource::Explicit) || (type & ApplicationCacheResource::Fallback)) {
            // Note that cacheUpdateFailed() can cause the cache group to be deleted.
            cacheUpdateFailed();
        } else if (response.httpStatusCode() == 404 || response.httpStatusCode() == 410) {
            // Skip this resource. It is dropped from the cache.
            m_currentHandle->cancel();
            m_currentHandle = 0;
            m_pendingEntries.remove(url);
            // Load the next resource, if any.
            startLoadingEntry();
        } else {
            // Copy the resource and its metadata from the newest application cache in cache group whose completeness flag is complete, and act
            // as if that was the fetched resource, ignoring the resource obtained from the network.
            ASSERT(m_newestCache);
            ApplicationCacheResource* newestCachedResource = m_newestCache->resourceForURL(handle->firstRequest().url());
            ASSERT(newestCachedResource);
            m_cacheBeingUpdated->addResource(ApplicationCacheResource::create(url, newestCachedResource->response(), type, newestCachedResource->data(), newestCachedResource->path()));
            m_pendingEntries.remove(m_currentHandle->firstRequest().url());
            m_currentHandle->cancel();
            m_currentHandle = 0;
            // Load the next resource, if any.
            startLoadingEntry();
        }
        return;
    }
    
    m_currentResource = ApplicationCacheResource::create(url, response, type);
}

void ApplicationCacheGroup::didReceiveData(ResourceHandle* handle, const char* data, int length, int encodedDataLength)
{
    UNUSED_PARAM(encodedDataLength);

#if ENABLE(INSPECTOR)
    InspectorInstrumentation::didReceiveContentLength(m_frame, m_currentResourceIdentifier, length, 0);
#endif

    if (handle == m_manifestHandle) {
        didReceiveManifestData(data, length);
        return;
    }
    
    ASSERT(handle == m_currentHandle);
    
    ASSERT(m_currentResource);
    m_currentResource->data()->append(data, length);

    m_loadedSize += length;
}

void ApplicationCacheGroup::didFinishLoading(ResourceHandle* handle, double finishTime)
{
#if ENABLE(INSPECTOR)
    InspectorInstrumentation::didFinishLoading(m_frame, m_currentResourceIdentifier, finishTime);
#endif

    if (handle == m_manifestHandle) {
        didFinishLoadingManifest();
        return;
    }

    // After finishing the loading of any resource, we check if it will
    // fit in our last known quota limit.
    if (m_availableSpaceInQuota == ApplicationCacheStorage::unknownQuota()) {
        // Failed to determine what is left in the quota. Fallback to allowing anything.
        if (!cacheStorage().remainingSizeForOriginExcludingCache(m_origin.get(), m_newestCache.get(), m_availableSpaceInQuota))
            m_availableSpaceInQuota = ApplicationCacheStorage::noQuota();
    }

    // Check each resource, as it loads, to see if it would fit in our
    // idea of the available quota space.
    if (m_availableSpaceInQuota < m_loadedSize) {
        m_currentResource = 0;
        cacheUpdateFailedDueToOriginQuota();
        return;
    }

    ASSERT(m_currentHandle == handle);
    ASSERT(m_pendingEntries.contains(handle->firstRequest().url()));
    
    m_pendingEntries.remove(handle->firstRequest().url());
    
    ASSERT(m_cacheBeingUpdated);

    m_cacheBeingUpdated->addResource(m_currentResource.release());
    m_currentHandle = 0;
    
    // Load the next resource, if any.
    startLoadingEntry();
}

void ApplicationCacheGroup::didFail(ResourceHandle* handle, const ResourceError& error)
{
#if ENABLE(INSPECTOR)
    InspectorInstrumentation::didFailLoading(m_frame, m_currentResourceIdentifier, error);
#endif

    if (handle == m_manifestHandle) {
        cacheUpdateFailed();
        return;
    }

    unsigned type = m_currentResource ? m_currentResource->type() : m_pendingEntries.get(handle->firstRequest().url());
    KURL url(handle->firstRequest().url());
    if (url.hasFragmentIdentifier())
        url.removeFragmentIdentifier();

    ASSERT(!m_currentResource || !m_pendingEntries.contains(url));
    m_currentResource = 0;
    m_pendingEntries.remove(url);

    if ((type & ApplicationCacheResource::Explicit) || (type & ApplicationCacheResource::Fallback)) {
        // Note that cacheUpdateFailed() can cause the cache group to be deleted.
        cacheUpdateFailed();
    } else {
        // Copy the resource and its metadata from the newest application cache in cache group whose completeness flag is complete, and act
        // as if that was the fetched resource, ignoring the resource obtained from the network.
        ASSERT(m_newestCache);
        ApplicationCacheResource* newestCachedResource = m_newestCache->resourceForURL(url);
        ASSERT(newestCachedResource);
        m_cacheBeingUpdated->addResource(ApplicationCacheResource::create(url, newestCachedResource->response(), type, newestCachedResource->data(), newestCachedResource->path()));
        // Load the next resource, if any.
        startLoadingEntry();
    }
}

void ApplicationCacheGroup::didReceiveManifestResponse(const ResourceResponse& response)
{
    ASSERT(!m_manifestResource);
    ASSERT(m_manifestHandle);

    if (response.httpStatusCode() == 404 || response.httpStatusCode() == 410) {
        manifestNotFound();
        return;
    }

    if (response.httpStatusCode() == 304)
        return;

    if (response.httpStatusCode() / 100 != 2 || response.url() != m_manifestHandle->firstRequest().url() || !equalIgnoringCase(response.mimeType(), "text/cache-manifest")) {
        cacheUpdateFailed();
        return;
    }

    m_manifestResource = ApplicationCacheResource::create(m_manifestHandle->firstRequest().url(), response, ApplicationCacheResource::Manifest);
}

void ApplicationCacheGroup::didReceiveManifestData(const char* data, int length)
{
    if (m_manifestResource)
        m_manifestResource->data()->append(data, length);
}

void ApplicationCacheGroup::didFinishLoadingManifest()
{
    bool isUpgradeAttempt = m_newestCache;

    if (!isUpgradeAttempt && !m_manifestResource) {
        // The server returned 304 Not Modified even though we didn't send a conditional request.
        cacheUpdateFailed();
        return;
    }

    m_manifestHandle = 0;

    // Check if the manifest was not modified.
    if (isUpgradeAttempt) {
        ApplicationCacheResource* newestManifest = m_newestCache->manifestResource();
        ASSERT(newestManifest);
    
        if (!m_manifestResource || // The resource will be null if HTTP response was 304 Not Modified.
            (newestManifest->data()->size() == m_manifestResource->data()->size() && !memcmp(newestManifest->data()->data(), m_manifestResource->data()->data(), newestManifest->data()->size()))) {

            m_completionType = NoUpdate;
            m_manifestResource = 0;
            deliverDelayedMainResources();

            return;
        }
    }
    
    Manifest manifest;
    if (!parseManifest(m_manifestURL, m_manifestResource->data()->data(), m_manifestResource->data()->size(), manifest)) {
        cacheUpdateFailed();
        return;
    }

    ASSERT(!m_cacheBeingUpdated);
    m_cacheBeingUpdated = ApplicationCache::create();
    m_cacheBeingUpdated->setGroup(this);

    HashSet<DocumentLoader*>::const_iterator masterEnd = m_pendingMasterResourceLoaders.end();
    for (HashSet<DocumentLoader*>::const_iterator iter = m_pendingMasterResourceLoaders.begin(); iter != masterEnd; ++iter)
        associateDocumentLoaderWithCache(*iter, m_cacheBeingUpdated.get());

    // We have the manifest, now download the resources.
    setUpdateStatus(Downloading);
    
    postListenerTask(ApplicationCacheHost::DOWNLOADING_EVENT, m_associatedDocumentLoaders);

    ASSERT(m_pendingEntries.isEmpty());

    if (isUpgradeAttempt) {
        ApplicationCache::ResourceMap::const_iterator end = m_newestCache->end();
        for (ApplicationCache::ResourceMap::const_iterator it = m_newestCache->begin(); it != end; ++it) {
            unsigned type = it->second->type();
            if (type & ApplicationCacheResource::Master)
                addEntry(it->first, type);
        }
    }
    
    HashSet<String>::const_iterator end = manifest.explicitURLs.end();
    for (HashSet<String>::const_iterator it = manifest.explicitURLs.begin(); it != end; ++it)
        addEntry(*it, ApplicationCacheResource::Explicit);

    size_t fallbackCount = manifest.fallbackURLs.size();
    for (size_t i = 0; i  < fallbackCount; ++i)
        addEntry(manifest.fallbackURLs[i].second, ApplicationCacheResource::Fallback);
    
    m_cacheBeingUpdated->setOnlineWhitelist(manifest.onlineWhitelistedURLs);
    m_cacheBeingUpdated->setFallbackURLs(manifest.fallbackURLs);
    m_cacheBeingUpdated->setAllowsAllNetworkRequests(manifest.allowAllNetworkRequests);

    m_progressTotal = m_pendingEntries.size();
    m_progressDone = 0;

    startLoadingEntry();
}

void ApplicationCacheGroup::didReachMaxAppCacheSize()
{
    ASSERT(m_frame);
    ASSERT(m_cacheBeingUpdated);
    m_frame->page()->chrome()->client()->reachedMaxAppCacheSize(cacheStorage().spaceNeeded(m_cacheBeingUpdated->estimatedSizeInStorage()));
    m_calledReachedMaxAppCacheSize = true;
    checkIfLoadIsComplete();
}

void ApplicationCacheGroup::didReachOriginQuota(PassRefPtr<Frame> frame)
{
    // Inform the client the origin quota has been reached,
    // they may decide to increase the quota.
    frame->page()->chrome()->client()->reachedApplicationCacheOriginQuota(m_origin.get());
}

void ApplicationCacheGroup::cacheUpdateFailed()
{
    stopLoading();
    m_manifestResource = 0;

    // Wait for master resource loads to finish.
    m_completionType = Failure;
    deliverDelayedMainResources();
}

void ApplicationCacheGroup::cacheUpdateFailedDueToOriginQuota()
{
    if (!m_originQuotaReached) {
        m_originQuotaReached = true;
        scheduleReachedOriginQuotaCallback();
    }

    cacheUpdateFailed();
}
    
void ApplicationCacheGroup::manifestNotFound()
{
    makeObsolete();

    postListenerTask(ApplicationCacheHost::OBSOLETE_EVENT, m_associatedDocumentLoaders);
    postListenerTask(ApplicationCacheHost::ERROR_EVENT, m_pendingMasterResourceLoaders);

    stopLoading();

    ASSERT(m_pendingEntries.isEmpty());
    m_manifestResource = 0;

    while (!m_pendingMasterResourceLoaders.isEmpty()) {
        HashSet<DocumentLoader*>::iterator it = m_pendingMasterResourceLoaders.begin();
        
        ASSERT((*it)->applicationCacheHost()->candidateApplicationCacheGroup() == this);
        ASSERT(!(*it)->applicationCacheHost()->applicationCache());
        (*it)->applicationCacheHost()->setCandidateApplicationCacheGroup(0);
        m_pendingMasterResourceLoaders.remove(it);
    }

    m_downloadingPendingMasterResourceLoadersCount = 0;
    setUpdateStatus(Idle);    
    m_frame = 0;
    
    if (m_caches.isEmpty()) {
        ASSERT(m_associatedDocumentLoaders.isEmpty());
        ASSERT(!m_cacheBeingUpdated);
        delete this;
    }
}

void ApplicationCacheGroup::checkIfLoadIsComplete()
{
    if (m_manifestHandle || !m_pendingEntries.isEmpty() || m_downloadingPendingMasterResourceLoadersCount)
        return;
    
    // We're done, all resources have finished downloading (successfully or not).

    bool isUpgradeAttempt = m_newestCache;

    switch (m_completionType) {
    case None:
        ASSERT_NOT_REACHED();
        return;
    case NoUpdate:
        ASSERT(isUpgradeAttempt);
        ASSERT(!m_cacheBeingUpdated);

        // The storage could have been manually emptied by the user.
        if (!m_storageID)
            cacheStorage().storeNewestCache(this);

        postListenerTask(ApplicationCacheHost::NOUPDATE_EVENT, m_associatedDocumentLoaders);
        break;
    case Failure:
        ASSERT(!m_cacheBeingUpdated);
        postListenerTask(ApplicationCacheHost::ERROR_EVENT, m_associatedDocumentLoaders);
        if (m_caches.isEmpty()) {
            ASSERT(m_associatedDocumentLoaders.isEmpty());
            delete this;
            return;
        }
        break;
    case Completed: {
        // FIXME: Fetch the resource from manifest URL again, and check whether it is identical to the one used for update (in case the application was upgraded server-side in the meanwhile). (<rdar://problem/6467625>)

        ASSERT(m_cacheBeingUpdated);
        if (m_manifestResource)
            m_cacheBeingUpdated->setManifestResource(m_manifestResource.release());
        else {
            // We can get here as a result of retrying the Complete step, following
            // a failure of the cache storage to save the newest cache due to hitting
            // the maximum size. In such a case, m_manifestResource may be 0, as
            // the manifest was already set on the newest cache object.
            ASSERT(cacheStorage().isMaximumSizeReached() && m_calledReachedMaxAppCacheSize);
        }

        ApplicationCacheStorage::FailureReason failureReason;
        RefPtr<ApplicationCache> oldNewestCache = (m_newestCache == m_cacheBeingUpdated) ? RefPtr<ApplicationCache>() : m_newestCache;
        setNewestCache(m_cacheBeingUpdated.release());
        if (cacheStorage().storeNewestCache(this, oldNewestCache.get(), failureReason)) {
            // New cache stored, now remove the old cache.
            if (oldNewestCache)
                cacheStorage().remove(oldNewestCache.get());

            // Fire the final progress event.
            ASSERT(m_progressDone == m_progressTotal);
            postListenerTask(ApplicationCacheHost::PROGRESS_EVENT, m_progressTotal, m_progressDone, m_associatedDocumentLoaders);

            // Fire the success event.
            postListenerTask(isUpgradeAttempt ? ApplicationCacheHost::UPDATEREADY_EVENT : ApplicationCacheHost::CACHED_EVENT, m_associatedDocumentLoaders);
            // It is clear that the origin quota was not reached, so clear the flag if it was set.
            m_originQuotaReached = false;
        } else {
            if (failureReason == ApplicationCacheStorage::OriginQuotaReached) {
                // We ran out of space for this origin. Roll back to previous state.
                if (oldNewestCache)
                    setNewestCache(oldNewestCache.release());
                cacheUpdateFailedDueToOriginQuota();
                return;
            }

            if (failureReason == ApplicationCacheStorage::TotalQuotaReached && !m_calledReachedMaxAppCacheSize) {
                // We ran out of space. All the changes in the cache storage have
                // been rolled back. We roll back to the previous state in here,
                // as well, call the chrome client asynchronously and retry to
                // save the new cache.

                // Save a reference to the new cache.
                m_cacheBeingUpdated = m_newestCache.release();
                if (oldNewestCache) {
                    // Reinstate the oldNewestCache.
                    setNewestCache(oldNewestCache.release());
                }
                scheduleReachedMaxAppCacheSizeCallback();
                return;
            }

            // Run the "cache failure steps"
            // Fire the error events to all pending master entries, as well any other cache hosts
            // currently associated with a cache in this group.
            postListenerTask(ApplicationCacheHost::ERROR_EVENT, m_associatedDocumentLoaders);
            // Disassociate the pending master entries from the failed new cache. Note that
            // all other loaders in the m_associatedDocumentLoaders are still associated with
            // some other cache in this group. They are not associated with the failed new cache.

            // Need to copy loaders, because the cache group may be destroyed at the end of iteration.
            Vector<DocumentLoader*> loaders;
            copyToVector(m_pendingMasterResourceLoaders, loaders);
            size_t count = loaders.size();
            for (size_t i = 0; i != count; ++i)
                disassociateDocumentLoader(loaders[i]); // This can delete this group.

            // Reinstate the oldNewestCache, if there was one.
            if (oldNewestCache) {
                // This will discard the failed new cache.
                setNewestCache(oldNewestCache.release());
            } else {
                // We must have been deleted by the last call to disassociateDocumentLoader().
                return;
            }
        }
        break;
    }
    }

    // Empty cache group's list of pending master entries.
    m_pendingMasterResourceLoaders.clear();
    m_completionType = None;
    setUpdateStatus(Idle);
    m_frame = 0;
    m_loadedSize = 0;
    m_availableSpaceInQuota = ApplicationCacheStorage::unknownQuota();
    m_calledReachedMaxAppCacheSize = false;
}

void ApplicationCacheGroup::startLoadingEntry()
{
    ASSERT(m_cacheBeingUpdated);

    if (m_pendingEntries.isEmpty()) {
        m_completionType = Completed;
        deliverDelayedMainResources();
        return;
    }
    
    EntryMap::const_iterator it = m_pendingEntries.begin();

    postListenerTask(ApplicationCacheHost::PROGRESS_EVENT, m_progressTotal, m_progressDone, m_associatedDocumentLoaders);
    m_progressDone++;

    ASSERT(!m_currentHandle);
    
    m_currentHandle = createResourceHandle(KURL(ParsedURLString, it->first), m_newestCache ? m_newestCache->resourceForURL(it->first) : 0);
}

void ApplicationCacheGroup::deliverDelayedMainResources()
{
    // Need to copy loaders, because the cache group may be destroyed at the end of iteration.
    Vector<DocumentLoader*> loaders;
    copyToVector(m_pendingMasterResourceLoaders, loaders);
    size_t count = loaders.size();
    for (size_t i = 0; i != count; ++i) {
        DocumentLoader* loader = loaders[i];
        if (loader->isLoadingMainResource())
            continue;

        const ResourceError& error = loader->mainDocumentError();
        if (error.isNull())
            finishedLoadingMainResource(loader);
        else
            failedLoadingMainResource(loader);
    }
    if (!count)
        checkIfLoadIsComplete();
}

void ApplicationCacheGroup::addEntry(const String& url, unsigned type)
{
    ASSERT(m_cacheBeingUpdated);
    ASSERT(!KURL(ParsedURLString, url).hasFragmentIdentifier());
    
    // Don't add the URL if we already have an master resource in the cache
    // (i.e., the main resource finished loading before the manifest).
    if (ApplicationCacheResource* resource = m_cacheBeingUpdated->resourceForURL(url)) {
        ASSERT(resource->type() & ApplicationCacheResource::Master);
        ASSERT(!m_frame->loader()->documentLoader()->isLoadingMainResource());
    
        resource->addType(type);
        return;
    }

    // Don't add the URL if it's the same as the manifest URL.
    ASSERT(m_manifestResource);
    if (m_manifestResource->url() == url) {
        m_manifestResource->addType(type);
        return;
    }
    
    pair<EntryMap::iterator, bool> result = m_pendingEntries.add(url, type);
    
    if (!result.second)
        result.first->second |= type;
}

void ApplicationCacheGroup::associateDocumentLoaderWithCache(DocumentLoader* loader, ApplicationCache* cache)
{
    // If teardown started already, revive the group.
    if (!m_newestCache && !m_cacheBeingUpdated)
        m_newestCache = cache;

    ASSERT(!m_isObsolete);

    loader->applicationCacheHost()->setApplicationCache(cache);

    ASSERT(!m_associatedDocumentLoaders.contains(loader));
    m_associatedDocumentLoaders.add(loader);
}

class ChromeClientCallbackTimer: public TimerBase {
public:
    ChromeClientCallbackTimer(ApplicationCacheGroup* cacheGroup)
        : m_cacheGroup(cacheGroup)
    {
    }

private:
    virtual void fired()
    {
        m_cacheGroup->didReachMaxAppCacheSize();
        delete this;
    }
    // Note that there is no need to use a RefPtr here. The ApplicationCacheGroup instance is guaranteed
    // to be alive when the timer fires since invoking the ChromeClient callback is part of its normal
    // update machinery and nothing can yet cause it to get deleted.
    ApplicationCacheGroup* m_cacheGroup;
};

void ApplicationCacheGroup::scheduleReachedMaxAppCacheSizeCallback()
{
    ASSERT(isMainThread());
    ChromeClientCallbackTimer* timer = new ChromeClientCallbackTimer(this);
    timer->startOneShot(0);
    // The timer will delete itself once it fires.
}

void ApplicationCacheGroup::scheduleReachedOriginQuotaCallback()
{
    // FIXME: it might be nice to run this asynchronously, because there is no return value to wait for.
    didReachOriginQuota(m_frame);
}

class CallCacheListenerTask : public ScriptExecutionContext::Task {
public:
    static PassOwnPtr<CallCacheListenerTask> create(PassRefPtr<DocumentLoader> loader, ApplicationCacheHost::EventID eventID, int progressTotal, int progressDone)
    {
        return adoptPtr(new CallCacheListenerTask(loader, eventID, progressTotal, progressDone));
    }

    virtual void performTask(ScriptExecutionContext* context)
    {
        
        ASSERT_UNUSED(context, context->isDocument());
        Frame* frame = m_documentLoader->frame();
        if (!frame)
            return;
    
        ASSERT(frame->loader()->documentLoader() == m_documentLoader.get());

        m_documentLoader->applicationCacheHost()->notifyDOMApplicationCache(m_eventID, m_progressTotal, m_progressDone);
    }

private:
    CallCacheListenerTask(PassRefPtr<DocumentLoader> loader, ApplicationCacheHost::EventID eventID, int progressTotal, int progressDone)
        : m_documentLoader(loader)
        , m_eventID(eventID)
        , m_progressTotal(progressTotal)
        , m_progressDone(progressDone)
    {
    }

    RefPtr<DocumentLoader> m_documentLoader;
    ApplicationCacheHost::EventID m_eventID;
    int m_progressTotal;
    int m_progressDone;
};

void ApplicationCacheGroup::postListenerTask(ApplicationCacheHost::EventID eventID, int progressTotal, int progressDone, const HashSet<DocumentLoader*>& loaderSet)
{
    HashSet<DocumentLoader*>::const_iterator loaderSetEnd = loaderSet.end();
    for (HashSet<DocumentLoader*>::const_iterator iter = loaderSet.begin(); iter != loaderSetEnd; ++iter)
        postListenerTask(eventID, progressTotal, progressDone, *iter);
}

void ApplicationCacheGroup::postListenerTask(ApplicationCacheHost::EventID eventID, int progressTotal, int progressDone, DocumentLoader* loader)
{
    Frame* frame = loader->frame();
    if (!frame)
        return;
    
    ASSERT(frame->loader()->documentLoader() == loader);

    frame->document()->postTask(CallCacheListenerTask::create(loader, eventID, progressTotal, progressDone));
}

void ApplicationCacheGroup::setUpdateStatus(UpdateStatus status)
{
    m_updateStatus = status;
    InspectorInstrumentation::updateApplicationCacheStatus(m_frame);
}

void ApplicationCacheGroup::clearStorageID()
{
    m_storageID = 0;
    
    HashSet<ApplicationCache*>::const_iterator end = m_caches.end();
    for (HashSet<ApplicationCache*>::const_iterator it = m_caches.begin(); it != end; ++it)
        (*it)->clearStorageID();
}


}

#endif // ENABLE(OFFLINE_WEB_APPLICATIONS)
