/*
 * Copyright (C) 2013 Adenilson Cavalcanti <cavalcantii@gmail.com>
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

static bool loadBeforeCrash = false;
static bool loadAfterCrash = false;

static void didFinishLoad(WKPageRef page, WKFrameRef frame, WKTypeRef userData, const void* clientInfo)
{
    // First load before WebProcess was terminated.
    if (!loadBeforeCrash) {
        loadBeforeCrash = true;
        return;
    }

    // Next load after WebProcess was terminated (hopefully
    // it will be correctly re-spawned).
    EXPECT_EQ(static_cast<uint32_t>(kWKFrameLoadStateFinished), WKFrameGetFrameLoadState(frame));
    EXPECT_FALSE(loadAfterCrash);

    // Set it, otherwise the loop will not end.
    loadAfterCrash = true;
}

static void didCrash(WKPageRef page, const void*)
{
    // Test if first load actually worked.
    EXPECT_TRUE(loadBeforeCrash);

    // Reload should re-spawn webprocess.
    WKPageReload(page);
}

TEST(WebKit2, ReloadPageAfterCrash)
{
    WKRetainPtr<WKContextRef> context(AdoptWK, WKContextCreate());
    PlatformWebView webView(context.get());

    WKPageLoaderClient loaderClient;
    memset(&loaderClient, 0, sizeof(loaderClient));
    loaderClient.didFinishLoadForFrame = didFinishLoad;
    loaderClient.processDidCrash = didCrash;
    WKPageSetPageLoaderClient(webView.page(), &loaderClient);

    WKRetainPtr<WKURLRef> url = adoptWK(WKURLCreateWithUTF8CString("about:blank"));
    // Load a blank page and next kills WebProcess.
    WKPageLoadURL(webView.page(), url.get());
    Util::run(&loadBeforeCrash);
    WKPageTerminate(webView.page());

    // Let's try load a page and see what happens.
    WKPageLoadURL(webView.page(), url.get());
    Util::run(&loadAfterCrash);
}

} // namespace TestWebKitAPI
