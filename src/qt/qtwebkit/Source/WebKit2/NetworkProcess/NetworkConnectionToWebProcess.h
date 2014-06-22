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

#ifndef NetworkConnectionToWebProcess_h
#define NetworkConnectionToWebProcess_h

#if ENABLE(NETWORK_PROCESS)

#include "BlockingResponseMap.h"
#include "Connection.h"
#include "NetworkConnectionToWebProcessMessages.h"
#include <WebCore/ResourceLoadPriority.h>
#include <wtf/HashSet.h>
#include <wtf/RefCounted.h>

namespace WebCore {
class ResourceRequest;
}

namespace WebKit {

class BlobRegistrationData;
class NetworkConnectionToWebProcess;
class NetworkResourceLoader;
class SyncNetworkResourceLoader;
typedef uint64_t ResourceLoadIdentifier;

class NetworkConnectionToWebProcess : public RefCounted<NetworkConnectionToWebProcess>, CoreIPC::Connection::Client {
public:
    static PassRefPtr<NetworkConnectionToWebProcess> create(CoreIPC::Connection::Identifier);
    virtual ~NetworkConnectionToWebProcess();

    CoreIPC::Connection* connection() const { return m_connection.get(); }

    bool isSerialLoadingEnabled() const { return m_serialLoadingEnabled; }

private:
    NetworkConnectionToWebProcess(CoreIPC::Connection::Identifier);

    // CoreIPC::Connection::Client
    virtual void didReceiveMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&);
    virtual void didReceiveSyncMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&, OwnPtr<CoreIPC::MessageEncoder>&);
    virtual void didClose(CoreIPC::Connection*);
    virtual void didReceiveInvalidMessage(CoreIPC::Connection*, CoreIPC::StringReference messageReceiverName, CoreIPC::StringReference messageName);

    // Message handlers.
    void didReceiveNetworkConnectionToWebProcessMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&);
    void didReceiveSyncNetworkConnectionToWebProcessMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&, OwnPtr<CoreIPC::MessageEncoder>&);
    
    void scheduleResourceLoad(const NetworkResourceLoadParameters&);
    void performSynchronousLoad(const NetworkResourceLoadParameters&, PassRefPtr<Messages::NetworkConnectionToWebProcess::PerformSynchronousLoad::DelayedReply>);

    void removeLoadIdentifier(ResourceLoadIdentifier);
    void crossOriginRedirectReceived(ResourceLoadIdentifier, const WebCore::KURL& redirectURL);
    void servePendingRequests(uint32_t resourceLoadPriority);
    void setSerialLoadingEnabled(bool);
    void startDownload(bool privateBrowsingEnabled, uint64_t downloadID, const WebCore::ResourceRequest&);
    void convertMainResourceLoadToDownload(uint64_t mainResourceLoadIdentifier, uint64_t downloadID, const WebCore::ResourceRequest&, const WebCore::ResourceResponse&);

    void cookiesForDOM(bool privateBrowsingEnabled, const WebCore::KURL& firstParty, const WebCore::KURL&, String& result);
    void setCookiesFromDOM(bool privateBrowsingEnabled, const WebCore::KURL& firstParty, const WebCore::KURL&, const String&);
    void cookiesEnabled(bool privateBrowsingEnabled, const WebCore::KURL& firstParty, const WebCore::KURL&, bool& result);
    void cookieRequestHeaderFieldValue(bool privateBrowsingEnabled, const WebCore::KURL& firstParty, const WebCore::KURL&, String& result);
    void getRawCookies(bool privateBrowsingEnabled, const WebCore::KURL& firstParty, const WebCore::KURL&, Vector<WebCore::Cookie>&);
    void deleteCookie(bool privateBrowsingEnabled, const WebCore::KURL&, const String& cookieName);

    void registerBlobURL(const WebCore::KURL&, const BlobRegistrationData&);
    void registerBlobURLFromURL(const WebCore::KURL&, const WebCore::KURL& srcURL);
    void unregisterBlobURL(const WebCore::KURL&);

    RefPtr<CoreIPC::Connection> m_connection;

    HashMap<ResourceLoadIdentifier, RefPtr<NetworkResourceLoader>> m_networkResourceLoaders;
    HashMap<ResourceLoadIdentifier, RefPtr<SyncNetworkResourceLoader>> m_syncNetworkResourceLoaders;

    bool m_serialLoadingEnabled;
};

} // namespace WebKit

#endif // ENABLE(NETWORK_PROCESS)

#endif // NetworkConnectionToWebProcess_h
