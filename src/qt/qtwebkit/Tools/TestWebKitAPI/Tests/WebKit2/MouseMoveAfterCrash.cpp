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
#include "JavaScriptTest.h"
#include "PlatformUtilities.h"
#include "PlatformWebView.h"

namespace TestWebKitAPI {

static bool didFinishLoad;

static void didFinishLoadForFrame(WKPageRef, WKFrameRef, WKTypeRef, const void*)
{
    didFinishLoad = true;
}

static void setPageLoaderClient(WKPageRef page)
{
    WKPageLoaderClient loaderClient;
    memset(&loaderClient, 0, sizeof(loaderClient));
    loaderClient.version = 0;
    loaderClient.clientInfo = 0;
    loaderClient.didFinishLoadForFrame = didFinishLoadForFrame;

    WKPageSetPageLoaderClient(page, &loaderClient);
}

#if PLATFORM(WIN)
TEST(WebKit2, DISABLED_MouseMoveAfterCrash)
#else
TEST(WebKit2, MouseMoveAfterCrash)
#endif
{
    WKRetainPtr<WKContextRef> context = adoptWK(Util::createContextForInjectedBundleTest("MouseMoveAfterCrashTest"));

    PlatformWebView webView(context.get());
    setPageLoaderClient(webView.page());

    WKRetainPtr<WKURLRef> url(AdoptWK, Util::createURLForResource("mouse-move-listener", "html"));
    WKPageLoadURL(webView.page(), url.get());
    Util::run(&didFinishLoad);

    didFinishLoad = false;

    WKContextPostMessageToInjectedBundle(context.get(), Util::toWK("Pause").get(), 0);

    webView.simulateSpacebarKeyPress();

    // Move the mouse once we are hung.
    webView.simulateMouseMove(10, 10);
    webView.simulateMouseMove(20, 20);

    // After moving the mouse (while the web process was hung on the Pause message), kill the web process. It is restarted by reloading the page.
    WKPageTerminate(webView.page());
    WKPageReload(webView.page());

    // Wait until we load the page a second time.
    Util::run(&didFinishLoad);

    EXPECT_JS_FALSE(webView.page(), "didMoveMouse()");

    // Once the page has reloaded, try moving the mouse to verify that we get mouse move events.
    webView.simulateMouseMove(10, 10);
    webView.simulateMouseMove(20, 20);

    EXPECT_JS_TRUE(webView.page(), "didMoveMouse()");
}

} // namespace TestWebKitAPI
