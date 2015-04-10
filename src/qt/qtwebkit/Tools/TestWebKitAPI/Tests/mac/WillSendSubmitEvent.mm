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
#import "PlatformWebView.h"
#import "Test.h"

#import <WebKit/WebFormDelegate.h>
#import <WebKit/WebViewPrivate.h>
#import <wtf/RetainPtr.h>

static bool didFinishLoad;

@interface FormDelegate : WebFormDelegate
@end

@implementation FormDelegate

- (void)willSendSubmitEventToForm:(DOMHTMLFormElement *)element inFrame:(WebFrame *)sourceFrame withValues:(NSDictionary *)values
{
    EXPECT_NOT_NULL(element);
    EXPECT_NOT_NULL(sourceFrame);

    EXPECT_WK_STREQ([values objectForKey:@"textField"], @"text field");
    EXPECT_WK_STREQ([values objectForKey:@"passwordField"], @"password field");

    // <input type="hidden"> fields are not sent.
    EXPECT_NULL([values objectForKey:@"hiddenField"]);

    didFinishLoad = true;
}

@end

namespace TestWebKitAPI {

TEST(WebKit1, WillSendSubmitEvent)
{
    @autoreleasepool {
        RetainPtr<WebView> webView = adoptNS([[WebView alloc] initWithFrame:NSMakeRect(0, 0, 120, 200) frameName:nil groupName:nil]);

        RetainPtr<FormDelegate> formDelegate = [[FormDelegate alloc] init];
        [webView _setFormDelegate:formDelegate.get()];

        [[webView.get() mainFrame] loadRequest:[NSURLRequest requestWithURL:[[NSBundle mainBundle] URLForResource:@"auto-submitting-form" withExtension:@"html" subdirectory:@"TestWebKitAPI.resources"]]];

        Util::run(&didFinishLoad);
    }
}

}
