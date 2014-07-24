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

#ifndef PluginTest_h
#define PluginTest_h

#include <WebKit/npfunctions.h>
#include <assert.h>
#include <map>
#include <string>

// Helper classes for implementing has_member
typedef char (&no_tag)[1];
typedef char (&yes_tag)[2];

#define DEFINE_HAS_MEMBER_CHECK(member, returnType, argumentTypes) \
template<typename T, returnType (T::*member) argumentTypes> struct pmf_##member##_helper {}; \
template<typename T> no_tag has_member_##member##_helper(...); \
template<typename T> yes_tag has_member_##member##_helper(pmf_##member##_helper<T, &T::member >*); \
template<typename T> struct has_member_##member { \
static const bool value = sizeof(has_member_##member##_helper<T>(0)) == sizeof(yes_tag); \
};

DEFINE_HAS_MEMBER_CHECK(hasMethod, bool, (NPIdentifier methodName));
DEFINE_HAS_MEMBER_CHECK(invoke, bool, (NPIdentifier methodName, const NPVariant*, uint32_t, NPVariant* result));
DEFINE_HAS_MEMBER_CHECK(invokeDefault, bool, (const NPVariant*, uint32_t, NPVariant* result));
DEFINE_HAS_MEMBER_CHECK(hasProperty, bool, (NPIdentifier propertyName));
DEFINE_HAS_MEMBER_CHECK(getProperty, bool, (NPIdentifier propertyName, NPVariant* result));
DEFINE_HAS_MEMBER_CHECK(removeProperty, bool, (NPIdentifier propertyName));

class PluginTest {
public:
    static PluginTest* create(NPP, const std::string& identifier);
    virtual ~PluginTest();

    static void NP_Shutdown();

    // NPP functions.
    virtual NPError NPP_New(NPMIMEType pluginType, uint16_t mode, int16_t argc, char *argn[], char *argv[], NPSavedData *saved);
    virtual NPError NPP_Destroy(NPSavedData**);
    virtual NPError NPP_SetWindow(NPWindow*);
    virtual NPError NPP_NewStream(NPMIMEType, NPStream*, NPBool seekable, uint16_t* stype);
    virtual NPError NPP_DestroyStream(NPStream*, NPReason);
    virtual int32_t NPP_WriteReady(NPStream*);
    virtual int32_t NPP_Write(NPStream*, int32_t offset, int32_t len, void* buffer);
    
    virtual int16_t NPP_HandleEvent(void* event);
    virtual bool NPP_URLNotify(const char* url, NPReason, void* notifyData);
    virtual NPError NPP_GetValue(NPPVariable, void* value);
    virtual NPError NPP_SetValue(NPNVariable, void *value);

    // NPN functions.
    NPError NPN_GetURL(const char* url, const char* target);
    NPError NPN_GetURLNotify(const char* url, const char* target, void* notifyData);
    NPError NPN_GetValue(NPNVariable, void* value);
    void NPN_InvalidateRect(NPRect* invalidRect);
    bool NPN_Invoke(NPObject *, NPIdentifier methodName, const NPVariant *args, uint32_t argCount, NPVariant *result);
    void* NPN_MemAlloc(uint32_t size);

    // NPRuntime NPN functions.
    NPIdentifier NPN_GetStringIdentifier(const NPUTF8* name);
    NPIdentifier NPN_GetIntIdentifier(int32_t intid);
    bool NPN_IdentifierIsString(NPIdentifier);
    NPUTF8* NPN_UTF8FromIdentifier(NPIdentifier);
    int32_t NPN_IntFromIdentifier(NPIdentifier);

    NPObject* NPN_CreateObject(NPClass*);
    NPObject* NPN_RetainObject(NPObject*);
    void NPN_ReleaseObject(NPObject*);
    bool NPN_RemoveProperty(NPObject*, NPIdentifier propertyName);
    void NPN_ReleaseVariantValue(NPVariant*);

#ifdef XP_MACOSX
    bool NPN_ConvertPoint(double sourceX, double sourceY, NPCoordinateSpace sourceSpace, double *destX, double *destY, NPCoordinateSpace destSpace);
#endif

    bool executeScript(const NPString*, NPVariant* result);
    void executeScript(const char*);
    void log(const char* format, ...);

    void registerNPShutdownFunction(void (*)());

    static void indicateTestFailure();

    template<typename TestClassTy> class Register {
    public:
        Register(const std::string& identifier)
        {
            registerCreateTestFunction(identifier, Register::create);
        }
    
    private:
        static PluginTest* create(NPP npp, const std::string& identifier) 
        {
            return new TestClassTy(npp, identifier);
        }
    };

protected:
    PluginTest(NPP npp, const std::string& identifier);

    // FIXME: A plug-in test shouldn't need to know about it's NPP. Make this private.
    NPP m_npp;

    const std::string& identifier() const { return m_identifier; }

    static NPNetscapeFuncs* netscapeFuncs();

    void waitUntilDone();
    void notifyDone();

    // NPObject helper template.
    template<typename T> struct Object : NPObject {
    public:
        static NPObject* create(PluginTest* pluginTest)
        {
            Object* object = static_cast<Object*>(pluginTest->NPN_CreateObject(npClass()));

            object->m_pluginTest = pluginTest;
            return object;
        }
    
        // These should never be called.
        bool hasMethod(NPIdentifier methodName)
        {
            assert(false);
            return false;
        }

        bool invoke(NPIdentifier methodName, const NPVariant*, uint32_t, NPVariant* result)
        {
            assert(false);
            return false;
        }
        
        bool invokeDefault(const NPVariant*, uint32_t, NPVariant* result)
        {
            assert(false);
            return false;
        }

        bool hasProperty(NPIdentifier propertyName)
        {
            assert(false);
            return false;
        }

        bool getProperty(NPIdentifier propertyName, NPVariant* result)
        {
            assert(false);
            return false;
        }

        bool removeProperty(NPIdentifier propertyName)
        {
            assert(false);
            return false;
        }

        // Helper functions.
        bool identifierIs(NPIdentifier identifier, const char* value)
        {
            return pluginTest()->NPN_GetStringIdentifier(value) == identifier;
        }

    protected:
        Object()
            : m_pluginTest(0)
        {
        }
        
        virtual ~Object() 
        { 
        }

        PluginTest* pluginTest() const { return m_pluginTest; }

    private:
        static NPObject* NP_Allocate(NPP npp, NPClass* aClass)
        {
            return new T;
        }

        static void NP_Deallocate(NPObject* npObject)
        {
            delete static_cast<T*>(npObject);
        }

        static bool NP_HasMethod(NPObject* npObject, NPIdentifier methodName)
        {
            return static_cast<T*>(npObject)->hasMethod(methodName);
        }

        static bool NP_Invoke(NPObject* npObject, NPIdentifier methodName, const NPVariant* arguments, uint32_t argumentCount, NPVariant* result)
        {
            return static_cast<T*>(npObject)->invoke(methodName, arguments, argumentCount, result);
        }

        static bool NP_InvokeDefault(NPObject* npObject, const NPVariant* arguments, uint32_t argumentCount, NPVariant* result)
        {
            return static_cast<T*>(npObject)->invokeDefault(arguments, argumentCount, result);
        }

        static bool NP_HasProperty(NPObject* npObject, NPIdentifier propertyName)
        {
            return static_cast<T*>(npObject)->hasProperty(propertyName);
        }

        static bool NP_GetProperty(NPObject* npObject, NPIdentifier propertyName, NPVariant* result)
        {
            return static_cast<T*>(npObject)->getProperty(propertyName, result);
        }

        static bool NP_RemoveProperty(NPObject* npObject, NPIdentifier propertyName)
        {
            return static_cast<T*>(npObject)->removeProperty(propertyName);
        }

        static NPClass* npClass()
        {
            static NPClass npClass = {
                NP_CLASS_STRUCT_VERSION, 
                NP_Allocate,
                NP_Deallocate,
                0, // NPClass::invalidate
                has_member_hasMethod<T>::value ? NP_HasMethod : 0,
                has_member_invoke<T>::value ? NP_Invoke : 0,
                has_member_invokeDefault<T>::value ? NP_InvokeDefault : 0,
                has_member_hasProperty<T>::value ? NP_HasProperty : 0,
                has_member_getProperty<T>::value ? NP_GetProperty : 0,
                0, // NPClass::setProperty
                has_member_removeProperty<T>::value ? NP_RemoveProperty : 0,
                0, // NPClass::enumerate
                0  // NPClass::construct
            };
            
            return &npClass;
        };

        PluginTest* m_pluginTest;
    };
    
private:
    typedef PluginTest* (*CreateTestFunction)(NPP, const std::string&);
    
    static void registerCreateTestFunction(const std::string&, CreateTestFunction);
    static std::map<std::string, CreateTestFunction>& createTestFunctions();
    
    std::string m_identifier;
};

#endif // PluginTest_h
