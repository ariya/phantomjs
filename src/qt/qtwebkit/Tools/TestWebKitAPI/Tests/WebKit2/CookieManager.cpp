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
#include <WebKit2/WKCookieManager.h>
#include <WebKit2/WKRetainPtr.h>

namespace TestWebKitAPI {

static bool testDone;
// Make sure that the policy on the machine running the test is not changed after running the test.
static WKHTTPCookieAcceptPolicy userPolicy;
static WKHTTPCookieAcceptPolicy testPolicy;
static WKRetainPtr<WKContextRef> wkContext;

static void didGetTestHTTPCookieAcceptPolicy(WKHTTPCookieAcceptPolicy policy, WKErrorRef, void* context)
{
    EXPECT_EQ(reinterpret_cast<void*>(0x1234578), context);
    EXPECT_EQ(testPolicy, policy);

    WKCookieManagerRef cookieManager = WKContextGetCookieManager(wkContext.get());
    WKCookieManagerSetHTTPCookieAcceptPolicy(cookieManager, userPolicy);

    testDone = true;
}

static void didGetUserHTTPCookieAcceptPolicy(WKHTTPCookieAcceptPolicy policy, WKErrorRef, void* context)
{
    EXPECT_EQ(reinterpret_cast<void*>(0x1234578), context);

    userPolicy = policy;

    // Make sure to choose a policy different from the policy the user currently has set.
    testPolicy = (userPolicy + 1) % 3;
    WKCookieManagerRef cookieManager = WKContextGetCookieManager(wkContext.get());
    WKCookieManagerSetHTTPCookieAcceptPolicy(cookieManager, testPolicy);
    WKCookieManagerGetHTTPCookieAcceptPolicy(cookieManager, reinterpret_cast<void*>(0x1234578), didGetTestHTTPCookieAcceptPolicy);
}

static void didFinishLoadForFrame(WKPageRef, WKFrameRef, WKTypeRef, const void*)
{
    WKCookieManagerRef cookieManager = WKContextGetCookieManager(wkContext.get());
    WKCookieManagerGetHTTPCookieAcceptPolicy(cookieManager, reinterpret_cast<void*>(0x1234578), didGetUserHTTPCookieAcceptPolicy);
}

TEST(WebKit2, CookieManager)
{
    wkContext.adopt(WKContextCreate());
    PlatformWebView webView(wkContext.get());

    WKPageLoaderClient loaderClient;
    memset(&loaderClient, 0, sizeof(loaderClient));

    loaderClient.version = 0;
    loaderClient.didFinishLoadForFrame = didFinishLoadForFrame;
    WKPageSetPageLoaderClient(webView.page(), &loaderClient);

    WKPageLoadURL(webView.page(), adoptWK(WKURLCreateWithUTF8CString("about:blank")).get());

    Util::run(&testDone);
}

} // namespace TestWebKitAPI
