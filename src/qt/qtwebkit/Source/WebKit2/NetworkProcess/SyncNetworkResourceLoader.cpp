/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
#include "SyncNetworkResourceLoader.h"

#if ENABLE(NETWORK_PROCESS)

#include "DataReference.h"
#include "NetworkProcess.h"
#include "RemoteNetworkingContext.h"
#include <WebCore/ResourceError.h>
#include <WebCore/ResourceHandle.h>
#include <WebCore/ResourceResponse.h>
#include <wtf/MainThread.h>

using namespace WebCore;

namespace WebKit {

SyncNetworkResourceLoader::SyncNetworkResourceLoader(const NetworkResourceLoadParameters& parameters, NetworkConnectionToWebProcess* connection, PassRefPtr<Messages::NetworkConnectionToWebProcess::PerformSynchronousLoad::DelayedReply> reply)
    : SchedulableLoader(parameters, connection)
    , m_delayedReply(reply)
{
}

void SyncNetworkResourceLoader::start()
{
    // FIXME (NetworkProcess): This is called on the NetworkProcess main thread, blocking any other requests from being scheduled.
    // This should move to a background thread, but we'd either need to be sure that:
    //   A - ResourceHandle::loadResourceSynchronously is safe to run on a background thread.
    //   B - Write custom loading logic that is known to be safe on a background thread.
    
    ASSERT(isMainThread());

    ResourceError error;
    ResourceResponse response;
    Vector<char> data;
    
    // FIXME (NetworkProcess): Create RemoteNetworkingContext with actual settings.
    RefPtr<RemoteNetworkingContext> networkingContext = RemoteNetworkingContext::create(false, false, inPrivateBrowsingMode(), shouldClearReferrerOnHTTPSToHTTPRedirect());

    consumeSandboxExtensions();

    ResourceHandle::loadResourceSynchronously(networkingContext.get(), request(), allowStoredCredentials(), error, response, data);

    cleanup();

    m_delayedReply->send(error, response, CoreIPC::DataReference((uint8_t*)data.data(), data.size()));
}

void SyncNetworkResourceLoader::abort()
{
    cleanup();
}

void SyncNetworkResourceLoader::cleanup()
{
    invalidateSandboxExtensions();
    NetworkProcess::shared().networkResourceLoadScheduler().scheduleRemoveLoader(this);
}

} // namespace WebKit

#endif // ENABLE(NETWORK_PROCESS)
