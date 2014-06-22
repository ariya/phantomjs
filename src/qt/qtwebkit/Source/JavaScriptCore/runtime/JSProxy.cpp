/*
 * Copyright (C) 2011, 2012 Apple Inc. All rights reserved.
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

#include "config.h"
#include "JSProxy.h"

#include "JSGlobalObject.h"
#include "Operations.h"

namespace JSC {

ASSERT_HAS_TRIVIAL_DESTRUCTOR(JSProxy);

const ClassInfo JSProxy::s_info = { "JSProxy", &Base::s_info, 0, 0, CREATE_METHOD_TABLE(JSProxy) };

void JSProxy::visitChildren(JSCell* cell, SlotVisitor& visitor)
{
    JSProxy* thisObject = jsCast<JSProxy*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, &s_info);

    COMPILE_ASSERT(StructureFlags & OverridesVisitChildren, OverridesVisitChildrenWithoutSettingFlag);
    ASSERT(thisObject->structure()->typeInfo().overridesVisitChildren());

    Base::visitChildren(thisObject, visitor);
    visitor.append(&thisObject->m_target);
}

void JSProxy::setTarget(VM& vm, JSGlobalObject* globalObject)
{
    ASSERT_ARG(globalObject, globalObject);
    m_target.set(vm, this, globalObject);
    setPrototype(vm, globalObject->prototype());

    PrototypeMap& prototypeMap = vm.prototypeMap;
    if (!prototypeMap.isPrototype(this))
        return;

    // This is slow but constant time. We think it's very rare for a proxy
    // to be a prototype, and reasonably rare to retarget a proxy,
    // so slow constant time is OK.
    for (size_t i = 0; i <= JSFinalObject::maxInlineCapacity(); ++i)
        prototypeMap.clearEmptyObjectStructureForPrototype(this, i);
}

String JSProxy::className(const JSObject* object)
{
    const JSProxy* thisObject = jsCast<const JSProxy*>(object);
    return thisObject->target()->methodTable()->className(thisObject->target());
}

bool JSProxy::getOwnPropertySlot(JSCell* cell, ExecState* exec, PropertyName propertyName, PropertySlot& slot)
{
    JSProxy* thisObject = jsCast<JSProxy*>(cell);
    return thisObject->target()->methodTable()->getOwnPropertySlot(thisObject->target(), exec, propertyName, slot);
}

bool JSProxy::getOwnPropertySlotByIndex(JSCell* cell, ExecState* exec, unsigned propertyName, PropertySlot& slot)
{
    JSProxy* thisObject = jsCast<JSProxy*>(cell);
    return thisObject->target()->methodTable()->getOwnPropertySlotByIndex(thisObject->target(), exec, propertyName, slot);
}

bool JSProxy::getOwnPropertyDescriptor(JSObject* object, ExecState* exec, PropertyName propertyName, PropertyDescriptor& descriptor)
{
    JSProxy* thisObject = jsCast<JSProxy*>(object);
    return thisObject->target()->methodTable()->getOwnPropertyDescriptor(thisObject->target(), exec, propertyName, descriptor);
}

void JSProxy::put(JSCell* cell, ExecState* exec, PropertyName propertyName, JSValue value, PutPropertySlot& slot)
{
    JSProxy* thisObject = jsCast<JSProxy*>(cell);
    thisObject->target()->methodTable()->put(thisObject->target(), exec, propertyName, value, slot);
}

void JSProxy::putByIndex(JSCell* cell, ExecState* exec, unsigned propertyName, JSValue value, bool shouldThrow)
{
    JSProxy* thisObject = jsCast<JSProxy*>(cell);
    thisObject->target()->methodTable()->putByIndex(thisObject->target(), exec, propertyName, value, shouldThrow);
}

void JSProxy::putDirectVirtual(JSObject* object, ExecState* exec, PropertyName propertyName, JSValue value, unsigned attributes)
{
    JSProxy* thisObject = jsCast<JSProxy*>(object);
    thisObject->target()->putDirectVirtual(thisObject->target(), exec, propertyName, value, attributes);
}

bool JSProxy::defineOwnProperty(JSC::JSObject* object, JSC::ExecState* exec, JSC::PropertyName propertyName, JSC::PropertyDescriptor& descriptor, bool shouldThrow)
{
    JSProxy* thisObject = jsCast<JSProxy*>(object);
    return thisObject->target()->methodTable()->defineOwnProperty(thisObject->target(), exec, propertyName, descriptor, shouldThrow);
}

bool JSProxy::deleteProperty(JSCell* cell, ExecState* exec, PropertyName propertyName)
{
    JSProxy* thisObject = jsCast<JSProxy*>(cell);
    return thisObject->target()->methodTable()->deleteProperty(thisObject->target(), exec, propertyName);
}

bool JSProxy::deletePropertyByIndex(JSCell* cell, ExecState* exec, unsigned propertyName)
{
    JSProxy* thisObject = jsCast<JSProxy*>(cell);
    return thisObject->target()->methodTable()->deletePropertyByIndex(thisObject->target(), exec, propertyName);
}

void JSProxy::getPropertyNames(JSObject* object, ExecState* exec, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    JSProxy* thisObject = jsCast<JSProxy*>(object);
    thisObject->target()->methodTable()->getPropertyNames(thisObject->target(), exec, propertyNames, mode);
}

void JSProxy::getOwnPropertyNames(JSObject* object, ExecState* exec, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    JSProxy* thisObject = jsCast<JSProxy*>(object);
    thisObject->target()->methodTable()->getOwnPropertyNames(thisObject->target(), exec, propertyNames, mode);
}

} // namespace JSC
