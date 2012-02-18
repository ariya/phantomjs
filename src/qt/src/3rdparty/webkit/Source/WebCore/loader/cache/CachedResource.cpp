/*
    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller (mueller@kde.org)
    Copyright (C) 2002 Waldo Bastian (bastian@kde.org)
    Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
    Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.

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

#include "MemoryCache.h"
#include "CachedMetadata.h"
#include "CachedResourceClient.h"
#include "CachedResourceClientWalker.h"
#include "CachedResourceHandle.h"
#include "CachedResourceLoader.h"
#include "CachedResourceRequest.h"
#include "Frame.h"
#include "FrameLoaderClient.h"
#include "KURL.h"
#include "Logging.h"
#include "PurgeableBuffer.h"
#include "ResourceHandle.h"
#include "SharedBuffer.h"
#include <wtf/CurrentTime.h>
#include <wtf/MathExtras.h>
#include <wtf/RefCountedLeakCounter.h>
#include <wtf/StdLibExtras.h>
#include <wtf/Vector.h>

using namespace WTF;

namespace WebCore {
    
static ResourceLoadPriority defaultPriorityForResourceType(CachedResource::Type type)
{
    switch (type) {
        case CachedResource::CSSStyleSheet:
#if ENABLE(XSLT)
        case CachedResource::XSLStyleSheet:
#endif
            return ResourceLoadPriorityHigh;
        case CachedResource::Script:
        case CachedResource::FontResource:
            return ResourceLoadPriorityMedium;
        case CachedResource::ImageResource:
            return ResourceLoadPriorityLow;
#if ENABLE(LINK_PREFETCH)
        case CachedResource::LinkResource:
            return ResourceLoadPriorityVeryLow;
#endif
    }
    ASSERT_NOT_REACHED();
    return ResourceLoadPriorityLow;
}

#ifndef NDEBUG
static RefCountedLeakCounter cachedResourceLeakCounter("CachedResource");
#endif

CachedResource::CachedResource(const String& url, Type type)
    : m_url(url)
    , m_request(0)
    , m_loadPriority(defaultPriorityForResourceType(type))
    , m_responseTimestamp(currentTime())
    , m_lastDecodedAccessTime(0)
    , m_encodedSize(0)
    , m_decodedSize(0)
    , m_accessCount(0)
    , m_handleCount(0)
    , m_preloadCount(0)
    , m_preloadResult(PreloadNotReferenced)
    , m_inLiveDecodedResourcesList(false)
    , m_requestedFromNetworkingLayer(false)
    , m_sendResourceLoadCallbacks(true)
    , m_inCache(false)
    , m_loading(false)
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
#ifndef NDEBUG
    cachedResourceLeakCounter.increment();
#endif
}

CachedResource::~CachedResource()
{
    ASSERT(!m_resourceToRevalidate); // Should be true because canDelete() checks this.
    ASSERT(canDelete());
    ASSERT(!inCache());
    ASSERT(!m_deleted);
    ASSERT(url().isNull() || memoryCache()->resourceForURL(KURL(ParsedURLString, url())) != this);
    
#ifndef NDEBUG
    m_deleted = true;
    cachedResourceLeakCounter.decrement();
#endif

    if (m_owningCachedResourceLoader)
        m_owningCachedResourceLoader->removeCachedResource(this);
}

void CachedResource::load(CachedResourceLoader* cachedResourceLoader, bool incremental, SecurityCheckPolicy securityCheck, bool sendResourceLoadCallbacks)
{
    m_sendResourceLoadCallbacks = sendResourceLoadCallbacks;
    cachedResourceLoader->load(this, incremental, securityCheck, sendResourceLoadCallbacks);
    m_loading = true;
}

void CachedResource::checkNotify()
{
    if (isLoading())
        return;

    CachedResourceClientWalker w(m_clients);
    while (CachedResourceClient* c = w.next())
        c->notifyFinished(this);
}

void CachedResource::data(PassRefPtr<SharedBuffer>, bool allDataReceived)
{
    if (!allDataReceived)
        return;
    
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

void CachedResource::finish()
{
    m_status = Cached;
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
    double apparentAge = isfinite(dateValue) ? max(0., m_responseTimestamp - dateValue) : 0;
    double ageValue = m_response.age();
    double correctedReceivedAge = isfinite(ageValue) ? max(apparentAge, ageValue) : apparentAge;
    double residentTime = currentTime() - m_responseTimestamp;
    return correctedReceivedAge + residentTime;
}
    
double CachedResource::freshnessLifetime() const
{
    // Cache non-http resources liberally
    if (!m_response.url().protocolInHTTPFamily())
        return std::numeric_limits<double>::max();

    // RFC2616 13.2.4
    double maxAgeValue = m_response.cacheControlMaxAge();
    if (isfinite(maxAgeValue))
        return maxAgeValue;
    double expiresValue = m_response.expires();
    double dateValue = m_response.date();
    double creationTime = isfinite(dateValue) ? dateValue : m_responseTimestamp;
    if (isfinite(expiresValue))
        return expiresValue - creationTime;
    double lastModifiedValue = m_response.lastModified();
    if (isfinite(lastModifiedValue))
        return (creationTime - lastModifiedValue) * 0.1;
    // If no cache headers are present, the specification leaves the decision to the UA. Other browsers seem to opt for 0.
    return 0;
}

void CachedResource::setResponse(const ResourceResponse& response)
{
    m_response = response;
    m_responseTimestamp = currentTime();
}

void CachedResource::setSerializedCachedMetadata(const char* data, size_t size)
{
    // We only expect to receive cached metadata from the platform once.
    // If this triggers, it indicates an efficiency problem which is most
    // likely unexpected in code designed to improve performance.
    ASSERT(!m_cachedMetadata);

    m_cachedMetadata = CachedMetadata::deserialize(data, size);
}

void CachedResource::setCachedMetadata(unsigned dataTypeID, const char* data, size_t size)
{
    // Currently, only one type of cached metadata per resource is supported.
    // If the need arises for multiple types of metadata per resource this could
    // be enhanced to store types of metadata in a map.
    ASSERT(!m_cachedMetadata);

    m_cachedMetadata = CachedMetadata::create(dataTypeID, data, size);
    ResourceHandle::cacheMetadata(m_response, m_cachedMetadata->serialize());
}

CachedMetadata* CachedResource::cachedMetadata(unsigned dataTypeID) const
{
    if (!m_cachedMetadata || m_cachedMetadata->dataTypeID() != dataTypeID)
        return 0;
    return m_cachedMetadata.get();
}

void CachedResource::setRequest(CachedResourceRequest* request)
{
    if (request && !m_request)
        m_status = Pending;
    m_request = request;

    CachedResourceHandle<CachedResource> protect(this);

    // All loads finish with data(allDataReceived = true) or error(), except for
    // canceled loads, which silently set our request to 0. Be sure to notify our
    // client in that case, so we don't seem to continue loading forever.
    if (!m_request && isLoading()) {
        setLoading(false);
        setStatus(Canceled);
        checkNotify();
    }
}

void CachedResource::addClient(CachedResourceClient* client)
{
    addClientToSet(client);
    didAddClient(client);
}

void CachedResource::didAddClient(CachedResourceClient* c)
{
    if (!isLoading())
        c->notifyFinished(this);
}

void CachedResource::addClientToSet(CachedResourceClient* client)
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
    m_clients.add(client);
}

void CachedResource::removeClient(CachedResourceClient* client)
{
    ASSERT(m_clients.contains(client));
    m_clients.remove(client);

    if (canDelete() && !inCache())
        delete this;
    else if (!hasClients() && inCache()) {
        memoryCache()->removeFromLiveResourcesSize(this);
        memoryCache()->removeFromLiveDecodedResourcesList(this);
        allClientsRemoved();
        if (response().cacheControlContainsNoStore()) {
            // RFC2616 14.9.2:
            // "no-store: ... MUST make a best-effort attempt to remove the information from volatile storage as promptly as possible"
            // "... History buffers MAY store such responses as part of their normal operation."
            // We allow non-secure content to be reused in history, but we do not allow secure content to be reused.
            if (protocolIs(url(), "https"))
                memoryCache()->remove(this);
        } else
            memoryCache()->prune();
    }
    // This object may be dead here.
}

void CachedResource::deleteIfPossible()
{
    if (canDelete() && !inCache())
        delete this;
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
    // A resource may start revalidation before this method has been called, so check that this resource is still the proxy resource before clearing it out.
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
        CachedResourceClient* client = it->first;
        unsigned count = it->second;
        while (count) {
            clientsToMove.append(client);
            --count;
        }
    }
    // Equivalent of calling removeClient() for all clients
    m_clients.clear();

    unsigned moveCount = clientsToMove.size();
    for (unsigned n = 0; n < moveCount; ++n)
        m_resourceToRevalidate->addClientToSet(clientsToMove[n]);
    for (unsigned n = 0; n < moveCount; ++n) {
        // Calling didAddClient for a client may end up removing another client. In that case it won't be in the set anymore.
        if (m_resourceToRevalidate->m_clients.contains(clientsToMove[n]))
            m_resourceToRevalidate->didAddClient(clientsToMove[n]);
    }
}
    
void CachedResource::updateResponseAfterRevalidation(const ResourceResponse& validatingResponse)
{
    m_responseTimestamp = currentTime();

    DEFINE_STATIC_LOCAL(const AtomicString, contentHeaderPrefix, ("content-"));
    // RFC2616 10.3.5
    // Update cached headers from the 304 response
    const HTTPHeaderMap& newHeaders = validatingResponse.httpHeaderFields();
    HTTPHeaderMap::const_iterator end = newHeaders.end();
    for (HTTPHeaderMap::const_iterator it = newHeaders.begin(); it != end; ++it) {
        // Don't allow 304 response to update content headers, these can't change but some servers send wrong values.
        if (it->first.startsWith(contentHeaderPrefix, false))
            continue;
        m_response.setHTTPHeaderField(it->first, it->second);
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
        
        if (m_data->hasPurgeableBuffer()) {
            m_purgeableData = m_data->releasePurgeableBuffer();
        } else {
            m_purgeableData = PurgeableBuffer::create(m_data->data(), m_data->size());
            if (!m_purgeableData)
                return false;
            m_purgeableData->setPurgePriority(purgePriority());
        }
        
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

    m_data = SharedBuffer::adoptPurgeableBuffer(m_purgeableData.release());
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
    return sizeof(CachedResource) + m_response.memoryUsage() + 576;
    /*
        576 = 192 +                   // average size of m_url
              384;                    // average size of m_clients hash map
    */
}
    
void CachedResource::setLoadPriority(ResourceLoadPriority loadPriority) 
{ 
    if (loadPriority == ResourceLoadPriorityUnresolved)
        return;
    m_loadPriority = loadPriority;
}

}
