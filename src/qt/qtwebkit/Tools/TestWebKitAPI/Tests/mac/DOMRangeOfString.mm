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

#import "config.h"
#import "PlatformUtilities.h"
#import <WebKit/WebViewPrivate.h>
#import <WebKit/DOM.h>
#import <wtf/RetainPtr.h>

@interface DOMRangeOfStringFrameLoadDelegate : NSObject {
}
@end

static bool didFinishLoad;

@implementation DOMRangeOfStringFrameLoadDelegate

- (void)webView:(WebView *)sender didFinishLoadForFrame:(WebFrame *)frame
{
    didFinishLoad = true;
}

@end

namespace TestWebKitAPI {

TEST(WebKit1, DOMRangeOfString)
{
    RetainPtr<WebView> webView = adoptNS([[WebView alloc] initWithFrame:NSZeroRect frameName:nil groupName:nil]);
    RetainPtr<DOMRangeOfStringFrameLoadDelegate> frameLoadDelegate = adoptNS([DOMRangeOfStringFrameLoadDelegate new]);

    webView.get().frameLoadDelegate = frameLoadDelegate.get();
    [[webView.get() mainFrame] loadRequest:[NSURLRequest requestWithURL:[[NSBundle mainBundle] URLForResource:@"DOMRangeOfString" withExtension:@"html" subdirectory:@"TestWebKitAPI.resources"]]];

    Util::run(&didFinishLoad);

    DOMRange *resultRange = [webView.get() DOMRangeOfString:@"needles" relativeTo:nil options:0];
    EXPECT_EQ(nil, resultRange);

    DOMRange *needleRange = [webView.get() DOMRangeOfString:@"needle" relativeTo:nil options:0];
    EXPECT_EQ(28, needleRange.startOffset);

    resultRange = [webView.get() DOMRangeOfString:@"stack" relativeTo:needleRange options:0];
    EXPECT_EQ(43, resultRange.startOffset);

    resultRange = [webView.get() DOMRangeOfString:@"stack" relativeTo:needleRange options:WebFindOptionsBackwards];
    EXPECT_EQ(nil, resultRange);

    resultRange = [webView.get() DOMRangeOfString:@"n" relativeTo:needleRange options:0];
    EXPECT_EQ(36, resultRange.startOffset);

    resultRange = [webView.get() DOMRangeOfString:@"n" relativeTo:needleRange options:WebFindOptionsStartInSelection];
    EXPECT_EQ(28, resultRange.startOffset);

    RetainPtr<WebView> otherWebView = adoptNS([[WebView alloc] initWithFrame:NSZeroRect frameName:nil groupName:nil]);
    DOMRange *foreignRange = [[[otherWebView.get() mainFrame] DOMDocument] createRange];
    resultRange = [webView.get() DOMRangeOfString:@"needle" relativeTo:foreignRange options:0];
    EXPECT_EQ(nil, resultRange);

    resultRange = [webView.get() DOMRangeOfString:@"here" relativeTo:needleRange options:0];
    EXPECT_EQ(1, resultRange.startOffset);

    resultRange = [webView.get() DOMRangeOfString:@"here" relativeTo:resultRange options:0];
    EXPECT_EQ(25, resultRange.startOffset);

    resultRange = [webView.get() DOMRangeOfString:@"here" relativeTo:resultRange options:WebFindOptionsWrapAround];
    EXPECT_EQ(6, resultRange.startOffset);
}

} // namespace TestWebKitAPI
