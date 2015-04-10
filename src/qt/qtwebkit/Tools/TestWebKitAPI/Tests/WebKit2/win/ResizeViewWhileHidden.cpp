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
#include <WebKit2/WKRetainPtr.h>

namespace TestWebKitAPI {

static bool didFinishLoad;

static void didFinishLoadForFrame(WKPageRef, WKFrameRef, WKTypeRef, const void*)
{
    didFinishLoad = true;
}

static void setPageLoaderClient(WKPageRef page)
{
    WKPageLoaderClient loaderClient;
    memset(&loaderClient, 0, sizeof(loaderClient));
    loaderClient.version = 0;
    loaderClient.didFinishLoadForFrame = didFinishLoadForFrame;

    WKPageSetPageLoaderClient(page, &loaderClient);
}

static void flushMessages(WKPageRef page)
{
    // In order to ensure all pending messages have been handled by the UI and web processes, we
    // load a URL and wait for the load to finish.

    setPageLoaderClient(page);

    WKPageLoadURL(page, adoptWK(Util::createURLForResource("simple", "html")).get());
    Util::run(&didFinishLoad);
    didFinishLoad = false;

    WKPageSetPageLoaderClient(page, 0);
}

static bool timerFired;
static void CALLBACK timerCallback(HWND hwnd, UINT, UINT_PTR timerID, DWORD)
{
    ::KillTimer(hwnd, timerID);
    timerFired = true;
}

static void runForDuration(double seconds)
{
    ::SetTimer(0, 0, seconds * 1000, timerCallback);
    Util::run(&timerFired);
    timerFired = false;
}

static void waitForBackingStoreUpdate(WKPageRef page)
{
    // Wait for the web process to handle the changes we just made, to perform a display (which
    // happens on a timer), and to tell the UI process about the display (which updates the backing
    // store).
    // FIXME: It would be much less fragile (and maybe faster) to have an explicit way to wait
    // until the backing store is updated.
    runForDuration(0.5);
    flushMessages(page);
}

TEST(WebKit2, ResizeViewWhileHidden)
{
    WKRetainPtr<WKContextRef> context(AdoptWK, WKContextCreate());
    PlatformWebView webView(context.get());

    HWND window = WKViewGetWindow(webView.platformView());

    RECT originalRect;
    ::GetClientRect(window, &originalRect);
    RECT newRect = originalRect;
    ::InflateRect(&newRect, 1, 1);

    // Show the WKView and resize it so that the WKView's backing store will be created. Ideally
    // we'd have some more explicit way of forcing the backing store to be created.
    ::ShowWindow(window, SW_SHOW);
    webView.resizeTo(newRect.right - newRect.left, newRect.bottom - newRect.top);

    waitForBackingStoreUpdate(webView.page());

    // Resize the window while hidden and show it again so that it will update its backing store at
    // the new size.
    ::ShowWindow(window, SW_HIDE);
    webView.resizeTo(originalRect.right - originalRect.left, originalRect.bottom - originalRect.top);
    ::ShowWindow(window, SW_SHOW);

    // Force the WKView to paint to try to trigger <http://webkit.org/b/54142>.
    ::SendMessage(window, WM_PAINT, 0, 0);

    // In Debug builds without the fix for <http://webkit.org/b/54141>, the web process will assert
    // at this point.
    // FIXME: It would be good to have a way to check that our behavior is correct in Release
    // builds, too!
    waitForBackingStoreUpdate(webView.page());
}

} // namespace TestWebKitAPI
