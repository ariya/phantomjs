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

WindowGeometryTest::WindowGeometryTest(NPP npp, const string& identifier)
    : WindowedPluginTest(npp, identifier)
    , m_testHarnessWindowWasVisible(false)
{
}

void WindowGeometryTest::startTest()
{
    // In WebKit1, our window's window region will be set immediately. In WebKit2, it won't be set
    // until the UI process paints. Since the UI process will also show our window when it paints,
    // we can detect when the paint occurs (and thus when our window region should be set) by
    // starting with our plugin element hidden, then making it visible and waiting for a
    // WM_WINDOWPOSCHANGED event to tell us our window has been shown.

    waitUntilDone();

    // If the test harness window isn't visible, we might not receive a WM_WINDOWPOSCHANGED message
    // when our window is made visible. So we temporarily show the test harness window during this test.
    showTestHarnessWindowIfNeeded();

    // Make our window visible. (In WebKit2, this won't take effect immediately.)
    executeScript("document.getElementsByTagName('embed')[0].style.visibility = 'visible';");

    // We trigger a UI process paint after a slight delay to ensure that the UI process has
    // received the "make the plugin window visible" message before it paints.
    // FIXME: It would be nice to have a way to guarantee that the UI process had received that
    // message before we triggered a paint. Hopefully that would let us get rid of this semi-
    // arbitrary timeout.
    ::SetTimer(window(), triggerPaintTimerID, 250, 0);
}

void WindowGeometryTest::finishTest()
{
    performWindowGeometryTest();
    hideTestHarnessWindowIfNeeded();
    notifyDone();
}

void WindowGeometryTest::showTestHarnessWindowIfNeeded()
{
    HWND testHarnessWindow = this->testHarnessWindow();
    m_testHarnessWindowWasVisible = ::IsWindowVisible(testHarnessWindow);
    if (m_testHarnessWindowWasVisible)
        return;
    ::ShowWindow(testHarnessWindow, SW_SHOWNA);
}

void WindowGeometryTest::hideTestHarnessWindowIfNeeded()
{
    if (m_testHarnessWindowWasVisible)
        return;
    ::ShowWindow(testHarnessWindow(), SW_HIDE);
}

LRESULT WindowGeometryTest::wndProc(UINT message, WPARAM wParam, LPARAM lParam, bool& handled)
{
    switch (message) {
    case WM_TIMER:
        if (wParam != triggerPaintTimerID)
            break;
        handled = true;
        ::KillTimer(window(), wParam);
        // Tell the UI process to paint.
        ::PostMessageW(::GetParent(window()), WM_PAINT, 0, 0);
        break;
    case WM_WINDOWPOSCHANGED: {
        WINDOWPOS* windowPos = reinterpret_cast<WINDOWPOS*>(lParam);
        if (!(windowPos->flags & SWP_SHOWWINDOW))
            break;
        finishTest();
        break;
    }

    }

    return 0;
}

NPError WindowGeometryTest::NPP_GetValue(NPPVariable variable, void* value)
{
    if (variable != NPPVpluginScriptableNPObject)
        return NPERR_GENERIC_ERROR;

    *static_cast<NPObject**>(value) = ScriptObject::create(this);

    return NPERR_NO_ERROR;
}

bool WindowGeometryTest::ScriptObject::hasMethod(NPIdentifier methodName)
{
    return methodName == pluginTest()->NPN_GetStringIdentifier("startTest");
}

bool WindowGeometryTest::ScriptObject::invoke(NPIdentifier identifier, const NPVariant*, uint32_t, NPVariant*)
{
    assert(identifier == pluginTest()->NPN_GetStringIdentifier("startTest"));
    static_cast<WindowGeometryTest*>(pluginTest())->startTest();
    return true;
}
