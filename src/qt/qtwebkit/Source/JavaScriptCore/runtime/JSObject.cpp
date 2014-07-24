/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2008, 2009, 2012, 2013 Apple Inc. All rights reserved.
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

#include "ButterflyInlines.h"
#include "CopiedSpaceInlines.h"
#include "CopyVisitor.h"
#include "CopyVisitorInlines.h"
#include "DatePrototype.h"
#include "ErrorConstructor.h"
#include "Executable.h"
#include "GetterSetter.h"
#include "IndexingHeaderInlines.h"
#include "JSFunction.h"
#include "JSGlobalObject.h"
#include "Lookup.h"
#include "NativeErrorConstructor.h"
#include "Nodes.h"
#include "ObjectPrototype.h"
#include "Operations.h"
#include "PropertyDescriptor.h"
#include "PropertyNameArray.h"
#include "Reject.h"
#include "SlotVisitorInlines.h"
#include <math.h>
#include <wtf/Assertions.h>

namespace JSC {

// We keep track of the size of the last array after it was grown. We use this
// as a simple heuristic for as the value to grow the next array from size 0.
// This value is capped by the constant FIRST_VECTOR_GROW defined in
// ArrayConventions.h.
static unsigned lastArraySize = 0;

JSCell* getCallableObjectSlow(JSCell* cell)
{
    Structure* structure = cell->structure();
    if (structure->typeInfo().type() == JSFunctionType)
        return cell;
    if (structure->classInfo()->isSubClassOf(&InternalFunction::s_info))
        return cell;
    return 0;
}

ASSERT_HAS_TRIVIAL_DESTRUCTOR(JSObject);
ASSERT_HAS_TRIVIAL_DESTRUCTOR(JSFinalObject);

const char* StrictModeReadonlyPropertyWriteError = "Attempted to assign to readonly property.";

const ClassInfo JSObject::s_info = { "Object", 0, 0, 0, CREATE_METHOD_TABLE(JSObject) };

const ClassInfo JSFinalObject::s_info = { "Object", &Base::s_info, 0, 0, CREATE_METHOD_TABLE(JSFinalObject) };

static inline void getClassPropertyNames(ExecState* exec, const ClassInfo* classInfo, PropertyNameArray& propertyNames, EnumerationMode mode, bool didReify)
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
            if (entry->key() && (!(entry->attributes() & DontEnum) || (mode == IncludeDontEnumProperties)) && !((entry->attributes() & Function) && didReify))
                propertyNames.add(entry->key());
        }
    }
}

ALWAYS_INLINE void JSObject::copyButterfly(CopyVisitor& visitor, Butterfly* butterfly, size_t storageSize)
{
    ASSERT(butterfly);
    
    Structure* structure = this->structure();
    
    size_t propertyCapacity = structure->outOfLineCapacity();
    size_t preCapacity;
    size_t indexingPayloadSizeInBytes;
    bool hasIndexingHeader = JSC::hasIndexingHeader(structure->indexingType());
    if (UNLIKELY(hasIndexingHeader)) {
        preCapacity = butterfly->indexingHeader()->preCapacity(structure);
        indexingPayloadSizeInBytes = butterfly->indexingHeader()->indexingPayloadSizeInBytes(structure);
    } else {
        preCapacity = 0;
        indexingPayloadSizeInBytes = 0;
    }
    size_t capacityInBytes = Butterfly::totalSize(preCapacity, propertyCapacity, hasIndexingHeader, indexingPayloadSizeInBytes);
    if (visitor.checkIfShouldCopy(butterfly->base(preCapacity, propertyCapacity))) {
        Butterfly* newButterfly = Butterfly::createUninitializedDuringCollection(visitor, preCapacity, propertyCapacity, hasIndexingHeader, indexingPayloadSizeInBytes);
        
        // Copy the properties.
        PropertyStorage currentTarget = newButterfly->propertyStorage();
        PropertyStorage currentSource = butterfly->propertyStorage();
        for (size_t count = storageSize; count--;)
            (--currentTarget)->setWithoutWriteBarrier((--currentSource)->get());
        
        if (UNLIKELY(hasIndexingHeader)) {
            *newButterfly->indexingHeader() = *butterfly->indexingHeader();
            
            // Copy the array if appropriate.
            
            WriteBarrier<Unknown>* currentTarget;
            WriteBarrier<Unknown>* currentSource;
            size_t count;
            
            switch (structure->indexingType()) {
            case ALL_UNDECIDED_INDEXING_TYPES:
            case ALL_CONTIGUOUS_INDEXING_TYPES:
            case ALL_INT32_INDEXING_TYPES:
            case ALL_DOUBLE_INDEXING_TYPES: {
                currentTarget = newButterfly->contiguous().data();
                currentSource = butterfly->contiguous().data();
                RELEASE_ASSERT(newButterfly->publicLength() <= newButterfly->vectorLength());
                count = newButterfly->vectorLength();
                break;
            }
                
            case ALL_ARRAY_STORAGE_INDEXING_TYPES: {
                newButterfly->arrayStorage()->copyHeaderFromDuringGC(*butterfly->arrayStorage());
                currentTarget = newButterfly->arrayStorage()->m_vector;
                currentSource = butterfly->arrayStorage()->m_vector;
                count = newButterfly->arrayStorage()->vectorLength();
                break;
            }
            default:
                CRASH();
                currentTarget = 0;
                currentSource = 0;
                count = 0;
                break;
            }

            memcpy(currentTarget, currentSource, count * sizeof(EncodedJSValue));
        }
        
        m_butterfly = newButterfly;
        visitor.didCopy(butterfly->base(preCapacity, propertyCapacity), capacityInBytes);
    } 
}

ALWAYS_INLINE void JSObject::visitButterfly(SlotVisitor& visitor, Butterfly* butterfly, size_t storageSize)
{
    ASSERT(butterfly);
    
    Structure* structure = this->structure();
    
    size_t propertyCapacity = structure->outOfLineCapacity();
    size_t preCapacity;
    size_t indexingPayloadSizeInBytes;
    bool hasIndexingHeader = JSC::hasIndexingHeader(structure->indexingType());
    if (UNLIKELY(hasIndexingHeader)) {
        preCapacity = butterfly->indexingHeader()->preCapacity(structure);
        indexingPayloadSizeInBytes = butterfly->indexingHeader()->indexingPayloadSizeInBytes(structure);
    } else {
        preCapacity = 0;
        indexingPayloadSizeInBytes = 0;
    }
    size_t capacityInBytes = Butterfly::totalSize(preCapacity, propertyCapacity, hasIndexingHeader, indexingPayloadSizeInBytes);

    // Mark the properties.
    visitor.appendValues(butterfly->propertyStorage() - storageSize, storageSize);
    visitor.copyLater(this, butterfly->base(preCapacity, propertyCapacity), capacityInBytes);
    
    // Mark the array if appropriate.
    switch (structure->indexingType()) {
    case ALL_CONTIGUOUS_INDEXING_TYPES:
        visitor.appendValues(butterfly->contiguous().data(), butterfly->publicLength());
        break;
    case ALL_ARRAY_STORAGE_INDEXING_TYPES:
        visitor.appendValues(butterfly->arrayStorage()->m_vector, butterfly->arrayStorage()->vectorLength());
        if (butterfly->arrayStorage()->m_sparseMap)
            visitor.append(&butterfly->arrayStorage()->m_sparseMap);
        break;
    default:
        break;
    }
}

void JSObject::visitChildren(JSCell* cell, SlotVisitor& visitor)
{
    JSObject* thisObject = jsCast<JSObject*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, &s_info);
#if !ASSERT_DISABLED
    bool wasCheckingForDefaultMarkViolation = visitor.m_isCheckingForDefaultMarkViolation;
    visitor.m_isCheckingForDefaultMarkViolation = false;
#endif
    
    JSCell::visitChildren(thisObject, visitor);

    Butterfly* butterfly = thisObject->butterfly();
    if (butterfly)
        thisObject->visitButterfly(visitor, butterfly, thisObject->structure()->outOfLineSize());

#if !ASSERT_DISABLED
    visitor.m_isCheckingForDefaultMarkViolation = wasCheckingForDefaultMarkViolation;
#endif
}

void JSObject::copyBackingStore(JSCell* cell, CopyVisitor& visitor)
{
    JSObject* thisObject = jsCast<JSObject*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, &s_info);
    
    Butterfly* butterfly = thisObject->butterfly();
    if (butterfly)
        thisObject->copyButterfly(visitor, butterfly, thisObject->structure()->outOfLineSize());
}

void JSFinalObject::visitChildren(JSCell* cell, SlotVisitor& visitor)
{
    JSFinalObject* thisObject = jsCast<JSFinalObject*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, &s_info);
#if !ASSERT_DISABLED
    bool wasCheckingForDefaultMarkViolation = visitor.m_isCheckingForDefaultMarkViolation;
    visitor.m_isCheckingForDefaultMarkViolation = false;
#endif
    
    JSCell::visitChildren(thisObject, visitor);

    Butterfly* butterfly = thisObject->butterfly();
    if (butterfly)
        thisObject->visitButterfly(visitor, butterfly, thisObject->structure()->outOfLineSize());

    size_t storageSize = thisObject->structure()->inlineSize();
    visitor.appendValues(thisObject->inlineStorage(), storageSize);

#if !ASSERT_DISABLED
    visitor.m_isCheckingForDefaultMarkViolation = wasCheckingForDefaultMarkViolation;
#endif
}

String JSObject::className(const JSObject* object)
{
    const ClassInfo* info = object->classInfo();
    ASSERT(info);
    return info->className;
}

bool JSObject::getOwnPropertySlotByIndex(JSCell* cell, ExecState* exec, unsigned i, PropertySlot& slot)
{
    // NB. The fact that we're directly consulting our indexed storage implies that it is not
    // legal for anyone to override getOwnPropertySlot() without also overriding
    // getOwnPropertySlotByIndex().
    
    JSObject* thisObject = jsCast<JSObject*>(cell);
    
    if (i > MAX_ARRAY_INDEX)
        return thisObject->methodTable()->getOwnPropertySlot(thisObject, exec, Identifier::from(exec, i), slot);
    
    switch (thisObject->structure()->indexingType()) {
    case ALL_BLANK_INDEXING_TYPES:
    case ALL_UNDECIDED_INDEXING_TYPES:
        break;
        
    case ALL_INT32_INDEXING_TYPES:
    case ALL_CONTIGUOUS_INDEXING_TYPES: {
        Butterfly* butterfly = thisObject->m_butterfly;
        if (i >= butterfly->vectorLength())
            return false;
        
        JSValue value = butterfly->contiguous()[i].get();
        if (value) {
            slot.setValue(value);
            return true;
        }
        
        return false;
    }
        
    case ALL_DOUBLE_INDEXING_TYPES: {
        Butterfly* butterfly = thisObject->m_butterfly;
        if (i >= butterfly->vectorLength())
            return false;
        
        double value = butterfly->contiguousDouble()[i];
        if (value == value) {
            slot.setValue(JSValue(JSValue::EncodeAsDouble, value));
            return true;
        }
        
        return false;
    }
        
    case ALL_ARRAY_STORAGE_INDEXING_TYPES: {
        ArrayStorage* storage = thisObject->m_butterfly->arrayStorage();
        if (i >= storage->length())
            return false;
        
        if (i < storage->vectorLength()) {
            JSValue value = storage->m_vector[i].get();
            if (value) {
                slot.setValue(value);
                return true;
            }
        } else if (SparseArrayValueMap* map = storage->m_sparseMap.get()) {
            SparseArrayValueMap::iterator it = map->find(i);
            if (it != map->notFound()) {
                it->value.get(slot);
                return true;
            }
        }
        break;
    }
        
    default:
        RELEASE_ASSERT_NOT_REACHED();
        break;
    }
    
    return false;
}

// ECMA 8.6.2.2
void JSObject::put(JSCell* cell, ExecState* exec, PropertyName propertyName, JSValue value, PutPropertySlot& slot)
{
    JSObject* thisObject = jsCast<JSObject*>(cell);
    ASSERT(value);
    ASSERT(!Heap::heap(value) || Heap::heap(value) == Heap::heap(thisObject));
    VM& vm = exec->vm();
    
    // Try indexed put first. This is required for correctness, since loads on property names that appear like
    // valid indices will never look in the named property storage.
    unsigned i = propertyName.asIndex();
    if (i != PropertyName::NotAnIndex) {
        putByIndex(thisObject, exec, i, value, slot.isStrictMode());
        return;
    }
    
    // Check if there are any setters or getters in the prototype chain
    JSValue prototype;
    if (propertyName != exec->propertyNames().underscoreProto) {
        for (JSObject* obj = thisObject; !obj->structure()->hasReadOnlyOrGetterSetterPropertiesExcludingProto(); obj = asObject(prototype)) {
            prototype = obj->prototype();
            if (prototype.isNull()) {
                ASSERT(!thisObject->structure()->prototypeChainMayInterceptStoreTo(exec->vm(), propertyName));
                if (!thisObject->putDirectInternal<PutModePut>(vm, propertyName, value, 0, slot, getCallableObject(value))
                    && slot.isStrictMode())
                    throwTypeError(exec, ASCIILiteral(StrictModeReadonlyPropertyWriteError));
                return;
            }
        }
    }

    JSObject* obj;
    for (obj = thisObject; ; obj = asObject(prototype)) {
        unsigned attributes;
        JSCell* specificValue;
        PropertyOffset offset = obj->structure()->get(vm, propertyName, attributes, specificValue);
        if (isValidOffset(offset)) {
            if (attributes & ReadOnly) {
                ASSERT(thisObject->structure()->prototypeChainMayInterceptStoreTo(exec->vm(), propertyName) || obj == thisObject);
                if (slot.isStrictMode())
                    throwError(exec, createTypeError(exec, ASCIILiteral(StrictModeReadonlyPropertyWriteError)));
                return;
            }

            JSValue gs = obj->getDirect(offset);
            if (gs.isGetterSetter()) {
                ASSERT(attributes & Accessor);
                ASSERT(thisObject->structure()->prototypeChainMayInterceptStoreTo(exec->vm(), propertyName) || obj == thisObject);
                JSObject* setterFunc = asGetterSetter(gs)->setter();        
                if (!setterFunc) {
                    if (slot.isStrictMode())
                        throwError(exec, createTypeError(exec, ASCIILiteral("setting a property that has only a getter")));
                    return;
                }
                
                CallData callData;
                CallType callType = setterFunc->methodTable()->getCallData(setterFunc, callData);
                MarkedArgumentBuffer args;
                args.append(value);

                // If this is WebCore's global object then we need to substitute the shell.
                call(exec, setterFunc, callType, callData, thisObject->methodTable()->toThisObject(thisObject, exec), args);
                return;
            } else
                ASSERT(!(attributes & Accessor));

            // If there's an existing property on the object or one of its 
            // prototypes it should be replaced, so break here.
            break;
        }

        prototype = obj->prototype();
        if (prototype.isNull())
            break;
    }
    
    ASSERT(!thisObject->structure()->prototypeChainMayInterceptStoreTo(exec->vm(), propertyName) || obj == thisObject);
    if (!thisObject->putDirectInternal<PutModePut>(vm, propertyName, value, 0, slot, getCallableObject(value)) && slot.isStrictMode())
        throwTypeError(exec, ASCIILiteral(StrictModeReadonlyPropertyWriteError));
    return;
}

void JSObject::putByIndex(JSCell* cell, ExecState* exec, unsigned propertyName, JSValue value, bool shouldThrow)
{
    JSObject* thisObject = jsCast<JSObject*>(cell);
    
    if (propertyName > MAX_ARRAY_INDEX) {
        PutPropertySlot slot(shouldThrow);
        thisObject->methodTable()->put(thisObject, exec, Identifier::from(exec, propertyName), value, slot);
        return;
    }
    
    switch (thisObject->structure()->indexingType()) {
    case ALL_BLANK_INDEXING_TYPES:
        break;
        
    case ALL_UNDECIDED_INDEXING_TYPES: {
        thisObject->convertUndecidedForValue(exec->vm(), value);
        // Reloop.
        putByIndex(cell, exec, propertyName, value, shouldThrow);
        return;
    }
        
    case ALL_INT32_INDEXING_TYPES: {
        if (!value.isInt32()) {
            thisObject->convertInt32ForValue(exec->vm(), value);
            putByIndex(cell, exec, propertyName, value, shouldThrow);
            return;
        }
        // Fall through.
    }
        
    case ALL_CONTIGUOUS_INDEXING_TYPES: {
        Butterfly* butterfly = thisObject->m_butterfly;
        if (propertyName >= butterfly->vectorLength())
            break;
        butterfly->contiguous()[propertyName].set(exec->vm(), thisObject, value);
        if (propertyName >= butterfly->publicLength())
            butterfly->setPublicLength(propertyName + 1);
        return;
    }
        
    case ALL_DOUBLE_INDEXING_TYPES: {
        if (!value.isNumber()) {
            thisObject->convertDoubleToContiguous(exec->vm());
            // Reloop.
            putByIndex(cell, exec, propertyName, value, shouldThrow);
            return;
        }
        double valueAsDouble = value.asNumber();
        if (valueAsDouble != valueAsDouble) {
            thisObject->convertDoubleToContiguous(exec->vm());
            // Reloop.
            putByIndex(cell, exec, propertyName, value, shouldThrow);
            return;
        }
        Butterfly* butterfly = thisObject->m_butterfly;
        if (propertyName >= butterfly->vectorLength())
            break;
        butterfly->contiguousDouble()[propertyName] = valueAsDouble;
        if (propertyName >= butterfly->publicLength())
            butterfly->setPublicLength(propertyName + 1);
        return;
    }
        
    case NonArrayWithArrayStorage:
    case ArrayWithArrayStorage: {
        ArrayStorage* storage = thisObject->m_butterfly->arrayStorage();
        
        if (propertyName >= storage->vectorLength())
            break;
        
        WriteBarrier<Unknown>& valueSlot = storage->m_vector[propertyName];
        unsigned length = storage->length();
        
        // Update length & m_numValuesInVector as necessary.
        if (propertyName >= length) {
            length = propertyName + 1;
            storage->setLength(length);
            ++storage->m_numValuesInVector;
        } else if (!valueSlot)
            ++storage->m_numValuesInVector;
        
        valueSlot.set(exec->vm(), thisObject, value);
        return;
    }
        
    case NonArrayWithSlowPutArrayStorage:
    case ArrayWithSlowPutArrayStorage: {
        ArrayStorage* storage = thisObject->m_butterfly->arrayStorage();
        
        if (propertyName >= storage->vectorLength())
            break;
        
        WriteBarrier<Unknown>& valueSlot = storage->m_vector[propertyName];
        unsigned length = storage->length();
        
        // Update length & m_numValuesInVector as necessary.
        if (propertyName >= length) {
            if (thisObject->attemptToInterceptPutByIndexOnHole(exec, propertyName, value, shouldThrow))
                return;
            length = propertyName + 1;
            storage->setLength(length);
            ++storage->m_numValuesInVector;
        } else if (!valueSlot) {
            if (thisObject->attemptToInterceptPutByIndexOnHole(exec, propertyName, value, shouldThrow))
                return;
            ++storage->m_numValuesInVector;
        }
        
        valueSlot.set(exec->vm(), thisObject, value);
        return;
    }
        
    default:
        RELEASE_ASSERT_NOT_REACHED();
    }
    
    thisObject->putByIndexBeyondVectorLength(exec, propertyName, value, shouldThrow);
}

ArrayStorage* JSObject::enterDictionaryIndexingModeWhenArrayStorageAlreadyExists(VM& vm, ArrayStorage* storage)
{
    SparseArrayValueMap* map = storage->m_sparseMap.get();

    if (!map)
        map = allocateSparseIndexMap(vm);

    if (map->sparseMode())
        return storage;

    map->setSparseMode();

    unsigned usedVectorLength = std::min(storage->length(), storage->vectorLength());
    for (unsigned i = 0; i < usedVectorLength; ++i) {
        JSValue value = storage->m_vector[i].get();
        // This will always be a new entry in the map, so no need to check we can write,
        // and attributes are default so no need to set them.
        if (value)
            map->add(this, i).iterator->value.set(vm, this, value);
    }

    Butterfly* newButterfly = storage->butterfly()->resizeArray(vm, structure(), 0, ArrayStorage::sizeFor(0));
    RELEASE_ASSERT(newButterfly);
    
    m_butterfly = newButterfly;
    newButterfly->arrayStorage()->m_indexBias = 0;
    newButterfly->arrayStorage()->setVectorLength(0);
    newButterfly->arrayStorage()->m_sparseMap.set(vm, this, map);
    
    return newButterfly->arrayStorage();
}

void JSObject::enterDictionaryIndexingMode(VM& vm)
{
    switch (structure()->indexingType()) {
    case ALL_BLANK_INDEXING_TYPES:
    case ALL_UNDECIDED_INDEXING_TYPES:
    case ALL_INT32_INDEXING_TYPES:
    case ALL_DOUBLE_INDEXING_TYPES:
    case ALL_CONTIGUOUS_INDEXING_TYPES:
        // NOTE: this is horribly inefficient, as it will perform two conversions. We could optimize
        // this case if we ever cared.
        enterDictionaryIndexingModeWhenArrayStorageAlreadyExists(vm, ensureArrayStorageSlow(vm));
        break;
    case ALL_ARRAY_STORAGE_INDEXING_TYPES:
        enterDictionaryIndexingModeWhenArrayStorageAlreadyExists(vm, m_butterfly->arrayStorage());
        break;
        
    default:
        break;
    }
}

void JSObject::notifyPresenceOfIndexedAccessors(VM& vm)
{
    if (mayInterceptIndexedAccesses())
        return;
    
    setStructure(vm, Structure::nonPropertyTransition(vm, structure(), AddIndexedAccessors));
    
    if (!vm.prototypeMap.isPrototype(this))
        return;
    
    globalObject()->haveABadTime(vm);
}

Butterfly* JSObject::createInitialIndexedStorage(VM& vm, unsigned length, size_t elementSize)
{
    ASSERT(length < MAX_ARRAY_INDEX);
    IndexingType oldType = structure()->indexingType();
    ASSERT_UNUSED(oldType, !hasIndexedProperties(oldType));
    ASSERT(!structure()->needsSlowPutIndexing());
    ASSERT(!indexingShouldBeSparse());
    unsigned vectorLength = std::max(length, BASE_VECTOR_LEN);
    Butterfly* newButterfly = Butterfly::createOrGrowArrayRight(m_butterfly, 
        vm, structure(), structure()->outOfLineCapacity(), false, 0,
        elementSize * vectorLength);
    newButterfly->setPublicLength(length);
    newButterfly->setVectorLength(vectorLength);
    return newButterfly;
}

Butterfly* JSObject::createInitialUndecided(VM& vm, unsigned length)
{
    Butterfly* newButterfly = createInitialIndexedStorage(vm, length, sizeof(EncodedJSValue));
    Structure* newStructure = Structure::nonPropertyTransition(vm, structure(), AllocateUndecided);
    setButterfly(vm, newButterfly, newStructure);
    return newButterfly;
}

ContiguousJSValues JSObject::createInitialInt32(VM& vm, unsigned length)
{
    Butterfly* newButterfly = createInitialIndexedStorage(vm, length, sizeof(EncodedJSValue));
    Structure* newStructure = Structure::nonPropertyTransition(vm, structure(), AllocateInt32);
    setButterfly(vm, newButterfly, newStructure);
    return newButterfly->contiguousInt32();
}

ContiguousDoubles JSObject::createInitialDouble(VM& vm, unsigned length)
{
    Butterfly* newButterfly = createInitialIndexedStorage(vm, length, sizeof(double));
    for (unsigned i = newButterfly->vectorLength(); i--;)
        newButterfly->contiguousDouble()[i] = QNaN;
    Structure* newStructure = Structure::nonPropertyTransition(vm, structure(), AllocateDouble);
    setButterfly(vm, newButterfly, newStructure);
    return newButterfly->contiguousDouble();
}

ContiguousJSValues JSObject::createInitialContiguous(VM& vm, unsigned length)
{
    Butterfly* newButterfly = createInitialIndexedStorage(vm, length, sizeof(EncodedJSValue));
    Structure* newStructure = Structure::nonPropertyTransition(vm, structure(), AllocateContiguous);
    setButterfly(vm, newButterfly, newStructure);
    return newButterfly->contiguous();
}

ArrayStorage* JSObject::createArrayStorage(VM& vm, unsigned length, unsigned vectorLength)
{
    IndexingType oldType = structure()->indexingType();
    ASSERT_UNUSED(oldType, !hasIndexedProperties(oldType));
    Butterfly* newButterfly = Butterfly::createOrGrowArrayRight(m_butterfly, 
        vm, structure(), structure()->outOfLineCapacity(), false, 0,
        ArrayStorage::sizeFor(vectorLength));
    RELEASE_ASSERT(newButterfly);

    ArrayStorage* result = newButterfly->arrayStorage();
    result->setLength(length);
    result->setVectorLength(vectorLength);
    result->m_sparseMap.clear();
    result->m_numValuesInVector = 0;
    result->m_indexBias = 0;
    Structure* newStructure = Structure::nonPropertyTransition(vm, structure(), structure()->suggestedArrayStorageTransition());
    setButterfly(vm, newButterfly, newStructure);
    return result;
}

ArrayStorage* JSObject::createInitialArrayStorage(VM& vm)
{
    return createArrayStorage(vm, 0, BASE_VECTOR_LEN);
}

ContiguousJSValues JSObject::convertUndecidedToInt32(VM& vm)
{
    ASSERT(hasUndecided(structure()->indexingType()));
    setStructure(vm, Structure::nonPropertyTransition(vm, structure(), AllocateInt32));
    return m_butterfly->contiguousInt32();
}

ContiguousDoubles JSObject::convertUndecidedToDouble(VM& vm)
{
    ASSERT(hasUndecided(structure()->indexingType()));
    
    for (unsigned i = m_butterfly->vectorLength(); i--;)
        m_butterfly->contiguousDouble()[i] = QNaN;
    
    setStructure(vm, Structure::nonPropertyTransition(vm, structure(), AllocateDouble));
    return m_butterfly->contiguousDouble();
}

ContiguousJSValues JSObject::convertUndecidedToContiguous(VM& vm)
{
    ASSERT(hasUndecided(structure()->indexingType()));
    setStructure(vm, Structure::nonPropertyTransition(vm, structure(), AllocateContiguous));
    return m_butterfly->contiguous();
}

ArrayStorage* JSObject::constructConvertedArrayStorageWithoutCopyingElements(VM& vm, unsigned neededLength)
{
    unsigned publicLength = m_butterfly->publicLength();
    unsigned propertyCapacity = structure()->outOfLineCapacity();
    unsigned propertySize = structure()->outOfLineSize();
    
    Butterfly* newButterfly = Butterfly::createUninitialized(
        vm, 0, propertyCapacity, true, ArrayStorage::sizeFor(neededLength));
    
    memcpy(
        newButterfly->propertyStorage() - propertySize,
        m_butterfly->propertyStorage() - propertySize,
        propertySize * sizeof(EncodedJSValue));
    
    ArrayStorage* newStorage = newButterfly->arrayStorage();
    newStorage->setVectorLength(neededLength);
    newStorage->setLength(publicLength);
    newStorage->m_sparseMap.clear();
    newStorage->m_indexBias = 0;
    newStorage->m_numValuesInVector = 0;
    
    return newStorage;
}

ArrayStorage* JSObject::convertUndecidedToArrayStorage(VM& vm, NonPropertyTransition transition, unsigned neededLength)
{
    ASSERT(hasUndecided(structure()->indexingType()));
    
    ArrayStorage* storage = constructConvertedArrayStorageWithoutCopyingElements(vm, neededLength);
    // No need to copy elements.
    
    Structure* newStructure = Structure::nonPropertyTransition(vm, structure(), transition);
    setButterfly(vm, storage->butterfly(), newStructure);
    return storage;
}

ArrayStorage* JSObject::convertUndecidedToArrayStorage(VM& vm, NonPropertyTransition transition)
{
    return convertUndecidedToArrayStorage(vm, transition, m_butterfly->vectorLength());
}

ArrayStorage* JSObject::convertUndecidedToArrayStorage(VM& vm)
{
    return convertUndecidedToArrayStorage(vm, structure()->suggestedArrayStorageTransition());
}

ContiguousDoubles JSObject::convertInt32ToDouble(VM& vm)
{
    ASSERT(hasInt32(structure()->indexingType()));
    
    for (unsigned i = m_butterfly->vectorLength(); i--;) {
        WriteBarrier<Unknown>* current = &m_butterfly->contiguousInt32()[i];
        double* currentAsDouble = bitwise_cast<double*>(current);
        JSValue v = current->get();
        if (!v) {
            *currentAsDouble = QNaN;
            continue;
        }
        ASSERT(v.isInt32());
        *currentAsDouble = v.asInt32();
    }
    
    setStructure(vm, Structure::nonPropertyTransition(vm, structure(), AllocateDouble));
    return m_butterfly->contiguousDouble();
}

ContiguousJSValues JSObject::convertInt32ToContiguous(VM& vm)
{
    ASSERT(hasInt32(structure()->indexingType()));
    
    setStructure(vm, Structure::nonPropertyTransition(vm, structure(), AllocateContiguous));
    return m_butterfly->contiguous();
}

ArrayStorage* JSObject::convertInt32ToArrayStorage(VM& vm, NonPropertyTransition transition, unsigned neededLength)
{
    ASSERT(hasInt32(structure()->indexingType()));
    
    ArrayStorage* newStorage = constructConvertedArrayStorageWithoutCopyingElements(vm, neededLength);
    for (unsigned i = m_butterfly->publicLength(); i--;) {
        JSValue v = m_butterfly->contiguous()[i].get();
        if (!v)
            continue;
        newStorage->m_vector[i].setWithoutWriteBarrier(v);
        newStorage->m_numValuesInVector++;
    }
    
    Structure* newStructure = Structure::nonPropertyTransition(vm, structure(), transition);
    setButterfly(vm, newStorage->butterfly(), newStructure);
    return newStorage;
}

ArrayStorage* JSObject::convertInt32ToArrayStorage(VM& vm, NonPropertyTransition transition)
{
    return convertInt32ToArrayStorage(vm, transition, m_butterfly->vectorLength());
}

ArrayStorage* JSObject::convertInt32ToArrayStorage(VM& vm)
{
    return convertInt32ToArrayStorage(vm, structure()->suggestedArrayStorageTransition());
}

template<JSObject::DoubleToContiguousMode mode>
ContiguousJSValues JSObject::genericConvertDoubleToContiguous(VM& vm)
{
    ASSERT(hasDouble(structure()->indexingType()));
    
    for (unsigned i = m_butterfly->vectorLength(); i--;) {
        double* current = &m_butterfly->contiguousDouble()[i];
        WriteBarrier<Unknown>* currentAsValue = bitwise_cast<WriteBarrier<Unknown>*>(current);
        double value = *current;
        if (value != value) {
            currentAsValue->clear();
            continue;
        }
        JSValue v;
        switch (mode) {
        case EncodeValueAsDouble:
            v = JSValue(JSValue::EncodeAsDouble, value);
            break;
        case RageConvertDoubleToValue:
            v = jsNumber(value);
            break;
        }
        ASSERT(v.isNumber());
        currentAsValue->setWithoutWriteBarrier(v);
    }
    
    setStructure(vm, Structure::nonPropertyTransition(vm, structure(), AllocateContiguous));
    return m_butterfly->contiguous();
}

ContiguousJSValues JSObject::convertDoubleToContiguous(VM& vm)
{
    return genericConvertDoubleToContiguous<EncodeValueAsDouble>(vm);
}

ContiguousJSValues JSObject::rageConvertDoubleToContiguous(VM& vm)
{
    return genericConvertDoubleToContiguous<RageConvertDoubleToValue>(vm);
}

ArrayStorage* JSObject::convertDoubleToArrayStorage(VM& vm, NonPropertyTransition transition, unsigned neededLength)
{
    ASSERT(hasDouble(structure()->indexingType()));
    
    ArrayStorage* newStorage = constructConvertedArrayStorageWithoutCopyingElements(vm, neededLength);
    for (unsigned i = m_butterfly->publicLength(); i--;) {
        double value = m_butterfly->contiguousDouble()[i];
        if (value != value)
            continue;
        newStorage->m_vector[i].setWithoutWriteBarrier(JSValue(JSValue::EncodeAsDouble, value));
        newStorage->m_numValuesInVector++;
    }
    
    Structure* newStructure = Structure::nonPropertyTransition(vm, structure(), transition);
    setButterfly(vm, newStorage->butterfly(), newStructure);
    return newStorage;
}

ArrayStorage* JSObject::convertDoubleToArrayStorage(VM& vm, NonPropertyTransition transition)
{
    return convertDoubleToArrayStorage(vm, transition, m_butterfly->vectorLength());
}

ArrayStorage* JSObject::convertDoubleToArrayStorage(VM& vm)
{
    return convertDoubleToArrayStorage(vm, structure()->suggestedArrayStorageTransition());
}

ArrayStorage* JSObject::convertContiguousToArrayStorage(VM& vm, NonPropertyTransition transition, unsigned neededLength)
{
    ASSERT(hasContiguous(structure()->indexingType()));
    
    ArrayStorage* newStorage = constructConvertedArrayStorageWithoutCopyingElements(vm, neededLength);
    for (unsigned i = m_butterfly->publicLength(); i--;) {
        JSValue v = m_butterfly->contiguous()[i].get();
        if (!v)
            continue;
        newStorage->m_vector[i].setWithoutWriteBarrier(v);
        newStorage->m_numValuesInVector++;
    }
    
    Structure* newStructure = Structure::nonPropertyTransition(vm, structure(), transition);
    setButterfly(vm, newStorage->butterfly(), newStructure);
    return newStorage;
}

ArrayStorage* JSObject::convertContiguousToArrayStorage(VM& vm, NonPropertyTransition transition)
{
    return convertContiguousToArrayStorage(vm, transition, m_butterfly->vectorLength());
}

ArrayStorage* JSObject::convertContiguousToArrayStorage(VM& vm)
{
    return convertContiguousToArrayStorage(vm, structure()->suggestedArrayStorageTransition());
}

void JSObject::convertUndecidedForValue(VM& vm, JSValue value)
{
    if (value.isInt32()) {
        convertUndecidedToInt32(vm);
        return;
    }
    
    if (value.isDouble()) {
        convertUndecidedToDouble(vm);
        return;
    }
    
    convertUndecidedToContiguous(vm);
}

void JSObject::convertInt32ForValue(VM& vm, JSValue value)
{
    ASSERT(!value.isInt32());
    
    if (value.isDouble()) {
        convertInt32ToDouble(vm);
        return;
    }
    
    convertInt32ToContiguous(vm);
}

void JSObject::setIndexQuicklyToUndecided(VM& vm, unsigned index, JSValue value)
{
    ASSERT(index < m_butterfly->publicLength());
    ASSERT(index < m_butterfly->vectorLength());
    convertUndecidedForValue(vm, value);
    setIndexQuickly(vm, index, value);
}

void JSObject::convertInt32ToDoubleOrContiguousWhilePerformingSetIndex(VM& vm, unsigned index, JSValue value)
{
    ASSERT(!value.isInt32());
    convertInt32ForValue(vm, value);
    setIndexQuickly(vm, index, value);
}

void JSObject::convertDoubleToContiguousWhilePerformingSetIndex(VM& vm, unsigned index, JSValue value)
{
    ASSERT(!value.isNumber() || value.asNumber() != value.asNumber());
    convertDoubleToContiguous(vm);
    setIndexQuickly(vm, index, value);
}

ContiguousJSValues JSObject::ensureInt32Slow(VM& vm)
{
    ASSERT(inherits(&s_info));
    
    switch (structure()->indexingType()) {
    case ALL_BLANK_INDEXING_TYPES:
        if (UNLIKELY(indexingShouldBeSparse() || structure()->needsSlowPutIndexing()))
            return ContiguousJSValues();
        return createInitialInt32(vm, 0);
        
    case ALL_UNDECIDED_INDEXING_TYPES:
        return convertUndecidedToInt32(vm);
        
    case ALL_DOUBLE_INDEXING_TYPES:
    case ALL_CONTIGUOUS_INDEXING_TYPES:
    case ALL_ARRAY_STORAGE_INDEXING_TYPES:
        return ContiguousJSValues();
        
    default:
        CRASH();
        return ContiguousJSValues();
    }
}

ContiguousDoubles JSObject::ensureDoubleSlow(VM& vm)
{
    ASSERT(inherits(&s_info));
    
    switch (structure()->indexingType()) {
    case ALL_BLANK_INDEXING_TYPES:
        if (UNLIKELY(indexingShouldBeSparse() || structure()->needsSlowPutIndexing()))
            return ContiguousDoubles();
        return createInitialDouble(vm, 0);
        
    case ALL_UNDECIDED_INDEXING_TYPES:
        return convertUndecidedToDouble(vm);
        
    case ALL_INT32_INDEXING_TYPES:
        return convertInt32ToDouble(vm);
        
    case ALL_CONTIGUOUS_INDEXING_TYPES:
    case ALL_ARRAY_STORAGE_INDEXING_TYPES:
        return ContiguousDoubles();
        
    default:
        CRASH();
        return ContiguousDoubles();
    }
}

ContiguousJSValues JSObject::ensureContiguousSlow(VM& vm, DoubleToContiguousMode mode)
{
    ASSERT(inherits(&s_info));
    
    switch (structure()->indexingType()) {
    case ALL_BLANK_INDEXING_TYPES:
        if (UNLIKELY(indexingShouldBeSparse() || structure()->needsSlowPutIndexing()))
            return ContiguousJSValues();
        return createInitialContiguous(vm, 0);
        
    case ALL_UNDECIDED_INDEXING_TYPES:
        return convertUndecidedToContiguous(vm);
        
    case ALL_INT32_INDEXING_TYPES:
        return convertInt32ToContiguous(vm);
        
    case ALL_DOUBLE_INDEXING_TYPES:
        if (mode == RageConvertDoubleToValue)
            return rageConvertDoubleToContiguous(vm);
        return convertDoubleToContiguous(vm);
        
    case ALL_ARRAY_STORAGE_INDEXING_TYPES:
        return ContiguousJSValues();
        
    default:
        CRASH();
        return ContiguousJSValues();
    }
}

ContiguousJSValues JSObject::ensureContiguousSlow(VM& vm)
{
    return ensureContiguousSlow(vm, EncodeValueAsDouble);
}

ContiguousJSValues JSObject::rageEnsureContiguousSlow(VM& vm)
{
    return ensureContiguousSlow(vm, RageConvertDoubleToValue);
}

ArrayStorage* JSObject::ensureArrayStorageSlow(VM& vm)
{
    ASSERT(inherits(&s_info));
    
    switch (structure()->indexingType()) {
    case ALL_BLANK_INDEXING_TYPES:
        if (UNLIKELY(indexingShouldBeSparse()))
            return ensureArrayStorageExistsAndEnterDictionaryIndexingMode(vm);
        return createInitialArrayStorage(vm);
        
    case ALL_UNDECIDED_INDEXING_TYPES:
        ASSERT(!indexingShouldBeSparse());
        ASSERT(!structure()->needsSlowPutIndexing());
        return convertUndecidedToArrayStorage(vm);
        
    case ALL_INT32_INDEXING_TYPES:
        ASSERT(!indexingShouldBeSparse());
        ASSERT(!structure()->needsSlowPutIndexing());
        return convertInt32ToArrayStorage(vm);
        
    case ALL_DOUBLE_INDEXING_TYPES:
        ASSERT(!indexingShouldBeSparse());
        ASSERT(!structure()->needsSlowPutIndexing());
        return convertDoubleToArrayStorage(vm);
        
    case ALL_CONTIGUOUS_INDEXING_TYPES:
        ASSERT(!indexingShouldBeSparse());
        ASSERT(!structure()->needsSlowPutIndexing());
        return convertContiguousToArrayStorage(vm);
        
    default:
        RELEASE_ASSERT_NOT_REACHED();
        return 0;
    }
}

ArrayStorage* JSObject::ensureArrayStorageExistsAndEnterDictionaryIndexingMode(VM& vm)
{
    switch (structure()->indexingType()) {
    case ALL_BLANK_INDEXING_TYPES: {
        createArrayStorage(vm, 0, 0);
        SparseArrayValueMap* map = allocateSparseIndexMap(vm);
        map->setSparseMode();
        return arrayStorage();
    }
        
    case ALL_UNDECIDED_INDEXING_TYPES:
        return enterDictionaryIndexingModeWhenArrayStorageAlreadyExists(vm, convertUndecidedToArrayStorage(vm));
        
    case ALL_INT32_INDEXING_TYPES:
        return enterDictionaryIndexingModeWhenArrayStorageAlreadyExists(vm, convertInt32ToArrayStorage(vm));
        
    case ALL_DOUBLE_INDEXING_TYPES:
        return enterDictionaryIndexingModeWhenArrayStorageAlreadyExists(vm, convertDoubleToArrayStorage(vm));
        
    case ALL_CONTIGUOUS_INDEXING_TYPES:
        return enterDictionaryIndexingModeWhenArrayStorageAlreadyExists(vm, convertContiguousToArrayStorage(vm));
        
    case ALL_ARRAY_STORAGE_INDEXING_TYPES:
        return enterDictionaryIndexingModeWhenArrayStorageAlreadyExists(vm, m_butterfly->arrayStorage());
        
    default:
        CRASH();
        return 0;
    }
}

void JSObject::switchToSlowPutArrayStorage(VM& vm)
{
    switch (structure()->indexingType()) {
    case ALL_UNDECIDED_INDEXING_TYPES:
        convertUndecidedToArrayStorage(vm, AllocateSlowPutArrayStorage);
        break;
        
    case ALL_INT32_INDEXING_TYPES:
        convertInt32ToArrayStorage(vm, AllocateSlowPutArrayStorage);
        break;
        
    case ALL_DOUBLE_INDEXING_TYPES:
        convertDoubleToArrayStorage(vm, AllocateSlowPutArrayStorage);
        break;
        
    case ALL_CONTIGUOUS_INDEXING_TYPES:
        convertContiguousToArrayStorage(vm, AllocateSlowPutArrayStorage);
        break;
        
    case NonArrayWithArrayStorage:
    case ArrayWithArrayStorage: {
        Structure* newStructure = Structure::nonPropertyTransition(vm, structure(), SwitchToSlowPutArrayStorage);
        setStructure(vm, newStructure);
        break;
    }
        
    default:
        CRASH();
        break;
    }
}

void JSObject::putDirectVirtual(JSObject* object, ExecState* exec, PropertyName propertyName, JSValue value, unsigned attributes)
{
    ASSERT(!value.isGetterSetter() && !(attributes & Accessor));
    PutPropertySlot slot;
    object->putDirectInternal<PutModeDefineOwnProperty>(exec->vm(), propertyName, value, attributes, slot, getCallableObject(value));
}

void JSObject::setPrototype(VM& vm, JSValue prototype)
{
    ASSERT(prototype);
    if (prototype.isObject())
        vm.prototypeMap.addPrototype(asObject(prototype));
    
    Structure* newStructure = Structure::changePrototypeTransition(vm, structure(), prototype);
    setStructure(vm, newStructure);
    
    if (!newStructure->anyObjectInChainMayInterceptIndexedAccesses())
        return;
    
    if (vm.prototypeMap.isPrototype(this)) {
        newStructure->globalObject()->haveABadTime(vm);
        return;
    }
    
    if (!hasIndexingHeader(structure()->indexingType()))
        return;
    
    if (shouldUseSlowPut(structure()->indexingType()))
        return;
    
    switchToSlowPutArrayStorage(vm);
}

bool JSObject::setPrototypeWithCycleCheck(VM& vm, JSValue prototype)
{
    JSValue checkFor = this;
    if (this->isGlobalObject())
        checkFor = jsCast<JSGlobalObject*>(this)->globalExec()->thisValue();

    JSValue nextPrototype = prototype;
    while (nextPrototype && nextPrototype.isObject()) {
        if (nextPrototype == checkFor)
            return false;
        nextPrototype = asObject(nextPrototype)->prototype();
    }
    setPrototype(vm, prototype);
    return true;
}

bool JSObject::allowsAccessFrom(ExecState* exec)
{
    JSGlobalObject* globalObject = this->globalObject();
    return globalObject->globalObjectMethodTable()->allowsAccessFrom(globalObject, exec);
}

void JSObject::putDirectAccessor(ExecState* exec, PropertyName propertyName, JSValue value, unsigned attributes)
{
    ASSERT(value.isGetterSetter() && (attributes & Accessor));

    unsigned index = propertyName.asIndex();
    if (index != PropertyName::NotAnIndex) {
        putDirectIndex(exec, index, value, attributes, PutDirectIndexLikePutDirect);
        return;
    }

    VM& vm = exec->vm();

    PutPropertySlot slot;
    putDirectInternal<PutModeDefineOwnProperty>(vm, propertyName, value, attributes, slot, getCallableObject(value));

    // putDirect will change our Structure if we add a new property. For
    // getters and setters, though, we also need to change our Structure
    // if we override an existing non-getter or non-setter.
    if (slot.type() != PutPropertySlot::NewProperty)
        setStructure(vm, Structure::attributeChangeTransition(vm, structure(), propertyName, attributes));

    if (attributes & ReadOnly)
        structure()->setContainsReadOnlyProperties();

    structure()->setHasGetterSetterProperties(propertyName == vm.propertyNames->underscoreProto);
}

bool JSObject::hasProperty(ExecState* exec, PropertyName propertyName) const
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
bool JSObject::deleteProperty(JSCell* cell, ExecState* exec, PropertyName propertyName)
{
    JSObject* thisObject = jsCast<JSObject*>(cell);
    
    unsigned i = propertyName.asIndex();
    if (i != PropertyName::NotAnIndex)
        return thisObject->methodTable()->deletePropertyByIndex(thisObject, exec, i);

    if (!thisObject->staticFunctionsReified())
        thisObject->reifyStaticFunctionsForDelete(exec);

    unsigned attributes;
    JSCell* specificValue;
    if (isValidOffset(thisObject->structure()->get(exec->vm(), propertyName, attributes, specificValue))) {
        if (attributes & DontDelete && !exec->vm().isInDefineOwnProperty())
            return false;
        thisObject->removeDirect(exec->vm(), propertyName);
        return true;
    }

    // Look in the static hashtable of properties
    const HashEntry* entry = thisObject->findPropertyHashEntry(exec, propertyName);
    if (entry) {
        if (entry->attributes() & DontDelete && !exec->vm().isInDefineOwnProperty())
            return false; // this builtin property can't be deleted

        putEntry(exec, entry, propertyName, jsUndefined(), thisObject);
    }

    return true;
}

bool JSObject::hasOwnProperty(ExecState* exec, PropertyName propertyName) const
{
    PropertySlot slot;
    return const_cast<JSObject*>(this)->methodTable()->getOwnPropertySlot(const_cast<JSObject*>(this), exec, propertyName, slot);
}

bool JSObject::deletePropertyByIndex(JSCell* cell, ExecState* exec, unsigned i)
{
    JSObject* thisObject = jsCast<JSObject*>(cell);
    
    if (i > MAX_ARRAY_INDEX)
        return thisObject->methodTable()->deleteProperty(thisObject, exec, Identifier::from(exec, i));
    
    switch (thisObject->structure()->indexingType()) {
    case ALL_BLANK_INDEXING_TYPES:
    case ALL_UNDECIDED_INDEXING_TYPES:
        return true;
        
    case ALL_INT32_INDEXING_TYPES:
    case ALL_CONTIGUOUS_INDEXING_TYPES: {
        Butterfly* butterfly = thisObject->m_butterfly;
        if (i >= butterfly->vectorLength())
            return true;
        butterfly->contiguous()[i].clear();
        return true;
    }
        
    case ALL_DOUBLE_INDEXING_TYPES: {
        Butterfly* butterfly = thisObject->m_butterfly;
        if (i >= butterfly->vectorLength())
            return true;
        butterfly->contiguousDouble()[i] = QNaN;
        return true;
    }
        
    case ALL_ARRAY_STORAGE_INDEXING_TYPES: {
        ArrayStorage* storage = thisObject->m_butterfly->arrayStorage();
        
        if (i < storage->vectorLength()) {
            WriteBarrier<Unknown>& valueSlot = storage->m_vector[i];
            if (valueSlot) {
                valueSlot.clear();
                --storage->m_numValuesInVector;
            }
        } else if (SparseArrayValueMap* map = storage->m_sparseMap.get()) {
            SparseArrayValueMap::iterator it = map->find(i);
            if (it != map->notFound()) {
                if (it->value.attributes & DontDelete)
                    return false;
                map->remove(it);
            }
        }
        
        return true;
    }
        
    default:
        RELEASE_ASSERT_NOT_REACHED();
        return false;
    }
}

static ALWAYS_INLINE JSValue callDefaultValueFunction(ExecState* exec, const JSObject* object, PropertyName propertyName)
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

bool JSObject::getPrimitiveNumber(ExecState* exec, double& number, JSValue& result) const
{
    result = methodTable()->defaultValue(this, exec, PreferNumber);
    number = result.toNumber(exec);
    return !result.isString();
}

// ECMA 8.6.2.6
JSValue JSObject::defaultValue(const JSObject* object, ExecState* exec, PreferredPrimitiveType hint)
{
    // Must call toString first for Date objects.
    if ((hint == PreferString) || (hint != PreferNumber && object->prototype() == exec->lexicalGlobalObject()->datePrototype())) {
        JSValue value = callDefaultValueFunction(exec, object, exec->propertyNames().toString);
        if (value)
            return value;
        value = callDefaultValueFunction(exec, object, exec->propertyNames().valueOf);
        if (value)
            return value;
    } else {
        JSValue value = callDefaultValueFunction(exec, object, exec->propertyNames().valueOf);
        if (value)
            return value;
        value = callDefaultValueFunction(exec, object, exec->propertyNames().toString);
        if (value)
            return value;
    }

    ASSERT(!exec->hadException());

    return throwError(exec, createTypeError(exec, ASCIILiteral("No default value")));
}

const HashEntry* JSObject::findPropertyHashEntry(ExecState* exec, PropertyName propertyName) const
{
    for (const ClassInfo* info = classInfo(); info; info = info->parentClass) {
        if (const HashTable* propHashTable = info->propHashTable(exec)) {
            if (const HashEntry* entry = propHashTable->entry(exec, propertyName))
                return entry;
        }
    }
    return 0;
}

bool JSObject::hasInstance(ExecState* exec, JSValue value)
{
    TypeInfo info = structure()->typeInfo();
    if (info.implementsDefaultHasInstance())
        return defaultHasInstance(exec, value, get(exec, exec->propertyNames().prototype));
    if (info.implementsHasInstance())
        return methodTable()->customHasInstance(this, exec, value);
    throwError(exec, createInvalidParameterError(exec, "instanceof" , this));
    return false;
}

bool JSObject::defaultHasInstance(ExecState* exec, JSValue value, JSValue proto)
{
    if (!value.isObject())
        return false;

    if (!proto.isObject()) {
        throwError(exec, createTypeError(exec, ASCIILiteral("instanceof called on an object with an invalid prototype property.")));
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
    if (!const_cast<JSObject*>(this)->methodTable()->getOwnPropertyDescriptor(const_cast<JSObject*>(this), exec, propertyName, descriptor))
        return false;
    return descriptor.enumerable();
}

bool JSObject::getPropertySpecificValue(ExecState* exec, PropertyName propertyName, JSCell*& specificValue) const
{
    unsigned attributes;
    if (isValidOffset(structure()->get(exec->vm(), propertyName, attributes, specificValue)))
        return true;

    // This could be a function within the static table? - should probably
    // also look in the hash?  This currently should not be a problem, since
    // we've currently always call 'get' first, which should have populated
    // the normal storage.
    return false;
}

void JSObject::getPropertyNames(JSObject* object, ExecState* exec, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    propertyNames.setBaseObject(object);
    object->methodTable()->getOwnPropertyNames(object, exec, propertyNames, mode);

    if (object->prototype().isNull())
        return;

    JSObject* prototype = asObject(object->prototype());
    while(1) {
        if (prototype->structure()->typeInfo().overridesGetPropertyNames()) {
            prototype->methodTable()->getPropertyNames(prototype, exec, propertyNames, mode);
            break;
        }
        prototype->methodTable()->getOwnPropertyNames(prototype, exec, propertyNames, mode);
        JSValue nextProto = prototype->prototype();
        if (nextProto.isNull())
            break;
        prototype = asObject(nextProto);
    }
}

void JSObject::getOwnPropertyNames(JSObject* object, ExecState* exec, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    // Add numeric properties first. That appears to be the accepted convention.
    // FIXME: Filling PropertyNameArray with an identifier for every integer
    // is incredibly inefficient for large arrays. We need a different approach,
    // which almost certainly means a different structure for PropertyNameArray.
    switch (object->structure()->indexingType()) {
    case ALL_BLANK_INDEXING_TYPES:
    case ALL_UNDECIDED_INDEXING_TYPES:
        break;
        
    case ALL_INT32_INDEXING_TYPES:
    case ALL_CONTIGUOUS_INDEXING_TYPES: {
        Butterfly* butterfly = object->m_butterfly;
        unsigned usedLength = butterfly->publicLength();
        for (unsigned i = 0; i < usedLength; ++i) {
            if (!butterfly->contiguous()[i])
                continue;
            propertyNames.add(Identifier::from(exec, i));
        }
        break;
    }
        
    case ALL_DOUBLE_INDEXING_TYPES: {
        Butterfly* butterfly = object->m_butterfly;
        unsigned usedLength = butterfly->publicLength();
        for (unsigned i = 0; i < usedLength; ++i) {
            double value = butterfly->contiguousDouble()[i];
            if (value != value)
                continue;
            propertyNames.add(Identifier::from(exec, i));
        }
        break;
    }
        
    case ALL_ARRAY_STORAGE_INDEXING_TYPES: {
        ArrayStorage* storage = object->m_butterfly->arrayStorage();
        
        unsigned usedVectorLength = std::min(storage->length(), storage->vectorLength());
        for (unsigned i = 0; i < usedVectorLength; ++i) {
            if (storage->m_vector[i])
                propertyNames.add(Identifier::from(exec, i));
        }
        
        if (SparseArrayValueMap* map = storage->m_sparseMap.get()) {
            Vector<unsigned, 0, UnsafeVectorOverflow> keys;
            keys.reserveInitialCapacity(map->size());
            
            SparseArrayValueMap::const_iterator end = map->end();
            for (SparseArrayValueMap::const_iterator it = map->begin(); it != end; ++it) {
                if (mode == IncludeDontEnumProperties || !(it->value.attributes & DontEnum))
                    keys.uncheckedAppend(static_cast<unsigned>(it->key));
            }
            
            std::sort(keys.begin(), keys.end());
            for (unsigned i = 0; i < keys.size(); ++i)
                propertyNames.add(Identifier::from(exec, keys[i]));
        }
        break;
    }
        
    default:
        RELEASE_ASSERT_NOT_REACHED();
    }
    
    object->methodTable()->getOwnNonIndexPropertyNames(object, exec, propertyNames, mode);
}

void JSObject::getOwnNonIndexPropertyNames(JSObject* object, ExecState* exec, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    getClassPropertyNames(exec, object->classInfo(), propertyNames, mode, object->staticFunctionsReified());

    bool canCachePropertiesFromStructure = !propertyNames.size();
    object->structure()->getPropertyNamesFromStructure(exec->vm(), propertyNames, mode);

    if (canCachePropertiesFromStructure)
        propertyNames.setNumCacheableSlotsForObject(object, propertyNames.size());
}

double JSObject::toNumber(ExecState* exec) const
{
    JSValue primitive = toPrimitive(exec, PreferNumber);
    if (exec->hadException()) // should be picked up soon in Nodes.cpp
        return 0.0;
    return primitive.toNumber(exec);
}

JSString* JSObject::toString(ExecState* exec) const
{
    JSValue primitive = toPrimitive(exec, PreferString);
    if (exec->hadException())
        return jsEmptyString(exec);
    return primitive.toString(exec);
}

JSObject* JSObject::toThisObject(JSCell* cell, ExecState*)
{
    return jsCast<JSObject*>(cell);
}

void JSObject::seal(VM& vm)
{
    if (isSealed(vm))
        return;
    preventExtensions(vm);
    setStructure(vm, Structure::sealTransition(vm, structure()));
}

void JSObject::freeze(VM& vm)
{
    if (isFrozen(vm))
        return;
    preventExtensions(vm);
    setStructure(vm, Structure::freezeTransition(vm, structure()));
}

void JSObject::preventExtensions(VM& vm)
{
    enterDictionaryIndexingMode(vm);
    if (isExtensible())
        setStructure(vm, Structure::preventExtensionsTransition(vm, structure()));
}

// This presently will flatten to an uncachable dictionary; this is suitable
// for use in delete, we may want to do something different elsewhere.
void JSObject::reifyStaticFunctionsForDelete(ExecState* exec)
{
    ASSERT(!staticFunctionsReified());
    VM& vm = exec->vm();

    // If this object's ClassInfo has no static properties, then nothing to reify!
    // We can safely set the flag to avoid the expensive check again in the future.
    if (!classInfo()->hasStaticProperties()) {
        structure()->setStaticFunctionsReified();
        return;
    }

    if (!structure()->isUncacheableDictionary())
        setStructure(vm, Structure::toUncacheableDictionaryTransition(vm, structure()));

    for (const ClassInfo* info = classInfo(); info; info = info->parentClass) {
        const HashTable* hashTable = info->propHashTable(globalObject()->globalExec());
        if (!hashTable)
            continue;
        PropertySlot slot;
        for (HashTable::ConstIterator iter = hashTable->begin(vm); iter != hashTable->end(vm); ++iter) {
            if (iter->attributes() & Function)
                setUpStaticFunctionSlot(globalObject()->globalExec(), *iter, this, Identifier(&vm, iter->key()), slot);
        }
    }

    structure()->setStaticFunctionsReified();
}

bool JSObject::removeDirect(VM& vm, PropertyName propertyName)
{
    if (!isValidOffset(structure()->get(vm, propertyName)))
        return false;

    PropertyOffset offset;
    if (structure()->isUncacheableDictionary()) {
        offset = structure()->removePropertyWithoutTransition(vm, propertyName);
        if (offset == invalidOffset)
            return false;
        putDirectUndefined(offset);
        return true;
    }

    setStructure(vm, Structure::removePropertyTransition(vm, structure(), propertyName, offset));
    if (offset == invalidOffset)
        return false;
    putDirectUndefined(offset);
    return true;
}

NEVER_INLINE void JSObject::fillGetterPropertySlot(PropertySlot& slot, PropertyOffset offset)
{
    if (JSObject* getterFunction = asGetterSetter(getDirect(offset))->getter()) {
        if (!structure()->isDictionary())
            slot.setCacheableGetterSlot(this, getterFunction, offset);
        else
            slot.setGetterSlot(getterFunction);
    } else
        slot.setUndefined();
}

void JSObject::putIndexedDescriptor(ExecState* exec, SparseArrayEntry* entryInMap, PropertyDescriptor& descriptor, PropertyDescriptor& oldDescriptor)
{
    if (descriptor.isDataDescriptor()) {
        if (descriptor.value())
            entryInMap->set(exec->vm(), this, descriptor.value());
        else if (oldDescriptor.isAccessorDescriptor())
            entryInMap->set(exec->vm(), this, jsUndefined());
        entryInMap->attributes = descriptor.attributesOverridingCurrent(oldDescriptor) & ~Accessor;
        return;
    }

    if (descriptor.isAccessorDescriptor()) {
        JSObject* getter = 0;
        if (descriptor.getterPresent())
            getter = descriptor.getterObject();
        else if (oldDescriptor.isAccessorDescriptor())
            getter = oldDescriptor.getterObject();
        JSObject* setter = 0;
        if (descriptor.setterPresent())
            setter = descriptor.setterObject();
        else if (oldDescriptor.isAccessorDescriptor())
            setter = oldDescriptor.setterObject();

        GetterSetter* accessor = GetterSetter::create(exec);
        if (getter)
            accessor->setGetter(exec->vm(), getter);
        if (setter)
            accessor->setSetter(exec->vm(), setter);

        entryInMap->set(exec->vm(), this, accessor);
        entryInMap->attributes = descriptor.attributesOverridingCurrent(oldDescriptor) & ~ReadOnly;
        return;
    }

    ASSERT(descriptor.isGenericDescriptor());
    entryInMap->attributes = descriptor.attributesOverridingCurrent(oldDescriptor);
}

// Defined in ES5.1 8.12.9
bool JSObject::defineOwnIndexedProperty(ExecState* exec, unsigned index, PropertyDescriptor& descriptor, bool throwException)
{
    ASSERT(index <= MAX_ARRAY_INDEX);
    
    if (!inSparseIndexingMode()) {
        // Fast case: we're putting a regular property to a regular array
        // FIXME: this will pessimistically assume that if attributes are missing then they'll default to false
        // however if the property currently exists missing attributes will override from their current 'true'
        // state (i.e. defineOwnProperty could be used to set a value without needing to entering 'SparseMode').
        if (!descriptor.attributes()) {
            ASSERT(!descriptor.isAccessorDescriptor());
            return putDirectIndex(exec, index, descriptor.value(), 0, throwException ? PutDirectIndexShouldThrow : PutDirectIndexShouldNotThrow);
        }
        
        ensureArrayStorageExistsAndEnterDictionaryIndexingMode(exec->vm());
    }

    if (descriptor.attributes() & (ReadOnly | Accessor))
        notifyPresenceOfIndexedAccessors(exec->vm());

    SparseArrayValueMap* map = m_butterfly->arrayStorage()->m_sparseMap.get();
    RELEASE_ASSERT(map);

    // 1. Let current be the result of calling the [[GetOwnProperty]] internal method of O with property name P.
    SparseArrayValueMap::AddResult result = map->add(this, index);
    SparseArrayEntry* entryInMap = &result.iterator->value;

    // 2. Let extensible be the value of the [[Extensible]] internal property of O.
    // 3. If current is undefined and extensible is false, then Reject.
    // 4. If current is undefined and extensible is true, then
    if (result.isNewEntry) {
        if (!isExtensible()) {
            map->remove(result.iterator);
            return reject(exec, throwException, "Attempting to define property on object that is not extensible.");
        }

        // 4.a. If IsGenericDescriptor(Desc) or IsDataDescriptor(Desc) is true, then create an own data property
        // named P of object O whose [[Value]], [[Writable]], [[Enumerable]] and [[Configurable]] attribute values
        // are described by Desc. If the value of an attribute field of Desc is absent, the attribute of the newly
        // created property is set to its default value.
        // 4.b. Else, Desc must be an accessor Property Descriptor so, create an own accessor property named P of
        // object O whose [[Get]], [[Set]], [[Enumerable]] and [[Configurable]] attribute values are described by
        // Desc. If the value of an attribute field of Desc is absent, the attribute of the newly created property
        // is set to its default value.
        // 4.c. Return true.

        PropertyDescriptor defaults;
        entryInMap->setWithoutWriteBarrier(jsUndefined());
        entryInMap->attributes = DontDelete | DontEnum | ReadOnly;
        entryInMap->get(defaults);

        putIndexedDescriptor(exec, entryInMap, descriptor, defaults);
        if (index >= m_butterfly->arrayStorage()->length())
            m_butterfly->arrayStorage()->setLength(index + 1);
        return true;
    }

    // 5. Return true, if every field in Desc is absent.
    // 6. Return true, if every field in Desc also occurs in current and the value of every field in Desc is the same value as the corresponding field in current when compared using the SameValue algorithm (9.12).
    PropertyDescriptor current;
    entryInMap->get(current);
    if (descriptor.isEmpty() || descriptor.equalTo(exec, current))
        return true;

    // 7. If the [[Configurable]] field of current is false then
    if (!current.configurable()) {
        // 7.a. Reject, if the [[Configurable]] field of Desc is true.
        if (descriptor.configurablePresent() && descriptor.configurable())
            return reject(exec, throwException, "Attempting to change configurable attribute of unconfigurable property.");
        // 7.b. Reject, if the [[Enumerable]] field of Desc is present and the [[Enumerable]] fields of current and Desc are the Boolean negation of each other.
        if (descriptor.enumerablePresent() && current.enumerable() != descriptor.enumerable())
            return reject(exec, throwException, "Attempting to change enumerable attribute of unconfigurable property.");
    }

    // 8. If IsGenericDescriptor(Desc) is true, then no further validation is required.
    if (!descriptor.isGenericDescriptor()) {
        // 9. Else, if IsDataDescriptor(current) and IsDataDescriptor(Desc) have different results, then
        if (current.isDataDescriptor() != descriptor.isDataDescriptor()) {
            // 9.a. Reject, if the [[Configurable]] field of current is false.
            if (!current.configurable())
                return reject(exec, throwException, "Attempting to change access mechanism for an unconfigurable property.");
            // 9.b. If IsDataDescriptor(current) is true, then convert the property named P of object O from a
            // data property to an accessor property. Preserve the existing values of the converted property's
            // [[Configurable]] and [[Enumerable]] attributes and set the rest of the property's attributes to
            // their default values.
            // 9.c. Else, convert the property named P of object O from an accessor property to a data property.
            // Preserve the existing values of the converted property's [[Configurable]] and [[Enumerable]]
            // attributes and set the rest of the property's attributes to their default values.
        } else if (current.isDataDescriptor() && descriptor.isDataDescriptor()) {
            // 10. Else, if IsDataDescriptor(current) and IsDataDescriptor(Desc) are both true, then
            // 10.a. If the [[Configurable]] field of current is false, then
            if (!current.configurable() && !current.writable()) {
                // 10.a.i. Reject, if the [[Writable]] field of current is false and the [[Writable]] field of Desc is true.
                if (descriptor.writable())
                    return reject(exec, throwException, "Attempting to change writable attribute of unconfigurable property.");
                // 10.a.ii. If the [[Writable]] field of current is false, then
                // 10.a.ii.1. Reject, if the [[Value]] field of Desc is present and SameValue(Desc.[[Value]], current.[[Value]]) is false.
                if (descriptor.value() && !sameValue(exec, descriptor.value(), current.value()))
                    return reject(exec, throwException, "Attempting to change value of a readonly property.");
            }
            // 10.b. else, the [[Configurable]] field of current is true, so any change is acceptable.
        } else {
            ASSERT(current.isAccessorDescriptor() && current.getterPresent() && current.setterPresent());
            // 11. Else, IsAccessorDescriptor(current) and IsAccessorDescriptor(Desc) are both true so, if the [[Configurable]] field of current is false, then
            if (!current.configurable()) {
                // 11.i. Reject, if the [[Set]] field of Desc is present and SameValue(Desc.[[Set]], current.[[Set]]) is false.
                if (descriptor.setterPresent() && descriptor.setter() != current.setter())
                    return reject(exec, throwException, "Attempting to change the setter of an unconfigurable property.");
                // 11.ii. Reject, if the [[Get]] field of Desc is present and SameValue(Desc.[[Get]], current.[[Get]]) is false.
                if (descriptor.getterPresent() && descriptor.getter() != current.getter())
                    return reject(exec, throwException, "Attempting to change the getter of an unconfigurable property.");
            }
        }
    }

    // 12. For each attribute field of Desc that is present, set the correspondingly named attribute of the property named P of object O to the value of the field.
    putIndexedDescriptor(exec, entryInMap, descriptor, current);
    // 13. Return true.
    return true;
}

SparseArrayValueMap* JSObject::allocateSparseIndexMap(VM& vm)
{
    SparseArrayValueMap* result = SparseArrayValueMap::create(vm);
    arrayStorage()->m_sparseMap.set(vm, this, result);
    return result;
}

void JSObject::deallocateSparseIndexMap()
{
    if (ArrayStorage* arrayStorage = arrayStorageOrNull())
        arrayStorage->m_sparseMap.clear();
}

bool JSObject::attemptToInterceptPutByIndexOnHoleForPrototype(ExecState* exec, JSValue thisValue, unsigned i, JSValue value, bool shouldThrow)
{
    for (JSObject* current = this; ;) {
        // This has the same behavior with respect to prototypes as JSObject::put(). It only
        // allows a prototype to intercept a put if (a) the prototype declares the property
        // we're after rather than intercepting it via an override of JSObject::put(), and
        // (b) that property is declared as ReadOnly or Accessor.
        
        ArrayStorage* storage = current->arrayStorageOrNull();
        if (storage && storage->m_sparseMap) {
            SparseArrayValueMap::iterator iter = storage->m_sparseMap->find(i);
            if (iter != storage->m_sparseMap->notFound() && (iter->value.attributes & (Accessor | ReadOnly))) {
                iter->value.put(exec, thisValue, storage->m_sparseMap.get(), value, shouldThrow);
                return true;
            }
        }
        
        JSValue prototypeValue = current->prototype();
        if (prototypeValue.isNull())
            return false;
        
        current = asObject(prototypeValue);
    }
}

bool JSObject::attemptToInterceptPutByIndexOnHole(ExecState* exec, unsigned i, JSValue value, bool shouldThrow)
{
    JSValue prototypeValue = prototype();
    if (prototypeValue.isNull())
        return false;
    
    return asObject(prototypeValue)->attemptToInterceptPutByIndexOnHoleForPrototype(exec, this, i, value, shouldThrow);
}

template<IndexingType indexingShape>
void JSObject::putByIndexBeyondVectorLengthWithoutAttributes(ExecState* exec, unsigned i, JSValue value)
{
    ASSERT((structure()->indexingType() & IndexingShapeMask) == indexingShape);
    ASSERT(!indexingShouldBeSparse());
    
    // For us to get here, the index is either greater than the public length, or greater than
    // or equal to the vector length.
    ASSERT(i >= m_butterfly->vectorLength());
    
    VM& vm = exec->vm();
    
    if (i >= MAX_ARRAY_INDEX - 1
        || (i >= MIN_SPARSE_ARRAY_INDEX
            && !isDenseEnoughForVector(i, countElements<indexingShape>(m_butterfly)))
        || indexIsSufficientlyBeyondLengthForSparseMap(i, m_butterfly->vectorLength())) {
        ASSERT(i <= MAX_ARRAY_INDEX);
        ensureArrayStorageSlow(vm);
        SparseArrayValueMap* map = allocateSparseIndexMap(vm);
        map->putEntry(exec, this, i, value, false);
        ASSERT(i >= arrayStorage()->length());
        arrayStorage()->setLength(i + 1);
        return;
    }

    ensureLength(vm, i + 1);

    RELEASE_ASSERT(i < m_butterfly->vectorLength());
    switch (indexingShape) {
    case Int32Shape:
        ASSERT(value.isInt32());
        m_butterfly->contiguousInt32()[i].setWithoutWriteBarrier(value);
        break;
        
    case DoubleShape: {
        ASSERT(value.isNumber());
        double valueAsDouble = value.asNumber();
        ASSERT(valueAsDouble == valueAsDouble);
        m_butterfly->contiguousDouble()[i] = valueAsDouble;
        break;
    }
        
    case ContiguousShape:
        m_butterfly->contiguous()[i].set(vm, this, value);
        break;
        
    default:
        CRASH();
    }
}

void JSObject::putByIndexBeyondVectorLengthWithArrayStorage(ExecState* exec, unsigned i, JSValue value, bool shouldThrow, ArrayStorage* storage)
{
    VM& vm = exec->vm();

    // i should be a valid array index that is outside of the current vector.
    ASSERT(i <= MAX_ARRAY_INDEX);
    ASSERT(i >= storage->vectorLength());
    
    SparseArrayValueMap* map = storage->m_sparseMap.get();
    
    // First, handle cases where we don't currently have a sparse map.
    if (LIKELY(!map)) {
        // If the array is not extensible, we should have entered dictionary mode, and created the sparse map.
        ASSERT(isExtensible());
    
        // Update m_length if necessary.
        if (i >= storage->length())
            storage->setLength(i + 1);

        // Check that it is sensible to still be using a vector, and then try to grow the vector.
        if (LIKELY(!indexIsSufficientlyBeyondLengthForSparseMap(i, storage->vectorLength())
            && isDenseEnoughForVector(i, storage->m_numValuesInVector)
            && increaseVectorLength(vm, i + 1))) {
            // success! - reread m_storage since it has likely been reallocated, and store to the vector.
            storage = arrayStorage();
            storage->m_vector[i].set(vm, this, value);
            ++storage->m_numValuesInVector;
            return;
        }
        // We don't want to, or can't use a vector to hold this property - allocate a sparse map & add the value.
        map = allocateSparseIndexMap(exec->vm());
        map->putEntry(exec, this, i, value, shouldThrow);
        return;
    }

    // Update m_length if necessary.
    unsigned length = storage->length();
    if (i >= length) {
        // Prohibit growing the array if length is not writable.
        if (map->lengthIsReadOnly() || !isExtensible()) {
            if (shouldThrow)
                throwTypeError(exec, StrictModeReadonlyPropertyWriteError);
            return;
        }
        length = i + 1;
        storage->setLength(length);
    }

    // We are currently using a map - check whether we still want to be doing so.
    // We will continue  to use a sparse map if SparseMode is set, a vector would be too sparse, or if allocation fails.
    unsigned numValuesInArray = storage->m_numValuesInVector + map->size();
    if (map->sparseMode() || !isDenseEnoughForVector(length, numValuesInArray) || !increaseVectorLength(exec->vm(), length)) {
        map->putEntry(exec, this, i, value, shouldThrow);
        return;
    }

    // Reread m_storage after increaseVectorLength, update m_numValuesInVector.
    storage = arrayStorage();
    storage->m_numValuesInVector = numValuesInArray;

    // Copy all values from the map into the vector, and delete the map.
    WriteBarrier<Unknown>* vector = storage->m_vector;
    SparseArrayValueMap::const_iterator end = map->end();
    for (SparseArrayValueMap::const_iterator it = map->begin(); it != end; ++it)
        vector[it->key].set(vm, this, it->value.getNonSparseMode());
    deallocateSparseIndexMap();

    // Store the new property into the vector.
    WriteBarrier<Unknown>& valueSlot = vector[i];
    if (!valueSlot)
        ++storage->m_numValuesInVector;
    valueSlot.set(vm, this, value);
}

void JSObject::putByIndexBeyondVectorLength(ExecState* exec, unsigned i, JSValue value, bool shouldThrow)
{
    VM& vm = exec->vm();

    // i should be a valid array index that is outside of the current vector.
    ASSERT(i <= MAX_ARRAY_INDEX);
    
    switch (structure()->indexingType()) {
    case ALL_BLANK_INDEXING_TYPES: {
        if (indexingShouldBeSparse()) {
            putByIndexBeyondVectorLengthWithArrayStorage(
                exec, i, value, shouldThrow,
                ensureArrayStorageExistsAndEnterDictionaryIndexingMode(vm));
            break;
        }
        if (indexIsSufficientlyBeyondLengthForSparseMap(i, 0) || i >= MIN_SPARSE_ARRAY_INDEX) {
            putByIndexBeyondVectorLengthWithArrayStorage(
                exec, i, value, shouldThrow, createArrayStorage(vm, 0, 0));
            break;
        }
        if (structure()->needsSlowPutIndexing()) {
            ArrayStorage* storage = createArrayStorage(vm, i + 1, getNewVectorLength(0, 0, i + 1));
            storage->m_vector[i].set(vm, this, value);
            storage->m_numValuesInVector++;
            break;
        }
            
        createInitialContiguous(vm, i + 1)[i].set(vm, this, value);
        break;
    }
        
    case ALL_UNDECIDED_INDEXING_TYPES: {
        CRASH();
        break;
    }
        
    case ALL_INT32_INDEXING_TYPES: {
        putByIndexBeyondVectorLengthWithoutAttributes<Int32Shape>(exec, i, value);
        break;
    }
        
    case ALL_DOUBLE_INDEXING_TYPES: {
        putByIndexBeyondVectorLengthWithoutAttributes<DoubleShape>(exec, i, value);
        break;
    }
        
    case ALL_CONTIGUOUS_INDEXING_TYPES: {
        putByIndexBeyondVectorLengthWithoutAttributes<ContiguousShape>(exec, i, value);
        break;
    }
        
    case NonArrayWithSlowPutArrayStorage:
    case ArrayWithSlowPutArrayStorage: {
        // No own property present in the vector, but there might be in the sparse map!
        SparseArrayValueMap* map = arrayStorage()->m_sparseMap.get();
        if (!(map && map->contains(i)) && attemptToInterceptPutByIndexOnHole(exec, i, value, shouldThrow))
            return;
        // Otherwise, fall though.
    }

    case NonArrayWithArrayStorage:
    case ArrayWithArrayStorage:
        putByIndexBeyondVectorLengthWithArrayStorage(exec, i, value, shouldThrow, arrayStorage());
        break;
        
    default:
        RELEASE_ASSERT_NOT_REACHED();
    }
}

bool JSObject::putDirectIndexBeyondVectorLengthWithArrayStorage(ExecState* exec, unsigned i, JSValue value, unsigned attributes, PutDirectIndexMode mode, ArrayStorage* storage)
{
    VM& vm = exec->vm();
    
    // i should be a valid array index that is outside of the current vector.
    ASSERT(hasArrayStorage(structure()->indexingType()));
    ASSERT(arrayStorage() == storage);
    ASSERT(i >= storage->vectorLength() || attributes);
    ASSERT(i <= MAX_ARRAY_INDEX);

    SparseArrayValueMap* map = storage->m_sparseMap.get();

    // First, handle cases where we don't currently have a sparse map.
    if (LIKELY(!map)) {
        // If the array is not extensible, we should have entered dictionary mode, and created the spare map.
        ASSERT(isExtensible());
    
        // Update m_length if necessary.
        if (i >= storage->length())
            storage->setLength(i + 1);

        // Check that it is sensible to still be using a vector, and then try to grow the vector.
        if (LIKELY(
                !attributes
                && (isDenseEnoughForVector(i, storage->m_numValuesInVector))
                && !indexIsSufficientlyBeyondLengthForSparseMap(i, storage->vectorLength()))
                && increaseVectorLength(vm, i + 1)) {
            // success! - reread m_storage since it has likely been reallocated, and store to the vector.
            storage = arrayStorage();
            storage->m_vector[i].set(vm, this, value);
            ++storage->m_numValuesInVector;
            return true;
        }
        // We don't want to, or can't use a vector to hold this property - allocate a sparse map & add the value.
        map = allocateSparseIndexMap(exec->vm());
        return map->putDirect(exec, this, i, value, attributes, mode);
    }

    // Update m_length if necessary.
    unsigned length = storage->length();
    if (i >= length) {
        if (mode != PutDirectIndexLikePutDirect) {
            // Prohibit growing the array if length is not writable.
            if (map->lengthIsReadOnly())
                return reject(exec, mode == PutDirectIndexShouldThrow, StrictModeReadonlyPropertyWriteError);
            if (!isExtensible())
                return reject(exec, mode == PutDirectIndexShouldThrow, "Attempting to define property on object that is not extensible.");
        }
        length = i + 1;
        storage->setLength(length);
    }

    // We are currently using a map - check whether we still want to be doing so.
    // We will continue  to use a sparse map if SparseMode is set, a vector would be too sparse, or if allocation fails.
    unsigned numValuesInArray = storage->m_numValuesInVector + map->size();
    if (map->sparseMode() || attributes || !isDenseEnoughForVector(length, numValuesInArray) || !increaseVectorLength(exec->vm(), length))
        return map->putDirect(exec, this, i, value, attributes, mode);

    // Reread m_storage after increaseVectorLength, update m_numValuesInVector.
    storage = arrayStorage();
    storage->m_numValuesInVector = numValuesInArray;

    // Copy all values from the map into the vector, and delete the map.
    WriteBarrier<Unknown>* vector = storage->m_vector;
    SparseArrayValueMap::const_iterator end = map->end();
    for (SparseArrayValueMap::const_iterator it = map->begin(); it != end; ++it)
        vector[it->key].set(vm, this, it->value.getNonSparseMode());
    deallocateSparseIndexMap();

    // Store the new property into the vector.
    WriteBarrier<Unknown>& valueSlot = vector[i];
    if (!valueSlot)
        ++storage->m_numValuesInVector;
    valueSlot.set(vm, this, value);
    return true;
}

bool JSObject::putDirectIndexBeyondVectorLength(ExecState* exec, unsigned i, JSValue value, unsigned attributes, PutDirectIndexMode mode)
{
    VM& vm = exec->vm();

    // i should be a valid array index that is outside of the current vector.
    ASSERT(i <= MAX_ARRAY_INDEX);
    
    if (attributes & (ReadOnly | Accessor))
        notifyPresenceOfIndexedAccessors(vm);
    
    switch (structure()->indexingType()) {
    case ALL_BLANK_INDEXING_TYPES: {
        if (indexingShouldBeSparse() || attributes) {
            return putDirectIndexBeyondVectorLengthWithArrayStorage(
                exec, i, value, attributes, mode,
                ensureArrayStorageExistsAndEnterDictionaryIndexingMode(vm));
        }
        if (i >= MIN_SPARSE_ARRAY_INDEX) {
            return putDirectIndexBeyondVectorLengthWithArrayStorage(
                exec, i, value, attributes, mode, createArrayStorage(vm, 0, 0));
        }
        if (structure()->needsSlowPutIndexing()) {
            ArrayStorage* storage = createArrayStorage(vm, i + 1, getNewVectorLength(0, 0, i + 1));
            storage->m_vector[i].set(vm, this, value);
            storage->m_numValuesInVector++;
            return true;
        }
        
        createInitialContiguous(vm, i + 1)[i].set(vm, this, value);
        return true;
    }
        
    case ALL_UNDECIDED_INDEXING_TYPES: {
        convertUndecidedForValue(exec->vm(), value);
        // Reloop.
        return putDirectIndex(exec, i, value, attributes, mode);
    }
        
    case ALL_INT32_INDEXING_TYPES: {
        if (attributes & (ReadOnly | Accessor)) {
            return putDirectIndexBeyondVectorLengthWithArrayStorage(
                exec, i, value, attributes, mode, convertInt32ToArrayStorage(vm));
        }
        if (!value.isInt32()) {
            convertInt32ForValue(vm, value);
            return putDirectIndexBeyondVectorLength(exec, i, value, attributes, mode);
        }
        putByIndexBeyondVectorLengthWithoutAttributes<Int32Shape>(exec, i, value);
        return true;
    }
        
    case ALL_DOUBLE_INDEXING_TYPES: {
        if (attributes & (ReadOnly | Accessor)) {
            return putDirectIndexBeyondVectorLengthWithArrayStorage(
                exec, i, value, attributes, mode, convertDoubleToArrayStorage(vm));
        }
        if (!value.isNumber()) {
            convertDoubleToContiguous(vm);
            return putDirectIndexBeyondVectorLength(exec, i, value, attributes, mode);
        }
        double valueAsDouble = value.asNumber();
        if (valueAsDouble != valueAsDouble) {
            convertDoubleToContiguous(vm);
            return putDirectIndexBeyondVectorLength(exec, i, value, attributes, mode);
        }
        putByIndexBeyondVectorLengthWithoutAttributes<DoubleShape>(exec, i, value);
        return true;
    }
        
    case ALL_CONTIGUOUS_INDEXING_TYPES: {
        if (attributes & (ReadOnly | Accessor)) {
            return putDirectIndexBeyondVectorLengthWithArrayStorage(
                exec, i, value, attributes, mode, convertContiguousToArrayStorage(vm));
        }
        putByIndexBeyondVectorLengthWithoutAttributes<ContiguousShape>(exec, i, value);
        return true;
    }

    case ALL_ARRAY_STORAGE_INDEXING_TYPES:
        return putDirectIndexBeyondVectorLengthWithArrayStorage(exec, i, value, attributes, mode, arrayStorage());
        
    default:
        RELEASE_ASSERT_NOT_REACHED();
        return false;
    }
}

void JSObject::putDirectNativeFunction(ExecState* exec, JSGlobalObject* globalObject, const PropertyName& propertyName, unsigned functionLength, NativeFunction nativeFunction, Intrinsic intrinsic, unsigned attributes)
{
    StringImpl* name = propertyName.publicName();
    ASSERT(name);
    
    JSFunction* function =
        JSFunction::create(exec, globalObject, functionLength, name, nativeFunction, intrinsic);
    putDirect(exec->vm(), propertyName, function, attributes);
}

ALWAYS_INLINE unsigned JSObject::getNewVectorLength(unsigned currentVectorLength, unsigned currentLength, unsigned desiredLength)
{
    ASSERT(desiredLength <= MAX_STORAGE_VECTOR_LENGTH);

    unsigned increasedLength;
    unsigned maxInitLength = std::min(currentLength, 100000U);

    if (desiredLength < maxInitLength)
        increasedLength = maxInitLength;
    else if (!currentVectorLength)
        increasedLength = std::max(desiredLength, lastArraySize);
    else {
        increasedLength = timesThreePlusOneDividedByTwo(desiredLength);
    }

    ASSERT(increasedLength >= desiredLength);

    lastArraySize = std::min(increasedLength, FIRST_VECTOR_GROW);

    return std::min(increasedLength, MAX_STORAGE_VECTOR_LENGTH);
}

ALWAYS_INLINE unsigned JSObject::getNewVectorLength(unsigned desiredLength)
{
    unsigned vectorLength;
    unsigned length;
    
    if (hasIndexedProperties(structure()->indexingType())) {
        vectorLength = m_butterfly->vectorLength();
        length = m_butterfly->publicLength();
    } else {
        vectorLength = 0;
        length = 0;
    }

    return getNewVectorLength(vectorLength, length, desiredLength);
}

template<IndexingType indexingShape>
unsigned JSObject::countElements(Butterfly* butterfly)
{
    unsigned numValues = 0;
    for (unsigned i = butterfly->publicLength(); i--;) {
        switch (indexingShape) {
        case Int32Shape:
        case ContiguousShape:
            if (butterfly->contiguous()[i])
                numValues++;
            break;
            
        case DoubleShape: {
            double value = butterfly->contiguousDouble()[i];
            if (value == value)
                numValues++;
            break;
        }
            
        default:
            CRASH();
        }
    }
    return numValues;
}

unsigned JSObject::countElements()
{
    switch (structure()->indexingType()) {
    case ALL_BLANK_INDEXING_TYPES:
    case ALL_UNDECIDED_INDEXING_TYPES:
        return 0;
        
    case ALL_INT32_INDEXING_TYPES:
        return countElements<Int32Shape>(m_butterfly);
        
    case ALL_DOUBLE_INDEXING_TYPES:
        return countElements<DoubleShape>(m_butterfly);
        
    case ALL_CONTIGUOUS_INDEXING_TYPES:
        return countElements<ContiguousShape>(m_butterfly);
        
    default:
        CRASH();
        return 0;
    }
}

bool JSObject::increaseVectorLength(VM& vm, unsigned newLength)
{
    // This function leaves the array in an internally inconsistent state, because it does not move any values from sparse value map
    // to the vector. Callers have to account for that, because they can do it more efficiently.
    if (newLength > MAX_STORAGE_VECTOR_LENGTH)
        return false;

    ArrayStorage* storage = arrayStorage();
    
    if (newLength >= MIN_SPARSE_ARRAY_INDEX
        && !isDenseEnoughForVector(newLength, storage->m_numValuesInVector))
        return false;

    unsigned indexBias = storage->m_indexBias;
    unsigned vectorLength = storage->vectorLength();
    ASSERT(newLength > vectorLength);
    unsigned newVectorLength = getNewVectorLength(newLength);

    // Fast case - there is no precapacity. In these cases a realloc makes sense.
    if (LIKELY(!indexBias)) {
        Butterfly* newButterfly = storage->butterfly()->growArrayRight(vm, structure(), structure()->outOfLineCapacity(), true, ArrayStorage::sizeFor(vectorLength), ArrayStorage::sizeFor(newVectorLength));
        if (!newButterfly)
            return false;
        m_butterfly = newButterfly;
        newButterfly->arrayStorage()->setVectorLength(newVectorLength);
        return true;
    }
    
    // Remove some, but not all of the precapacity. Atomic decay, & capped to not overflow array length.
    unsigned newIndexBias = std::min(indexBias >> 1, MAX_STORAGE_VECTOR_LENGTH - newVectorLength);
    Butterfly* newButterfly = storage->butterfly()->resizeArray(
        vm,
        structure()->outOfLineCapacity(), true, ArrayStorage::sizeFor(vectorLength),
        newIndexBias, true, ArrayStorage::sizeFor(newVectorLength));
    if (!newButterfly)
        return false;
    
    m_butterfly = newButterfly;
    newButterfly->arrayStorage()->setVectorLength(newVectorLength);
    newButterfly->arrayStorage()->m_indexBias = newIndexBias;
    return true;
}

void JSObject::ensureLengthSlow(VM& vm, unsigned length)
{
    ASSERT(length < MAX_ARRAY_INDEX);
    ASSERT(hasContiguous(structure()->indexingType()) || hasInt32(structure()->indexingType()) || hasDouble(structure()->indexingType()) || hasUndecided(structure()->indexingType()));
    ASSERT(length > m_butterfly->vectorLength());
    
    unsigned newVectorLength = std::min(
        length << 1,
        MAX_STORAGE_VECTOR_LENGTH);
    unsigned oldVectorLength = m_butterfly->vectorLength();
    m_butterfly = m_butterfly->growArrayRight(
        vm, structure(), structure()->outOfLineCapacity(), true,
        oldVectorLength * sizeof(EncodedJSValue),
        newVectorLength * sizeof(EncodedJSValue));
    if (hasDouble(structure()->indexingType())) {
        for (unsigned i = oldVectorLength; i < newVectorLength; ++i)
            m_butterfly->contiguousDouble().data()[i] = QNaN;
    }
    m_butterfly->setVectorLength(newVectorLength);
}

Butterfly* JSObject::growOutOfLineStorage(VM& vm, size_t oldSize, size_t newSize)
{
    ASSERT(newSize > oldSize);

    // It's important that this function not rely on structure(), for the property
    // capacity, since we might have already mutated the structure in-place.
    
    return m_butterfly->growPropertyStorage(vm, structure(), oldSize, newSize);
}

bool JSObject::getOwnPropertyDescriptor(JSObject* object, ExecState* exec, PropertyName propertyName, PropertyDescriptor& descriptor)
{
    unsigned attributes = 0;
    JSCell* cell = 0;
    PropertyOffset offset = object->structure()->get(exec->vm(), propertyName, attributes, cell);
    if (isValidOffset(offset)) {
        descriptor.setDescriptor(object->getDirect(offset), attributes);
        return true;
    }
    
    unsigned i = propertyName.asIndex();
    if (i == PropertyName::NotAnIndex)
        return false;
    
    switch (object->structure()->indexingType()) {
    case ALL_BLANK_INDEXING_TYPES:
    case ALL_UNDECIDED_INDEXING_TYPES:
        return false;
        
    case ALL_INT32_INDEXING_TYPES:
    case ALL_CONTIGUOUS_INDEXING_TYPES: {
        Butterfly* butterfly = object->m_butterfly;
        if (i >= butterfly->vectorLength())
            return false;
        JSValue value = butterfly->contiguous()[i].get();
        if (!value)
            return false;
        descriptor.setDescriptor(value, 0);
        return true;
    }
        
    case ALL_DOUBLE_INDEXING_TYPES: {
        Butterfly* butterfly = object->m_butterfly;
        if (i >= butterfly->vectorLength())
            return false;
        double value = butterfly->contiguousDouble()[i];
        if (value != value)
            return false;
        descriptor.setDescriptor(JSValue(JSValue::EncodeAsDouble, value), 0);
        return true;
    }
        
    case ALL_ARRAY_STORAGE_INDEXING_TYPES: {
        ArrayStorage* storage = object->m_butterfly->arrayStorage();
        if (i >= storage->length())
            return false;
        if (i < storage->vectorLength()) {
            WriteBarrier<Unknown>& value = storage->m_vector[i];
            if (!value)
                return false;
            descriptor.setDescriptor(value.get(), 0);
            return true;
        }
        if (SparseArrayValueMap* map = storage->m_sparseMap.get()) {
            SparseArrayValueMap::iterator it = map->find(i);
            if (it == map->notFound())
                return false;
            it->value.get(descriptor);
            return true;
        }
        return false;
    }
        
    default:
        RELEASE_ASSERT_NOT_REACHED();
        return false;
    }
}

bool JSObject::getPropertyDescriptor(ExecState* exec, PropertyName propertyName, PropertyDescriptor& descriptor)
{
    JSObject* object = this;
    while (true) {
        if (object->methodTable()->getOwnPropertyDescriptor(object, exec, propertyName, descriptor))
            return true;
        JSValue prototype = object->prototype();
        if (!prototype.isObject())
            return false;
        object = asObject(prototype);
    }
}

static bool putDescriptor(ExecState* exec, JSObject* target, PropertyName propertyName, PropertyDescriptor& descriptor, unsigned attributes, const PropertyDescriptor& oldDescriptor)
{
    if (descriptor.isGenericDescriptor() || descriptor.isDataDescriptor()) {
        if (descriptor.isGenericDescriptor() && oldDescriptor.isAccessorDescriptor()) {
            GetterSetter* accessor = GetterSetter::create(exec);
            if (oldDescriptor.getterPresent())
                accessor->setGetter(exec->vm(), oldDescriptor.getterObject());
            if (oldDescriptor.setterPresent())
                accessor->setSetter(exec->vm(), oldDescriptor.setterObject());
            target->putDirectAccessor(exec, propertyName, accessor, attributes | Accessor);
            return true;
        }
        JSValue newValue = jsUndefined();
        if (descriptor.value())
            newValue = descriptor.value();
        else if (oldDescriptor.value())
            newValue = oldDescriptor.value();
        target->putDirect(exec->vm(), propertyName, newValue, attributes & ~Accessor);
        if (attributes & ReadOnly)
            target->structure()->setContainsReadOnlyProperties();
        return true;
    }
    attributes &= ~ReadOnly;
    GetterSetter* accessor = GetterSetter::create(exec);

    if (descriptor.getterPresent())
        accessor->setGetter(exec->vm(), descriptor.getterObject());
    else if (oldDescriptor.getterPresent())
        accessor->setGetter(exec->vm(), oldDescriptor.getterObject());
    if (descriptor.setterPresent())
        accessor->setSetter(exec->vm(), descriptor.setterObject());
    else if (oldDescriptor.setterPresent())
        accessor->setSetter(exec->vm(), oldDescriptor.setterObject());

    target->putDirectAccessor(exec, propertyName, accessor, attributes | Accessor);
    return true;
}

void JSObject::putDirectMayBeIndex(ExecState* exec, PropertyName propertyName, JSValue value)
{
    unsigned asIndex = propertyName.asIndex();
    if (asIndex == PropertyName::NotAnIndex)
        putDirect(exec->vm(), propertyName, value);
    else
        putDirectIndex(exec, asIndex, value);
}

class DefineOwnPropertyScope {
public:
    DefineOwnPropertyScope(ExecState* exec)
        : m_vm(exec->vm())
    {
        m_vm.setInDefineOwnProperty(true);
    }

    ~DefineOwnPropertyScope()
    {
        m_vm.setInDefineOwnProperty(false);
    }

private:
    VM& m_vm;
};

bool JSObject::defineOwnNonIndexProperty(ExecState* exec, PropertyName propertyName, PropertyDescriptor& descriptor, bool throwException)
{
    // Track on the globaldata that we're in define property.
    // Currently DefineOwnProperty uses delete to remove properties when they are being replaced
    // (particularly when changing attributes), however delete won't allow non-configurable (i.e.
    // DontDelete) properties to be deleted. For now, we can use this flag to make this work.
    DefineOwnPropertyScope scope(exec);
    
    // If we have a new property we can just put it on normally
    PropertyDescriptor current;
    if (!methodTable()->getOwnPropertyDescriptor(this, exec, propertyName, current)) {
        // unless extensions are prevented!
        if (!isExtensible()) {
            if (throwException)
                throwError(exec, createTypeError(exec, ASCIILiteral("Attempting to define property on object that is not extensible.")));
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
                throwError(exec, createTypeError(exec, ASCIILiteral("Attempting to configurable attribute of unconfigurable property.")));
            return false;
        }
        if (descriptor.enumerablePresent() && descriptor.enumerable() != current.enumerable()) {
            if (throwException)
                throwError(exec, createTypeError(exec, ASCIILiteral("Attempting to change enumerable attribute of unconfigurable property.")));
            return false;
        }
    }

    // A generic descriptor is simply changing the attributes of an existing property
    if (descriptor.isGenericDescriptor()) {
        if (!current.attributesEqual(descriptor)) {
            methodTable()->deleteProperty(this, exec, propertyName);
            return putDescriptor(exec, this, propertyName, descriptor, descriptor.attributesOverridingCurrent(current), current);
        }
        return true;
    }

    // Changing between a normal property or an accessor property
    if (descriptor.isDataDescriptor() != current.isDataDescriptor()) {
        if (!current.configurable()) {
            if (throwException)
                throwError(exec, createTypeError(exec, ASCIILiteral("Attempting to change access mechanism for an unconfigurable property.")));
            return false;
        }
        methodTable()->deleteProperty(this, exec, propertyName);
        return putDescriptor(exec, this, propertyName, descriptor, descriptor.attributesOverridingCurrent(current), current);
    }

    // Changing the value and attributes of an existing property
    if (descriptor.isDataDescriptor()) {
        if (!current.configurable()) {
            if (!current.writable() && descriptor.writable()) {
                if (throwException)
                    throwError(exec, createTypeError(exec, ASCIILiteral("Attempting to change writable attribute of unconfigurable property.")));
                return false;
            }
            if (!current.writable()) {
                if (descriptor.value() && !sameValue(exec, current.value(), descriptor.value())) {
                    if (throwException)
                        throwError(exec, createTypeError(exec, ASCIILiteral("Attempting to change value of a readonly property.")));
                    return false;
                }
            }
        }
        if (current.attributesEqual(descriptor) && !descriptor.value())
            return true;
        methodTable()->deleteProperty(this, exec, propertyName);
        return putDescriptor(exec, this, propertyName, descriptor, descriptor.attributesOverridingCurrent(current), current);
    }

    // Changing the accessor functions of an existing accessor property
    ASSERT(descriptor.isAccessorDescriptor());
    if (!current.configurable()) {
        if (descriptor.setterPresent() && !(current.setterPresent() && JSValue::strictEqual(exec, current.setter(), descriptor.setter()))) {
            if (throwException)
                throwError(exec, createTypeError(exec, ASCIILiteral("Attempting to change the setter of an unconfigurable property.")));
            return false;
        }
        if (descriptor.getterPresent() && !(current.getterPresent() && JSValue::strictEqual(exec, current.getter(), descriptor.getter()))) {
            if (throwException)
                throwError(exec, createTypeError(exec, ASCIILiteral("Attempting to change the getter of an unconfigurable property.")));
            return false;
        }
    }
    JSValue accessor = getDirect(exec->vm(), propertyName);
    if (!accessor)
        return false;
    GetterSetter* getterSetter = asGetterSetter(accessor);
    if (descriptor.setterPresent())
        getterSetter->setSetter(exec->vm(), descriptor.setterObject());
    if (descriptor.getterPresent())
        getterSetter->setGetter(exec->vm(), descriptor.getterObject());
    if (current.attributesEqual(descriptor))
        return true;
    methodTable()->deleteProperty(this, exec, propertyName);
    unsigned attrs = descriptor.attributesOverridingCurrent(current);
    putDirectAccessor(exec, propertyName, getterSetter, attrs | Accessor);
    return true;
}

bool JSObject::defineOwnProperty(JSObject* object, ExecState* exec, PropertyName propertyName, PropertyDescriptor& descriptor, bool throwException)
{
    // If it's an array index, then use the indexed property storage.
    unsigned index = propertyName.asIndex();
    if (index != PropertyName::NotAnIndex) {
        // c. Let succeeded be the result of calling the default [[DefineOwnProperty]] internal method (8.12.9) on A passing P, Desc, and false as arguments.
        // d. Reject if succeeded is false.
        // e. If index >= oldLen
        // e.i. Set oldLenDesc.[[Value]] to index + 1.
        // e.ii. Call the default [[DefineOwnProperty]] internal method (8.12.9) on A passing "length", oldLenDesc, and false as arguments. This call will always return true.
        // f. Return true.
        return object->defineOwnIndexedProperty(exec, index, descriptor, throwException);
    }
    
    return object->defineOwnNonIndexProperty(exec, propertyName, descriptor, throwException);
}

bool JSObject::getOwnPropertySlotSlow(ExecState* exec, PropertyName propertyName, PropertySlot& slot)
{
    unsigned i = propertyName.asIndex();
    if (i != PropertyName::NotAnIndex)
        return getOwnPropertySlotByIndex(this, exec, i, slot);
    return false;
}

JSObject* throwTypeError(ExecState* exec, const String& message)
{
    return throwError(exec, createTypeError(exec, message));
}

} // namespace JSC
