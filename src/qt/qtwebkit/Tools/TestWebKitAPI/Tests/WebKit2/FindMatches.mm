/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
#import <WebKit/WebDocumentPrivate.h>
#import <WebKit/DOMPrivate.h>
#include <WebKit2/WKImage.h>
#import <wtf/RetainPtr.h>

@interface FindMatchesWK1FrameLoadDelegate : NSObject {
}
@end

static bool didFinishLoadWK1;

@implementation FindMatchesWK1FrameLoadDelegate

- (void)webView:(WebView *)sender didFinishLoadForFrame:(WebFrame *)frame
{
    didFinishLoadWK1 = true;
}

@end

namespace TestWebKitAPI {

static bool didFinishLoad = false;
static bool didCallFindStringMatches = false;
static bool didCallGetImage = false;
static WKFindOptions findOptions = kWKFindOptionsAtWordStarts;

RetainPtr<WebView> webkit1View;

static void didFinishLoadForFrame(WKPageRef page, WKFrameRef frame, WKTypeRef userData, const void* clientInfo)
{
    didFinishLoad = true;
}

static void didFindStringMatches(WKPageRef page, WKStringRef string, WKArrayRef matches, int firstIndex, const void* clientInfo)
{
    if (WKStringIsEqualToUTF8CString(string, "Hello")) {
        size_t numMatches = WKArrayGetSize(matches);
        EXPECT_EQ(3u, numMatches);

        if (findOptions & kWKFindOptionsBackwards)
            EXPECT_EQ(1, firstIndex);
        else
            EXPECT_EQ(2, firstIndex);

        for (size_t i = 0; i < numMatches; ++i) {
            WKTypeRef items = WKArrayGetItemAtIndex(matches, i);
            WKTypeID type = WKGetTypeID(items);
            EXPECT_EQ(type, WKArrayGetTypeID());
            WKArrayRef rects = reinterpret_cast<WKArrayRef>(items);
            size_t numRects = WKArrayGetSize(rects);
            EXPECT_EQ(1u, numRects);
            items = WKArrayGetItemAtIndex(rects, 0);
            type = WKGetTypeID(items);
            EXPECT_EQ(type, WKRectGetTypeID());
            WKRect rect = WKRectGetValue(reinterpret_cast<WKRectRef>(items));
            rect = rect;
        }
    } else if (WKStringIsEqualToUTF8CString(string, "crazy")) {
        size_t numMatches = WKArrayGetSize(matches);
        EXPECT_EQ(1u, numMatches);
        EXPECT_EQ(kWKFindResultNoMatchAfterUserSelection, firstIndex);
    }
    didCallFindStringMatches = true;
}

static void didGetImageForMatchResult(WKPageRef page, WKImageRef image, uint32_t index, const void* clientInfo)
{
    WKSize size = WKImageGetSize(image);

    DOMDocument *document = webkit1View.get().mainFrameDocument;
    DOMRange *range = [document createRange];
    DOMNode *target = [document getElementById:@"target"];
    [range selectNode:target];
    NSImage *expectedImage = [range renderedImageForcingBlackText:YES];
    NSSize expectedSize = [expectedImage size];
    EXPECT_EQ(size.width, expectedSize.width);
    EXPECT_EQ(size.height, expectedSize.height);
    didCallGetImage = true;
}

TEST(WebKit2, FindMatches)
{
    WKRetainPtr<WKContextRef> context(AdoptWK, WKContextCreate());
    PlatformWebView webView(context.get());
    
    WKPageLoaderClient loaderClient;
    memset(&loaderClient, 0, sizeof(loaderClient));
    
    loaderClient.version = 0;
    loaderClient.didFinishLoadForFrame = didFinishLoadForFrame;
    WKPageSetPageLoaderClient(webView.page(), &loaderClient);

    WKPageFindMatchesClient findMatchesClient;
    memset(&findMatchesClient, 0, sizeof(findMatchesClient));

    findMatchesClient.version = 0;
    findMatchesClient.didFindStringMatches = didFindStringMatches;
    findMatchesClient.didGetImageForMatchResult = didGetImageForMatchResult;

    WKPageSetPageFindMatchesClient(webView.page(), &findMatchesClient);

    // This HTML file contains 3 occurrences of the word Hello and has the second occurence of the word 'world' selected.
    // It contains 1 occurrence of the word 'crazy' that is before the selected word.
    WKRetainPtr<WKURLRef> url(AdoptWK, Util::createURLForResource("findRanges", "html"));
    WKPageLoadURL(webView.page(), url.get());

    Util::run(&didFinishLoad);

    WKRetainPtr<WKStringRef> findString(AdoptWK, WKStringCreateWithUTF8CString("Hello"));

    WKPageFindStringMatches(webView.page(), findString.get(), findOptions, 100);
    Util::run(&didCallFindStringMatches);

    didCallFindStringMatches = false;
    findOptions |= kWKFindOptionsBackwards;
    WKPageFindStringMatches(webView.page(), findString.get(), findOptions, 100);
    Util::run(&didCallFindStringMatches);

    webkit1View = adoptNS([[WebView alloc] initWithFrame:NSMakeRect(0, 0, 120, 200) frameName:nil groupName:nil]);
    RetainPtr<FindMatchesWK1FrameLoadDelegate> frameLoadDelegate = adoptNS([FindMatchesWK1FrameLoadDelegate new]);

    webkit1View.get().frameLoadDelegate = frameLoadDelegate.get();
    [webkit1View.get().mainFrame loadHTMLString:@"Test search. Hello <span id=\"target\">Hello</span> Hello!" baseURL:[NSURL URLWithString:@"about:blank"]];

    Util::run(&didFinishLoadWK1);

    WKPageGetImageForFindMatch(webView.page(), 0);
    Util::run(&didCallGetImage);

    didCallFindStringMatches = false;
    findOptions &= ~kWKFindOptionsBackwards;
    WKRetainPtr<WKStringRef> findOtherString(AdoptWK, WKStringCreateWithUTF8CString("crazy"));
    WKPageFindStringMatches(webView.page(), findOtherString.get(), findOptions, 100);
    Util::run(&didCallFindStringMatches);

    WKPageHideFindUI(webView.page());
}

} // namespace TestWebKitAPI
