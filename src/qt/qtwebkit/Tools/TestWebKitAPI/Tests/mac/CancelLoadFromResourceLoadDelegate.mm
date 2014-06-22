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
#import <wtf/RetainPtr.h>

@interface CancelLoadFromResourceLoadDelegate : NSObject {
    size_t resourceCount;
}

@end

@implementation CancelLoadFromResourceLoadDelegate

- (void)webView:(WebView *)sender resource:(id)identifier didFinishLoadingFromDataSource:(WebDataSource *)dataSource
{
    // Break the load once we have loaded the <script> and <img>.
    ++resourceCount;
    if (resourceCount > 2)
        [sender stringByEvaluatingJavaScriptFromString:@"cancelLoadAndJumpToBlank()"];
}
@end


static bool didFinishLoad = false;

@interface CancelLoadFromResourceLoadDelegateFrameLoadDelegate : NSObject
@end

@implementation CancelLoadFromResourceLoadDelegateFrameLoadDelegate
- (void)webView:(WebView *)sender didFinishLoadForFrame:(WebFrame *)frame
{
    if ([[sender mainFrameURL] isEqualToString:@"about:blank"])
        didFinishLoad = true;
}
@end

namespace TestWebKitAPI {

TEST(WebKit1, CancelLoadFromResourceLoadDelegate)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    RetainPtr<WebView> webView = adoptNS([[WebView alloc] initWithFrame:NSMakeRect(0, 0, 120, 200) frameName:nil groupName:nil]);

    RetainPtr<CancelLoadFromResourceLoadDelegate> resourceLoadDelegate = adoptNS([[CancelLoadFromResourceLoadDelegate alloc] init]);
    webView.get().resourceLoadDelegate = resourceLoadDelegate.get();
    RetainPtr<CancelLoadFromResourceLoadDelegateFrameLoadDelegate> frameLoadDelegate = adoptNS([[CancelLoadFromResourceLoadDelegateFrameLoadDelegate alloc] init]);
    webView.get().frameLoadDelegate = frameLoadDelegate.get();

    [[webView.get() mainFrame] loadRequest:[NSURLRequest requestWithURL:[[NSBundle mainBundle] URLForResource:@"CancelLoadFromResourceLoadDelegate" withExtension:@"html" subdirectory:@"TestWebKitAPI.resources"]]];

    Util::run(&didFinishLoad);

    [pool drain];
    // If we finished without crashing, the test passed.
}

} // namespace TestWebKitAPI
