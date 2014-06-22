/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "JSStorage.h"

#include "Storage.h"
#include <runtime/PropertyNameArray.h>
#include <wtf/text/WTFString.h>

using namespace JSC;

namespace WebCore {

bool JSStorage::canGetItemsForName(ExecState* exec, Storage* impl, PropertyName propertyName)
{
    ExceptionCode ec = 0;
    bool result = impl->contains(propertyNameToString(propertyName), ec);
    setDOMException(exec, ec);
    return result;
}

JSValue JSStorage::nameGetter(ExecState* exec, JSValue slotBase, PropertyName propertyName)
{
    JSStorage* thisObj = jsCast<JSStorage*>(asObject(slotBase));
        
    JSValue prototype = asObject(slotBase)->prototype();
    if (prototype.isObject() && asObject(prototype)->hasProperty(exec, propertyName))
        return asObject(prototype)->get(exec, propertyName);
 
    ExceptionCode ec = 0;
    JSValue result = jsStringOrNull(exec, thisObj->impl()->getItem(propertyNameToString(propertyName), ec));
    setDOMException(exec, ec);
    return result;
}

bool JSStorage::deleteProperty(JSCell* cell, ExecState* exec, PropertyName propertyName)
{
    JSStorage* thisObject = jsCast<JSStorage*>(cell);
    // Only perform the custom delete if the object doesn't have a native property by this name.
    // Since hasProperty() would end up calling canGetItemsForName() and be fooled, we need to check
    // the native property slots manually.
    PropertySlot slot;
    if (getStaticValueSlot<JSStorage, Base>(exec, s_info.propHashTable(exec), thisObject, propertyName, slot))
        return false;
        
    JSValue prototype = thisObject->prototype();
    if (prototype.isObject() && asObject(prototype)->hasProperty(exec, propertyName))
        return false;

    ExceptionCode ec = 0;
    thisObject->m_impl->removeItem(propertyNameToString(propertyName), ec);
    setDOMException(exec, ec);
    return true;
}

bool JSStorage::deletePropertyByIndex(JSCell* cell, ExecState* exec, unsigned propertyName)
{
    return deleteProperty(cell, exec, Identifier::from(exec, propertyName));
}

void JSStorage::getOwnPropertyNames(JSObject* object, ExecState* exec, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    JSStorage* thisObject = jsCast<JSStorage*>(object);
    ExceptionCode ec = 0;
    unsigned length = thisObject->m_impl->length(ec);
    setDOMException(exec, ec);
    if (exec->hadException())
        return;
    for (unsigned i = 0; i < length; ++i) {
        propertyNames.add(Identifier(exec, thisObject->m_impl->key(i, ec)));
        setDOMException(exec, ec);
        if (exec->hadException())
            return;
    }
        
    Base::getOwnPropertyNames(thisObject, exec, propertyNames, mode);
}

bool JSStorage::putDelegate(ExecState* exec, PropertyName propertyName, JSValue value, PutPropertySlot&)
{
    // Only perform the custom put if the object doesn't have a native property by this name.
    // Since hasProperty() would end up calling canGetItemsForName() and be fooled, we need to check
    // the native property slots manually.
    PropertySlot slot;
    if (getStaticValueSlot<JSStorage, Base>(exec, s_info.propHashTable(exec), this, propertyName, slot))
        return false;
        
    JSValue prototype = this->prototype();
    if (prototype.isObject() && asObject(prototype)->hasProperty(exec, propertyName))
        return false;
    
    String stringValue = value.toString(exec)->value(exec);
    if (exec->hadException())
        return true;
    
    ExceptionCode ec = 0;
    impl()->setItem(propertyNameToString(propertyName), stringValue, ec);
    setDOMException(exec, ec);

    return true;
}

} // namespace WebCore
