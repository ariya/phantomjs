/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2012 Apple Inc. All rights reserved.
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

#ifndef JSObject_h
#define JSObject_h

#include "ArgList.h"
#include "ArrayConventions.h"
#include "ArrayStorage.h"
#include "Butterfly.h"
#include "ClassInfo.h"
#include "CommonIdentifiers.h"
#include "CallFrame.h"
#include "JSCell.h"
#include "PropertySlot.h"
#include "PropertyStorage.h"
#include "PutDirectIndexMode.h"
#include "PutPropertySlot.h"

#include "Structure.h"
#include "VM.h"
#include "JSString.h"
#include "SlotVisitorInlines.h"
#include "SparseArrayValueMap.h"
#include <wtf/StdLibExtras.h>

namespace JSC {

inline JSCell* getJSFunction(JSValue value)
{
    if (value.isCell() && (value.asCell()->structure()->typeInfo().type() == JSFunctionType))
        return value.asCell();
    return 0;
}

JS_EXPORT_PRIVATE JSCell* getCallableObjectSlow(JSCell*);

inline JSCell* getCallableObject(JSValue value)
{
    if (!value.isCell())
        return 0;
    return getCallableObjectSlow(value.asCell());
}

class GetterSetter;
class HashEntry;
class InternalFunction;
class LLIntOffsetsExtractor;
class MarkedBlock;
class PropertyDescriptor;
class PropertyNameArray;
class Structure;
struct HashTable;

JS_EXPORT_PRIVATE JSObject* throwTypeError(ExecState*, const String&);
extern JS_EXPORTDATA const char* StrictModeReadonlyPropertyWriteError;

// ECMA 262-3 8.6.1
// Property attributes
enum Attribute {
    None         = 0,
    ReadOnly     = 1 << 1,  // property can be only read, not written
    DontEnum     = 1 << 2,  // property doesn't appear in (for .. in ..)
    DontDelete   = 1 << 3,  // property can't be deleted
    Function     = 1 << 4,  // property is a function - only used by static hashtables
    Accessor     = 1 << 5,  // property is a getter/setter
};

COMPILE_ASSERT(None < FirstInternalAttribute, None_is_below_FirstInternalAttribute);
COMPILE_ASSERT(ReadOnly < FirstInternalAttribute, ReadOnly_is_below_FirstInternalAttribute);
COMPILE_ASSERT(DontEnum < FirstInternalAttribute, DontEnum_is_below_FirstInternalAttribute);
COMPILE_ASSERT(DontDelete < FirstInternalAttribute, DontDelete_is_below_FirstInternalAttribute);
COMPILE_ASSERT(Function < FirstInternalAttribute, Function_is_below_FirstInternalAttribute);
COMPILE_ASSERT(Accessor < FirstInternalAttribute, Accessor_is_below_FirstInternalAttribute);

class JSFinalObject;

class JSObject : public JSCell {
    friend class BatchedTransitionOptimizer;
    friend class JIT;
    friend class JSCell;
    friend class JSFinalObject;
    friend class MarkedBlock;
    JS_EXPORT_PRIVATE friend bool setUpStaticFunctionSlot(ExecState*, const HashEntry*, JSObject*, PropertyName, PropertySlot&);

    enum PutMode {
        PutModePut,
        PutModeDefineOwnProperty,
    };

public:
    typedef JSCell Base;
        
    static size_t allocationSize(size_t inlineCapacity)
    {
        return sizeof(JSObject) + inlineCapacity * sizeof(WriteBarrierBase<Unknown>);
    }
        
    JS_EXPORT_PRIVATE static void visitChildren(JSCell*, SlotVisitor&);
    JS_EXPORT_PRIVATE static void copyBackingStore(JSCell*, CopyVisitor&);

    JS_EXPORT_PRIVATE static String className(const JSObject*);

    JSValue prototype() const;
    void setPrototype(VM&, JSValue prototype);
    bool setPrototypeWithCycleCheck(VM&, JSValue prototype);
        
    bool mayInterceptIndexedAccesses()
    {
        return structure()->mayInterceptIndexedAccesses();
    }
        
    JSValue get(ExecState*, PropertyName) const;
    JSValue get(ExecState*, unsigned propertyName) const;

    bool getPropertySlot(ExecState*, PropertyName, PropertySlot&);
    bool getPropertySlot(ExecState*, unsigned propertyName, PropertySlot&);
    JS_EXPORT_PRIVATE bool getPropertyDescriptor(ExecState*, PropertyName, PropertyDescriptor&);

    static bool getOwnPropertySlot(JSCell*, ExecState*, PropertyName, PropertySlot&);
    JS_EXPORT_PRIVATE static bool getOwnPropertySlotByIndex(JSCell*, ExecState*, unsigned propertyName, PropertySlot&);
    JS_EXPORT_PRIVATE static bool getOwnPropertyDescriptor(JSObject*, ExecState*, PropertyName, PropertyDescriptor&);

    bool allowsAccessFrom(ExecState*);

    unsigned getArrayLength() const
    {
        if (!hasIndexedProperties(structure()->indexingType()))
            return 0;
        return m_butterfly->publicLength();
    }
        
    unsigned getVectorLength()
    {
        if (!hasIndexedProperties(structure()->indexingType()))
            return 0;
        return m_butterfly->vectorLength();
    }
        
    JS_EXPORT_PRIVATE static void put(JSCell*, ExecState*, PropertyName, JSValue, PutPropertySlot&);
    JS_EXPORT_PRIVATE static void putByIndex(JSCell*, ExecState*, unsigned propertyName, JSValue, bool shouldThrow);
        
    void putByIndexInline(ExecState* exec, unsigned propertyName, JSValue value, bool shouldThrow)
    {
        if (canSetIndexQuickly(propertyName)) {
            setIndexQuickly(exec->vm(), propertyName, value);
            return;
        }
        methodTable()->putByIndex(this, exec, propertyName, value, shouldThrow);
    }
        
    // This is similar to the putDirect* methods:
    //  - the prototype chain is not consulted
    //  - accessors are not called.
    //  - it will ignore extensibility and read-only properties if PutDirectIndexLikePutDirect is passed as the mode (the default).
    // This method creates a property with attributes writable, enumerable and configurable all set to true.
    bool putDirectIndex(ExecState* exec, unsigned propertyName, JSValue value, unsigned attributes, PutDirectIndexMode mode)
    {
        if (!attributes && canSetIndexQuicklyForPutDirect(propertyName)) {
            setIndexQuickly(exec->vm(), propertyName, value);
            return true;
        }
        return putDirectIndexBeyondVectorLength(exec, propertyName, value, attributes, mode);
    }
    bool putDirectIndex(ExecState* exec, unsigned propertyName, JSValue value)
    {
        return putDirectIndex(exec, propertyName, value, 0, PutDirectIndexLikePutDirect);
    }

    // A non-throwing version of putDirect and putDirectIndex.
    JS_EXPORT_PRIVATE void putDirectMayBeIndex(ExecState*, PropertyName, JSValue);
        
    bool canGetIndexQuickly(unsigned i)
    {
        switch (structure()->indexingType()) {
        case ALL_BLANK_INDEXING_TYPES:
        case ALL_UNDECIDED_INDEXING_TYPES:
            return false;
        case ALL_INT32_INDEXING_TYPES:
        case ALL_CONTIGUOUS_INDEXING_TYPES:
            return i < m_butterfly->vectorLength() && m_butterfly->contiguous()[i];
        case ALL_DOUBLE_INDEXING_TYPES: {
            if (i >= m_butterfly->vectorLength())
                return false;
            double value = m_butterfly->contiguousDouble()[i];
            if (value != value)
                return false;
            return true;
        }
        case ALL_ARRAY_STORAGE_INDEXING_TYPES:
            return i < m_butterfly->arrayStorage()->vectorLength() && m_butterfly->arrayStorage()->m_vector[i];
        default:
            RELEASE_ASSERT_NOT_REACHED();
            return false;
        }
    }
        
    JSValue getIndexQuickly(unsigned i)
    {
        switch (structure()->indexingType()) {
        case ALL_INT32_INDEXING_TYPES:
        case ALL_CONTIGUOUS_INDEXING_TYPES:
            return m_butterfly->contiguous()[i].get();
        case ALL_DOUBLE_INDEXING_TYPES:
            return JSValue(JSValue::EncodeAsDouble, m_butterfly->contiguousDouble()[i]);
        case ALL_ARRAY_STORAGE_INDEXING_TYPES:
            return m_butterfly->arrayStorage()->m_vector[i].get();
        default:
            RELEASE_ASSERT_NOT_REACHED();
            return JSValue();
        }
    }
        
    JSValue tryGetIndexQuickly(unsigned i)
    {
        switch (structure()->indexingType()) {
        case ALL_BLANK_INDEXING_TYPES:
        case ALL_UNDECIDED_INDEXING_TYPES:
            break;
        case ALL_INT32_INDEXING_TYPES:
        case ALL_CONTIGUOUS_INDEXING_TYPES:
            if (i < m_butterfly->publicLength())
                return m_butterfly->contiguous()[i].get();
            break;
        case ALL_DOUBLE_INDEXING_TYPES: {
            if (i >= m_butterfly->publicLength())
                break;
            double result = m_butterfly->contiguousDouble()[i];
            if (result != result)
                break;
            return JSValue(JSValue::EncodeAsDouble, result);
        }
        case ALL_ARRAY_STORAGE_INDEXING_TYPES:
            if (i < m_butterfly->arrayStorage()->vectorLength())
                return m_butterfly->arrayStorage()->m_vector[i].get();
            break;
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }
        return JSValue();
    }
        
    JSValue getDirectIndex(ExecState* exec, unsigned i)
    {
        if (JSValue result = tryGetIndexQuickly(i))
            return result;
        PropertySlot slot(this);
        if (methodTable()->getOwnPropertySlotByIndex(this, exec, i, slot))
            return slot.getValue(exec, i);
        return JSValue();
    }
        
    JSValue getIndex(ExecState* exec, unsigned i)
    {
        if (JSValue result = tryGetIndexQuickly(i))
            return result;
        return get(exec, i);
    }
        
    bool canSetIndexQuickly(unsigned i)
    {
        switch (structure()->indexingType()) {
        case ALL_BLANK_INDEXING_TYPES:
        case ALL_UNDECIDED_INDEXING_TYPES:
            return false;
        case ALL_INT32_INDEXING_TYPES:
        case ALL_DOUBLE_INDEXING_TYPES:
        case ALL_CONTIGUOUS_INDEXING_TYPES:
        case NonArrayWithArrayStorage:
        case ArrayWithArrayStorage:
            return i < m_butterfly->vectorLength();
        case NonArrayWithSlowPutArrayStorage:
        case ArrayWithSlowPutArrayStorage:
            return i < m_butterfly->arrayStorage()->vectorLength()
                && !!m_butterfly->arrayStorage()->m_vector[i];
        default:
            RELEASE_ASSERT_NOT_REACHED();
            return false;
        }
    }
        
    bool canSetIndexQuicklyForPutDirect(unsigned i)
    {
        switch (structure()->indexingType()) {
        case ALL_BLANK_INDEXING_TYPES:
        case ALL_UNDECIDED_INDEXING_TYPES:
            return false;
        case ALL_INT32_INDEXING_TYPES:
        case ALL_DOUBLE_INDEXING_TYPES:
        case ALL_CONTIGUOUS_INDEXING_TYPES:
        case ALL_ARRAY_STORAGE_INDEXING_TYPES:
            return i < m_butterfly->vectorLength();
        default:
            RELEASE_ASSERT_NOT_REACHED();
            return false;
        }
    }
        
    void setIndexQuickly(VM& vm, unsigned i, JSValue v)
    {
        switch (structure()->indexingType()) {
        case ALL_INT32_INDEXING_TYPES: {
            ASSERT(i < m_butterfly->vectorLength());
            if (!v.isInt32()) {
                convertInt32ToDoubleOrContiguousWhilePerformingSetIndex(vm, i, v);
                return;
            }
            // Fall through to contiguous case.
        }
        case ALL_CONTIGUOUS_INDEXING_TYPES: {
            ASSERT(i < m_butterfly->vectorLength());
            m_butterfly->contiguous()[i].set(vm, this, v);
            if (i >= m_butterfly->publicLength())
                m_butterfly->setPublicLength(i + 1);
            break;
        }
        case ALL_DOUBLE_INDEXING_TYPES: {
            ASSERT(i < m_butterfly->vectorLength());
            if (!v.isNumber()) {
                convertDoubleToContiguousWhilePerformingSetIndex(vm, i, v);
                return;
            }
            double value = v.asNumber();
            if (value != value) {
                convertDoubleToContiguousWhilePerformingSetIndex(vm, i, v);
                return;
            }
            m_butterfly->contiguousDouble()[i] = value;
            if (i >= m_butterfly->publicLength())
                m_butterfly->setPublicLength(i + 1);
            break;
        }
        case ALL_ARRAY_STORAGE_INDEXING_TYPES: {
            ArrayStorage* storage = m_butterfly->arrayStorage();
            WriteBarrier<Unknown>& x = storage->m_vector[i];
            JSValue old = x.get();
            x.set(vm, this, v);
            if (!old) {
                ++storage->m_numValuesInVector;
                if (i >= storage->length())
                    storage->setLength(i + 1);
            }
            break;
        }
        default:
            RELEASE_ASSERT_NOT_REACHED();
        }
    }
        
    void initializeIndex(VM& vm, unsigned i, JSValue v)
    {
        switch (structure()->indexingType()) {
        case ALL_UNDECIDED_INDEXING_TYPES: {
            setIndexQuicklyToUndecided(vm, i, v);
            break;
        }
        case ALL_INT32_INDEXING_TYPES: {
            ASSERT(i < m_butterfly->publicLength());
            ASSERT(i < m_butterfly->vectorLength());
            if (!v.isInt32()) {
                convertInt32ToDoubleOrContiguousWhilePerformingSetIndex(vm, i, v);
                break;
            }
            // Fall through.
        }
        case ALL_CONTIGUOUS_INDEXING_TYPES: {
            ASSERT(i < m_butterfly->publicLength());
            ASSERT(i < m_butterfly->vectorLength());
            m_butterfly->contiguous()[i].set(vm, this, v);
            break;
        }
        case ALL_DOUBLE_INDEXING_TYPES: {
            ASSERT(i < m_butterfly->publicLength());
            ASSERT(i < m_butterfly->vectorLength());
            if (!v.isNumber()) {
                convertDoubleToContiguousWhilePerformingSetIndex(vm, i, v);
                return;
            }
            double value = v.asNumber();
            if (value != value) {
                convertDoubleToContiguousWhilePerformingSetIndex(vm, i, v);
                return;
            }
            m_butterfly->contiguousDouble()[i] = value;
            break;
        }
        case ALL_ARRAY_STORAGE_INDEXING_TYPES: {
            ArrayStorage* storage = m_butterfly->arrayStorage();
            ASSERT(i < storage->length());
            ASSERT(i < storage->m_numValuesInVector);
            storage->m_vector[i].set(vm, this, v);
            break;
        }
        default:
            RELEASE_ASSERT_NOT_REACHED();
        }
    }
        
    bool hasSparseMap()
    {
        switch (structure()->indexingType()) {
        case ALL_BLANK_INDEXING_TYPES:
        case ALL_UNDECIDED_INDEXING_TYPES:
        case ALL_INT32_INDEXING_TYPES:
        case ALL_DOUBLE_INDEXING_TYPES:
        case ALL_CONTIGUOUS_INDEXING_TYPES:
            return false;
        case ALL_ARRAY_STORAGE_INDEXING_TYPES:
            return m_butterfly->arrayStorage()->m_sparseMap;
        default:
            RELEASE_ASSERT_NOT_REACHED();
            return false;
        }
    }
        
    bool inSparseIndexingMode()
    {
        switch (structure()->indexingType()) {
        case ALL_BLANK_INDEXING_TYPES:
        case ALL_UNDECIDED_INDEXING_TYPES:
        case ALL_INT32_INDEXING_TYPES:
        case ALL_DOUBLE_INDEXING_TYPES:
        case ALL_CONTIGUOUS_INDEXING_TYPES:
            return false;
        case ALL_ARRAY_STORAGE_INDEXING_TYPES:
            return m_butterfly->arrayStorage()->inSparseMode();
        default:
            RELEASE_ASSERT_NOT_REACHED();
            return false;
        }
    }
        
    void enterDictionaryIndexingMode(VM&);

    // putDirect is effectively an unchecked vesion of 'defineOwnProperty':
    //  - the prototype chain is not consulted
    //  - accessors are not called.
    //  - attributes will be respected (after the call the property will exist with the given attributes)
    //  - the property name is assumed to not be an index.
    JS_EXPORT_PRIVATE static void putDirectVirtual(JSObject*, ExecState*, PropertyName, JSValue, unsigned attributes);
    void putDirect(VM&, PropertyName, JSValue, unsigned attributes = 0);
    void putDirect(VM&, PropertyName, JSValue, PutPropertySlot&);
    void putDirectWithoutTransition(VM&, PropertyName, JSValue, unsigned attributes = 0);
    void putDirectAccessor(ExecState*, PropertyName, JSValue, unsigned attributes);

    bool propertyIsEnumerable(ExecState*, const Identifier& propertyName) const;

    JS_EXPORT_PRIVATE bool hasProperty(ExecState*, PropertyName) const;
    JS_EXPORT_PRIVATE bool hasProperty(ExecState*, unsigned propertyName) const;
    bool hasOwnProperty(ExecState*, PropertyName) const;

    JS_EXPORT_PRIVATE static bool deleteProperty(JSCell*, ExecState*, PropertyName);
    JS_EXPORT_PRIVATE static bool deletePropertyByIndex(JSCell*, ExecState*, unsigned propertyName);

    JS_EXPORT_PRIVATE static JSValue defaultValue(const JSObject*, ExecState*, PreferredPrimitiveType);

    bool hasInstance(ExecState*, JSValue);
    static bool defaultHasInstance(ExecState*, JSValue, JSValue prototypeProperty);

    JS_EXPORT_PRIVATE static void getOwnPropertyNames(JSObject*, ExecState*, PropertyNameArray&, EnumerationMode);
    JS_EXPORT_PRIVATE static void getOwnNonIndexPropertyNames(JSObject*, ExecState*, PropertyNameArray&, EnumerationMode);
    JS_EXPORT_PRIVATE static void getPropertyNames(JSObject*, ExecState*, PropertyNameArray&, EnumerationMode);

    JSValue toPrimitive(ExecState*, PreferredPrimitiveType = NoPreference) const;
    bool getPrimitiveNumber(ExecState*, double& number, JSValue&) const;
    JS_EXPORT_PRIVATE double toNumber(ExecState*) const;
    JS_EXPORT_PRIVATE JSString* toString(ExecState*) const;

    // NOTE: JSObject and its subclasses must be able to gracefully handle ExecState* = 0,
    // because this call may come from inside the compiler.
    JS_EXPORT_PRIVATE static JSObject* toThisObject(JSCell*, ExecState*);

    bool getPropertySpecificValue(ExecState*, PropertyName, JSCell*& specificFunction) const;

    // This get function only looks at the property map.
    JSValue getDirect(VM& vm, PropertyName propertyName) const
    {
        PropertyOffset offset = structure()->get(vm, propertyName);
        checkOffset(offset, structure()->inlineCapacity());
        return offset != invalidOffset ? getDirect(offset) : JSValue();
    }

    PropertyOffset getDirectOffset(VM& vm, PropertyName propertyName)
    {
        PropertyOffset offset = structure()->get(vm, propertyName);
        checkOffset(offset, structure()->inlineCapacity());
        return offset;
    }

    bool hasInlineStorage() const { return structure()->hasInlineStorage(); }
    ConstPropertyStorage inlineStorageUnsafe() const
    {
        return bitwise_cast<ConstPropertyStorage>(this + 1);
    }
    PropertyStorage inlineStorageUnsafe()
    {
        return bitwise_cast<PropertyStorage>(this + 1);
    }
    ConstPropertyStorage inlineStorage() const
    {
        ASSERT(hasInlineStorage());
        return inlineStorageUnsafe();
    }
    PropertyStorage inlineStorage()
    {
        ASSERT(hasInlineStorage());
        return inlineStorageUnsafe();
    }
        
    const Butterfly* butterfly() const { return m_butterfly; }
    Butterfly* butterfly() { return m_butterfly; }
        
    ConstPropertyStorage outOfLineStorage() const { return m_butterfly->propertyStorage(); }
    PropertyStorage outOfLineStorage() { return m_butterfly->propertyStorage(); }

    const WriteBarrierBase<Unknown>* locationForOffset(PropertyOffset offset) const
    {
        if (isInlineOffset(offset))
            return &inlineStorage()[offsetInInlineStorage(offset)];
        return &outOfLineStorage()[offsetInOutOfLineStorage(offset)];
    }

    WriteBarrierBase<Unknown>* locationForOffset(PropertyOffset offset)
    {
        if (isInlineOffset(offset))
            return &inlineStorage()[offsetInInlineStorage(offset)];
        return &outOfLineStorage()[offsetInOutOfLineStorage(offset)];
    }

    void transitionTo(VM&, Structure*);

    bool removeDirect(VM&, PropertyName); // Return true if anything is removed.
    bool hasCustomProperties() { return structure()->didTransition(); }
    bool hasGetterSetterProperties() { return structure()->hasGetterSetterProperties(); }

    // putOwnDataProperty has 'put' like semantics, however this method:
    //  - assumes the object contains no own getter/setter properties.
    //  - provides no special handling for __proto__
    //  - does not walk the prototype chain (to check for accessors or non-writable properties).
    // This is used by JSActivation.
    bool putOwnDataProperty(VM&, PropertyName, JSValue, PutPropertySlot&);

    // Fast access to known property offsets.
    JSValue getDirect(PropertyOffset offset) const { return locationForOffset(offset)->get(); }
    void putDirect(VM& vm, PropertyOffset offset, JSValue value) { locationForOffset(offset)->set(vm, this, value); }
    void putDirectUndefined(PropertyOffset offset) { locationForOffset(offset)->setUndefined(); }

    void putDirectNativeFunction(ExecState*, JSGlobalObject*, const PropertyName&, unsigned functionLength, NativeFunction, Intrinsic, unsigned attributes);

    JS_EXPORT_PRIVATE static bool defineOwnProperty(JSObject*, ExecState*, PropertyName, PropertyDescriptor&, bool shouldThrow);

    bool isGlobalObject() const;
    bool isVariableObject() const;
    bool isStaticScopeObject() const;
    bool isNameScopeObject() const;
    bool isActivationObject() const;
    bool isErrorInstance() const;

    void seal(VM&);
    void freeze(VM&);
    JS_EXPORT_PRIVATE void preventExtensions(VM&);
    bool isSealed(VM& vm) { return structure()->isSealed(vm); }
    bool isFrozen(VM& vm) { return structure()->isFrozen(vm); }
    bool isExtensible() { return structure()->isExtensible(); }
    bool indexingShouldBeSparse()
    {
        return !isExtensible()
            || structure()->typeInfo().interceptsGetOwnPropertySlotByIndexEvenWhenLengthIsNotZero();
    }

    bool staticFunctionsReified() { return structure()->staticFunctionsReified(); }
    void reifyStaticFunctionsForDelete(ExecState* exec);

    JS_EXPORT_PRIVATE Butterfly* growOutOfLineStorage(VM&, size_t oldSize, size_t newSize);
    void setButterfly(VM&, Butterfly*, Structure*);
    void setButterflyWithoutChangingStructure(Butterfly*); // You probably don't want to call this.
        
    void setStructureAndReallocateStorageIfNecessary(VM&, unsigned oldCapacity, Structure*);
    void setStructureAndReallocateStorageIfNecessary(VM&, Structure*);

    void flattenDictionaryObject(VM& vm)
    {
        structure()->flattenDictionaryStructure(vm, this);
    }

    JSGlobalObject* globalObject() const
    {
        ASSERT(structure()->globalObject());
        ASSERT(!isGlobalObject() || ((JSObject*)structure()->globalObject()) == this);
        return structure()->globalObject();
    }
        
    void switchToSlowPutArrayStorage(VM&);
        
    // The receiver is the prototype in this case. The following:
    //
    // asObject(foo->structure()->storedPrototype())->attemptToInterceptPutByIndexOnHoleForPrototype(...)
    //
    // is equivalent to:
    //
    // foo->attemptToInterceptPutByIndexOnHole(...);
    bool attemptToInterceptPutByIndexOnHoleForPrototype(ExecState*, JSValue thisValue, unsigned propertyName, JSValue, bool shouldThrow);
        
    // Returns 0 if int32 storage cannot be created - either because
    // indexing should be sparse, we're having a bad time, or because
    // we already have a more general form of storage (double,
    // contiguous, array storage).
    ContiguousJSValues ensureInt32(VM& vm)
    {
        if (LIKELY(hasInt32(structure()->indexingType())))
            return m_butterfly->contiguousInt32();
            
        return ensureInt32Slow(vm);
    }
        
    // Returns 0 if double storage cannot be created - either because
    // indexing should be sparse, we're having a bad time, or because
    // we already have a more general form of storage (contiguous,
    // or array storage).
    ContiguousDoubles ensureDouble(VM& vm)
    {
        if (LIKELY(hasDouble(structure()->indexingType())))
            return m_butterfly->contiguousDouble();
            
        return ensureDoubleSlow(vm);
    }
        
    // Returns 0 if contiguous storage cannot be created - either because
    // indexing should be sparse or because we're having a bad time.
    ContiguousJSValues ensureContiguous(VM& vm)
    {
        if (LIKELY(hasContiguous(structure()->indexingType())))
            return m_butterfly->contiguous();
            
        return ensureContiguousSlow(vm);
    }
        
    // Same as ensureContiguous(), except that if the indexed storage is in
    // double mode, then it does a rage conversion to contiguous: it
    // attempts to convert each double to an int32.
    ContiguousJSValues rageEnsureContiguous(VM& vm)
    {
        if (LIKELY(hasContiguous(structure()->indexingType())))
            return m_butterfly->contiguous();
            
        return rageEnsureContiguousSlow(vm);
    }
        
    // Ensure that the object is in a mode where it has array storage. Use
    // this if you're about to perform actions that would have required the
    // object to be converted to have array storage, if it didn't have it
    // already.
    ArrayStorage* ensureArrayStorage(VM& vm)
    {
        if (LIKELY(hasArrayStorage(structure()->indexingType())))
            return m_butterfly->arrayStorage();
            
        return ensureArrayStorageSlow(vm);
    }
        
    static size_t offsetOfInlineStorage();
        
    static ptrdiff_t butterflyOffset()
    {
        return OBJECT_OFFSETOF(JSObject, m_butterfly);
    }
        
    void* butterflyAddress()
    {
        return &m_butterfly;
    }

    static JS_EXPORTDATA const ClassInfo s_info;

protected:
    void finishCreation(VM& vm)
    {
        Base::finishCreation(vm);
        ASSERT(inherits(&s_info));
        ASSERT(!structure()->outOfLineCapacity());
        ASSERT(structure()->isEmpty());
        ASSERT(prototype().isNull() || Heap::heap(this) == Heap::heap(prototype()));
        ASSERT(structure()->isObject());
        ASSERT(classInfo());
    }

    static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue prototype)
    {
        return Structure::create(vm, globalObject, prototype, TypeInfo(ObjectType, StructureFlags), &s_info);
    }

    // To instantiate objects you likely want JSFinalObject, below.
    // To create derived types you likely want JSNonFinalObject, below.
    JSObject(VM&, Structure*, Butterfly* = 0);
        
    void visitButterfly(SlotVisitor&, Butterfly*, size_t storageSize);
    void copyButterfly(CopyVisitor&, Butterfly*, size_t storageSize);

    // Call this if you know that the object is in a mode where it has array
    // storage. This will assert otherwise.
    ArrayStorage* arrayStorage()
    {
        ASSERT(hasArrayStorage(structure()->indexingType()));
        return m_butterfly->arrayStorage();
    }
        
    // Call this if you want to predicate some actions on whether or not the
    // object is in a mode where it has array storage.
    ArrayStorage* arrayStorageOrNull()
    {
        switch (structure()->indexingType()) {
        case ALL_ARRAY_STORAGE_INDEXING_TYPES:
            return m_butterfly->arrayStorage();
                
        default:
            return 0;
        }
    }
        
    Butterfly* createInitialUndecided(VM&, unsigned length);
    ContiguousJSValues createInitialInt32(VM&, unsigned length);
    ContiguousDoubles createInitialDouble(VM&, unsigned length);
    ContiguousJSValues createInitialContiguous(VM&, unsigned length);
        
    void convertUndecidedForValue(VM&, JSValue);
    void convertInt32ForValue(VM&, JSValue);
        
    ArrayStorage* createArrayStorage(VM&, unsigned length, unsigned vectorLength);
    ArrayStorage* createInitialArrayStorage(VM&);
        
    ContiguousJSValues convertUndecidedToInt32(VM&);
    ContiguousDoubles convertUndecidedToDouble(VM&);
    ContiguousJSValues convertUndecidedToContiguous(VM&);
    ArrayStorage* convertUndecidedToArrayStorage(VM&, NonPropertyTransition, unsigned neededLength);
    ArrayStorage* convertUndecidedToArrayStorage(VM&, NonPropertyTransition);
    ArrayStorage* convertUndecidedToArrayStorage(VM&);
        
    ContiguousDoubles convertInt32ToDouble(VM&);
    ContiguousJSValues convertInt32ToContiguous(VM&);
    ArrayStorage* convertInt32ToArrayStorage(VM&, NonPropertyTransition, unsigned neededLength);
    ArrayStorage* convertInt32ToArrayStorage(VM&, NonPropertyTransition);
    ArrayStorage* convertInt32ToArrayStorage(VM&);
    
    ContiguousJSValues convertDoubleToContiguous(VM&);
    ContiguousJSValues rageConvertDoubleToContiguous(VM&);
    ArrayStorage* convertDoubleToArrayStorage(VM&, NonPropertyTransition, unsigned neededLength);
    ArrayStorage* convertDoubleToArrayStorage(VM&, NonPropertyTransition);
    ArrayStorage* convertDoubleToArrayStorage(VM&);
        
    ArrayStorage* convertContiguousToArrayStorage(VM&, NonPropertyTransition, unsigned neededLength);
    ArrayStorage* convertContiguousToArrayStorage(VM&, NonPropertyTransition);
    ArrayStorage* convertContiguousToArrayStorage(VM&);

        
    ArrayStorage* ensureArrayStorageExistsAndEnterDictionaryIndexingMode(VM&);
        
    bool defineOwnNonIndexProperty(ExecState*, PropertyName, PropertyDescriptor&, bool throwException);

    template<IndexingType indexingShape>
    void putByIndexBeyondVectorLengthWithoutAttributes(ExecState*, unsigned propertyName, JSValue);
    void putByIndexBeyondVectorLengthWithArrayStorage(ExecState*, unsigned propertyName, JSValue, bool shouldThrow, ArrayStorage*);

    bool increaseVectorLength(VM&, unsigned newLength);
    void deallocateSparseIndexMap();
    bool defineOwnIndexedProperty(ExecState*, unsigned, PropertyDescriptor&, bool throwException);
    SparseArrayValueMap* allocateSparseIndexMap(VM&);
        
    void notifyPresenceOfIndexedAccessors(VM&);
        
    bool attemptToInterceptPutByIndexOnHole(ExecState*, unsigned index, JSValue, bool shouldThrow);
        
    // Call this if you want setIndexQuickly to succeed and you're sure that
    // the array is contiguous.
    void ensureLength(VM& vm, unsigned length)
    {
        ASSERT(length < MAX_ARRAY_INDEX);
        ASSERT(hasContiguous(structure()->indexingType()) || hasInt32(structure()->indexingType()) || hasDouble(structure()->indexingType()) || hasUndecided(structure()->indexingType()));
            
        if (m_butterfly->vectorLength() < length)
            ensureLengthSlow(vm, length);
            
        if (m_butterfly->publicLength() < length)
            m_butterfly->setPublicLength(length);
    }
        
    template<IndexingType indexingShape>
    unsigned countElements(Butterfly*);
        
    // This is relevant to undecided, int32, double, and contiguous.
    unsigned countElements();
        
    // This strange method returns a pointer to the start of the indexed data
    // as if it contained JSValues. But it won't always contain JSValues.
    // Make sure you cast this to the appropriate type before using.
    template<IndexingType indexingType>
    ContiguousJSValues indexingData()
    {
        switch (indexingType) {
        case ALL_INT32_INDEXING_TYPES:
        case ALL_DOUBLE_INDEXING_TYPES:
        case ALL_CONTIGUOUS_INDEXING_TYPES:
            return m_butterfly->contiguous();
                
        case ALL_ARRAY_STORAGE_INDEXING_TYPES:
            return m_butterfly->arrayStorage()->vector();

        default:
            CRASH();
            return ContiguousJSValues();
        }
    }

    ContiguousJSValues currentIndexingData()
    {
        switch (structure()->indexingType()) {
        case ALL_INT32_INDEXING_TYPES:
        case ALL_CONTIGUOUS_INDEXING_TYPES:
            return m_butterfly->contiguous();

        case ALL_ARRAY_STORAGE_INDEXING_TYPES:
            return m_butterfly->arrayStorage()->vector();

        default:
            CRASH();
            return ContiguousJSValues();
        }
    }
        
    JSValue getHolyIndexQuickly(unsigned i)
    {
        ASSERT(i < m_butterfly->vectorLength());
        switch (structure()->indexingType()) {
        case ALL_INT32_INDEXING_TYPES:
        case ALL_CONTIGUOUS_INDEXING_TYPES:
            return m_butterfly->contiguous()[i].get();
        case ALL_DOUBLE_INDEXING_TYPES: {
            double value = m_butterfly->contiguousDouble()[i];
            if (value == value)
                return JSValue(JSValue::EncodeAsDouble, value);
            return JSValue();
        }
        case ALL_ARRAY_STORAGE_INDEXING_TYPES:
            return m_butterfly->arrayStorage()->m_vector[i].get();
        default:
            CRASH();
            return JSValue();
        }
    }
        
    template<IndexingType indexingType>
    unsigned relevantLength()
    {
        switch (indexingType) {
        case ALL_INT32_INDEXING_TYPES:
        case ALL_DOUBLE_INDEXING_TYPES:
        case ALL_CONTIGUOUS_INDEXING_TYPES:
            return m_butterfly->publicLength();
                
        case ALL_ARRAY_STORAGE_INDEXING_TYPES:
            return std::min(
                m_butterfly->arrayStorage()->length(),
                m_butterfly->arrayStorage()->vectorLength());
                
        default:
            CRASH();
            return 0;
        }
    }

    unsigned currentRelevantLength()
    {
        switch (structure()->indexingType()) {
        case ALL_INT32_INDEXING_TYPES:
        case ALL_DOUBLE_INDEXING_TYPES:
        case ALL_CONTIGUOUS_INDEXING_TYPES:
            return m_butterfly->publicLength();

        case ALL_ARRAY_STORAGE_INDEXING_TYPES:
            return std::min(
                m_butterfly->arrayStorage()->length(),
                m_butterfly->arrayStorage()->vectorLength());

        default:
            CRASH();
            return 0;
        }
    }

private:
    friend class LLIntOffsetsExtractor;
        
    // Nobody should ever ask any of these questions on something already known to be a JSObject.
    using JSCell::isAPIValueWrapper;
    using JSCell::isGetterSetter;
    void getObject();
    void getString(ExecState* exec);
    void isObject();
    void isString();
        
    Butterfly* createInitialIndexedStorage(VM&, unsigned length, size_t elementSize);
        
    ArrayStorage* enterDictionaryIndexingModeWhenArrayStorageAlreadyExists(VM&, ArrayStorage*);
        
    template<PutMode>
    bool putDirectInternal(VM&, PropertyName, JSValue, unsigned attr, PutPropertySlot&, JSCell*);

    bool inlineGetOwnPropertySlot(ExecState*, PropertyName, PropertySlot&);
    JS_EXPORT_PRIVATE void fillGetterPropertySlot(PropertySlot&, PropertyOffset);

    const HashEntry* findPropertyHashEntry(ExecState*, PropertyName) const;
        
    void putIndexedDescriptor(ExecState*, SparseArrayEntry*, PropertyDescriptor&, PropertyDescriptor& old);
        
    void putByIndexBeyondVectorLength(ExecState*, unsigned propertyName, JSValue, bool shouldThrow);
    bool putDirectIndexBeyondVectorLengthWithArrayStorage(ExecState*, unsigned propertyName, JSValue, unsigned attributes, PutDirectIndexMode, ArrayStorage*);
    JS_EXPORT_PRIVATE bool putDirectIndexBeyondVectorLength(ExecState*, unsigned propertyName, JSValue, unsigned attributes, PutDirectIndexMode);
        
    unsigned getNewVectorLength(unsigned currentVectorLength, unsigned currentLength, unsigned desiredLength);
    unsigned getNewVectorLength(unsigned desiredLength);

    JS_EXPORT_PRIVATE bool getOwnPropertySlotSlow(ExecState*, PropertyName, PropertySlot&);
        
    ArrayStorage* constructConvertedArrayStorageWithoutCopyingElements(VM&, unsigned neededLength);
        
    JS_EXPORT_PRIVATE void setIndexQuicklyToUndecided(VM&, unsigned index, JSValue);
    JS_EXPORT_PRIVATE void convertInt32ToDoubleOrContiguousWhilePerformingSetIndex(VM&, unsigned index, JSValue);
    JS_EXPORT_PRIVATE void convertDoubleToContiguousWhilePerformingSetIndex(VM&, unsigned index, JSValue);
        
    void ensureLengthSlow(VM&, unsigned length);
        
    ContiguousJSValues ensureInt32Slow(VM&);
    ContiguousDoubles ensureDoubleSlow(VM&);
    ContiguousJSValues ensureContiguousSlow(VM&);
    ContiguousJSValues rageEnsureContiguousSlow(VM&);
    ArrayStorage* ensureArrayStorageSlow(VM&);
    
    enum DoubleToContiguousMode { EncodeValueAsDouble, RageConvertDoubleToValue };
    template<DoubleToContiguousMode mode>
    ContiguousJSValues genericConvertDoubleToContiguous(VM&);
    ContiguousJSValues ensureContiguousSlow(VM&, DoubleToContiguousMode);
    
protected:
    Butterfly* m_butterfly;
};

// JSNonFinalObject is a type of JSObject that has some internal storage,
// but also preserves some space in the collector cell for additional
// data members in derived types.
class JSNonFinalObject : public JSObject {
    friend class JSObject;

public:
    typedef JSObject Base;

    static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue prototype)
    {
        return Structure::create(vm, globalObject, prototype, TypeInfo(ObjectType, StructureFlags), &s_info);
    }

protected:
    explicit JSNonFinalObject(VM& vm, Structure* structure, Butterfly* butterfly = 0)
        : JSObject(vm, structure, butterfly)
    {
    }

    void finishCreation(VM& vm)
    {
        Base::finishCreation(vm);
        ASSERT(!this->structure()->totalStorageCapacity());
        ASSERT(classInfo());
    }
};

class JSFinalObject;

// JSFinalObject is a type of JSObject that contains sufficent internal
// storage to fully make use of the colloctor cell containing it.
class JSFinalObject : public JSObject {
    friend class JSObject;

public:
    typedef JSObject Base;

    static const unsigned defaultSize = 64;
    static inline unsigned defaultInlineCapacity()
    {
        return (defaultSize - allocationSize(0)) / sizeof(WriteBarrier<Unknown>);
    }

    static const unsigned maxSize = 512;
    static inline unsigned maxInlineCapacity()
    {
        return (maxSize - allocationSize(0)) / sizeof(WriteBarrier<Unknown>);
    }

    static JSFinalObject* create(ExecState*, Structure*);
    static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue prototype, unsigned inlineCapacity)
    {
        return Structure::create(vm, globalObject, prototype, TypeInfo(FinalObjectType, StructureFlags), &s_info, NonArray, inlineCapacity);
    }

    JS_EXPORT_PRIVATE static void visitChildren(JSCell*, SlotVisitor&);

    static JS_EXPORTDATA const ClassInfo s_info;

protected:
    void visitChildrenCommon(SlotVisitor&);
        
    void finishCreation(VM& vm)
    {
        Base::finishCreation(vm);
        ASSERT(structure()->totalStorageCapacity() == structure()->inlineCapacity());
        ASSERT(classInfo());
    }

private:
    friend class LLIntOffsetsExtractor;

    explicit JSFinalObject(VM& vm, Structure* structure)
        : JSObject(vm, structure)
    {
    }

    static const unsigned StructureFlags = JSObject::StructureFlags;
};

inline JSFinalObject* JSFinalObject::create(ExecState* exec, Structure* structure)
{
    JSFinalObject* finalObject = new (
        NotNull, 
        allocateCell<JSFinalObject>(
            *exec->heap(),
            allocationSize(structure->inlineCapacity())
        )
    ) JSFinalObject(exec->vm(), structure);
    finalObject->finishCreation(exec->vm());
    return finalObject;
}

inline bool isJSFinalObject(JSCell* cell)
{
    return cell->classInfo() == &JSFinalObject::s_info;
}

inline bool isJSFinalObject(JSValue value)
{
    return value.isCell() && isJSFinalObject(value.asCell());
}

inline size_t JSObject::offsetOfInlineStorage()
{
    return sizeof(JSObject);
}

inline bool JSObject::isGlobalObject() const
{
    return structure()->typeInfo().type() == GlobalObjectType;
}

inline bool JSObject::isVariableObject() const
{
    return structure()->typeInfo().type() >= VariableObjectType;
}


inline bool JSObject::isStaticScopeObject() const
{
    JSType type = structure()->typeInfo().type();
    return type == NameScopeObjectType || type == ActivationObjectType;
}


inline bool JSObject::isNameScopeObject() const
{
    return structure()->typeInfo().type() == NameScopeObjectType;
}

inline bool JSObject::isActivationObject() const
{
    return structure()->typeInfo().type() == ActivationObjectType;
}

inline bool JSObject::isErrorInstance() const
{
    return structure()->typeInfo().type() == ErrorInstanceType;
}

inline void JSObject::setButterfly(VM& vm, Butterfly* butterfly, Structure* structure)
{
    ASSERT(structure);
    ASSERT(!butterfly == (!structure->outOfLineCapacity() && !hasIndexingHeader(structure->indexingType())));
    setStructure(vm, structure);
    m_butterfly = butterfly;
}

inline void JSObject::setButterflyWithoutChangingStructure(Butterfly* butterfly)
{
    m_butterfly = butterfly;
}

inline CallType getCallData(JSValue value, CallData& callData)
{
    CallType result = value.isCell() ? value.asCell()->methodTable()->getCallData(value.asCell(), callData) : CallTypeNone;
    ASSERT(result == CallTypeNone || value.isValidCallee());
    return result;
}

inline ConstructType getConstructData(JSValue value, ConstructData& constructData)
{
    ConstructType result = value.isCell() ? value.asCell()->methodTable()->getConstructData(value.asCell(), constructData) : ConstructTypeNone;
    ASSERT(result == ConstructTypeNone || value.isValidCallee());
    return result;
}

inline JSObject* asObject(JSCell* cell)
{
    ASSERT(cell->isObject());
    return jsCast<JSObject*>(cell);
}

inline JSObject* asObject(JSValue value)
{
    return asObject(value.asCell());
}

inline JSObject::JSObject(VM& vm, Structure* structure, Butterfly* butterfly)
    : JSCell(vm, structure)
    , m_butterfly(butterfly)
{
}

inline JSValue JSObject::prototype() const
{
    return structure()->storedPrototype();
}

ALWAYS_INLINE bool JSObject::inlineGetOwnPropertySlot(ExecState* exec, PropertyName propertyName, PropertySlot& slot)
{
    PropertyOffset offset = structure()->get(exec->vm(), propertyName);
    if (LIKELY(isValidOffset(offset))) {
        JSValue value = getDirect(offset);
        if (structure()->hasGetterSetterProperties() && value.isGetterSetter())
            fillGetterPropertySlot(slot, offset);
        else
            slot.setValue(this, value, offset);
        return true;
    }

    return getOwnPropertySlotSlow(exec, propertyName, slot);
}

// It may seem crazy to inline a function this large, especially a virtual function,
// but it makes a big difference to property lookup that derived classes can inline their
// base class call to this.
ALWAYS_INLINE bool JSObject::getOwnPropertySlot(JSCell* cell, ExecState* exec, PropertyName propertyName, PropertySlot& slot)
{
    return jsCast<JSObject*>(cell)->inlineGetOwnPropertySlot(exec, propertyName, slot);
}

// It may seem crazy to inline a function this large but it makes a big difference
// since this is function very hot in variable lookup
ALWAYS_INLINE bool JSObject::getPropertySlot(ExecState* exec, PropertyName propertyName, PropertySlot& slot)
{
    JSObject* object = this;
    while (true) {
        if (object->fastGetOwnPropertySlot(exec, propertyName, slot))
            return true;
        JSValue prototype = object->prototype();
        if (!prototype.isObject())
            return false;
        object = asObject(prototype);
    }
}

ALWAYS_INLINE bool JSObject::getPropertySlot(ExecState* exec, unsigned propertyName, PropertySlot& slot)
{
    JSObject* object = this;
    while (true) {
        if (object->methodTable()->getOwnPropertySlotByIndex(object, exec, propertyName, slot))
            return true;
        JSValue prototype = object->prototype();
        if (!prototype.isObject())
            return false;
        object = asObject(prototype);
    }
}

inline JSValue JSObject::get(ExecState* exec, PropertyName propertyName) const
{
    PropertySlot slot(this);
    if (const_cast<JSObject*>(this)->getPropertySlot(exec, propertyName, slot))
        return slot.getValue(exec, propertyName);
    
    return jsUndefined();
}

inline JSValue JSObject::get(ExecState* exec, unsigned propertyName) const
{
    PropertySlot slot(this);
    if (const_cast<JSObject*>(this)->getPropertySlot(exec, propertyName, slot))
        return slot.getValue(exec, propertyName);

    return jsUndefined();
}

template<JSObject::PutMode mode>
inline bool JSObject::putDirectInternal(VM& vm, PropertyName propertyName, JSValue value, unsigned attributes, PutPropertySlot& slot, JSCell* specificFunction)
{
    ASSERT(value);
    ASSERT(value.isGetterSetter() == !!(attributes & Accessor));
    ASSERT(!Heap::heap(value) || Heap::heap(value) == Heap::heap(this));
    ASSERT(propertyName.asIndex() == PropertyName::NotAnIndex);

    if (structure()->isDictionary()) {
        unsigned currentAttributes;
        JSCell* currentSpecificFunction;
        PropertyOffset offset = structure()->get(vm, propertyName, currentAttributes, currentSpecificFunction);
        if (offset != invalidOffset) {
            // If there is currently a specific function, and there now either isn't,
            // or the new value is different, then despecify.
            if (currentSpecificFunction && (specificFunction != currentSpecificFunction))
                structure()->despecifyDictionaryFunction(vm, propertyName);
            if ((mode == PutModePut) && currentAttributes & ReadOnly)
                return false;

            putDirect(vm, offset, value);
            // At this point, the objects structure only has a specific value set if previously there
            // had been one set, and if the new value being specified is the same (otherwise we would
            // have despecified, above).  So, if currentSpecificFunction is not set, or if the new
            // value is different (or there is no new value), then the slot now has no value - and
            // as such it is cachable.
            // If there was previously a value, and the new value is the same, then we cannot cache.
            if (!currentSpecificFunction || (specificFunction != currentSpecificFunction))
                slot.setExistingProperty(this, offset);
            return true;
        }

        if ((mode == PutModePut) && !isExtensible())
            return false;

        Butterfly* newButterfly = m_butterfly;
        if (structure()->putWillGrowOutOfLineStorage())
            newButterfly = growOutOfLineStorage(vm, structure()->outOfLineCapacity(), structure()->suggestedNewOutOfLineStorageCapacity());
        offset = structure()->addPropertyWithoutTransition(vm, propertyName, attributes, specificFunction);
        setButterfly(vm, newButterfly, structure());

        validateOffset(offset);
        ASSERT(structure()->isValidOffset(offset));
        putDirect(vm, offset, value);
        // See comment on setNewProperty call below.
        if (!specificFunction)
            slot.setNewProperty(this, offset);
        if (attributes & ReadOnly)
            structure()->setContainsReadOnlyProperties();
        return true;
    }

    PropertyOffset offset;
    size_t currentCapacity = structure()->outOfLineCapacity();
    if (Structure* structure = Structure::addPropertyTransitionToExistingStructure(this->structure(), propertyName, attributes, specificFunction, offset)) {
        Butterfly* newButterfly = m_butterfly;
        if (currentCapacity != structure->outOfLineCapacity())
            newButterfly = growOutOfLineStorage(vm, currentCapacity, structure->outOfLineCapacity());

        validateOffset(offset);
        ASSERT(structure->isValidOffset(offset));
        setButterfly(vm, newButterfly, structure);
        putDirect(vm, offset, value);
        // This is a new property; transitions with specific values are not currently cachable,
        // so leave the slot in an uncachable state.
        if (!specificFunction)
            slot.setNewProperty(this, offset);
        return true;
    }

    unsigned currentAttributes;
    JSCell* currentSpecificFunction;
    offset = structure()->get(vm, propertyName, currentAttributes, currentSpecificFunction);
    if (offset != invalidOffset) {
        if ((mode == PutModePut) && currentAttributes & ReadOnly)
            return false;

        // There are three possibilities here:
        //  (1) There is an existing specific value set, and we're overwriting with *the same value*.
        //       * Do nothing - no need to despecify, but that means we can't cache (a cached
        //         put could write a different value). Leave the slot in an uncachable state.
        //  (2) There is a specific value currently set, but we're writing a different value.
        //       * First, we have to despecify.  Having done so, this is now a regular slot
        //         with no specific value, so go ahead & cache like normal.
        //  (3) Normal case, there is no specific value set.
        //       * Go ahead & cache like normal.
        if (currentSpecificFunction) {
            // case (1) Do the put, then return leaving the slot uncachable.
            if (specificFunction == currentSpecificFunction) {
                putDirect(vm, offset, value);
                return true;
            }
            // case (2) Despecify, fall through to (3).
            setStructure(vm, Structure::despecifyFunctionTransition(vm, structure(), propertyName));
        }

        // case (3) set the slot, do the put, return.
        slot.setExistingProperty(this, offset);
        putDirect(vm, offset, value);
        return true;
    }

    if ((mode == PutModePut) && !isExtensible())
        return false;

    Structure* structure = Structure::addPropertyTransition(vm, this->structure(), propertyName, attributes, specificFunction, offset);
    
    validateOffset(offset);
    ASSERT(structure->isValidOffset(offset));
    setStructureAndReallocateStorageIfNecessary(vm, structure);

    putDirect(vm, offset, value);
    // This is a new property; transitions with specific values are not currently cachable,
    // so leave the slot in an uncachable state.
    if (!specificFunction)
        slot.setNewProperty(this, offset);
    if (attributes & ReadOnly)
        structure->setContainsReadOnlyProperties();
    return true;
}

inline void JSObject::setStructureAndReallocateStorageIfNecessary(VM& vm, unsigned oldCapacity, Structure* newStructure)
{
    ASSERT(oldCapacity <= newStructure->outOfLineCapacity());
    
    if (oldCapacity == newStructure->outOfLineCapacity()) {
        setStructure(vm, newStructure);
        return;
    }
    
    Butterfly* newButterfly = growOutOfLineStorage(
        vm, oldCapacity, newStructure->outOfLineCapacity());
    setButterfly(vm, newButterfly, newStructure);
}

inline void JSObject::setStructureAndReallocateStorageIfNecessary(VM& vm, Structure* newStructure)
{
    setStructureAndReallocateStorageIfNecessary(
        vm, structure()->outOfLineCapacity(), newStructure);
}

inline bool JSObject::putOwnDataProperty(VM& vm, PropertyName propertyName, JSValue value, PutPropertySlot& slot)
{
    ASSERT(value);
    ASSERT(!Heap::heap(value) || Heap::heap(value) == Heap::heap(this));
    ASSERT(!structure()->hasGetterSetterProperties());

    return putDirectInternal<PutModePut>(vm, propertyName, value, 0, slot, getCallableObject(value));
}

inline void JSObject::putDirect(VM& vm, PropertyName propertyName, JSValue value, unsigned attributes)
{
    ASSERT(!value.isGetterSetter() && !(attributes & Accessor));
    PutPropertySlot slot;
    putDirectInternal<PutModeDefineOwnProperty>(vm, propertyName, value, attributes, slot, getCallableObject(value));
}

inline void JSObject::putDirect(VM& vm, PropertyName propertyName, JSValue value, PutPropertySlot& slot)
{
    ASSERT(!value.isGetterSetter());
    putDirectInternal<PutModeDefineOwnProperty>(vm, propertyName, value, 0, slot, getCallableObject(value));
}

inline void JSObject::putDirectWithoutTransition(VM& vm, PropertyName propertyName, JSValue value, unsigned attributes)
{
    ASSERT(!value.isGetterSetter() && !(attributes & Accessor));
    Butterfly* newButterfly = m_butterfly;
    if (structure()->putWillGrowOutOfLineStorage())
        newButterfly = growOutOfLineStorage(vm, structure()->outOfLineCapacity(), structure()->suggestedNewOutOfLineStorageCapacity());
    PropertyOffset offset = structure()->addPropertyWithoutTransition(vm, propertyName, attributes, getCallableObject(value));
    setButterfly(vm, newButterfly, structure());
    putDirect(vm, offset, value);
}

inline JSValue JSObject::toPrimitive(ExecState* exec, PreferredPrimitiveType preferredType) const
{
    return methodTable()->defaultValue(this, exec, preferredType);
}

ALWAYS_INLINE JSObject* Register::function() const
{
    if (!jsValue())
        return 0;
    return asObject(jsValue());
}

ALWAYS_INLINE Register Register::withCallee(JSObject* callee)
{
    Register r;
    r = JSValue(callee);
    return r;
}

inline size_t offsetInButterfly(PropertyOffset offset)
{
    return offsetInOutOfLineStorage(offset) + Butterfly::indexOfPropertyStorage();
}

// Helpers for patching code where you want to emit a load or store and
// the base is:
// For inline offsets: a pointer to the out-of-line storage pointer.
// For out-of-line offsets: the base of the out-of-line storage.
inline size_t offsetRelativeToPatchedStorage(PropertyOffset offset)
{
    if (isOutOfLineOffset(offset))
        return sizeof(EncodedJSValue) * offsetInButterfly(offset);
    return JSObject::offsetOfInlineStorage() - JSObject::butterflyOffset() + sizeof(EncodedJSValue) * offsetInInlineStorage(offset);
}

// Returns the maximum offset (away from zero) a load instruction will encode.
inline size_t maxOffsetRelativeToPatchedStorage(PropertyOffset offset)
{
    ptrdiff_t addressOffset = static_cast<ptrdiff_t>(offsetRelativeToPatchedStorage(offset));
#if USE(JSVALUE32_64)
    if (addressOffset >= 0)
        return static_cast<size_t>(addressOffset) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag);
#endif
    return static_cast<size_t>(addressOffset);
}

inline int indexRelativeToBase(PropertyOffset offset)
{
    if (isOutOfLineOffset(offset))
        return offsetInOutOfLineStorage(offset) + Butterfly::indexOfPropertyStorage();
    ASSERT(!(JSObject::offsetOfInlineStorage() % sizeof(EncodedJSValue)));
    return JSObject::offsetOfInlineStorage() / sizeof(EncodedJSValue) + offsetInInlineStorage(offset);
}

inline int offsetRelativeToBase(PropertyOffset offset)
{
    if (isOutOfLineOffset(offset))
        return offsetInOutOfLineStorage(offset) * sizeof(EncodedJSValue) + Butterfly::offsetOfPropertyStorage();
    return JSObject::offsetOfInlineStorage() + offsetInInlineStorage(offset) * sizeof(EncodedJSValue);
}

COMPILE_ASSERT(!(sizeof(JSObject) % sizeof(WriteBarrierBase<Unknown>)), JSObject_inline_storage_has_correct_alignment);

ALWAYS_INLINE Identifier makeIdentifier(ExecState* exec, const char* name)
{
    return Identifier(exec, name);
}

ALWAYS_INLINE Identifier makeIdentifier(ExecState*, const Identifier& name)
{
    return name;
}

// Helper for defining native functions, if you're not using a static hash table.
// Use this macro from within finishCreation() methods in prototypes. This assumes
// you've defined variables called exec, globalObject, and vm, and they
// have the expected meanings.
#define JSC_NATIVE_INTRINSIC_FUNCTION(jsName, cppName, attributes, length, intrinsic) \
    putDirectNativeFunction(\
        exec, globalObject, makeIdentifier(exec, (jsName)), (length), cppName, \
        (intrinsic), (attributes))

// As above, but this assumes that the function you're defining doesn't have an
// intrinsic.
#define JSC_NATIVE_FUNCTION(jsName, cppName, attributes, length) \
    JSC_NATIVE_INTRINSIC_FUNCTION(jsName, cppName, (attributes), (length), NoIntrinsic)

} // namespace JSC

#endif // JSObject_h
