/*
 * Copyright (C) 2009, 2010, 2012 Apple Inc. All rights reserved.
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
#include "WebProcess.h"

#include "AuthenticationManager.h"
#include "EventDispatcher.h"
#include "InjectedBundle.h"
#include "InjectedBundleUserMessageCoders.h"
#include "Logging.h"
#include "PluginProcessConnectionManager.h"
#include "StatisticsData.h"
#include "WebApplicationCacheManager.h"
#include "WebConnectionToUIProcess.h"
#include "WebContextMessages.h"
#include "WebCookieManager.h"
#include "WebCoreArgumentCoders.h"
#include "WebFrame.h"
#include "WebFrameNetworkingContext.h"
#include "WebGeolocationManager.h"
#include "WebIconDatabaseProxy.h"
#include "WebMediaCacheManager.h"
#include "WebMemorySampler.h"
#include "WebPage.h"
#include "WebPageCreationParameters.h"
#include "WebPageGroupProxyMessages.h"
#include "WebPlatformStrategies.h"
#include "WebPreferencesStore.h"
#include "WebProcessCreationParameters.h"
#include "WebProcessMessages.h"
#include "WebProcessProxyMessages.h"
#include "WebResourceCacheManager.h"
#include <JavaScriptCore/JSLock.h>
#include <JavaScriptCore/MemoryStatistics.h>
#include <WebCore/AXObjectCache.h>
#include <WebCore/ApplicationCacheStorage.h>
#include <WebCore/AuthenticationChallenge.h>
#include <WebCore/CrossOriginPreflightResultCache.h>
#include <WebCore/Font.h>
#include <WebCore/FontCache.h>
#include <WebCore/Frame.h>
#include <WebCore/FrameLoader.h>
#include <WebCore/GCController.h>
#include <WebCore/GlyphPageTreeNode.h>
#include <WebCore/IconDatabase.h>
#include <WebCore/JSDOMWindow.h>
#include <WebCore/Language.h>
#include <WebCore/MemoryCache.h>
#include <WebCore/MemoryPressureHandler.h>
#include <WebCore/Page.h>
#include <WebCore/PageCache.h>
#include <WebCore/PageGroup.h>
#include <WebCore/ResourceHandle.h>
#include <WebCore/RunLoop.h>
#include <WebCore/SchemeRegistry.h>
#include <WebCore/SecurityOrigin.h>
#include <WebCore/Settings.h>
#include <WebCore/StorageTracker.h>
#include <wtf/CurrentTime.h>
#include <wtf/HashCountedSet.h>
#include <wtf/PassRefPtr.h>
#include <wtf/text/StringHash.h>

#if ENABLE(NETWORK_INFO)
#include "WebNetworkInfoManager.h"
#include "WebNetworkInfoManagerMessages.h"
#endif

#if ENABLE(NETWORK_PROCESS)
#include "CookieStorageShim.h"
#include "NetworkProcessConnection.h"
#endif

#if !OS(WINDOWS)
#include <unistd.h>
#endif

#if !ENABLE(PLUGIN_PROCESS)
#include "NetscapePluginModule.h"
#endif

#if ENABLE(CUSTOM_PROTOCOLS)
#include "CustomProtocolManager.h"
#endif

#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
#include "WebNotificationManager.h"
#endif

#if ENABLE(SQL_DATABASE)
#include "WebDatabaseManager.h"
#endif

#if ENABLE(BATTERY_STATUS)
#include "WebBatteryManager.h"
#endif

#if ENABLE(NETWORK_PROCESS)
#include "WebResourceLoadScheduler.h"
#endif

#if ENABLE(PLUGIN_PROCESS)
#include "PluginProcessConnectionManager.h"
#endif

#if USE(SECURITY_FRAMEWORK)
#include "SecItemShim.h"
#endif

#if USE(SOUP)
#include "WebSoupRequestManager.h"
#endif

using namespace JSC;
using namespace WebCore;

// This should be less than plugInAutoStartExpirationTimeThreshold in PlugInAutoStartProvider.
static const double plugInAutoStartExpirationTimeUpdateThreshold = 29 * 24 * 60 * 60;

// This should be greater than tileRevalidationTimeout in TileController.
static const double nonVisibleProcessCleanupDelay = 10;

namespace WebKit {

WebProcess& WebProcess::shared()
{
    static WebProcess& process = *new WebProcess;
    return process;
}

WebProcess::WebProcess()
    : m_eventDispatcher(EventDispatcher::create())
    , m_inDidClose(false)
    , m_shouldTrackVisitedLinks(true)
    , m_hasSetCacheModel(false)
    , m_cacheModel(CacheModelDocumentViewer)
#if USE(ACCELERATED_COMPOSITING) && PLATFORM(MAC)
    , m_compositingRenderServerPort(MACH_PORT_NULL)
#endif
#if PLATFORM(MAC)
    , m_clearResourceCachesDispatchGroup(0)
#endif
    , m_fullKeyboardAccessEnabled(false)
#if PLATFORM(QT)
    , m_networkAccessManager(0)
#endif
    , m_textCheckerState()
    , m_iconDatabaseProxy(new WebIconDatabaseProxy(this))
#if ENABLE(NETWORK_PROCESS)
    , m_usesNetworkProcess(false)
    , m_webResourceLoadScheduler(new WebResourceLoadScheduler)
#endif
#if ENABLE(PLUGIN_PROCESS)
    , m_pluginProcessConnectionManager(PluginProcessConnectionManager::create())
#endif
    , m_nonVisibleProcessCleanupTimer(this, &WebProcess::nonVisibleProcessCleanupTimerFired)
{
    // Initialize our platform strategies.
    WebPlatformStrategies::initialize();

    // FIXME: This should moved to where WebProcess::initialize is called,
    // so that ports have a chance to customize, and ifdefs in this file are
    // limited.
    addSupplement<WebGeolocationManager>();
    addSupplement<WebApplicationCacheManager>();
    addSupplement<WebResourceCacheManager>();
    addSupplement<WebCookieManager>();
    addSupplement<WebMediaCacheManager>();
    addSupplement<AuthenticationManager>();
    
#if ENABLE(SQL_DATABASE)
    addSupplement<WebDatabaseManager>();
#endif
#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    addSupplement<WebNotificationManager>();
#endif
#if ENABLE(CUSTOM_PROTOCOLS)
    addSupplement<CustomProtocolManager>();
#endif
#if ENABLE(BATTERY_STATUS)
    addSupplement<WebBatteryManager>();
#endif
#if ENABLE(NETWORK_INFO)
    addSupplement<WebNetworkInfoManager>();
#endif
#if USE(SOUP)
    addSupplement<WebSoupRequestManager>();
#endif
}

void WebProcess::initializeProcess(const ChildProcessInitializationParameters& parameters)
{
    platformInitializeProcess(parameters);
}

void WebProcess::initializeConnection(CoreIPC::Connection* connection)
{
    ChildProcess::initializeConnection(connection);

    connection->setShouldExitOnSyncMessageSendFailure(true);

    m_eventDispatcher->initializeConnection(connection);

#if ENABLE(PLUGIN_PROCESS)
    m_pluginProcessConnectionManager->initializeConnection(connection);
#endif

#if USE(SECURITY_FRAMEWORK)
    SecItemShim::shared().initializeConnection(connection);
#endif
    
    WebProcessSupplementMap::const_iterator it = m_supplements.begin();
    WebProcessSupplementMap::const_iterator end = m_supplements.end();
    for (; it != end; ++it)
        it->value->initializeConnection(connection);

    m_webConnection = WebConnectionToUIProcess::create(this);

    // In order to ensure that the asynchronous messages that are used for notifying the UI process
    // about when WebFrame objects come and go are always delivered before the synchronous policy messages,
    // use this flag to force synchronous messages to be treated as asynchronous messages in the UI process
    // unless when doing so would lead to a deadlock.
    connection->setOnlySendMessagesAsDispatchWhenWaitingForSyncReplyWhenProcessingSuchAMessage(true);
}

void WebProcess::didCreateDownload()
{
    disableTermination();
}

void WebProcess::didDestroyDownload()
{
    enableTermination();
}

CoreIPC::Connection* WebProcess::downloadProxyConnection()
{
    return parentProcessConnection();
}

AuthenticationManager& WebProcess::downloadsAuthenticationManager()
{
    return *supplement<AuthenticationManager>();
}

void WebProcess::initializeWebProcess(const WebProcessCreationParameters& parameters, CoreIPC::MessageDecoder& decoder)
{
    ASSERT(m_pageMap.isEmpty());

    platformInitializeWebProcess(parameters, decoder);

    memoryPressureHandler().install();

    RefPtr<APIObject> injectedBundleInitializationUserData;
    InjectedBundleUserMessageDecoder messageDecoder(injectedBundleInitializationUserData);
    if (!decoder.decode(messageDecoder))
        return;

    if (!parameters.injectedBundlePath.isEmpty()) {
        m_injectedBundle = InjectedBundle::create(parameters.injectedBundlePath);
        m_injectedBundle->setSandboxExtension(SandboxExtension::create(parameters.injectedBundlePathExtensionHandle));

        if (!m_injectedBundle->load(injectedBundleInitializationUserData.get())) {
            // Don't keep around the InjectedBundle reference if the load fails.
            m_injectedBundle.clear();
        }
    }

    WebProcessSupplementMap::const_iterator it = m_supplements.begin();
    WebProcessSupplementMap::const_iterator end = m_supplements.end();
    for (; it != end; ++it)
        it->value->initialize(parameters);

#if ENABLE(ICONDATABASE)
    m_iconDatabaseProxy->setEnabled(parameters.iconDatabaseEnabled);
#endif

    if (!parameters.applicationCacheDirectory.isEmpty())
        cacheStorage().setCacheDirectory(parameters.applicationCacheDirectory);

    setShouldTrackVisitedLinks(parameters.shouldTrackVisitedLinks);
    setCacheModel(static_cast<uint32_t>(parameters.cacheModel));

    if (!parameters.languages.isEmpty())
        overrideUserPreferredLanguages(parameters.languages);

    m_textCheckerState = parameters.textCheckerState;

    m_fullKeyboardAccessEnabled = parameters.fullKeyboardAccessEnabled;

    for (size_t i = 0; i < parameters.urlSchemesRegistererdAsEmptyDocument.size(); ++i)
        registerURLSchemeAsEmptyDocument(parameters.urlSchemesRegistererdAsEmptyDocument[i]);

    for (size_t i = 0; i < parameters.urlSchemesRegisteredAsSecure.size(); ++i)
        registerURLSchemeAsSecure(parameters.urlSchemesRegisteredAsSecure[i]);

    for (size_t i = 0; i < parameters.urlSchemesForWhichDomainRelaxationIsForbidden.size(); ++i)
        setDomainRelaxationForbiddenForURLScheme(parameters.urlSchemesForWhichDomainRelaxationIsForbidden[i]);

    for (size_t i = 0; i < parameters.urlSchemesRegisteredAsLocal.size(); ++i)
        registerURLSchemeAsLocal(parameters.urlSchemesRegisteredAsLocal[i]);

    for (size_t i = 0; i < parameters.urlSchemesRegisteredAsNoAccess.size(); ++i)
        registerURLSchemeAsNoAccess(parameters.urlSchemesRegisteredAsNoAccess[i]);

    for (size_t i = 0; i < parameters.urlSchemesRegisteredAsDisplayIsolated.size(); ++i)
        registerURLSchemeAsDisplayIsolated(parameters.urlSchemesRegisteredAsDisplayIsolated[i]);

    for (size_t i = 0; i < parameters.urlSchemesRegisteredAsCORSEnabled.size(); ++i)
        registerURLSchemeAsCORSEnabled(parameters.urlSchemesRegisteredAsCORSEnabled[i]);

    setDefaultRequestTimeoutInterval(parameters.defaultRequestTimeoutInterval);

    if (parameters.shouldAlwaysUseComplexTextCodePath)
        setAlwaysUsesComplexTextCodePath(true);

    if (parameters.shouldUseFontSmoothing)
        setShouldUseFontSmoothing(true);

#if PLATFORM(MAC) || USE(CFNETWORK)
    WebFrameNetworkingContext::setPrivateBrowsingStorageSessionIdentifierBase(parameters.uiProcessBundleIdentifier);
#endif

#if ENABLE(NETWORK_PROCESS)
    m_usesNetworkProcess = parameters.usesNetworkProcess;
    ensureNetworkProcessConnection();

    if (usesNetworkProcess())
        CookieStorageShim::shared().initialize();
#endif
    setTerminationTimeout(parameters.terminationTimeout);

    resetPlugInAutoStartOriginHashes(parameters.plugInAutoStartOriginHashes);
    for (size_t i = 0; i < parameters.plugInAutoStartOrigins.size(); ++i)
        m_plugInAutoStartOrigins.add(parameters.plugInAutoStartOrigins[i]);
}

#if ENABLE(NETWORK_PROCESS)
void WebProcess::ensureNetworkProcessConnection()
{
    if (!m_usesNetworkProcess)
        return;

    if (m_networkProcessConnection)
        return;

    CoreIPC::Attachment encodedConnectionIdentifier;

    if (!parentProcessConnection()->sendSync(Messages::WebProcessProxy::GetNetworkProcessConnection(),
        Messages::WebProcessProxy::GetNetworkProcessConnection::Reply(encodedConnectionIdentifier), 0))
        return;

#if PLATFORM(MAC)
    CoreIPC::Connection::Identifier connectionIdentifier(encodedConnectionIdentifier.port());
    if (CoreIPC::Connection::identifierIsNull(connectionIdentifier))
        return;
#else
    ASSERT_NOT_REACHED();
#endif
    m_networkProcessConnection = NetworkProcessConnection::create(connectionIdentifier);
}
#endif // ENABLE(NETWORK_PROCESS)

void WebProcess::setShouldTrackVisitedLinks(bool shouldTrackVisitedLinks)
{
    m_shouldTrackVisitedLinks = shouldTrackVisitedLinks;
    PageGroup::setShouldTrackVisitedLinks(shouldTrackVisitedLinks);
}

void WebProcess::registerURLSchemeAsEmptyDocument(const String& urlScheme)
{
    SchemeRegistry::registerURLSchemeAsEmptyDocument(urlScheme);
}

void WebProcess::registerURLSchemeAsSecure(const String& urlScheme) const
{
    SchemeRegistry::registerURLSchemeAsSecure(urlScheme);
}

void WebProcess::setDomainRelaxationForbiddenForURLScheme(const String& urlScheme) const
{
    SchemeRegistry::setDomainRelaxationForbiddenForURLScheme(true, urlScheme);
}

void WebProcess::registerURLSchemeAsLocal(const String& urlScheme) const
{
    SchemeRegistry::registerURLSchemeAsLocal(urlScheme);
}

void WebProcess::registerURLSchemeAsNoAccess(const String& urlScheme) const
{
    SchemeRegistry::registerURLSchemeAsNoAccess(urlScheme);
}

void WebProcess::registerURLSchemeAsDisplayIsolated(const String& urlScheme) const
{
    SchemeRegistry::registerURLSchemeAsDisplayIsolated(urlScheme);
}

void WebProcess::registerURLSchemeAsCORSEnabled(const String& urlScheme) const
{
    SchemeRegistry::registerURLSchemeAsCORSEnabled(urlScheme);
}

void WebProcess::setDefaultRequestTimeoutInterval(double timeoutInterval)
{
    ResourceRequest::setDefaultTimeoutInterval(timeoutInterval);
}

void WebProcess::setAlwaysUsesComplexTextCodePath(bool alwaysUseComplexText)
{
    WebCore::Font::setCodePath(alwaysUseComplexText ? WebCore::Font::Complex : WebCore::Font::Auto);
}

void WebProcess::setShouldUseFontSmoothing(bool useFontSmoothing)
{
    WebCore::Font::setShouldUseSmoothing(useFontSmoothing);
}

void WebProcess::userPreferredLanguagesChanged(const Vector<String>& languages) const
{
    overrideUserPreferredLanguages(languages);
    languageDidChange();
}

void WebProcess::fullKeyboardAccessModeChanged(bool fullKeyboardAccessEnabled)
{
    m_fullKeyboardAccessEnabled = fullKeyboardAccessEnabled;
}

void WebProcess::ensurePrivateBrowsingSession()
{
#if PLATFORM(MAC) || USE(CFNETWORK)
    WebFrameNetworkingContext::ensurePrivateBrowsingSession();
#endif
}

void WebProcess::destroyPrivateBrowsingSession()
{
#if PLATFORM(MAC) || USE(CFNETWORK)
    WebFrameNetworkingContext::destroyPrivateBrowsingSession();
#endif
}

DownloadManager& WebProcess::downloadManager()
{
#if ENABLE(NETWORK_PROCESS)
    ASSERT(!m_usesNetworkProcess);
#endif

    DEFINE_STATIC_LOCAL(DownloadManager, downloadManager, (this));
    return downloadManager;
}

#if ENABLE(PLUGIN_PROCESS)
PluginProcessConnectionManager& WebProcess::pluginProcessConnectionManager()
{
    return *m_pluginProcessConnectionManager;
}
#endif

void WebProcess::setVisitedLinkTable(const SharedMemory::Handle& handle)
{
    RefPtr<SharedMemory> sharedMemory = SharedMemory::create(handle, SharedMemory::ReadOnly);
    if (!sharedMemory)
        return;

    m_visitedLinkTable.setSharedMemory(sharedMemory.release());
}

void WebProcess::visitedLinkStateChanged(const Vector<WebCore::LinkHash>& linkHashes)
{
    // FIXME: We may want to track visited links per WebPageGroup rather than per WebContext.
    for (size_t i = 0; i < linkHashes.size(); ++i) {
        HashMap<uint64_t, RefPtr<WebPageGroupProxy> >::const_iterator it = m_pageGroupMap.begin();
        HashMap<uint64_t, RefPtr<WebPageGroupProxy> >::const_iterator end = m_pageGroupMap.end();
        for (; it != end; ++it)
            Page::visitedStateChanged(PageGroup::pageGroup(it->value->identifier()), linkHashes[i]);
    }

    pageCache()->markPagesForVistedLinkStyleRecalc();
}

void WebProcess::allVisitedLinkStateChanged()
{
    // FIXME: We may want to track visited links per WebPageGroup rather than per WebContext.
    HashMap<uint64_t, RefPtr<WebPageGroupProxy> >::const_iterator it = m_pageGroupMap.begin();
    HashMap<uint64_t, RefPtr<WebPageGroupProxy> >::const_iterator end = m_pageGroupMap.end();
    for (; it != end; ++it)
        Page::allVisitedStateChanged(PageGroup::pageGroup(it->value->identifier()));

    pageCache()->markPagesForVistedLinkStyleRecalc();
}

bool WebProcess::isLinkVisited(LinkHash linkHash) const
{
    return m_visitedLinkTable.isLinkVisited(linkHash);
}

void WebProcess::addVisitedLink(WebCore::LinkHash linkHash)
{
    if (isLinkVisited(linkHash) || !m_shouldTrackVisitedLinks)
        return;
    parentProcessConnection()->send(Messages::WebContext::AddVisitedLinkHash(linkHash), 0);
}

void WebProcess::setCacheModel(uint32_t cm)
{
    CacheModel cacheModel = static_cast<CacheModel>(cm);

    if (!m_hasSetCacheModel || cacheModel != m_cacheModel) {
        m_hasSetCacheModel = true;
        m_cacheModel = cacheModel;
        platformSetCacheModel(cacheModel);
    }
}

WebPage* WebProcess::focusedWebPage() const
{    
    HashMap<uint64_t, RefPtr<WebPage> >::const_iterator end = m_pageMap.end();
    for (HashMap<uint64_t, RefPtr<WebPage> >::const_iterator it = m_pageMap.begin(); it != end; ++it) {
        WebPage* page = (*it).value.get();
        if (page->windowAndWebPageAreFocused())
            return page;
    }
    return 0;
}
    
#if PLATFORM(MAC)
void WebProcess::setProcessSuppressionEnabled(bool processSuppressionEnabled)
{
    HashMap<uint64_t, RefPtr<WebPage> >::const_iterator end = m_pageMap.end();
    for (HashMap<uint64_t, RefPtr<WebPage> >::const_iterator it = m_pageMap.begin(); it != end; ++it) {
        WebPage* page = (*it).value.get();
        page->setThrottled(processSuppressionEnabled);
    }
    
    ChildProcess::setProcessSuppressionEnabled(processSuppressionEnabled);
}
#endif

WebPage* WebProcess::webPage(uint64_t pageID) const
{
    return m_pageMap.get(pageID);
}

void WebProcess::createWebPage(uint64_t pageID, const WebPageCreationParameters& parameters)
{
    // It is necessary to check for page existence here since during a window.open() (or targeted
    // link) the WebPage gets created both in the synchronous handler and through the normal way. 
    HashMap<uint64_t, RefPtr<WebPage> >::AddResult result = m_pageMap.add(pageID, 0);
    if (result.isNewEntry) {
        ASSERT(!result.iterator->value);
        result.iterator->value = WebPage::create(pageID, parameters);

        // Balanced by an enableTermination in removeWebPage.
        disableTermination();
    }

    ASSERT(result.iterator->value);
}

void WebProcess::removeWebPage(uint64_t pageID)
{
    ASSERT(m_pageMap.contains(pageID));

    pageWillLeaveWindow(pageID);
    m_pageMap.remove(pageID);

    enableTermination();
}

bool WebProcess::shouldTerminate()
{
    ASSERT(m_pageMap.isEmpty());

#if ENABLE(NETWORK_PROCESS)
    ASSERT(m_usesNetworkProcess || !downloadManager().isDownloading());
#else
    ASSERT(!downloadManager().isDownloading());
#endif

    // FIXME: the ShouldTerminate message should also send termination parameters, such as any session cookies that need to be preserved.
    bool shouldTerminate = false;
    if (parentProcessConnection()->sendSync(Messages::WebProcessProxy::ShouldTerminate(), Messages::WebProcessProxy::ShouldTerminate::Reply(shouldTerminate), 0)
        && !shouldTerminate)
        return false;

    return true;
}

void WebProcess::terminate()
{
#ifndef NDEBUG
    gcController().garbageCollectNow();
    fontCache()->invalidate();
    memoryCache()->setDisabled(true);
#endif

    m_webConnection->invalidate();
    m_webConnection = nullptr;

    platformTerminate();

    ChildProcess::terminate();
}

void WebProcess::didReceiveSyncMessage(CoreIPC::Connection* connection, CoreIPC::MessageDecoder& decoder, OwnPtr<CoreIPC::MessageEncoder>& replyEncoder)
{
    messageReceiverMap().dispatchSyncMessage(connection, decoder, replyEncoder);
}

void WebProcess::didReceiveMessage(CoreIPC::Connection* connection, CoreIPC::MessageDecoder& decoder)
{
    if (messageReceiverMap().dispatchMessage(connection, decoder))
        return;

    if (decoder.messageReceiverName() == Messages::WebProcess::messageReceiverName()) {
        didReceiveWebProcessMessage(connection, decoder);
        return;
    }

    if (decoder.messageReceiverName() == Messages::WebPageGroupProxy::messageReceiverName()) {
        uint64_t pageGroupID = decoder.destinationID();
        if (!pageGroupID)
            return;
        
        WebPageGroupProxy* pageGroupProxy = webPageGroup(pageGroupID);
        if (!pageGroupProxy)
            return;
        
        pageGroupProxy->didReceiveMessage(connection, decoder);
    }
}

void WebProcess::didClose(CoreIPC::Connection*)
{
#ifndef NDEBUG
    m_inDidClose = true;

    // Close all the live pages.
    Vector<RefPtr<WebPage> > pages;
    copyValuesToVector(m_pageMap, pages);
    for (size_t i = 0; i < pages.size(); ++i)
        pages[i]->close();
    pages.clear();

    gcController().garbageCollectSoon();
    fontCache()->invalidate();
    memoryCache()->setDisabled(true);
#endif    

    // The UI process closed this connection, shut down.
    RunLoop::main()->stop();
}

void WebProcess::didReceiveInvalidMessage(CoreIPC::Connection*, CoreIPC::StringReference, CoreIPC::StringReference)
{
    // We received an invalid message, but since this is from the UI process (which we trust),
    // we'll let it slide.
}

WebFrame* WebProcess::webFrame(uint64_t frameID) const
{
    return m_frameMap.get(frameID);
}

void WebProcess::addWebFrame(uint64_t frameID, WebFrame* frame)
{
    m_frameMap.set(frameID, frame);
}

void WebProcess::removeWebFrame(uint64_t frameID)
{
    m_frameMap.remove(frameID);

    // We can end up here after our connection has closed when WebCore's frame life-support timer
    // fires when the application is shutting down. There's no need (and no way) to update the UI
    // process in this case.
    if (!parentProcessConnection())
        return;

    parentProcessConnection()->send(Messages::WebProcessProxy::DidDestroyFrame(frameID), 0);
}

WebPageGroupProxy* WebProcess::webPageGroup(PageGroup* pageGroup)
{
    for (HashMap<uint64_t, RefPtr<WebPageGroupProxy> >::const_iterator it = m_pageGroupMap.begin(), end = m_pageGroupMap.end(); it != end; ++it) {
        if (it->value->corePageGroup() == pageGroup)
            return it->value.get();
    }

    return 0;
}

WebPageGroupProxy* WebProcess::webPageGroup(uint64_t pageGroupID)
{
    return m_pageGroupMap.get(pageGroupID);
}

WebPageGroupProxy* WebProcess::webPageGroup(const WebPageGroupData& pageGroupData)
{
    HashMap<uint64_t, RefPtr<WebPageGroupProxy> >::AddResult result = m_pageGroupMap.add(pageGroupData.pageGroupID, 0);
    if (result.isNewEntry) {
        ASSERT(!result.iterator->value);
        result.iterator->value = WebPageGroupProxy::create(pageGroupData);
    }

    return result.iterator->value.get();
}

void WebProcess::clearResourceCaches(ResourceCachesToClear resourceCachesToClear)
{
    platformClearResourceCaches(resourceCachesToClear);

    // Toggling the cache model like this forces the cache to evict all its in-memory resources.
    // FIXME: We need a better way to do this.
    CacheModel cacheModel = m_cacheModel;
    setCacheModel(CacheModelDocumentViewer);
    setCacheModel(cacheModel);

    memoryCache()->evictResources();

    // Empty the cross-origin preflight cache.
    CrossOriginPreflightResultCache::shared().empty();
}

void WebProcess::clearApplicationCache()
{
    // Empty the application cache.
    cacheStorage().empty();
}

#if ENABLE(NETSCAPE_PLUGIN_API) && !ENABLE(PLUGIN_PROCESS)
void WebProcess::getSitesWithPluginData(const Vector<String>& pluginPaths, uint64_t callbackID)
{
    HashSet<String> sitesSet;

#if ENABLE(NETSCAPE_PLUGIN_API)
    for (size_t i = 0; i < pluginPaths.size(); ++i) {
        RefPtr<NetscapePluginModule> netscapePluginModule = NetscapePluginModule::getOrCreate(pluginPaths[i]);
        if (!netscapePluginModule)
            continue;

        Vector<String> sites = netscapePluginModule->sitesWithData();
        for (size_t i = 0; i < sites.size(); ++i)
            sitesSet.add(sites[i]);
    }
#else
    UNUSED_PARAM(pluginPaths);
#endif

    Vector<String> sites;
    copyToVector(sitesSet, sites);

    parentProcessConnection()->send(Messages::WebProcessProxy::DidGetSitesWithPluginData(sites, callbackID), 0);
}

void WebProcess::clearPluginSiteData(const Vector<String>& pluginPaths, const Vector<String>& sites, uint64_t flags, uint64_t maxAgeInSeconds, uint64_t callbackID)
{
#if ENABLE(NETSCAPE_PLUGIN_API)
    for (size_t i = 0; i < pluginPaths.size(); ++i) {
        RefPtr<NetscapePluginModule> netscapePluginModule = NetscapePluginModule::getOrCreate(pluginPaths[i]);
        if (!netscapePluginModule)
            continue;

        if (sites.isEmpty()) {
            // Clear everything.
            netscapePluginModule->clearSiteData(String(), flags, maxAgeInSeconds);
            continue;
        }

        for (size_t i = 0; i < sites.size(); ++i)
            netscapePluginModule->clearSiteData(sites[i], flags, maxAgeInSeconds);
    }
#else
    UNUSED_PARAM(pluginPaths);
    UNUSED_PARAM(sites);
    UNUSED_PARAM(flags);
    UNUSED_PARAM(maxAgeInSeconds);
#endif

    parentProcessConnection()->send(Messages::WebProcessProxy::DidClearPluginSiteData(callbackID), 0);
}
#endif

static inline void addCaseFoldedCharacters(StringHasher& hasher, const String& string)
{
    if (string.isEmpty())
        return;
    if (string.is8Bit())
        return hasher.addCharacters<LChar, CaseFoldingHash::foldCase<LChar> >(string.characters8(), string.length());
    return hasher.addCharacters<UChar, CaseFoldingHash::foldCase<UChar> >(string.characters16(), string.length());
}

static unsigned hashForPlugInOrigin(const String& pageOrigin, const String& pluginOrigin, const String& mimeType)
{
    // We want to avoid concatenating the strings and then taking the hash, since that could lead to an expensive conversion.
    // We also want to avoid using the hash() function in StringImpl or CaseFoldingHash because that masks out bits for the use of flags.
    StringHasher hasher;
    addCaseFoldedCharacters(hasher, pageOrigin);
    hasher.addCharacter(0);
    addCaseFoldedCharacters(hasher, pluginOrigin);
    hasher.addCharacter(0);
    addCaseFoldedCharacters(hasher, mimeType);
    return hasher.hash();
}

bool WebProcess::isPlugInAutoStartOriginHash(unsigned plugInOriginHash)
{
    HashMap<unsigned, double>::const_iterator it = m_plugInAutoStartOriginHashes.find(plugInOriginHash);
    if (it == m_plugInAutoStartOriginHashes.end())
        return false;
    return currentTime() < it->value;
}

bool WebProcess::shouldPlugInAutoStartFromOrigin(const WebPage* page, const String& pageOrigin, const String& pluginOrigin, const String& mimeType)
{
    if (m_plugInAutoStartOrigins.contains(pluginOrigin))
        return true;

#ifdef ENABLE_PRIMARY_SNAPSHOTTED_PLUGIN_HEURISTIC
    // The plugin wasn't in the general whitelist, so check if it similar to the primary plugin for the page (if we've found one).
    if (page && page->matchesPrimaryPlugIn(pageOrigin, pluginOrigin, mimeType))
        return true;
#else
    UNUSED_PARAM(page);
#endif

    // Lastly check against the more explicit hash list.
    return isPlugInAutoStartOriginHash(hashForPlugInOrigin(pageOrigin, pluginOrigin, mimeType));
}

void WebProcess::plugInDidStartFromOrigin(const String& pageOrigin, const String& pluginOrigin, const String& mimeType)
{
    if (pageOrigin.isEmpty()) {
        LOG(Plugins, "Not adding empty page origin");
        return;
    }

    unsigned plugInOriginHash = hashForPlugInOrigin(pageOrigin, pluginOrigin, mimeType);
    if (isPlugInAutoStartOriginHash(plugInOriginHash)) {
        LOG(Plugins, "Hash %x already exists as auto-start origin (request for %s)", plugInOriginHash, pageOrigin.utf8().data());
        return;
    }

    // We might attempt to start another plugin before the didAddPlugInAutoStartOrigin message
    // comes back from the parent process. Temporarily add this hash to the list with a thirty
    // second timeout. That way, even if the parent decides not to add it, we'll only be
    // incorrect for a little while.
    m_plugInAutoStartOriginHashes.set(plugInOriginHash, currentTime() + 30 * 1000);

    parentProcessConnection()->send(Messages::WebContext::AddPlugInAutoStartOriginHash(pageOrigin, plugInOriginHash), 0);
}

void WebProcess::didAddPlugInAutoStartOriginHash(unsigned plugInOriginHash, double expirationTime)
{
    // When called, some web process (which also might be this one) added the origin for auto-starting,
    // or received user interaction.
    // Set the bit to avoid having redundantly call into the UI process upon user interaction.
    m_plugInAutoStartOriginHashes.set(plugInOriginHash, expirationTime);
}

void WebProcess::resetPlugInAutoStartOriginHashes(const HashMap<unsigned, double>& hashes)
{
    m_plugInAutoStartOriginHashes.swap(const_cast<HashMap<unsigned, double>&>(hashes));
}

void WebProcess::plugInDidReceiveUserInteraction(const String& pageOrigin, const String& pluginOrigin, const String& mimeType)
{
    if (pageOrigin.isEmpty())
        return;

    unsigned plugInOriginHash = hashForPlugInOrigin(pageOrigin, pluginOrigin, mimeType);
    if (!plugInOriginHash)
        return;

    HashMap<unsigned, double>::iterator it = m_plugInAutoStartOriginHashes.find(plugInOriginHash);
    if (it == m_plugInAutoStartOriginHashes.end())
        return;
    if (it->value - currentTime() > plugInAutoStartExpirationTimeUpdateThreshold)
        return;

    parentProcessConnection()->send(Messages::WebContext::PlugInDidReceiveUserInteraction(plugInOriginHash), 0);
}

static void fromCountedSetToHashMap(TypeCountSet* countedSet, HashMap<String, uint64_t>& map)
{
    TypeCountSet::const_iterator end = countedSet->end();
    for (TypeCountSet::const_iterator it = countedSet->begin(); it != end; ++it)
        map.set(it->key, it->value);
}

static void getWebCoreMemoryCacheStatistics(Vector<HashMap<String, uint64_t> >& result)
{
    String imagesString(ASCIILiteral("Images"));
    String cssString(ASCIILiteral("CSS"));
    String xslString(ASCIILiteral("XSL"));
    String javaScriptString(ASCIILiteral("JavaScript"));
    
    MemoryCache::Statistics memoryCacheStatistics = memoryCache()->getStatistics();
    
    HashMap<String, uint64_t> counts;
    counts.set(imagesString, memoryCacheStatistics.images.count);
    counts.set(cssString, memoryCacheStatistics.cssStyleSheets.count);
    counts.set(xslString, memoryCacheStatistics.xslStyleSheets.count);
    counts.set(javaScriptString, memoryCacheStatistics.scripts.count);
    result.append(counts);
    
    HashMap<String, uint64_t> sizes;
    sizes.set(imagesString, memoryCacheStatistics.images.size);
    sizes.set(cssString, memoryCacheStatistics.cssStyleSheets.size);
    sizes.set(xslString, memoryCacheStatistics.xslStyleSheets.size);
    sizes.set(javaScriptString, memoryCacheStatistics.scripts.size);
    result.append(sizes);
    
    HashMap<String, uint64_t> liveSizes;
    liveSizes.set(imagesString, memoryCacheStatistics.images.liveSize);
    liveSizes.set(cssString, memoryCacheStatistics.cssStyleSheets.liveSize);
    liveSizes.set(xslString, memoryCacheStatistics.xslStyleSheets.liveSize);
    liveSizes.set(javaScriptString, memoryCacheStatistics.scripts.liveSize);
    result.append(liveSizes);
    
    HashMap<String, uint64_t> decodedSizes;
    decodedSizes.set(imagesString, memoryCacheStatistics.images.decodedSize);
    decodedSizes.set(cssString, memoryCacheStatistics.cssStyleSheets.decodedSize);
    decodedSizes.set(xslString, memoryCacheStatistics.xslStyleSheets.decodedSize);
    decodedSizes.set(javaScriptString, memoryCacheStatistics.scripts.decodedSize);
    result.append(decodedSizes);
    
    HashMap<String, uint64_t> purgeableSizes;
    purgeableSizes.set(imagesString, memoryCacheStatistics.images.purgeableSize);
    purgeableSizes.set(cssString, memoryCacheStatistics.cssStyleSheets.purgeableSize);
    purgeableSizes.set(xslString, memoryCacheStatistics.xslStyleSheets.purgeableSize);
    purgeableSizes.set(javaScriptString, memoryCacheStatistics.scripts.purgeableSize);
    result.append(purgeableSizes);
    
    HashMap<String, uint64_t> purgedSizes;
    purgedSizes.set(imagesString, memoryCacheStatistics.images.purgedSize);
    purgedSizes.set(cssString, memoryCacheStatistics.cssStyleSheets.purgedSize);
    purgedSizes.set(xslString, memoryCacheStatistics.xslStyleSheets.purgedSize);
    purgedSizes.set(javaScriptString, memoryCacheStatistics.scripts.purgedSize);
    result.append(purgedSizes);
}

void WebProcess::getWebCoreStatistics(uint64_t callbackID)
{
    StatisticsData data;
    
    // Gather JavaScript statistics.
    {
        JSLockHolder lock(JSDOMWindow::commonVM());
        data.statisticsNumbers.set(ASCIILiteral("JavaScriptObjectsCount"), JSDOMWindow::commonVM()->heap.objectCount());
        data.statisticsNumbers.set(ASCIILiteral("JavaScriptGlobalObjectsCount"), JSDOMWindow::commonVM()->heap.globalObjectCount());
        data.statisticsNumbers.set(ASCIILiteral("JavaScriptProtectedObjectsCount"), JSDOMWindow::commonVM()->heap.protectedObjectCount());
        data.statisticsNumbers.set(ASCIILiteral("JavaScriptProtectedGlobalObjectsCount"), JSDOMWindow::commonVM()->heap.protectedGlobalObjectCount());
        
        OwnPtr<TypeCountSet> protectedObjectTypeCounts(JSDOMWindow::commonVM()->heap.protectedObjectTypeCounts());
        fromCountedSetToHashMap(protectedObjectTypeCounts.get(), data.javaScriptProtectedObjectTypeCounts);
        
        OwnPtr<TypeCountSet> objectTypeCounts(JSDOMWindow::commonVM()->heap.objectTypeCounts());
        fromCountedSetToHashMap(objectTypeCounts.get(), data.javaScriptObjectTypeCounts);
        
        uint64_t javaScriptHeapSize = JSDOMWindow::commonVM()->heap.size();
        data.statisticsNumbers.set(ASCIILiteral("JavaScriptHeapSize"), javaScriptHeapSize);
        data.statisticsNumbers.set(ASCIILiteral("JavaScriptFreeSize"), JSDOMWindow::commonVM()->heap.capacity() - javaScriptHeapSize);
    }

    WTF::FastMallocStatistics fastMallocStatistics = WTF::fastMallocStatistics();
    data.statisticsNumbers.set(ASCIILiteral("FastMallocReservedVMBytes"), fastMallocStatistics.reservedVMBytes);
    data.statisticsNumbers.set(ASCIILiteral("FastMallocCommittedVMBytes"), fastMallocStatistics.committedVMBytes);
    data.statisticsNumbers.set(ASCIILiteral("FastMallocFreeListBytes"), fastMallocStatistics.freeListBytes);
    
    // Gather icon statistics.
    data.statisticsNumbers.set(ASCIILiteral("IconPageURLMappingCount"), iconDatabase().pageURLMappingCount());
    data.statisticsNumbers.set(ASCIILiteral("IconRetainedPageURLCount"), iconDatabase().retainedPageURLCount());
    data.statisticsNumbers.set(ASCIILiteral("IconRecordCount"), iconDatabase().iconRecordCount());
    data.statisticsNumbers.set(ASCIILiteral("IconsWithDataCount"), iconDatabase().iconRecordCountWithData());
    
    // Gather font statistics.
    data.statisticsNumbers.set(ASCIILiteral("CachedFontDataCount"), fontCache()->fontDataCount());
    data.statisticsNumbers.set(ASCIILiteral("CachedFontDataInactiveCount"), fontCache()->inactiveFontDataCount());
    
    // Gather glyph page statistics.
    data.statisticsNumbers.set(ASCIILiteral("GlyphPageCount"), GlyphPageTreeNode::treeGlyphPageCount());
    
    // Get WebCore memory cache statistics
    getWebCoreMemoryCacheStatistics(data.webCoreCacheStatistics);
    
    parentProcessConnection()->send(Messages::WebContext::DidGetStatistics(data, callbackID), 0);
}

void WebProcess::garbageCollectJavaScriptObjects()
{
    gcController().garbageCollectNow();
}

void WebProcess::setJavaScriptGarbageCollectorTimerEnabled(bool flag)
{
    gcController().setJavaScriptGarbageCollectorTimerEnabled(flag);
}

void WebProcess::postInjectedBundleMessage(const CoreIPC::DataReference& messageData)
{
    InjectedBundle* injectedBundle = WebProcess::shared().injectedBundle();
    if (!injectedBundle)
        return;

    OwnPtr<CoreIPC::ArgumentDecoder> decoder = CoreIPC::ArgumentDecoder::create(messageData.data(), messageData.size());

    String messageName;
    if (!decoder->decode(messageName))
        return;

    RefPtr<APIObject> messageBody;
    InjectedBundleUserMessageDecoder messageBodyDecoder(messageBody);
    if (!decoder->decode(messageBodyDecoder))
        return;

    injectedBundle->didReceiveMessage(messageName, messageBody.get());
}

#if ENABLE(NETWORK_PROCESS)
NetworkProcessConnection* WebProcess::networkConnection()
{
    ASSERT(m_usesNetworkProcess);

    // If we've lost our connection to the network process (e.g. it crashed) try to re-establish it.
    if (!m_networkProcessConnection)
        ensureNetworkProcessConnection();
    
    // If we failed to re-establish it then we are beyond recovery and should crash.
    if (!m_networkProcessConnection)
        CRASH();
    
    return m_networkProcessConnection.get();
}

void WebProcess::networkProcessConnectionClosed(NetworkProcessConnection* connection)
{
    ASSERT(m_networkProcessConnection);
    ASSERT(m_networkProcessConnection == connection);

    m_networkProcessConnection = 0;
    
    m_webResourceLoadScheduler->networkProcessCrashed();
}

WebResourceLoadScheduler& WebProcess::webResourceLoadScheduler()
{
    return *m_webResourceLoadScheduler;
}
#endif

void WebProcess::downloadRequest(uint64_t downloadID, uint64_t initiatingPageID, const ResourceRequest& request)
{
    WebPage* initiatingPage = initiatingPageID ? webPage(initiatingPageID) : 0;

    ResourceRequest requestWithOriginalURL = request;
    if (initiatingPage)
        initiatingPage->mainFrame()->loader()->setOriginalURLForDownloadRequest(requestWithOriginalURL);

    downloadManager().startDownload(downloadID, requestWithOriginalURL);
}

void WebProcess::cancelDownload(uint64_t downloadID)
{
    downloadManager().cancelDownload(downloadID);
}

#if PLATFORM(QT)
void WebProcess::startTransfer(uint64_t downloadID, const String& destination)
{
    downloadManager().startTransfer(downloadID, destination);
}
#endif

void WebProcess::setEnhancedAccessibility(bool flag)
{
    WebCore::AXObjectCache::setEnhancedUserInterfaceAccessibility(flag);
}
    
void WebProcess::startMemorySampler(const SandboxExtension::Handle& sampleLogFileHandle, const String& sampleLogFilePath, const double interval)
{
#if ENABLE(MEMORY_SAMPLER)    
    WebMemorySampler::shared()->start(sampleLogFileHandle, sampleLogFilePath, interval);
#endif
}
    
void WebProcess::stopMemorySampler()
{
#if ENABLE(MEMORY_SAMPLER)
    WebMemorySampler::shared()->stop();
#endif
}

void WebProcess::setTextCheckerState(const TextCheckerState& textCheckerState)
{
    bool continuousSpellCheckingTurnedOff = !textCheckerState.isContinuousSpellCheckingEnabled && m_textCheckerState.isContinuousSpellCheckingEnabled;
    bool grammarCheckingTurnedOff = !textCheckerState.isGrammarCheckingEnabled && m_textCheckerState.isGrammarCheckingEnabled;

    m_textCheckerState = textCheckerState;

    if (!continuousSpellCheckingTurnedOff && !grammarCheckingTurnedOff)
        return;

    HashMap<uint64_t, RefPtr<WebPage> >::iterator end = m_pageMap.end();
    for (HashMap<uint64_t, RefPtr<WebPage> >::iterator it = m_pageMap.begin(); it != end; ++it) {
        WebPage* page = (*it).value.get();
        if (continuousSpellCheckingTurnedOff)
            page->unmarkAllMisspellings();
        if (grammarCheckingTurnedOff)
            page->unmarkAllBadGrammar();
    }
}

void WebProcess::releasePageCache()
{
    int savedPageCacheCapacity = pageCache()->capacity();
    pageCache()->setCapacity(0);
    pageCache()->setCapacity(savedPageCacheCapacity);
}

#if !PLATFORM(MAC)
void WebProcess::initializeProcessName(const ChildProcessInitializationParameters&)
{
}

void WebProcess::initializeSandbox(const ChildProcessInitializationParameters&, SandboxInitializationParameters&)
{
}

void WebProcess::platformInitializeProcess(const ChildProcessInitializationParameters&)
{
}

void WebProcess::updateActivePages()
{
}

#endif
    
void WebProcess::pageDidEnterWindow(uint64_t pageID)
{
    m_pagesInWindows.add(pageID);
    m_nonVisibleProcessCleanupTimer.stop();
}

void WebProcess::pageWillLeaveWindow(uint64_t pageID)
{
    m_pagesInWindows.remove(pageID);

    if (m_pagesInWindows.isEmpty() && !m_nonVisibleProcessCleanupTimer.isActive())
        m_nonVisibleProcessCleanupTimer.startOneShot(nonVisibleProcessCleanupDelay);
}
    
void WebProcess::nonVisibleProcessCleanupTimerFired(Timer<WebProcess>*)
{
    ASSERT(m_pagesInWindows.isEmpty());
    if (!m_pagesInWindows.isEmpty())
        return;

#if PLATFORM(MAC)
    wkDestroyRenderingResources();
#endif
}

} // namespace WebKit
