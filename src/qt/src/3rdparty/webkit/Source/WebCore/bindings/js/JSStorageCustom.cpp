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
#include "JSStorageCustom.h"

#if ENABLE(DOM_STORAGE)

#include "PlatformString.h"
#include <runtime/PropertyNameArray.h>
#include "Storage.h"

using namespace JSC;

namespace WebCore {

bool JSStorage::canGetItemsForName(ExecState*, Storage* impl, const Identifier& propertyName)
{
    return impl->contains(identifierToString(propertyName));
}

JSValue JSStorage::nameGetter(ExecState* exec, JSValue slotBase, const Identifier& propertyName)
{
    JSStorage* thisObj = static_cast<JSStorage*>(asObject(slotBase));
    return jsStringOrNull(exec, thisObj->impl()->getItem(identifierToString(propertyName)));
}

bool JSStorage::deleteProperty(ExecState* exec, const Identifier& propertyName)
{
    // Only perform the custom delete if the object doesn't have a native property by this name.
    // Since hasProperty() would end up calling canGetItemsForName() and be fooled, we need to check
    // the native property slots manually.
    PropertySlot slot;
    if (getStaticValueSlot<JSStorage, Base>(exec, s_info.propHashTable(exec), this, propertyName, slot))
        return false;
        
    JSValue prototype = this->prototype();
    if (prototype.isObject() && asObject(prototype)->hasProperty(exec, propertyName))
        return false;

    m_impl->removeItem(identifierToString(propertyName));
    return true;
}

void JSStorage::getOwnPropertyNames(ExecState* exec, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    unsigned length = m_impl->length();
    for (unsigned i = 0; i < length; ++i)
        propertyNames.add(Identifier(exec, stringToUString(m_impl->key(i))));
        
    Base::getOwnPropertyNames(exec, propertyNames, mode);
}

bool JSStorage::putDelegate(ExecState* exec, const Identifier& propertyName, JSValue value, PutPropertySlot&)
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
    
    String stringValue = ustringToString(value.toString(exec));
    if (exec->hadException())
        return true;
    
    ExceptionCode ec = 0;
    impl()->setItem(identifierToString(propertyName), stringValue, ec);
    setDOMException(exec, ec);

    return true;
}

} // namespace WebCore

#endif // ENABLE(DOM_STORAGE)
