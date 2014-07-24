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
#import <WebKit/WebFrameLoadDelegatePrivate.h>
#import <wtf/RetainPtr.h>

@interface DidRemoveFrameFromHierarchyFrameLoadDelegate : NSObject
@end

static bool didFinishLoad;
static bool didRemoveFrame;

@implementation DidRemoveFrameFromHierarchyFrameLoadDelegate

- (void)webView:(WebView *)sender didFinishLoadForFrame:(WebFrame *)frame
{
    didFinishLoad = true;
}

- (void)webView:(WebView *)sender didRemoveFrameFromHierarchy:(WebFrame *)frame
{
    didRemoveFrame = true;
}

@end

namespace TestWebKitAPI {

TEST(WebKit1, DidRemoveFrameFromHierarchy)
{
    RetainPtr<WebView> webView = adoptNS([[WebView alloc] initWithFrame:NSMakeRect(0, 0, 120, 200) frameName:nil groupName:nil]);
    RetainPtr<DidRemoveFrameFromHierarchyFrameLoadDelegate> frameLoadDelegate = adoptNS([[DidRemoveFrameFromHierarchyFrameLoadDelegate alloc] init]);

    webView.get().frameLoadDelegate = frameLoadDelegate.get();
    WebFrame *mainFrame = webView.get().mainFrame;

    NSString *bodyWithIFrameString = @"<body><iframe id='iframe'></iframe></body>";
    NSURL *aboutBlankURL = [NSURL URLWithString:@"about:blank"];

    [mainFrame loadHTMLString:bodyWithIFrameString baseURL:aboutBlankURL];
    Util::run(&didFinishLoad);
    
    EXPECT_FALSE(didRemoveFrame);
    [webView.get() stringByEvaluatingJavaScriptFromString:@"document.body.removeChild(document.getElementById('iframe'))"];
    EXPECT_TRUE(didRemoveFrame);

    didFinishLoad = false;
    didRemoveFrame = false;

    [mainFrame loadHTMLString:bodyWithIFrameString baseURL:aboutBlankURL];
    Util::run(&didFinishLoad);

    // The delegate method is not called when the frame is removed due to navigation in an ancestor frame.
    EXPECT_FALSE(didRemoveFrame);
    [mainFrame loadHTMLString:@"<body></body>" baseURL:aboutBlankURL];
    EXPECT_FALSE(didRemoveFrame);
}

} // namespace TestWebKitAPI
