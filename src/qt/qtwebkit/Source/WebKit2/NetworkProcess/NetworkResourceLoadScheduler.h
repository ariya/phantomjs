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

#ifndef NetworkResourceLoadScheduler_h
#define NetworkResourceLoadScheduler_h

#include <WebCore/ResourceLoadPriority.h>
#include <WebCore/Timer.h>
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/text/StringHash.h>

#if ENABLE(NETWORK_PROCESS)

namespace WebCore {
class KURL;
}

namespace WebKit {

class HostRecord;
class SchedulableLoader;

class NetworkResourceLoadScheduler {
    WTF_MAKE_NONCOPYABLE(NetworkResourceLoadScheduler); WTF_MAKE_FAST_ALLOCATED;

public:
    NetworkResourceLoadScheduler();
    
    // Adds the request to the queue for its host.
    void scheduleLoader(PassRefPtr<SchedulableLoader>);

    // Called by the WebProcess when a ResourceLoader is being cleaned up.
    void removeLoader(SchedulableLoader*);

    // Called within the NetworkProcess on a background thread when a resource load has finished.
    void scheduleRemoveLoader(SchedulableLoader*);

    void receivedRedirect(SchedulableLoader*, const WebCore::KURL& redirectURL);
    void servePendingRequests(WebCore::ResourceLoadPriority = WebCore::ResourceLoadPriorityVeryLow);

    // For NetworkProcess statistics reporting.
    uint64_t hostsPendingCount() const;
    uint64_t loadsPendingCount() const;
    uint64_t hostsActiveCount() const;
    uint64_t loadsActiveCount() const;

private:
    enum CreateHostPolicy {
        CreateIfNotFound,
        FindOnly
    };
    
    HostRecord* hostForURL(const WebCore::KURL&, CreateHostPolicy = FindOnly);
    
    void scheduleServePendingRequests();
    void requestTimerFired(WebCore::Timer<NetworkResourceLoadScheduler>*);

    void platformInitializeMaximumHTTPConnectionCountPerHost();

    static void removeScheduledLoaders(void* context);
    void removeScheduledLoaders();

    typedef HashMap<String, RefPtr<HostRecord>, StringHash> HostMap;
    HostMap m_hosts;

    typedef HashSet<RefPtr<SchedulableLoader>> SchedulableLoaderSet;
    SchedulableLoaderSet m_loaders;

    RefPtr<HostRecord> m_nonHTTPProtocolHost;

    bool m_isSerialLoadingEnabled;

    WebCore::Timer<NetworkResourceLoadScheduler> m_requestTimer;
    
    Mutex m_loadersToRemoveMutex;
    Vector<RefPtr<SchedulableLoader>> m_loadersToRemove;

    unsigned m_maxRequestsInFlightPerHost;
};

} // namespace WebKit

#endif // ENABLE(NETWORK_PROCESS)

#endif // NetworkResourceLoadScheduler_h
