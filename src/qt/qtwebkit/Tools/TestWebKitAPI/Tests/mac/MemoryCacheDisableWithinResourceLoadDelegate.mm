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

#import "config.h"
#import "PlatformUtilities.h"
#import <WebKit/WebCache.h>
#import <wtf/RetainPtr.h>

@interface MemoryCacheDisableTestResourceLoadDelegate : NSObject {
}
@end

static bool didFinishLoad;

@implementation MemoryCacheDisableTestResourceLoadDelegate

- (id)webView:(WebView *)sender identifierForInitialRequest:(NSURLRequest *)request fromDataSource:(WebDataSource *)dataSource
{
    [WebCache setDisabled:YES];
    [WebCache setDisabled:NO];

    return self;
}

- (NSURLRequest *)webView:(WebView *)sender resource:(id)identifier willSendRequest:(NSURLRequest *)request redirectResponse:(NSURLResponse *)redirectResponse fromDataSource:(WebDataSource *)dataSource
{
    [WebCache setDisabled:YES];
    [WebCache setDisabled:NO];

    return request;
}

- (void)webView:(WebView *)sender resource:(id)identifier didFinishLoadingFromDataSource:(WebDataSource *)dataSource
{
    [WebCache setDisabled:YES];
    [WebCache setDisabled:NO];

    didFinishLoad = YES;
}

- (void)webView:(WebView *)sender resource:(id)identifier didFailLoadingWithError:(NSError *)error fromDataSource:(WebDataSource *)dataSource
{
    [WebCache setDisabled:YES];
    [WebCache setDisabled:NO];
}

@end

namespace TestWebKitAPI {

TEST(WebKit1, MemoryCacheDisableWithinResourceLoadDelegate)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    RetainPtr<WebView> webView = adoptNS([[WebView alloc] initWithFrame:NSMakeRect(0, 0, 120, 200) frameName:nil groupName:nil]);

    RetainPtr<MemoryCacheDisableTestResourceLoadDelegate> resourceLoadDelegate = adoptNS([[MemoryCacheDisableTestResourceLoadDelegate alloc] init]);
    webView.get().resourceLoadDelegate = resourceLoadDelegate.get();

    [[webView.get() mainFrame] loadRequest:[NSURLRequest requestWithURL:[[NSBundle mainBundle] URLForResource:@"MemoryCacheDisableWithinResourceLoadDelegate" withExtension:@"html" subdirectory:@"TestWebKitAPI.resources"]]];

    Util::run(&didFinishLoad);
    
    [pool drain];
    // If we finished without crashing, the test passed.
}

} // namespace TestWebKitAPI
