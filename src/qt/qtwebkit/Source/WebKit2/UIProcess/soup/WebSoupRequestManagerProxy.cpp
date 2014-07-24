/*
 * Copyright (C) 2012 Igalia S.L.
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "WebSoupRequestManagerProxy.h"

#include "WebContext.h"
#include "WebData.h"
#include "WebSoupRequestManagerMessages.h"
#include "WebSoupRequestManagerProxyMessages.h"

namespace WebKit {

const char* WebSoupRequestManagerProxy::supplementName()
{
    return "WebSoupRequestManagerProxy";
}

PassRefPtr<WebSoupRequestManagerProxy> WebSoupRequestManagerProxy::create(WebContext* context)
{
    return adoptRef(new WebSoupRequestManagerProxy(context));
}

WebSoupRequestManagerProxy::WebSoupRequestManagerProxy(WebContext* context)
    : WebContextSupplement(context)
    , m_loadFailed(false)
{
    WebContextSupplement::context()->addMessageReceiver(Messages::WebSoupRequestManagerProxy::messageReceiverName(), this);
}

WebSoupRequestManagerProxy::~WebSoupRequestManagerProxy()
{
}

void WebSoupRequestManagerProxy::initializeClient(const WKSoupRequestManagerClient* client)
{
    m_client.initialize(client);
}

// WebContextSupplement

void WebSoupRequestManagerProxy::contextDestroyed()
{
}

void WebSoupRequestManagerProxy::processDidClose(WebProcessProxy*)
{
}

void WebSoupRequestManagerProxy::refWebContextSupplement()
{
    APIObject::ref();
}

void WebSoupRequestManagerProxy::derefWebContextSupplement()
{
    APIObject::deref();
}

void WebSoupRequestManagerProxy::registerURIScheme(const String& scheme)
{
    if (!context())
        return;

    context()->sendToAllProcessesRelaunchingThemIfNecessary(Messages::WebSoupRequestManager::RegisterURIScheme(scheme));

    ASSERT(!m_registeredURISchemes.contains(scheme));
    m_registeredURISchemes.append(scheme);
}

void WebSoupRequestManagerProxy::didHandleURIRequest(const WebData* requestData, uint64_t contentLength, const String& mimeType, uint64_t requestID)
{
    if (!context())
        return;

    context()->sendToAllProcesses(Messages::WebSoupRequestManager::DidHandleURIRequest(requestData->dataReference(), contentLength, mimeType, requestID));
}

void WebSoupRequestManagerProxy::didReceiveURIRequestData(const WebData* requestData, uint64_t requestID)
{
    if (!context())
        return;

    if (m_loadFailed)
        return;

    context()->sendToAllProcesses(Messages::WebSoupRequestManager::DidReceiveURIRequestData(requestData->dataReference(), requestID));
}

void WebSoupRequestManagerProxy::didReceiveURIRequest(const String& uriString, WebPageProxy* initiaingPage, uint64_t requestID)
{
    if (!m_client.didReceiveURIRequest(this, WebURL::create(uriString).get(), initiaingPage, requestID))
        didHandleURIRequest(WebData::create(0, 0).get(), 0, String(), requestID);
}

void WebSoupRequestManagerProxy::didFailToLoadURIRequest(uint64_t requestID)
{
    m_loadFailed = true;
    m_client.didFailToLoadURIRequest(this, requestID);
}

void WebSoupRequestManagerProxy::didFailURIRequest(const WebCore::ResourceError& error, uint64_t requestID)
{
    if (!context())
        return;

    m_loadFailed = true;
    context()->sendToAllProcesses(Messages::WebSoupRequestManager::DidFailURIRequest(error, requestID));
}

} // namespace WebKit
