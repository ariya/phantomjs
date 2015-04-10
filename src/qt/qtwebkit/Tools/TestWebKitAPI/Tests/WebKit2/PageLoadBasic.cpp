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
#include "PlatformUtilities.h"
#include "PlatformWebView.h"
#include "Test.h"
#include <WebKit2/WKRetainPtr.h>

namespace TestWebKitAPI {

static bool test1Done;

struct State {
    State()
        : didDecidePolicyForNavigationAction(false)
        , didStartProvisionalLoadForFrame(false)
        , didCommitLoadForFrame(false)
    {
    }

    bool didDecidePolicyForNavigationAction;
    bool didStartProvisionalLoadForFrame;
    bool didCommitLoadForFrame;
};

static void didStartProvisionalLoadForFrame(WKPageRef page, WKFrameRef frame, WKTypeRef userData, const void* clientInfo)
{
    State* state = reinterpret_cast<State*>(const_cast<void*>(clientInfo));
    EXPECT_TRUE(state->didDecidePolicyForNavigationAction);
    EXPECT_FALSE(state->didCommitLoadForFrame);

    // The commited URL should be null.
    EXPECT_NULL(WKFrameCopyURL(frame));

    EXPECT_FALSE(state->didStartProvisionalLoadForFrame);

    state->didStartProvisionalLoadForFrame = true;
}

static void didCommitLoadForFrame(WKPageRef page, WKFrameRef frame, WKTypeRef userData, const void* clientInfo)
{
    State* state = reinterpret_cast<State*>(const_cast<void*>(clientInfo));
    EXPECT_TRUE(state->didDecidePolicyForNavigationAction);
    EXPECT_TRUE(state->didStartProvisionalLoadForFrame);

    // The provisional URL should be null.
    EXPECT_NULL(WKFrameCopyProvisionalURL(frame));

    state->didCommitLoadForFrame = true;
}

static void didFinishLoadForFrame(WKPageRef page, WKFrameRef frame, WKTypeRef userData, const void* clientInfo)
{
    State* state = reinterpret_cast<State*>(const_cast<void*>(clientInfo));
    EXPECT_TRUE(state->didDecidePolicyForNavigationAction);
    EXPECT_TRUE(state->didStartProvisionalLoadForFrame);
    EXPECT_TRUE(state->didCommitLoadForFrame);

    // The provisional URL should be null.
    EXPECT_NULL(WKFrameCopyProvisionalURL(frame));

    test1Done = true;
}

static void decidePolicyForNavigationAction(WKPageRef page, WKFrameRef frame, WKFrameNavigationType navigationType, WKEventModifiers modifiers, WKEventMouseButton mouseButton, WKURLRequestRef request, WKFramePolicyListenerRef listener, WKTypeRef userData, const void* clientInfo)
{
    State* state = reinterpret_cast<State*>(const_cast<void*>(clientInfo));
    EXPECT_FALSE(state->didStartProvisionalLoadForFrame);
    EXPECT_FALSE(state->didCommitLoadForFrame);
    EXPECT_TRUE(mouseButton == kWKEventMouseButtonNoButton);

    state->didDecidePolicyForNavigationAction = true;

    WKFramePolicyListenerUse(listener);
}

static void decidePolicyForNewWindowAction(WKPageRef page, WKFrameRef frame, WKFrameNavigationType navigationType, WKEventModifiers modifiers, WKEventMouseButton mouseButton, WKURLRequestRef request, WKStringRef frameName, WKFramePolicyListenerRef listener, WKTypeRef userData, const void* clientInfo)
{
    EXPECT_TRUE(mouseButton == kWKEventMouseButtonNoButton);
    WKFramePolicyListenerUse(listener);
}

static void decidePolicyForResponse(WKPageRef page, WKFrameRef frame, WKURLResponseRef response, WKURLRequestRef request, WKFramePolicyListenerRef listener, WKTypeRef userData, const void* clientInfo)
{
    WKFramePolicyListenerUse(listener);
}

TEST(WebKit2, PageLoadBasic)
{
    State state;

    WKRetainPtr<WKContextRef> context(AdoptWK, WKContextCreate());
    PlatformWebView webView(context.get());

    WKPageLoaderClient loaderClient;
    memset(&loaderClient, 0, sizeof(loaderClient));

    loaderClient.version = 0;
    loaderClient.clientInfo = &state;
    loaderClient.didStartProvisionalLoadForFrame = didStartProvisionalLoadForFrame;
    loaderClient.didCommitLoadForFrame = didCommitLoadForFrame;
    loaderClient.didFinishLoadForFrame = didFinishLoadForFrame;
    WKPageSetPageLoaderClient(webView.page(), &loaderClient);

    WKPagePolicyClient policyClient;
    memset(&policyClient, 0, sizeof(policyClient));

    policyClient.version = 0;
    policyClient.clientInfo = &state;
    policyClient.decidePolicyForNavigationAction = decidePolicyForNavigationAction;
    policyClient.decidePolicyForNewWindowAction = decidePolicyForNewWindowAction;
    policyClient.decidePolicyForResponse = decidePolicyForResponse;
    WKPageSetPagePolicyClient(webView.page(), &policyClient);

    // Before loading anything, the active url should be null
    EXPECT_NULL(WKPageCopyActiveURL(webView.page()));

    WKRetainPtr<WKURLRef> url(AdoptWK, Util::createURLForResource("simple", "html"));
    WKPageLoadURL(webView.page(), url.get());

    // But immediately after starting a load, the active url should reflect the request
    WKRetainPtr<WKURLRef> activeUrl = adoptWK(WKPageCopyActiveURL(webView.page()));
    ASSERT_NOT_NULL(activeUrl.get());
    EXPECT_TRUE(WKURLIsEqual(activeUrl.get(), url.get()));

    Util::run(&test1Done);
}

} // namespace TestWebKitAPI
