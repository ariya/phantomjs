/*
 * Copyright (C) 2008, 2009 Apple Inc. All rights reserved.
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
#include "DebuggerActivation.h"

#include "JSActivation.h"
#include "Operations.h"

namespace JSC {

ASSERT_HAS_TRIVIAL_DESTRUCTOR(DebuggerActivation);

const ClassInfo DebuggerActivation::s_info = { "DebuggerActivation", &Base::s_info, 0, 0, CREATE_METHOD_TABLE(DebuggerActivation) };

DebuggerActivation::DebuggerActivation(VM& vm)
    : JSNonFinalObject(vm, vm.debuggerActivationStructure.get())
{
}

void DebuggerActivation::finishCreation(VM& vm, JSObject* activation)
{
    Base::finishCreation(vm);
    ASSERT(activation);
    ASSERT(activation->isActivationObject());
    m_activation.set(vm, this, jsCast<JSActivation*>(activation));
}

void DebuggerActivation::visitChildren(JSCell* cell, SlotVisitor& visitor)
{
    DebuggerActivation* thisObject = jsCast<DebuggerActivation*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, &s_info);
    COMPILE_ASSERT(StructureFlags & OverridesVisitChildren, OverridesVisitChildrenWithoutSettingFlag);
    ASSERT(thisObject->structure()->typeInfo().overridesVisitChildren());

    JSObject::visitChildren(thisObject, visitor);
    visitor.append(&thisObject->m_activation);
}

String DebuggerActivation::className(const JSObject* object)
{
    const DebuggerActivation* thisObject = jsCast<const DebuggerActivation*>(object);
    return thisObject->m_activation->methodTable()->className(thisObject->m_activation.get());
}

bool DebuggerActivation::getOwnPropertySlot(JSCell* cell, ExecState* exec, PropertyName propertyName, PropertySlot& slot)
{
    DebuggerActivation* thisObject = jsCast<DebuggerActivation*>(cell);
    return thisObject->m_activation->methodTable()->getOwnPropertySlot(thisObject->m_activation.get(), exec, propertyName, slot);
}

void DebuggerActivation::put(JSCell* cell, ExecState* exec, PropertyName propertyName, JSValue value, PutPropertySlot& slot)
{
    DebuggerActivation* thisObject = jsCast<DebuggerActivation*>(cell);
    thisObject->m_activation->methodTable()->put(thisObject->m_activation.get(), exec, propertyName, value, slot);
}

void DebuggerActivation::putDirectVirtual(JSObject* object, ExecState* exec, PropertyName propertyName, JSValue value, unsigned attributes)
{
    DebuggerActivation* thisObject = jsCast<DebuggerActivation*>(object);
    thisObject->m_activation->methodTable()->putDirectVirtual(thisObject->m_activation.get(), exec, propertyName, value, attributes);
}

bool DebuggerActivation::deleteProperty(JSCell* cell, ExecState* exec, PropertyName propertyName)
{
    DebuggerActivation* thisObject = jsCast<DebuggerActivation*>(cell);
    return thisObject->m_activation->methodTable()->deleteProperty(thisObject->m_activation.get(), exec, propertyName);
}

void DebuggerActivation::getOwnPropertyNames(JSObject* object, ExecState* exec, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    DebuggerActivation* thisObject = jsCast<DebuggerActivation*>(object);
    thisObject->m_activation->methodTable()->getPropertyNames(thisObject->m_activation.get(), exec, propertyNames, mode);
}

bool DebuggerActivation::getOwnPropertyDescriptor(JSObject* object, ExecState* exec, PropertyName propertyName, PropertyDescriptor& descriptor)
{
    DebuggerActivation* thisObject = jsCast<DebuggerActivation*>(object);
    return thisObject->m_activation->methodTable()->getOwnPropertyDescriptor(thisObject->m_activation.get(), exec, propertyName, descriptor);
}

bool DebuggerActivation::defineOwnProperty(JSObject* object, ExecState* exec, PropertyName propertyName, PropertyDescriptor& descriptor, bool shouldThrow)
{
    DebuggerActivation* thisObject = jsCast<DebuggerActivation*>(object);
    return thisObject->m_activation->methodTable()->defineOwnProperty(thisObject->m_activation.get(), exec, propertyName, descriptor, shouldThrow);
}

} // namespace JSC
