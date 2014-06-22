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
#include "PlatformUtilities.h"
#include "PlatformWebView.h"
#include <wtf/RetainPtr.h>

#include <WebKit/WebBackForwardList.h>

@interface BackForwardListTest : NSObject {
}
@end

static bool didFinishLoad;

@implementation BackForwardListTest

- (void)webView:(WebView *)sender didFinishLoadForFrame:(WebFrame *)frame
{
    didFinishLoad = true;
}

- (void)webView:(WebView *)sender didFailProvisionalLoadWithError:(NSError *)error forFrame:(WebFrame *)frame
{
    [frame loadAlternateHTMLString:@"<html></html>" baseURL:[NSURL URLWithString:@"about:blank"] forUnreachableURL:[[error userInfo] valueForKey:NSURLErrorFailingURLErrorKey]];
}

@end

namespace TestWebKitAPI {

TEST(WebKit1, ReloadBackForward)
{
    RetainPtr<WebView> webView = adoptNS([[WebView alloc] initWithFrame:NSMakeRect(0, 0, 120, 200) frameName:nil groupName:nil]);
    RetainPtr<BackForwardListTest> testController = adoptNS([BackForwardListTest new]);
    webView.get().frameLoadDelegate = testController.get();
    [[webView.get() mainFrame] loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:@"http://does-not-exist.example"]]];
    Util::run(&didFinishLoad);
    didFinishLoad = false;

    [[webView.get() mainFrame] reload];
    Util::run(&didFinishLoad);
    didFinishLoad = false;

    WebBackForwardList *bfList = [webView.get() backForwardList];
    EXPECT_EQ(0, [bfList backListCount]);
}

} // namespace TestWebKitAPI
