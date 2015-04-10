/*
 * Copyright (C) 2010, 2011, 2012, 2013 Apple Inc. All rights reserved.
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

#ifndef WebContext_h
#define WebContext_h

#include "APIObject.h"
#include "DownloadProxyMap.h"
#include "GenericCallback.h"
#include "ImmutableArray.h"
#include "ImmutableDictionary.h"
#include "MessageReceiver.h"
#include "MessageReceiverMap.h"
#include "PlugInAutoStartProvider.h"
#include "PluginInfoStore.h"
#include "ProcessModel.h"
#include "StatisticsRequest.h"
#include "StorageManager.h"
#include "VisitedLinkProvider.h"
#include "WebContextClient.h"
#include "WebContextConnectionClient.h"
#include "WebContextInjectedBundleClient.h"
#include "WebDownloadClient.h"
#include "WebHistoryClient.h"
#include "WebProcessProxy.h"
#include <WebCore/LinkHash.h>
#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/text/StringHash.h>
#include <wtf/text/WTFString.h>

#if ENABLE(NETWORK_PROCESS)
#include "NetworkProcessProxy.h"
#endif

#if PLATFORM(MAC)
OBJC_CLASS NSObject;
OBJC_CLASS NSString;
#endif

namespace WebKit {

class DownloadProxy;
class WebContextSupplement;
class WebIconDatabase;
class WebPageGroup;
class WebPageProxy;
struct StatisticsData;
struct WebProcessCreationParameters;
    
typedef GenericCallback<WKDictionaryRef> DictionaryCallback;

#if ENABLE(NETWORK_INFO)
class WebNetworkInfoManagerProxy;
#endif
#if ENABLE(NETWORK_PROCESS)
struct NetworkProcessCreationParameters;
#endif

#if PLATFORM(MAC)
extern NSString *SchemeForCustomProtocolRegisteredNotificationName;
extern NSString *SchemeForCustomProtocolUnregisteredNotificationName;
#endif

class WebContext : public TypedAPIObject<APIObject::TypeContext>, private CoreIPC::MessageReceiver
#if ENABLE(NETSCAPE_PLUGIN_API)
    , private PluginInfoStoreClient
#endif
    {
public:
    static PassRefPtr<WebContext> create(const String& injectedBundlePath);
    virtual ~WebContext();

    static const Vector<WebContext*>& allContexts();

    template <typename T>
    T* supplement()
    {
        return static_cast<T*>(m_supplements.get(T::supplementName()));
    }

    template <typename T>
    void addSupplement()
    {
        m_supplements.add(T::supplementName(), T::create(this));
    }

    void addMessageReceiver(CoreIPC::StringReference messageReceiverName, CoreIPC::MessageReceiver*);
    void addMessageReceiver(CoreIPC::StringReference messageReceiverName, uint64_t destinationID, CoreIPC::MessageReceiver*);
    void removeMessageReceiver(CoreIPC::StringReference messageReceiverName, uint64_t destinationID);

    bool dispatchMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&);
    bool dispatchSyncMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&, OwnPtr<CoreIPC::MessageEncoder>&);

    void initializeClient(const WKContextClient*);
    void initializeInjectedBundleClient(const WKContextInjectedBundleClient*);
    void initializeConnectionClient(const WKContextConnectionClient*);
    void initializeHistoryClient(const WKContextHistoryClient*);
    void initializeDownloadClient(const WKContextDownloadClient*);

    void setProcessModel(ProcessModel); // Can only be called when there are no processes running.
    ProcessModel processModel() const { return m_processModel; }

    void setMaximumNumberOfProcesses(unsigned); // Can only be called when there are no processes running.
    unsigned maximumNumberOfProcesses() const { return m_webProcessCountLimit; }

    // WebProcess or NetworkProcess as approporiate for current process model. The connection must be non-null.
    CoreIPC::Connection* networkingProcessConnection();

    template<typename U> void sendToAllProcesses(const U& message);
    template<typename U> void sendToAllProcessesRelaunchingThemIfNecessary(const U& message);
    template<typename U> void sendToOneProcess(const U& message);

    // Sends the message to WebProcess or NetworkProcess as approporiate for current process model.
    template<typename U> void sendToNetworkingProcess(const U& message);
    template<typename U> void sendToNetworkingProcessRelaunchingIfNecessary(const U& message);

    void processWillOpenConnection(WebProcessProxy*);
    void processWillCloseConnection(WebProcessProxy*);
    void processDidFinishLaunching(WebProcessProxy*);

    // Disconnect the process from the context.
    void disconnectProcess(WebProcessProxy*);

    StorageManager& storageManager() const { return *m_storageManager; }

    PassRefPtr<WebPageProxy> createWebPage(PageClient*, WebPageGroup*, WebPageProxy* relatedPage = 0);

    const String& injectedBundlePath() const { return m_injectedBundlePath; }

    DownloadProxy* download(WebPageProxy* initiatingPage, const WebCore::ResourceRequest&);

    void setInjectedBundleInitializationUserData(PassRefPtr<APIObject> userData) { m_injectedBundleInitializationUserData = userData; }

    void postMessageToInjectedBundle(const String&, APIObject*);

    // InjectedBundle client
    void didReceiveMessageFromInjectedBundle(const String&, APIObject*);
    void didReceiveSynchronousMessageFromInjectedBundle(const String&, APIObject*, RefPtr<APIObject>& returnData);

    void populateVisitedLinks();

#if ENABLE(NETSCAPE_PLUGIN_API)
    void setAdditionalPluginsDirectory(const String&);

    PluginInfoStore& pluginInfoStore() { return m_pluginInfoStore; }
#endif

    void setAlwaysUsesComplexTextCodePath(bool);
    void setShouldUseFontSmoothing(bool);
    
    void registerURLSchemeAsEmptyDocument(const String&);
    void registerURLSchemeAsSecure(const String&);
    void setDomainRelaxationForbiddenForURLScheme(const String&);
    void registerURLSchemeAsLocal(const String&);
    void registerURLSchemeAsNoAccess(const String&);
    void registerURLSchemeAsDisplayIsolated(const String&);
    void registerURLSchemeAsCORSEnabled(const String&);

    void addVisitedLink(const String&);
    void addVisitedLinkHash(WebCore::LinkHash);

    // MessageReceiver.
    virtual void didReceiveMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&) OVERRIDE;
    virtual void didReceiveSyncMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&, OwnPtr<CoreIPC::MessageEncoder>&) OVERRIDE;

    void setCacheModel(CacheModel);
    CacheModel cacheModel() const { return m_cacheModel; }

    void setDefaultRequestTimeoutInterval(double);

    void startMemorySampler(const double interval);
    void stopMemorySampler();

#if USE(SOUP)
    void setInitialHTTPCookieAcceptPolicy(HTTPCookieAcceptPolicy policy) { m_initialHTTPCookieAcceptPolicy = policy; }
#endif
    void setEnhancedAccessibility(bool);
    
    // Downloads.
    DownloadProxy* createDownloadProxy();
    WebDownloadClient& downloadClient() { return m_downloadClient; }

    WebHistoryClient& historyClient() { return m_historyClient; }
    WebContextClient& client() { return m_client; }

    WebIconDatabase* iconDatabase() const { return m_iconDatabase.get(); }
#if ENABLE(NETSCAPE_PLUGIN_API)
    WebPluginSiteDataManager* pluginSiteDataManager() const { return m_pluginSiteDataManager.get(); }
#endif

    struct Statistics {
        unsigned wkViewCount;
        unsigned wkPageCount;
        unsigned wkFrameCount;
    };
    static Statistics& statistics();    

    void setApplicationCacheDirectory(const String& dir) { m_overrideApplicationCacheDirectory = dir; }
    void setDatabaseDirectory(const String& dir) { m_overrideDatabaseDirectory = dir; }
    void setIconDatabasePath(const String&);
    String iconDatabasePath() const;
    void setLocalStorageDirectory(const String&);
    void setDiskCacheDirectory(const String& dir) { m_overrideDiskCacheDirectory = dir; }
    void setCookieStorageDirectory(const String& dir) { m_overrideCookieStorageDirectory = dir; }

    void allowSpecificHTTPSCertificateForHost(const WebCertificateInfo*, const String& host);

    WebProcessProxy* ensureSharedWebProcess();
    WebProcessProxy* createNewWebProcessRespectingProcessCountLimit(); // Will return an existing one if limit is met.
    void warmInitialProcess();

    bool shouldTerminate(WebProcessProxy*);

    void disableProcessTermination() { m_processTerminationEnabled = false; }
    void enableProcessTermination();

    // Defaults to false.
    void setHTTPPipeliningEnabled(bool);
    bool httpPipeliningEnabled() const;

    void getStatistics(uint32_t statisticsMask, PassRefPtr<DictionaryCallback>);
    
    void garbageCollectJavaScriptObjects();
    void setJavaScriptGarbageCollectorTimerEnabled(bool flag);

#if PLATFORM(MAC)
    static bool omitPDFSupport();
#endif

    void fullKeyboardAccessModeChanged(bool fullKeyboardAccessEnabled);

    void textCheckerStateChanged();

    PassRefPtr<ImmutableDictionary> plugInAutoStartOriginHashes() const;
    void setPlugInAutoStartOriginHashes(ImmutableDictionary&);
    void setPlugInAutoStartOrigins(ImmutableArray&);

    // Network Process Management

    void setUsesNetworkProcess(bool);
    bool usesNetworkProcess() const;

#if ENABLE(NETWORK_PROCESS)
    void ensureNetworkProcess();
    NetworkProcessProxy* networkProcess() { return m_networkProcess.get(); }
    void networkProcessCrashed(NetworkProcessProxy*);

    void getNetworkProcessConnection(PassRefPtr<Messages::WebProcessProxy::GetNetworkProcessConnection::DelayedReply>);
#endif


#if PLATFORM(MAC)
    void setProcessSuppressionEnabled(bool);
    bool processSuppressionEnabled() const { return m_processSuppressionEnabled; }
    bool canEnableProcessSuppressionForNetworkProcess() const;
    bool canEnableProcessSuppressionForWebProcess(const WebProcessProxy*) const;
    static bool canEnableProcessSuppressionForGlobalChildProcesses();
    void updateProcessSuppressionStateOfChildProcesses();
#endif

    static void willStartUsingPrivateBrowsing();
    static void willStopUsingPrivateBrowsing();

#if USE(SOUP)
    void setIgnoreTLSErrors(bool);
    bool ignoreTLSErrors() const { return m_ignoreTLSErrors; }
#endif

    static void setInvalidMessageCallback(void (*)(WKStringRef));
    static void didReceiveInvalidMessage(const CoreIPC::StringReference& messageReceiverName, const CoreIPC::StringReference& messageName);

    void processDidCachePage(WebProcessProxy*);

    bool isURLKnownHSTSHost(const String& urlString, bool privateBrowsingEnabled) const;
    void resetHSTSHosts();

private:
    WebContext(ProcessModel, const String& injectedBundlePath);
    void platformInitialize();

    void platformInitializeWebProcess(WebProcessCreationParameters&);
    void platformInvalidateContext();

    WebProcessProxy* createNewWebProcess();

    void requestWebContentStatistics(StatisticsRequest*);
    void requestNetworkingStatistics(StatisticsRequest*);

#if ENABLE(NETWORK_PROCESS)
    void platformInitializeNetworkProcess(NetworkProcessCreationParameters&);
#endif

#if PLATFORM(MAC)
    void getPasteboardTypes(const String& pasteboardName, Vector<String>& pasteboardTypes);
    void getPasteboardPathnamesForType(const String& pasteboardName, const String& pasteboardType, Vector<String>& pathnames);
    void getPasteboardStringForType(const String& pasteboardName, const String& pasteboardType, String&);
    void getPasteboardBufferForType(const String& pasteboardName, const String& pasteboardType, SharedMemory::Handle&, uint64_t& size);
    void pasteboardCopy(const String& fromPasteboard, const String& toPasteboard);
    void getPasteboardChangeCount(const String& pasteboardName, uint64_t& changeCount);
    void getPasteboardUniqueName(String& pasteboardName);
    void getPasteboardColor(const String& pasteboardName, WebCore::Color&);
    void getPasteboardURL(const String& pasteboardName, WTF::String&);
    void addPasteboardTypes(const String& pasteboardName, const Vector<String>& pasteboardTypes);
    void setPasteboardTypes(const String& pasteboardName, const Vector<String>& pasteboardTypes);
    void setPasteboardPathnamesForType(const String& pasteboardName, const String& pasteboardType, const Vector<String>& pathnames);
    void setPasteboardStringForType(const String& pasteboardName, const String& pasteboardType, const String&);
    void setPasteboardBufferForType(const String& pasteboardName, const String& pasteboardType, const SharedMemory::Handle&, uint64_t size);
#endif

#if !PLATFORM(MAC)
    // FIXME: This a dummy message, to avoid breaking the build for platforms that don't require
    // any synchronous messages, and should be removed when <rdar://problem/8775115> is fixed.
    void dummy(bool&);
#endif

    void didGetStatistics(const StatisticsData&, uint64_t callbackID);
        
    // Implemented in generated WebContextMessageReceiver.cpp
    void didReceiveWebContextMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&);
    void didReceiveSyncWebContextMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&, OwnPtr<CoreIPC::MessageEncoder>&);

    static void languageChanged(void* context);
    void languageChanged();

    String applicationCacheDirectory() const;
    String platformDefaultApplicationCacheDirectory() const;

    String databaseDirectory() const;
    String platformDefaultDatabaseDirectory() const;

    String platformDefaultIconDatabasePath() const;

    String localStorageDirectory() const;
    String platformDefaultLocalStorageDirectory() const;

    String diskCacheDirectory() const;
    String platformDefaultDiskCacheDirectory() const;

    String cookieStorageDirectory() const;
    String platformDefaultCookieStorageDirectory() const;

#if PLATFORM(MAC)
    void processSuppressionEnabledChanged();
    void registerNotificationObservers();
    void unregisterNotificationObservers();
#endif

#if ENABLE(CUSTOM_PROTOCOLS)
    void registerSchemeForCustomProtocol(const String&);
    void unregisterSchemeForCustomProtocol(const String&);
#endif

    void addPlugInAutoStartOriginHash(const String& pageOrigin, unsigned plugInOriginHash);
    void plugInDidReceiveUserInteraction(unsigned plugInOriginHash);

#if ENABLE(NETSCAPE_PLUGIN_API)
    // PluginInfoStoreClient:
    virtual void pluginInfoStoreDidLoadPlugins(PluginInfoStore*) OVERRIDE;
#endif

    CoreIPC::MessageReceiverMap m_messageReceiverMap;

    ProcessModel m_processModel;
    unsigned m_webProcessCountLimit; // The limit has no effect when process model is ProcessModelSharedSecondaryProcess.
    
    Vector<RefPtr<WebProcessProxy> > m_processes;
    bool m_haveInitialEmptyProcess;

    WebProcessProxy* m_processWithPageCache;

    RefPtr<WebPageGroup> m_defaultPageGroup;

    RefPtr<APIObject> m_injectedBundleInitializationUserData;
    String m_injectedBundlePath;
    WebContextInjectedBundleClient m_injectedBundleClient;

    WebContextClient m_client;
    WebContextConnectionClient m_connectionClient;
    WebDownloadClient m_downloadClient;
    WebHistoryClient m_historyClient;

#if ENABLE(NETSCAPE_PLUGIN_API)
    PluginInfoStore m_pluginInfoStore;
#endif
    VisitedLinkProvider m_visitedLinkProvider;
    PlugInAutoStartProvider m_plugInAutoStartProvider;
        
    HashSet<String> m_schemesToRegisterAsEmptyDocument;
    HashSet<String> m_schemesToRegisterAsSecure;
    HashSet<String> m_schemesToSetDomainRelaxationForbiddenFor;
    HashSet<String> m_schemesToRegisterAsLocal;
    HashSet<String> m_schemesToRegisterAsNoAccess;
    HashSet<String> m_schemesToRegisterAsDisplayIsolated;
    HashSet<String> m_schemesToRegisterAsCORSEnabled;

    bool m_alwaysUsesComplexTextCodePath;
    bool m_shouldUseFontSmoothing;

    // Messages that were posted before any pages were created.
    // The client should use initialization messages instead, so that a restarted process would get the same state.
    Vector<pair<String, RefPtr<APIObject> > > m_messagesToInjectedBundlePostedToEmptyContext;

    CacheModel m_cacheModel;

    bool m_memorySamplerEnabled;
    double m_memorySamplerInterval;

    RefPtr<WebIconDatabase> m_iconDatabase;
#if ENABLE(NETSCAPE_PLUGIN_API)
    RefPtr<WebPluginSiteDataManager> m_pluginSiteDataManager;
#endif

    RefPtr<StorageManager> m_storageManager;

    typedef HashMap<const char*, RefPtr<WebContextSupplement>, PtrHash<const char*> > WebContextSupplementMap;
    WebContextSupplementMap m_supplements;

#if USE(SOUP)
    HTTPCookieAcceptPolicy m_initialHTTPCookieAcceptPolicy;
#endif

#if PLATFORM(MAC)
    RetainPtr<NSObject> m_enhancedAccessibilityObserver;
    RetainPtr<NSObject> m_customSchemeRegisteredObserver;
    RetainPtr<NSObject> m_customSchemeUnregisteredObserver;

    RetainPtr<NSObject> m_automaticTextReplacementNotificationObserver;
    RetainPtr<NSObject> m_automaticSpellingCorrectionNotificationObserver;
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
    RetainPtr<NSObject> m_automaticQuoteSubstitutionNotificationObserver;
    RetainPtr<NSObject> m_automaticDashSubstitutionNotificationObserver;
#endif
#endif

    String m_overrideApplicationCacheDirectory;
    String m_overrideDatabaseDirectory;
    String m_overrideIconDatabasePath;
    String m_overrideLocalStorageDirectory;
    String m_overrideDiskCacheDirectory;
    String m_overrideCookieStorageDirectory;

    bool m_processTerminationEnabled;

#if ENABLE(NETWORK_PROCESS)
    bool m_usesNetworkProcess;
    RefPtr<NetworkProcessProxy> m_networkProcess;
#endif
    
    HashMap<uint64_t, RefPtr<DictionaryCallback> > m_dictionaryCallbacks;
    HashMap<uint64_t, RefPtr<StatisticsRequest> > m_statisticsRequests;

#if PLATFORM(MAC)
    bool m_processSuppressionEnabled;
#endif

#if USE(SOUP)
    bool m_ignoreTLSErrors;
#endif
};

template<typename U> inline void WebContext::sendToNetworkingProcess(const U& message)
{
    switch (m_processModel) {
    case ProcessModelSharedSecondaryProcess:
        if (!m_processes.isEmpty() && m_processes[0]->canSendMessage())
            m_processes[0]->send(message, 0);
        return;
    case ProcessModelMultipleSecondaryProcesses:
#if ENABLE(NETWORK_PROCESS)
        if (m_networkProcess->canSendMessage())
            m_networkProcess->send(message, 0);
        return;
#else
        break;
#endif
    }
    ASSERT_NOT_REACHED();
}

template<typename U> void WebContext::sendToNetworkingProcessRelaunchingIfNecessary(const U& message)
{
    switch (m_processModel) {
    case ProcessModelSharedSecondaryProcess:
        ensureSharedWebProcess();
        m_processes[0]->send(message, 0);
        return;
    case ProcessModelMultipleSecondaryProcesses:
#if ENABLE(NETWORK_PROCESS)
        ensureNetworkProcess();
        m_networkProcess->send(message, 0);
        return;
#else
        break;
#endif
    }
    ASSERT_NOT_REACHED();
}

template<typename U> inline void WebContext::sendToAllProcesses(const U& message)
{
    size_t processCount = m_processes.size();
    for (size_t i = 0; i < processCount; ++i) {
        WebProcessProxy* process = m_processes[i].get();
        if (process->canSendMessage())
            process->send(message, 0);
    }
}

template<typename U> void WebContext::sendToAllProcessesRelaunchingThemIfNecessary(const U& message)
{
// FIXME (Multi-WebProcess): WebContext doesn't track processes that have exited, so it cannot relaunch these. Perhaps this functionality won't be needed in this mode.
    if (m_processModel == ProcessModelSharedSecondaryProcess)
        ensureSharedWebProcess();
    sendToAllProcesses(message);
}

template<typename U> inline void WebContext::sendToOneProcess(const U& message)
{
    if (m_processModel == ProcessModelSharedSecondaryProcess)
        ensureSharedWebProcess();

    bool messageSent = false;
    size_t processCount = m_processes.size();
    for (size_t i = 0; i < processCount; ++i) {
        WebProcessProxy* process = m_processes[i].get();
        if (process->canSendMessage()) {
            process->send(message, 0);
            messageSent = true;
            break;
        }
    }

    if (!messageSent && m_processModel == ProcessModelMultipleSecondaryProcesses) {
        warmInitialProcess();
        RefPtr<WebProcessProxy> process = m_processes.last();
        if (process->canSendMessage())
            process->send(message, 0);
    }
}

} // namespace WebKit

#endif // WebContext_h
