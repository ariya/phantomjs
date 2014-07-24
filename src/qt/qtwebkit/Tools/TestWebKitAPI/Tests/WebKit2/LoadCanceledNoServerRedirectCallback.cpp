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

#include <WebKit2/WKContext.h>
#include <WebKit2/WKFrame.h>
#include <WebKit2/WKRetainPtr.h>

namespace TestWebKitAPI {

static bool loadedMainFrame;
static bool loadedIFrame;
static bool loadedAllFrames;

static bool performedServerRedirect;

static void didFinishLoadForFrame(WKPageRef, WKFrameRef frame, WKTypeRef, const void*)
{
    if (WKFrameIsMainFrame(frame))
        loadedMainFrame = true;
    else
        loadedIFrame = true;

    loadedAllFrames = loadedMainFrame && loadedIFrame;
}

static void didPerformServerRedirect(WKContextRef context, WKPageRef page, WKURLRef sourceURL, WKURLRef destinationURL, WKFrameRef frame, const void *clientInfo)
{
    performedServerRedirect = true;
}

TEST(WebKit2, LoadCanceledNoServerRedirectCallback)
{
    WKRetainPtr<WKContextRef> context(AdoptWK, Util::createContextForInjectedBundleTest("LoadCanceledNoServerRedirectCallbackTest"));
    
    WKContextInjectedBundleClient injectedBundleClient;
    memset(&injectedBundleClient, 0, sizeof(injectedBundleClient));
    injectedBundleClient.version = 0;
    injectedBundleClient.clientInfo = 0;
    WKContextSetInjectedBundleClient(context.get(), &injectedBundleClient);

    PlatformWebView webView(context.get());

    WKPageLoaderClient loaderClient;
    memset(&loaderClient, 0, sizeof(loaderClient));
    
    loaderClient.version = 0;
    loaderClient.didFinishLoadForFrame = didFinishLoadForFrame;
    WKPageSetPageLoaderClient(webView.page(), &loaderClient);
    
    WKContextHistoryClient historyClient;
    memset(&historyClient, 0, sizeof(historyClient));
    
    historyClient.version = 0;
    historyClient.didPerformServerRedirect = didPerformServerRedirect;
    WKContextSetHistoryClient(context.get(), &historyClient);

    WKRetainPtr<WKURLRef> url(AdoptWK, Util::createURLForResource("simple-iframe", "html"));
    WKPageLoadURL(webView.page(), url.get());
    Util::run(&loadedAllFrames);
    
    // We shouldn't have performed a server redirect when the iframe load was cancelled.
    EXPECT_FALSE(performedServerRedirect);
}

} // namespace TestWebKitAPI
