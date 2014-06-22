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

#include "WindowedPluginTest.h"

#include "PluginObject.h"

using namespace std;

// NPN_InvalidateRect should invalidate the plugin's HWND.

static const wchar_t instancePointerProperty[] = L"org.webkit.TestNetscapePlugin.NPNInvalidateRectInvalidatesWindow.InstancePointer";

class TemporaryWindowMover {
public:
    TemporaryWindowMover(HWND);
    ~TemporaryWindowMover();

    bool moveSucceeded() const { return m_moveSucceeded; }

private:
    static const UINT standardSetWindowPosFlags = SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER;
    bool m_moveSucceeded;
    HWND m_window;
    RECT m_savedWindowRect;
};

TemporaryWindowMover::TemporaryWindowMover(HWND window)
    : m_window(window)
{
    m_moveSucceeded = false;

    if (!::GetWindowRect(m_window, &m_savedWindowRect))
        return;

    if (!::SetWindowPos(m_window, 0, 0, 0, 0, 0, SWP_SHOWWINDOW | standardSetWindowPosFlags))
        return;

    m_moveSucceeded = true;
};

TemporaryWindowMover::~TemporaryWindowMover()
{
    if (!m_moveSucceeded)
        return;

    ::SetWindowPos(m_window, 0, m_savedWindowRect.left, m_savedWindowRect.top, 0, 0, SWP_HIDEWINDOW | standardSetWindowPosFlags);
}

class NPNInvalidateRectInvalidatesWindow : public WindowedPluginTest {
public:
    NPNInvalidateRectInvalidatesWindow(NPP, const string& identifier);
    ~NPNInvalidateRectInvalidatesWindow();

private:
    virtual LRESULT wndProc(UINT message, WPARAM, LPARAM, bool& handled);

    void onPaint();
    void testInvalidateRect();

    virtual NPError NPP_SetWindow(NPP, NPWindow*);

    TemporaryWindowMover* m_windowMover;
};

NPNInvalidateRectInvalidatesWindow::NPNInvalidateRectInvalidatesWindow(NPP npp, const string& identifier)
    : WindowedPluginTest(npp, identifier)
    , m_windowMover(0)
{
}

NPNInvalidateRectInvalidatesWindow::~NPNInvalidateRectInvalidatesWindow()
{
    delete m_windowMover;
}

NPError NPNInvalidateRectInvalidatesWindow::NPP_SetWindow(NPP instance, NPWindow* npWindow)
{
    NPError error = WindowedPluginTest::NPP_SetWindow(instance, npWindow);
    if (error != NPERR_NO_ERROR)
        return error;

    if (!window())
        return NPERR_NO_ERROR;

    // The test harness's window (the one that contains the WebView) is off-screen and hidden.
    // We need to move it on-screen and make it visible in order for the plugin's window to
    // accumulate an update region when the DWM is disabled.

    HWND testHarnessWindow = this->testHarnessWindow();
    if (!testHarnessWindow) {
        pluginLog(instance, "Failed to get test harness window");
        return NPERR_GENERIC_ERROR;
    }

    m_windowMover = new TemporaryWindowMover(testHarnessWindow);
    if (!m_windowMover->moveSucceeded()) {
        pluginLog(instance, "Failed to move test harness window on-screen");
        return NPERR_GENERIC_ERROR;
    }

    // Wait until we receive a WM_PAINT message to ensure that the window is on-screen before we do
    // the NPN_InvalidateRect test.
    waitUntilDone();
    return NPERR_NO_ERROR;
}

LRESULT NPNInvalidateRectInvalidatesWindow::wndProc(UINT message, WPARAM wParam, LPARAM lParam, bool& handled)
{
    if (message == WM_PAINT)
        onPaint();

    handled = false;
    return 0;
}

void NPNInvalidateRectInvalidatesWindow::onPaint()
{
    testInvalidateRect();
    notifyDone();
    delete m_windowMover;
    m_windowMover = 0;
}

void NPNInvalidateRectInvalidatesWindow::testInvalidateRect()
{
    RECT clientRect;
    if (!::GetClientRect(window(), &clientRect)) {
        pluginLog(m_npp, "::GetClientRect failed");
        return;
    }

    if (::IsRectEmpty(&clientRect)) {
        pluginLog(m_npp, "Plugin's HWND has not been sized when NPP_SetWindow is called");
        return;
    }

    // Clear the invalid region.
    if (!::ValidateRect(window(), 0)) {
        pluginLog(m_npp, "::ValidateRect failed");
        return;
    }

    // Invalidate our lower-right quadrant.
    NPRect rectToInvalidate;
    rectToInvalidate.left = (clientRect.right - clientRect.left) / 2;
    rectToInvalidate.top = (clientRect.bottom - clientRect.top) / 2;
    rectToInvalidate.right = clientRect.right;
    rectToInvalidate.bottom = clientRect.bottom;
    NPN_InvalidateRect(&rectToInvalidate);

    RECT invalidRect;
    if (!::GetUpdateRect(window(), &invalidRect, FALSE)) {
        pluginLog(m_npp, "::GetUpdateRect failed");
        return;
    }

    if (invalidRect.left != rectToInvalidate.left || invalidRect.top != rectToInvalidate.top || invalidRect.right != rectToInvalidate.right || invalidRect.bottom != rectToInvalidate.bottom) {
        pluginLog(m_npp, "Expected invalid rect {left=%u, top=%u, right=%u, bottom=%u}, but got {left=%d, top=%d, right=%d, bottom=%d}", rectToInvalidate.left, rectToInvalidate.top, rectToInvalidate.right, rectToInvalidate.bottom, invalidRect.left, invalidRect.top, invalidRect.right, invalidRect.bottom);
        return;
    }

    pluginLog(m_npp, "Plugin's HWND has been invalidated as expected");
}

static PluginTest::Register<NPNInvalidateRectInvalidatesWindow> registrar("npn-invalidate-rect-invalidates-window");
