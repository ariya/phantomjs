/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

namespace TestWebKitAPI {

static bool didFinishLoad;
static bool didBecomeUnresponsive;
static bool didBrieflyPause;

static void didReceiveMessageFromInjectedBundle(WKContextRef, WKStringRef messageName, WKTypeRef, const void*)
{
    didBrieflyPause = true;
    EXPECT_WK_STREQ("DidBrieflyPause", messageName);
}

static void didFinishLoadForFrame(WKPageRef, WKFrameRef, WKTypeRef, const void*)
{
    didFinishLoad = true;
}

static void processDidBecomeUnresponsive(WKPageRef, const void*)
{
    didBecomeUnresponsive = true;
}

static void setInjectedBundleClient(WKContextRef context)
{
    WKContextInjectedBundleClient injectedBundleClient;
    memset(&injectedBundleClient, 0, sizeof(injectedBundleClient));
    injectedBundleClient.version = 0;
    injectedBundleClient.clientInfo = 0;
    injectedBundleClient.didReceiveMessageFromInjectedBundle = didReceiveMessageFromInjectedBundle;

    WKContextSetInjectedBundleClient(context, &injectedBundleClient);
}

static void setPageLoaderClient(WKPageRef page)
{
    WKPageLoaderClient loaderClient;
    memset(&loaderClient, 0, sizeof(loaderClient));
    loaderClient.version = 0;
    loaderClient.clientInfo = 0;
    loaderClient.didFinishLoadForFrame = didFinishLoadForFrame;
    loaderClient.processDidBecomeUnresponsive = processDidBecomeUnresponsive;

    WKPageSetPageLoaderClient(page, &loaderClient);
}

TEST(WebKit2, ResponsivenessTimerDoesntFireEarly)
{
    WKRetainPtr<WKContextRef> context = adoptWK(Util::createContextForInjectedBundleTest("ResponsivenessTimerDoesntFireEarlyTest"));
    setInjectedBundleClient(context.get());

    PlatformWebView webView(context.get());
    setPageLoaderClient(webView.page());

    WKPageLoadURL(webView.page(), adoptWK(Util::createURLForResource("simple", "html")).get());
    Util::run(&didFinishLoad);

    WKContextPostMessageToInjectedBundle(context.get(), Util::toWK("BrieflyPause").get(), 0);

    // Pressing a key on the keyboard should start the responsiveness timer. Since the web process
    // is going to pause before it receives this keypress, it should take a little while to respond
    // (but not so long that the responsiveness timer fires).
    webView.simulateSpacebarKeyPress();

    Util::run(&didBrieflyPause);

    EXPECT_FALSE(didBecomeUnresponsive);
}

} // namespace TestWebKitAPI
