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

using namespace std;

static const wchar_t instancePointerProperty[] = L"org.webkit.TestNetscapePlugin.WindowedPluginTest.InstancePointer";

WindowedPluginTest::WindowedPluginTest(NPP npp, const string& identifier)
    : PluginTest(npp, identifier)
    , m_window(0)
    , m_originalWndProc(0)
{
}

HWND WindowedPluginTest::testHarnessWindow() const
{
    return ::GetAncestor(window(), GA_ROOT);
}

NPError WindowedPluginTest::NPP_SetWindow(NPP instance, NPWindow* window)
{
    HWND newWindow = reinterpret_cast<HWND>(window->window);
    if (newWindow == m_window)
        return NPERR_NO_ERROR;

    if (m_window) {
        ::RemovePropW(m_window, instancePointerProperty);
        ::SetWindowLongPtr(m_window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(m_originalWndProc));
        m_originalWndProc = 0;
    }

    m_window = newWindow;
    if (!m_window)
        return NPERR_NO_ERROR;

    m_originalWndProc = reinterpret_cast<WNDPROC>(::SetWindowLongPtrW(m_window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(staticWndProc)));
    ::SetPropW(m_window, instancePointerProperty, this);

    return NPERR_NO_ERROR;
}

LRESULT WindowedPluginTest::staticWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WindowedPluginTest* instance = reinterpret_cast<WindowedPluginTest*>(::GetPropW(hwnd, instancePointerProperty));

    bool handled = false;
    LRESULT result = instance->wndProc(message, wParam, lParam, handled);
    if (handled)
        return result;

    return ::CallWindowProcW(instance->m_originalWndProc, hwnd, message, wParam, lParam);
}
