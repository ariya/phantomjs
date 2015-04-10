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

#ifndef WebProcessConnection_h
#define WebProcessConnection_h

#if ENABLE(PLUGIN_PROCESS)

#include "Connection.h"
#include "Plugin.h"
#include "WebProcessConnectionMessages.h"
#include <wtf/HashSet.h>
#include <wtf/RefCounted.h>

namespace WebKit {

class NPRemoteObjectMap;
class PluginControllerProxy;
struct PluginCreationParameters;
    
// A connection from a plug-in process to a web process.

class WebProcessConnection : public RefCounted<WebProcessConnection>, CoreIPC::Connection::Client {
public:
    static PassRefPtr<WebProcessConnection> create(CoreIPC::Connection::Identifier);
    virtual ~WebProcessConnection();

    CoreIPC::Connection* connection() const { return m_connection.get(); }
    NPRemoteObjectMap* npRemoteObjectMap() const { return m_npRemoteObjectMap.get(); }

    void removePluginControllerProxy(PluginControllerProxy*, Plugin*);

    static void setGlobalException(const String&);

private:
    WebProcessConnection(CoreIPC::Connection::Identifier);

    void addPluginControllerProxy(PassOwnPtr<PluginControllerProxy>);

    void destroyPluginControllerProxy(PluginControllerProxy*);

    // CoreIPC::Connection::Client
    virtual void didReceiveMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&) OVERRIDE;
    virtual void didReceiveSyncMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&, OwnPtr<CoreIPC::MessageEncoder>&) OVERRIDE;
    virtual void didClose(CoreIPC::Connection*);
    virtual void didReceiveInvalidMessage(CoreIPC::Connection*, CoreIPC::StringReference messageReceiverName, CoreIPC::StringReference messageName);

    // Message handlers.
    void didReceiveWebProcessConnectionMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&);
    void didReceiveSyncWebProcessConnectionMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&, OwnPtr<CoreIPC::MessageEncoder>&);
    void createPlugin(const PluginCreationParameters&, PassRefPtr<Messages::WebProcessConnection::CreatePlugin::DelayedReply>);
    void createPluginAsynchronously(const PluginCreationParameters&);
    void destroyPlugin(uint64_t pluginInstanceID, bool asynchronousCreationIncomplete);
    
    void createPluginInternal(const PluginCreationParameters&, bool& result, bool& wantsWheelEvents, uint32_t& remoteLayerClientID);

    RefPtr<CoreIPC::Connection> m_connection;

    HashMap<uint64_t, OwnPtr<PluginControllerProxy> > m_pluginControllers;
    RefPtr<NPRemoteObjectMap> m_npRemoteObjectMap;
    HashSet<uint64_t> m_asynchronousInstanceIDsToIgnore;
};

} // namespace WebKit

#endif // ENABLE(PLUGIN_PROCESS)


#endif // WebProcessConnection_h
