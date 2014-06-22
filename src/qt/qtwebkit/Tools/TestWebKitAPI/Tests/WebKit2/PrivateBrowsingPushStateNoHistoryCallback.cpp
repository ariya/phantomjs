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
#include "Test.h"
#include <WebKit2/WKRetainPtr.h>

namespace TestWebKitAPI {

static bool testDone;

static void didNavigateWithNavigationData(WKContextRef context, WKPageRef page, WKNavigationDataRef navigationData, WKFrameRef frame, const void* clientInfo)
{
    // This should never be called when navigating in Private Browsing.
    FAIL();
}

static void didSameDocumentNavigationForFrame(WKPageRef page, WKFrameRef frame, WKSameDocumentNavigationType type, WKTypeRef userData, const void *clientInfo)
{
    testDone = true;
}

TEST(WebKit2, PrivateBrowsingPushStateNoHistoryCallback)
{
    WKRetainPtr<WKContextRef> context(AdoptWK, WKContextCreate());

    WKContextHistoryClient historyClient;
    memset(&historyClient, 0, sizeof(historyClient));

    historyClient.version = 0;
    historyClient.clientInfo = 0;
    historyClient.didNavigateWithNavigationData = didNavigateWithNavigationData;
    WKContextSetHistoryClient(context.get(), &historyClient);

    PlatformWebView webView(context.get());

    WKPageLoaderClient pageLoaderClient;
    memset(&pageLoaderClient, 0, sizeof(pageLoaderClient));

    pageLoaderClient.version = 0;
    pageLoaderClient.clientInfo = 0;
    pageLoaderClient.didSameDocumentNavigationForFrame = didSameDocumentNavigationForFrame;
    WKPageSetPageLoaderClient(webView.page(), &pageLoaderClient);

    WKRetainPtr<WKPreferencesRef> preferences(AdoptWK, WKPreferencesCreate());
    WKPreferencesSetPrivateBrowsingEnabled(preferences.get(), true);

    WKPageGroupRef pageGroup = WKPageGetPageGroup(webView.page());
    WKPageGroupSetPreferences(pageGroup, preferences.get());

    WKRetainPtr<WKURLRef> url(AdoptWK, Util::createURLForResource("push-state", "html"));
    WKPageLoadURL(webView.page(), url.get());

    Util::run(&testDone);
}

} // namespace TestWebKitAPI
