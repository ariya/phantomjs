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
#include "JavaScriptTest.h"
#include "PlatformUtilities.h"
#include "PlatformWebView.h"
#include <WebKit2/WKRetainPtr.h>

namespace TestWebKitAPI {

static bool didFinishLoad;
static bool didNotHandleKeyDownEvent;

static void didFinishLoadForFrame(WKPageRef, WKFrameRef, WKTypeRef, const void*)
{
    didFinishLoad = true;
}

static void didNotHandleKeyEventCallback(WKPageRef, WKNativeEventPtr event, const void*)
{
    if (Util::isKeyDown(event))
        didNotHandleKeyDownEvent = true;
}

TEST(WebKit2, SpacebarScrolling)
{
    WKRetainPtr<WKContextRef> context(AdoptWK, Util::createContextWithInjectedBundle());
    PlatformWebView webView(context.get());

    WKPageLoaderClient loaderClient;
    memset(&loaderClient, 0, sizeof(loaderClient));
    
    loaderClient.version = 0;
    loaderClient.didFinishLoadForFrame = didFinishLoadForFrame;
    WKPageSetPageLoaderClient(webView.page(), &loaderClient);

    WKPageUIClient uiClient;
    memset(&uiClient, 0, sizeof(uiClient));

    uiClient.didNotHandleKeyEvent = didNotHandleKeyEventCallback;
    WKPageSetPageUIClient(webView.page(), &uiClient);

    WKRetainPtr<WKURLRef> url(AdoptWK, Util::createURLForResource("spacebar-scrolling", "html"));
    WKPageLoadURL(webView.page(), url.get());
    Util::run(&didFinishLoad);

    EXPECT_JS_FALSE(webView.page(), "isDocumentScrolled()");
    EXPECT_JS_FALSE(webView.page(), "textFieldContainsSpace()");

    webView.simulateSpacebarKeyPress();

    EXPECT_JS_FALSE(webView.page(), "isDocumentScrolled()");
    EXPECT_JS_TRUE(webView.page(), "textFieldContainsSpace()");

    // On Mac, a key down event represents both a raw key down and a key press. On Windows, a key
    // down event only represents a raw key down. We expect the key press to be handled (because it
    // inserts text into the text field). But the raw key down should not be handled.
#if PLATFORM(MAC)
    EXPECT_FALSE(didNotHandleKeyDownEvent);
#elif PLATFORM(WIN)
    EXPECT_TRUE(didNotHandleKeyDownEvent);
#endif

    EXPECT_JS_EQ(webView.page(), "blurTextField()", "undefined");

    didNotHandleKeyDownEvent = false;
    webView.simulateSpacebarKeyPress();

    // This EXPECT_JS_TRUE test fails on Windows port
    // https://bugs.webkit.org/show_bug.cgi?id=84961
#if !PLATFORM(WIN)
    EXPECT_JS_TRUE(webView.page(), "isDocumentScrolled()");
#endif
    EXPECT_JS_TRUE(webView.page(), "textFieldContainsSpace()");

#if PLATFORM(MAC)
    EXPECT_FALSE(didNotHandleKeyDownEvent);
#elif PLATFORM(WIN)
    EXPECT_TRUE(didNotHandleKeyDownEvent);
#endif
}

} // namespace TestWebKitAPI
