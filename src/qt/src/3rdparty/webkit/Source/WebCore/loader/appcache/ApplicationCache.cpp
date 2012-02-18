/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
#include "ApplicationCache.h"

#if ENABLE(OFFLINE_WEB_APPLICATIONS)

#include "ApplicationCacheGroup.h"
#include "ApplicationCacheResource.h"
#include "ApplicationCacheStorage.h"
#include "ResourceRequest.h"
#include "SecurityOrigin.h"
#include <wtf/text/CString.h>
#include <stdio.h>

namespace WebCore {
 
ApplicationCache::ApplicationCache()
    : m_group(0)
    , m_manifest(0)
    , m_estimatedSizeInStorage(0)
    , m_storageID(0)
{
}

ApplicationCache::~ApplicationCache()
{
    if (m_group && !m_group->isCopy())
        m_group->cacheDestroyed(this);
}
    
void ApplicationCache::setGroup(ApplicationCacheGroup* group)
{
    ASSERT(!m_group || group == m_group);
    m_group = group;
}

bool ApplicationCache::isComplete() const
{
    return !m_group->cacheIsBeingUpdated(this);
}

void ApplicationCache::setManifestResource(PassRefPtr<ApplicationCacheResource> manifest)
{
    ASSERT(manifest);
    ASSERT(!m_manifest);
    ASSERT(manifest->type() & ApplicationCacheResource::Manifest);
    
    m_manifest = manifest.get();
    
    addResource(manifest);
}
    
void ApplicationCache::addResource(PassRefPtr<ApplicationCacheResource> resource)
{
    ASSERT(resource);
    
    const String& url = resource->url();
    
    ASSERT(!m_resources.contains(url));
    
    if (m_storageID) {
        ASSERT(!resource->storageID());
        ASSERT(resource->type() & ApplicationCacheResource::Master);
        
        // Add the resource to the storage.
        cacheStorage().store(resource.get(), this);
    }

    m_estimatedSizeInStorage += resource->estimatedSizeInStorage();

    m_resources.set(url, resource);
}

unsigned ApplicationCache::removeResource(const String& url)
{
    HashMap<String, RefPtr<ApplicationCacheResource> >::iterator it = m_resources.find(url);
    if (it == m_resources.end())
        return 0;

    // The resource exists, get its type so we can return it.
    unsigned type = it->second->type();

    m_resources.remove(it);

    m_estimatedSizeInStorage -= it->second->estimatedSizeInStorage();

    return type;
}    
    
ApplicationCacheResource* ApplicationCache::resourceForURL(const String& url)
{
    ASSERT(!KURL(ParsedURLString, url).hasFragmentIdentifier());
    return m_resources.get(url).get();
}    

bool ApplicationCache::requestIsHTTPOrHTTPSGet(const ResourceRequest& request)
{
    if (!request.url().protocolInHTTPFamily())
        return false;
    
    if (!equalIgnoringCase(request.httpMethod(), "GET"))
        return false;

    return true;
}    

ApplicationCacheResource* ApplicationCache::resourceForRequest(const ResourceRequest& request)
{
    // We only care about HTTP/HTTPS GET requests.
    if (!requestIsHTTPOrHTTPSGet(request))
        return 0;

    KURL url(request.url());
    if (url.hasFragmentIdentifier())
        url.removeFragmentIdentifier();

    return resourceForURL(url);
}

void ApplicationCache::setOnlineWhitelist(const Vector<KURL>& onlineWhitelist)
{
    ASSERT(m_onlineWhitelist.isEmpty());
    m_onlineWhitelist = onlineWhitelist; 
}

bool ApplicationCache::isURLInOnlineWhitelist(const KURL& url)
{
    size_t whitelistSize = m_onlineWhitelist.size();
    for (size_t i = 0; i < whitelistSize; ++i) {
        if (protocolHostAndPortAreEqual(url, m_onlineWhitelist[i]) && url.string().startsWith(m_onlineWhitelist[i].string()))
            return true;
    }
    return false;
}

void ApplicationCache::setFallbackURLs(const FallbackURLVector& fallbackURLs)
{
    ASSERT(m_fallbackURLs.isEmpty());
    m_fallbackURLs = fallbackURLs;
}

bool ApplicationCache::urlMatchesFallbackNamespace(const KURL& url, KURL* fallbackURL)
{
    size_t fallbackCount = m_fallbackURLs.size();
    for (size_t i = 0; i < fallbackCount; ++i) {
        if (protocolHostAndPortAreEqual(url, m_fallbackURLs[i].first) && url.string().startsWith(m_fallbackURLs[i].first.string())) {
            if (fallbackURL)
                *fallbackURL = m_fallbackURLs[i].second;
            return true;
        }
    }
    return false;
}

void ApplicationCache::clearStorageID()
{
    m_storageID = 0;
    
    ResourceMap::const_iterator end = m_resources.end();
    for (ResourceMap::const_iterator it = m_resources.begin(); it != end; ++it)
        it->second->clearStorageID();
}
    
void ApplicationCache::deleteCacheForOrigin(SecurityOrigin* origin)
{
    Vector<KURL> urls;
    if (!cacheStorage().manifestURLs(&urls)) {
        LOG_ERROR("Failed to retrieve ApplicationCache manifest URLs");
        return;
    }

    KURL originURL(KURL(), origin->toString());

    size_t count = urls.size();
    for (size_t i = 0; i < count; ++i) {
        if (protocolHostAndPortAreEqual(urls[i], originURL)) {
            ApplicationCacheGroup* group = cacheStorage().findInMemoryCacheGroup(urls[i]);
            if (group)
                group->makeObsolete();
            else
                cacheStorage().deleteCacheGroup(urls[i]);
        }
    }
}
    
#ifndef NDEBUG
void ApplicationCache::dump()
{
    HashMap<String, RefPtr<ApplicationCacheResource> >::const_iterator end = m_resources.end();
    
    for (HashMap<String, RefPtr<ApplicationCacheResource> >::const_iterator it = m_resources.begin(); it != end; ++it) {
        printf("%s ", it->first.ascii().data());
        ApplicationCacheResource::dumpType(it->second->type());
    }
}
#endif

}

#endif // ENABLE(OFFLINE_WEB_APPLICATIONS)
