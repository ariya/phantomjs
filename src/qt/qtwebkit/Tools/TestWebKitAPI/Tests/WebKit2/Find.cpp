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
#include <WebKit2/WKRetainPtr.h>

namespace TestWebKitAPI {

static bool didFinishLoad = false;
static bool didCallCountStringMatches = false;

static void didFinishLoadForFrame(WKPageRef page, WKFrameRef frame, WKTypeRef userData, const void* clientInfo)
{
    didFinishLoad = true;
}

static void didCountStringMatches(WKPageRef page, WKStringRef string, unsigned numMatches, const void* clientInfo)
{
    EXPECT_WK_STREQ("Hello", string);
    EXPECT_EQ(3u, numMatches);

    didCallCountStringMatches = true;
}

TEST(WebKit2, Find)
{
    WKRetainPtr<WKContextRef> context(AdoptWK, WKContextCreate());
    PlatformWebView webView(context.get());
    
    WKPageLoaderClient loaderClient;
    memset(&loaderClient, 0, sizeof(loaderClient));
    
    loaderClient.version = 0;
    loaderClient.didFinishLoadForFrame = didFinishLoadForFrame;
    WKPageSetPageLoaderClient(webView.page(), &loaderClient);

    WKPageFindClient findClient;
    memset(&findClient, 0, sizeof(findClient));

    findClient.version = 0;
    findClient.didCountStringMatches = didCountStringMatches;
    WKPageSetPageFindClient(webView.page(), &findClient);

    WKRetainPtr<WKURLRef> url(AdoptWK, Util::createURLForResource("find", "html"));
    WKPageLoadURL(webView.page(), url.get());

    Util::run(&didFinishLoad);

    WKRetainPtr<WKStringRef> findString(AdoptWK, WKStringCreateWithUTF8CString("Hello"));
    WKPageCountStringMatches(webView.page(), findString.get(), true, 100);

    Util::run(&didCallCountStringMatches);
}

} // namespace TestWebKitAPI
