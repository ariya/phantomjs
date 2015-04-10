/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

class WMSysCommandObserver : public WindowMessageObserver {
public:
    WMSysCommandObserver() : m_windowDidReceiveWMSysCommand(false) { }

    bool windowDidReceiveWMSysCommand() const { return m_windowDidReceiveWMSysCommand; }

private:
    virtual void windowReceivedMessage(HWND, UINT message, WPARAM, LPARAM)
    {
        if (message == WM_SYSCOMMAND)
            m_windowDidReceiveWMSysCommand = true;
    }

    bool m_windowDidReceiveWMSysCommand;
};

static bool didNotHandleWMSysKeyUp;

static void didNotHandleKeyEventCallback(WKPageRef, WKNativeEventPtr event, const void*)
{
    if (event->message != WM_SYSKEYUP)
        return;

    didNotHandleWMSysKeyUp = true;
}

TEST(WebKit2, AltKeyGeneratesWMSysCommand)
{
    WKRetainPtr<WKContextRef> context(AdoptWK, WKContextCreate());
    PlatformWebView webView(context.get());

    WKPageUIClient uiClient;
    memset(&uiClient, 0, sizeof(uiClient));

    uiClient.didNotHandleKeyEvent = didNotHandleKeyEventCallback;
    WKPageSetPageUIClient(webView.page(), &uiClient);

    WMSysCommandObserver observer;
    webView.setParentWindowMessageObserver(&observer);

    webView.simulateAltKeyPress();

    Util::run(&didNotHandleWMSysKeyUp);

    webView.setParentWindowMessageObserver(0);

    // The WM_SYSKEYUP message should have generated a WM_SYSCOMMAND message that was sent to the
    // WKView's parent window.
    EXPECT_TRUE(observer.windowDidReceiveWMSysCommand());
}

} // namespace TestWebKitAPI
