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
#include "runtime_array.h"

#include <runtime/ArrayPrototype.h>
#include <runtime/Error.h>
#include <runtime/PropertyNameArray.h>
#include "JSDOMBinding.h"

using namespace WebCore;

namespace JSC {

const ClassInfo RuntimeArray::s_info = { "RuntimeArray", &JSArray::s_info, 0, 0 };

RuntimeArray::RuntimeArray(ExecState* exec, Bindings::Array* array)
    // FIXME: deprecatedGetDOMStructure uses the prototype off of the wrong global object
    // We need to pass in the right global object for "array".
    : JSArray(exec->globalData(), deprecatedGetDOMStructure<RuntimeArray>(exec))
{
    ASSERT(inherits(&s_info));
    setSubclassData(array);
}

RuntimeArray::~RuntimeArray()
{
    delete getConcreteArray();
}

JSValue RuntimeArray::lengthGetter(ExecState*, JSValue slotBase, const Identifier&)
{
    RuntimeArray* thisObj = static_cast<RuntimeArray*>(asObject(slotBase));
    return jsNumber(thisObj->getLength());
}

JSValue RuntimeArray::indexGetter(ExecState* exec, JSValue slotBase, unsigned index)
{
    RuntimeArray* thisObj = static_cast<RuntimeArray*>(asObject(slotBase));
    return thisObj->getConcreteArray()->valueAt(exec, index);
}

void RuntimeArray::getOwnPropertyNames(ExecState* exec, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    unsigned length = getLength();
    for (unsigned i = 0; i < length; ++i)
        propertyNames.add(Identifier::from(exec, i));

    if (mode == IncludeDontEnumProperties)
        propertyNames.add(exec->propertyNames().length);

    JSObject::getOwnPropertyNames(exec, propertyNames, mode);
}

bool RuntimeArray::getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    if (propertyName == exec->propertyNames().length) {
        slot.setCacheableCustom(this, lengthGetter);
        return true;
    }
    
    bool ok;
    unsigned index = propertyName.toArrayIndex(ok);
    if (ok) {
        if (index < getLength()) {
            slot.setCustomIndex(this, index, indexGetter);
            return true;
        }
    }
    
    return JSObject::getOwnPropertySlot(exec, propertyName, slot);
}

bool RuntimeArray::getOwnPropertyDescriptor(ExecState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    if (propertyName == exec->propertyNames().length) {
        PropertySlot slot;
        slot.setCustom(this, lengthGetter);
        descriptor.setDescriptor(slot.getValue(exec, propertyName), ReadOnly | DontDelete | DontEnum);
        return true;
    }
    
    bool ok;
    unsigned index = propertyName.toArrayIndex(ok);
    if (ok) {
        if (index < getLength()) {
            PropertySlot slot;
            slot.setCustomIndex(this, index, indexGetter);
            descriptor.setDescriptor(slot.getValue(exec, propertyName), DontDelete | DontEnum);
            return true;
        }
    }
    
    return JSObject::getOwnPropertyDescriptor(exec, propertyName, descriptor);
}

bool RuntimeArray::getOwnPropertySlot(ExecState *exec, unsigned index, PropertySlot& slot)
{
    if (index < getLength()) {
        slot.setCustomIndex(this, index, indexGetter);
        return true;
    }
    
    return JSObject::getOwnPropertySlot(exec, index, slot);
}

void RuntimeArray::put(ExecState* exec, const Identifier& propertyName, JSValue value, PutPropertySlot& slot)
{
    if (propertyName == exec->propertyNames().length) {
        throwError(exec, createRangeError(exec, "Range error"));
        return;
    }
    
    bool ok;
    unsigned index = propertyName.toArrayIndex(ok);
    if (ok) {
        getConcreteArray()->setValueAt(exec, index, value);
        return;
    }
    
    JSObject::put(exec, propertyName, value, slot);
}

void RuntimeArray::put(ExecState* exec, unsigned index, JSValue value)
{
    if (index >= getLength()) {
        throwError(exec, createRangeError(exec, "Range error"));
        return;
    }
    
    getConcreteArray()->setValueAt(exec, index, value);
}

bool RuntimeArray::deleteProperty(ExecState*, const Identifier&)
{
    return false;
}

bool RuntimeArray::deleteProperty(ExecState*, unsigned)
{
    return false;
}

}
