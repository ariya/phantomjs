/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2008 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "config.h"
#include "ObjectConstructor.h"

#include "ButterflyInlines.h"
#include "CopiedSpaceInlines.h"
#include "Error.h"
#include "ExceptionHelpers.h"
#include "JSFunction.h"
#include "JSArray.h"
#include "JSGlobalObject.h"
#include "Lookup.h"
#include "ObjectPrototype.h"
#include "Operations.h"
#include "PropertyDescriptor.h"
#include "PropertyNameArray.h"

namespace JSC {

static EncodedJSValue JSC_HOST_CALL objectConstructorGetPrototypeOf(ExecState*);
static EncodedJSValue JSC_HOST_CALL objectConstructorGetOwnPropertyDescriptor(ExecState*);
static EncodedJSValue JSC_HOST_CALL objectConstructorGetOwnPropertyNames(ExecState*);
static EncodedJSValue JSC_HOST_CALL objectConstructorKeys(ExecState*);
static EncodedJSValue JSC_HOST_CALL objectConstructorDefineProperty(ExecState*);
static EncodedJSValue JSC_HOST_CALL objectConstructorDefineProperties(ExecState*);
static EncodedJSValue JSC_HOST_CALL objectConstructorCreate(ExecState*);
static EncodedJSValue JSC_HOST_CALL objectConstructorSeal(ExecState*);
static EncodedJSValue JSC_HOST_CALL objectConstructorFreeze(ExecState*);
static EncodedJSValue JSC_HOST_CALL objectConstructorPreventExtensions(ExecState*);
static EncodedJSValue JSC_HOST_CALL objectConstructorIsSealed(ExecState*);
static EncodedJSValue JSC_HOST_CALL objectConstructorIsFrozen(ExecState*);
static EncodedJSValue JSC_HOST_CALL objectConstructorIsExtensible(ExecState*);

}

#include "ObjectConstructor.lut.h"

namespace JSC {

ASSERT_HAS_TRIVIAL_DESTRUCTOR(ObjectConstructor);

const ClassInfo ObjectConstructor::s_info = { "Function", &InternalFunction::s_info, 0, ExecState::objectConstructorTable, CREATE_METHOD_TABLE(ObjectConstructor) };

/* Source for ObjectConstructor.lut.h
@begin objectConstructorTable
  getPrototypeOf            objectConstructorGetPrototypeOf             DontEnum|Function 1
  getOwnPropertyDescriptor  objectConstructorGetOwnPropertyDescriptor   DontEnum|Function 2
  getOwnPropertyNames       objectConstructorGetOwnPropertyNames        DontEnum|Function 1
  keys                      objectConstructorKeys                       DontEnum|Function 1
  defineProperty            objectConstructorDefineProperty             DontEnum|Function 3
  defineProperties          objectConstructorDefineProperties           DontEnum|Function 2
  create                    objectConstructorCreate                     DontEnum|Function 2
  seal                      objectConstructorSeal                       DontEnum|Function 1
  freeze                    objectConstructorFreeze                     DontEnum|Function 1
  preventExtensions         objectConstructorPreventExtensions          DontEnum|Function 1
  isSealed                  objectConstructorIsSealed                   DontEnum|Function 1
  isFrozen                  objectConstructorIsFrozen                   DontEnum|Function 1
  isExtensible              objectConstructorIsExtensible               DontEnum|Function 1
@end
*/

ObjectConstructor::ObjectConstructor(JSGlobalObject* globalObject, Structure* structure)
    : InternalFunction(globalObject, structure)
{
}

void ObjectConstructor::finishCreation(ExecState* exec, ObjectPrototype* objectPrototype)
{
    Base::finishCreation(exec->vm(), Identifier(exec, "Object").string());
    // ECMA 15.2.3.1
    putDirectWithoutTransition(exec->vm(), exec->propertyNames().prototype, objectPrototype, DontEnum | DontDelete | ReadOnly);
    // no. of arguments for constructor
    putDirectWithoutTransition(exec->vm(), exec->propertyNames().length, jsNumber(1), ReadOnly | DontEnum | DontDelete);
}

bool ObjectConstructor::getOwnPropertySlot(JSCell* cell, ExecState* exec, PropertyName propertyName, PropertySlot &slot)
{
    return getStaticFunctionSlot<JSObject>(exec, ExecState::objectConstructorTable(exec), jsCast<ObjectConstructor*>(cell), propertyName, slot);
}

bool ObjectConstructor::getOwnPropertyDescriptor(JSObject* object, ExecState* exec, PropertyName propertyName, PropertyDescriptor& descriptor)
{
    return getStaticFunctionDescriptor<JSObject>(exec, ExecState::objectConstructorTable(exec), jsCast<ObjectConstructor*>(object), propertyName, descriptor);
}

static ALWAYS_INLINE JSObject* constructObject(ExecState* exec)
{
    JSGlobalObject* globalObject = exec->callee()->globalObject();
    ArgList args(exec);
    JSValue arg = args.at(0);
    if (arg.isUndefinedOrNull())
        return constructEmptyObject(exec, globalObject->objectPrototype());
    return arg.toObject(exec, globalObject);
}

static EncodedJSValue JSC_HOST_CALL constructWithObjectConstructor(ExecState* exec)
{
    return JSValue::encode(constructObject(exec));
}

ConstructType ObjectConstructor::getConstructData(JSCell*, ConstructData& constructData)
{
    constructData.native.function = constructWithObjectConstructor;
    return ConstructTypeHost;
}

static EncodedJSValue JSC_HOST_CALL callObjectConstructor(ExecState* exec)
{
    return JSValue::encode(constructObject(exec));
}

CallType ObjectConstructor::getCallData(JSCell*, CallData& callData)
{
    callData.native.function = callObjectConstructor;
    return CallTypeHost;
}

EncodedJSValue JSC_HOST_CALL objectConstructorGetPrototypeOf(ExecState* exec)
{
    if (!exec->argument(0).isObject())
        return throwVMError(exec, createTypeError(exec, ASCIILiteral("Requested prototype of a value that is not an object.")));
    JSObject* object = asObject(exec->argument(0));
    if (!object->allowsAccessFrom(exec->trueCallerFrame()))
        return JSValue::encode(jsUndefined());
    return JSValue::encode(object->prototype());
}

EncodedJSValue JSC_HOST_CALL objectConstructorGetOwnPropertyDescriptor(ExecState* exec)
{
    if (!exec->argument(0).isObject())
        return throwVMError(exec, createTypeError(exec, ASCIILiteral("Requested property descriptor of a value that is not an object.")));
    String propertyName = exec->argument(1).toString(exec)->value(exec);
    if (exec->hadException())
        return JSValue::encode(jsNull());
    JSObject* object = asObject(exec->argument(0));
    PropertyDescriptor descriptor;
    if (!object->methodTable()->getOwnPropertyDescriptor(object, exec, Identifier(exec, propertyName), descriptor))
        return JSValue::encode(jsUndefined());
    if (exec->hadException())
        return JSValue::encode(jsUndefined());

    JSObject* description = constructEmptyObject(exec);
    if (!descriptor.isAccessorDescriptor()) {
        description->putDirect(exec->vm(), exec->propertyNames().value, descriptor.value() ? descriptor.value() : jsUndefined(), 0);
        description->putDirect(exec->vm(), exec->propertyNames().writable, jsBoolean(descriptor.writable()), 0);
    } else {
        ASSERT(descriptor.getter());
        ASSERT(descriptor.setter());
        description->putDirect(exec->vm(), exec->propertyNames().get, descriptor.getter(), 0);
        description->putDirect(exec->vm(), exec->propertyNames().set, descriptor.setter(), 0);
    }
    
    description->putDirect(exec->vm(), exec->propertyNames().enumerable, jsBoolean(descriptor.enumerable()), 0);
    description->putDirect(exec->vm(), exec->propertyNames().configurable, jsBoolean(descriptor.configurable()), 0);

    return JSValue::encode(description);
}

// FIXME: Use the enumeration cache.
EncodedJSValue JSC_HOST_CALL objectConstructorGetOwnPropertyNames(ExecState* exec)
{
    if (!exec->argument(0).isObject())
        return throwVMError(exec, createTypeError(exec, ASCIILiteral("Requested property names of a value that is not an object.")));
    PropertyNameArray properties(exec);
    asObject(exec->argument(0))->methodTable()->getOwnPropertyNames(asObject(exec->argument(0)), exec, properties, IncludeDontEnumProperties);
    JSArray* names = constructEmptyArray(exec, 0);
    size_t numProperties = properties.size();
    for (size_t i = 0; i < numProperties; i++)
        names->push(exec, jsOwnedString(exec, properties[i].string()));
    return JSValue::encode(names);
}

// FIXME: Use the enumeration cache.
EncodedJSValue JSC_HOST_CALL objectConstructorKeys(ExecState* exec)
{
    if (!exec->argument(0).isObject())
        return throwVMError(exec, createTypeError(exec, ASCIILiteral("Requested keys of a value that is not an object.")));
    PropertyNameArray properties(exec);
    asObject(exec->argument(0))->methodTable()->getOwnPropertyNames(asObject(exec->argument(0)), exec, properties, ExcludeDontEnumProperties);
    JSArray* keys = constructEmptyArray(exec, 0);
    size_t numProperties = properties.size();
    for (size_t i = 0; i < numProperties; i++)
        keys->push(exec, jsOwnedString(exec, properties[i].string()));
    return JSValue::encode(keys);
}

// ES5 8.10.5 ToPropertyDescriptor
static bool toPropertyDescriptor(ExecState* exec, JSValue in, PropertyDescriptor& desc)
{
    if (!in.isObject()) {
        throwError(exec, createTypeError(exec, ASCIILiteral("Property description must be an object.")));
        return false;
    }
    JSObject* description = asObject(in);

    PropertySlot enumerableSlot(description);
    if (description->getPropertySlot(exec, exec->propertyNames().enumerable, enumerableSlot)) {
        desc.setEnumerable(enumerableSlot.getValue(exec, exec->propertyNames().enumerable).toBoolean(exec));
        if (exec->hadException())
            return false;
    }

    PropertySlot configurableSlot(description);
    if (description->getPropertySlot(exec, exec->propertyNames().configurable, configurableSlot)) {
        desc.setConfigurable(configurableSlot.getValue(exec, exec->propertyNames().configurable).toBoolean(exec));
        if (exec->hadException())
            return false;
    }

    JSValue value;
    PropertySlot valueSlot(description);
    if (description->getPropertySlot(exec, exec->propertyNames().value, valueSlot)) {
        desc.setValue(valueSlot.getValue(exec, exec->propertyNames().value));
        if (exec->hadException())
            return false;
    }

    PropertySlot writableSlot(description);
    if (description->getPropertySlot(exec, exec->propertyNames().writable, writableSlot)) {
        desc.setWritable(writableSlot.getValue(exec, exec->propertyNames().writable).toBoolean(exec));
        if (exec->hadException())
            return false;
    }

    PropertySlot getSlot(description);
    if (description->getPropertySlot(exec, exec->propertyNames().get, getSlot)) {
        JSValue get = getSlot.getValue(exec, exec->propertyNames().get);
        if (exec->hadException())
            return false;
        if (!get.isUndefined()) {
            CallData callData;
            if (getCallData(get, callData) == CallTypeNone) {
                throwError(exec, createTypeError(exec, ASCIILiteral("Getter must be a function.")));
                return false;
            }
        }
        desc.setGetter(get);
    }

    PropertySlot setSlot(description);
    if (description->getPropertySlot(exec, exec->propertyNames().set, setSlot)) {
        JSValue set = setSlot.getValue(exec, exec->propertyNames().set);
        if (exec->hadException())
            return false;
        if (!set.isUndefined()) {
            CallData callData;
            if (getCallData(set, callData) == CallTypeNone) {
                throwError(exec, createTypeError(exec, ASCIILiteral("Setter must be a function.")));
                return false;
            }
        }
        desc.setSetter(set);
    }

    if (!desc.isAccessorDescriptor())
        return true;

    if (desc.value()) {
        throwError(exec, createTypeError(exec, ASCIILiteral("Invalid property.  'value' present on property with getter or setter.")));
        return false;
    }

    if (desc.writablePresent()) {
        throwError(exec, createTypeError(exec, ASCIILiteral("Invalid property.  'writable' present on property with getter or setter.")));
        return false;
    }
    return true;
}

EncodedJSValue JSC_HOST_CALL objectConstructorDefineProperty(ExecState* exec)
{
    if (!exec->argument(0).isObject())
        return throwVMError(exec, createTypeError(exec, ASCIILiteral("Properties can only be defined on Objects.")));
    JSObject* O = asObject(exec->argument(0));
    String propertyName = exec->argument(1).toString(exec)->value(exec);
    if (exec->hadException())
        return JSValue::encode(jsNull());
    PropertyDescriptor descriptor;
    if (!toPropertyDescriptor(exec, exec->argument(2), descriptor))
        return JSValue::encode(jsNull());
    ASSERT((descriptor.attributes() & Accessor) || (!descriptor.isAccessorDescriptor()));
    ASSERT(!exec->hadException());
    O->methodTable()->defineOwnProperty(O, exec, Identifier(exec, propertyName), descriptor, true);
    return JSValue::encode(O);
}

static JSValue defineProperties(ExecState* exec, JSObject* object, JSObject* properties)
{
    PropertyNameArray propertyNames(exec);
    asObject(properties)->methodTable()->getOwnPropertyNames(asObject(properties), exec, propertyNames, ExcludeDontEnumProperties);
    size_t numProperties = propertyNames.size();
    Vector<PropertyDescriptor> descriptors;
    MarkedArgumentBuffer markBuffer;
    for (size_t i = 0; i < numProperties; i++) {
        PropertySlot slot;
        JSValue prop = properties->get(exec, propertyNames[i]);
        if (exec->hadException())
            return jsNull();
        PropertyDescriptor descriptor;
        if (!toPropertyDescriptor(exec, prop, descriptor))
            return jsNull();
        descriptors.append(descriptor);
        // Ensure we mark all the values that we're accumulating
        if (descriptor.isDataDescriptor() && descriptor.value())
            markBuffer.append(descriptor.value());
        if (descriptor.isAccessorDescriptor()) {
            if (descriptor.getter())
                markBuffer.append(descriptor.getter());
            if (descriptor.setter())
                markBuffer.append(descriptor.setter());
        }
    }
    for (size_t i = 0; i < numProperties; i++) {
        object->methodTable()->defineOwnProperty(object, exec, propertyNames[i], descriptors[i], true);
        if (exec->hadException())
            return jsNull();
    }
    return object;
}

EncodedJSValue JSC_HOST_CALL objectConstructorDefineProperties(ExecState* exec)
{
    if (!exec->argument(0).isObject())
        return throwVMError(exec, createTypeError(exec, ASCIILiteral("Properties can only be defined on Objects.")));
    return JSValue::encode(defineProperties(exec, asObject(exec->argument(0)), exec->argument(1).toObject(exec)));
}

EncodedJSValue JSC_HOST_CALL objectConstructorCreate(ExecState* exec)
{
    if (!exec->argument(0).isObject() && !exec->argument(0).isNull())
        return throwVMError(exec, createTypeError(exec, ASCIILiteral("Object prototype may only be an Object or null.")));
    JSValue proto = exec->argument(0);
    JSObject* newObject = proto.isObject()
        ? constructEmptyObject(exec, asObject(proto))
        : constructEmptyObject(exec, exec->lexicalGlobalObject()->nullPrototypeObjectStructure());
    if (exec->argument(1).isUndefined())
        return JSValue::encode(newObject);
    if (!exec->argument(1).isObject())
        return throwVMError(exec, createTypeError(exec, ASCIILiteral("Property descriptor list must be an Object.")));
    return JSValue::encode(defineProperties(exec, newObject, asObject(exec->argument(1))));
}

EncodedJSValue JSC_HOST_CALL objectConstructorSeal(ExecState* exec)
{
    // 1. If Type(O) is not Object throw a TypeError exception.
    JSValue obj = exec->argument(0);
    if (!obj.isObject())
        return throwVMError(exec, createTypeError(exec, ASCIILiteral("Object.seal can only be called on Objects.")));
    JSObject* object = asObject(obj);

    if (isJSFinalObject(object)) {
        object->seal(exec->vm());
        return JSValue::encode(obj);
    }

    // 2. For each named own property name P of O,
    PropertyNameArray properties(exec);
    object->methodTable()->getOwnPropertyNames(object, exec, properties, IncludeDontEnumProperties);
    PropertyNameArray::const_iterator end = properties.end();
    for (PropertyNameArray::const_iterator iter = properties.begin(); iter != end; ++iter) {
        // a. Let desc be the result of calling the [[GetOwnProperty]] internal method of O with P.
        PropertyDescriptor desc;
        if (!object->methodTable()->getOwnPropertyDescriptor(object, exec, *iter, desc))
            continue;
        // b. If desc.[[Configurable]] is true, set desc.[[Configurable]] to false.
        desc.setConfigurable(false);
        // c. Call the [[DefineOwnProperty]] internal method of O with P, desc, and true as arguments.
        object->methodTable()->defineOwnProperty(object, exec, *iter, desc, true);
        if (exec->hadException())
            return JSValue::encode(obj);
    }

    // 3. Set the [[Extensible]] internal property of O to false.
    object->preventExtensions(exec->vm());

    // 4. Return O.
    return JSValue::encode(obj);
}

EncodedJSValue JSC_HOST_CALL objectConstructorFreeze(ExecState* exec)
{
    // 1. If Type(O) is not Object throw a TypeError exception.
    JSValue obj = exec->argument(0);
    if (!obj.isObject())
        return throwVMError(exec, createTypeError(exec, ASCIILiteral("Object.freeze can only be called on Objects.")));
    JSObject* object = asObject(obj);

    if (isJSFinalObject(object) && !hasIndexedProperties(object->structure()->indexingType())) {
        object->freeze(exec->vm());
        return JSValue::encode(obj);
    }

    // 2. For each named own property name P of O,
    PropertyNameArray properties(exec);
    object->methodTable()->getOwnPropertyNames(object, exec, properties, IncludeDontEnumProperties);
    PropertyNameArray::const_iterator end = properties.end();
    for (PropertyNameArray::const_iterator iter = properties.begin(); iter != end; ++iter) {
        // a. Let desc be the result of calling the [[GetOwnProperty]] internal method of O with P.
        PropertyDescriptor desc;
        if (!object->methodTable()->getOwnPropertyDescriptor(object, exec, *iter, desc))
            continue;
        // b. If IsDataDescriptor(desc) is true, then
        // i. If desc.[[Writable]] is true, set desc.[[Writable]] to false.
        if (desc.isDataDescriptor())
            desc.setWritable(false);
        // c. If desc.[[Configurable]] is true, set desc.[[Configurable]] to false.
        desc.setConfigurable(false);
        // d. Call the [[DefineOwnProperty]] internal method of O with P, desc, and true as arguments.
        object->methodTable()->defineOwnProperty(object, exec, *iter, desc, true);
        if (exec->hadException())
            return JSValue::encode(obj);
    }

    // 3. Set the [[Extensible]] internal property of O to false.
    object->preventExtensions(exec->vm());

    // 4. Return O.
    return JSValue::encode(obj);
}

EncodedJSValue JSC_HOST_CALL objectConstructorPreventExtensions(ExecState* exec)
{
    JSValue obj = exec->argument(0);
    if (!obj.isObject())
        return throwVMError(exec, createTypeError(exec, ASCIILiteral("Object.preventExtensions can only be called on Objects.")));
    asObject(obj)->preventExtensions(exec->vm());
    return JSValue::encode(obj);
}

EncodedJSValue JSC_HOST_CALL objectConstructorIsSealed(ExecState* exec)
{
    // 1. If Type(O) is not Object throw a TypeError exception.
    JSValue obj = exec->argument(0);
    if (!obj.isObject())
        return throwVMError(exec, createTypeError(exec, ASCIILiteral("Object.isSealed can only be called on Objects.")));
    JSObject* object = asObject(obj);

    if (isJSFinalObject(object))
        return JSValue::encode(jsBoolean(object->isSealed(exec->vm())));

    // 2. For each named own property name P of O,
    PropertyNameArray properties(exec);
    object->methodTable()->getOwnPropertyNames(object, exec, properties, IncludeDontEnumProperties);
    PropertyNameArray::const_iterator end = properties.end();
    for (PropertyNameArray::const_iterator iter = properties.begin(); iter != end; ++iter) {
        // a. Let desc be the result of calling the [[GetOwnProperty]] internal method of O with P.
        PropertyDescriptor desc;
        if (!object->methodTable()->getOwnPropertyDescriptor(object, exec, *iter, desc))
            continue;
        // b. If desc.[[Configurable]] is true, then return false.
        if (desc.configurable())
            return JSValue::encode(jsBoolean(false));
    }

    // 3. If the [[Extensible]] internal property of O is false, then return true.
    // 4. Otherwise, return false.
    return JSValue::encode(jsBoolean(!object->isExtensible()));
}

EncodedJSValue JSC_HOST_CALL objectConstructorIsFrozen(ExecState* exec)
{
    // 1. If Type(O) is not Object throw a TypeError exception.
    JSValue obj = exec->argument(0);
    if (!obj.isObject())
        return throwVMError(exec, createTypeError(exec, ASCIILiteral("Object.isFrozen can only be called on Objects.")));
    JSObject* object = asObject(obj);

    if (isJSFinalObject(object))
        return JSValue::encode(jsBoolean(object->isFrozen(exec->vm())));

    // 2. For each named own property name P of O,
    PropertyNameArray properties(exec);
    object->methodTable()->getOwnPropertyNames(object, exec, properties, IncludeDontEnumProperties);
    PropertyNameArray::const_iterator end = properties.end();
    for (PropertyNameArray::const_iterator iter = properties.begin(); iter != end; ++iter) {
        // a. Let desc be the result of calling the [[GetOwnProperty]] internal method of O with P.
        PropertyDescriptor desc;
        if (!object->methodTable()->getOwnPropertyDescriptor(object, exec, *iter, desc))
            continue;
        // b. If IsDataDescriptor(desc) is true then
        // i. If desc.[[Writable]] is true, return false. c. If desc.[[Configurable]] is true, then return false.
        if ((desc.isDataDescriptor() && desc.writable()) || desc.configurable())
            return JSValue::encode(jsBoolean(false));
    }

    // 3. If the [[Extensible]] internal property of O is false, then return true.
    // 4. Otherwise, return false.
    return JSValue::encode(jsBoolean(!object->isExtensible()));
}

EncodedJSValue JSC_HOST_CALL objectConstructorIsExtensible(ExecState* exec)
{
    JSValue obj = exec->argument(0);
    if (!obj.isObject())
        return throwVMError(exec, createTypeError(exec, ASCIILiteral("Object.isExtensible can only be called on Objects.")));
    return JSValue::encode(jsBoolean(asObject(obj)->isExtensible()));
}

} // namespace JSC
