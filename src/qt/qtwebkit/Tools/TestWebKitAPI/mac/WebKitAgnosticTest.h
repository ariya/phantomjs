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

#ifndef WebKitAgnosticTest_h
#define WebKitAgnosticTest_h

#include "PlatformUtilities.h"

namespace TestWebKitAPI {

// This test fixture can be used to write test that work with both WebKit1 and WebKit2. Calling
// runWebKit1Test or runWebKit2Test will create a WebView or WKView (respectively), load the URL
// specified by url(), and then call didLoadURL. Your test's logic should go in didLoadURL.
class WebKitAgnosticTest : public ::testing::Test {
public:
    WebKitAgnosticTest();

    void runWebKit1Test();
    void runWebKit2Test();

    void loadURL(WebView *, NSURL *);
    void loadURL(WKView *, NSURL *);

    void goBack(WebView *);
    void goBack(WKView *);

    void waitForLoadToFinish();

    NSRect viewFrame;

private:
    virtual NSURL *url() const = 0;
    virtual void didLoadURL(WebView *) = 0;
    virtual void didLoadURL(WKView *) = 0;

    virtual void initializeView(WebView *) { }
    virtual void initializeView(WKView *) { }

    virtual void teardownView(WebView *) { }
    virtual void teardownView(WKView *) { }

    bool didFinishLoad;
};

} // namespace TestWebKitAPI

#endif // WebKitAgnosticTest_h
