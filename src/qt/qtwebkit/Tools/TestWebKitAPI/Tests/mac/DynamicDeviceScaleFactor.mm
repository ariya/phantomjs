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

#include "JavaScriptTest.h"
#include "PlatformUtilities.h"
#include "SyntheticBackingScaleFactorWindow.h"
#include <wtf/RetainPtr.h>

namespace TestWebKitAPI {

class DynamicDeviceScaleFactor : public WebKitAgnosticTest {
public:
    RetainPtr<SyntheticBackingScaleFactorWindow> createWindow();

    template <typename View> void runTest(View);

    // WebKitAgnosticTest
    virtual NSURL *url() const { return [[NSBundle mainBundle] URLForResource:@"devicePixelRatio" withExtension:@"html" subdirectory:@"TestWebKitAPI.resources"]; }
    virtual void didLoadURL(WebView *webView) { runTest(webView); }
    virtual void didLoadURL(WKView *wkView) { runTest(wkView); }
};

RetainPtr<SyntheticBackingScaleFactorWindow> DynamicDeviceScaleFactor::createWindow()
{
    RetainPtr<SyntheticBackingScaleFactorWindow> window = adoptNS([[SyntheticBackingScaleFactorWindow alloc] initWithContentRect:viewFrame styleMask:NSBorderlessWindowMask backing:NSBackingStoreBuffered defer:YES]);
    [window.get() setReleasedWhenClosed:NO];
    return window;
}

template <typename View>
void DynamicDeviceScaleFactor::runTest(View view)
{
    EXPECT_JS_EQ(view, "window.devicePixelRatio", "1");
    EXPECT_JS_EQ(view, "devicePixelRatioFromStyle()", "1");

    RetainPtr<SyntheticBackingScaleFactorWindow> window1 = createWindow();
    [window1.get() setBackingScaleFactor:3];

    [[window1.get() contentView] addSubview:view];
    EXPECT_JS_EQ(view, "window.devicePixelRatio", "3");
    EXPECT_JS_EQ(view, "devicePixelRatioFromStyle()", "3");

    RetainPtr<SyntheticBackingScaleFactorWindow> window2 = createWindow();
    [window2.get() setBackingScaleFactor:4];

    [[window2.get() contentView] addSubview:view];
    EXPECT_JS_EQ(view, "window.devicePixelRatio", "4");
    EXPECT_JS_EQ(view, "devicePixelRatioFromStyle()", "4");

    [view removeFromSuperview];
    EXPECT_JS_EQ(view, "window.devicePixelRatio", "1");
    EXPECT_JS_EQ(view, "devicePixelRatioFromStyle()", "1");
}

TEST_F(DynamicDeviceScaleFactor, WebKit)
{
    runWebKit1Test();
}

TEST_F(DynamicDeviceScaleFactor, WebKit2)
{
    runWebKit2Test();
}

} // namespace TestWebKitAPI
