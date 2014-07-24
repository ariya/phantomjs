/*
 *  Copyright (C) 1999-2002 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 *  Copyright (C) 2007 Cameron Zwarich (cwzwarich@uwaterloo.ca)
 *  Copyright (C) 2007 Maks Orlovich
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "Arguments.h"

#include "JSActivation.h"
#include "JSFunction.h"
#include "JSGlobalObject.h"
#include "Operations.h"

using namespace std;

namespace JSC {

const ClassInfo Arguments::s_info = { "Arguments", &Base::s_info, 0, 0, CREATE_METHOD_TABLE(Arguments) };

void Arguments::visitChildren(JSCell* cell, SlotVisitor& visitor)
{
    Arguments* thisObject = jsCast<Arguments*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, &s_info);
    COMPILE_ASSERT(StructureFlags & OverridesVisitChildren, OverridesVisitChildrenWithoutSettingFlag);
    ASSERT(thisObject->structure()->typeInfo().overridesVisitChildren());
    JSObject::visitChildren(thisObject, visitor);

    if (thisObject->m_registerArray)
        visitor.appendValues(thisObject->m_registerArray.get(), thisObject->m_numArguments);
    visitor.append(&thisObject->m_callee);
    visitor.append(&thisObject->m_activation);
}

void Arguments::destroy(JSCell* cell)
{
    static_cast<Arguments*>(cell)->Arguments::~Arguments();
}

void Arguments::copyToArguments(ExecState* exec, CallFrame* callFrame, uint32_t length)
{
    if (UNLIKELY(m_overrodeLength)) {
        length = min(get(exec, exec->propertyNames().length).toUInt32(exec), length);
        for (unsigned i = 0; i < length; i++)
            callFrame->setArgument(i, get(exec, i));
        return;
    }
    ASSERT(length == this->length(exec));
    for (size_t i = 0; i < length; ++i) {
        if (JSValue value = tryGetArgument(i))
            callFrame->setArgument(i, value);
        else
            callFrame->setArgument(i, get(exec, i));
    }
}

void Arguments::fillArgList(ExecState* exec, MarkedArgumentBuffer& args)
{
    if (UNLIKELY(m_overrodeLength)) {
        unsigned length = get(exec, exec->propertyNames().length).toUInt32(exec); 
        for (unsigned i = 0; i < length; i++) 
            args.append(get(exec, i)); 
        return;
    }
    uint32_t length = this->length(exec);
    for (size_t i = 0; i < length; ++i) {
        if (JSValue value = tryGetArgument(i))
            args.append(value);
        else
            args.append(get(exec, i));
    }
}

bool Arguments::getOwnPropertySlotByIndex(JSCell* cell, ExecState* exec, unsigned i, PropertySlot& slot)
{
    Arguments* thisObject = jsCast<Arguments*>(cell);
    if (JSValue value = thisObject->tryGetArgument(i)) {
        slot.setValue(value);
        return true;
    }

    return JSObject::getOwnPropertySlot(thisObject, exec, Identifier(exec, String::number(i)), slot);
}
    
void Arguments::createStrictModeCallerIfNecessary(ExecState* exec)
{
    if (m_overrodeCaller)
        return;

    m_overrodeCaller = true;
    PropertyDescriptor descriptor;
    descriptor.setAccessorDescriptor(globalObject()->throwTypeErrorGetterSetter(exec), DontEnum | DontDelete | Accessor);
    methodTable()->defineOwnProperty(this, exec, exec->propertyNames().caller, descriptor, false);
}

void Arguments::createStrictModeCalleeIfNecessary(ExecState* exec)
{
    if (m_overrodeCallee)
        return;
    
    m_overrodeCallee = true;
    PropertyDescriptor descriptor;
    descriptor.setAccessorDescriptor(globalObject()->throwTypeErrorGetterSetter(exec), DontEnum | DontDelete | Accessor);
    methodTable()->defineOwnProperty(this, exec, exec->propertyNames().callee, descriptor, false);
}

bool Arguments::getOwnPropertySlot(JSCell* cell, ExecState* exec, PropertyName propertyName, PropertySlot& slot)
{
    Arguments* thisObject = jsCast<Arguments*>(cell);
    unsigned i = propertyName.asIndex();
    if (JSValue value = thisObject->tryGetArgument(i)) {
        RELEASE_ASSERT(i < PropertyName::NotAnIndex);
        slot.setValue(value);
        return true;
    }

    if (propertyName == exec->propertyNames().length && LIKELY(!thisObject->m_overrodeLength)) {
        slot.setValue(jsNumber(thisObject->m_numArguments));
        return true;
    }

    if (propertyName == exec->propertyNames().callee && LIKELY(!thisObject->m_overrodeCallee)) {
        if (!thisObject->m_isStrictMode) {
            slot.setValue(thisObject->m_callee.get());
            return true;
        }
        thisObject->createStrictModeCalleeIfNecessary(exec);
    }

    if (propertyName == exec->propertyNames().caller && thisObject->m_isStrictMode)
        thisObject->createStrictModeCallerIfNecessary(exec);

    return JSObject::getOwnPropertySlot(thisObject, exec, propertyName, slot);
}

bool Arguments::getOwnPropertyDescriptor(JSObject* object, ExecState* exec, PropertyName propertyName, PropertyDescriptor& descriptor)
{
    Arguments* thisObject = jsCast<Arguments*>(object);
    unsigned i = propertyName.asIndex();
    if (JSValue value = thisObject->tryGetArgument(i)) {
        RELEASE_ASSERT(i < PropertyName::NotAnIndex);
        descriptor.setDescriptor(value, None);
        return true;
    }
    
    if (propertyName == exec->propertyNames().length && LIKELY(!thisObject->m_overrodeLength)) {
        descriptor.setDescriptor(jsNumber(thisObject->m_numArguments), DontEnum);
        return true;
    }
    
    if (propertyName == exec->propertyNames().callee && LIKELY(!thisObject->m_overrodeCallee)) {
        if (!thisObject->m_isStrictMode) {
            descriptor.setDescriptor(thisObject->m_callee.get(), DontEnum);
            return true;
        }
        thisObject->createStrictModeCalleeIfNecessary(exec);
    }

    if (propertyName == exec->propertyNames().caller && thisObject->m_isStrictMode)
        thisObject->createStrictModeCallerIfNecessary(exec);
    
    return JSObject::getOwnPropertyDescriptor(thisObject, exec, propertyName, descriptor);
}

void Arguments::getOwnPropertyNames(JSObject* object, ExecState* exec, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    Arguments* thisObject = jsCast<Arguments*>(object);
    for (unsigned i = 0; i < thisObject->m_numArguments; ++i) {
        if (!thisObject->isArgument(i))
            continue;
        propertyNames.add(Identifier(exec, String::number(i)));
    }
    if (mode == IncludeDontEnumProperties) {
        propertyNames.add(exec->propertyNames().callee);
        propertyNames.add(exec->propertyNames().length);
    }
    JSObject::getOwnPropertyNames(thisObject, exec, propertyNames, mode);
}

void Arguments::putByIndex(JSCell* cell, ExecState* exec, unsigned i, JSValue value, bool shouldThrow)
{
    Arguments* thisObject = jsCast<Arguments*>(cell);
    if (thisObject->trySetArgument(exec->vm(), i, value))
        return;

    PutPropertySlot slot(shouldThrow);
    JSObject::put(thisObject, exec, Identifier(exec, String::number(i)), value, slot);
}

void Arguments::put(JSCell* cell, ExecState* exec, PropertyName propertyName, JSValue value, PutPropertySlot& slot)
{
    Arguments* thisObject = jsCast<Arguments*>(cell);
    unsigned i = propertyName.asIndex();
    if (thisObject->trySetArgument(exec->vm(), i, value))
        return;

    if (propertyName == exec->propertyNames().length && !thisObject->m_overrodeLength) {
        thisObject->m_overrodeLength = true;
        thisObject->putDirect(exec->vm(), propertyName, value, DontEnum);
        return;
    }

    if (propertyName == exec->propertyNames().callee && !thisObject->m_overrodeCallee) {
        if (!thisObject->m_isStrictMode) {
            thisObject->m_overrodeCallee = true;
            thisObject->putDirect(exec->vm(), propertyName, value, DontEnum);
            return;
        }
        thisObject->createStrictModeCalleeIfNecessary(exec);
    }

    if (propertyName == exec->propertyNames().caller && thisObject->m_isStrictMode)
        thisObject->createStrictModeCallerIfNecessary(exec);

    JSObject::put(thisObject, exec, propertyName, value, slot);
}

bool Arguments::deletePropertyByIndex(JSCell* cell, ExecState* exec, unsigned i) 
{
    Arguments* thisObject = jsCast<Arguments*>(cell);
    if (i < thisObject->m_numArguments) {
        if (!Base::deletePropertyByIndex(cell, exec, i))
            return false;
        if (thisObject->tryDeleteArgument(i))
            return true;
    }
    return JSObject::deletePropertyByIndex(thisObject, exec, i);
}

bool Arguments::deleteProperty(JSCell* cell, ExecState* exec, PropertyName propertyName) 
{
    if (exec->vm().isInDefineOwnProperty())
        return Base::deleteProperty(cell, exec, propertyName);

    Arguments* thisObject = jsCast<Arguments*>(cell);
    unsigned i = propertyName.asIndex();
    if (i < thisObject->m_numArguments) {
        RELEASE_ASSERT(i < PropertyName::NotAnIndex);
        if (!Base::deleteProperty(cell, exec, propertyName))
            return false;
        if (thisObject->tryDeleteArgument(i))
            return true;
    }

    if (propertyName == exec->propertyNames().length && !thisObject->m_overrodeLength) {
        thisObject->m_overrodeLength = true;
        return true;
    }

    if (propertyName == exec->propertyNames().callee && !thisObject->m_overrodeCallee) {
        if (!thisObject->m_isStrictMode) {
            thisObject->m_overrodeCallee = true;
            return true;
        }
        thisObject->createStrictModeCalleeIfNecessary(exec);
    }
    
    if (propertyName == exec->propertyNames().caller && thisObject->m_isStrictMode)
        thisObject->createStrictModeCallerIfNecessary(exec);

    return JSObject::deleteProperty(thisObject, exec, propertyName);
}

bool Arguments::defineOwnProperty(JSObject* object, ExecState* exec, PropertyName propertyName, PropertyDescriptor& descriptor, bool shouldThrow)
{
    Arguments* thisObject = jsCast<Arguments*>(object);
    unsigned i = propertyName.asIndex();
    if (i < thisObject->m_numArguments) {
        RELEASE_ASSERT(i < PropertyName::NotAnIndex);
        // If the property is not yet present on the object, and is not yet marked as deleted, then add it now.
        PropertySlot slot;
        if (!thisObject->isDeletedArgument(i) && !JSObject::getOwnPropertySlot(thisObject, exec, propertyName, slot)) {
            JSValue value = thisObject->tryGetArgument(i);
            ASSERT(value);
            object->putDirectMayBeIndex(exec, propertyName, value);
        }
        if (!Base::defineOwnProperty(object, exec, propertyName, descriptor, shouldThrow))
            return false;

        // From ES 5.1, 10.6 Arguments Object
        // 5. If the value of isMapped is not undefined, then
        if (thisObject->isArgument(i)) {
            // a. If IsAccessorDescriptor(Desc) is true, then
            if (descriptor.isAccessorDescriptor()) {
                // i. Call the [[Delete]] internal method of map passing P, and false as the arguments.
                thisObject->tryDeleteArgument(i);
            } else { // b. Else
                // i. If Desc.[[Value]] is present, then
                // 1. Call the [[Put]] internal method of map passing P, Desc.[[Value]], and Throw as the arguments.
                if (descriptor.value())
                    thisObject->trySetArgument(exec->vm(), i, descriptor.value());
                // ii. If Desc.[[Writable]] is present and its value is false, then
                // 1. Call the [[Delete]] internal method of map passing P and false as arguments.
                if (descriptor.writablePresent() && !descriptor.writable())
                    thisObject->tryDeleteArgument(i);
            }
        }
        return true;
    }

    if (propertyName == exec->propertyNames().length && !thisObject->m_overrodeLength) {
        thisObject->putDirect(exec->vm(), propertyName, jsNumber(thisObject->m_numArguments), DontEnum);
        thisObject->m_overrodeLength = true;
    } else if (propertyName == exec->propertyNames().callee && !thisObject->m_overrodeCallee) {
        thisObject->putDirect(exec->vm(), propertyName, thisObject->m_callee.get(), DontEnum);
        thisObject->m_overrodeCallee = true;
    } else if (propertyName == exec->propertyNames().caller && thisObject->m_isStrictMode)
        thisObject->createStrictModeCallerIfNecessary(exec);

    return Base::defineOwnProperty(object, exec, propertyName, descriptor, shouldThrow);
}

void Arguments::tearOff(CallFrame* callFrame)
{
    if (isTornOff())
        return;

    if (!m_numArguments)
        return;

    // Must be called for the same call frame from which it was created.
    ASSERT(bitwise_cast<WriteBarrier<Unknown>*>(callFrame) == m_registers);
    
    m_registerArray = adoptArrayPtr(new WriteBarrier<Unknown>[m_numArguments]);
    m_registers = m_registerArray.get() + CallFrame::offsetFor(m_numArguments + 1);

    // If we have a captured argument that logically aliases activation storage,
    // but we optimize away the activation, the argument needs to tear off into
    // our storage. The simplest way to do this is to revert it to Normal status.
    if (m_slowArguments && !m_activation) {
        for (size_t i = 0; i < m_numArguments; ++i) {
            if (m_slowArguments[i].status != SlowArgument::Captured)
                continue;
            m_slowArguments[i].status = SlowArgument::Normal;
            m_slowArguments[i].index = CallFrame::argumentOffset(i);
        }
    }

    if (!callFrame->isInlineCallFrame()) {
        for (size_t i = 0; i < m_numArguments; ++i)
            trySetArgument(callFrame->vm(), i, callFrame->argumentAfterCapture(i));
        return;
    }

    tearOffForInlineCallFrame(
        callFrame->vm(), callFrame->registers(), callFrame->inlineCallFrame());
}

void Arguments::didTearOffActivation(ExecState* exec, JSActivation* activation)
{
    RELEASE_ASSERT(activation);
    if (isTornOff())
        return;

    if (!m_numArguments)
        return;
    
    m_activation.set(exec->vm(), this, activation);
    tearOff(exec);
}

void Arguments::tearOff(CallFrame* callFrame, InlineCallFrame* inlineCallFrame)
{
    if (isTornOff())
        return;
    
    if (!m_numArguments)
        return;
    
    m_registerArray = adoptArrayPtr(new WriteBarrier<Unknown>[m_numArguments]);
    m_registers = m_registerArray.get() + CallFrame::offsetFor(m_numArguments + 1);

    tearOffForInlineCallFrame(
        callFrame->vm(), callFrame->registers() + inlineCallFrame->stackOffset,
        inlineCallFrame);
}

void Arguments::tearOffForInlineCallFrame(VM& vm, Register* registers, InlineCallFrame* inlineCallFrame)
{
    for (size_t i = 0; i < m_numArguments; ++i) {
        ValueRecovery& recovery = inlineCallFrame->arguments[i + 1];
        // In the future we'll support displaced recoveries (indicating that the
        // argument was flushed to a different location), but for now we don't do
        // that so this code will fail if that were to happen. On the other hand,
        // it's much less likely that we'll support in-register recoveries since
        // this code does not (easily) have access to registers.
        JSValue value;
        Register* location = &registers[CallFrame::argumentOffset(i)];
        switch (recovery.technique()) {
        case AlreadyInJSStack:
            value = location->jsValue();
            break;
        case AlreadyInJSStackAsUnboxedInt32:
            value = jsNumber(location->unboxedInt32());
            break;
        case AlreadyInJSStackAsUnboxedCell:
            value = location->unboxedCell();
            break;
        case AlreadyInJSStackAsUnboxedBoolean:
            value = jsBoolean(location->unboxedBoolean());
            break;
        case AlreadyInJSStackAsUnboxedDouble:
#if USE(JSVALUE64)
            value = jsNumber(*bitwise_cast<double*>(location));
#else
            value = location->jsValue();
#endif
            break;
        case Constant:
            value = recovery.constant();
            break;
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }
        trySetArgument(vm, i, value);
    }
}

} // namespace JSC
