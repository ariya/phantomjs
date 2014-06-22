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
#include "WebPolicyClient.h"

#include "APIObject.h"
#include "WKAPICast.h"
#include "WebURLRequest.h"

using namespace WebCore;

namespace WebKit {

bool WebPolicyClient::decidePolicyForNavigationAction(WebPageProxy* page, WebFrameProxy* frame, NavigationType type, WebEvent::Modifiers modifiers, WebMouseEvent::Button mouseButton, const ResourceRequest& resourceRequest, WebFramePolicyListenerProxy* listener, APIObject* userData)
{
    if (!m_client.decidePolicyForNavigationAction)
        return false;

    RefPtr<WebURLRequest> request = WebURLRequest::create(resourceRequest);

    m_client.decidePolicyForNavigationAction(toAPI(page), toAPI(frame), toAPI(type), toAPI(modifiers), toAPI(mouseButton), toAPI(request.get()), toAPI(listener), toAPI(userData), m_client.clientInfo);
    return true;
}

bool WebPolicyClient::decidePolicyForNewWindowAction(WebPageProxy* page, WebFrameProxy* frame, NavigationType type, WebEvent::Modifiers modifiers, WebMouseEvent::Button mouseButton, const ResourceRequest& resourceRequest, const String& frameName, WebFramePolicyListenerProxy* listener, APIObject* userData)
{
    if (!m_client.decidePolicyForNewWindowAction)
        return false;

    RefPtr<WebURLRequest> request = WebURLRequest::create(resourceRequest);

    m_client.decidePolicyForNewWindowAction(toAPI(page), toAPI(frame), toAPI(type), toAPI(modifiers), toAPI(mouseButton), toAPI(request.get()), toAPI(frameName.impl()), toAPI(listener), toAPI(userData), m_client.clientInfo);
    return true;
}

bool WebPolicyClient::decidePolicyForResponse(WebPageProxy* page, WebFrameProxy* frame, const ResourceResponse& resourceResponse, const ResourceRequest& resourceRequest, WebFramePolicyListenerProxy* listener, APIObject* userData)
{
    if (!m_client.decidePolicyForResponse)
        return false;

    RefPtr<WebURLResponse> response = WebURLResponse::create(resourceResponse);
    RefPtr<WebURLRequest> request = WebURLRequest::create(resourceRequest);

    m_client.decidePolicyForResponse(toAPI(page), toAPI(frame), toAPI(response.get()), toAPI(request.get()), toAPI(listener), toAPI(userData), m_client.clientInfo);
    return true;
}

void WebPolicyClient::unableToImplementPolicy(WebPageProxy* page, WebFrameProxy* frame, const ResourceError& error, APIObject* userData)
{
    if (!m_client.unableToImplementPolicy)
        return;

    m_client.unableToImplementPolicy(toAPI(page), toAPI(frame), toAPI(error), toAPI(userData), m_client.clientInfo);
}

} // namespace WebKit
