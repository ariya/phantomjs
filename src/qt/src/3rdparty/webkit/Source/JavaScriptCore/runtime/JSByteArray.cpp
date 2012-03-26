/*
 * Copyright (C) 2009 Apple Inc. All Rights Reserved.
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
#include "JSByteArray.h"

#include "JSGlobalObject.h"
#include "PropertyNameArray.h"

using namespace WTF;

namespace JSC {

const ClassInfo JSByteArray::s_defaultInfo = { "ByteArray", &Base::s_info, 0, 0 };

JSByteArray::JSByteArray(ExecState* exec, Structure* structure, ByteArray* storage)
    : JSNonFinalObject(exec->globalData(), structure)
    , m_storage(storage)
{
    putDirect(exec->globalData(), exec->globalData().propertyNames->length, jsNumber(m_storage->length()), ReadOnly | DontDelete);
}

#if !ASSERT_DISABLED
JSByteArray::~JSByteArray()
{
    ASSERT(vptr() == JSGlobalData::jsByteArrayVPtr);
}
#endif


Structure* JSByteArray::createStructure(JSGlobalData& globalData, JSValue prototype, const JSC::ClassInfo* classInfo)
{
    return Structure::create(globalData, prototype, TypeInfo(ObjectType, StructureFlags), AnonymousSlotCount, classInfo);
}

bool JSByteArray::getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    bool ok;
    unsigned index = propertyName.toUInt32(ok);
    if (ok && canAccessIndex(index)) {
        slot.setValue(getIndex(exec, index));
        return true;
    }
    return JSObject::getOwnPropertySlot(exec, propertyName, slot);
}

bool JSByteArray::getOwnPropertyDescriptor(ExecState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    bool ok;
    unsigned index = propertyName.toUInt32(ok);
    if (ok && canAccessIndex(index)) {
        descriptor.setDescriptor(getIndex(exec, index), DontDelete);
        return true;
    }
    return JSObject::getOwnPropertyDescriptor(exec, propertyName, descriptor);
}

bool JSByteArray::getOwnPropertySlot(ExecState* exec, unsigned propertyName, PropertySlot& slot)
{
    if (canAccessIndex(propertyName)) {
        slot.setValue(getIndex(exec, propertyName));
        return true;
    }
    return JSObject::getOwnPropertySlot(exec, Identifier::from(exec, propertyName), slot);
}

void JSByteArray::put(ExecState* exec, const Identifier& propertyName, JSValue value, PutPropertySlot& slot)
{
    bool ok;
    unsigned index = propertyName.toUInt32(ok);
    if (ok) {
        setIndex(exec, index, value);
        return;
    }
    JSObject::put(exec, propertyName, value, slot);
}

void JSByteArray::put(ExecState* exec, unsigned propertyName, JSValue value)
{
    setIndex(exec, propertyName, value);
}

void JSByteArray::getOwnPropertyNames(ExecState* exec, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    unsigned length = m_storage->length();
    for (unsigned i = 0; i < length; ++i)
        propertyNames.add(Identifier::from(exec, i));
    JSObject::getOwnPropertyNames(exec, propertyNames, mode);
}

}

