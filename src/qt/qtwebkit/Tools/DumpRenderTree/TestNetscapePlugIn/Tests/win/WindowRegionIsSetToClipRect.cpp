/*
 * Copyright (C) 2010, 2011 Apple Inc. All rights reserved.
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

#include "WindowGeometryTest.h"

#include "PluginObject.h"

using namespace std;

// The plugin's window's window region should be set to the plugin's clip rect.

class WindowRegionIsSetToClipRect : public WindowGeometryTest {
public:
    WindowRegionIsSetToClipRect(NPP, const string& identifier);

private:
    virtual void performWindowGeometryTest();
};

static PluginTest::Register<WindowRegionIsSetToClipRect> registrar("window-region-is-set-to-clip-rect");

WindowRegionIsSetToClipRect::WindowRegionIsSetToClipRect(NPP npp, const string& identifier)
    : WindowGeometryTest(npp, identifier)
{
}

void WindowRegionIsSetToClipRect::performWindowGeometryTest()
{
    RECT regionRect;
    if (::GetWindowRgnBox(window(), &regionRect) == ERROR) {
        log("::GetWindowRgnBox failed, or window has no window region");
        return;
    }

    // This expected rect is based on the layout of window-region-is-set-to-clip-rect.html.
    RECT expectedRect = { 50, 50, 100, 100 };
    if (!::EqualRect(&regionRect, &expectedRect)) {
        log("Expected region rect {left=%u, top=%u, right=%u, bottom=%u}, but got {left=%d, top=%d, right=%d, bottom=%d}", expectedRect.left, expectedRect.top, expectedRect.right, expectedRect.bottom, regionRect.left, regionRect.top, regionRect.right, regionRect.bottom);
        return;
    }

    log("PASS: Plugin's window's window region has been set as expected");

    // While we're here, check that our window class doesn't have the CS_PARENTDC style, which
    // defeats clipping by ignoring the window region and always clipping to the parent window.
    // FIXME: It would be nice to have a pixel test that shows that we're
    // getting clipped correctly, but unfortunately window regions are ignored
    // during WM_PRINT (see <http://webkit.org/b/49034>).
    wchar_t className[512];
    if (!::GetClassNameW(window(), className, _countof(className))) {
        log("::GetClassName failed with error %u", ::GetLastError());
        return;
    }

#ifdef DEBUG_ALL
    const wchar_t webKitDLLName[] = L"WebKit_debug.dll";
#else
    const wchar_t webKitDLLName[] = L"WebKit.dll";
#endif
    HMODULE webKitModule = ::GetModuleHandleW(webKitDLLName);
    if (!webKitModule) {
        log("::GetModuleHandleW failed with error %u", ::GetLastError());
        return;
    }

    WNDCLASSW wndClass;
    if (!::GetClassInfoW(webKitModule, className, &wndClass)) {
        log("::GetClassInfoW failed with error %u", ::GetLastError());
        return;
    }

    if (wndClass.style & CS_PARENTDC)
        log("FAIL: Plugin's window's class has the CS_PARENTDC style, which will defeat clipping");
    else
        log("PASS: Plugin's window's class does not have the CS_PARENTDC style");
}
