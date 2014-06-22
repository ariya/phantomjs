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

#include "config.h"
#include "WebProcessConnection.h"

#if ENABLE(PLUGIN_PROCESS)

#include "ActivityAssertion.h"
#include "ArgumentCoders.h"
#include "ConnectionStack.h"
#include "NPObjectMessageReceiverMessages.h"
#include "NPRemoteObjectMap.h"
#include "PluginControllerProxy.h"
#include "PluginCreationParameters.h"
#include "PluginProcess.h"
#include "PluginProcessConnectionMessages.h"
#include "PluginProxyMessages.h"
#include "WebProcessConnectionMessages.h"
#include <WebCore/RunLoop.h>
#include <unistd.h>

using namespace WebCore;

namespace WebKit {

PassRefPtr<WebProcessConnection> WebProcessConnection::create(CoreIPC::Connection::Identifier connectionIdentifier)
{
    return adoptRef(new WebProcessConnection(connectionIdentifier));
}

WebProcessConnection::~WebProcessConnection()
{
    ASSERT(m_pluginControllers.isEmpty());
    ASSERT(!m_npRemoteObjectMap);
    ASSERT(!m_connection);
}
    
WebProcessConnection::WebProcessConnection(CoreIPC::Connection::Identifier connectionIdentifier)
{
    m_connection = CoreIPC::Connection::createServerConnection(connectionIdentifier, this, RunLoop::main());
    m_npRemoteObjectMap = NPRemoteObjectMap::create(m_connection.get());

    m_connection->setOnlySendMessagesAsDispatchWhenWaitingForSyncReplyWhenProcessingSuchAMessage(true);
    m_connection->open();
}

void WebProcessConnection::addPluginControllerProxy(PassOwnPtr<PluginControllerProxy> pluginController)
{
    uint64_t pluginInstanceID = pluginController->pluginInstanceID();

    ASSERT(!m_pluginControllers.contains(pluginInstanceID));
    m_pluginControllers.set(pluginInstanceID, pluginController);
}

void WebProcessConnection::destroyPluginControllerProxy(PluginControllerProxy* pluginController)
{
    // This may end up calling removePluginControllerProxy which ends up deleting
    // the WebProcessConnection object if this was the last object.
    pluginController->destroy();
}

void WebProcessConnection::removePluginControllerProxy(PluginControllerProxy* pluginController, Plugin* plugin)
{
    {
        ASSERT(m_pluginControllers.contains(pluginController->pluginInstanceID()));

        OwnPtr<PluginControllerProxy> pluginControllerOwnPtr = m_pluginControllers.take(pluginController->pluginInstanceID());
        ASSERT(pluginControllerOwnPtr == pluginController);
    }

    // Invalidate all objects related to this plug-in.
    if (plugin)
        m_npRemoteObjectMap->pluginDestroyed(plugin);

    if (!m_pluginControllers.isEmpty())
        return;

    m_npRemoteObjectMap = nullptr;

    // The last plug-in went away, close this connection.
    m_connection->invalidate();
    m_connection = nullptr;

    // This will cause us to be deleted.    
    PluginProcess::shared().removeWebProcessConnection(this);
}

void WebProcessConnection::setGlobalException(const String& exceptionString)
{
    CoreIPC::Connection* connection = ConnectionStack::shared().current();
    if (!connection)
        return;

    connection->sendSync(Messages::PluginProcessConnection::SetException(exceptionString), Messages::PluginProcessConnection::SetException::Reply(), 0);
}

void WebProcessConnection::didReceiveMessage(CoreIPC::Connection* connection, CoreIPC::MessageDecoder& decoder)
{
    ConnectionStack::CurrentConnectionPusher currentConnection(ConnectionStack::shared(), connection);

    if (decoder.messageReceiverName() == Messages::WebProcessConnection::messageReceiverName()) {
        didReceiveWebProcessConnectionMessage(connection, decoder);
        return;
    }

    if (!decoder.destinationID()) {
        ASSERT_NOT_REACHED();
        return;
    }

    PluginControllerProxy* pluginControllerProxy = m_pluginControllers.get(decoder.destinationID());
    if (!pluginControllerProxy)
        return;

    PluginController::PluginDestructionProtector protector(pluginControllerProxy->asPluginController());
    pluginControllerProxy->didReceivePluginControllerProxyMessage(connection, decoder);
}

void WebProcessConnection::didReceiveSyncMessage(CoreIPC::Connection* connection, CoreIPC::MessageDecoder& decoder, OwnPtr<CoreIPC::MessageEncoder>& replyEncoder)
{
    // Force all timers to run at full speed when processing a synchronous message
    ActivityAssertion activityAssertion(PluginProcess::shared());

    ConnectionStack::CurrentConnectionPusher currentConnection(ConnectionStack::shared(), connection);

    uint64_t destinationID = decoder.destinationID();

    if (!destinationID) {
        didReceiveSyncWebProcessConnectionMessage(connection, decoder, replyEncoder);
        return;
    }

    if (decoder.messageReceiverName() == Messages::NPObjectMessageReceiver::messageReceiverName()) {
        m_npRemoteObjectMap->didReceiveSyncMessage(connection, decoder, replyEncoder);
        return;
    }

    PluginControllerProxy* pluginControllerProxy = m_pluginControllers.get(decoder.destinationID());
    if (!pluginControllerProxy)
        return;

    PluginController::PluginDestructionProtector protector(pluginControllerProxy->asPluginController());
    pluginControllerProxy->didReceiveSyncPluginControllerProxyMessage(connection, decoder, replyEncoder);
}

void WebProcessConnection::didClose(CoreIPC::Connection*)
{
    // The web process crashed. Destroy all the plug-in controllers. Destroying the last plug-in controller
    // will cause the web process connection itself to be destroyed.
    Vector<PluginControllerProxy*> pluginControllers;
    HashMap<uint64_t, OwnPtr<PluginControllerProxy> >::iterator::Values it = m_pluginControllers.values().begin();
    HashMap<uint64_t, OwnPtr<PluginControllerProxy> >::iterator::Values end = m_pluginControllers.values().end();

    for (; it != end; ++it)
        pluginControllers.append(it->get());

    for (size_t i = 0; i < pluginControllers.size(); ++i)
        destroyPluginControllerProxy(pluginControllers[i]);
}

void WebProcessConnection::destroyPlugin(uint64_t pluginInstanceID, bool asynchronousCreationIncomplete)
{
    // Ensure we don't clamp any timers during destruction
    ActivityAssertion activityAssertion(PluginProcess::shared());

    PluginControllerProxy* pluginControllerProxy = m_pluginControllers.get(pluginInstanceID);
    
    // If there is no PluginControllerProxy then this plug-in doesn't exist yet and we probably have nothing to do.
    if (!pluginControllerProxy) {
        // If the plugin we're supposed to destroy was requested asynchronously and doesn't exist yet,
        // we need to flag the instance ID so it is not created later.
        if (asynchronousCreationIncomplete)
            m_asynchronousInstanceIDsToIgnore.add(pluginInstanceID);
        
        return;
    }
    
    destroyPluginControllerProxy(pluginControllerProxy);
}

void WebProcessConnection::didReceiveInvalidMessage(CoreIPC::Connection*, CoreIPC::StringReference, CoreIPC::StringReference)
{
    // FIXME: Implement.
}

void WebProcessConnection::createPluginInternal(const PluginCreationParameters& creationParameters, bool& result, bool& wantsWheelEvents, uint32_t& remoteLayerClientID)
{
    OwnPtr<PluginControllerProxy> pluginControllerProxy = PluginControllerProxy::create(this, creationParameters);

    PluginControllerProxy* pluginControllerProxyPtr = pluginControllerProxy.get();

    // Make sure to add the proxy to the map before initializing it, since the plug-in might call out to the web process from 
    // its NPP_New function. This will hand over ownership of the proxy to the web process connection.
    addPluginControllerProxy(pluginControllerProxy.release());

    // Now try to initialize the plug-in.
    result = pluginControllerProxyPtr->initialize(creationParameters);

    if (!result)
        return;

    wantsWheelEvents = pluginControllerProxyPtr->wantsWheelEvents();
#if PLATFORM(MAC)
    remoteLayerClientID = pluginControllerProxyPtr->remoteLayerClientID();
#else
    UNUSED_PARAM(remoteLayerClientID);
#endif
}

void WebProcessConnection::createPlugin(const PluginCreationParameters& creationParameters, PassRefPtr<Messages::WebProcessConnection::CreatePlugin::DelayedReply> reply)
{
    // Ensure we don't clamp any timers during initialization
    ActivityAssertion activityAssertion(PluginProcess::shared());

    PluginControllerProxy* pluginControllerProxy = m_pluginControllers.get(creationParameters.pluginInstanceID);

    // The controller proxy for the plug-in we're being asked to create synchronously might already exist if it was requested asynchronously before.
    if (pluginControllerProxy) {
        // It might still be in the middle of initialization in which case we have to let that initialization complete and respond to this message later.
        if (pluginControllerProxy->isInitializing()) {
            pluginControllerProxy->setInitializationReply(reply);
            return;
        }
        
        // If its initialization is complete then we need to respond to this message with the correct information about its creation.
#if PLATFORM(MAC)
        reply->send(true, pluginControllerProxy->wantsWheelEvents(), pluginControllerProxy->remoteLayerClientID());
#else
        reply->send(true, pluginControllerProxy->wantsWheelEvents(), 0);
#endif
        return;
    }
    
    // The plugin we're supposed to create might have been requested asynchronously before.
    // In that case we need to create it synchronously now but flag the instance ID so we don't recreate it asynchronously later.
    if (creationParameters.asynchronousCreationIncomplete)
        m_asynchronousInstanceIDsToIgnore.add(creationParameters.pluginInstanceID);
    
    bool result = false;
    bool wantsWheelEvents = false;
    uint32_t remoteLayerClientID = 0;
    createPluginInternal(creationParameters, result, wantsWheelEvents, remoteLayerClientID);
    
    reply->send(result, wantsWheelEvents, remoteLayerClientID);
}

void WebProcessConnection::createPluginAsynchronously(const PluginCreationParameters& creationParameters)
{
    // In the time since this plugin was requested asynchronously we might have created it synchronously or destroyed it.
    // In either of those cases we need to ignore this creation request.
    if (m_asynchronousInstanceIDsToIgnore.contains(creationParameters.pluginInstanceID)) {
        m_asynchronousInstanceIDsToIgnore.remove(creationParameters.pluginInstanceID);
        return;
    }
    
    // This version of CreatePlugin is only used by plug-ins that are known to behave when started asynchronously.
    bool result = false;
    bool wantsWheelEvents = false;
    uint32_t remoteLayerClientID = 0;
    
    if (creationParameters.artificialPluginInitializationDelayEnabled) {
        unsigned artificialPluginInitializationDelay = 5;
        sleep(artificialPluginInitializationDelay);
    }

    // Since plug-in creation can often message to the WebProcess synchronously (with NPP_Evaluate for example)
    // we need to make sure that the web process will handle the plug-in process's synchronous messages,
    // even if the web process is waiting on a synchronous reply itself.
    // Normally the plug-in process doesn't give its synchronous messages the special flag to allow for that.
    // We can force it to do so by incrementing the "DispatchMessageMarkedDispatchWhenWaitingForSyncReply" count.
    m_connection->incrementDispatchMessageMarkedDispatchWhenWaitingForSyncReplyCount();

    // The call to createPluginInternal can potentially cause the plug-in to be destroyed and
    // thus free the WebProcessConnection object. Protect it.
    RefPtr<WebProcessConnection> protect(this);
    createPluginInternal(creationParameters, result, wantsWheelEvents, remoteLayerClientID);

    if (!m_connection) {
        // createPluginInternal caused the connection to go away.
        return;
    }

    m_connection->decrementDispatchMessageMarkedDispatchWhenWaitingForSyncReplyCount();

    // If someone asked for this plug-in synchronously while it was in the middle of being created then we need perform the
    // synchronous reply instead of sending the asynchronous reply.
    PluginControllerProxy* pluginControllerProxy = m_pluginControllers.get(creationParameters.pluginInstanceID);
    ASSERT(pluginControllerProxy);
    if (RefPtr<Messages::WebProcessConnection::CreatePlugin::DelayedReply> delayedSyncReply = pluginControllerProxy->takeInitializationReply()) {
        delayedSyncReply->send(result, wantsWheelEvents, remoteLayerClientID);
        return;
    }

    // Otherwise, send the asynchronous results now.
    if (!result) {
        m_connection->sendSync(Messages::PluginProxy::DidFailToCreatePlugin(), Messages::PluginProxy::DidFailToCreatePlugin::Reply(), creationParameters.pluginInstanceID);
        return;
    }

    m_connection->sendSync(Messages::PluginProxy::DidCreatePlugin(wantsWheelEvents, remoteLayerClientID), Messages::PluginProxy::DidCreatePlugin::Reply(), creationParameters.pluginInstanceID);
}

} // namespace WebKit

#endif // ENABLE(PLUGIN_PROCESS)
