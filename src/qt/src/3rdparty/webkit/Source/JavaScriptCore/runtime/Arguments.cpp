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

using namespace std;

namespace JSC {

ASSERT_CLASS_FITS_IN_CELL(Arguments);

const ClassInfo Arguments::s_info = { "Arguments", &JSNonFinalObject::s_info, 0, 0 };

Arguments::~Arguments()
{
    if (d->extraArguments != d->extraArgumentsFixedBuffer)
        delete [] d->extraArguments;
}

void Arguments::visitChildren(SlotVisitor& visitor)
{
    JSObject::visitChildren(visitor);

    if (d->registerArray)
        visitor.appendValues(d->registerArray.get(), d->numParameters);

    if (d->extraArguments) {
        unsigned numExtraArguments = d->numArguments - d->numParameters;
        visitor.appendValues(d->extraArguments, numExtraArguments);
    }

    visitor.append(&d->callee);

    if (d->activation)
        visitor.append(&d->activation);
}

void Arguments::copyToRegisters(ExecState* exec, Register* buffer, uint32_t maxSize)
{
    if (UNLIKELY(d->overrodeLength)) {
        unsigned length = min(get(exec, exec->propertyNames().length).toUInt32(exec), maxSize);
        for (unsigned i = 0; i < length; i++)
            buffer[i] = get(exec, i);
        return;
    }

    if (LIKELY(!d->deletedArguments)) {
        unsigned parametersLength = min(min(d->numParameters, d->numArguments), maxSize);
        unsigned i = 0;
        for (; i < parametersLength; ++i)
            buffer[i] = d->registers[d->firstParameterIndex + i].get();
        for (; i < d->numArguments; ++i)
            buffer[i] = d->extraArguments[i - d->numParameters].get();
        return;
    }
    
    unsigned parametersLength = min(min(d->numParameters, d->numArguments), maxSize);
    unsigned i = 0;
    for (; i < parametersLength; ++i) {
        if (!d->deletedArguments[i])
            buffer[i] = d->registers[d->firstParameterIndex + i].get();
        else
            buffer[i] = get(exec, i);
    }
    for (; i < d->numArguments; ++i) {
        if (!d->deletedArguments[i])
            buffer[i] = d->extraArguments[i - d->numParameters].get();
        else
            buffer[i] = get(exec, i);
    }
}

void Arguments::fillArgList(ExecState* exec, MarkedArgumentBuffer& args)
{
    if (UNLIKELY(d->overrodeLength)) {
        unsigned length = get(exec, exec->propertyNames().length).toUInt32(exec); 
        for (unsigned i = 0; i < length; i++) 
            args.append(get(exec, i)); 
        return;
    }

    if (LIKELY(!d->deletedArguments)) {
        if (LIKELY(!d->numParameters)) {
            args.initialize(d->extraArguments, d->numArguments);
            return;
        }

        if (d->numParameters == d->numArguments) {
            args.initialize(&d->registers[d->firstParameterIndex], d->numArguments);
            return;
        }

        unsigned parametersLength = min(d->numParameters, d->numArguments);
        unsigned i = 0;
        for (; i < parametersLength; ++i)
            args.append(d->registers[d->firstParameterIndex + i].get());
        for (; i < d->numArguments; ++i)
            args.append(d->extraArguments[i - d->numParameters].get());
        return;
    }

    unsigned parametersLength = min(d->numParameters, d->numArguments);
    unsigned i = 0;
    for (; i < parametersLength; ++i) {
        if (!d->deletedArguments[i])
            args.append(d->registers[d->firstParameterIndex + i].get());
        else
            args.append(get(exec, i));
    }
    for (; i < d->numArguments; ++i) {
        if (!d->deletedArguments[i])
            args.append(d->extraArguments[i - d->numParameters].get());
        else
            args.append(get(exec, i));
    }
}

bool Arguments::getOwnPropertySlot(ExecState* exec, unsigned i, PropertySlot& slot)
{
    if (i < d->numArguments && (!d->deletedArguments || !d->deletedArguments[i])) {
        if (i < d->numParameters) {
            slot.setValue(d->registers[d->firstParameterIndex + i].get());
        } else
            slot.setValue(d->extraArguments[i - d->numParameters].get());
        return true;
    }

    return JSObject::getOwnPropertySlot(exec, Identifier(exec, UString::number(i)), slot);
}
    
void Arguments::createStrictModeCallerIfNecessary(ExecState* exec)
{
    if (d->overrodeCaller)
        return;

    d->overrodeCaller = true;
    PropertyDescriptor descriptor;
    JSValue thrower = createTypeErrorFunction(exec, "Unable to access caller of strict mode function");
    descriptor.setAccessorDescriptor(thrower, thrower, DontEnum | DontDelete | Getter | Setter);
    defineOwnProperty(exec, exec->propertyNames().caller, descriptor, false);
}

void Arguments::createStrictModeCalleeIfNecessary(ExecState* exec)
{
    if (d->overrodeCallee)
        return;
    
    d->overrodeCallee = true;
    PropertyDescriptor descriptor;
    JSValue thrower = createTypeErrorFunction(exec, "Unable to access callee of strict mode function");
    descriptor.setAccessorDescriptor(thrower, thrower, DontEnum | DontDelete | Getter | Setter);
    defineOwnProperty(exec, exec->propertyNames().callee, descriptor, false);
}

bool Arguments::getOwnPropertySlot(ExecState* exec, const Identifier& propertyName, PropertySlot& slot)
{
    bool isArrayIndex;
    unsigned i = propertyName.toArrayIndex(isArrayIndex);
    if (isArrayIndex && i < d->numArguments && (!d->deletedArguments || !d->deletedArguments[i])) {
        if (i < d->numParameters) {
            slot.setValue(d->registers[d->firstParameterIndex + i].get());
        } else
            slot.setValue(d->extraArguments[i - d->numParameters].get());
        return true;
    }

    if (propertyName == exec->propertyNames().length && LIKELY(!d->overrodeLength)) {
        slot.setValue(jsNumber(d->numArguments));
        return true;
    }

    if (propertyName == exec->propertyNames().callee && LIKELY(!d->overrodeCallee)) {
        if (!d->isStrictMode) {
            slot.setValue(d->callee.get());
            return true;
        }
        createStrictModeCalleeIfNecessary(exec);
    }

    if (propertyName == exec->propertyNames().caller && d->isStrictMode)
        createStrictModeCallerIfNecessary(exec);

    return JSObject::getOwnPropertySlot(exec, propertyName, slot);
}

bool Arguments::getOwnPropertyDescriptor(ExecState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    bool isArrayIndex;
    unsigned i = propertyName.toArrayIndex(isArrayIndex);
    if (isArrayIndex && i < d->numArguments && (!d->deletedArguments || !d->deletedArguments[i])) {
        if (i < d->numParameters) {
            descriptor.setDescriptor(d->registers[d->firstParameterIndex + i].get(), DontEnum);
        } else
            descriptor.setDescriptor(d->extraArguments[i - d->numParameters].get(), DontEnum);
        return true;
    }
    
    if (propertyName == exec->propertyNames().length && LIKELY(!d->overrodeLength)) {
        descriptor.setDescriptor(jsNumber(d->numArguments), DontEnum);
        return true;
    }
    
    if (propertyName == exec->propertyNames().callee && LIKELY(!d->overrodeCallee)) {
        if (!d->isStrictMode) {
            descriptor.setDescriptor(d->callee.get(), DontEnum);
            return true;
        }
        createStrictModeCalleeIfNecessary(exec);
    }

    if (propertyName == exec->propertyNames().caller && d->isStrictMode)
        createStrictModeCallerIfNecessary(exec);
    
    return JSObject::getOwnPropertyDescriptor(exec, propertyName, descriptor);
}

void Arguments::getOwnPropertyNames(ExecState* exec, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    if (mode == IncludeDontEnumProperties) {
        for (unsigned i = 0; i < d->numArguments; ++i) {
            if (!d->deletedArguments || !d->deletedArguments[i])
                propertyNames.add(Identifier(exec, UString::number(i)));
        }
        propertyNames.add(exec->propertyNames().callee);
        propertyNames.add(exec->propertyNames().length);
    }
    JSObject::getOwnPropertyNames(exec, propertyNames, mode);
}

void Arguments::put(ExecState* exec, unsigned i, JSValue value)
{
    if (i < d->numArguments && (!d->deletedArguments || !d->deletedArguments[i])) {
        if (i < d->numParameters)
            d->registers[d->firstParameterIndex + i].set(exec->globalData(), d->activation ? static_cast<JSCell*>(d->activation.get()) : static_cast<JSCell*>(this), value);
        else
            d->extraArguments[i - d->numParameters].set(exec->globalData(), this, value);
        return;
    }

    PutPropertySlot slot;
    JSObject::put(exec, Identifier(exec, UString::number(i)), value, slot);
}

void Arguments::put(ExecState* exec, const Identifier& propertyName, JSValue value, PutPropertySlot& slot)
{
    bool isArrayIndex;
    unsigned i = propertyName.toArrayIndex(isArrayIndex);
    if (isArrayIndex && i < d->numArguments && (!d->deletedArguments || !d->deletedArguments[i])) {
        if (i < d->numParameters)
            d->registers[d->firstParameterIndex + i].set(exec->globalData(), d->activation ? static_cast<JSCell*>(d->activation.get()) : static_cast<JSCell*>(this), value);
        else
            d->extraArguments[i - d->numParameters].set(exec->globalData(), this, value);
        return;
    }

    if (propertyName == exec->propertyNames().length && !d->overrodeLength) {
        d->overrodeLength = true;
        putDirect(exec->globalData(), propertyName, value, DontEnum);
        return;
    }

    if (propertyName == exec->propertyNames().callee && !d->overrodeCallee) {
        if (!d->isStrictMode) {
            d->overrodeCallee = true;
            putDirect(exec->globalData(), propertyName, value, DontEnum);
            return;
        }
        createStrictModeCalleeIfNecessary(exec);
    }

    if (propertyName == exec->propertyNames().caller && d->isStrictMode)
        createStrictModeCallerIfNecessary(exec);

    JSObject::put(exec, propertyName, value, slot);
}

bool Arguments::deleteProperty(ExecState* exec, unsigned i) 
{
    if (i < d->numArguments) {
        if (!d->deletedArguments) {
            d->deletedArguments = adoptArrayPtr(new bool[d->numArguments]);
            memset(d->deletedArguments.get(), 0, sizeof(bool) * d->numArguments);
        }
        if (!d->deletedArguments[i]) {
            d->deletedArguments[i] = true;
            return true;
        }
    }

    return JSObject::deleteProperty(exec, Identifier(exec, UString::number(i)));
}

bool Arguments::deleteProperty(ExecState* exec, const Identifier& propertyName) 
{
    bool isArrayIndex;
    unsigned i = propertyName.toArrayIndex(isArrayIndex);
    if (isArrayIndex && i < d->numArguments) {
        if (!d->deletedArguments) {
            d->deletedArguments = adoptArrayPtr(new bool[d->numArguments]);
            memset(d->deletedArguments.get(), 0, sizeof(bool) * d->numArguments);
        }
        if (!d->deletedArguments[i]) {
            d->deletedArguments[i] = true;
            return true;
        }
    }

    if (propertyName == exec->propertyNames().length && !d->overrodeLength) {
        d->overrodeLength = true;
        return true;
    }

    if (propertyName == exec->propertyNames().callee && !d->overrodeCallee) {
        if (!d->isStrictMode) {
            d->overrodeCallee = true;
            return true;
        }
        createStrictModeCalleeIfNecessary(exec);
    }
    
    if (propertyName == exec->propertyNames().caller && !d->isStrictMode)
        createStrictModeCallerIfNecessary(exec);

    return JSObject::deleteProperty(exec, propertyName);
}

} // namespace JSC
