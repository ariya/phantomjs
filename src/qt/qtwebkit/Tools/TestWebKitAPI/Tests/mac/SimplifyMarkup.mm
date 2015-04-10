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
#include <wtf/RetainPtr.h>

#import <WebKit/DOM.h>
#import <WebKit/WebViewPrivate.h>

@interface SimplifyMarkupTest : NSObject {
}
@end

static bool didFinishLoad;

@implementation SimplifyMarkupTest

- (void)webView:(WebView *)sender didFinishLoadForFrame:(WebFrame *)frame
{
    didFinishLoad = true;
}
@end

namespace TestWebKitAPI {

TEST(WebKit1, SimplifyMarkupTest)
{
    RetainPtr<WebView> webView1 = adoptNS([[WebView alloc] initWithFrame:NSMakeRect(0, 0, 120, 200) frameName:nil groupName:nil]);
    RetainPtr<WebView> webView2 = adoptNS([[WebView alloc] initWithFrame:NSMakeRect(0, 0, 120, 200) frameName:nil groupName:nil]);
    RetainPtr<SimplifyMarkupTest> testController = adoptNS([SimplifyMarkupTest new]);
    
    webView1.get().frameLoadDelegate = testController.get();
    [[webView1.get() mainFrame] loadRequest:[NSURLRequest requestWithURL:[[NSBundle mainBundle] URLForResource:@"verboseMarkup" withExtension:@"html" subdirectory:@"TestWebKitAPI.resources"]]];
    
    Util::run(&didFinishLoad);
    didFinishLoad = false;
 
    webView2.get().frameLoadDelegate = testController.get();
    [[webView2.get() mainFrame] loadRequest:[NSURLRequest requestWithURL:[[NSBundle mainBundle] URLForResource:@"verboseMarkup" withExtension:@"html" subdirectory:@"TestWebKitAPI.resources"]]];
    
    Util::run(&didFinishLoad);
    didFinishLoad = false;

    DOMDocument *document1 = webView1.get().mainFrameDocument;
    NSString* markupBefore = [[document1 body] innerHTML];
    DOMDocument *document2 = webView2.get().mainFrameDocument;
    
    // If start is after end, nothing is done
    DOMNode *start = [document1 getElementById:@"test2"];
    DOMNode *end = [document1 getElementById:@"test1"];

    [webView1.get() _simplifyMarkup:[document1 body] endNode:end];
    NSString* markupAfter = [[document1 body] innerHTML];

    EXPECT_WK_STREQ(markupBefore, markupAfter);
    EXPECT_EQ([markupBefore length], [markupAfter length]);

    // If the two nodes are not in the same webView, nothing is done.
    start = [document1 getElementById:@"test1"];
    end = [document2 getElementById:@"test2"];
    [webView1.get() _simplifyMarkup:start endNode:end];
    markupAfter = [[document1 body] innerHTML];
    
    EXPECT_WK_STREQ(markupBefore, markupAfter);
    EXPECT_EQ([markupBefore length], [markupAfter length]);

    // If the two nodes are not in the same document, nothing is done.
    DOMHTMLFrameElement* frame = (DOMHTMLFrameElement *)[document1 getElementById:@"test3"];
    end = [[frame contentDocument] firstChild];
    
    [webView1.get() _simplifyMarkup:start endNode:end];
    markupAfter = [[document1 body] innerHTML];
    
    EXPECT_WK_STREQ(markupBefore, markupAfter);
    EXPECT_EQ([markupBefore length], [markupAfter length]);

    // If the nodes are in the same webView, same document and in the right order,
    // we should have a simplified markup.
    [webView1.get() _simplifyMarkup:[document1 body] endNode:nil];
    markupAfter = [[document1 body] innerHTML];
    // We only verify that the markup has changed and that it is less verbose
    // then the original version.
    // The accuracy of the operation is tested by the DRT tests already.
    EXPECT_GT([markupBefore length], [markupAfter length]);
}

} // namespace TestWebKitAPI
