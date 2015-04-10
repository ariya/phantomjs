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
#import <WebKit/WebFramePrivate.h>
#import <wtf/RetainPtr.h>

@interface ElementAtPointFrameLoadDelegate : NSObject
@end

static bool didFinishLoad;

@implementation ElementAtPointFrameLoadDelegate

- (void)webView:(WebView *)sender didFinishLoadForFrame:(WebFrame *)frame
{
    didFinishLoad = true;
}

@end

namespace TestWebKitAPI {

TEST(WebKit1, ElementAtPoint)
{
    RetainPtr<WebView> webView = adoptNS([[WebView alloc] initWithFrame:NSMakeRect(0, 0, 120, 200) frameName:nil groupName:nil]);
    RetainPtr<ElementAtPointFrameLoadDelegate> frameLoadDelegate = adoptNS([[ElementAtPointFrameLoadDelegate alloc] init]);

    webView.get().frameLoadDelegate = frameLoadDelegate.get();
    WebFrame *mainFrame = webView.get().mainFrame;

    [mainFrame loadHTMLString:@"<style> div { position:absolute; width:60px; height:100px; } </style> <div name='first'></div> <div name='second' style='left:60px; top:100px;'></div>"
                      baseURL:[NSURL URLWithString:@"about:blank"]];

    Util::run(&didFinishLoad);
    
    NSDictionary *elementDictionary = [mainFrame elementAtPoint:NSMakePoint(30, 50)];
    DOMElement *domElement = [elementDictionary objectForKey:WebElementDOMNodeKey];
    EXPECT_WK_STREQ(@"first", [domElement getAttribute:@"name"]);

    elementDictionary = [mainFrame elementAtPoint:NSMakePoint(90, 150)];
    domElement = [elementDictionary objectForKey:WebElementDOMNodeKey];
    EXPECT_WK_STREQ(@"second", [domElement getAttribute:@"name"]);

    elementDictionary = [mainFrame elementAtPoint:NSMakePoint(30, 150)];
    domElement = [elementDictionary objectForKey:WebElementDOMNodeKey];
    EXPECT_WK_STREQ(@"BODY", [domElement tagName]);
}

} // namespace TestWebKitAPI
