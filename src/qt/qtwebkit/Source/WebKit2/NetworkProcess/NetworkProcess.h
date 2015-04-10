/*
 * Copyright (C) 2012, 2013 Apple Inc. All rights reserved.
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

#ifndef NetworkProcess_h
#define NetworkProcess_h

#if ENABLE(NETWORK_PROCESS)

#include "CacheModel.h"
#include "ChildProcess.h"
#include "DownloadManager.h"
#include "MessageReceiverMap.h"
#include "NetworkResourceLoadScheduler.h"
#include <wtf/Forward.h>

namespace WebCore {
    class RunLoop;
}

namespace WebKit {

class AuthenticationManager;
class NetworkConnectionToWebProcess;
class NetworkProcessSupplement;
class PlatformCertificateInfo;
struct NetworkProcessCreationParameters;

class NetworkProcess : public ChildProcess, private DownloadManager::Client {
    WTF_MAKE_NONCOPYABLE(NetworkProcess);
public:
    static NetworkProcess& shared();

    template <typename T>
    T* supplement()
    {
        return static_cast<T*>(m_supplements.get(T::supplementName()));
    }

    template <typename T>
    void addSupplement()
    {
        m_supplements.add(T::supplementName(), adoptPtr<NetworkProcessSupplement>(new T(this)));
    }

    void removeNetworkConnectionToWebProcess(NetworkConnectionToWebProcess*);

    NetworkResourceLoadScheduler& networkResourceLoadScheduler() { return m_networkResourceLoadScheduler; }

    AuthenticationManager& authenticationManager();
    DownloadManager& downloadManager();

private:
    NetworkProcess();
    ~NetworkProcess();

    void platformInitializeNetworkProcess(const NetworkProcessCreationParameters&);

    virtual void terminate() OVERRIDE;
    void platformTerminate();

    // ChildProcess
    virtual void initializeProcess(const ChildProcessInitializationParameters&) OVERRIDE;
    virtual void initializeProcessName(const ChildProcessInitializationParameters&) OVERRIDE;
    virtual void initializeSandbox(const ChildProcessInitializationParameters&, SandboxInitializationParameters&) OVERRIDE;
    virtual void initializeConnection(CoreIPC::Connection*) OVERRIDE;
    virtual bool shouldTerminate() OVERRIDE;

    // CoreIPC::Connection::Client
    virtual void didReceiveMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&) OVERRIDE;
    virtual void didReceiveSyncMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&, OwnPtr<CoreIPC::MessageEncoder>&);
    virtual void didClose(CoreIPC::Connection*) OVERRIDE;
    virtual void didReceiveInvalidMessage(CoreIPC::Connection*, CoreIPC::StringReference messageReceiverName, CoreIPC::StringReference messageName) OVERRIDE;

    // DownloadManager::Client
    virtual void didCreateDownload() OVERRIDE;
    virtual void didDestroyDownload() OVERRIDE;
    virtual CoreIPC::Connection* downloadProxyConnection() OVERRIDE;
    virtual AuthenticationManager& downloadsAuthenticationManager() OVERRIDE;

    // Message Handlers
    void didReceiveNetworkProcessMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&);
    void initializeNetworkProcess(const NetworkProcessCreationParameters&);
    void createNetworkConnectionToWebProcess();
    void ensurePrivateBrowsingSession();
    void destroyPrivateBrowsingSession();
    void downloadRequest(uint64_t downloadID, const WebCore::ResourceRequest&);
    void cancelDownload(uint64_t downloadID);
    void setCacheModel(uint32_t);
    void allowSpecificHTTPSCertificateForHost(const PlatformCertificateInfo&, const String& host);
    void getNetworkProcessStatistics(uint64_t callbackID);
    void clearCacheForAllOrigins(uint32_t cachesToClear);

    // Platform Helpers
    void platformSetCacheModel(CacheModel);

    // Connections to WebProcesses.
    Vector<RefPtr<NetworkConnectionToWebProcess>> m_webProcessConnections;

    NetworkResourceLoadScheduler m_networkResourceLoadScheduler;

    String m_diskCacheDirectory;
    bool m_hasSetCacheModel;
    CacheModel m_cacheModel;

    typedef HashMap<const char*, OwnPtr<NetworkProcessSupplement>, PtrHash<const char*>> NetworkProcessSupplementMap;
    NetworkProcessSupplementMap m_supplements;

#if PLATFORM(MAC)
    // FIXME: We'd like to be able to do this without the #ifdef, but WorkQueue + BinarySemaphore isn't good enough since
    // multiple requests to clear the cache can come in before previous requests complete, and we need to wait for all of them.
    // In the future using WorkQueue and a counting semaphore would work, as would WorkQueue supporting the libdispatch concept of "work groups".
    dispatch_group_t m_clearCacheDispatchGroup;
#endif
};

} // namespace WebKit

#endif // ENABLE(NETWORK_PROCESS)

#endif // NetworkProcess_h
