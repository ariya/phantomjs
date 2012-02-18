/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009 Apple Inc. All rights reserved.
 *  Copyright (C) 2007 Eric Seidel (eric@webkit.org)
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
#include "JSObject.h"

#include "DatePrototype.h"
#include "ErrorConstructor.h"
#include "GetterSetter.h"
#include "JSFunction.h"
#include "JSGlobalObject.h"
#include "NativeErrorConstructor.h"
#include "ObjectPrototype.h"
#include "PropertyDescriptor.h"
#include "PropertyNameArray.h"
#include "Lookup.h"
#include "Nodes.h"
#include "Operations.h"
#include <math.h>
#include <wtf/Assertions.h>

namespace JSC {

ASSERT_CLASS_FITS_IN_CELL(JSObject);
ASSERT_CLASS_FITS_IN_CELL(JSNonFinalObject);
ASSERT_CLASS_FITS_IN_CELL(JSFinalObject);

const char* StrictModeReadonlyPropertyWriteError = "Attempted to assign to readonly property.";

const ClassInfo JSObject::s_info = { "Object", 0, 0, 0 };

static inline void getClassPropertyNames(ExecState* exec, const ClassInfo* classInfo, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    // Add properties from the static hashtables of properties
    for (; classInfo; classInfo = classInfo->parentClass) {
        const HashTable* table = classInfo->propHashTable(exec);
        if (!table)
            continue;
        table->initializeIfNeeded(exec);
        ASSERT(table->table);

        int hashSizeMask = table->compactSize - 1;
        const HashEntry* entry = table->table;
        for (int i = 0; i <= hashSizeMask; ++i, ++entry) {
            if (entry->key() && (!(entry->attributes() & DontEnum) || (mode == IncludeDontEnumProperties)))
                propertyNames.add(entry->key());
        }
    }
}

void JSObject::visitChildren(SlotVisitor& visitor)
{
#ifndef NDEBUG
    bool wasCheckingForDefaultMarkViolation = visitor.m_isCheckingForDefaultMarkViolation;
    visitor.m_isCheckingForDefaultMarkViolation = false;
#endif

    visitChildrenDirect(visitor);

#ifndef NDEBUG
    visitor.m_isCheckingForDefaultMarkViolation = wasCheckingForDefaultMarkViolation;
#endif
}

UString JSObject::className() const
{
    const ClassInfo* info = classInfo();
    ASSERT(info);
    return info->className;
}

bool JSObject::getOwnPropertySlot(ExecState* exec, unsigned propertyName, PropertySlot& slot)
{
    return getOwnPropertySlot(exec, Identifier::from(exec, propertyName), slot);
}

static void throwSetterError(ExecState* exec)
{
    throwError(exec, createTypeError(exec, "setting a property that has only a getter"));
}

// ECMA 8.6.2.2
void JSObject::put(ExecState* exec, const Identifier& propertyName, JSValue value, PutPropertySlot& slot)
{
    ASSERT(value);
    ASSERT(!Heap::heap(value) || Heap::heap(value) == Heap::heap(this));

    if (propertyName == exec->propertyNames().underscoreProto) {
        // Setting __proto__ to a non-object, non-null value is silently ignored to match Mozilla.
        if (!value.isObject() && !value.isNull())
            return;
        if (!setPrototypeWithCycleCheck(exec->globalData(), value))
            throwError(exec, createError(exec, "cyclic __proto__ value"));
        return;
    }

    // Check if there are any setters or getters in the prototype chain
    JSValue prototype;
    for (JSObject* obj = this; !obj->structure()->hasGetterSetterProperties(); obj = asObject(prototype)) {
        prototype = obj->prototype();
        if (prototype.isNull()) {
            if (!putDirectInternal(exec->globalData(), propertyName, value, 0, true, slot) && slot.isStrictMode())
                throwTypeError(exec, StrictModeReadonlyPropertyWriteError);
            return;
        }
    }
    
    unsigned attributes;
    JSCell* specificValue;
    if ((m_structure->get(exec->globalData(), propertyName, attributes, specificValue) != WTF::notFound) && attributes & ReadOnly) {
        if (slot.isStrictMode())
            throwError(exec, createTypeError(exec, StrictModeReadonlyPropertyWriteError));
        return;
    }

    for (JSObject* obj = this; ; obj = asObject(prototype)) {
        if (JSValue gs = obj->getDirect(exec->globalData(), propertyName)) {
            if (gs.isGetterSetter()) {
                JSObject* setterFunc = asGetterSetter(gs)->setter();        
                if (!setterFunc) {
                    throwSetterError(exec);
                    return;
                }
                
                CallData callData;
                CallType callType = setterFunc->getCallData(callData);
                MarkedArgumentBuffer args;
                args.append(value);
                call(exec, setterFunc, callType, callData, this, args);
                return;
            }

            // If there's an existing property on the object or one of its 
            // prototypes it should be replaced, so break here.
            break;
        }

        prototype = obj->prototype();
        if (prototype.isNull())
            break;
    }

    if (!putDirectInternal(exec->globalData(), propertyName, value, 0, true, slot) && slot.isStrictMode())
        throwTypeError(exec, StrictModeReadonlyPropertyWriteError);
    return;
}

void JSObject::put(ExecState* exec, unsigned propertyName, JSValue value)
{
    PutPropertySlot slot;
    put(exec, Identifier::from(exec, propertyName), value, slot);
}

void JSObject::putWithAttributes(JSGlobalData* globalData, const Identifier& propertyName, JSValue value, unsigned attributes, bool checkReadOnly, PutPropertySlot& slot)
{
    putDirectInternal(*globalData, propertyName, value, attributes, checkReadOnly, slot);
}

void JSObject::putWithAttributes(JSGlobalData* globalData, const Identifier& propertyName, JSValue value, unsigned attributes)
{
    putDirectInternal(*globalData, propertyName, value, attributes);
}

void JSObject::putWithAttributes(JSGlobalData* globalData, unsigned propertyName, JSValue value, unsigned attributes)
{
    putWithAttributes(globalData, Identifier::from(globalData, propertyName), value, attributes);
}

void JSObject::putWithAttributes(ExecState* exec, const Identifier& propertyName, JSValue value, unsigned attributes, bool checkReadOnly, PutPropertySlot& slot)
{
    putDirectInternal(exec->globalData(), propertyName, value, attributes, checkReadOnly, slot);
}

void JSObject::putWithAttributes(ExecState* exec, const Identifier& propertyName, JSValue value, unsigned attributes)
{
    putDirectInternal(exec->globalData(), propertyName, value, attributes);
}

void JSObject::putWithAttributes(ExecState* exec, unsigned propertyName, JSValue value, unsigned attributes)
{
    putWithAttributes(exec, Identifier::from(exec, propertyName), value, attributes);
}

bool JSObject::hasProperty(ExecState* exec, const Identifier& propertyName) const
{
    PropertySlot slot;
    return const_cast<JSObject*>(this)->getPropertySlot(exec, propertyName, slot);
}

bool JSObject::hasProperty(ExecState* exec, unsigned propertyName) const
{
    PropertySlot slot;
    return const_cast<JSObject*>(this)->getPropertySlot(exec, propertyName, slot);
}

// ECMA 8.6.2.5
bool JSObject::deleteProperty(ExecState* exec, const Identifier& propertyName)
{
    unsigned attributes;
    JSCell* specificValue;
    if (m_structure->get(exec->globalData(), propertyName, attributes, specificValue) != WTF::notFound) {
        if ((attributes & DontDelete))
            return false;
        removeDirect(exec->globalData(), propertyName);
        return true;
    }

    // Look in the static hashtable of properties
    const HashEntry* entry = findPropertyHashEntry(exec, propertyName);
    if (entry && entry->attributes() & DontDelete)
        return false; // this builtin property can't be deleted

    // FIXME: Should the code here actually do some deletion?
    return true;
}

bool JSObject::hasOwnProperty(ExecState* exec, const Identifier& propertyName) const
{
    PropertySlot slot;
    return const_cast<JSObject*>(this)->getOwnPropertySlot(exec, propertyName, slot);
}

bool JSObject::deleteProperty(ExecState* exec, unsigned propertyName)
{
    return deleteProperty(exec, Identifier::from(exec, propertyName));
}

static ALWAYS_INLINE JSValue callDefaultValueFunction(ExecState* exec, const JSObject* object, const Identifier& propertyName)
{
    JSValue function = object->get(exec, propertyName);
    CallData callData;
    CallType callType = getCallData(function, callData);
    if (callType == CallTypeNone)
        return exec->exception();

    // Prevent "toString" and "valueOf" from observing execution if an exception
    // is pending.
    if (exec->hadException())
        return exec->exception();

    JSValue result = call(exec, function, callType, callData, const_cast<JSObject*>(object), exec->emptyList());
    ASSERT(!result.isGetterSetter());
    if (exec->hadException())
        return exec->exception();
    if (result.isObject())
        return JSValue();
    return result;
}

bool JSObject::getPrimitiveNumber(ExecState* exec, double& number, JSValue& result)
{
    result = defaultValue(exec, PreferNumber);
    number = result.toNumber(exec);
    return !result.isString();
}

// ECMA 8.6.2.6
JSValue JSObject::defaultValue(ExecState* exec, PreferredPrimitiveType hint) const
{
    // Must call toString first for Date objects.
    if ((hint == PreferString) || (hint != PreferNumber && prototype() == exec->lexicalGlobalObject()->datePrototype())) {
        JSValue value = callDefaultValueFunction(exec, this, exec->propertyNames().toString);
        if (value)
            return value;
        value = callDefaultValueFunction(exec, this, exec->propertyNames().valueOf);
        if (value)
            return value;
    } else {
        JSValue value = callDefaultValueFunction(exec, this, exec->propertyNames().valueOf);
        if (value)
            return value;
        value = callDefaultValueFunction(exec, this, exec->propertyNames().toString);
        if (value)
            return value;
    }

    ASSERT(!exec->hadException());

    return throwError(exec, createTypeError(exec, "No default value"));
}

const HashEntry* JSObject::findPropertyHashEntry(ExecState* exec, const Identifier& propertyName) const
{
    for (const ClassInfo* info = classInfo(); info; info = info->parentClass) {
        if (const HashTable* propHashTable = info->propHashTable(exec)) {
            if (const HashEntry* entry = propHashTable->entry(exec, propertyName))
                return entry;
        }
    }
    return 0;
}

void JSObject::defineGetter(ExecState* exec, const Identifier& propertyName, JSObject* getterFunction, unsigned attributes)
{
    JSValue object = getDirect(exec->globalData(), propertyName);
    if (object && object.isGetterSetter()) {
        ASSERT(m_structure->hasGetterSetterProperties());
        asGetterSetter(object)->setGetter(exec->globalData(), getterFunction);
        return;
    }

    JSGlobalData& globalData = exec->globalData();
    PutPropertySlot slot;
    GetterSetter* getterSetter = new (exec) GetterSetter(exec);
    putDirectInternal(globalData, propertyName, getterSetter, attributes | Getter, true, slot);

    // putDirect will change our Structure if we add a new property. For
    // getters and setters, though, we also need to change our Structure
    // if we override an existing non-getter or non-setter.
    if (slot.type() != PutPropertySlot::NewProperty) {
        if (!m_structure->isDictionary())
            setStructure(exec->globalData(), Structure::getterSetterTransition(globalData, m_structure.get()));
    }

    m_structure->setHasGetterSetterProperties(true);
    getterSetter->setGetter(globalData, getterFunction);
}

void JSObject::defineSetter(ExecState* exec, const Identifier& propertyName, JSObject* setterFunction, unsigned attributes)
{
    JSValue object = getDirect(exec->globalData(), propertyName);
    if (object && object.isGetterSetter()) {
        ASSERT(m_structure->hasGetterSetterProperties());
        asGetterSetter(object)->setSetter(exec->globalData(), setterFunction);
        return;
    }

    PutPropertySlot slot;
    GetterSetter* getterSetter = new (exec) GetterSetter(exec);
    putDirectInternal(exec->globalData(), propertyName, getterSetter, attributes | Setter, true, slot);

    // putDirect will change our Structure if we add a new property. For
    // getters and setters, though, we also need to change our Structure
    // if we override an existing non-getter or non-setter.
    if (slot.type() != PutPropertySlot::NewProperty) {
        if (!m_structure->isDictionary())
            setStructure(exec->globalData(), Structure::getterSetterTransition(exec->globalData(), m_structure.get()));
    }

    m_structure->setHasGetterSetterProperties(true);
    getterSetter->setSetter(exec->globalData(), setterFunction);
}

JSValue JSObject::lookupGetter(ExecState* exec, const Identifier& propertyName)
{
    JSObject* object = this;
    while (true) {
        if (JSValue value = object->getDirect(exec->globalData(), propertyName)) {
            if (!value.isGetterSetter())
                return jsUndefined();
            JSObject* functionObject = asGetterSetter(value)->getter();
            if (!functionObject)
                return jsUndefined();
            return functionObject;
        }

        if (!object->prototype() || !object->prototype().isObject())
            return jsUndefined();
        object = asObject(object->prototype());
    }
}

JSValue JSObject::lookupSetter(ExecState* exec, const Identifier& propertyName)
{
    JSObject* object = this;
    while (true) {
        if (JSValue value = object->getDirect(exec->globalData(), propertyName)) {
            if (!value.isGetterSetter())
                return jsUndefined();
            JSObject* functionObject = asGetterSetter(value)->setter();
            if (!functionObject)
                return jsUndefined();
            return functionObject;
        }

        if (!object->prototype() || !object->prototype().isObject())
            return jsUndefined();
        object = asObject(object->prototype());
    }
}

bool JSObject::hasInstance(ExecState* exec, JSValue value, JSValue proto)
{
    if (!value.isObject())
        return false;

    if (!proto.isObject()) {
        throwError(exec, createTypeError(exec, "instanceof called on an object with an invalid prototype property."));
        return false;
    }

    JSObject* object = asObject(value);
    while ((object = object->prototype().getObject())) {
        if (proto == object)
            return true;
    }
    return false;
}

bool JSObject::propertyIsEnumerable(ExecState* exec, const Identifier& propertyName) const
{
    PropertyDescriptor descriptor;
    if (!const_cast<JSObject*>(this)->getOwnPropertyDescriptor(exec, propertyName, descriptor))
        return false;
    return descriptor.enumerable();
}

bool JSObject::getPropertySpecificValue(ExecState* exec, const Identifier& propertyName, JSCell*& specificValue) const
{
    unsigned attributes;
    if (m_structure->get(exec->globalData(), propertyName, attributes, specificValue) != WTF::notFound)
        return true;

    // This could be a function within the static table? - should probably
    // also look in the hash?  This currently should not be a problem, since
    // we've currently always call 'get' first, which should have populated
    // the normal storage.
    return false;
}

void JSObject::getPropertyNames(ExecState* exec, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    getOwnPropertyNames(exec, propertyNames, mode);

    if (prototype().isNull())
        return;

    JSObject* prototype = asObject(this->prototype());
    while(1) {
        if (prototype->structure()->typeInfo().overridesGetPropertyNames()) {
            prototype->getPropertyNames(exec, propertyNames, mode);
            break;
        }
        prototype->getOwnPropertyNames(exec, propertyNames, mode);
        JSValue nextProto = prototype->prototype();
        if (nextProto.isNull())
            break;
        prototype = asObject(nextProto);
    }
}

void JSObject::getOwnPropertyNames(ExecState* exec, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    m_structure->getPropertyNames(exec->globalData(), propertyNames, mode);
    getClassPropertyNames(exec, classInfo(), propertyNames, mode);
}

bool JSObject::toBoolean(ExecState*) const
{
    return true;
}

double JSObject::toNumber(ExecState* exec) const
{
    JSValue primitive = toPrimitive(exec, PreferNumber);
    if (exec->hadException()) // should be picked up soon in Nodes.cpp
        return 0.0;
    return primitive.toNumber(exec);
}

UString JSObject::toString(ExecState* exec) const
{
    JSValue primitive = toPrimitive(exec, PreferString);
    if (exec->hadException())
        return "";
    return primitive.toString(exec);
}

JSObject* JSObject::toObject(ExecState*, JSGlobalObject*) const
{
    return const_cast<JSObject*>(this);
}

JSObject* JSObject::toThisObject(ExecState*) const
{
    return const_cast<JSObject*>(this);
}

JSValue JSObject::toStrictThisObject(ExecState*) const
{
    return const_cast<JSObject*>(this);
}

JSObject* JSObject::unwrappedObject()
{
    return this;
}

void JSObject::seal(JSGlobalData& globalData)
{
    if (isSealed(globalData))
        return;
    preventExtensions(globalData);
    setStructure(globalData, Structure::sealTransition(globalData, m_structure.get()));
}

void JSObject::freeze(JSGlobalData& globalData)
{
    if (isFrozen(globalData))
        return;
    preventExtensions(globalData);
    setStructure(globalData, Structure::freezeTransition(globalData, m_structure.get()));
}

void JSObject::preventExtensions(JSGlobalData& globalData)
{
    if (isExtensible())
        setStructure(globalData, Structure::preventExtensionsTransition(globalData, m_structure.get()));
}

void JSObject::removeDirect(JSGlobalData& globalData, const Identifier& propertyName)
{
    if (m_structure->get(globalData, propertyName) == WTF::notFound)
        return;

    size_t offset;
    if (m_structure->isUncacheableDictionary()) {
        offset = m_structure->removePropertyWithoutTransition(globalData, propertyName);
        if (offset != WTF::notFound)
            putUndefinedAtDirectOffset(offset);
        return;
    }

    setStructure(globalData, Structure::removePropertyTransition(globalData, m_structure.get(), propertyName, offset));
    if (offset != WTF::notFound)
        putUndefinedAtDirectOffset(offset);
}

void JSObject::putDirectFunction(ExecState* exec, InternalFunction* function, unsigned attr)
{
    putDirectFunction(exec->globalData(), Identifier(exec, function->name(exec)), function, attr);
}

void JSObject::putDirectFunction(ExecState* exec, JSFunction* function, unsigned attr)
{
    putDirectFunction(exec->globalData(), Identifier(exec, function->name(exec)), function, attr);
}

void JSObject::putDirectFunctionWithoutTransition(ExecState* exec, InternalFunction* function, unsigned attr)
{
    putDirectFunctionWithoutTransition(exec->globalData(), Identifier(exec, function->name(exec)), function, attr);
}

void JSObject::putDirectFunctionWithoutTransition(ExecState* exec, JSFunction* function, unsigned attr)
{
    putDirectFunctionWithoutTransition(exec->globalData(), Identifier(exec, function->name(exec)), function, attr);
}

NEVER_INLINE void JSObject::fillGetterPropertySlot(PropertySlot& slot, WriteBarrierBase<Unknown>* location)
{
    if (JSObject* getterFunction = asGetterSetter(location->get())->getter()) {
        if (!structure()->isDictionary())
            slot.setCacheableGetterSlot(this, getterFunction, offsetForLocation(location));
        else
            slot.setGetterSlot(getterFunction);
    } else
        slot.setUndefined();
}

Structure* JSObject::createInheritorID(JSGlobalData& globalData)
{
    m_inheritorID.set(globalData, this, createEmptyObjectStructure(globalData, this));
    ASSERT(m_inheritorID->isEmpty());
    return m_inheritorID.get();
}

void JSObject::allocatePropertyStorage(size_t oldSize, size_t newSize)
{
    ASSERT(newSize > oldSize);

    // It's important that this function not rely on m_structure, since
    // we might be in the middle of a transition.
    bool wasInline = (oldSize < JSObject::baseExternalStorageCapacity);

    PropertyStorage oldPropertyStorage = m_propertyStorage;
    PropertyStorage newPropertyStorage = new WriteBarrierBase<Unknown>[newSize];

    for (unsigned i = 0; i < oldSize; ++i)
       newPropertyStorage[i] = oldPropertyStorage[i];

    if (!wasInline)
        delete [] oldPropertyStorage;

    m_propertyStorage = newPropertyStorage;
}

bool JSObject::getOwnPropertyDescriptor(ExecState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    unsigned attributes = 0;
    JSCell* cell = 0;
    size_t offset = m_structure->get(exec->globalData(), propertyName, attributes, cell);
    if (offset == WTF::notFound)
        return false;
    descriptor.setDescriptor(getDirectOffset(offset), attributes);
    return true;
}

bool JSObject::getPropertyDescriptor(ExecState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor)
{
    JSObject* object = this;
    while (true) {
        if (object->getOwnPropertyDescriptor(exec, propertyName, descriptor))
            return true;
        JSValue prototype = object->prototype();
        if (!prototype.isObject())
            return false;
        object = asObject(prototype);
    }
}

static bool putDescriptor(ExecState* exec, JSObject* target, const Identifier& propertyName, PropertyDescriptor& descriptor, unsigned attributes, const PropertyDescriptor& oldDescriptor)
{
    if (descriptor.isGenericDescriptor() || descriptor.isDataDescriptor()) {
        if (descriptor.isGenericDescriptor() && oldDescriptor.isAccessorDescriptor()) {
            GetterSetter* accessor = new (exec) GetterSetter(exec);
            if (oldDescriptor.getter()) {
                attributes |= Getter;
                accessor->setGetter(exec->globalData(), asObject(oldDescriptor.getter()));
            }
            if (oldDescriptor.setter()) {
                attributes |= Setter;
                accessor->setSetter(exec->globalData(), asObject(oldDescriptor.setter()));
            }
            target->putWithAttributes(exec, propertyName, accessor, attributes);
            return true;
        }
        JSValue newValue = jsUndefined();
        if (descriptor.value())
            newValue = descriptor.value();
        else if (oldDescriptor.value())
            newValue = oldDescriptor.value();
        target->putWithAttributes(exec, propertyName, newValue, attributes & ~(Getter | Setter));
        return true;
    }
    attributes &= ~ReadOnly;
    if (descriptor.getter() && descriptor.getter().isObject())
        target->defineGetter(exec, propertyName, asObject(descriptor.getter()), attributes);
    if (exec->hadException())
        return false;
    if (descriptor.setter() && descriptor.setter().isObject())
        target->defineSetter(exec, propertyName, asObject(descriptor.setter()), attributes);
    return !exec->hadException();
}

bool JSObject::defineOwnProperty(ExecState* exec, const Identifier& propertyName, PropertyDescriptor& descriptor, bool throwException)
{
    // If we have a new property we can just put it on normally
    PropertyDescriptor current;
    if (!getOwnPropertyDescriptor(exec, propertyName, current)) {
        // unless extensions are prevented!
        if (!isExtensible()) {
            if (throwException)
                throwError(exec, createTypeError(exec, "Attempting to define property on object that is not extensible."));
            return false;
        }
        PropertyDescriptor oldDescriptor;
        oldDescriptor.setValue(jsUndefined());
        return putDescriptor(exec, this, propertyName, descriptor, descriptor.attributes(), oldDescriptor);
    }

    if (descriptor.isEmpty())
        return true;

    if (current.equalTo(exec, descriptor))
        return true;

    // Filter out invalid changes
    if (!current.configurable()) {
        if (descriptor.configurable()) {
            if (throwException)
                throwError(exec, createTypeError(exec, "Attempting to configurable attribute of unconfigurable property."));
            return false;
        }
        if (descriptor.enumerablePresent() && descriptor.enumerable() != current.enumerable()) {
            if (throwException)
                throwError(exec, createTypeError(exec, "Attempting to change enumerable attribute of unconfigurable property."));
            return false;
        }
    }

    // A generic descriptor is simply changing the attributes of an existing property
    if (descriptor.isGenericDescriptor()) {
        if (!current.attributesEqual(descriptor)) {
            deleteProperty(exec, propertyName);
            putDescriptor(exec, this, propertyName, descriptor, current.attributesWithOverride(descriptor), current);
        }
        return true;
    }

    // Changing between a normal property or an accessor property
    if (descriptor.isDataDescriptor() != current.isDataDescriptor()) {
        if (!current.configurable()) {
            if (throwException)
                throwError(exec, createTypeError(exec, "Attempting to change access mechanism for an unconfigurable property."));
            return false;
        }
        deleteProperty(exec, propertyName);
        return putDescriptor(exec, this, propertyName, descriptor, current.attributesWithOverride(descriptor), current);
    }

    // Changing the value and attributes of an existing property
    if (descriptor.isDataDescriptor()) {
        if (!current.configurable()) {
            if (!current.writable() && descriptor.writable()) {
                if (throwException)
                    throwError(exec, createTypeError(exec, "Attempting to change writable attribute of unconfigurable property."));
                return false;
            }
            if (!current.writable()) {
                if (descriptor.value() || !JSValue::strictEqual(exec, current.value(), descriptor.value())) {
                    if (throwException)
                        throwError(exec, createTypeError(exec, "Attempting to change value of a readonly property."));
                    return false;
                }
            }
        } else if (current.attributesEqual(descriptor)) {
            if (!descriptor.value())
                return true;
            PutPropertySlot slot;
            put(exec, propertyName, descriptor.value(), slot);
            if (exec->hadException())
                return false;
            return true;
        }
        deleteProperty(exec, propertyName);
        return putDescriptor(exec, this, propertyName, descriptor, current.attributesWithOverride(descriptor), current);
    }

    // Changing the accessor functions of an existing accessor property
    ASSERT(descriptor.isAccessorDescriptor());
    if (!current.configurable()) {
        if (descriptor.setterPresent() && !(current.setterPresent() && JSValue::strictEqual(exec, current.setter(), descriptor.setter()))) {
            if (throwException)
                throwError(exec, createTypeError(exec, "Attempting to change the setter of an unconfigurable property."));
            return false;
        }
        if (descriptor.getterPresent() && !(current.getterPresent() && JSValue::strictEqual(exec, current.getter(), descriptor.getter()))) {
            if (throwException)
                throwError(exec, createTypeError(exec, "Attempting to change the getter of an unconfigurable property."));
            return false;
        }
    }
    JSValue accessor = getDirect(exec->globalData(), propertyName);
    if (!accessor)
        return false;
    GetterSetter* getterSetter = asGetterSetter(accessor);
    if (current.attributesEqual(descriptor)) {
        if (descriptor.setter())
            getterSetter->setSetter(exec->globalData(), asObject(descriptor.setter()));
        if (descriptor.getter())
            getterSetter->setGetter(exec->globalData(), asObject(descriptor.getter()));
        return true;
    }
    deleteProperty(exec, propertyName);
    unsigned attrs = current.attributesWithOverride(descriptor);
    if (descriptor.setter())
        attrs |= Setter;
    if (descriptor.getter())
        attrs |= Getter;
    putDirect(exec->globalData(), propertyName, getterSetter, attrs);
    return true;
}

JSObject* throwTypeError(ExecState* exec, const UString& message)
{
    return throwError(exec, createTypeError(exec, message));
}

} // namespace JSC
