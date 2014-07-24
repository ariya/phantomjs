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

using namespace std;

// From NPP_New, call NPN_GetURLNotify with a URL that fails to load (NPP_NewStream won't be called).
// The plug-in should still get a NPP_URLNotify indicating that the load failed.
static const char *urlThatFailsToLoad = "foo://bar/";

class GetURLNotifyWithURLThatFailsToLoad : public PluginTest {
public:
    GetURLNotifyWithURLThatFailsToLoad(NPP npp, const string& identifier)
        : PluginTest(npp, identifier)
    {
    }

private:

    virtual NPError NPP_New(NPMIMEType pluginType, uint16_t mode, int16_t argc, char *argn[], char *argv[], NPSavedData *saved)
    {
        NPN_GetURLNotify(urlThatFailsToLoad, 0, reinterpret_cast<void*>(0x12345678));
        return NPERR_NO_ERROR;
    }

    bool NPP_URLNotify(const char* url, NPReason reason, void* notifyData)
    {
        bool didFail = false;

        if (strcmp(url, urlThatFailsToLoad))
            didFail = true;

        if (reason != NPRES_NETWORK_ERR)
            didFail = true;

        if (notifyData != reinterpret_cast<void*>(0x12345678))
            didFail = true;

        if (!didFail)
            executeScript("testSucceeded()");
        else
            executeScript("notifyDone()");
        return true;
    }
};

static PluginTest::Register<GetURLNotifyWithURLThatFailsToLoad> getURLWithJavaScriptURLDestroyingPlugin("get-url-notify-with-url-that-fails-to-load");
