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

#include "PluginTest.h"

#include <string.h>
#include <vector>

using namespace std;

// Test that evaluating JavaScript that removes the plug-in from the page
// destroys the plug-in asynchronously.
class CallJSThatDestroysPlugin : public PluginTest {
public:
    CallJSThatDestroysPlugin(NPP npp, const string& identifier)
        : PluginTest(npp, identifier)
        , m_isDestroyed(false)
        , m_window(0)
    {
    }
    ~CallJSThatDestroysPlugin();

    void runTest();

private:
    virtual NPError NPP_New(NPMIMEType, uint16_t, int16_t, char*[], char*[], NPSavedData*);
    virtual NPError NPP_Destroy(NPSavedData**);

    bool m_isDestroyed;
    HWND m_window;
};

static PluginTest::Register<CallJSThatDestroysPlugin> registrar("call-javascript-that-destroys-plugin");

static const LPCWSTR pluginTestProperty = L"PluginTestProperty";
static const UINT runTestWindowMessage = WM_USER + 1;
static const LPCWSTR messageWindowClassName = L"CallJSThatDestroysPluginMessageWindow";

CallJSThatDestroysPlugin::~CallJSThatDestroysPlugin()
{
    ::RemovePropW(m_window, pluginTestProperty);
    ::DestroyWindow(m_window);
}

void CallJSThatDestroysPlugin::runTest()
{
    executeScript("var plugin = document.getElementsByTagName('embed')[0]; plugin.parentElement.removeChild(plugin);");
    if (!m_isDestroyed)
        log("Success: executed script, and plug-in is not yet destroyed.");
}

static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg != runTestWindowMessage)
        return ::DefWindowProcW(hwnd, msg, wParam, lParam);

    CallJSThatDestroysPlugin* pluginTest = reinterpret_cast<CallJSThatDestroysPlugin*>(::GetPropW(hwnd, pluginTestProperty));
    pluginTest->runTest();
    return 0;
}

NPError CallJSThatDestroysPlugin::NPP_New(NPMIMEType, uint16_t, int16_t, char*[], char*[], NPSavedData*)
{
    assert(!m_window);

    waitUntilDone();

    WNDCLASSEXW wndClass = {0};
    wndClass.cbSize = sizeof(wndClass);
    wndClass.lpfnWndProc = wndProc;
    wndClass.hCursor = LoadCursor(0, IDC_ARROW);
    wndClass.hInstance = GetModuleHandle(0);
    wndClass.lpszClassName = messageWindowClassName;

    ::RegisterClassExW(&wndClass);
    m_window = ::CreateWindowExW(0, messageWindowClassName, 0, WS_CHILD, 0, 0, 0, 0, HWND_MESSAGE, 0, GetModuleHandle(0), 0);

    ::SetPropW(m_window, pluginTestProperty, reinterpret_cast<HANDLE>(this));
    ::PostMessageW(m_window, runTestWindowMessage, 0, 0);

    return NPERR_NO_ERROR;
}

NPError CallJSThatDestroysPlugin::NPP_Destroy(NPSavedData**)
{
    m_isDestroyed = true;
    log("Plug-in destroyed.");
    notifyDone();
    return NPERR_NO_ERROR;
}
