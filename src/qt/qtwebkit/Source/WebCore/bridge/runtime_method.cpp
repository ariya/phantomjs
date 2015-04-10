/*
 * Copyright (C) 2003, 2008 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
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
 
#include "config.h"
#include "runtime_method.h"

#include "JSDOMBinding.h"
#include "JSHTMLElement.h"
#include "JSPluginElementFunctions.h"
#include "runtime_object.h"
#include <runtime/Error.h>
#include <runtime/FunctionPrototype.h>

using namespace WebCore;

namespace JSC {

using namespace Bindings;

const ClassInfo RuntimeMethod::s_info = { "RuntimeMethod", &InternalFunction::s_info, 0, 0, CREATE_METHOD_TABLE(RuntimeMethod) };

RuntimeMethod::RuntimeMethod(JSGlobalObject* globalObject, Structure* structure, Method* method)
    // Callers will need to pass in the right global object corresponding to this native object "method".
    : InternalFunction(globalObject, structure)
    , m_method(method)
{
}

void RuntimeMethod::finishCreation(VM& vm, const String& ident)
{
    Base::finishCreation(vm, ident);
    ASSERT(inherits(&s_info));
}

JSValue RuntimeMethod::lengthGetter(ExecState*, JSValue slotBase, PropertyName)
{
    RuntimeMethod* thisObj = static_cast<RuntimeMethod*>(asObject(slotBase));

    return jsNumber(thisObj->m_method->numParameters());
}

bool RuntimeMethod::getOwnPropertySlot(JSCell* cell, ExecState* exec, PropertyName propertyName, PropertySlot &slot)
{
    RuntimeMethod* thisObject = jsCast<RuntimeMethod*>(cell);
    if (propertyName == exec->propertyNames().length) {
        slot.setCacheableCustom(thisObject, thisObject->lengthGetter);
        return true;
    }
    
    return InternalFunction::getOwnPropertySlot(thisObject, exec, propertyName, slot);
}

bool RuntimeMethod::getOwnPropertyDescriptor(JSObject* object, ExecState* exec, PropertyName propertyName, PropertyDescriptor &descriptor)
{
    RuntimeMethod* thisObject = jsCast<RuntimeMethod*>(object);
    if (propertyName == exec->propertyNames().length) {
        PropertySlot slot;
        slot.setCustom(thisObject, lengthGetter);
        descriptor.setDescriptor(slot.getValue(exec, propertyName), ReadOnly | DontDelete | DontEnum);
        return true;
    }
    
    return InternalFunction::getOwnPropertyDescriptor(thisObject, exec, propertyName, descriptor);
}

static EncodedJSValue JSC_HOST_CALL callRuntimeMethod(ExecState* exec)
{
    RuntimeMethod* method = static_cast<RuntimeMethod*>(exec->callee());

    if (!method->method())
        return JSValue::encode(jsUndefined());

    RefPtr<Instance> instance;

    JSValue thisValue = exec->hostThisValue();
    if (thisValue.inherits(&RuntimeObject::s_info)) {
        RuntimeObject* runtimeObject = static_cast<RuntimeObject*>(asObject(thisValue));
        instance = runtimeObject->getInternalInstance();
        if (!instance) 
            return JSValue::encode(RuntimeObject::throwInvalidAccessError(exec));
    } else {
        // Calling a runtime object of a plugin element?
        if (thisValue.inherits(&JSHTMLElement::s_info)) {
            HTMLElement* element = jsCast<JSHTMLElement*>(asObject(thisValue))->impl();
            instance = pluginInstance(element);
        }
        if (!instance)
            return throwVMTypeError(exec);
    }
    ASSERT(instance);

    instance->begin();
    JSValue result = instance->invokeMethod(exec, method);
    instance->end();
    return JSValue::encode(result);
}

CallType RuntimeMethod::getCallData(JSCell*, CallData& callData)
{
    callData.native.function = callRuntimeMethod;
    return CallTypeHost;
}

}
