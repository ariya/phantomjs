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

using namespace std;

// Test that NPN_ConvertPoint converts correctly.
class ConvertPoint : public PluginTest {
public:
    ConvertPoint(NPP npp, const string& identifier)
        : PluginTest(npp, identifier)
    {
    }

    bool testConvert(double x, double y, NPCoordinateSpace sourceSpace, NPCoordinateSpace destSpace)
    {
        // First convert from src to dest.
        double destX, destY;
        if (!NPN_ConvertPoint(x, y, sourceSpace, &destX, &destY, destSpace))
            return false;

        // Then convert back to src
        double srcX, srcY;
        if (!NPN_ConvertPoint(destX, destY, destSpace, &srcX, &srcY, sourceSpace))
            return false;


        // Then compare.
        if (srcX != x || srcY != y)
            return false;

        return true;
    }

    bool testConvert()
    {
        static const NPCoordinateSpace spaces[] = { NPCoordinateSpacePlugin, NPCoordinateSpaceWindow, NPCoordinateSpaceFlippedWindow, NPCoordinateSpaceScreen, NPCoordinateSpaceFlippedScreen };

        static const size_t numSpaces = sizeof(spaces) / sizeof(spaces[0]);
        for (size_t i = 0; i < numSpaces; ++i) {
            for (size_t j = 0; j < numSpaces; ++j) {
                if (!testConvert(1234, 5678, spaces[i], spaces[j]))
                    return false;
            }
        }
        return true;
    }
private:
    NPError NPP_New(NPMIMEType pluginType, uint16_t mode, int16_t argc, char *argn[], char *argv[], NPSavedData *saved)
    {
        if (testConvert())
            executeScript("document.getElementById('result').innerHTML = 'SUCCESS!'");

        executeScript("setTimeout(function() { testRunner.notifyDone() }, 0)");
        return NPERR_NO_ERROR;
    }
};

static PluginTest::Register<ConvertPoint> convertPoint("convert-point");
