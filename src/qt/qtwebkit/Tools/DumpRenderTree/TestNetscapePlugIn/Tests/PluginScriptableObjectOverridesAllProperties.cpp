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

#include <string.h>

using namespace std;

class PluginScriptableObjectOverridesAllProperties : public PluginTest {
public:
    PluginScriptableObjectOverridesAllProperties(NPP npp, const string& identifier)
        : PluginTest(npp, identifier)
    {
    }
    
private:
    class PluginObject : public Object<PluginObject> {
    public:
        PluginObject()
        {
        }

        ~PluginObject()
        {
        }

        bool hasProperty(NPIdentifier propertyName)
        {
            return true;
        }

        bool getProperty(NPIdentifier propertyName, NPVariant* result)
        {
            static const char* message = "My name is ";
            char* propertyString = pluginTest()->NPN_UTF8FromIdentifier(propertyName);
            
            int bufferLength = strlen(propertyString) + strlen(message) + 1;
            char* resultBuffer = static_cast<char*>(pluginTest()->NPN_MemAlloc(bufferLength));
            snprintf(resultBuffer, bufferLength, "%s%s", message, propertyString);
            
            STRINGZ_TO_NPVARIANT(resultBuffer, *result);

            return true;
        }
    };
    
    virtual NPError NPP_GetValue(NPPVariable variable, void *value)
    {
        if (variable != NPPVpluginScriptableNPObject)
            return NPERR_GENERIC_ERROR;
        
        *(NPObject**)value = PluginObject::create(this);
        
        return NPERR_NO_ERROR;
    }
    
};

static PluginTest::Register<PluginScriptableObjectOverridesAllProperties> pluginScriptableObjectOverridesAllProperties("plugin-scriptable-object-overrides-all-properties");
