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

#ifndef NPJSObject_h
#define NPJSObject_h

#if ENABLE(NETSCAPE_PLUGIN_API)

#include <JavaScriptCore/Strong.h>
#include <WebCore/npruntime_internal.h>
#include <wtf/Noncopyable.h>

namespace JSC {

class VM;
class JSGlobalObject;
class JSObject;

}

namespace WebKit {

class NPRuntimeObjectMap;
    
// NPJSObject is an NPObject that wraps a JSObject.
class NPJSObject : public NPObject {
    WTF_MAKE_NONCOPYABLE(NPJSObject);
public:
    static NPJSObject* create(JSC::VM&, NPRuntimeObjectMap*, JSC::JSObject*);

    JSC::JSObject* jsObject() const { return m_jsObject.get(); }

    static bool isNPJSObject(NPObject*);

    static NPJSObject* toNPJSObject(NPObject* npObject)
    {
        ASSERT(isNPJSObject(npObject));
        return static_cast<NPJSObject*>(npObject);
    }

private:
    NPJSObject();
    ~NPJSObject();

    void initialize(JSC::VM&, NPRuntimeObjectMap*, JSC::JSObject*);

    bool hasMethod(NPIdentifier methodName);
    bool invoke(NPIdentifier methodName, const NPVariant* arguments, uint32_t argumentCount, NPVariant* result);
    bool invokeDefault(const NPVariant* arguments, uint32_t argumentCount, NPVariant* result);
    bool hasProperty(NPIdentifier propertyName);
    bool getProperty(NPIdentifier propertyName, NPVariant* result);
    bool setProperty(NPIdentifier propertyName, const NPVariant* value);
    bool removeProperty(NPIdentifier propertyName);
    bool enumerate(NPIdentifier** identifiers, uint32_t* identifierCount);
    bool construct(const NPVariant* arguments, uint32_t argumentCount, NPVariant* result);

    bool invoke(JSC::ExecState*, JSC::JSGlobalObject*, JSC::JSValue function, const NPVariant* arguments, uint32_t argumentCount, NPVariant* result);

    static NPClass* npClass();
    static NPObject* NP_Allocate(NPP, NPClass*);
    static void NP_Deallocate(NPObject*);
    static bool NP_HasMethod(NPObject*, NPIdentifier methodName);
    static bool NP_Invoke(NPObject*, NPIdentifier methodName, const NPVariant* arguments, uint32_t argumentCount, NPVariant* result);
    static bool NP_InvokeDefault(NPObject*, const NPVariant* arguments, uint32_t argumentCount, NPVariant* result);
    static bool NP_HasProperty(NPObject*, NPIdentifier propertyName);
    static bool NP_GetProperty(NPObject*, NPIdentifier propertyName, NPVariant* result);
    static bool NP_SetProperty(NPObject*, NPIdentifier propertyName, const NPVariant* value);
    static bool NP_RemoveProperty(NPObject*, NPIdentifier propertyName);
    static bool NP_Enumerate(NPObject*, NPIdentifier** identifiers, uint32_t* identifierCount);
    static bool NP_Construct(NPObject*, const NPVariant* arguments, uint32_t argumentCount, NPVariant* result);
    
    NPRuntimeObjectMap* m_objectMap;
    JSC::Strong<JSC::JSObject> m_jsObject;
};

} // namespace WebKit

#endif // ENABLE(NETSCAPE_PLUGIN_API)

#endif // NPJSObject_h
