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

class NPRuntimeObjectFromDestroyedPlugin : public PluginTest {
public:
    NPRuntimeObjectFromDestroyedPlugin(NPP npp, const string& identifier)
        : PluginTest(npp, identifier)
    {
    }

private:
    // This is the test object.
    class TestObject : public Object<TestObject> { };

    // This is the scriptable object. It has a single "testObject" property and an "evaluate" function.
    class ScriptableObject : public Object<ScriptableObject> { 
    public:
        bool hasMethod(NPIdentifier methodName)
        {
            return identifierIs(methodName, "evaluate");
        }

        bool invoke(NPIdentifier methodName, const NPVariant* args, uint32_t argCount, NPVariant* result)
        {
            if (!identifierIs(methodName, "evaluate"))
                return false;

            if (argCount != 1 || !NPVARIANT_IS_STRING(args[0]))
                return false;

            return pluginTest()->executeScript(&NPVARIANT_TO_STRING(args[0]), result);
        }

        bool hasProperty(NPIdentifier propertyName)
        {
            return identifierIs(propertyName, "testObject");
        }

        bool getProperty(NPIdentifier propertyName, NPVariant* result)
        {
            if (propertyName != pluginTest()->NPN_GetStringIdentifier("testObject"))
                return false;

            NPObject* testObject = TestObject::create(pluginTest());
            OBJECT_TO_NPVARIANT(testObject, *result);
            return true;
        }
    };

    virtual NPError NPP_GetValue(NPPVariable variable, void *value)
    {
        if (variable != NPPVpluginScriptableNPObject)
            return NPERR_GENERIC_ERROR;
        
        *(NPObject**)value = ScriptableObject::create(this);
        
        return NPERR_NO_ERROR;
    }
};

static PluginTest::Register<NPRuntimeObjectFromDestroyedPlugin> npRuntimeObjectFromDestroyedPlugin("npruntime-object-from-destroyed-plugin");

