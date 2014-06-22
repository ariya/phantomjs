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
#include "WebResourceCacheManagerProxy.h"

#include "ImmutableArray.h"
#include "ImmutableDictionary.h"
#include "SecurityOriginData.h"
#include "WebContext.h"
#include "WebResourceCacheManagerMessages.h"
#include "WebResourceCacheManagerProxyMessages.h"
#include "WebSecurityOrigin.h"

#if ENABLE(NETWORK_PROCESS)
#include "NetworkProcessMessages.h"
#endif

using namespace WebCore;

namespace WebKit {

const char* WebResourceCacheManagerProxy::supplementName()
{
    return "WebResourceCacheManagerProxy";
}

PassRefPtr<WebResourceCacheManagerProxy> WebResourceCacheManagerProxy::create(WebContext* webContext)
{
    return adoptRef(new WebResourceCacheManagerProxy(webContext));
}

WebResourceCacheManagerProxy::WebResourceCacheManagerProxy(WebContext* webContext)
    : WebContextSupplement(webContext)
{
    WebContextSupplement::context()->addMessageReceiver(Messages::WebResourceCacheManagerProxy::messageReceiverName(), this);
}

WebResourceCacheManagerProxy::~WebResourceCacheManagerProxy()
{
}

// WebContextSupplement

void WebResourceCacheManagerProxy::contextDestroyed()
{
    invalidateCallbackMap(m_arrayCallbacks);
}

void WebResourceCacheManagerProxy::processDidClose(WebProcessProxy*)
{
    invalidateCallbackMap(m_arrayCallbacks);
}

bool WebResourceCacheManagerProxy::shouldTerminate(WebProcessProxy*) const
{
    return m_arrayCallbacks.isEmpty();
}

void WebResourceCacheManagerProxy::refWebContextSupplement()
{
    APIObject::ref();
}

void WebResourceCacheManagerProxy::derefWebContextSupplement()
{
    APIObject::deref();
}

void WebResourceCacheManagerProxy::getCacheOrigins(PassRefPtr<ArrayCallback> prpCallback)
{
    RefPtr<ArrayCallback> callback = prpCallback;
    uint64_t callbackID = callback->callbackID();
    m_arrayCallbacks.set(callbackID, callback.release());

    // FIXME (Multi-WebProcess): <rdar://problem/12239765> When multi-process is enabled, we need to aggregate the callback data from all processes.
    context()->sendToAllProcessesRelaunchingThemIfNecessary(Messages::WebResourceCacheManager::GetCacheOrigins(callbackID));
}

void WebResourceCacheManagerProxy::didGetCacheOrigins(const Vector<SecurityOriginData>& origins, uint64_t callbackID)
{
    RefPtr<ArrayCallback> callback = m_arrayCallbacks.take(callbackID);
    performAPICallbackWithSecurityOriginDataVector(origins, callback.get());
}

void WebResourceCacheManagerProxy::clearCacheForOrigin(WebSecurityOrigin* origin, ResourceCachesToClear cachesToClear)
{
    SecurityOriginData securityOrigin;
    securityOrigin.protocol = origin->protocol();
    securityOrigin.host = origin->host();
    securityOrigin.port = origin->port();

    // FIXME (Multi-WebProcess): <rdar://problem/12239765> There is no need to relaunch all processes. One process to take care of persistent cache is enough.
    context()->sendToAllProcessesRelaunchingThemIfNecessary(Messages::WebResourceCacheManager::ClearCacheForOrigin(securityOrigin, cachesToClear));
}

void WebResourceCacheManagerProxy::clearCacheForAllOrigins(ResourceCachesToClear cachesToClear)
{
#if ENABLE(NETWORK_PROCESS)
    context()->sendToNetworkingProcessRelaunchingIfNecessary(Messages::NetworkProcess::ClearCacheForAllOrigins(cachesToClear));
#endif

    // FIXME (Multi-WebProcess): <rdar://problem/12239765> There is no need to relaunch all processes. One process to take care of persistent cache is enough.
    context()->sendToAllProcessesRelaunchingThemIfNecessary(Messages::WebResourceCacheManager::ClearCacheForAllOrigins(cachesToClear));
}

} // namespace WebKit
