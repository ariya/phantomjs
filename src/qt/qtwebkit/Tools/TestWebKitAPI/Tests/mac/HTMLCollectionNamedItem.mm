/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

@interface HTMLCollectionNamedItemTest : NSObject {
}
@end

static bool didFinishLoad;

@implementation HTMLCollectionNamedItemTest

- (void)webView:(WebView *)sender didFinishLoadForFrame:(WebFrame *)frame
{
    didFinishLoad = true;
}
@end

namespace TestWebKitAPI {

TEST(WebKit1, HTMLCollectionNamedItemTest)
{
    RetainPtr<WebView> webView = adoptNS([[WebView alloc] initWithFrame:NSMakeRect(0, 0, 120, 200) frameName:nil groupName:nil]);
    RetainPtr<HTMLCollectionNamedItemTest> testController = adoptNS([HTMLCollectionNamedItemTest new]);

    webView.get().frameLoadDelegate = testController.get();
    [[webView.get() mainFrame] loadRequest:[NSURLRequest requestWithURL:[[NSBundle mainBundle]
        URLForResource:@"HTMLCollectionNamedItem" withExtension:@"html" subdirectory:@"TestWebKitAPI.resources"]]];

    Util::run(&didFinishLoad);
    didFinishLoad = false;

    DOMDocument *document = webView.get().mainFrameDocument;
    RetainPtr<DOMHTMLCollection> collection = [[document body] children];

    EXPECT_EQ([collection.get() length], (unsigned)4);
    EXPECT_WK_STREQ([[collection.get() item:0] value], @"firstItem");
    EXPECT_WK_STREQ([[collection.get() item:1] value], @"secondItem");
    EXPECT_WK_STREQ([[collection.get() namedItem:@"idForTwoTextFields"] value], @"firstItem");
    EXPECT_WK_STREQ([[collection.get() item:1] value], @"secondItem");
    EXPECT_WK_STREQ([[collection.get() item:0] value], @"firstItem");

    EXPECT_WK_STREQ([(DOMHTMLElement*)[collection.get() item:2] title], @"thirdItem");
    EXPECT_WK_STREQ([(DOMHTMLElement*)[collection.get() item:3] title], @"fourthItem");
    EXPECT_WK_STREQ([(DOMHTMLElement*)[collection.get() namedItem:@"nameForTwoImages"] title], @"thirdItem");
    EXPECT_WK_STREQ([(DOMHTMLElement*)[collection.get() item:3] title], @"fourthItem");
    EXPECT_WK_STREQ([(DOMHTMLElement*)[collection.get() item:2] title], @"thirdItem");
}

} // namespace TestWebKitAPI
