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

#include "config.h"
#include "ActivateFonts.h"

#include <string>
#include <wtf/Vector.h>

static LPCWSTR fontsEnvironmentVariable = L"WEBKIT_TESTFONTS";

namespace WTR {

using namespace std;

static const wstring& fontsPath()
{
    static wstring path;
    static bool initialized;

    if (initialized)
        return path;
    initialized = true;

    DWORD size = ::GetEnvironmentVariableW(fontsEnvironmentVariable, 0, 0);
    Vector<WCHAR> buffer(size);
    if (!::GetEnvironmentVariableW(fontsEnvironmentVariable, buffer.data(), buffer.size()))
        return path;

    path = buffer.data();
    if (path[path.length() - 1] != '\\')
        path.append(L"\\");

    return path;
}


void activateFonts()
{
    static LPCWSTR fontsToInstall[] = {
        TEXT("AHEM____.ttf"),
        TEXT("Apple Chancery.ttf"),
        TEXT("Courier Bold.ttf"),
        TEXT("Courier.ttf"),
        TEXT("Helvetica Bold Oblique.ttf"),
        TEXT("Helvetica Bold.ttf"),
        TEXT("Helvetica Oblique.ttf"),
        TEXT("Helvetica.ttf"),
        TEXT("Helvetica Neue Bold Italic.ttf"),
        TEXT("Helvetica Neue Bold.ttf"),
        TEXT("Helvetica Neue Condensed Black.ttf"),
        TEXT("Helvetica Neue Condensed Bold.ttf"),
        TEXT("Helvetica Neue Italic.ttf"),
        TEXT("Helvetica Neue Light Italic.ttf"),
        TEXT("Helvetica Neue Light.ttf"),
        TEXT("Helvetica Neue UltraLight Italic.ttf"),
        TEXT("Helvetica Neue UltraLight.ttf"),
        TEXT("Helvetica Neue.ttf"),
        TEXT("Lucida Grande.ttf"),
        TEXT("Lucida Grande Bold.ttf"),
        TEXT("Monaco.ttf"),
        TEXT("Papyrus.ttf"),
        TEXT("Times Bold Italic.ttf"),
        TEXT("Times Bold.ttf"),
        TEXT("Times Italic.ttf"),
        TEXT("Times Roman.ttf"),
        TEXT("WebKit Layout Tests 2.ttf"),
        TEXT("WebKit Layout Tests.ttf"),
        TEXT("WebKitWeightWatcher100.ttf"),
        TEXT("WebKitWeightWatcher200.ttf"),
        TEXT("WebKitWeightWatcher300.ttf"),
        TEXT("WebKitWeightWatcher400.ttf"),
        TEXT("WebKitWeightWatcher500.ttf"),
        TEXT("WebKitWeightWatcher600.ttf"),
        TEXT("WebKitWeightWatcher700.ttf"),
        TEXT("WebKitWeightWatcher800.ttf"),
        TEXT("WebKitWeightWatcher900.ttf")
    };

    wstring resourcesPath = fontsPath();

    for (unsigned i = 0; i < ARRAYSIZE(fontsToInstall); ++i)
        ::AddFontResourceExW(wstring(resourcesPath + fontsToInstall[i]).c_str(), FR_PRIVATE, 0);
}

}
