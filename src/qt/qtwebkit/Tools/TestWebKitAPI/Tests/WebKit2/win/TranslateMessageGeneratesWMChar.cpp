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
#include "PlatformUtilities.h"
#include "PlatformWebView.h"
#include "WindowMessageObserver.h"
#include <WebKit2/WKRetainPtr.h>

namespace TestWebKitAPI {

static bool didSeeWMChar;
static bool didNotHandleKeyEventCalled;

static void didNotHandleKeyEventCallback(WKPageRef, WKNativeEventPtr event, const void*)
{
    if (event->message != WM_KEYDOWN)
        return;
    
    // Don't call TranslateMessage() here so a WM_CHAR isn't generated.
    didNotHandleKeyEventCalled = true;
}

static void runAndWatchForWMChar(bool* done)
{
    while (!*done) {
        MSG msg;
        BOOL result = ::GetMessageW(&msg, 0, 0, 0);
        if (!result || result == -1)
            break;
        
        if (msg.message == WM_CHAR)
            didSeeWMChar = true;
        
        if (Util::shouldTranslateMessage(msg))
            ::TranslateMessage(&msg);
        
        ::DispatchMessage(&msg);
    }
}

TEST(WebKit2, TranslateMessageGeneratesWMChar)
{
    WKRetainPtr<WKContextRef> context(AdoptWK, WKContextCreate());
    PlatformWebView webView(context.get());

    webView.simulateAKeyDown();

    // WebKit should call TranslateMessage() on the WM_KEYDOWN message to generate the WM_CHAR message.
    runAndWatchForWMChar(&didSeeWMChar);
    
    didSeeWMChar = false;

    WKPageUIClient uiClient;
    memset(&uiClient, 0, sizeof(uiClient));

    uiClient.didNotHandleKeyEvent = didNotHandleKeyEventCallback;
    WKPageSetPageUIClient(webView.page(), &uiClient);

    webView.simulateAKeyDown();

    runAndWatchForWMChar(&didNotHandleKeyEventCalled);

    // WebKit should not have called TranslateMessage() on the WM_KEYDOWN message since we installed a didNotHandleKeyEvent callback.
    EXPECT_FALSE(didSeeWMChar);
}

} // namespace TestWebKitAPI
