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

using namespace std;

// From NPP_New, call NPN_GetURL to evaluate JavaScript that destroys the plug-in.

class GetURLWithJavaScriptURLDestroyingPlugin : public PluginTest {
public:
    GetURLWithJavaScriptURLDestroyingPlugin(NPP npp, const string& identifier)
        : PluginTest(npp, identifier)
    {
    }

private:

    virtual NPError NPP_New(NPMIMEType pluginType, uint16_t mode, int16_t argc, char *argn[], char *argv[], NPSavedData *saved)
    {
        NPN_GetURL("javascript:removePlugin()", 0);
        return NPERR_NO_ERROR;
    }        
};

static PluginTest::Register<GetURLWithJavaScriptURLDestroyingPlugin> getURLWithJavaScriptURLDestroyingPlugin("get-url-with-javascript-url-destroying-plugin");
