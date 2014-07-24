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

#ifndef NPObjectProxy_h
#define NPObjectProxy_h

#if ENABLE(PLUGIN_PROCESS)

#include <WebCore/npruntime_internal.h>
#include <wtf/Noncopyable.h>

namespace WebKit {

class NPRemoteObjectMap;
class Plugin;

class NPObjectProxy : public NPObject {
    WTF_MAKE_NONCOPYABLE(NPObjectProxy);

public:
    static NPObjectProxy* create(NPRemoteObjectMap*, Plugin*, uint64_t npObjectID);

    static bool isNPObjectProxy(NPObject*);
    
    static NPObjectProxy* toNPObjectProxy(NPObject* npObject)
    {
        ASSERT(isNPObjectProxy(npObject));
        return static_cast<NPObjectProxy*>(npObject);
    }

    Plugin* plugin() const { return m_plugin; }
    uint64_t npObjectID() const { return m_npObjectID; }

    void invalidate();

private:
    NPObjectProxy();
    ~NPObjectProxy();

    void initialize(NPRemoteObjectMap*, Plugin*, uint64_t npObjectID);

    bool hasMethod(NPIdentifier methodName);
    bool invoke(NPIdentifier methodName, const NPVariant* arguments, uint32_t argumentCount, NPVariant* result);
    bool invokeDefault(const NPVariant* arguments, uint32_t argumentCount, NPVariant* result);
    bool hasProperty(NPIdentifier propertyName);
    bool getProperty(NPIdentifier propertyName, NPVariant* result);
    bool setProperty(NPIdentifier propertyName, const NPVariant* value);
    bool removeProperty(NPIdentifier propertyName);
    bool enumerate(NPIdentifier** identifiers, uint32_t* identifierCount);
    bool construct(const NPVariant* arguments, uint32_t argumentCount, NPVariant* result);

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

    NPRemoteObjectMap* m_npRemoteObjectMap;
    Plugin* m_plugin;
    uint64_t m_npObjectID;
};
    
} // namespace WebKit

#endif // ENABLE(PLUGIN_PROCESS)

#endif // NPObjectProxy_h
