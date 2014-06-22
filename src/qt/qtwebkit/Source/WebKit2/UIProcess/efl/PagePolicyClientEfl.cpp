/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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
#include "PagePolicyClientEfl.h"

#include "EwkView.h"
#include "WKFrame.h"
#include "WKFramePolicyListener.h"
#include "WKString.h"
#include "WKURLResponse.h"
#include "ewk_navigation_policy_decision.h"
#include "ewk_navigation_policy_decision_private.h"
#include <WebCore/HTTPStatusCodes.h>
#include <wtf/text/CString.h>

using namespace EwkViewCallbacks;

namespace WebKit {

static inline PagePolicyClientEfl* toPagePolicyClientEfl(const void* clientInfo)
{
    return static_cast<PagePolicyClientEfl*>(const_cast<void*>(clientInfo));
}

void PagePolicyClientEfl::decidePolicyForNavigationAction(WKPageRef, WKFrameRef, WKFrameNavigationType navigationType, WKEventModifiers modifiers, WKEventMouseButton mouseButton, WKURLRequestRef request, WKFramePolicyListenerRef listener, WKTypeRef /*userData*/, const void* clientInfo)
{
    PagePolicyClientEfl* policyClient = toPagePolicyClientEfl(clientInfo);

    RefPtr<EwkNavigationPolicyDecision> decision = EwkNavigationPolicyDecision::create(navigationType, mouseButton, modifiers, request, 0, listener);
    policyClient->m_view->smartCallback<NavigationPolicyDecision>().call(decision.get());
}

void PagePolicyClientEfl::decidePolicyForNewWindowAction(WKPageRef, WKFrameRef, WKFrameNavigationType navigationType, WKEventModifiers modifiers, WKEventMouseButton mouseButton, WKURLRequestRef request, WKStringRef frameName, WKFramePolicyListenerRef listener, WKTypeRef /*userData*/, const void* clientInfo)
{
    PagePolicyClientEfl* policyClient = toPagePolicyClientEfl(clientInfo);

    RefPtr<EwkNavigationPolicyDecision> decision = EwkNavigationPolicyDecision::create(navigationType, mouseButton, modifiers, request, toImpl(frameName)->string().utf8().data(), listener);
    policyClient->m_view->smartCallback<NewWindowPolicyDecision>().call(decision.get());
}

void PagePolicyClientEfl::decidePolicyForResponseCallback(WKPageRef, WKFrameRef frame, WKURLResponseRef response, WKURLRequestRef, WKFramePolicyListenerRef listener, WKTypeRef /*userData*/, const void* /*clientInfo*/)
{
    // Ignore responses with an HTTP status code of 204 (No Content)
    if (WKURLResponseHTTPStatusCode(response) == WebCore::HTTPNoContent) {
        WKFramePolicyListenerIgnore(listener);
        return;
    }

    // If the URL Response has "Content-Disposition: attachment;" header, then
    // we should download it.
    if (WKURLResponseIsAttachment(response)) {
        WKFramePolicyListenerDownload(listener);
        return;
    }

    WKRetainPtr<WKStringRef> mimeType = adoptWK(WKURLResponseCopyMIMEType(response));
    bool canShowMIMEType = WKFrameCanShowMIMEType(frame, mimeType.get());
    if (WKFrameIsMainFrame(frame)) {
        if (canShowMIMEType) {
            WKFramePolicyListenerUse(listener);
            return;
        }

        // If we can't use (show) it then we should download it.
        WKFramePolicyListenerDownload(listener);
        return;
    }

    // We should ignore downloadable top-level content for subframes, with an exception for text/xml and application/xml so we can still support Acid3 test.
    // It makes the browser intentionally behave differently when it comes to text(application)/xml content in subframes vs. mainframe.
    bool isXMLType = WKStringIsEqualToUTF8CString(mimeType.get(), "text/xml") || WKStringIsEqualToUTF8CString(mimeType.get(), "application/xml");
    if (!canShowMIMEType && !isXMLType) {
        WKFramePolicyListenerIgnore(listener);
        return;
    }

    WKFramePolicyListenerUse(listener);
}

PagePolicyClientEfl::PagePolicyClientEfl(EwkView* view)
    : m_view(view)
{
    WKPageRef pageRef = m_view->wkPage();
    ASSERT(pageRef);

    WKPagePolicyClient policyClient;
    memset(&policyClient, 0, sizeof(WKPagePolicyClient));
    policyClient.version = kWKPagePolicyClientCurrentVersion;
    policyClient.clientInfo = this;
    policyClient.decidePolicyForNavigationAction = decidePolicyForNavigationAction;
    policyClient.decidePolicyForNewWindowAction = decidePolicyForNewWindowAction;
    policyClient.decidePolicyForResponse = decidePolicyForResponseCallback;

    WKPageSetPagePolicyClient(pageRef, &policyClient);
}

} // namespace WebKit
