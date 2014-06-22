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
#import "Test.h"

#import "PlatformUtilities.h"
#import "PlatformWebView.h"
#import "TestBrowsingContextLoadDelegate.h"
#import "TestProtocol.h"
#import <WebKit2/WebKit2.h>
#import <WebKit2/WKViewPrivate.h>

static bool testFinished = false;

namespace TestWebKitAPI {

TEST(WebKit2, PreventImageLoadWithAutoResizingTest)
{
    [NSURLProtocol registerClass:[TestProtocol class]];
    [WKBrowsingContextController registerSchemeForCustomProtocol:[TestProtocol scheme]];

    WKRetainPtr<WKContextRef> context = adoptWK(Util::createContextForInjectedBundleTest("DenyWillSendRequestTest"));
    PlatformWebView webView(context.get());

    webView.platformView().minimumSizeForAutoLayout = NSMakeSize(400, 300);
    webView.platformView().browsingContextController.loadDelegate = [[TestBrowsingContextLoadDelegate alloc] initWithBlockToRunOnLoad:^(WKBrowsingContextController *sender) {
        testFinished = true;
    }];
    [webView.platformView().browsingContextController loadHTMLString:@"<html><body style='background-image:url(data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAoAAAAKCAYAAACNMs+9AAAAGXRFWHRTb2Z0d2FyZQBBZG9iZSBJbWFnZVJlYWR5ccllPAAAAFZJREFUeF59z4EJADEIQ1F36k7u5E7ZKXeUQPACJ3wK7UNokVxVk9kHnQH7bY9hbDyDhNXgjpRLqFlo4M2GgfyJHhjq8V4agfrgPQX3JtJQGbofmCHgA/nAKks+JAjFAAAAAElFTkSuQmCC);'></body></html>" baseURL:[NSURL URLWithString:@"about:blank"]];

    Util::run(&testFinished);
}

} // namespace TestWebKitAPI
