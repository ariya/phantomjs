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
#include <wtf/RetainPtr.h>

#import <WebKit/DOMPrivate.h>
#import <WebKit/WebViewPrivate.h>

@interface HTMLTableCellElementCellAboveTest : NSObject
@end

static bool didFinishLoad;

@implementation HTMLTableCellElementCellAboveTest

- (void)webView:(WebView *)sender didFinishLoadForFrame:(WebFrame *)frame
{
    didFinishLoad = true;
}
@end

namespace TestWebKitAPI {

static void expectCellAboveCell(DOMDocument *document, NSString *cellID, NSString *cellAboveID)
{
    DOMHTMLTableCellElement *cell = (DOMHTMLTableCellElement *)[document getElementById:cellID];
    DOMHTMLTableCellElement *cellAbove = [cell _cellAbove];

    EXPECT_WK_STREQ(cellAboveID, [cellAbove getAttribute:@"id"]);
}

TEST(WebKit1, HTMLTableCellElementCellAbove)
{
    RetainPtr<WebView> webView = adoptNS([[WebView alloc] initWithFrame:NSMakeRect(0, 0, 120, 200) frameName:nil groupName:nil]);
    RetainPtr<HTMLTableCellElementCellAboveTest> testController = adoptNS([HTMLTableCellElementCellAboveTest new]);

    webView.get().frameLoadDelegate = testController.get();
    [[webView.get() mainFrame] loadRequest:[NSURLRequest requestWithURL:[[NSBundle mainBundle]
        URLForResource:@"DOMHTMLTableCellElementCellAbove" withExtension:@"html" subdirectory:@"TestWebKitAPI.resources"]]];

    Util::run(&didFinishLoad);
    didFinishLoad = false;

    DOMDocument *document = webView.get().mainFrameDocument;

    expectCellAboveCell(document, @"cell-4-2", @"cell-3-2");
    expectCellAboveCell(document, @"cell-3-1", @"cell-2-1");
    expectCellAboveCell(document, @"cell-2-1", @"cell-1-1");
    expectCellAboveCell(document, @"cell-1-2", @"cell-h-2");
}

} // namespace TestWebKitAPI
