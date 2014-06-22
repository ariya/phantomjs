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

static bool wasShutdownCalled = false;

class NPDeallocateCalledBeforeNPShutdown : public PluginTest {
public:
    NPDeallocateCalledBeforeNPShutdown(NPP npp, const string& identifier)
        : PluginTest(npp, identifier)
    {
    }

private:
    // This is the test object.
    class TestObject : public Object<TestObject> { 
    public:
        ~TestObject() 
        {
            if (wasShutdownCalled)
                indicateTestFailure();
        }
    };

    // This is the scriptable object. It has a single "testObject" property.
    class ScriptableObject : public Object<ScriptableObject> { 
    public:
        bool hasProperty(NPIdentifier propertyName)
        {
            return propertyName == pluginTest()->NPN_GetStringIdentifier("testObject");
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

    virtual NPError NPP_New(NPMIMEType pluginType, uint16_t mode, int16_t argc, char *argn[], char *argv[], NPSavedData *saved)
    {
        registerNPShutdownFunction(shutdown);

        return NPERR_NO_ERROR;
    }

    virtual NPError NPP_GetValue(NPPVariable variable, void *value)
    {
        if (variable != NPPVpluginScriptableNPObject)
            return NPERR_GENERIC_ERROR;
        
        *(NPObject**)value = ScriptableObject::create(this);
        
        return NPERR_NO_ERROR;
    }

    static void shutdown()
    {
        wasShutdownCalled = true;
    }

};

static PluginTest::Register<NPDeallocateCalledBeforeNPShutdown> npRuntimeObjectFromDestroyedPlugin("np-deallocate-called-before-np-shutdown");

