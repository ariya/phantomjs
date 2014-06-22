/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#include <WebKit2/WKString.h>

namespace TestWebKitAPI {

static bool finished = false;
static bool receivedProperBackForwardCallbacks = false;

static void didFinishLoadForFrame(WKPageRef, WKFrameRef frame, WKTypeRef, const void*)
{
    // Only mark finished when the main frame loads
    if (!WKFrameIsMainFrame(frame))
        return;

    finished = true;
}

static void willGoToBackForwardListItem(WKPageRef, WKBackForwardListItemRef, WKTypeRef userData, const void*)
{
    if (WKGetTypeID(userData) == WKStringGetTypeID()) {
        if (WKStringIsEqualToUTF8CString((WKStringRef)userData, "shouldGoToBackForwardListItemCallback called as expected"))
            receivedProperBackForwardCallbacks = true;
    }

    finished = true;
}

static void setPageLoaderClient(WKPageRef page)
{
    WKPageLoaderClient loaderClient;
    memset(&loaderClient, 0, sizeof(loaderClient));
    loaderClient.version = 1;
    loaderClient.clientInfo = 0;
    loaderClient.didFinishLoadForFrame = didFinishLoadForFrame;
    loaderClient.willGoToBackForwardListItem = willGoToBackForwardListItem;

    WKPageSetPageLoaderClient(page, &loaderClient);
}

TEST(WebKit2, ShouldGoToBackForwardListItem)
{
    WKRetainPtr<WKContextRef> context = adoptWK(Util::createContextForInjectedBundleTest("ShouldGoToBackForwardListItemTest"));
    // Enable the page cache so we can test the WKBundleBackForwardListItemIsInPageCache API
    WKContextSetCacheModel(context.get(), kWKCacheModelDocumentBrowser);

    PlatformWebView webView(context.get());
    setPageLoaderClient(webView.page());

    WKPageLoadURL(webView.page(), adoptWK(Util::createURLForResource("simple", "html")).get());
    Util::run(&finished);
    
    finished = false;
    WKPageLoadURL(webView.page(), adoptWK(Util::createURLForResource("simple-iframe", "html")).get());
    Util::run(&finished);

    finished = false;
    WKPageGoBack(webView.page());
    Util::run(&finished);
    
    EXPECT_EQ(receivedProperBackForwardCallbacks, true);
}

} // namespace TestWebKitAPI
