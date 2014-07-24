/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

// Executing JS within NPP_New when initializing asynchronously should not be able to deadlock with the WebProcess

class InvokeDestroysPluginWithinNPP_New : public PluginTest {
public:
    InvokeDestroysPluginWithinNPP_New(NPP, const string& identifier);

private:
    virtual NPError NPP_New(NPMIMEType pluginType, uint16_t mode, int16_t argc, char* argn[], char* argv[], NPSavedData *);

};

InvokeDestroysPluginWithinNPP_New::InvokeDestroysPluginWithinNPP_New(NPP npp, const string& identifier)
    : PluginTest(npp, identifier)
{
}

NPError InvokeDestroysPluginWithinNPP_New::NPP_New(NPMIMEType pluginType, uint16_t mode, int16_t argc, char* argn[], char* argv[], NPSavedData *saved)
{
    // Give the WebProcess enough time to be deadlocked waiting for the PluginProcess if things aren't working correctly.
    usleep(15000);
    
    NPObject* windowObject = 0;
    if (NPN_GetValue(NPNVWindowNPObject, &windowObject) != NPERR_NO_ERROR)
        return NPERR_GENERIC_ERROR;
    
    if (!windowObject)
        return NPERR_GENERIC_ERROR;
    
    NPVariant result;
    if (!NPN_Invoke(windowObject, NPN_GetStringIdentifier("removePluginElement"), 0, 0, &result))
        return NPERR_GENERIC_ERROR;

    return NPERR_NO_ERROR;
}

static PluginTest::Register<InvokeDestroysPluginWithinNPP_New> registrar("invoke-destroys-plugin-within-npp-new");
