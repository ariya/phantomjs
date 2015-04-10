/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#ifndef PluginProcessProxy_h
#define PluginProcessProxy_h

#if ENABLE(PLUGIN_PROCESS)

#include "ChildProcessProxy.h"
#include "Connection.h"
#include "PluginModuleInfo.h"
#include "PluginProcess.h"
#include "PluginProcessAttributes.h"
#include "ProcessLauncher.h"
#include "WebProcessProxyMessages.h"
#include <wtf/Deque.h>

#if PLATFORM(MAC)
#include <wtf/RetainPtr.h>
OBJC_CLASS NSObject;
OBJC_CLASS WKPlaceholderModalWindow;
#endif

// FIXME: This is platform specific.
namespace CoreIPC {
    class MachPort;
}

namespace WebKit {

class PluginProcessManager;
class WebPluginSiteDataManager;
class WebProcessProxy;
struct PluginProcessCreationParameters;

#if PLUGIN_ARCHITECTURE(X11)
struct RawPluginMetaData {
    String name;
    String description;
    String mimeDescription;
};
#endif

class PluginProcessProxy : public ChildProcessProxy {
public:
    static PassRefPtr<PluginProcessProxy> create(PluginProcessManager*, const PluginProcessAttributes&, uint64_t pluginProcessToken);
    ~PluginProcessProxy();

    const PluginProcessAttributes& pluginProcessAttributes() const { return m_pluginProcessAttributes; }
    uint64_t pluginProcessToken() const { return m_pluginProcessToken; }

    // Asks the plug-in process to create a new connection to a web process. The connection identifier will be
    // encoded in the given argument encoder and sent back to the connection of the given web process.
    void getPluginProcessConnection(PassRefPtr<Messages::WebProcessProxy::GetPluginProcessConnection::DelayedReply>);
    
    // Asks the plug-in process to get a list of domains for which the plug-in has data stored.
    void getSitesWithData(WebPluginSiteDataManager*, uint64_t callbackID);

    // Asks the plug-in process to clear the data for the given sites.
    void clearSiteData(WebPluginSiteDataManager*, const Vector<String>& sites, uint64_t flags, uint64_t maxAgeInSeconds, uint64_t callbackID);

    bool isValid() const { return m_connection; }

#if PLATFORM(MAC)
    void setProcessSuppressionEnabled(bool);

    // Returns whether the plug-in needs the heap to be marked executable.
    static bool pluginNeedsExecutableHeap(const PluginModuleInfo&);

    // Creates a property list in ~/Library/Preferences that contains all the MIME types supported by the plug-in.
    static bool createPropertyListFile(const PluginModuleInfo&);
#endif

#if PLUGIN_ARCHITECTURE(X11)
    static bool scanPlugin(const String& pluginPath, RawPluginMetaData& result);
#endif

private:
    PluginProcessProxy(PluginProcessManager*, const PluginProcessAttributes&, uint64_t pluginProcessToken);

    virtual void getLaunchOptions(ProcessLauncher::LaunchOptions&) OVERRIDE;
    void platformGetLaunchOptions(ProcessLauncher::LaunchOptions&, const PluginProcessAttributes&);

    void pluginProcessCrashedOrFailedToLaunch();

    // CoreIPC::Connection::Client
    virtual void didReceiveMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&) OVERRIDE;
    virtual void didReceiveSyncMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&, OwnPtr<CoreIPC::MessageEncoder>&) OVERRIDE;

    virtual void didClose(CoreIPC::Connection*) OVERRIDE;
    virtual void didReceiveInvalidMessage(CoreIPC::Connection*, CoreIPC::StringReference messageReceiverName, CoreIPC::StringReference messageName) OVERRIDE;

    // ProcessLauncher::Client
    virtual void didFinishLaunching(ProcessLauncher*, CoreIPC::Connection::Identifier);

    // Message handlers
    void didCreateWebProcessConnection(const CoreIPC::Attachment&, bool supportsAsynchronousPluginInitialization);
    void didGetSitesWithData(const Vector<String>& sites, uint64_t callbackID);
    void didClearSiteData(uint64_t callbackID);

#if PLATFORM(MAC)
    bool getPluginProcessSerialNumber(ProcessSerialNumber&);
    void makePluginProcessTheFrontProcess();
    void makeUIProcessTheFrontProcess();

    void setFullscreenWindowIsShowing(bool);
    void enterFullscreen();
    void exitFullscreen();

    void setModalWindowIsShowing(bool);
    void beginModal();
    void endModal();

    void applicationDidBecomeActive();
    void openPluginPreferencePane();
    void launchProcess(const String& launchPath, const Vector<String>& arguments, bool& result);
    void launchApplicationAtURL(const String& urlString, const Vector<String>& arguments, bool& result);
    void openURL(const String& url, bool& result, int32_t& status, String& launchedURLString);
#endif

    void platformInitializePluginProcess(PluginProcessCreationParameters& parameters);

    // The plug-in host process manager.
    PluginProcessManager* m_pluginProcessManager;

    PluginProcessAttributes m_pluginProcessAttributes;
    uint64_t m_pluginProcessToken;

    // The connection to the plug-in host process.
    RefPtr<CoreIPC::Connection> m_connection;

    Deque<RefPtr<Messages::WebProcessProxy::GetPluginProcessConnection::DelayedReply> > m_pendingConnectionReplies;

    Vector<uint64_t> m_pendingGetSitesRequests;
    HashMap<uint64_t, RefPtr<WebPluginSiteDataManager> > m_pendingGetSitesReplies;

    struct ClearSiteDataRequest {
        Vector<String> sites;
        uint64_t flags;
        uint64_t maxAgeInSeconds;
        uint64_t callbackID;
    };
    Vector<ClearSiteDataRequest> m_pendingClearSiteDataRequests;
    HashMap<uint64_t, RefPtr<WebPluginSiteDataManager> > m_pendingClearSiteDataReplies;

    // If createPluginConnection is called while the process is still launching we'll keep count of it and send a bunch of requests
    // when the process finishes launching.
    unsigned m_numPendingConnectionRequests;

#if PLATFORM(MAC)
    RetainPtr<NSObject> m_activationObserver;
    RetainPtr<WKPlaceholderModalWindow *> m_placeholderWindow;
    bool m_modalWindowIsShowing;
    bool m_fullscreenWindowIsShowing;
    unsigned m_preFullscreenAppPresentationOptions;
#endif
};

} // namespace WebKit

#endif // ENABLE(PLUGIN_PROCESS)

#endif // PluginProcessProxy_h
