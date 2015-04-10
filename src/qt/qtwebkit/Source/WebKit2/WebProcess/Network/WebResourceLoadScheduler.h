/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#ifndef WebResourceLoadScheduler_h
#define WebResourceLoadScheduler_h

#include "WebResourceLoader.h"
#include <WebCore/ResourceLoadPriority.h>
#include <WebCore/ResourceLoadScheduler.h>
#include <WebCore/ResourceLoader.h>
#include <WebCore/RunLoop.h>

#if ENABLE(NETWORK_PROCESS)

namespace WebKit {

class NetworkProcessConnection;
typedef uint64_t ResourceLoadIdentifier;

class WebResourceLoadScheduler : public WebCore::ResourceLoadScheduler {
    WTF_MAKE_NONCOPYABLE(WebResourceLoadScheduler); WTF_MAKE_FAST_ALLOCATED;
public:
    WebResourceLoadScheduler();
    virtual ~WebResourceLoadScheduler();
    
    virtual PassRefPtr<WebCore::SubresourceLoader> scheduleSubresourceLoad(WebCore::Frame*, WebCore::CachedResource*, const WebCore::ResourceRequest&, WebCore::ResourceLoadPriority, const WebCore::ResourceLoaderOptions&) OVERRIDE;
    virtual PassRefPtr<WebCore::NetscapePlugInStreamLoader> schedulePluginStreamLoad(WebCore::Frame*, WebCore::NetscapePlugInStreamLoaderClient*, const WebCore::ResourceRequest&) OVERRIDE;
    
    virtual void remove(WebCore::ResourceLoader*) OVERRIDE;
    virtual void crossOriginRedirectReceived(WebCore::ResourceLoader*, const WebCore::KURL& redirectURL) OVERRIDE;
    
    virtual void servePendingRequests(WebCore::ResourceLoadPriority minimumPriority = WebCore::ResourceLoadPriorityVeryLow) OVERRIDE;

    virtual void suspendPendingRequests() OVERRIDE;
    virtual void resumePendingRequests() OVERRIDE;

    virtual void setSerialLoadingEnabled(bool) OVERRIDE;

    WebResourceLoader* webResourceLoaderForIdentifier(ResourceLoadIdentifier identifier) const { return m_webResourceLoaders.get(identifier); }

    void networkProcessCrashed();

private:
    void scheduleLoad(WebCore::ResourceLoader*, WebCore::CachedResource*, WebCore::ResourceLoadPriority, bool shouldClearReferrerOnHTTPSToHTTPRedirect);
    void scheduleInternallyFailedLoad(WebCore::ResourceLoader*);
    void internallyFailedLoadTimerFired();
    
    HashSet<RefPtr<WebCore::ResourceLoader> > m_internallyFailedResourceLoaders;
    WebCore::RunLoop::Timer<WebResourceLoadScheduler> m_internallyFailedLoadTimer;
    
    HashMap<unsigned long, RefPtr<WebResourceLoader> > m_webResourceLoaders;
    
    unsigned m_suspendPendingRequestsCount;

};

} // namespace WebKit

#endif // ENABLE(NETWORK_PROCESS)

#endif // WebResourceLoadScheduler_h
