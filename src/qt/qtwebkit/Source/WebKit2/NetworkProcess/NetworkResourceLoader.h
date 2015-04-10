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

#ifndef NetworkResourceLoader_h
#define NetworkResourceLoader_h

#if ENABLE(NETWORK_PROCESS)

#include "MessageSender.h"
#include "SchedulableLoader.h"
#include "ShareableResource.h"
#include <WebCore/ResourceHandleClient.h>
#include <WebCore/RunLoop.h>

typedef const struct _CFCachedURLResponse* CFCachedURLResponseRef;

namespace WebCore {
class ResourceBuffer;
class ResourceHandle;
class ResourceRequest;
}

namespace WebKit {

class NetworkConnectionToWebProcess;
class RemoteNetworkingContext;

class NetworkResourceLoader : public SchedulableLoader, public WebCore::ResourceHandleClient, public CoreIPC::MessageSender {
public:
    static RefPtr<NetworkResourceLoader> create(const NetworkResourceLoadParameters& parameters, NetworkConnectionToWebProcess* connection)
    {
        return adoptRef(new NetworkResourceLoader(parameters, connection));
    }
    
    ~NetworkResourceLoader();

    WebCore::ResourceHandle* handle() const { return m_handle.get(); }
    void didConvertHandleToDownload();

    virtual void start() OVERRIDE;
    virtual void abort() OVERRIDE;

    // ResourceHandleClient methods
    virtual void willSendRequestAsync(WebCore::ResourceHandle*, const WebCore::ResourceRequest&, const WebCore::ResourceResponse& redirectResponse) OVERRIDE;
    virtual void didSendData(WebCore::ResourceHandle*, unsigned long long bytesSent, unsigned long long totalBytesToBeSent) OVERRIDE;
    virtual void didReceiveResponseAsync(WebCore::ResourceHandle*, const WebCore::ResourceResponse&) OVERRIDE;
    virtual void didReceiveData(WebCore::ResourceHandle*, const char*, int, int encodedDataLength) OVERRIDE;
    virtual void didReceiveBuffer(WebCore::ResourceHandle*, PassRefPtr<WebCore::SharedBuffer>, int encodedDataLength) OVERRIDE;
    virtual void didFinishLoading(WebCore::ResourceHandle*, double finishTime) OVERRIDE;
    virtual void didFail(WebCore::ResourceHandle*, const WebCore::ResourceError&) OVERRIDE;
    virtual void wasBlocked(WebCore::ResourceHandle*) OVERRIDE;
    virtual void cannotShowURL(WebCore::ResourceHandle*) OVERRIDE;
    virtual bool shouldUseCredentialStorage(WebCore::ResourceHandle*) OVERRIDE;
    virtual void shouldUseCredentialStorageAsync(WebCore::ResourceHandle*) OVERRIDE;
    virtual void didReceiveAuthenticationChallenge(WebCore::ResourceHandle*, const WebCore::AuthenticationChallenge&) OVERRIDE;
    virtual void didCancelAuthenticationChallenge(WebCore::ResourceHandle*, const WebCore::AuthenticationChallenge&) OVERRIDE;
    virtual bool usesAsyncCallbacks() OVERRIDE { return true; }

#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
    virtual void canAuthenticateAgainstProtectionSpaceAsync(WebCore::ResourceHandle*, const WebCore::ProtectionSpace&) OVERRIDE;
#endif

#if USE(NETWORK_CFDATA_ARRAY_CALLBACK)
    virtual bool supportsDataArray() OVERRIDE;
    virtual void didReceiveDataArray(WebCore::ResourceHandle*, CFArrayRef) OVERRIDE;
#endif

#if PLATFORM(MAC)
    static size_t fileBackedResourceMinimumSize();
    virtual void willCacheResponseAsync(WebCore::ResourceHandle*, NSCachedURLResponse *) OVERRIDE;
    virtual void willStopBufferingData(WebCore::ResourceHandle*, const char*, int) OVERRIDE;
#endif // PLATFORM(MAC)

    // Message handlers.
    void didReceiveNetworkResourceLoaderMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&);

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
    static void tryGetShareableHandleFromCFURLCachedResponse(ShareableResource::Handle&, CFCachedURLResponseRef);
#endif

private:
    NetworkResourceLoader(const NetworkResourceLoadParameters&, NetworkConnectionToWebProcess*);

    // CoreIPC::MessageSender
    virtual CoreIPC::Connection* messageSenderConnection() OVERRIDE;
    virtual uint64_t messageSenderDestinationID() OVERRIDE;

    void continueWillSendRequest(const WebCore::ResourceRequest& newRequest);
    void continueDidReceiveResponse();
#if USE(PROTECTION_SPACE_AUTH_CALLBACK)
    void continueCanAuthenticateAgainstProtectionSpace(bool);
#endif

    void cleanup();
    
    void platformDidReceiveResponse(const WebCore::ResourceResponse&);

    template<typename U> bool sendAbortingOnFailure(const U& message, unsigned messageSendFlags = 0);

    RefPtr<RemoteNetworkingContext> m_networkingContext;
    RefPtr<WebCore::ResourceHandle> m_handle;

    // Keep the suggested request around while asynchronously asking to update it, because some parts of the request don't survive IPC.
    WebCore::ResourceRequest m_suggestedRequestForWillSendRequest;

    uint64_t m_bytesReceived;

    bool m_handleConvertedToDownload;

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
    static void tryGetShareableHandleFromSharedBuffer(ShareableResource::Handle&, WebCore::SharedBuffer*);
#endif
};

} // namespace WebKit

#endif // ENABLE(NETWORK_PROCESS)

#endif // NetworkResourceLoader_h
