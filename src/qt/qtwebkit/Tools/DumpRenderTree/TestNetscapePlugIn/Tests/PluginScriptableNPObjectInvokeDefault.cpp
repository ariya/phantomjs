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

// A test where the plug-ins scriptable object either has or doesn't have an invokeDefault function.
class PluginScriptableNPObjectInvokeDefault : public PluginTest {
public:
    PluginScriptableNPObjectInvokeDefault(NPP npp, const string& identifier)
        : PluginTest(npp, identifier)
    {
    }

private:
    struct NPObjectWithoutInvokeDefault : Object<NPObjectWithoutInvokeDefault> { };

    struct NPObjectWithInvokeDefault : Object<NPObjectWithInvokeDefault> { 
    public:
        bool invokeDefault(const NPVariant*, uint32_t, NPVariant* result)
        {
            INT32_TO_NPVARIANT(1, *result);
            return true;
        }
    };

    virtual NPError NPP_GetValue(NPPVariable variable, void *value)
    {
        if (variable != NPPVpluginScriptableNPObject)
            return NPERR_GENERIC_ERROR;

        NPObject* object;
        if (identifier() == "plugin-scriptable-npobject-invoke-default")
            object = NPObjectWithInvokeDefault::create(this);
        else
            object = NPObjectWithoutInvokeDefault::create(this);
        
        *(NPObject**)value = object;
        
        return NPERR_NO_ERROR;
    }
};

static PluginTest::Register<PluginScriptableNPObjectInvokeDefault> pluginScriptableNPObjectInvokeDefault("plugin-scriptable-npobject-invoke-default");
static PluginTest::Register<PluginScriptableNPObjectInvokeDefault> pluginScriptableNPObjectNoInvokeDefault("plugin-scriptable-npobject-no-invoke-default");
