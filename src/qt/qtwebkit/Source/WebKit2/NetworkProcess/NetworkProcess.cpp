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

#include "config.h"
#include "NetworkProcess.h"

#if ENABLE(NETWORK_PROCESS)

#include "ArgumentCoders.h"
#include "Attachment.h"
#include "AuthenticationManager.h"
#include "CustomProtocolManager.h"
#include "Logging.h"
#include "NetworkConnectionToWebProcess.h"
#include "NetworkProcessCreationParameters.h"
#include "NetworkProcessPlatformStrategies.h"
#include "NetworkProcessProxyMessages.h"
#include "RemoteNetworkingContext.h"
#include "SchedulableLoader.h"
#include "StatisticsData.h"
#include "WebContextMessages.h"
#include "WebCookieManager.h"
#include <WebCore/InitializeLogging.h>
#include <WebCore/ResourceRequest.h>
#include <WebCore/RunLoop.h>
#include <wtf/text/CString.h>

#if USE(SECURITY_FRAMEWORK)
#include "SecItemShim.h"
#endif

using namespace WebCore;

namespace WebKit {

NetworkProcess& NetworkProcess::shared()
{
    DEFINE_STATIC_LOCAL(NetworkProcess, networkProcess, ());
    return networkProcess;
}

NetworkProcess::NetworkProcess()
    : m_hasSetCacheModel(false)
    , m_cacheModel(CacheModelDocumentViewer)
#if PLATFORM(MAC)
    , m_clearCacheDispatchGroup(0)
#endif
{
    NetworkProcessPlatformStrategies::initialize();

    addSupplement<AuthenticationManager>();
    addSupplement<WebCookieManager>();
    addSupplement<CustomProtocolManager>();
}

NetworkProcess::~NetworkProcess()
{
}

AuthenticationManager& NetworkProcess::authenticationManager()
{
    return *supplement<AuthenticationManager>();
}

DownloadManager& NetworkProcess::downloadManager()
{
    DEFINE_STATIC_LOCAL(DownloadManager, downloadManager, (this));
    return downloadManager;
}

void NetworkProcess::removeNetworkConnectionToWebProcess(NetworkConnectionToWebProcess* connection)
{
    size_t vectorIndex = m_webProcessConnections.find(connection);
    ASSERT(vectorIndex != notFound);

    m_webProcessConnections.remove(vectorIndex);
}

bool NetworkProcess::shouldTerminate()
{
    // Network process keeps session cookies and credentials, so it should never terminate (as long as UI process connection is alive).
    return false;
}

void NetworkProcess::didReceiveMessage(CoreIPC::Connection* connection, CoreIPC::MessageDecoder& decoder)
{
    if (messageReceiverMap().dispatchMessage(connection, decoder))
        return;

    didReceiveNetworkProcessMessage(connection, decoder);
}

void NetworkProcess::didReceiveSyncMessage(CoreIPC::Connection* connection, CoreIPC::MessageDecoder& decoder, OwnPtr<CoreIPC::MessageEncoder>& replyEncoder)
{
    messageReceiverMap().dispatchSyncMessage(connection, decoder, replyEncoder);
}

void NetworkProcess::didClose(CoreIPC::Connection*)
{
    // The UIProcess just exited.
    RunLoop::current()->stop();
}

void NetworkProcess::didReceiveInvalidMessage(CoreIPC::Connection*, CoreIPC::StringReference, CoreIPC::StringReference)
{
    RunLoop::current()->stop();
}

void NetworkProcess::didCreateDownload()
{
    disableTermination();
}

void NetworkProcess::didDestroyDownload()
{
    enableTermination();
}

CoreIPC::Connection* NetworkProcess::downloadProxyConnection()
{
    return parentProcessConnection();
}

AuthenticationManager& NetworkProcess::downloadsAuthenticationManager()
{
    return authenticationManager();
}

void NetworkProcess::initializeNetworkProcess(const NetworkProcessCreationParameters& parameters)
{
    platformInitializeNetworkProcess(parameters);

    setCacheModel(static_cast<uint32_t>(parameters.cacheModel));

#if PLATFORM(MAC) || USE(CFNETWORK)
    RemoteNetworkingContext::setPrivateBrowsingStorageSessionIdentifierBase(parameters.uiProcessBundleIdentifier);
#endif

    if (parameters.privateBrowsingEnabled)
        RemoteNetworkingContext::ensurePrivateBrowsingSession();

    NetworkProcessSupplementMap::const_iterator it = m_supplements.begin();
    NetworkProcessSupplementMap::const_iterator end = m_supplements.end();
    for (; it != end; ++it)
        it->value->initialize(parameters);
}

void NetworkProcess::initializeConnection(CoreIPC::Connection* connection)
{
    ChildProcess::initializeConnection(connection);

#if USE(SECURITY_FRAMEWORK)
    SecItemShim::shared().initializeConnection(connection);
#endif

    NetworkProcessSupplementMap::const_iterator it = m_supplements.begin();
    NetworkProcessSupplementMap::const_iterator end = m_supplements.end();
    for (; it != end; ++it)
        it->value->initializeConnection(connection);
}

void NetworkProcess::createNetworkConnectionToWebProcess()
{
#if PLATFORM(MAC)
    // Create the listening port.
    mach_port_t listeningPort;
    mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &listeningPort);

    // Create a listening connection.
    RefPtr<NetworkConnectionToWebProcess> connection = NetworkConnectionToWebProcess::create(CoreIPC::Connection::Identifier(listeningPort));
    m_webProcessConnections.append(connection.release());

    CoreIPC::Attachment clientPort(listeningPort, MACH_MSG_TYPE_MAKE_SEND);
    parentProcessConnection()->send(Messages::NetworkProcessProxy::DidCreateNetworkConnectionToWebProcess(clientPort), 0);
#else
    notImplemented();
#endif
}

void NetworkProcess::ensurePrivateBrowsingSession()
{
    RemoteNetworkingContext::ensurePrivateBrowsingSession();
}

void NetworkProcess::destroyPrivateBrowsingSession()
{
    RemoteNetworkingContext::destroyPrivateBrowsingSession();
}

void NetworkProcess::downloadRequest(uint64_t downloadID, const ResourceRequest& request)
{
    downloadManager().startDownload(downloadID, request);
}

void NetworkProcess::cancelDownload(uint64_t downloadID)
{
    downloadManager().cancelDownload(downloadID);
}

void NetworkProcess::setCacheModel(uint32_t cm)
{
    CacheModel cacheModel = static_cast<CacheModel>(cm);

    if (!m_hasSetCacheModel || cacheModel != m_cacheModel) {
        m_hasSetCacheModel = true;
        m_cacheModel = cacheModel;
        platformSetCacheModel(cacheModel);
    }
}

void NetworkProcess::getNetworkProcessStatistics(uint64_t callbackID)
{
    NetworkResourceLoadScheduler& scheduler = NetworkProcess::shared().networkResourceLoadScheduler();

    StatisticsData data;

    data.statisticsNumbers.set("HostsPendingCount", scheduler.hostsPendingCount());
    data.statisticsNumbers.set("HostsActiveCount", scheduler.hostsActiveCount());
    data.statisticsNumbers.set("LoadsPendingCount", scheduler.loadsPendingCount());
    data.statisticsNumbers.set("LoadsActiveCount", scheduler.loadsActiveCount());
    data.statisticsNumbers.set("DownloadsActiveCount", shared().downloadManager().activeDownloadCount());
    data.statisticsNumbers.set("OutstandingAuthenticationChallengesCount", shared().authenticationManager().outstandingAuthenticationChallengeCount());

    parentProcessConnection()->send(Messages::WebContext::DidGetStatistics(data, callbackID), 0);
}

void NetworkProcess::terminate()
{
    platformTerminate();
    ChildProcess::terminate();
}

#if !PLATFORM(MAC)
void NetworkProcess::initializeProcess(const ChildProcessInitializationParameters&)
{
}

void NetworkProcess::initializeProcessName(const ChildProcessInitializationParameters&)
{
}

void NetworkProcess::initializeSandbox(const ChildProcessInitializationParameters&, SandboxInitializationParameters&)
{
}
#endif

} // namespace WebKit

#endif // ENABLE(NETWORK_PROCESS)
