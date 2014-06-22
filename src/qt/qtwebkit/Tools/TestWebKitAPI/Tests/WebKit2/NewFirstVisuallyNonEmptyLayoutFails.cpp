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
#include <WebKit2/WKContextPrivate.h>
#include <WebKit2/WKRetainPtr.h>

namespace TestWebKitAPI {

static bool didHitRelevantRepaintedObjectsAreaThresholdAchieved;
static bool test1Done;
static bool test2Done;
    
static void didForceRepaint(WKErrorRef error, void*)
{
    EXPECT_NULL(error);
    test2Done = true;
}

static void didFinishLoadForFrame(WKPageRef page, WKFrameRef frame, WKTypeRef userData, const void* clientInfo)
{
    test1Done = true;
    WKPageForceRepaint(page, 0, didForceRepaint);
}

static void didLayout(WKPageRef, WKLayoutMilestones type, WKTypeRef, const void *)
{
    if (type == kWKDidHitRelevantRepaintedObjectsAreaThreshold)
        didHitRelevantRepaintedObjectsAreaThresholdAchieved = true;
}

static void setPageLoaderClient(WKPageRef page)
{
    WKPageLoaderClient loaderClient;
    memset(&loaderClient, 0, sizeof(loaderClient));
    loaderClient.version = kWKPageLoaderClientCurrentVersion;
    loaderClient.didFinishLoadForFrame = didFinishLoadForFrame;
    loaderClient.didLayout = didLayout;

    WKPageSetPageLoaderClient(page, &loaderClient);
}

TEST(WebKit2, NewFirstVisuallyNonEmptyLayoutFails)
{
    WKRetainPtr<WKContextRef> context(AdoptWK, Util::createContextForInjectedBundleTest("NewFirstVisuallyNonEmptyLayoutFailsTest"));

    PlatformWebView webView(context.get());
    setPageLoaderClient(webView.page());

    // This test is expected to fail because simple.html is a small document and the relevant painted
    // objects take up less than 10% of the view.
    WKPageLoadURL(webView.page(), adoptWK(Util::createURLForResource("simple", "html")).get());

    Util::run(&test1Done);
    Util::run(&test2Done);

    // By the time the forced repaint has finished, the counter would have been hit 
    // if it was sized reasonably for the page.
    EXPECT_FALSE(didHitRelevantRepaintedObjectsAreaThresholdAchieved);
}

} // namespace TestWebKitAPI
