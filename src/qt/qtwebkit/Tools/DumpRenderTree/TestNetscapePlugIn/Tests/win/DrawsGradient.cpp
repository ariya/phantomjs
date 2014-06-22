/* Copyright (C) 2010 Apple Inc. All rights reserved.
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

// Just fills its window with some gradients

class DrawsGradient : public WindowedPluginTest {
public:
    DrawsGradient(NPP, const string& identifier);

private:
    void paint(HDC) const;

    LRESULT onPaint(WPARAM, LPARAM, bool& handled);
    LRESULT onPrintClient(WPARAM, LPARAM, bool& handled);

    virtual LRESULT wndProc(UINT message, WPARAM, LPARAM, bool& handled);
};

static PluginTest::Register<DrawsGradient> registrar("draws-gradient");

DrawsGradient::DrawsGradient(NPP npp, const string& identifier)
    : WindowedPluginTest(npp, identifier)
{
}

LRESULT DrawsGradient::wndProc(UINT message, WPARAM wParam, LPARAM lParam, bool& handled)
{
    LRESULT result = 0;

    switch (message) {
    case WM_PAINT:
        result = onPaint(wParam, lParam, handled);
        break;
    case WM_PRINTCLIENT:
        result = onPrintClient(wParam, lParam, handled);
        break;
    default:
        handled = false;
    }

    return result;
}

LRESULT DrawsGradient::onPaint(WPARAM, LPARAM, bool& handled)
{
    PAINTSTRUCT paintStruct;
    HDC dc = ::BeginPaint(window(), &paintStruct);
    if (!dc)
        return 0;

    paint(dc);
    ::EndPaint(window(), &paintStruct);

    handled = true;
    return 0;
}

LRESULT DrawsGradient::onPrintClient(WPARAM wParam, LPARAM, bool& handled)
{
    paint(reinterpret_cast<HDC>(wParam));

    handled = true;
    return 0;
}

void DrawsGradient::paint(HDC dc) const
{
    RECT clientRect;
    if (!::GetClientRect(window(), &clientRect))
        return;

    TRIVERTEX vertices[] = {
        // Upper-left: green
        { clientRect.left, clientRect.top, 0, 0xff00, 0, 0 },
        // Upper-right: blue
        { clientRect.right, clientRect.top, 0, 0, 0xff00, 0 },
        // Lower-left: yellow
        { clientRect.left, clientRect.bottom, 0xff00, 0xff00, 0, 0 },
        // Lower-right: red
        { clientRect.right, clientRect.bottom, 0xff00, 0, 0, 0 },
    };

    GRADIENT_TRIANGLE mesh[] = {
        // Upper-left triangle
        { 0, 1, 2 },
        // Lower-right triangle
        { 1, 2, 3 },
    };

    ::GradientFill(dc, vertices, _countof(vertices), mesh, _countof(mesh), GRADIENT_FILL_TRIANGLE);
}
