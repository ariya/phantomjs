/*
    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller (mueller@kde.org)
    Copyright (C) 2002 Waldo Bastian (bastian@kde.org)
    Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
    Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All rights reserved.

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
*/

#include "config.h"
#include "CachedResource.h"

#include "CachedResourceClient.h"
#include "CachedResourceClientWalker.h"
#include "CachedResourceHandle.h"
#include "CachedResourceLoader.h"
#include "CrossOriginAccessControl.h"
#include "Document.h"
#include "DocumentLoader.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "InspectorInstrumentation.h"
#include "KURL.h"
#include "LoaderStrategy.h"
#include "Logging.h"
#include "MemoryCache.h"
#include "PlatformStrategies.h"
#include "PurgeableBuffer.h"
#include "ResourceBuffer.h"
#include "ResourceHandle.h"
#include "ResourceLoadScheduler.h"
#include "SchemeRegistry.h"
#include "SecurityOrigin.h"
#include "SecurityPolicy.h"
#include "SubresourceLoader.h"
#include <wtf/CurrentTime.h>
#include <wtf/MathExtras.h>
#include <wtf/RefCountedLeakCounter.h>
#include <wtf/StdLibExtras.h>
#include <wtf/text/CString.h>
#include <wtf/Vector.h>

using namespace WTF;

namespace WebCore {

// These response headers are not copied from a revalidated response to the
// cached response headers. For compatibility, this list is based on Chromium's
// net/http/http_response_headers.cc.
const char* const headersToIgnoreAfterRevalidation[] = {
    "allow",
    "connection",
    "etag",
    "expires",
    "keep-alive",
    "last-modified"
    "proxy-authenticate",
    "proxy-connection",
    "trailer",
    "transfer-encoding",
    "upgrade",
    "www-authenticate",
    "x-frame-options",
    "x-xss-protection",
};

// Some header prefixes mean "Don't copy this header from a 304 response.".
// Rather than listing all the relevant headers, we can consolidate them into
// this list, also grabbed from Chromium's net/http/http_response_headers.cc.
const char* const headerPrefixesToIgnoreAfterRevalidation[] = {
    "content-",
    "x-content-",
    "x-webkit-"
};

static inline bool shouldUpdateHeaderAfterRevalidation(const AtomicString& header)
{
    for (size_t i = 0; i < WTF_ARRAY_LENGTH(headersToIgnoreAfterRevalidation); i++) {
        if (equalIgnoringCase(header, headersToIgnoreAfterRevalidation[i]))
            return false;
    }
    for (size_t i = 0; i < WTF_ARRAY_LENGTH(headerPrefixesToIgnoreAfterRevalidation); i++) {
        if (header.startsWith(headerPrefixesToIgnoreAfterRevalidation[i], false))
            return false;
    }
    return true;
}

static ResourceLoadPriority defaultPriorityForResourceType(CachedResource::Type type)
{
    switch (type) {
    case CachedResource::MainResource:
        return ResourceLoadPriorityVeryHigh;
    case CachedResource::CSSStyleSheet:
        return ResourceLoadPriorityHigh;
    case CachedResource::Script:
    case CachedResource::FontResource:
    case CachedResource::RawResource:
        return ResourceLoadPriorityMedium;
    case CachedResource::ImageResource:
        return ResourceLoadPriorityLow;
#if ENABLE(XSLT)
    case CachedResource::XSLStyleSheet:
        return ResourceLoadPriorityHigh;
#endif
#if ENABLE(SVG)
    case CachedResource::SVGDocumentResource:
        return ResourceLoadPriorityLow;
#endif
#if ENABLE(LINK_PREFETCH)
    case CachedResource::LinkPrefetch:
        return ResourceLoadPriorityVeryLow;
    case CachedResource::LinkSubresource:
        return ResourceLoadPriorityVeryLow;
#endif
#if ENABLE(VIDEO_TRACK)
    case CachedResource::TextTrackResource:
        return ResourceLoadPriorityLow;
#endif
#if ENABLE(CSS_SHADERS)
    case CachedResource::ShaderResource:
        return ResourceLoadPriorityMedium;
#endif
    }
    ASSERT_NOT_REACHED();
    return ResourceLoadPriorityLow;
}

#if PLATFORM(BLACKBERRY)
static ResourceRequest::TargetType cachedResourceTypeToTargetType(CachedResource::Type type)
{
    switch (type) {
    case CachedResource::MainResource:
        return ResourceRequest::TargetIsMainFrame;
    case CachedResource::CSSStyleSheet:
#if ENABLE(XSLT)
    case CachedResource::XSLStyleSheet:
#endif
        return ResourceRequest::TargetIsStyleSheet;
    case CachedResource::Script: 
        return ResourceRequest::TargetIsScript;
    case CachedResource::FontResource:
        return ResourceRequest::TargetIsFontResource;
    case CachedResource::ImageResource:
        return ResourceRequest::TargetIsImage;
#if ENABLE(CSS_SHADERS)
    case CachedResource::ShaderResource:
#endif
    case CachedResource::RawResource:
        return ResourceRequest::TargetIsSubresource;    
#if ENABLE(LINK_PREFETCH)
    case CachedResource::LinkPrefetch:
        return ResourceRequest::TargetIsPrefetch;
    case CachedResource::LinkSubresource:
        return ResourceRequest::TargetIsSubresource;
#endif
#if ENABLE(VIDEO_TRACK)
    case CachedResource::TextTrackResource:
        return ResourceRequest::TargetIsTextTrack;
#endif
#if ENABLE(SVG)
    case CachedResource::SVGDocumentResource:
        return ResourceRequest::TargetIsImage;
#endif
    }
    ASSERT_NOT_REACHED();
    return ResourceRequest::TargetIsSubresource;
}
#endif

static double deadDecodedDataDeletionIntervalForResourceType(CachedResource::Type type)
{
    if (type == CachedResource::Script)
        return 0;
    return memoryCache()->deadDecodedDataDeletionInterval();
}

DEFINE_DEBUG_ONLY_GLOBAL(RefCountedLeakCounter, cachedResourceLeakCounter, ("CachedResource"));

CachedResource::CachedResource(const ResourceRequest& request, Type type)
    : m_resourceRequest(request)
    , m_loadPriority(defaultPriorityForResourceType(type))
    , m_responseTimestamp(currentTime())
    , m_decodedDataDeletionTimer(this, &CachedResource::decodedDataDeletionTimerFired, deadDecodedDataDeletionIntervalForResourceType(type))
    , m_lastDecodedAccessTime(0)
    , m_loadFinishTime(0)
    , m_encodedSize(0)
    , m_decodedSize(0)
    , m_accessCount(0)
    , m_handleCount(0)
    , m_preloadCount(0)
    , m_preloadResult(PreloadNotReferenced)
    , m_inLiveDecodedResourcesList(false)
    , m_requestedFromNetworkingLayer(false)
    , m_inCache(false)
    , m_loading(false)
    , m_switchingClientsToRevalidatedResource(false)
    , m_type(type)
    , m_status(Pending)
#ifndef NDEBUG
    , m_deleted(false)
    , m_lruIndex(0)
#endif
    , m_nextInAllResourcesList(0)
    , m_prevInAllResourcesList(0)
    , m_nextInLiveResourcesList(0)
    , m_prevInLiveResourcesList(0)
    , m_owningCachedResourceLoader(0)
    , m_resourceToRevalidate(0)
    , m_proxyResource(0)
{
    ASSERT(m_type == unsigned(type)); // m_type is a bitfield, so this tests careless updates of the enum.
#ifndef NDEBUG
    cachedResourceLeakCounter.increment();
#endif

    if (!m_resourceRequest.url().hasFragmentIdentifier())
        return;
    KURL urlForCache = MemoryCache::removeFragmentIdentifierIfNeeded(m_resourceRequest.url());
    if (urlForCache.hasFragmentIdentifier())
        return;
    m_fragmentIdentifierForRequest = m_resourceRequest.url().fragmentIdentifier();
    m_resourceRequest.setURL(urlForCache);
}

CachedResource::~CachedResource()
{
    ASSERT(!m_resourceToRevalidate); // Should be true because canDelete() checks this.
    ASSERT(canDelete());
    ASSERT(!inCache());
    ASSERT(!m_deleted);
    ASSERT(url().isNull() || memoryCache()->resourceForRequest(resourceRequest()) != this);

#ifndef NDEBUG
    m_deleted = true;
    cachedResourceLeakCounter.decrement();
#endif

    if (m_owningCachedResourceLoader)
        m_owningCachedResourceLoader->removeCachedResource(this);
}

void CachedResource::failBeforeStarting()
{
    // FIXME: What if resources in other frames were waiting for this revalidation?
    LOG(ResourceLoading, "Cannot start loading '%s'", url().string().latin1().data());
    if (m_resourceToRevalidate) 
        memoryCache()->revalidationFailed(this); 
    error(CachedResource::LoadError);
}

void CachedResource::addAdditionalRequestHeaders(CachedResourceLoader* cachedResourceLoader)
{
    // Note: We skip the Content-Security-Policy check here because we check
    // the Content-Security-Policy at the CachedResourceLoader layer so we can
    // handle different resource types differently.

    FrameLoader* frameLoader = cachedResourceLoader->frame()->loader();
    String outgoingReferrer;
    String outgoingOrigin;
    if (m_resourceRequest.httpReferrer().isNull()) {
        outgoingReferrer = frameLoader->outgoingReferrer();
        outgoingOrigin = frameLoader->outgoingOrigin();
    } else {
        outgoingReferrer = m_resourceRequest.httpReferrer();
        outgoingOrigin = SecurityOrigin::createFromString(outgoingReferrer)->toString();
    }

    outgoingReferrer = SecurityPolicy::generateReferrerHeader(cachedResourceLoader->document()->referrerPolicy(), m_resourceRequest.url(), outgoingReferrer);
    if (outgoingReferrer.isEmpty())
        m_resourceRequest.clearHTTPReferrer();
    else if (!m_resourceRequest.httpReferrer())
        m_resourceRequest.setHTTPReferrer(outgoingReferrer);
    FrameLoader::addHTTPOriginIfNeeded(m_resourceRequest, outgoingOrigin);

    frameLoader->addExtraFieldsToSubresourceRequest(m_resourceRequest);
}

void CachedResource::load(CachedResourceLoader* cachedResourceLoader, const ResourceLoaderOptions& options)
{
    if (!cachedResourceLoader->frame()) {
        failBeforeStarting();
        return;
    }

    FrameLoader* frameLoader = cachedResourceLoader->frame()->loader();
    if (options.securityCheck == DoSecurityCheck && (frameLoader->state() == FrameStateProvisional || !frameLoader->activeDocumentLoader() || frameLoader->activeDocumentLoader()->isStopping())) {
        failBeforeStarting();
        return;
    }

    m_options = options;
    m_loading = true;

#if PLATFORM(BLACKBERRY)
    if (m_resourceRequest.targetType() == ResourceRequest::TargetIsUnspecified)
        m_resourceRequest.setTargetType(cachedResourceTypeToTargetType(type()));
#endif

    if (!accept().isEmpty())
        m_resourceRequest.setHTTPAccept(accept());

    if (isCacheValidator()) {
        CachedResource* resourceToRevalidate = m_resourceToRevalidate;
        ASSERT(resourceToRevalidate->canUseCacheValidator());
        ASSERT(resourceToRevalidate->isLoaded());
        const String& lastModified = resourceToRevalidate->response().httpHeaderField("Last-Modified");
        const String& eTag = resourceToRevalidate->response().httpHeaderField("ETag");
        if (!lastModified.isEmpty() || !eTag.isEmpty()) {
            ASSERT(cachedResourceLoader->cachePolicy(type()) != CachePolicyReload);
            if (cachedResourceLoader->cachePolicy(type()) == CachePolicyRevalidate)
                m_resourceRequest.setHTTPHeaderField("Cache-Control", "max-age=0");
            if (!lastModified.isEmpty())
                m_resourceRequest.setHTTPHeaderField("If-Modified-Since", lastModified);
            if (!eTag.isEmpty())
                m_resourceRequest.setHTTPHeaderField("If-None-Match", eTag);
        }
    }

#if ENABLE(LINK_PREFETCH)
    if (type() == CachedResource::LinkPrefetch || type() == CachedResource::LinkSubresource)
        m_resourceRequest.setHTTPHeaderField("Purpose", "prefetch");
#endif
    m_resourceRequest.setPriority(loadPriority());

    if (type() != MainResource)
        addAdditionalRequestHeaders(cachedResourceLoader);

    // FIXME: It's unfortunate that the cache layer and below get to know anything about fragment identifiers.
    // We should look into removing the expectation of that knowledge from the platform network stacks.
    ResourceRequest request(m_resourceRequest);
    if (!m_fragmentIdentifierForRequest.isNull()) {
        KURL url = request.url();
        url.setFragmentIdentifier(m_fragmentIdentifierForRequest);
        request.setURL(url);
        m_fragmentIdentifierForRequest = String();
    }

    m_loader = platformStrategies()->loaderStrategy()->resourceLoadScheduler()->scheduleSubresourceLoad(cachedResourceLoader->frame(), this, request, request.priority(), options);
    if (!m_loader) {
        failBeforeStarting();
        return;
    }

    m_status = Pending;
}

void CachedResource::checkNotify()
{
    if (isLoading() || stillNeedsLoad())
        return;

    CachedResourceClientWalker<CachedResourceClient> w(m_clients);
    while (CachedResourceClient* c = w.next())
        c->notifyFinished(this);
}

void CachedResource::addDataBuffer(ResourceBuffer*)
{
    ASSERT(m_options.dataBufferingPolicy == BufferData);
}

void CachedResource::addData(const char*, unsigned)
{
    ASSERT(m_options.dataBufferingPolicy == DoNotBufferData);
}

void CachedResource::finishLoading(ResourceBuffer*)
{
    setLoading(false);
    checkNotify();
}

void CachedResource::error(CachedResource::Status status)
{
    setStatus(status);
    ASSERT(errorOccurred());
    m_data.clear();

    setLoading(false);
    checkNotify();
}
    
void CachedResource::cancelLoad()
{
    if (!isLoading())
        return;

    setStatus(LoadError);
    setLoading(false);
    checkNotify();
}

void CachedResource::finish()
{
    if (!errorOccurred())
        m_status = Cached;
}

bool CachedResource::passesAccessControlCheck(SecurityOrigin* securityOrigin)
{
    String errorDescription;
    return WebCore::passesAccessControlCheck(m_response, resourceRequest().allowCookies() ? AllowStoredCredentials : DoNotAllowStoredCredentials, securityOrigin, errorDescription);
}

bool CachedResource::isExpired() const
{
    if (m_response.isNull())
        return false;

    return currentAge() > freshnessLifetime();
}

double CachedResource::currentAge() const
{
    // RFC2616 13.2.3
    // No compensation for latency as that is not terribly important in practice
    double dateValue = m_response.date();
    double apparentAge = std::isfinite(dateValue) ? std::max(0., m_responseTimestamp - dateValue) : 0;
    double ageValue = m_response.age();
    double correctedReceivedAge = std::isfinite(ageValue) ? std::max(apparentAge, ageValue) : apparentAge;
    double residentTime = currentTime() - m_responseTimestamp;
    return correctedReceivedAge + residentTime;
}

double CachedResource::freshnessLifetime() const
{
    if (!m_response.url().protocolIsInHTTPFamily()) {
        // Don't cache non-HTTP main resources since we can't check for freshness.
        // FIXME: We should not cache subresources either, but when we tried this
        // it caused performance and flakiness issues in our test infrastructure.
        if (m_type == MainResource && !SchemeRegistry::shouldCacheResponsesFromURLSchemeIndefinitely(m_response.url().protocol()))
            return 0;

        return std::numeric_limits<double>::max();
    }

    // RFC2616 13.2.4
    double maxAgeValue = m_response.cacheControlMaxAge();
    if (std::isfinite(maxAgeValue))
        return maxAgeValue;
    double expiresValue = m_response.expires();
    double dateValue = m_response.date();
    double creationTime = std::isfinite(dateValue) ? dateValue : m_responseTimestamp;
    if (std::isfinite(expiresValue))
        return expiresValue - creationTime;
    double lastModifiedValue = m_response.lastModified();
    if (std::isfinite(lastModifiedValue))
        return (creationTime - lastModifiedValue) * 0.1;
    // If no cache headers are present, the specification leaves the decision to the UA. Other browsers seem to opt for 0.
    return 0;
}

void CachedResource::responseReceived(const ResourceResponse& response)
{
    setResponse(response);
    m_responseTimestamp = currentTime();
    String encoding = response.textEncodingName();
    if (!encoding.isNull())
        setEncoding(encoding);
}

void CachedResource::clearLoader()
{
    ASSERT(m_loader);
    m_loader = 0;
}

void CachedResource::addClient(CachedResourceClient* client)
{
    if (addClientToSet(client))
        didAddClient(client);
}

void CachedResource::didAddClient(CachedResourceClient* c)
{
    if (m_decodedDataDeletionTimer.isActive())
        m_decodedDataDeletionTimer.stop();

    if (m_clientsAwaitingCallback.contains(c)) {
        m_clients.add(c);
        m_clientsAwaitingCallback.remove(c);
    }
    if (!isLoading() && !stillNeedsLoad())
        c->notifyFinished(this);
}

bool CachedResource::addClientToSet(CachedResourceClient* client)
{
    ASSERT(!isPurgeable());

    if (m_preloadResult == PreloadNotReferenced) {
        if (isLoaded())
            m_preloadResult = PreloadReferencedWhileComplete;
        else if (m_requestedFromNetworkingLayer)
            m_preloadResult = PreloadReferencedWhileLoading;
        else
            m_preloadResult = PreloadReferenced;
    }
    if (!hasClients() && inCache())
        memoryCache()->addToLiveResourcesSize(this);

    if (((m_type == RawResource || m_type == MainResource) && !m_response.isNull() && !m_proxyResource)
        || (m_proxyResource && m_proxyResource->m_switchingClientsToRevalidatedResource)) {
        // Certain resources (especially XHRs and main resources) do crazy things if an asynchronous load returns
        // synchronously (e.g., scripts may not have set all the state they need to handle the load).
        // Therefore, rather than immediately sending callbacks on a cache hit like other CachedResources,
        // we schedule the callbacks and ensure we never finish synchronously.
        // Revalidated sources are handled async to avoid loaded scripts from changing the our
        // data-structures while we are still processing them.
        ASSERT(!m_clientsAwaitingCallback.contains(client));
        m_clientsAwaitingCallback.add(client, CachedResourceCallback::schedule(this, client));
        return false;
    }

    m_clients.add(client);
    return true;
}

void CachedResource::removeClient(CachedResourceClient* client)
{
    OwnPtr<CachedResourceCallback> callback = m_clientsAwaitingCallback.take(client);
    if (callback) {
        ASSERT(!m_clients.contains(client));
        callback->cancel();
        callback.clear();
    } else {
        ASSERT(m_clients.contains(client));
        m_clients.remove(client);
        didRemoveClient(client);
    }

    bool deleted = deleteIfPossible();
    if (!deleted && !hasClients()) {
        if (inCache()) {
            memoryCache()->removeFromLiveResourcesSize(this);
            memoryCache()->removeFromLiveDecodedResourcesList(this);
        }
        if (!m_switchingClientsToRevalidatedResource)
            allClientsRemoved();
        destroyDecodedDataIfNeeded();
        if (response().cacheControlContainsNoStore()) {
            // RFC2616 14.9.2:
            // "no-store: ... MUST make a best-effort attempt to remove the information from volatile storage as promptly as possible"
            // "... History buffers MAY store such responses as part of their normal operation."
            // We allow non-secure content to be reused in history, but we do not allow secure content to be reused.
            if (url().protocolIs("https"))
                memoryCache()->remove(this);
        } else
            memoryCache()->prune();
    }
    // This object may be dead here.
}

void CachedResource::destroyDecodedDataIfNeeded()
{
    if (!m_decodedSize)
        return;
    if (!memoryCache()->deadDecodedDataDeletionInterval())
        return;
    m_decodedDataDeletionTimer.restart();
}

void CachedResource::decodedDataDeletionTimerFired(DeferrableOneShotTimer<CachedResource>*)
{
    destroyDecodedData();
}

bool CachedResource::deleteIfPossible()
{
    if (canDelete() && !inCache()) {
        InspectorInstrumentation::willDestroyCachedResource(this);
        delete this;
        return true;
    }
    return false;
}

void CachedResource::setDecodedSize(unsigned size)
{
    if (size == m_decodedSize)
        return;

    int delta = size - m_decodedSize;

    // The object must now be moved to a different queue, since its size has been changed.
    // We have to remove explicitly before updating m_decodedSize, so that we find the correct previous
    // queue.
    if (inCache())
        memoryCache()->removeFromLRUList(this);
    
    m_decodedSize = size;
   
    if (inCache()) { 
        // Now insert into the new LRU list.
        memoryCache()->insertInLRUList(this);
        
        // Insert into or remove from the live decoded list if necessary.
        // When inserting into the LiveDecodedResourcesList it is possible
        // that the m_lastDecodedAccessTime is still zero or smaller than
        // the m_lastDecodedAccessTime of the current list head. This is a
        // violation of the invariant that the list is to be kept sorted
        // by access time. The weakening of the invariant does not pose
        // a problem. For more details please see: https://bugs.webkit.org/show_bug.cgi?id=30209
        if (m_decodedSize && !m_inLiveDecodedResourcesList && hasClients())
            memoryCache()->insertInLiveDecodedResourcesList(this);
        else if (!m_decodedSize && m_inLiveDecodedResourcesList)
            memoryCache()->removeFromLiveDecodedResourcesList(this);

        // Update the cache's size totals.
        memoryCache()->adjustSize(hasClients(), delta);
    }
}

void CachedResource::setEncodedSize(unsigned size)
{
    if (size == m_encodedSize)
        return;

    // The size cannot ever shrink (unless it is being nulled out because of an error).  If it ever does, assert.
    ASSERT(size == 0 || size >= m_encodedSize);
    
    int delta = size - m_encodedSize;

    // The object must now be moved to a different queue, since its size has been changed.
    // We have to remove explicitly before updating m_encodedSize, so that we find the correct previous
    // queue.
    if (inCache())
        memoryCache()->removeFromLRUList(this);
    
    m_encodedSize = size;
   
    if (inCache()) { 
        // Now insert into the new LRU list.
        memoryCache()->insertInLRUList(this);
        
        // Update the cache's size totals.
        memoryCache()->adjustSize(hasClients(), delta);
    }
}

void CachedResource::didAccessDecodedData(double timeStamp)
{
    m_lastDecodedAccessTime = timeStamp;
    
    if (inCache()) {
        if (m_inLiveDecodedResourcesList) {
            memoryCache()->removeFromLiveDecodedResourcesList(this);
            memoryCache()->insertInLiveDecodedResourcesList(this);
        }
        memoryCache()->prune();
    }
}
    
void CachedResource::setResourceToRevalidate(CachedResource* resource) 
{ 
    ASSERT(resource);
    ASSERT(!m_resourceToRevalidate);
    ASSERT(resource != this);
    ASSERT(m_handlesToRevalidate.isEmpty());
    ASSERT(resource->type() == type());

    LOG(ResourceLoading, "CachedResource %p setResourceToRevalidate %p", this, resource);

    // The following assert should be investigated whenever it occurs. Although it should never fire, it currently does in rare circumstances.
    // https://bugs.webkit.org/show_bug.cgi?id=28604.
    // So the code needs to be robust to this assert failing thus the "if (m_resourceToRevalidate->m_proxyResource == this)" in CachedResource::clearResourceToRevalidate.
    ASSERT(!resource->m_proxyResource);

    resource->m_proxyResource = this;
    m_resourceToRevalidate = resource;
}

void CachedResource::clearResourceToRevalidate() 
{ 
    ASSERT(m_resourceToRevalidate);
    if (m_switchingClientsToRevalidatedResource)
        return;

    // A resource may start revalidation before this method has been called, so check that this resource is still the proxy resource before clearing it out.
    ASSERT(m_resourceToRevalidate->m_proxyResource == this);
    if (m_resourceToRevalidate->m_proxyResource == this) {
        m_resourceToRevalidate->m_proxyResource = 0;
        m_resourceToRevalidate->deleteIfPossible();
    }
    m_handlesToRevalidate.clear();
    m_resourceToRevalidate = 0;
    deleteIfPossible();
}
    
void CachedResource::switchClientsToRevalidatedResource()
{
    ASSERT(m_resourceToRevalidate);
    ASSERT(m_resourceToRevalidate->inCache());
    ASSERT(!inCache());

    LOG(ResourceLoading, "CachedResource %p switchClientsToRevalidatedResource %p", this, m_resourceToRevalidate);

    m_switchingClientsToRevalidatedResource = true;
    HashSet<CachedResourceHandleBase*>::iterator end = m_handlesToRevalidate.end();
    for (HashSet<CachedResourceHandleBase*>::iterator it = m_handlesToRevalidate.begin(); it != end; ++it) {
        CachedResourceHandleBase* handle = *it;
        handle->m_resource = m_resourceToRevalidate;
        m_resourceToRevalidate->registerHandle(handle);
        --m_handleCount;
    }
    ASSERT(!m_handleCount);
    m_handlesToRevalidate.clear();

    Vector<CachedResourceClient*> clientsToMove;
    HashCountedSet<CachedResourceClient*>::iterator end2 = m_clients.end();
    for (HashCountedSet<CachedResourceClient*>::iterator it = m_clients.begin(); it != end2; ++it) {
        CachedResourceClient* client = it->key;
        unsigned count = it->value;
        while (count) {
            clientsToMove.append(client);
            --count;
        }
    }

    unsigned moveCount = clientsToMove.size();
    for (unsigned n = 0; n < moveCount; ++n)
        removeClient(clientsToMove[n]);
    ASSERT(m_clients.isEmpty());

    for (unsigned n = 0; n < moveCount; ++n)
        m_resourceToRevalidate->addClientToSet(clientsToMove[n]);

    m_switchingClientsToRevalidatedResource = false;
}

void CachedResource::updateResponseAfterRevalidation(const ResourceResponse& validatingResponse)
{
    m_responseTimestamp = currentTime();

    // RFC2616 10.3.5
    // Update cached headers from the 304 response
    const HTTPHeaderMap& newHeaders = validatingResponse.httpHeaderFields();
    HTTPHeaderMap::const_iterator end = newHeaders.end();
    for (HTTPHeaderMap::const_iterator it = newHeaders.begin(); it != end; ++it) {
        // Entity headers should not be sent by servers when generating a 304
        // response; misconfigured servers send them anyway. We shouldn't allow
        // such headers to update the original request. We'll base this on the
        // list defined by RFC2616 7.1, with a few additions for extension headers
        // we care about.
        if (!shouldUpdateHeaderAfterRevalidation(it->key))
            continue;
        m_response.setHTTPHeaderField(it->key, it->value);
    }
}

void CachedResource::registerHandle(CachedResourceHandleBase* h)
{
    ++m_handleCount;
    if (m_resourceToRevalidate)
        m_handlesToRevalidate.add(h);
}

void CachedResource::unregisterHandle(CachedResourceHandleBase* h)
{
    ASSERT(m_handleCount > 0);
    --m_handleCount;

    if (m_resourceToRevalidate)
         m_handlesToRevalidate.remove(h);

    if (!m_handleCount)
        deleteIfPossible();
}

bool CachedResource::canUseCacheValidator() const
{
    if (m_loading || errorOccurred())
        return false;

    if (m_response.cacheControlContainsNoStore())
        return false;
    return m_response.hasCacheValidatorFields();
}

bool CachedResource::mustRevalidateDueToCacheHeaders(CachePolicy cachePolicy) const
{    
    ASSERT(cachePolicy == CachePolicyRevalidate || cachePolicy == CachePolicyCache || cachePolicy == CachePolicyVerify);

    if (cachePolicy == CachePolicyRevalidate)
        return true;

    if (m_response.cacheControlContainsNoCache() || m_response.cacheControlContainsNoStore()) {
        LOG(ResourceLoading, "CachedResource %p mustRevalidate because of m_response.cacheControlContainsNoCache() || m_response.cacheControlContainsNoStore()\n", this);
        return true;
    }

    if (cachePolicy == CachePolicyCache) {
        if (m_response.cacheControlContainsMustRevalidate() && isExpired()) {
            LOG(ResourceLoading, "CachedResource %p mustRevalidate because of cachePolicy == CachePolicyCache and m_response.cacheControlContainsMustRevalidate() && isExpired()\n", this);
            return true;
        }
        return false;
    }

    // CachePolicyVerify
    if (isExpired()) {
        LOG(ResourceLoading, "CachedResource %p mustRevalidate because of isExpired()\n", this);
        return true;
    }

    return false;
}

bool CachedResource::isSafeToMakePurgeable() const
{ 
    return !hasClients() && !m_proxyResource && !m_resourceToRevalidate;
}

bool CachedResource::makePurgeable(bool purgeable) 
{ 
    if (purgeable) {
        ASSERT(isSafeToMakePurgeable());

        if (m_purgeableData) {
            ASSERT(!m_data);
            return true;
        }
        if (!m_data)
            return false;
        
        // Should not make buffer purgeable if it has refs other than this since we don't want two copies.
        if (!m_data->hasOneRef())
            return false;

        m_data->createPurgeableBuffer();
        if (!m_data->hasPurgeableBuffer())
            return false;

        m_purgeableData = m_data->releasePurgeableBuffer();
        m_purgeableData->setPurgePriority(purgePriority());
        m_purgeableData->makePurgeable(true);
        m_data.clear();
        return true;
    }

    if (!m_purgeableData)
        return true;
    ASSERT(!m_data);
    ASSERT(!hasClients());

    if (!m_purgeableData->makePurgeable(false))
        return false; 

    m_data = ResourceBuffer::adoptSharedBuffer(SharedBuffer::adoptPurgeableBuffer(m_purgeableData.release()));
    return true;
}

bool CachedResource::isPurgeable() const
{
    return m_purgeableData && m_purgeableData->isPurgeable();
}

bool CachedResource::wasPurged() const
{
    return m_purgeableData && m_purgeableData->wasPurged();
}

unsigned CachedResource::overheadSize() const
{
    static const int kAverageClientsHashMapSize = 384;
    return sizeof(CachedResource) + m_response.memoryUsage() + kAverageClientsHashMapSize + m_resourceRequest.url().string().length() * 2;
}

void CachedResource::setLoadPriority(ResourceLoadPriority loadPriority)
{
    if (loadPriority == ResourceLoadPriorityUnresolved)
        loadPriority = defaultPriorityForResourceType(type());
    if (loadPriority == m_loadPriority)
        return;
    m_loadPriority = loadPriority;
    if (m_loader)
        m_loader->didChangePriority(loadPriority);
}

CachedResource::CachedResourceCallback::CachedResourceCallback(CachedResource* resource, CachedResourceClient* client)
    : m_resource(resource)
    , m_client(client)
    , m_callbackTimer(this, &CachedResourceCallback::timerFired)
{
    m_callbackTimer.startOneShot(0);
}

void CachedResource::CachedResourceCallback::cancel()
{
    if (m_callbackTimer.isActive())
        m_callbackTimer.stop();
}

void CachedResource::CachedResourceCallback::timerFired(Timer<CachedResourceCallback>*)
{
    m_resource->didAddClient(m_client);
}

#if PLATFORM(MAC)
void CachedResource::tryReplaceEncodedData(PassRefPtr<SharedBuffer> newBuffer)
{
    if (!m_data)
        return;
    
    if (!mayTryReplaceEncodedData())
        return;
    
    // Because the disk cache is asynchronous and racey with regards to the data we might be asked to replace,
    // we need to verify that the new buffer has the same contents as our old buffer.
    if (m_data->size() != newBuffer->size() || memcmp(m_data->data(), newBuffer->data(), m_data->size()))
        return;

    m_data->tryReplaceSharedBufferContents(newBuffer.get());
}
#endif

}
