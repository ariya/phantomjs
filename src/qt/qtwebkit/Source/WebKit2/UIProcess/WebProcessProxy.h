/*
 * Copyright (C) 2010, 2011 Apple Inc. All rights reserved.
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

#ifndef WebProcessProxy_h
#define WebProcessProxy_h

#include "ChildProcessProxy.h"
#include "MessageReceiverMap.h"
#include "PlatformProcessIdentifier.h"
#include "PluginInfoStore.h"
#include "ProcessLauncher.h"
#include "ResponsivenessTimer.h"
#include "WebConnectionToWebProcess.h"
#include "WebPageProxy.h"
#include "WebProcessProxyMessages.h"
#include <WebCore/LinkHash.h>
#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

#if ENABLE(CUSTOM_PROTOCOLS)
#include "CustomProtocolManagerProxy.h"
#endif

namespace WebCore {
class KURL;
struct PluginInfo;
};

namespace WebKit {

class DownloadProxyMap;
class WebBackForwardListItem;
class WebContext;
class WebPageGroup;
struct WebNavigationDataStore;

class WebProcessProxy : public ChildProcessProxy, ResponsivenessTimer::Client {
public:
    typedef HashMap<uint64_t, RefPtr<WebBackForwardListItem> > WebBackForwardListItemMap;
    typedef HashMap<uint64_t, RefPtr<WebFrameProxy> > WebFrameProxyMap;
    typedef HashMap<uint64_t, WebPageProxy*> WebPageProxyMap;

    static PassRefPtr<WebProcessProxy> create(PassRefPtr<WebContext>);
    ~WebProcessProxy();

    static WebProcessProxy* fromConnection(CoreIPC::Connection* connection)
    {
        return static_cast<WebProcessProxy*>(ChildProcessProxy::fromConnection(connection));
    }

    WebConnection* webConnection() const { return m_webConnection.get(); }

    WebContext* context() const { return m_context.get(); }

    static WebPageProxy* webPage(uint64_t pageID);
    PassRefPtr<WebPageProxy> createWebPage(PageClient*, WebContext*, WebPageGroup*);
    void addExistingWebPage(WebPageProxy*, uint64_t pageID);
    void removeWebPage(uint64_t pageID);
    Vector<WebPageProxy*> pages() const;

    WebBackForwardListItem* webBackForwardItem(uint64_t itemID) const;

    ResponsivenessTimer* responsivenessTimer() { return &m_responsivenessTimer; }

    WebFrameProxy* webFrame(uint64_t) const;
    bool canCreateFrame(uint64_t frameID) const;
    void frameCreated(uint64_t, WebFrameProxy*);
    void disconnectFramesFromPage(WebPageProxy*); // Including main frame.
    size_t frameCountInPage(WebPageProxy*) const; // Including main frame.

    void updateTextCheckerState();

    void registerNewWebBackForwardListItem(WebBackForwardListItem*);

    void willAcquireUniversalFileReadSandboxExtension() { m_mayHaveUniversalFileReadSandboxExtension = true; }
    void assumeReadAccessToBaseURL(const String&);

    bool checkURLReceivedFromWebProcess(const String&);
    bool checkURLReceivedFromWebProcess(const WebCore::KURL&);

    static bool fullKeyboardAccessEnabled();

    DownloadProxy* createDownloadProxy();

    void pageVisibilityChanged(WebPageProxy*);
    void pagePreferencesChanged(WebPageProxy*);

    void didSaveToPageCache();
    void releasePageCache();

#if PLATFORM(MAC)
    bool allPagesAreProcessSuppressible() const;
    static bool pageIsProcessSuppressible(WebPageProxy*);
    void updateProcessSuppressionState();
#endif

    void enableSuddenTermination();
    void disableSuddenTermination();

    void requestTermination();

private:
    explicit WebProcessProxy(PassRefPtr<WebContext>);

    // From ChildProcessProxy
    virtual void getLaunchOptions(ProcessLauncher::LaunchOptions&) OVERRIDE;
    void platformGetLaunchOptions(ProcessLauncher::LaunchOptions&);
    virtual void connectionWillOpen(CoreIPC::Connection*) OVERRIDE;
    virtual void connectionWillClose(CoreIPC::Connection*) OVERRIDE;

    // Called when the web process has crashed or we know that it will terminate soon.
    // Will potentially cause the WebProcessProxy object to be freed.
    void disconnect();

    // CoreIPC message handlers.
    void addBackForwardItem(uint64_t itemID, const String& originalURLString, const String& urlString, const String& title, const CoreIPC::DataReference& backForwardData);
    void didDestroyFrame(uint64_t);
    
    void shouldTerminate(bool& shouldTerminate);

    // Plugins
#if ENABLE(NETSCAPE_PLUGIN_API)
    void getPlugins(bool refresh, Vector<WebCore::PluginInfo>& plugins);
#endif // ENABLE(NETSCAPE_PLUGIN_API)
#if ENABLE(PLUGIN_PROCESS)
    void getPluginProcessConnection(uint64_t pluginProcessToken, PassRefPtr<Messages::WebProcessProxy::GetPluginProcessConnection::DelayedReply>);
#elif ENABLE(NETSCAPE_PLUGIN_API)
    void didGetSitesWithPluginData(const Vector<String>& sites, uint64_t callbackID);
    void didClearPluginSiteData(uint64_t callbackID);
#endif
#if ENABLE(NETWORK_PROCESS)
    void getNetworkProcessConnection(PassRefPtr<Messages::WebProcessProxy::GetNetworkProcessConnection::DelayedReply>);
#endif
#if ENABLE(SHARED_WORKER_PROCESS)
    void getSharedWorkerProcessConnection(const String& url, const String& name, PassRefPtr<Messages::WebProcessProxy::GetSharedWorkerProcessConnection::DelayedReply>);
#endif

    // CoreIPC::Connection::Client
    friend class WebConnectionToWebProcess;
    virtual void didReceiveMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&) OVERRIDE;
    virtual void didReceiveSyncMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&, OwnPtr<CoreIPC::MessageEncoder>&) OVERRIDE;
    virtual void didClose(CoreIPC::Connection*) OVERRIDE;
    virtual void didReceiveInvalidMessage(CoreIPC::Connection*, CoreIPC::StringReference messageReceiverName, CoreIPC::StringReference messageName) OVERRIDE;

    // ResponsivenessTimer::Client
    void didBecomeUnresponsive(ResponsivenessTimer*) OVERRIDE;
    void interactionOccurredWhileUnresponsive(ResponsivenessTimer*) OVERRIDE;
    void didBecomeResponsive(ResponsivenessTimer*) OVERRIDE;

    // ProcessLauncher::Client
    virtual void didFinishLaunching(ProcessLauncher*, CoreIPC::Connection::Identifier) OVERRIDE;

    // History client
    void didNavigateWithNavigationData(uint64_t pageID, const WebNavigationDataStore&, uint64_t frameID);
    void didPerformClientRedirect(uint64_t pageID, const String& sourceURLString, const String& destinationURLString, uint64_t frameID);
    void didPerformServerRedirect(uint64_t pageID, const String& sourceURLString, const String& destinationURLString, uint64_t frameID);
    void didUpdateHistoryTitle(uint64_t pageID, const String& title, const String& url, uint64_t frameID);

    // Implemented in generated WebProcessProxyMessageReceiver.cpp
    void didReceiveWebProcessProxyMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&);
    void didReceiveSyncWebProcessProxyMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&, OwnPtr<CoreIPC::MessageEncoder>&);

    bool canTerminateChildProcess();

    ResponsivenessTimer m_responsivenessTimer;
    
    RefPtr<WebConnectionToWebProcess> m_webConnection;
    RefPtr<WebContext> m_context;

    bool m_mayHaveUniversalFileReadSandboxExtension; // True if a read extension for "/" was ever granted - we don't track whether WebProcess still has it.
    HashSet<String> m_localPathsWithAssumedReadAccess;

    WebPageProxyMap m_pageMap;
    WebFrameProxyMap m_frameMap;
    WebBackForwardListItemMap m_backForwardListItemMap;

    OwnPtr<DownloadProxyMap> m_downloadProxyMap;

#if ENABLE(CUSTOM_PROTOCOLS)
    CustomProtocolManagerProxy m_customProtocolManagerProxy;
#endif

#if PLATFORM(MAC)
    HashSet<uint64_t> m_processSuppressiblePages;
    bool m_processSuppressionEnabled;
#endif
};
    
} // namespace WebKit

#endif // WebProcessProxy_h
