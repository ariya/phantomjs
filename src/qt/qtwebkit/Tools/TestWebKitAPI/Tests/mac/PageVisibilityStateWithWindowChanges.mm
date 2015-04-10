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
#import "JavaScriptTest.h"
#import "Test.h"
#import "WebKitAgnosticTest.h"
#import <WebKit/WebView.h>
#import <WebKit2/WKViewPrivate.h>
#import <wtf/RetainPtr.h>

static bool didGetPageSignalToContinue;

// WebKit1 WebUIDelegate

@interface PageVisibilityStateDelegate : NSObject
@end

@implementation PageVisibilityStateDelegate

- (void)webView:(WebView *)sender runJavaScriptAlertPanelWithMessage:(NSString *)message initiatedByFrame:(WebFrame *)frame
{
    didGetPageSignalToContinue = true;
}

@end

// WebKit2 WKPageUIClient

static void runJavaScriptAlert(WKPageRef page, WKStringRef alertText, WKFrameRef frame, const void* clientInfo)
{
    didGetPageSignalToContinue = true;
}

// WebKitAgnosticTest

namespace TestWebKitAPI {

class PageVisibilityStateWithWindowChanges : public WebKitAgnosticTest {
public:
    template <typename View> void runTest(View);

    // WebKitAgnosticTest
    virtual NSURL *url() const OVERRIDE { return [[NSBundle mainBundle] URLForResource:@"PageVisibilityStateWithWindowChanges" withExtension:@"html" subdirectory:@"TestWebKitAPI.resources"]; }
    virtual void didLoadURL(WebView *webView) OVERRIDE { runTest(webView); }
    virtual void didLoadURL(WKView *wkView) OVERRIDE { runTest(wkView); }

    // Setup and teardown the UIDelegate which gets alert() signals from the page.
    virtual void initializeView(WebView *) OVERRIDE;
    virtual void initializeView(WKView *) OVERRIDE;
    virtual void teardownView(WebView *) OVERRIDE;
    virtual void teardownView(WKView *) OVERRIDE;
};

void PageVisibilityStateWithWindowChanges::initializeView(WebView *webView)
{
    // Released in teardownView.
    webView.UIDelegate = [[PageVisibilityStateDelegate alloc] init];
}

void PageVisibilityStateWithWindowChanges::teardownView(WebView *webView)
{
    id uiDelegate = webView.UIDelegate;
    webView.UIDelegate = nil;
    [uiDelegate release];
}

void PageVisibilityStateWithWindowChanges::initializeView(WKView *wkView)
{
    WKPageUIClient uiClient;
    memset(&uiClient, 0, sizeof(uiClient));
    uiClient.version = 0;
    uiClient.clientInfo = 0;
    uiClient.runJavaScriptAlert = runJavaScriptAlert;
    WKPageSetPageUIClient(wkView.pageRef, &uiClient);
}

void PageVisibilityStateWithWindowChanges::teardownView(WKView *wkView)
{
    // We do not need to teardown the WKPageUIClient.
}

template <typename View>
void PageVisibilityStateWithWindowChanges::runTest(View view)
{
    // This WebView does not have a window and superview. PageVisibility should be "hidden".
    EXPECT_NULL([view window]);
    EXPECT_NULL([view superview]);
    EXPECT_JS_EQ(view, "document.visibilityState", "hidden");

    // Add it to a non-visible window. PageVisibility should still be "hidden".
    RetainPtr<NSWindow> window = adoptNS([[NSWindow alloc] initWithContentRect:view.frame styleMask:NSBorderlessWindowMask backing:NSBackingStoreBuffered defer:NO]);
    [window.get().contentView addSubview:view];
    EXPECT_NOT_NULL([view window]);
    EXPECT_NOT_NULL([view superview]);
    EXPECT_FALSE([window.get() isVisible]);
    EXPECT_JS_EQ(view, "document.visibilityState", "hidden");

    // Make the window visible. PageVisibility should become "visible".
    didGetPageSignalToContinue = false;    
    [window.get() makeKeyAndOrderFront:nil];
    EXPECT_TRUE([window.get() isVisible]);
    Util::run(&didGetPageSignalToContinue);
    EXPECT_JS_EQ(view, "document.visibilityState", "visible");

    // Minimize the window. PageVisibility should become "hidden".
    didGetPageSignalToContinue = false;
    [window.get() miniaturize:nil];
    Util::run(&didGetPageSignalToContinue);
    EXPECT_JS_EQ(view, "document.visibilityState", "hidden");

    // Deminimize the window. PageVisibility should become "visible".
    didGetPageSignalToContinue = false;
    [window.get() deminiaturize:nil];
    Util::run(&didGetPageSignalToContinue);
    EXPECT_JS_EQ(view, "document.visibilityState", "visible");

    // Remove the WebView from its superview. PageVisibility should become "hidden".
    didGetPageSignalToContinue = false;
    [view removeFromSuperview];
    EXPECT_NULL([view window]);
    EXPECT_NULL([view superview]);
    EXPECT_TRUE([window.get() isVisible]);
    Util::run(&didGetPageSignalToContinue);
    EXPECT_JS_EQ(view, "document.visibilityState", "hidden");
}
    
TEST_F(PageVisibilityStateWithWindowChanges, WebKit)
{
    runWebKit1Test();
}

TEST_F(PageVisibilityStateWithWindowChanges, WebKit2)
{
    runWebKit1Test();
}

} // namespace TestWebKitAPI
