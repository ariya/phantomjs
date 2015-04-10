/*
 * Copyright (C) 2011, 2013 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebApplicationCacheManager.h"

#include "ChildProcess.h"
#include "SecurityOriginData.h"
#include "WebApplicationCacheManagerMessages.h"
#include "WebApplicationCacheManagerProxyMessages.h"
#include "WebCoreArgumentCoders.h"
#include <WebCore/ApplicationCache.h>
#include <WebCore/ApplicationCacheStorage.h>
#include <WebCore/SecurityOrigin.h>
#include <WebCore/SecurityOriginHash.h>

using namespace WebCore;

namespace WebKit {

const char* WebApplicationCacheManager::supplementName()
{
    return "WebApplicationCacheManager";
}

WebApplicationCacheManager::WebApplicationCacheManager(ChildProcess* childProcess)
    : m_childProcess(childProcess)
{
    m_childProcess->addMessageReceiver(Messages::WebApplicationCacheManager::messageReceiverName(), this);
}

void WebApplicationCacheManager::getApplicationCacheOrigins(uint64_t callbackID)
{
    HashSet<RefPtr<SecurityOrigin> > origins;

    cacheStorage().getOriginsWithCache(origins);

    Vector<SecurityOriginData> identifiers;
    identifiers.reserveCapacity(origins.size());

    HashSet<RefPtr<SecurityOrigin> >::iterator end = origins.end();
    HashSet<RefPtr<SecurityOrigin> >::iterator i = origins.begin();
    for (; i != end; ++i) {
        RefPtr<SecurityOrigin> origin = *i;
        
        SecurityOriginData originData;
        originData.protocol = origin->protocol();
        originData.host = origin->host();
        originData.port = origin->port();

        identifiers.uncheckedAppend(originData);
    }

    m_childProcess->send(Messages::WebApplicationCacheManagerProxy::DidGetApplicationCacheOrigins(identifiers, callbackID), 0);
}

void WebApplicationCacheManager::deleteEntriesForOrigin(const SecurityOriginData& originData)
{
    RefPtr<SecurityOrigin> origin = SecurityOrigin::create(originData.protocol, originData.host, originData.port);
    if (!origin)
        return;
    
    ApplicationCache::deleteCacheForOrigin(origin.get());
}

void WebApplicationCacheManager::deleteAllEntries()
{
    cacheStorage().deleteAllEntries();
}

void WebApplicationCacheManager::setAppCacheMaximumSize(uint64_t size)
{
    cacheStorage().setMaximumSize(size);
}

} // namespace WebKit
