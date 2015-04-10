/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2007, 2008 Apple Inc. All rights reserved.
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
#include "JSCell.h"

#include "JSFunction.h"
#include "JSString.h"
#include "JSObject.h"
#include "NumberObject.h"
#include "Operations.h"
#include <wtf/MathExtras.h>

namespace JSC {

ASSERT_HAS_TRIVIAL_DESTRUCTOR(JSCell);

void JSCell::destroy(JSCell* cell)
{
    cell->JSCell::~JSCell();
}

void JSCell::copyBackingStore(JSCell*, CopyVisitor&)
{
}

bool JSCell::getString(ExecState* exec, String& stringValue) const
{
    if (!isString())
        return false;
    stringValue = static_cast<const JSString*>(this)->value(exec);
    return true;
}

String JSCell::getString(ExecState* exec) const
{
    return isString() ? static_cast<const JSString*>(this)->value(exec) : String();
}

JSObject* JSCell::getObject()
{
    return isObject() ? asObject(this) : 0;
}

const JSObject* JSCell::getObject() const
{
    return isObject() ? static_cast<const JSObject*>(this) : 0;
}

CallType JSCell::getCallData(JSCell*, CallData& callData)
{
    callData.js.functionExecutable = 0;
    callData.js.scope = 0;
    callData.native.function = 0;
    return CallTypeNone;
}

ConstructType JSCell::getConstructData(JSCell*, ConstructData& constructData)
{
    constructData.js.functionExecutable = 0;
    constructData.js.scope = 0;
    constructData.native.function = 0;
    return ConstructTypeNone;
}

bool JSCell::getOwnPropertySlot(JSCell* cell, ExecState* exec, PropertyName identifier, PropertySlot& slot)
{
    // This is not a general purpose implementation of getOwnPropertySlot.
    // It should only be called by JSValue::get.
    // It calls getPropertySlot, not getOwnPropertySlot.
    JSObject* object = cell->toObject(exec, exec->lexicalGlobalObject());
    slot.setBase(object);
    if (!object->getPropertySlot(exec, identifier, slot))
        slot.setUndefined();
    return true;
}

bool JSCell::getOwnPropertySlotByIndex(JSCell* cell, ExecState* exec, unsigned identifier, PropertySlot& slot)
{
    // This is not a general purpose implementation of getOwnPropertySlot.
    // It should only be called by JSValue::get.
    // It calls getPropertySlot, not getOwnPropertySlot.
    JSObject* object = cell->toObject(exec, exec->lexicalGlobalObject());
    slot.setBase(object);
    if (!object->getPropertySlot(exec, identifier, slot))
        slot.setUndefined();
    return true;
}

void JSCell::put(JSCell* cell, ExecState* exec, PropertyName identifier, JSValue value, PutPropertySlot& slot)
{
    if (cell->isString()) {
        JSValue(cell).putToPrimitive(exec, identifier, value, slot);
        return;
    }
    JSObject* thisObject = cell->toObject(exec, exec->lexicalGlobalObject());
    thisObject->methodTable()->put(thisObject, exec, identifier, value, slot);
}

void JSCell::putByIndex(JSCell* cell, ExecState* exec, unsigned identifier, JSValue value, bool shouldThrow)
{
    if (cell->isString()) {
        PutPropertySlot slot(shouldThrow);
        JSValue(cell).putToPrimitive(exec, Identifier::from(exec, identifier), value, slot);
        return;
    }
    JSObject* thisObject = cell->toObject(exec, exec->lexicalGlobalObject());
    thisObject->methodTable()->putByIndex(thisObject, exec, identifier, value, shouldThrow);
}

bool JSCell::deleteProperty(JSCell* cell, ExecState* exec, PropertyName identifier)
{
    JSObject* thisObject = cell->toObject(exec, exec->lexicalGlobalObject());
    return thisObject->methodTable()->deleteProperty(thisObject, exec, identifier);
}

bool JSCell::deletePropertyByIndex(JSCell* cell, ExecState* exec, unsigned identifier)
{
    JSObject* thisObject = cell->toObject(exec, exec->lexicalGlobalObject());
    return thisObject->methodTable()->deletePropertyByIndex(thisObject, exec, identifier);
}

JSObject* JSCell::toThisObject(JSCell* cell, ExecState* exec)
{
    return cell->toObject(exec, exec->lexicalGlobalObject());
}

JSValue JSCell::toPrimitive(ExecState* exec, PreferredPrimitiveType preferredType) const
{
    if (isString())
        return static_cast<const JSString*>(this)->toPrimitive(exec, preferredType);
    return static_cast<const JSObject*>(this)->toPrimitive(exec, preferredType);
}

bool JSCell::getPrimitiveNumber(ExecState* exec, double& number, JSValue& value) const
{
    if (isString())
        return static_cast<const JSString*>(this)->getPrimitiveNumber(exec, number, value);
    return static_cast<const JSObject*>(this)->getPrimitiveNumber(exec, number, value);
}

double JSCell::toNumber(ExecState* exec) const
{ 
    if (isString())
        return static_cast<const JSString*>(this)->toNumber(exec);
    return static_cast<const JSObject*>(this)->toNumber(exec);
}

JSObject* JSCell::toObject(ExecState* exec, JSGlobalObject* globalObject) const
{
    if (isString())
        return static_cast<const JSString*>(this)->toObject(exec, globalObject);
    ASSERT(isObject());
    return jsCast<JSObject*>(const_cast<JSCell*>(this));
}

void slowValidateCell(JSCell* cell)
{
    ASSERT_GC_OBJECT_LOOKS_VALID(cell);
}

JSValue JSCell::defaultValue(const JSObject*, ExecState*, PreferredPrimitiveType)
{
    RELEASE_ASSERT_NOT_REACHED();
    return jsUndefined();
}

void JSCell::getOwnPropertyNames(JSObject*, ExecState*, PropertyNameArray&, EnumerationMode)
{
    RELEASE_ASSERT_NOT_REACHED();
}

void JSCell::getOwnNonIndexPropertyNames(JSObject*, ExecState*, PropertyNameArray&, EnumerationMode)
{
    RELEASE_ASSERT_NOT_REACHED();
}

String JSCell::className(const JSObject*)
{
    RELEASE_ASSERT_NOT_REACHED();
    return String();
}

const char* JSCell::className()
{
    return classInfo()->className;
}

void JSCell::getPropertyNames(JSObject*, ExecState*, PropertyNameArray&, EnumerationMode)
{
    RELEASE_ASSERT_NOT_REACHED();
}

bool JSCell::customHasInstance(JSObject*, ExecState*, JSValue)
{
    RELEASE_ASSERT_NOT_REACHED();
    return false;
}

void JSCell::putDirectVirtual(JSObject*, ExecState*, PropertyName, JSValue, unsigned)
{
    RELEASE_ASSERT_NOT_REACHED();
}

bool JSCell::defineOwnProperty(JSObject*, ExecState*, PropertyName, PropertyDescriptor&, bool)
{
    RELEASE_ASSERT_NOT_REACHED();
    return false;
}

bool JSCell::getOwnPropertyDescriptor(JSObject*, ExecState*, PropertyName, PropertyDescriptor&)
{
    RELEASE_ASSERT_NOT_REACHED();
    return false;
}

} // namespace JSC
