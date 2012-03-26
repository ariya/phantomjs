// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


#include "config.h"
#include "InspectorFrontend.h"
#include <wtf/text/StringConcatenate.h>
#include <wtf/text/CString.h>

#if ENABLE(INSPECTOR)

#include "InspectorFrontendChannel.h"
#include "InspectorValues.h"
#include "PlatformString.h"

namespace WebCore {



InspectorFrontend::InspectorFrontend(InspectorFrontendChannel* inspectorFrontendChannel)
    : m_inspectorFrontendChannel(inspectorFrontendChannel)
    , m_inspector(inspectorFrontendChannel)
    , m_page(inspectorFrontendChannel)
    , m_console(inspectorFrontendChannel)
    , m_network(inspectorFrontendChannel)
    , m_database(inspectorFrontendChannel)
    , m_domstorage(inspectorFrontendChannel)
    , m_applicationcache(inspectorFrontendChannel)
    , m_dom(inspectorFrontendChannel)
    , m_timeline(inspectorFrontendChannel)
    , m_debugger(inspectorFrontendChannel)
    , m_profiler(inspectorFrontendChannel)
    , m_worker(inspectorFrontendChannel)
{
}

void InspectorFrontend::Inspector::frontendReused()
{
    RefPtr<InspectorObject> frontendReusedMessage = InspectorObject::create();
    frontendReusedMessage->setString("method", "Inspector.frontendReused");
    m_inspectorFrontendChannel->sendMessageToFrontend(frontendReusedMessage->toJSONString());
}

void InspectorFrontend::Inspector::bringToFront()
{
    RefPtr<InspectorObject> bringToFrontMessage = InspectorObject::create();
    bringToFrontMessage->setString("method", "Inspector.bringToFront");
    m_inspectorFrontendChannel->sendMessageToFrontend(bringToFrontMessage->toJSONString());
}

void InspectorFrontend::Inspector::disconnectFromBackend()
{
    RefPtr<InspectorObject> disconnectFromBackendMessage = InspectorObject::create();
    disconnectFromBackendMessage->setString("method", "Inspector.disconnectFromBackend");
    m_inspectorFrontendChannel->sendMessageToFrontend(disconnectFromBackendMessage->toJSONString());
}

void InspectorFrontend::Inspector::reset()
{
    RefPtr<InspectorObject> resetMessage = InspectorObject::create();
    resetMessage->setString("method", "Inspector.reset");
    m_inspectorFrontendChannel->sendMessageToFrontend(resetMessage->toJSONString());
}

void InspectorFrontend::Inspector::showPanel(const String& panel)
{
    RefPtr<InspectorObject> showPanelMessage = InspectorObject::create();
    showPanelMessage->setString("method", "Inspector.showPanel");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setString("panel", panel);
    showPanelMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(showPanelMessage->toJSONString());
}

void InspectorFrontend::Inspector::startUserInitiatedDebugging()
{
    RefPtr<InspectorObject> startUserInitiatedDebuggingMessage = InspectorObject::create();
    startUserInitiatedDebuggingMessage->setString("method", "Inspector.startUserInitiatedDebugging");
    m_inspectorFrontendChannel->sendMessageToFrontend(startUserInitiatedDebuggingMessage->toJSONString());
}

void InspectorFrontend::Inspector::evaluateForTestInFrontend(int testCallId, const String& script)
{
    RefPtr<InspectorObject> evaluateForTestInFrontendMessage = InspectorObject::create();
    evaluateForTestInFrontendMessage->setString("method", "Inspector.evaluateForTestInFrontend");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setNumber("testCallId", testCallId);
    paramsObject->setString("script", script);
    evaluateForTestInFrontendMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(evaluateForTestInFrontendMessage->toJSONString());
}

void InspectorFrontend::Inspector::inspect(PassRefPtr<InspectorObject> object, PassRefPtr<InspectorObject> hints)
{
    RefPtr<InspectorObject> inspectMessage = InspectorObject::create();
    inspectMessage->setString("method", "Inspector.inspect");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setObject("object", object);
    paramsObject->setObject("hints", hints);
    inspectMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(inspectMessage->toJSONString());
}

void InspectorFrontend::Inspector::didCreateWorker(int id, const String& url, bool isShared)
{
    RefPtr<InspectorObject> didCreateWorkerMessage = InspectorObject::create();
    didCreateWorkerMessage->setString("method", "Inspector.didCreateWorker");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setNumber("id", id);
    paramsObject->setString("url", url);
    paramsObject->setBoolean("isShared", isShared);
    didCreateWorkerMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(didCreateWorkerMessage->toJSONString());
}

void InspectorFrontend::Inspector::didDestroyWorker(int id)
{
    RefPtr<InspectorObject> didDestroyWorkerMessage = InspectorObject::create();
    didDestroyWorkerMessage->setString("method", "Inspector.didDestroyWorker");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setNumber("id", id);
    didDestroyWorkerMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(didDestroyWorkerMessage->toJSONString());
}

void InspectorFrontend::Page::domContentEventFired(double timestamp)
{
    RefPtr<InspectorObject> domContentEventFiredMessage = InspectorObject::create();
    domContentEventFiredMessage->setString("method", "Page.domContentEventFired");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setNumber("timestamp", timestamp);
    domContentEventFiredMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(domContentEventFiredMessage->toJSONString());
}

void InspectorFrontend::Page::loadEventFired(double timestamp)
{
    RefPtr<InspectorObject> loadEventFiredMessage = InspectorObject::create();
    loadEventFiredMessage->setString("method", "Page.loadEventFired");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setNumber("timestamp", timestamp);
    loadEventFiredMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(loadEventFiredMessage->toJSONString());
}

void InspectorFrontend::Page::frameNavigated(PassRefPtr<InspectorObject> frame, const String& loaderId)
{
    RefPtr<InspectorObject> frameNavigatedMessage = InspectorObject::create();
    frameNavigatedMessage->setString("method", "Page.frameNavigated");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setObject("frame", frame);
    paramsObject->setString("loaderId", loaderId);
    frameNavigatedMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(frameNavigatedMessage->toJSONString());
}

void InspectorFrontend::Page::frameDetached(const String& frameId)
{
    RefPtr<InspectorObject> frameDetachedMessage = InspectorObject::create();
    frameDetachedMessage->setString("method", "Page.frameDetached");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setString("frameId", frameId);
    frameDetachedMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(frameDetachedMessage->toJSONString());
}

void InspectorFrontend::Console::messageAdded(PassRefPtr<InspectorObject> messageObj)
{
    RefPtr<InspectorObject> messageAddedMessage = InspectorObject::create();
    messageAddedMessage->setString("method", "Console.messageAdded");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setObject("messageObj", messageObj);
    messageAddedMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(messageAddedMessage->toJSONString());
}

void InspectorFrontend::Console::messageRepeatCountUpdated(int count)
{
    RefPtr<InspectorObject> messageRepeatCountUpdatedMessage = InspectorObject::create();
    messageRepeatCountUpdatedMessage->setString("method", "Console.messageRepeatCountUpdated");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setNumber("count", count);
    messageRepeatCountUpdatedMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(messageRepeatCountUpdatedMessage->toJSONString());
}

void InspectorFrontend::Console::messagesCleared()
{
    RefPtr<InspectorObject> messagesClearedMessage = InspectorObject::create();
    messagesClearedMessage->setString("method", "Console.messagesCleared");
    m_inspectorFrontendChannel->sendMessageToFrontend(messagesClearedMessage->toJSONString());
}

void InspectorFrontend::Network::requestWillBeSent(int identifier, const String& frameId, const String& loaderId, const String& documentURL, PassRefPtr<InspectorObject> request, double timestamp, PassRefPtr<InspectorArray> stackTrace, PassRefPtr<InspectorObject> redirectResponse)
{
    RefPtr<InspectorObject> requestWillBeSentMessage = InspectorObject::create();
    requestWillBeSentMessage->setString("method", "Network.requestWillBeSent");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setNumber("identifier", identifier);
    paramsObject->setString("frameId", frameId);
    paramsObject->setString("loaderId", loaderId);
    paramsObject->setString("documentURL", documentURL);
    paramsObject->setObject("request", request);
    paramsObject->setNumber("timestamp", timestamp);
    paramsObject->setArray("stackTrace", stackTrace);
    if (redirectResponse)
        paramsObject->setObject("redirectResponse", redirectResponse);
    requestWillBeSentMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(requestWillBeSentMessage->toJSONString());
}

void InspectorFrontend::Network::resourceMarkedAsCached(int identifier)
{
    RefPtr<InspectorObject> resourceMarkedAsCachedMessage = InspectorObject::create();
    resourceMarkedAsCachedMessage->setString("method", "Network.resourceMarkedAsCached");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setNumber("identifier", identifier);
    resourceMarkedAsCachedMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(resourceMarkedAsCachedMessage->toJSONString());
}

void InspectorFrontend::Network::responseReceived(int identifier, double timestamp, const String& type, PassRefPtr<InspectorObject> response)
{
    RefPtr<InspectorObject> responseReceivedMessage = InspectorObject::create();
    responseReceivedMessage->setString("method", "Network.responseReceived");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setNumber("identifier", identifier);
    paramsObject->setNumber("timestamp", timestamp);
    paramsObject->setString("type", type);
    paramsObject->setObject("response", response);
    responseReceivedMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(responseReceivedMessage->toJSONString());
}

void InspectorFrontend::Network::dataReceived(int identifier, double timestamp, int dataLength, int encodedDataLength)
{
    RefPtr<InspectorObject> dataReceivedMessage = InspectorObject::create();
    dataReceivedMessage->setString("method", "Network.dataReceived");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setNumber("identifier", identifier);
    paramsObject->setNumber("timestamp", timestamp);
    paramsObject->setNumber("dataLength", dataLength);
    paramsObject->setNumber("encodedDataLength", encodedDataLength);
    dataReceivedMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(dataReceivedMessage->toJSONString());
}

void InspectorFrontend::Network::loadingFinished(int identifier, double timestamp)
{
    RefPtr<InspectorObject> loadingFinishedMessage = InspectorObject::create();
    loadingFinishedMessage->setString("method", "Network.loadingFinished");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setNumber("identifier", identifier);
    paramsObject->setNumber("timestamp", timestamp);
    loadingFinishedMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(loadingFinishedMessage->toJSONString());
}

void InspectorFrontend::Network::loadingFailed(int identifier, double timestamp, const String& errorText, bool canceled)
{
    RefPtr<InspectorObject> loadingFailedMessage = InspectorObject::create();
    loadingFailedMessage->setString("method", "Network.loadingFailed");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setNumber("identifier", identifier);
    paramsObject->setNumber("timestamp", timestamp);
    paramsObject->setString("errorText", errorText);
    if (canceled)
        paramsObject->setBoolean("canceled", canceled);
    loadingFailedMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(loadingFailedMessage->toJSONString());
}

void InspectorFrontend::Network::resourceLoadedFromMemoryCache(const String& frameId, const String& loaderId, const String& documentURL, double timestamp, PassRefPtr<InspectorObject> resource)
{
    RefPtr<InspectorObject> resourceLoadedFromMemoryCacheMessage = InspectorObject::create();
    resourceLoadedFromMemoryCacheMessage->setString("method", "Network.resourceLoadedFromMemoryCache");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setString("frameId", frameId);
    paramsObject->setString("loaderId", loaderId);
    paramsObject->setString("documentURL", documentURL);
    paramsObject->setNumber("timestamp", timestamp);
    paramsObject->setObject("resource", resource);
    resourceLoadedFromMemoryCacheMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(resourceLoadedFromMemoryCacheMessage->toJSONString());
}

void InspectorFrontend::Network::initialContentSet(int identifier, const String& content, const String& type)
{
    RefPtr<InspectorObject> initialContentSetMessage = InspectorObject::create();
    initialContentSetMessage->setString("method", "Network.initialContentSet");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setNumber("identifier", identifier);
    paramsObject->setString("content", content);
    paramsObject->setString("type", type);
    initialContentSetMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(initialContentSetMessage->toJSONString());
}

void InspectorFrontend::Network::webSocketWillSendHandshakeRequest(int identifier, double timestamp, PassRefPtr<InspectorObject> request)
{
    RefPtr<InspectorObject> webSocketWillSendHandshakeRequestMessage = InspectorObject::create();
    webSocketWillSendHandshakeRequestMessage->setString("method", "Network.webSocketWillSendHandshakeRequest");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setNumber("identifier", identifier);
    paramsObject->setNumber("timestamp", timestamp);
    paramsObject->setObject("request", request);
    webSocketWillSendHandshakeRequestMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(webSocketWillSendHandshakeRequestMessage->toJSONString());
}

void InspectorFrontend::Network::webSocketHandshakeResponseReceived(int identifier, double timestamp, PassRefPtr<InspectorObject> response)
{
    RefPtr<InspectorObject> webSocketHandshakeResponseReceivedMessage = InspectorObject::create();
    webSocketHandshakeResponseReceivedMessage->setString("method", "Network.webSocketHandshakeResponseReceived");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setNumber("identifier", identifier);
    paramsObject->setNumber("timestamp", timestamp);
    paramsObject->setObject("response", response);
    webSocketHandshakeResponseReceivedMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(webSocketHandshakeResponseReceivedMessage->toJSONString());
}

void InspectorFrontend::Network::webSocketCreated(int identifier, const String& url)
{
    RefPtr<InspectorObject> webSocketCreatedMessage = InspectorObject::create();
    webSocketCreatedMessage->setString("method", "Network.webSocketCreated");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setNumber("identifier", identifier);
    paramsObject->setString("url", url);
    webSocketCreatedMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(webSocketCreatedMessage->toJSONString());
}

void InspectorFrontend::Network::webSocketClosed(int identifier, double timestamp)
{
    RefPtr<InspectorObject> webSocketClosedMessage = InspectorObject::create();
    webSocketClosedMessage->setString("method", "Network.webSocketClosed");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setNumber("identifier", identifier);
    paramsObject->setNumber("timestamp", timestamp);
    webSocketClosedMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(webSocketClosedMessage->toJSONString());
}

void InspectorFrontend::Database::addDatabase(PassRefPtr<InspectorObject> database)
{
    RefPtr<InspectorObject> addDatabaseMessage = InspectorObject::create();
    addDatabaseMessage->setString("method", "Database.addDatabase");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setObject("database", database);
    addDatabaseMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(addDatabaseMessage->toJSONString());
}

void InspectorFrontend::Database::sqlTransactionSucceeded(int transactionId, PassRefPtr<InspectorArray> columnNames, PassRefPtr<InspectorArray> values)
{
    RefPtr<InspectorObject> sqlTransactionSucceededMessage = InspectorObject::create();
    sqlTransactionSucceededMessage->setString("method", "Database.sqlTransactionSucceeded");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setNumber("transactionId", transactionId);
    paramsObject->setArray("columnNames", columnNames);
    paramsObject->setArray("values", values);
    sqlTransactionSucceededMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(sqlTransactionSucceededMessage->toJSONString());
}

void InspectorFrontend::Database::sqlTransactionFailed(int transactionId, PassRefPtr<InspectorObject> sqlError)
{
    RefPtr<InspectorObject> sqlTransactionFailedMessage = InspectorObject::create();
    sqlTransactionFailedMessage->setString("method", "Database.sqlTransactionFailed");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setNumber("transactionId", transactionId);
    paramsObject->setObject("sqlError", sqlError);
    sqlTransactionFailedMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(sqlTransactionFailedMessage->toJSONString());
}

void InspectorFrontend::DOMStorage::addDOMStorage(PassRefPtr<InspectorObject> storage)
{
    RefPtr<InspectorObject> addDOMStorageMessage = InspectorObject::create();
    addDOMStorageMessage->setString("method", "DOMStorage.addDOMStorage");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setObject("storage", storage);
    addDOMStorageMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(addDOMStorageMessage->toJSONString());
}

void InspectorFrontend::DOMStorage::updateDOMStorage(int storageId)
{
    RefPtr<InspectorObject> updateDOMStorageMessage = InspectorObject::create();
    updateDOMStorageMessage->setString("method", "DOMStorage.updateDOMStorage");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setNumber("storageId", storageId);
    updateDOMStorageMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(updateDOMStorageMessage->toJSONString());
}

void InspectorFrontend::ApplicationCache::updateApplicationCacheStatus(int status)
{
    RefPtr<InspectorObject> updateApplicationCacheStatusMessage = InspectorObject::create();
    updateApplicationCacheStatusMessage->setString("method", "ApplicationCache.updateApplicationCacheStatus");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setNumber("status", status);
    updateApplicationCacheStatusMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(updateApplicationCacheStatusMessage->toJSONString());
}

void InspectorFrontend::ApplicationCache::updateNetworkState(bool isNowOnline)
{
    RefPtr<InspectorObject> updateNetworkStateMessage = InspectorObject::create();
    updateNetworkStateMessage->setString("method", "ApplicationCache.updateNetworkState");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setBoolean("isNowOnline", isNowOnline);
    updateNetworkStateMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(updateNetworkStateMessage->toJSONString());
}

void InspectorFrontend::DOM::documentUpdated()
{
    RefPtr<InspectorObject> documentUpdatedMessage = InspectorObject::create();
    documentUpdatedMessage->setString("method", "DOM.documentUpdated");
    m_inspectorFrontendChannel->sendMessageToFrontend(documentUpdatedMessage->toJSONString());
}

void InspectorFrontend::DOM::setChildNodes(int parentId, PassRefPtr<InspectorArray> nodes)
{
    RefPtr<InspectorObject> setChildNodesMessage = InspectorObject::create();
    setChildNodesMessage->setString("method", "DOM.setChildNodes");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setNumber("parentId", parentId);
    paramsObject->setArray("nodes", nodes);
    setChildNodesMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(setChildNodesMessage->toJSONString());
}

void InspectorFrontend::DOM::attributesUpdated(int id, PassRefPtr<InspectorArray> attributes)
{
    RefPtr<InspectorObject> attributesUpdatedMessage = InspectorObject::create();
    attributesUpdatedMessage->setString("method", "DOM.attributesUpdated");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setNumber("id", id);
    paramsObject->setArray("attributes", attributes);
    attributesUpdatedMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(attributesUpdatedMessage->toJSONString());
}

void InspectorFrontend::DOM::characterDataModified(int id, const String& newValue)
{
    RefPtr<InspectorObject> characterDataModifiedMessage = InspectorObject::create();
    characterDataModifiedMessage->setString("method", "DOM.characterDataModified");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setNumber("id", id);
    paramsObject->setString("newValue", newValue);
    characterDataModifiedMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(characterDataModifiedMessage->toJSONString());
}

void InspectorFrontend::DOM::childNodeCountUpdated(int id, int newValue)
{
    RefPtr<InspectorObject> childNodeCountUpdatedMessage = InspectorObject::create();
    childNodeCountUpdatedMessage->setString("method", "DOM.childNodeCountUpdated");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setNumber("id", id);
    paramsObject->setNumber("newValue", newValue);
    childNodeCountUpdatedMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(childNodeCountUpdatedMessage->toJSONString());
}

void InspectorFrontend::DOM::childNodeInserted(int parentId, int prevId, PassRefPtr<InspectorObject> node)
{
    RefPtr<InspectorObject> childNodeInsertedMessage = InspectorObject::create();
    childNodeInsertedMessage->setString("method", "DOM.childNodeInserted");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setNumber("parentId", parentId);
    paramsObject->setNumber("prevId", prevId);
    paramsObject->setObject("node", node);
    childNodeInsertedMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(childNodeInsertedMessage->toJSONString());
}

void InspectorFrontend::DOM::childNodeRemoved(int parentId, int id)
{
    RefPtr<InspectorObject> childNodeRemovedMessage = InspectorObject::create();
    childNodeRemovedMessage->setString("method", "DOM.childNodeRemoved");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setNumber("parentId", parentId);
    paramsObject->setNumber("id", id);
    childNodeRemovedMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(childNodeRemovedMessage->toJSONString());
}

void InspectorFrontend::DOM::searchResults(PassRefPtr<InspectorArray> nodeIds)
{
    RefPtr<InspectorObject> searchResultsMessage = InspectorObject::create();
    searchResultsMessage->setString("method", "DOM.searchResults");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setArray("nodeIds", nodeIds);
    searchResultsMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(searchResultsMessage->toJSONString());
}

void InspectorFrontend::DOM::shadowRootUpdated(int hostId, PassRefPtr<InspectorObject> shadowRoot)
{
    RefPtr<InspectorObject> shadowRootUpdatedMessage = InspectorObject::create();
    shadowRootUpdatedMessage->setString("method", "DOM.shadowRootUpdated");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setNumber("hostId", hostId);
    paramsObject->setObject("shadowRoot", shadowRoot);
    shadowRootUpdatedMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(shadowRootUpdatedMessage->toJSONString());
}

void InspectorFrontend::Timeline::started()
{
    RefPtr<InspectorObject> startedMessage = InspectorObject::create();
    startedMessage->setString("method", "Timeline.started");
    m_inspectorFrontendChannel->sendMessageToFrontend(startedMessage->toJSONString());
}

void InspectorFrontend::Timeline::stopped()
{
    RefPtr<InspectorObject> stoppedMessage = InspectorObject::create();
    stoppedMessage->setString("method", "Timeline.stopped");
    m_inspectorFrontendChannel->sendMessageToFrontend(stoppedMessage->toJSONString());
}

void InspectorFrontend::Timeline::eventRecorded(PassRefPtr<InspectorObject> record)
{
    RefPtr<InspectorObject> eventRecordedMessage = InspectorObject::create();
    eventRecordedMessage->setString("method", "Timeline.eventRecorded");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setObject("record", record);
    eventRecordedMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(eventRecordedMessage->toJSONString());
}

void InspectorFrontend::Debugger::debuggerWasEnabled()
{
    RefPtr<InspectorObject> debuggerWasEnabledMessage = InspectorObject::create();
    debuggerWasEnabledMessage->setString("method", "Debugger.debuggerWasEnabled");
    m_inspectorFrontendChannel->sendMessageToFrontend(debuggerWasEnabledMessage->toJSONString());
}

void InspectorFrontend::Debugger::debuggerWasDisabled()
{
    RefPtr<InspectorObject> debuggerWasDisabledMessage = InspectorObject::create();
    debuggerWasDisabledMessage->setString("method", "Debugger.debuggerWasDisabled");
    m_inspectorFrontendChannel->sendMessageToFrontend(debuggerWasDisabledMessage->toJSONString());
}

void InspectorFrontend::Debugger::scriptParsed(const String& sourceID, const String& url, int startLine, int startColumn, int endLine, int endColumn, bool isContentScript)
{
    RefPtr<InspectorObject> scriptParsedMessage = InspectorObject::create();
    scriptParsedMessage->setString("method", "Debugger.scriptParsed");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setString("sourceID", sourceID);
    paramsObject->setString("url", url);
    paramsObject->setNumber("startLine", startLine);
    paramsObject->setNumber("startColumn", startColumn);
    paramsObject->setNumber("endLine", endLine);
    paramsObject->setNumber("endColumn", endColumn);
    if (isContentScript)
        paramsObject->setBoolean("isContentScript", isContentScript);
    scriptParsedMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(scriptParsedMessage->toJSONString());
}

void InspectorFrontend::Debugger::scriptFailedToParse(const String& url, const String& data, int firstLine, int errorLine, const String& errorMessage)
{
    RefPtr<InspectorObject> scriptFailedToParseMessage = InspectorObject::create();
    scriptFailedToParseMessage->setString("method", "Debugger.scriptFailedToParse");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setString("url", url);
    paramsObject->setString("data", data);
    paramsObject->setNumber("firstLine", firstLine);
    paramsObject->setNumber("errorLine", errorLine);
    paramsObject->setString("errorMessage", errorMessage);
    scriptFailedToParseMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(scriptFailedToParseMessage->toJSONString());
}

void InspectorFrontend::Debugger::breakpointResolved(const String& breakpointId, PassRefPtr<InspectorObject> location)
{
    RefPtr<InspectorObject> breakpointResolvedMessage = InspectorObject::create();
    breakpointResolvedMessage->setString("method", "Debugger.breakpointResolved");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setString("breakpointId", breakpointId);
    paramsObject->setObject("location", location);
    breakpointResolvedMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(breakpointResolvedMessage->toJSONString());
}

void InspectorFrontend::Debugger::paused(PassRefPtr<InspectorObject> details)
{
    RefPtr<InspectorObject> pausedMessage = InspectorObject::create();
    pausedMessage->setString("method", "Debugger.paused");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setObject("details", details);
    pausedMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(pausedMessage->toJSONString());
}

void InspectorFrontend::Debugger::resumed()
{
    RefPtr<InspectorObject> resumedMessage = InspectorObject::create();
    resumedMessage->setString("method", "Debugger.resumed");
    m_inspectorFrontendChannel->sendMessageToFrontend(resumedMessage->toJSONString());
}

void InspectorFrontend::Profiler::profilerWasEnabled()
{
    RefPtr<InspectorObject> profilerWasEnabledMessage = InspectorObject::create();
    profilerWasEnabledMessage->setString("method", "Profiler.profilerWasEnabled");
    m_inspectorFrontendChannel->sendMessageToFrontend(profilerWasEnabledMessage->toJSONString());
}

void InspectorFrontend::Profiler::profilerWasDisabled()
{
    RefPtr<InspectorObject> profilerWasDisabledMessage = InspectorObject::create();
    profilerWasDisabledMessage->setString("method", "Profiler.profilerWasDisabled");
    m_inspectorFrontendChannel->sendMessageToFrontend(profilerWasDisabledMessage->toJSONString());
}

void InspectorFrontend::Profiler::addProfileHeader(PassRefPtr<InspectorObject> header)
{
    RefPtr<InspectorObject> addProfileHeaderMessage = InspectorObject::create();
    addProfileHeaderMessage->setString("method", "Profiler.addProfileHeader");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setObject("header", header);
    addProfileHeaderMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(addProfileHeaderMessage->toJSONString());
}

void InspectorFrontend::Profiler::addHeapSnapshotChunk(int uid, const String& chunk)
{
    RefPtr<InspectorObject> addHeapSnapshotChunkMessage = InspectorObject::create();
    addHeapSnapshotChunkMessage->setString("method", "Profiler.addHeapSnapshotChunk");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setNumber("uid", uid);
    paramsObject->setString("chunk", chunk);
    addHeapSnapshotChunkMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(addHeapSnapshotChunkMessage->toJSONString());
}

void InspectorFrontend::Profiler::finishHeapSnapshot(int uid)
{
    RefPtr<InspectorObject> finishHeapSnapshotMessage = InspectorObject::create();
    finishHeapSnapshotMessage->setString("method", "Profiler.finishHeapSnapshot");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setNumber("uid", uid);
    finishHeapSnapshotMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(finishHeapSnapshotMessage->toJSONString());
}

void InspectorFrontend::Profiler::setRecordingProfile(bool isProfiling)
{
    RefPtr<InspectorObject> setRecordingProfileMessage = InspectorObject::create();
    setRecordingProfileMessage->setString("method", "Profiler.setRecordingProfile");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setBoolean("isProfiling", isProfiling);
    setRecordingProfileMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(setRecordingProfileMessage->toJSONString());
}

void InspectorFrontend::Profiler::resetProfiles()
{
    RefPtr<InspectorObject> resetProfilesMessage = InspectorObject::create();
    resetProfilesMessage->setString("method", "Profiler.resetProfiles");
    m_inspectorFrontendChannel->sendMessageToFrontend(resetProfilesMessage->toJSONString());
}

void InspectorFrontend::Profiler::reportHeapSnapshotProgress(int done, int total)
{
    RefPtr<InspectorObject> reportHeapSnapshotProgressMessage = InspectorObject::create();
    reportHeapSnapshotProgressMessage->setString("method", "Profiler.reportHeapSnapshotProgress");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setNumber("done", done);
    paramsObject->setNumber("total", total);
    reportHeapSnapshotProgressMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(reportHeapSnapshotProgressMessage->toJSONString());
}

void InspectorFrontend::Worker::workerCreated(int workerId)
{
    RefPtr<InspectorObject> workerCreatedMessage = InspectorObject::create();
    workerCreatedMessage->setString("method", "Worker.workerCreated");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setNumber("workerId", workerId);
    workerCreatedMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(workerCreatedMessage->toJSONString());
}

void InspectorFrontend::Worker::dispatchMessageFromWorker(int workerId, PassRefPtr<InspectorObject> message)
{
    RefPtr<InspectorObject> dispatchMessageFromWorkerMessage = InspectorObject::create();
    dispatchMessageFromWorkerMessage->setString("method", "Worker.dispatchMessageFromWorker");
    RefPtr<InspectorObject> paramsObject = InspectorObject::create();
    paramsObject->setNumber("workerId", workerId);
    paramsObject->setObject("message", message);
    dispatchMessageFromWorkerMessage->setObject("params", paramsObject);
    m_inspectorFrontendChannel->sendMessageToFrontend(dispatchMessageFromWorkerMessage->toJSONString());
}


} // namespace WebCore

#endif // ENABLE(INSPECTOR)
