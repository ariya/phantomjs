/*
 * Copyright (C) 2013 Nokia Corporation and/or its subsidiary(-ies).
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "WebKit2/WKView.h"
#include "WebKit2/WKRetainPtr.h"

namespace TestWebKitAPI {

TEST(WebKit2, WKViewUserViewportToContents)
{
    // This test creates a WKView and uses the WKViewUserViewportToContents
    // function to convert viewport coordinates to contents (page) coordinates.
    // It scrolls and scales around the contents and check if the coordinates
    // conversion math is right.

    WKRetainPtr<WKContextRef> context(AdoptWK, WKContextCreate());
    WKRetainPtr<WKViewRef> webView(AdoptWK, WKViewCreate(context.get(), 0));

    WKViewInitialize(webView.get());
    WKPageSetUseFixedLayout(WKViewGetPage(webView.get()), false);

    WKPoint out;

    // At scale 1.0 the viewport and contents coordinates should match.

    WKViewSetContentScaleFactor(webView.get(), 1.0);
    WKViewSetContentPosition(webView.get(), WKPointMake(0, 0));

    out = WKViewUserViewportToContents(webView.get(), WKPointMake(0, 0));
    EXPECT_EQ(out.x, 0);
    EXPECT_EQ(out.y, 0);

    out = WKViewUserViewportToContents(webView.get(), WKPointMake(10, 10));
    EXPECT_EQ(out.x, 10);
    EXPECT_EQ(out.y, 10);

    WKViewSetContentPosition(webView.get(), WKPointMake(20, 20));

    out = WKViewUserViewportToContents(webView.get(), WKPointMake(0, 0));
    EXPECT_EQ(out.x, 20);
    EXPECT_EQ(out.y, 20);

    out = WKViewUserViewportToContents(webView.get(), WKPointMake(10, 10));
    EXPECT_EQ(out.x, 30);
    EXPECT_EQ(out.y, 30);

    // At scale 2.0 the viewport distance values will be half
    // the ones seem in the contents.

    WKViewSetContentScaleFactor(webView.get(), 2.0);
    WKViewSetContentPosition(webView.get(), WKPointMake(0, 0));

    out = WKViewUserViewportToContents(webView.get(), WKPointMake(0, 0));
    EXPECT_EQ(out.x, 0);
    EXPECT_EQ(out.y, 0);

    out = WKViewUserViewportToContents(webView.get(), WKPointMake(10, 10));
    EXPECT_EQ(out.x, 5);
    EXPECT_EQ(out.y, 5);

    WKViewSetContentPosition(webView.get(), WKPointMake(20, 20));

    out = WKViewUserViewportToContents(webView.get(), WKPointMake(0, 0));
    EXPECT_EQ(out.x, 20);
    EXPECT_EQ(out.y, 20);

    out = WKViewUserViewportToContents(webView.get(), WKPointMake(10, 10));
    EXPECT_EQ(out.x, 25);
    EXPECT_EQ(out.y, 25);

    // At scale 0.5 the viewport distance values will be twice
    // the ones seem in the contents.

    WKViewSetContentScaleFactor(webView.get(), 0.5);
    WKViewSetContentPosition(webView.get(), WKPointMake(0, 0));

    out = WKViewUserViewportToContents(webView.get(), WKPointMake(0, 0));
    EXPECT_EQ(out.x, 0);
    EXPECT_EQ(out.y, 0);

    out = WKViewUserViewportToContents(webView.get(), WKPointMake(10, 10));
    EXPECT_EQ(out.x, 20);
    EXPECT_EQ(out.y, 20);

    WKViewSetContentPosition(webView.get(), WKPointMake(20, 20));

    out = WKViewUserViewportToContents(webView.get(), WKPointMake(0, 0));
    EXPECT_EQ(out.x, 20);
    EXPECT_EQ(out.y, 20);

    out = WKViewUserViewportToContents(webView.get(), WKPointMake(10, 10));
    EXPECT_EQ(out.x, 40);
    EXPECT_EQ(out.y, 40);

    // Let's add translation to the viewport.

    const int delta = 10;
    WKViewSetUserViewportTranslation(webView.get(), delta, delta);

    WKViewSetContentPosition(webView.get(), WKPointMake(0, 0));

    out = WKViewUserViewportToContents(webView.get(), WKPointMake(0, 0));
    EXPECT_EQ(out.x, 0 - delta / 0.5);
    EXPECT_EQ(out.y, 0 - delta / 0.5);

    out = WKViewUserViewportToContents(webView.get(), WKPointMake(10, 10));
    EXPECT_EQ(out.x, 20 - delta / 0.5);
    EXPECT_EQ(out.y, 20 - delta / 0.5);

    WKViewSetContentPosition(webView.get(), WKPointMake(20, 20));

    out = WKViewUserViewportToContents(webView.get(), WKPointMake(0, 0));
    EXPECT_EQ(out.x, 20 - delta / 0.5);
    EXPECT_EQ(out.y, 20 - delta / 0.5);

    out = WKViewUserViewportToContents(webView.get(), WKPointMake(10, 10));
    EXPECT_EQ(out.x, 40 - delta / 0.5);
    EXPECT_EQ(out.y, 40 - delta / 0.5);
}

}
