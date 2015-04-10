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

#include "PluginTest.h"

#include "PluginObject.h"

using namespace std;

// Plugin's HWND should be sized/positioned before NPP_SetWindow is called.

class WindowGeometryInitializedBeforeSetWindow : public PluginTest {
public:
    WindowGeometryInitializedBeforeSetWindow(NPP npp, const string& identifier)
        : PluginTest(npp, identifier)
        , m_didReceiveInitialSetWindowCall(false)
    {
    }

private:
    virtual NPError NPP_SetWindow(NPP instance, NPWindow* window)
    {
        if (m_didReceiveInitialSetWindowCall)
            return NPERR_NO_ERROR;
        m_didReceiveInitialSetWindowCall = true;

        if (window->type != NPWindowTypeWindow) {
            pluginLog(instance, "window->type should be NPWindowTypeWindow but was %d", window->type);
            return NPERR_GENERIC_ERROR;
        }

        HWND hwnd = reinterpret_cast<HWND>(window->window);
        RECT rect;
        if (!::GetClientRect(hwnd, &rect)) {
            pluginLog(instance, "::GetClientRect failed");
            return NPERR_GENERIC_ERROR;
        }

        if (::IsRectEmpty(&rect)) {
            pluginLog(instance, "Plugin's HWND has not been sized when NPP_SetWindow is called");
            return NPERR_GENERIC_ERROR;
        }

        if ((rect.right - rect.left) != window->width || (rect.bottom - rect.top) != window->height) {
            pluginLog(instance, "Size of HWND's rect and size of NPWindow's rect are not equal");
            return NPERR_GENERIC_ERROR;
        }

        pluginLog(instance, "Plugin's HWND has been sized before NPP_SetWindow was called");
        return NPERR_NO_ERROR;
    }

    bool m_didReceiveInitialSetWindowCall;
};

static PluginTest::Register<WindowGeometryInitializedBeforeSetWindow> windowGeometryInitializedBeforeSetWindow("window-geometry-initialized-before-set-window");
