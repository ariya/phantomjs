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

#import "config.h"
#import "PlatformUtilities.h"
#import <wtf/RetainPtr.h>

static bool testFinished = false;

@interface TestDelegate : NSObject

+ (TestDelegate *)shared;

@end

@implementation TestDelegate

+ (TestDelegate *)shared
{
    static TestDelegate *sharedTestDelegate = [[TestDelegate alloc] init];
    return sharedTestDelegate;
}

- (void)webView:(WebView *)webView decidePolicyForNavigationAction:(NSDictionary *)actionInformation request:(NSURLRequest *)request frame:(WebFrame *)frame decisionListener:(id<WebPolicyDecisionListener>)listener
{
    if (!request) {
        [listener use];
        return;
    }

    [webView close];
    [listener ignore];
    testFinished = true;
}

- (WebView *)webView:(WebView *)sender createWebViewWithRequest:(NSURLRequest *)request
{
    WebView *webView = [[WebView alloc] init];
    webView.policyDelegate = [TestDelegate shared];
    [[webView mainFrame] loadRequest:request];
    return webView;
}

@end

namespace TestWebKitAPI {

TEST(WebKit1, CloseNewWindowInNavigationPolicyDelegate)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    RetainPtr<WebView> webView = adoptNS([[WebView alloc] init]);
    webView.get().preferences.javaScriptCanOpenWindowsAutomatically = YES;
    webView.get().UIDelegate = [TestDelegate shared];
    [[webView.get() mainFrame] loadRequest:[NSURLRequest requestWithURL:[[NSBundle mainBundle] URLForResource:@"OpenNewWindow" withExtension:@"html" subdirectory:@"TestWebKitAPI.resources"]]];
    
    Util::run(&testFinished);
    
    [pool drain];
}
    
} // namespace TestWebKitAPI
