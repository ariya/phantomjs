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
#include "JavaScriptTest.h"
#include "PlatformUtilities.h"
#include <wtf/RetainPtr.h>


static bool didFinishLoad;

@interface WindowlessWebViewWithMediaFrameLoadDelegate : NSObject
@end

@implementation WindowlessWebViewWithMediaFrameLoadDelegate

- (void)webView:(WebView *)sender didFinishLoadForFrame:(WebFrame *)frame
{
    didFinishLoad = true;
}

@end


namespace TestWebKitAPI {

static void spinLoop(NSTimeInterval timeout, BOOL (^block)())
{
    if (timeout <= 0)
        return;

    NSTimeInterval end = [[NSDate date] timeIntervalSinceReferenceDate] + timeout;
    NSDate *endDate = [NSDate dateWithTimeIntervalSinceReferenceDate:end];

    while (!block()) {
        [[NSRunLoop currentRunLoop] runMode:NSDefaultRunLoopMode beforeDate:endDate];
        if ([[NSDate date] timeIntervalSinceReferenceDate] > end)
            break;
    }
}

TEST(WebKit1, WindowlessWebViewWithMedia)
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    RetainPtr<WebView> webView = adoptNS([[WebView alloc] initWithFrame:NSMakeRect(0, 0, 120, 200) frameName:nil groupName:nil]);
    RetainPtr<WindowlessWebViewWithMediaFrameLoadDelegate> testController = adoptNS([WindowlessWebViewWithMediaFrameLoadDelegate new]);
    webView.get().frameLoadDelegate = testController.get();
    [[webView.get() mainFrame] loadRequest:[NSURLRequest requestWithURL:[[NSBundle mainBundle] URLForResource:@"WindowlessWebViewWithMedia" withExtension:@"html" subdirectory:@"TestWebKitAPI.resources"]]];

    EXPECT_EQ(nil, [webView.get() window]);

    Util::run(&didFinishLoad);    

    spinLoop(0.25, ^{
        return [[webView.get() stringByEvaluatingJavaScriptFromString:@"window.didTriggerLoad"] isEqualToString:@"true"];
    });

    EXPECT_JS_EQ(webView.get(), "window.didTriggerLoad", "true");

    [pool drain];
}

} // namespace TestWebKitAPI
