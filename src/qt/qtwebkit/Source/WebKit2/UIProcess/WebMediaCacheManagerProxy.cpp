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
#include "WebMediaCacheManagerProxy.h"

#include "WebContext.h"
#include "WebMediaCacheManagerMessages.h"
#include "WebMediaCacheManagerProxyMessages.h"
#include "WebSecurityOrigin.h"

namespace WebKit {

const char* WebMediaCacheManagerProxy::supplementName()
{
    return "WebMediaCacheManagerProxy";
}

PassRefPtr<WebMediaCacheManagerProxy> WebMediaCacheManagerProxy::create(WebContext* context)
{
    return adoptRef(new WebMediaCacheManagerProxy(context));
}

WebMediaCacheManagerProxy::WebMediaCacheManagerProxy(WebContext* context)
    : WebContextSupplement(context)
{
    WebContextSupplement::context()->addMessageReceiver(Messages::WebMediaCacheManagerProxy::messageReceiverName(), this);
}

WebMediaCacheManagerProxy::~WebMediaCacheManagerProxy()
{
}

// WebContextSupplement

void WebMediaCacheManagerProxy::contextDestroyed()
{
    invalidateCallbackMap(m_arrayCallbacks);
}

void WebMediaCacheManagerProxy::processDidClose(WebProcessProxy*)
{
    invalidateCallbackMap(m_arrayCallbacks);
}

bool WebMediaCacheManagerProxy::shouldTerminate(WebProcessProxy*) const
{
    return m_arrayCallbacks.isEmpty();
}

void WebMediaCacheManagerProxy::refWebContextSupplement()
{
    APIObject::ref();
}

void WebMediaCacheManagerProxy::derefWebContextSupplement()
{
    APIObject::deref();
}

void WebMediaCacheManagerProxy::getHostnamesWithMediaCache(PassRefPtr<ArrayCallback> prpCallback)
{
    RefPtr<ArrayCallback> callback = prpCallback;
    uint64_t callbackID = callback->callbackID();
    m_arrayCallbacks.set(callbackID, callback.release());

    // FIXME (Multi-WebProcess): <rdar://problem/12239765> When we're sending this to multiple processes, we need to aggregate the callback data when it comes back.
    context()->sendToAllProcessesRelaunchingThemIfNecessary(Messages::WebMediaCacheManager::GetHostnamesWithMediaCache(callbackID));
}
    
void WebMediaCacheManagerProxy::didGetHostnamesWithMediaCache(const Vector<String>& hostnameList, uint64_t callbackID)
{
    RefPtr<ArrayCallback> callback = m_arrayCallbacks.take(callbackID);
    if (!callback) {
        // FIXME: Log error or assert.
        return;
    }

    size_t hostnameCount = hostnameList.size();
    Vector<RefPtr<APIObject> > hostnames(hostnameCount);

    for (size_t i = 0; i < hostnameCount; ++i)
        hostnames[i] = WebString::create(hostnameList[i]);

    callback->performCallbackWithReturnValue(ImmutableArray::adopt(hostnames).get());
}

void WebMediaCacheManagerProxy::clearCacheForHostname(const String& hostname)
{
    context()->sendToAllProcessesRelaunchingThemIfNecessary(Messages::WebMediaCacheManager::ClearCacheForHostname(hostname));
}

void WebMediaCacheManagerProxy::clearCacheForAllHostnames()
{
    context()->sendToAllProcessesRelaunchingThemIfNecessary(Messages::WebMediaCacheManager::ClearCacheForAllHostnames());
}

} // namespace WebKit
