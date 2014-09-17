/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "InspectorResourceAgent.h"

#if ENABLE(INSPECTOR)

#include "CachedResource.h"
#include "CachedResourceLoader.h"
#include "DocumentLoader.h"
#include "EventsCollector.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "HTTPHeaderMap.h"
#include "InspectorFrontend.h"
#include "InspectorFrontendChannel.h"
#include "InspectorFrontendProxy.h"
#include "InspectorPageAgent.h"
#include "InspectorState.h"
#include "InspectorValues.h"
#include "InstrumentingAgents.h"
#include "KURL.h"
#include "ProgressTracker.h"
#include "ResourceError.h"
#include "ResourceRequest.h"
#include "ResourceResponse.h"
#include "ScriptCallStack.h"
#include "ScriptCallStackFactory.h"
#include "WebSocketHandshakeRequest.h"
#include "WebSocketHandshakeResponse.h"

#include <wtf/CurrentTime.h>
#include <wtf/HexNumber.h>
#include <wtf/ListHashSet.h>
#include <wtf/RefPtr.h>
#include <wtf/text/StringBuilder.h>

namespace WebCore {

namespace ResourceAgentState {
static const char resourceAgentEnabled[] = "resourceAgentEnabled";
static const char extraRequestHeaders[] = "extraRequestHeaders";
}

void InspectorResourceAgent::setFrontend(InspectorFrontend* frontend)
{
    m_frontend = frontend->network();
    if (backgroundEventsCollectionEnabled()) {
        // Insert Message Proxy in receiver chain.
        InspectorFrontendChannel* client = m_frontend->getInspectorFrontendChannel();
        m_inspectorFrontendProxy->setInspectorFrontendChannel(client);
        m_frontend->setInspectorFrontendChannel(m_inspectorFrontendProxy.get());
        m_eventsCollector->sendCollectedEvents(client);
    }
}

void InspectorResourceAgent::clearFrontend()
{
    if (backgroundEventsCollectionEnabled()) {
        m_inspectorFrontendProxy->setInspectorFrontendChannel(0);
        m_frontend = m_mockFrontend.get();
    } else
        m_frontend = 0;

    m_userAgentOverride = "";
    disable(0);
}

void InspectorResourceAgent::restore()
{
    if (m_state->getBoolean(ResourceAgentState::resourceAgentEnabled))
        enable();
}

static PassRefPtr<InspectorObject> buildObjectForHeaders(const HTTPHeaderMap& headers)
{
    RefPtr<InspectorObject> headersObject = InspectorObject::create();
    HTTPHeaderMap::const_iterator end = headers.end();
    for (HTTPHeaderMap::const_iterator it = headers.begin(); it != end; ++it)
        headersObject->setString(it->first.string(), it->second);
    return headersObject;
}

static PassRefPtr<InspectorObject> buildObjectForTiming(const ResourceLoadTiming& timing)
{
    RefPtr<InspectorObject> timingObject = InspectorObject::create();
    timingObject->setNumber("requestTime", timing.requestTime);
    timingObject->setNumber("proxyStart", timing.proxyStart);
    timingObject->setNumber("proxyEnd", timing.proxyEnd);
    timingObject->setNumber("dnsStart", timing.dnsStart);
    timingObject->setNumber("dnsEnd", timing.dnsEnd);
    timingObject->setNumber("connectStart", timing.connectStart);
    timingObject->setNumber("connectEnd", timing.connectEnd);
    timingObject->setNumber("sslStart", timing.sslStart);
    timingObject->setNumber("sslEnd", timing.sslEnd);
    timingObject->setNumber("sendStart", timing.sendStart);
    timingObject->setNumber("sendEnd", timing.sendEnd);
    timingObject->setNumber("receiveHeadersEnd", timing.receiveHeadersEnd);
    return timingObject;
}

static PassRefPtr<InspectorObject> buildObjectForResourceRequest(const ResourceRequest& request)
{
    RefPtr<InspectorObject> requestObject = InspectorObject::create();
    requestObject->setString("url", request.url().string());
    requestObject->setString("method", request.httpMethod());
    requestObject->setObject("headers", buildObjectForHeaders(request.httpHeaderFields()));
    if (request.httpBody() && !request.httpBody()->isEmpty())
        requestObject->setString("postData", request.httpBody()->flattenToString());
    return requestObject;
}

static PassRefPtr<InspectorObject> buildObjectForResourceResponse(const ResourceResponse& response)
{
    if (response.isNull())
        return 0;

    RefPtr<InspectorObject> responseObject = InspectorObject::create();
    responseObject->setNumber("status", response.resourceLoadInfo() ? response.resourceLoadInfo()->httpStatusCode : response.httpStatusCode());
    responseObject->setString("statusText", response.resourceLoadInfo() ? response.resourceLoadInfo()->httpStatusText : response.httpStatusText());

    responseObject->setString("mimeType", response.mimeType());
    responseObject->setBoolean("connectionReused", response.connectionReused());
    responseObject->setNumber("connectionID", response.connectionID());
    responseObject->setBoolean("fromDiskCache", response.wasCached());
    if (response.resourceLoadTiming())
        responseObject->setObject("timing", buildObjectForTiming(*response.resourceLoadTiming()));

    if (response.resourceLoadInfo()) {
        responseObject->setObject("headers", buildObjectForHeaders(response.resourceLoadInfo()->responseHeaders));
        if (!response.resourceLoadInfo()->responseHeadersText.isEmpty())
            responseObject->setString("headersText", response.resourceLoadInfo()->responseHeadersText);

        responseObject->setObject("requestHeaders", buildObjectForHeaders(response.resourceLoadInfo()->requestHeaders));
        if (!response.resourceLoadInfo()->requestHeadersText.isEmpty())
            responseObject->setString("requestHeadersText", response.resourceLoadInfo()->requestHeadersText);
    } else
        responseObject->setObject("headers", buildObjectForHeaders(response.httpHeaderFields()));

    return responseObject;
}

static PassRefPtr<InspectorObject> buildObjectForCachedResource(const CachedResource& cachedResource)
{
    RefPtr<InspectorObject> resourceObject = InspectorObject::create();
    resourceObject->setString("url", cachedResource.url());
    resourceObject->setString("type", InspectorPageAgent::cachedResourceTypeString(cachedResource));
    resourceObject->setNumber("bodySize", cachedResource.encodedSize());
    RefPtr<InspectorObject> resourceResponse = buildObjectForResourceResponse(cachedResource.response());
    if (resourceResponse)
        resourceObject->setObject("response", resourceResponse);
    return resourceObject;
}

InspectorResourceAgent::~InspectorResourceAgent()
{
    if (m_state->getBoolean(ResourceAgentState::resourceAgentEnabled)) {
        ErrorString error;
        disable(&error);
    }
    ASSERT(!m_instrumentingAgents->inspectorResourceAgent());
}

void InspectorResourceAgent::willSendRequest(unsigned long identifier, DocumentLoader* loader, ResourceRequest& request, const ResourceResponse& redirectResponse)
{
    RefPtr<InspectorObject> headers = m_state->getObject(ResourceAgentState::extraRequestHeaders);

    if (headers) {
        InspectorObject::const_iterator end = headers->end();
        for (InspectorObject::const_iterator it = headers->begin(); it != end; ++it) {
            String value;
            if (it->second->asString(&value))
                request.setHTTPHeaderField(it->first, value);
        }
    }

    request.setReportLoadTiming(true);
    request.setReportRawHeaders(true);

    RefPtr<ScriptCallStack> callStack = createScriptCallStack(ScriptCallStack::maxCallStackSizeToCapture, true);
    RefPtr<InspectorArray> callStackValue;
    if (callStack)
        callStackValue = callStack->buildInspectorArray();
    else
        callStackValue = InspectorArray::create();
    m_frontend->requestWillBeSent(static_cast<int>(identifier), m_pageAgent->frameId(loader->frame()), m_pageAgent->loaderId(loader), loader->url().string(), buildObjectForResourceRequest(request), currentTime(), callStackValue, buildObjectForResourceResponse(redirectResponse));
}

void InspectorResourceAgent::markResourceAsCached(unsigned long identifier)
{
    m_frontend->resourceMarkedAsCached(static_cast<int>(identifier));
}

void InspectorResourceAgent::didReceiveResponse(unsigned long identifier, DocumentLoader* loader, const ResourceResponse& response)
{
    RefPtr<InspectorObject> resourceResponse = buildObjectForResourceResponse(response);
    InspectorPageAgent::ResourceType type = InspectorPageAgent::OtherResource;
    long cachedResourceSize = 0;

    if (loader) {
        CachedResource* cachedResource = InspectorPageAgent::cachedResource(loader->frame(), response.url());
        if (cachedResource) {
            type = InspectorPageAgent::cachedResourceType(*cachedResource);
            cachedResourceSize = cachedResource->encodedSize();
            // Use mime type from cached resource in case the one in response is empty.
            if (response.mimeType().isEmpty())
                resourceResponse->setString("mimeType", cachedResource->response().mimeType());
        }
        if (equalIgnoringFragmentIdentifier(response.url(), loader->frameLoader()->iconURL()))
            type = InspectorPageAgent::ImageResource;
        else if (equalIgnoringFragmentIdentifier(response.url(), loader->url()) && type == InspectorPageAgent::OtherResource)
            type = InspectorPageAgent::DocumentResource;
    }
    m_frontend->responseReceived(static_cast<int>(identifier), currentTime(), InspectorPageAgent::resourceTypeString(type), resourceResponse);
    // If we revalidated the resource and got Not modified, send content length following didReceiveResponse
    // as there will be no calls to didReceiveContentLength from the network stack.
    if (cachedResourceSize && response.httpStatusCode() == 304)
        didReceiveContentLength(identifier, cachedResourceSize, 0);
}

void InspectorResourceAgent::didReceiveContentLength(unsigned long identifier, int dataLength, int encodedDataLength)
{
    m_frontend->dataReceived(static_cast<int>(identifier), currentTime(), dataLength, encodedDataLength);
}

void InspectorResourceAgent::didFinishLoading(unsigned long identifier, double finishTime)
{
    if (!finishTime)
        finishTime = currentTime();

    m_frontend->loadingFinished(static_cast<int>(identifier), finishTime);
}

void InspectorResourceAgent::didFailLoading(unsigned long identifier, const ResourceError& error)
{
    m_frontend->loadingFailed(static_cast<int>(identifier), currentTime(), error.localizedDescription(), error.isCancellation());
}

void InspectorResourceAgent::didLoadResourceFromMemoryCache(DocumentLoader* loader, const CachedResource* resource)
{
    m_frontend->resourceLoadedFromMemoryCache(m_pageAgent->frameId(loader->frame()), m_pageAgent->loaderId(loader), loader->url().string(), currentTime(), buildObjectForCachedResource(*resource));
}

void InspectorResourceAgent::setInitialScriptContent(unsigned long identifier, const String& sourceString)
{
    m_frontend->initialContentSet(static_cast<int>(identifier), sourceString, InspectorPageAgent::resourceTypeString(InspectorPageAgent::ScriptResource));
}

void InspectorResourceAgent::setInitialXHRContent(unsigned long identifier, const String& sourceString)
{
    m_frontend->initialContentSet(static_cast<int>(identifier), sourceString, InspectorPageAgent::resourceTypeString(InspectorPageAgent::XHRResource));
}

void InspectorResourceAgent::applyUserAgentOverride(String* userAgent)
{
    if (!m_userAgentOverride.isEmpty())
        *userAgent = m_userAgentOverride;
}

#if ENABLE(WEB_SOCKETS)

// FIXME: More this into the front-end?
// Create human-readable binary representation, like "01:23:45:67:89:AB:CD:EF".
static String createReadableStringFromBinary(const unsigned char* value, size_t length)
{
    ASSERT(length > 0);
    StringBuilder builder;
    builder.reserveCapacity(length * 3 - 1);
    for (size_t i = 0; i < length; ++i) {
        if (i > 0)
            builder.append(':');
        appendByteAsHex(value[i], builder);
    }
    return builder.toString();
}

void InspectorResourceAgent::didCreateWebSocket(unsigned long identifier, const KURL& requestURL)
{
    m_frontend->webSocketCreated(static_cast<int>(identifier), requestURL.string());
}

void InspectorResourceAgent::willSendWebSocketHandshakeRequest(unsigned long identifier, const WebSocketHandshakeRequest& request)
{
    RefPtr<InspectorObject> requestObject = InspectorObject::create();
    requestObject->setObject("headers", buildObjectForHeaders(request.headerFields()));
    requestObject->setString("requestKey3", createReadableStringFromBinary(request.key3().value, sizeof(request.key3().value)));
    m_frontend->webSocketWillSendHandshakeRequest(static_cast<int>(identifier), currentTime(), requestObject);
}

void InspectorResourceAgent::didReceiveWebSocketHandshakeResponse(unsigned long identifier, const WebSocketHandshakeResponse& response)
{
    RefPtr<InspectorObject> responseObject = InspectorObject::create();
    responseObject->setNumber("status", response.statusCode());
    responseObject->setString("statusText", response.statusText());
    responseObject->setObject("headers", buildObjectForHeaders(response.headerFields()));
    responseObject->setString("challengeResponse", createReadableStringFromBinary(response.challengeResponse().value, sizeof(response.challengeResponse().value)));
    m_frontend->webSocketHandshakeResponseReceived(static_cast<int>(identifier), currentTime(), responseObject);
}

void InspectorResourceAgent::didCloseWebSocket(unsigned long identifier)
{
    m_frontend->webSocketClosed(static_cast<int>(identifier), currentTime());
}
#endif // ENABLE(WEB_SOCKETS)

bool InspectorResourceAgent::backgroundEventsCollectionEnabled()
{
    // FIXME (https://bugs.webkit.org/show_bug.cgi?id=58652)
    // Add here condition to enable background events collection.
    // Now this function is disable.
    return false;
}

void InspectorResourceAgent::enable(ErrorString*)
{
    enable();
}

void InspectorResourceAgent::enable()
{
    if (!m_frontend)
        return;
    m_state->setBoolean(ResourceAgentState::resourceAgentEnabled, true);
    m_instrumentingAgents->setInspectorResourceAgent(this);
}

void InspectorResourceAgent::disable(ErrorString*)
{
    m_state->setBoolean(ResourceAgentState::resourceAgentEnabled, false);
    m_instrumentingAgents->setInspectorResourceAgent(0);
}

void InspectorResourceAgent::setUserAgentOverride(ErrorString*, const String& userAgent)
{
    m_userAgentOverride = userAgent;
}

void InspectorResourceAgent::setExtraHeaders(ErrorString*, PassRefPtr<InspectorObject> headers)
{
    m_state->setObject(ResourceAgentState::extraRequestHeaders, headers);
}

InspectorResourceAgent::InspectorResourceAgent(InstrumentingAgents* instrumentingAgents, InspectorPageAgent* pageAgent, InspectorState* state)
    : m_instrumentingAgents(instrumentingAgents)
    , m_pageAgent(pageAgent)
    , m_state(state)
{
    if (backgroundEventsCollectionEnabled()) {
        m_eventsCollector = adoptPtr(new EventsCollector);
        m_inspectorFrontendProxy = adoptPtr(new InspectorFrontendProxy(m_eventsCollector.get()));
        // Create mock frontend, so we can collect network events.
        m_mockFrontend = adoptPtr(new InspectorFrontend::Network(m_inspectorFrontendProxy.get()));
        m_frontend = m_mockFrontend.get();
        enable();
    } else
        m_frontend = 0;
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
