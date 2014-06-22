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
#include "WebKitAgnosticTest.h"

#include <wtf/RetainPtr.h>

@interface NSApplication (TestWebKitAPINSApplicationDetails)
- (void)_setCurrentEvent:(NSEvent *)event;
@end

namespace TestWebKitAPI {

class AcceptsFirstMouse : public WebKitAgnosticTest {
public:
    template <typename View> void runTest(View);

    // WebKitAgnosticTest
    virtual NSURL *url() const { return [[NSBundle mainBundle] URLForResource:@"acceptsFirstMouse" withExtension:@"html" subdirectory:@"TestWebKitAPI.resources"]; }
    virtual void didLoadURL(WebView *webView) { runTest(webView); }
    virtual void didLoadURL(WKView *wkView) { runTest(wkView); }
};

template <typename View>
void AcceptsFirstMouse::runTest(View view)
{
    RetainPtr<NSWindow> window = adoptNS([[NSWindow alloc] initWithContentRect:view.frame styleMask:NSBorderlessWindowMask backing:NSBackingStoreBuffered defer:YES]);
    [[window.get() contentView] addSubview:view];

    CGFloat viewHeight = view.bounds.size.height;

    NSPoint pointInsideSelection = NSMakePoint(50, viewHeight - 50);
    NSEvent *mouseEventInsideSelection = [NSEvent mouseEventWithType:NSLeftMouseDown location:pointInsideSelection modifierFlags:0 timestamp:0 windowNumber:[window.get() windowNumber] context:nil eventNumber:0 clickCount:1 pressure:1];
    EXPECT_TRUE([[view hitTest:pointInsideSelection] acceptsFirstMouse:mouseEventInsideSelection]);

    NSPoint pointOutsideSelection = NSMakePoint(50, viewHeight - 150);
    NSEvent *mouseEventOutsideSelection = [NSEvent mouseEventWithType:NSLeftMouseDown location:pointOutsideSelection modifierFlags:0 timestamp:0 windowNumber:[window.get() windowNumber] context:nil eventNumber:0 clickCount:1 pressure:1];
    EXPECT_FALSE([[view hitTest:pointInsideSelection] acceptsFirstMouse:mouseEventOutsideSelection]);
}

TEST_F(AcceptsFirstMouse, WebKit)
{
    // Ensure that [NSApp currentEvent] is not a previously-simulated spacebar key press, since this
    // causes the scrollBy() in the test to perform a smooth scroll.
    [NSApp _setCurrentEvent:nil];
    runWebKit1Test();
}

TEST_F(AcceptsFirstMouse, WebKit2)
{
    runWebKit2Test();
}

} // namespace TestWebKitAPI
