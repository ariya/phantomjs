/*
 * Copyright (C) 2008, 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
 
#include "config.h"
#include "JSActivation.h"

#include "Arguments.h"
#include "Interpreter.h"
#include "JSFunction.h"
#include "Operations.h"

using namespace std;

namespace JSC {

const ClassInfo JSActivation::s_info = { "JSActivation", &Base::s_info, 0, 0, CREATE_METHOD_TABLE(JSActivation) };

void JSActivation::visitChildren(JSCell* cell, SlotVisitor& visitor)
{
    JSActivation* thisObject = jsCast<JSActivation*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, &s_info);
    COMPILE_ASSERT(StructureFlags & OverridesVisitChildren, OverridesVisitChildrenWithoutSettingFlag);
    ASSERT(thisObject->structure()->typeInfo().overridesVisitChildren());
    Base::visitChildren(thisObject, visitor);

    // No need to mark our registers if they're still in the JSStack.
    if (!thisObject->isTornOff())
        return;

    for (int i = 0; i < thisObject->symbolTable()->captureCount(); ++i)
        visitor.append(&thisObject->storage()[i]);
}

inline bool JSActivation::symbolTableGet(PropertyName propertyName, PropertySlot& slot)
{
    SymbolTableEntry entry = symbolTable()->inlineGet(propertyName.publicName());
    if (entry.isNull())
        return false;

    // Defend against the inspector asking for a var after it has been optimized out.
    if (isTornOff() && !isValid(entry))
        return false;

    slot.setValue(registerAt(entry.getIndex()).get());
    return true;
}

inline bool JSActivation::symbolTableGet(PropertyName propertyName, PropertyDescriptor& descriptor)
{
    SymbolTableEntry entry = symbolTable()->inlineGet(propertyName.publicName());
    if (entry.isNull())
        return false;

    // Defend against the inspector asking for a var after it has been optimized out.
    if (isTornOff() && !isValid(entry))
        return false;

    descriptor.setDescriptor(registerAt(entry.getIndex()).get(), entry.getAttributes());
    return true;
}

inline bool JSActivation::symbolTablePut(ExecState* exec, PropertyName propertyName, JSValue value, bool shouldThrow)
{
    VM& vm = exec->vm();
    ASSERT(!Heap::heap(value) || Heap::heap(value) == Heap::heap(this));
    
    SymbolTableEntry entry = symbolTable()->inlineGet(propertyName.publicName());
    if (entry.isNull())
        return false;
    if (entry.isReadOnly()) {
        if (shouldThrow)
            throwTypeError(exec, StrictModeReadonlyPropertyWriteError);
        return true;
    }

    // Defend against the inspector asking for a var after it has been optimized out.
    if (isTornOff() && !isValid(entry))
        return false;

    registerAt(entry.getIndex()).set(vm, this, value);
    return true;
}

void JSActivation::getOwnNonIndexPropertyNames(JSObject* object, ExecState* exec, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    JSActivation* thisObject = jsCast<JSActivation*>(object);

    if (mode == IncludeDontEnumProperties && !thisObject->isTornOff())
        propertyNames.add(exec->propertyNames().arguments);

    SymbolTable::const_iterator end = thisObject->symbolTable()->end();
    for (SymbolTable::const_iterator it = thisObject->symbolTable()->begin(); it != end; ++it) {
        if (it->value.getAttributes() & DontEnum && mode != IncludeDontEnumProperties)
            continue;
        if (!thisObject->isValid(it->value))
            continue;
        propertyNames.add(Identifier(exec, it->key.get()));
    }
    // Skip the JSVariableObject implementation of getOwnNonIndexPropertyNames
    JSObject::getOwnNonIndexPropertyNames(thisObject, exec, propertyNames, mode);
}

inline bool JSActivation::symbolTablePutWithAttributes(VM& vm, PropertyName propertyName, JSValue value, unsigned attributes)
{
    ASSERT(!Heap::heap(value) || Heap::heap(value) == Heap::heap(this));
    
    SymbolTable::iterator iter = symbolTable()->find(propertyName.publicName());
    if (iter == symbolTable()->end())
        return false;
    SymbolTableEntry& entry = iter->value;
    ASSERT(!entry.isNull());
    if (!isValid(entry))
        return false;

    entry.setAttributes(attributes);
    registerAt(entry.getIndex()).set(vm, this, value);
    return true;
}

bool JSActivation::getOwnPropertySlot(JSCell* cell, ExecState* exec, PropertyName propertyName, PropertySlot& slot)
{
    JSActivation* thisObject = jsCast<JSActivation*>(cell);

    if (propertyName == exec->propertyNames().arguments) {
        // Defend against the inspector asking for the arguments object after it has been optimized out.
        if (!thisObject->isTornOff()) {
            slot.setCustom(thisObject, thisObject->getArgumentsGetter());
            return true;
        }
    }

    if (thisObject->symbolTableGet(propertyName, slot))
        return true;

    if (JSValue value = thisObject->getDirect(exec->vm(), propertyName)) {
        slot.setValue(value);
        return true;
    }

    // We don't call through to JSObject because there's no way to give an 
    // activation object getter properties or a prototype.
    ASSERT(!thisObject->hasGetterSetterProperties());
    ASSERT(thisObject->prototype().isNull());
    return false;
}

bool JSActivation::getOwnPropertyDescriptor(JSObject* object, ExecState* exec, PropertyName propertyName, PropertyDescriptor& descriptor)
{
    JSActivation* thisObject = jsCast<JSActivation*>(object);

    if (propertyName == exec->propertyNames().arguments) {
        // Defend against the inspector asking for the arguments object after it has been optimized out.
        if (!thisObject->isTornOff()) {
            PropertySlot slot;
            JSActivation::getOwnPropertySlot(thisObject, exec, propertyName, slot);
            descriptor.setDescriptor(slot.getValue(exec, propertyName), DontEnum);
            return true;
        }
    }

    if (thisObject->symbolTableGet(propertyName, descriptor))
        return true;

    return Base::getOwnPropertyDescriptor(object, exec, propertyName, descriptor);
}

void JSActivation::put(JSCell* cell, ExecState* exec, PropertyName propertyName, JSValue value, PutPropertySlot& slot)
{
    JSActivation* thisObject = jsCast<JSActivation*>(cell);
    ASSERT(!Heap::heap(value) || Heap::heap(value) == Heap::heap(thisObject));

    if (thisObject->symbolTablePut(exec, propertyName, value, slot.isStrictMode()))
        return;

    // We don't call through to JSObject because __proto__ and getter/setter 
    // properties are non-standard extensions that other implementations do not
    // expose in the activation object.
    ASSERT(!thisObject->hasGetterSetterProperties());
    thisObject->putOwnDataProperty(exec->vm(), propertyName, value, slot);
}

// FIXME: Make this function honor ReadOnly (const) and DontEnum
void JSActivation::putDirectVirtual(JSObject* object, ExecState* exec, PropertyName propertyName, JSValue value, unsigned attributes)
{
    JSActivation* thisObject = jsCast<JSActivation*>(object);
    ASSERT(!Heap::heap(value) || Heap::heap(value) == Heap::heap(thisObject));

    if (thisObject->symbolTablePutWithAttributes(exec->vm(), propertyName, value, attributes))
        return;

    // We don't call through to JSObject because __proto__ and getter/setter 
    // properties are non-standard extensions that other implementations do not
    // expose in the activation object.
    ASSERT(!thisObject->hasGetterSetterProperties());
    JSObject::putDirectVirtual(thisObject, exec, propertyName, value, attributes);
}

bool JSActivation::deleteProperty(JSCell* cell, ExecState* exec, PropertyName propertyName)
{
    if (propertyName == exec->propertyNames().arguments)
        return false;

    return Base::deleteProperty(cell, exec, propertyName);
}

JSObject* JSActivation::toThisObject(JSCell*, ExecState* exec)
{
    return exec->globalThisValue();
}

JSValue JSActivation::argumentsGetter(ExecState*, JSValue slotBase, PropertyName)
{
    JSActivation* activation = jsCast<JSActivation*>(slotBase);
    if (activation->isTornOff())
        return jsUndefined();

    CallFrame* callFrame = CallFrame::create(reinterpret_cast<Register*>(activation->m_registers));
    int argumentsRegister = callFrame->codeBlock()->argumentsRegister();
    if (JSValue arguments = callFrame->uncheckedR(argumentsRegister).jsValue())
        return arguments;
    int realArgumentsRegister = unmodifiedArgumentsRegister(argumentsRegister);

    JSValue arguments = JSValue(Arguments::create(callFrame->vm(), callFrame));
    callFrame->uncheckedR(argumentsRegister) = arguments;
    callFrame->uncheckedR(realArgumentsRegister) = arguments;
    
    ASSERT(callFrame->uncheckedR(realArgumentsRegister).jsValue().inherits(&Arguments::s_info));
    return callFrame->uncheckedR(realArgumentsRegister).jsValue();
}

// These two functions serve the purpose of isolating the common case from a
// PIC branch.

PropertySlot::GetValueFunc JSActivation::getArgumentsGetter()
{
    return argumentsGetter;
}

} // namespace JSC
