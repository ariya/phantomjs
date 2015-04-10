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
#include <WebKit2/WKContextPrivate.h>

namespace TestWebKitAPI {

static bool didFirstLayoutAchieved;
static bool didFirstVisuallyNonEmptyLayoutAchieved;
static bool didHitRelevantRepaintedObjectsAreaThresholdAchieved;
static bool didUnlockAllLayoutMilestones;

static void didLayout(WKPageRef, WKLayoutMilestones type, WKTypeRef, const void *)
{
    switch (type) {
    case kWKDidFirstLayout:
        didFirstLayoutAchieved = true;
        break;
    case kWKDidFirstVisuallyNonEmptyLayout:
        didFirstVisuallyNonEmptyLayoutAchieved = true;
        break;
    case kWKDidHitRelevantRepaintedObjectsAreaThreshold:
        didHitRelevantRepaintedObjectsAreaThresholdAchieved = true;
        break;
    default:
        break;
    }
    
    if (didFirstLayoutAchieved && didFirstVisuallyNonEmptyLayoutAchieved && didHitRelevantRepaintedObjectsAreaThresholdAchieved)
        didUnlockAllLayoutMilestones = true;
}

static void setPageLoaderClient(WKPageRef page)
{
    WKPageLoaderClient loaderClient;
    memset(&loaderClient, 0, sizeof(loaderClient));
    loaderClient.version = kWKPageLoaderClientCurrentVersion;
    loaderClient.didLayout = didLayout;

    WKPageSetPageLoaderClient(page, &loaderClient);
}

// FIXME: This test has been broken since http://trac.webkit.org/changeset/115752 It's failing because
// the frame load is completing before didLayout() manages to unlock the
// kWKDidHitRelevantRepaintedObjectsAreaThreshold achievement. We probably need to fix this by making
// this test have a long-running resource.
TEST(WebKit2, DISABLED_NewFirstVisuallyNonEmptyLayoutForImages)
{
    WKRetainPtr<WKContextRef> context(AdoptWK, Util::createContextForInjectedBundleTest("NewFirstVisuallyNonEmptyLayoutForImagesTest"));

    PlatformWebView webView(context.get());
    setPageLoaderClient(webView.page());

    // This test is expected to succeed because lots-of-images.html is a large document and the relevant painted
    // objects take up more than 10% of the view.
    WKPageLoadURL(webView.page(), adoptWK(Util::createURLForResource("lots-of-images", "html")).get());

    Util::run(&didUnlockAllLayoutMilestones);
    EXPECT_TRUE(didUnlockAllLayoutMilestones);
}

} // namespace TestWebKitAPI
