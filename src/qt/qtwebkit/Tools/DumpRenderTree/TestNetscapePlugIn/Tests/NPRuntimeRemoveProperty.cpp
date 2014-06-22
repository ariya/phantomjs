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

#include <string.h>

using namespace std;

class NPRuntimeRemoveProperty : public PluginTest {
public:
    NPRuntimeRemoveProperty(NPP npp, const string& identifier)
        : PluginTest(npp, identifier)
    {
    }
    
private:
    struct TestObject : Object<TestObject> {
    public:
        TestObject()
            : m_lastRemovedProperty(0)
        {
        }

        bool hasProperty(NPIdentifier propertyName)
        {
            if (identifierIs(propertyName, "lastRemovedProperty"))
                return true;
            
            return false;
        }

        bool getProperty(NPIdentifier propertyName, NPVariant* result)
        {
            assert(identifierIs(propertyName, "lastRemovedProperty"));

            if (!m_lastRemovedProperty)
                return false;

            if (pluginTest()->NPN_IdentifierIsString(m_lastRemovedProperty)) {
                char* lastRemovedPropertyName = pluginTest()->NPN_UTF8FromIdentifier(m_lastRemovedProperty);
                
                STRINGZ_TO_NPVARIANT(lastRemovedPropertyName, *result);
                return true;
            }

            int intIdentifier = pluginTest()->NPN_IntFromIdentifier(m_lastRemovedProperty);
            DOUBLE_TO_NPVARIANT(intIdentifier, *result);
            return true;
        }

        bool removeProperty(NPIdentifier propertyName)
        {
            m_lastRemovedProperty = propertyName;
            return true;
        }

    private:
        NPIdentifier m_lastRemovedProperty;
    };

    struct PluginObject : Object<PluginObject> {
    public:
        PluginObject()
            : m_testObject(0)
        {
        }

        ~PluginObject()
        {
            if (m_testObject)
                pluginTest()->NPN_ReleaseObject(m_testObject);
        }

        bool hasMethod(NPIdentifier methodName)
        {
            if (identifierIs(methodName, "testRemoveProperty"))
                return true;

            return false;
        }

        bool invoke(NPIdentifier methodName, const NPVariant* arguments, uint32_t argumentCount, NPVariant* result)
        {
            assert(identifierIs(methodName, "testRemoveProperty"));

            if (argumentCount != 2)
                return false;

            if (!NPVARIANT_IS_OBJECT(arguments[0]))
                return false;
            
            if (!NPVARIANT_IS_STRING(arguments[1]) && !NPVARIANT_IS_DOUBLE(arguments[1]))
                return false;
            
            NPIdentifier propertyName;
            if (NPVARIANT_IS_STRING(arguments[1])) {
                string propertyNameString(arguments[1].value.stringValue.UTF8Characters,
                                          arguments[1].value.stringValue.UTF8Length);
            
                propertyName = pluginTest()->NPN_GetStringIdentifier(propertyNameString.c_str());
            } else {
                int32_t number = static_cast<int32_t>(arguments[1].value.doubleValue);
                propertyName = pluginTest()->NPN_GetIntIdentifier(number);
            }
            
            pluginTest()->NPN_RemoveProperty(NPVARIANT_TO_OBJECT(arguments[0]), propertyName);

            VOID_TO_NPVARIANT(*result);
            return true;
        }

        bool hasProperty(NPIdentifier propertyName)
        {
            if (identifierIs(propertyName, "testObject"))
                return true;

            return false;
        }

        bool getProperty(NPIdentifier propertyName, NPVariant* result)
        {
            assert(identifierIs(propertyName, "testObject"));

            if (!m_testObject)
                m_testObject = TestObject::create(pluginTest());

            OBJECT_TO_NPVARIANT(pluginTest()->NPN_RetainObject(m_testObject), *result);
            return true;
        }

    private:
        NPObject* m_testObject;
    };
    
    virtual NPError NPP_GetValue(NPPVariable variable, void *value)
    {
        if (variable != NPPVpluginScriptableNPObject)
            return NPERR_GENERIC_ERROR;
        
        *(NPObject**)value = PluginObject::create(this);
        
        return NPERR_NO_ERROR;
    }
    
};

static PluginTest::Register<NPRuntimeRemoveProperty> npRuntimeRemoveProperty("npruntime-remove-property");
