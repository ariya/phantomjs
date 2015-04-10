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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#if USE(PLUGIN_HOST_PROCESS)

#ifndef ProxyRuntimeObject_h
#define ProxyRuntimeObject_h

#include <WebCore/JSDOMBinding.h>
#include <WebCore/runtime_object.h>

namespace WebKit {

class ProxyInstance;

class ProxyRuntimeObject : public JSC::Bindings::RuntimeObject {
public:
    typedef JSC::Bindings::RuntimeObject Base;

    static ProxyRuntimeObject* create(JSC::ExecState* exec, JSC::JSGlobalObject* globalObject, PassRefPtr<ProxyInstance> instance)
    {
        // FIXME: deprecatedGetDOMStructure uses the prototype off of the wrong global object.
        // exec->vm() is also likely wrong.
        JSC::Structure* structure = WebCore::deprecatedGetDOMStructure<ProxyRuntimeObject>(exec);
        ProxyRuntimeObject* object = new (JSC::allocateCell<ProxyRuntimeObject>(*exec->heap())) ProxyRuntimeObject(exec, globalObject, structure, instance);
        object->finishCreation(globalObject);
        return object;
    }

    ProxyInstance* getInternalProxyInstance() const;

    static JSC::Structure* createStructure(JSC::VM& vm, JSC::JSGlobalObject* globalObject, JSC::JSValue prototype)
    {
        return JSC::Structure::create(vm, globalObject, prototype, JSC::TypeInfo(JSC::ObjectType, StructureFlags), &s_info);
    }

    static const JSC::ClassInfo s_info;
private:
    ProxyRuntimeObject(JSC::ExecState*, JSC::JSGlobalObject*, JSC::Structure*, PassRefPtr<ProxyInstance>);
    void finishCreation(JSC::JSGlobalObject*);
};

}

#endif
#endif
