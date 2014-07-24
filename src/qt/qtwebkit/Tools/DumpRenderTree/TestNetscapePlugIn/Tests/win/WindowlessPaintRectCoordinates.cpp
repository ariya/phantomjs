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

#include "PluginObject.h"

using namespace std;

// The rect passed in the WM_PAINT event for a windowless plugin should be relative to the page's
// HWND and clipped to the plugin's bounds.

class WindowlessPaintRectCoordinates : public PluginTest {
public:
    WindowlessPaintRectCoordinates(NPP, const string& identifier);

private:
    virtual NPError NPP_New(NPMIMEType, uint16_t mode, int16_t argc, char* argn[], char* argv[], NPSavedData*);
    virtual int16_t NPP_HandleEvent(void* event);
};

static PluginTest::Register<WindowlessPaintRectCoordinates> registrar("windowless-paint-rect-coordinates");

WindowlessPaintRectCoordinates::WindowlessPaintRectCoordinates(NPP npp, const string& identifier)
    : PluginTest(npp, identifier)
{
}

NPError WindowlessPaintRectCoordinates::NPP_New(NPMIMEType, uint16_t, int16_t, char*[], char*[], NPSavedData*)
{
    browser->setvalue(m_npp, NPPVpluginWindowBool, 0);
    return NPERR_NO_ERROR;
}

int16_t WindowlessPaintRectCoordinates::NPP_HandleEvent(void* typelessEvent)
{
    NPEvent* event = static_cast<NPEvent*>(typelessEvent);
    if (!event) {
        pluginLog(m_npp, "NPP_HandleEvent was passed a null event pointer");
        return 0;
    }

    if (event->event != WM_PAINT)
        return 0;

    RECT* paintRect = reinterpret_cast<RECT*>(event->lParam);
    if (!paintRect) {
        pluginLog(m_npp, "A null paint rect was passed in the WM_PAINT event");
        return 1;
    }

    // Keep these values in sync with windowless-paint-rect-coordinates.html.
    RECT expectedRect = { 100, 100, 200, 200 };

    if (::EqualRect(paintRect, &expectedRect))
        pluginLog(m_npp, "Success");
    else
        pluginLog(m_npp, "Expected paint rect {left=%d, top=%d, right=%d, bottom=%d}, but got {left=%d, top=%d, right=%d, bottom=%d}", expectedRect.left, expectedRect.top, expectedRect.right, expectedRect.bottom, paintRect->left, paintRect->top, paintRect->right, paintRect->bottom);

    return 1;
}
