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

#include "WindowGeometryTest.h"

#include "PluginObject.h"

using namespace std;

// This plugin dumps its window rect (relative to the test harness window) to the console when it
// is instantiated.

class DumpWindowRect : public WindowGeometryTest {
public:
    DumpWindowRect(NPP, const string& identifier);

private:
    virtual void performWindowGeometryTest();
};

static PluginTest::Register<DumpWindowRect> registrar("dump-window-rect");

DumpWindowRect::DumpWindowRect(NPP npp, const string& identifier)
    : WindowGeometryTest(npp, identifier)
{
}


void DumpWindowRect::performWindowGeometryTest()
{
    RECT rect;
    if (!::GetClientRect(window(), &rect)) {
        log("::GetClientRect failed");
        return;
    }

    // MSDN says that calling ::MapWindowPoints this way will tell it to treat the points as a rect.
    if (!::MapWindowPoints(window(), testHarnessWindow(), reinterpret_cast<LPPOINT>(&rect), 2)) {
        log("::MapWindowPoints failed");
        return;
    }

    log("Plugin window rect: {left=%d, top=%d, right=%d, bottom=%d}", rect.left, rect.top, rect.right, rect.bottom);
}
