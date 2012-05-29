/*
    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller <mueller@kde.org>
    Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
    Copyright (C) 2004, 2005, 2006, 2007 Apple Inc. All rights reserved.

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

#ifndef CachedResource_h
#define CachedResource_h

#include "CachePolicy.h"
#include "FrameLoaderTypes.h"
#include "PlatformString.h"
#include "PurgePriority.h"
#include "ResourceLoadPriority.h"
#include "ResourceResponse.h"
#include <wtf/HashCountedSet.h>
#include <wtf/HashSet.h>
#include <wtf/OwnPtr.h>
#include <wtf/Vector.h>
#include <time.h>

namespace WebCore {

class MemoryCache;
class CachedMetadata;
class CachedResourceClient;
class CachedResourceHandleBase;
class CachedResourceLoader;
class CachedResourceRequest;
class Frame;
class InspectorResource;
class PurgeableBuffer;

// A resource that is held in the cache. Classes who want to use this object should derive
// from CachedResourceClient, to get the function calls in case the requested data has arrived.
// This class also does the actual communication with the loader to obtain the resource from the network.
class CachedResource {
    WTF_MAKE_NONCOPYABLE(CachedResource); WTF_MAKE_FAST_ALLOCATED;
    friend class MemoryCache;
    friend class InspectorResource;
    
public:
    enum Type {
        ImageResource,
        CSSStyleSheet,
        Script,
        FontResource
#if ENABLE(XSLT)
        , XSLStyleSheet
#endif
#if ENABLE(LINK_PREFETCH)
        , LinkResource
#endif
    };

    enum Status {
        Unknown,      // let cache decide what to do with it
        Pending,      // only partially loaded
        Cached,       // regular case
        Canceled,
        LoadError,
        DecodeError
    };

    CachedResource(const String& url, Type);
    virtual ~CachedResource();
    
    virtual void load(CachedResourceLoader* cachedResourceLoader)  { load(cachedResourceLoader, false, DoSecurityCheck, true); }
    void load(CachedResourceLoader*, bool incremental, SecurityCheckPolicy, bool sendResourceLoadCallbacks);

    virtual void setEncoding(const String&) { }
    virtual String encoding() const { return String(); }
    virtual void data(PassRefPtr<SharedBuffer> data, bool allDataReceived);
    virtual void error(CachedResource::Status);

    virtual bool shouldIgnoreHTTPStatusCodeErrors() const { return false; }

    const String &url() const { return m_url; }
    Type type() const { return static_cast<Type>(m_type); }
    
    ResourceLoadPriority loadPriority() const { return m_loadPriority; }
    void setLoadPriority(ResourceLoadPriority);

    void addClient(CachedResourceClient*);
    void removeClient(CachedResourceClient*);
    bool hasClients() const { return !m_clients.isEmpty(); }
    void deleteIfPossible();

    enum PreloadResult {
        PreloadNotReferenced,
        PreloadReferenced,
        PreloadReferencedWhileLoading,
        PreloadReferencedWhileComplete
    };
    PreloadResult preloadResult() const { return static_cast<PreloadResult>(m_preloadResult); }
    void setRequestedFromNetworkingLayer() { m_requestedFromNetworkingLayer = true; }

    virtual void didAddClient(CachedResourceClient*);
    virtual void allClientsRemoved() { }

    unsigned count() const { return m_clients.size(); }

    Status status() const { return static_cast<Status>(m_status); }
    void setStatus(Status status) { m_status = status; }

    unsigned size() const { return encodedSize() + decodedSize() + overheadSize(); }
    unsigned encodedSize() const { return m_encodedSize; }
    unsigned decodedSize() const { return m_decodedSize; }
    unsigned overheadSize() const;
    
    bool isLoaded() const { return !m_loading; } // FIXME. Method name is inaccurate. Loading might not have started yet.

    bool isLoading() const { return m_loading; }
    void setLoading(bool b) { m_loading = b; }

    virtual bool isImage() const { return false; }
    bool isLinkResource() const
    {
#if ENABLE(LINK_PREFETCH)
        return type() == LinkResource;
#else
        return false;
#endif
    }

    unsigned accessCount() const { return m_accessCount; }
    void increaseAccessCount() { m_accessCount++; }

    // Computes the status of an object after loading.  
    // Updates the expire date on the cache entry file
    void finish();

    // Called by the cache if the object has been removed from the cache
    // while still being referenced. This means the object should delete itself
    // if the number of clients observing it ever drops to 0.
    // The resource can be brought back to cache after successful revalidation.
    void setInCache(bool inCache) { m_inCache = inCache; }
    bool inCache() const { return m_inCache; }
    
    void setInLiveDecodedResourcesList(bool b) { m_inLiveDecodedResourcesList = b; }
    bool inLiveDecodedResourcesList() { return m_inLiveDecodedResourcesList; }
    
    void setRequest(CachedResourceRequest*);

    SharedBuffer* data() const { ASSERT(!m_purgeableData); return m_data.get(); }

    void setResponse(const ResourceResponse&);
    const ResourceResponse& response() const { return m_response; }

    // Sets the serialized metadata retrieved from the platform's cache.
    void setSerializedCachedMetadata(const char*, size_t);

    // Caches the given metadata in association with this resource and suggests
    // that the platform persist it. The dataTypeID is a pseudo-randomly chosen
    // identifier that is used to distinguish data generated by the caller.
    void setCachedMetadata(unsigned dataTypeID, const char*, size_t);

    // Returns cached metadata of the given type associated with this resource.
    CachedMetadata* cachedMetadata(unsigned dataTypeID) const;

    bool canDelete() const { return !hasClients() && !m_request && !m_preloadCount && !m_handleCount && !m_resourceToRevalidate && !m_proxyResource; }

    bool isExpired() const;

    // List of acceptable MIME types separated by ",".
    // A MIME type may contain a wildcard, e.g. "text/*".
    String accept() const { return m_accept; }
    void setAccept(const String& accept) { m_accept = accept; }

    bool wasCanceled() const { return m_status == Canceled; }
    bool errorOccurred() const { return (m_status == LoadError || m_status == DecodeError); }

    bool sendResourceLoadCallbacks() const { return m_sendResourceLoadCallbacks; }
    
    virtual void destroyDecodedData() { }

    void setOwningCachedResourceLoader(CachedResourceLoader* cachedResourceLoader) { m_owningCachedResourceLoader = cachedResourceLoader; }
    
    bool isPreloaded() const { return m_preloadCount; }
    void increasePreloadCount() { ++m_preloadCount; }
    void decreasePreloadCount() { ASSERT(m_preloadCount); --m_preloadCount; }
    
    void registerHandle(CachedResourceHandleBase* h);
    void unregisterHandle(CachedResourceHandleBase* h);
    
    bool canUseCacheValidator() const;
    bool mustRevalidateDueToCacheHeaders(CachePolicy) const;
    bool isCacheValidator() const { return m_resourceToRevalidate; }
    CachedResource* resourceToRevalidate() const { return m_resourceToRevalidate; }
    
    bool isPurgeable() const;
    bool wasPurged() const;
    
    // This is used by the archive machinery to get at a purged resource without
    // triggering a load. We should make it protected again if we can find a
    // better way to handle the archive case.
    bool makePurgeable(bool purgeable);
    
    // HTTP revalidation support methods for CachedResourceLoader.
    void setResourceToRevalidate(CachedResource*);
    void switchClientsToRevalidatedResource();
    void clearResourceToRevalidate();
    void updateResponseAfterRevalidation(const ResourceResponse& validatingResponse);

protected:
    void checkNotify();

    void setEncodedSize(unsigned);
    void setDecodedSize(unsigned);
    void didAccessDecodedData(double timeStamp);

    bool isSafeToMakePurgeable() const;
    
    HashCountedSet<CachedResourceClient*> m_clients;

    String m_url;
    String m_accept;
    CachedResourceRequest* m_request;
    ResourceLoadPriority m_loadPriority;

    ResourceResponse m_response;
    double m_responseTimestamp;

    RefPtr<SharedBuffer> m_data;
    OwnPtr<PurgeableBuffer> m_purgeableData;

private:
    void addClientToSet(CachedResourceClient*);

    virtual PurgePriority purgePriority() const { return PurgeDefault; }

    double currentAge() const;
    double freshnessLifetime() const;

    RefPtr<CachedMetadata> m_cachedMetadata;

    double m_lastDecodedAccessTime; // Used as a "thrash guard" in the cache

    unsigned m_encodedSize;
    unsigned m_decodedSize;
    unsigned m_accessCount;
    unsigned m_handleCount;
    unsigned m_preloadCount;

    unsigned m_preloadResult : 2; // PreloadResult

    bool m_inLiveDecodedResourcesList : 1;
    bool m_requestedFromNetworkingLayer : 1;
    bool m_sendResourceLoadCallbacks : 1;

    bool m_inCache : 1;
    bool m_loading : 1;

    unsigned m_type : 3; // Type
    unsigned m_status : 3; // Status

#ifndef NDEBUG
    bool m_deleted;
    unsigned m_lruIndex;
#endif

    CachedResource* m_nextInAllResourcesList;
    CachedResource* m_prevInAllResourcesList;
    
    CachedResource* m_nextInLiveResourcesList;
    CachedResource* m_prevInLiveResourcesList;

    CachedResourceLoader* m_owningCachedResourceLoader; // only non-0 for resources that are not in the cache
    
    // If this field is non-null we are using the resource as a proxy for checking whether an existing resource is still up to date
    // using HTTP If-Modified-Since/If-None-Match headers. If the response is 304 all clients of this resource are moved
    // to to be clients of m_resourceToRevalidate and the resource is deleted. If not, the field is zeroed and this
    // resources becomes normal resource load.
    CachedResource* m_resourceToRevalidate;

    // If this field is non-null, the resource has a proxy for checking whether it is still up to date (see m_resourceToRevalidate).
    CachedResource* m_proxyResource;

    // These handles will need to be updated to point to the m_resourceToRevalidate in case we get 304 response.
    HashSet<CachedResourceHandleBase*> m_handlesToRevalidate;
};

}

#endif
