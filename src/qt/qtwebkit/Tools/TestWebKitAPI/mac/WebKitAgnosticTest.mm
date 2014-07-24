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

#include "config.h"
#include "WebKitAgnosticTest.h"

#include <WebKit2/WKURLCF.h>
#include <WebKit2/WKViewPrivate.h>
#include <wtf/RetainPtr.h>

@interface FrameLoadDelegate : NSObject {
    bool* _didFinishLoad;
}

- (id)initWithDidFinishLoadBoolean:(bool*)didFinishLoad;

@end

@implementation FrameLoadDelegate

- (id)initWithDidFinishLoadBoolean:(bool*)didFinishLoad
{
    self = [super init];
    if (!self)
        return nil;

    _didFinishLoad = didFinishLoad;
    return self;
}

- (void)webView:(WebView *)webView didFinishLoadForFrame:(WebFrame *)webFrame
{
    *_didFinishLoad = true;
}

@end

namespace TestWebKitAPI {

static void didFinishLoadForFrame(WKPageRef, WKFrameRef, WKTypeRef, const void* context)
{
    *static_cast<bool*>(const_cast<void*>(context)) = true;
}

static void setPageLoaderClient(WKPageRef page, bool* didFinishLoad)
{
    WKPageLoaderClient loaderClient;
    memset(&loaderClient, 0, sizeof(loaderClient));
    loaderClient.version = 0;
    loaderClient.clientInfo = didFinishLoad;
    loaderClient.didFinishLoadForFrame = didFinishLoadForFrame;

    WKPageSetPageLoaderClient(page, &loaderClient);
}

WebKitAgnosticTest::WebKitAgnosticTest()
    : viewFrame(NSMakeRect(0, 0, 800, 600))
    , didFinishLoad(false)
{
}

void WebKitAgnosticTest::runWebKit1Test()
{
    RetainPtr<WebView> webView = adoptNS([[WebView alloc] initWithFrame:viewFrame]);
    RetainPtr<FrameLoadDelegate> delegate = adoptNS([[FrameLoadDelegate alloc] initWithDidFinishLoadBoolean:&didFinishLoad]);
    [webView.get() setFrameLoadDelegate:delegate.get()];
    initializeView(webView.get());

    loadURL(webView.get(), url());
    waitForLoadToFinish();
    didLoadURL(webView.get());
    teardownView(webView.get());
}

void WebKitAgnosticTest::runWebKit2Test()
{
    WKRetainPtr<WKContextRef> context = adoptWK(WKContextCreate());
    WKRetainPtr<WKPageGroupRef> pageGroup = adoptWK(WKPageGroupCreateWithIdentifier(Util::toWK("WebKitAgnosticTest").get()));
    RetainPtr<WKView> view = adoptNS([[WKView alloc] initWithFrame:viewFrame contextRef:context.get() pageGroupRef:pageGroup.get()]);
    setPageLoaderClient([view.get() pageRef], &didFinishLoad);
    initializeView(view.get());

    loadURL(view.get(), url());
    waitForLoadToFinish();
    didLoadURL(view.get());
    teardownView(view.get());
}

void WebKitAgnosticTest::loadURL(WebView *webView, NSURL *url)
{
    EXPECT_FALSE(didFinishLoad);
    [[webView mainFrame] loadRequest:[NSURLRequest requestWithURL:url]];
}

void WebKitAgnosticTest::loadURL(WKView *view, NSURL *url)
{
    EXPECT_FALSE(didFinishLoad);
    WKPageLoadURL([view pageRef], adoptWK(WKURLCreateWithCFURL((CFURLRef)url)).get());
}

void WebKitAgnosticTest::goBack(WebView *webView)
{
    EXPECT_FALSE(didFinishLoad);
    [webView goBack];
}

void WebKitAgnosticTest::goBack(WKView *view)
{
    EXPECT_FALSE(didFinishLoad);
    WKPageGoBack([view pageRef]);
}

void WebKitAgnosticTest::waitForLoadToFinish()
{
    Util::run(&didFinishLoad);
    didFinishLoad = false;
}

} // namespace TestWebKitAPI
