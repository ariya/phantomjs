/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2003, 2007, 2008, 2009, 2012 Apple Inc. All rights reserved.
 *  Copyright (C) 2003 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2006 Alexey Proskuryakov (ap@nypop.com)
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
#include "JSArray.h"

#include "ArrayPrototype.h"
#include "ButterflyInlines.h"
#include "CachedCall.h"
#include "CopiedSpace.h"
#include "CopiedSpaceInlines.h"
#include "Error.h"
#include "Executable.h"
#include "GetterSetter.h"
#include "IndexingHeaderInlines.h"
#include "PropertyNameArray.h"
#include "Reject.h"
#include <wtf/AVLTree.h>
#include <wtf/Assertions.h>
#include <wtf/OwnPtr.h>
#include <Operations.h>

using namespace std;
using namespace WTF;

namespace JSC {

ASSERT_HAS_TRIVIAL_DESTRUCTOR(JSArray);

const ClassInfo JSArray::s_info = {"Array", &JSNonFinalObject::s_info, 0, 0, CREATE_METHOD_TABLE(JSArray)};

Butterfly* createArrayButterflyInDictionaryIndexingMode(VM& vm, unsigned initialLength)
{
    Butterfly* butterfly = Butterfly::create(
        vm, 0, 0, true, IndexingHeader(), ArrayStorage::sizeFor(0));
    ArrayStorage* storage = butterfly->arrayStorage();
    storage->setLength(initialLength);
    storage->setVectorLength(0);
    storage->m_indexBias = 0;
    storage->m_sparseMap.clear();
    storage->m_numValuesInVector = 0;
    return butterfly;
}

void JSArray::setLengthWritable(ExecState* exec, bool writable)
{
    ASSERT(isLengthWritable() || !writable);
    if (!isLengthWritable() || writable)
        return;

    enterDictionaryIndexingMode(exec->vm());

    SparseArrayValueMap* map = arrayStorage()->m_sparseMap.get();
    ASSERT(map);
    map->setLengthIsReadOnly();
}

// Defined in ES5.1 15.4.5.1
bool JSArray::defineOwnProperty(JSObject* object, ExecState* exec, PropertyName propertyName, PropertyDescriptor& descriptor, bool throwException)
{
    JSArray* array = jsCast<JSArray*>(object);

    // 3. If P is "length", then
    if (propertyName == exec->propertyNames().length) {
        // All paths through length definition call the default [[DefineOwnProperty]], hence:
        // from ES5.1 8.12.9 7.a.
        if (descriptor.configurablePresent() && descriptor.configurable())
            return reject(exec, throwException, "Attempting to change configurable attribute of unconfigurable property.");
        // from ES5.1 8.12.9 7.b.
        if (descriptor.enumerablePresent() && descriptor.enumerable())
            return reject(exec, throwException, "Attempting to change enumerable attribute of unconfigurable property.");

        // a. If the [[Value]] field of Desc is absent, then
        // a.i. Return the result of calling the default [[DefineOwnProperty]] internal method (8.12.9) on A passing "length", Desc, and Throw as arguments.
        if (descriptor.isAccessorDescriptor())
            return reject(exec, throwException, "Attempting to change access mechanism for an unconfigurable property.");
        // from ES5.1 8.12.9 10.a.
        if (!array->isLengthWritable() && descriptor.writablePresent() && descriptor.writable())
            return reject(exec, throwException, "Attempting to change writable attribute of unconfigurable property.");
        // This descriptor is either just making length read-only, or changing nothing!
        if (!descriptor.value()) {
            if (descriptor.writablePresent())
                array->setLengthWritable(exec, descriptor.writable());
            return true;
        }
        
        // b. Let newLenDesc be a copy of Desc.
        // c. Let newLen be ToUint32(Desc.[[Value]]).
        unsigned newLen = descriptor.value().toUInt32(exec);
        // d. If newLen is not equal to ToNumber( Desc.[[Value]]), throw a RangeError exception.
        if (newLen != descriptor.value().toNumber(exec)) {
            throwError(exec, createRangeError(exec, "Invalid array length"));
            return false;
        }

        // Based on SameValue check in 8.12.9, this is always okay.
        if (newLen == array->length()) {
            if (descriptor.writablePresent())
                array->setLengthWritable(exec, descriptor.writable());
            return true;
        }

        // e. Set newLenDesc.[[Value] to newLen.
        // f. If newLen >= oldLen, then
        // f.i. Return the result of calling the default [[DefineOwnProperty]] internal method (8.12.9) on A passing "length", newLenDesc, and Throw as arguments.
        // g. Reject if oldLenDesc.[[Writable]] is false.
        if (!array->isLengthWritable())
            return reject(exec, throwException, "Attempting to change value of a readonly property.");
        
        // h. If newLenDesc.[[Writable]] is absent or has the value true, let newWritable be true.
        // i. Else,
        // i.i. Need to defer setting the [[Writable]] attribute to false in case any elements cannot be deleted.
        // i.ii. Let newWritable be false.
        // i.iii. Set newLenDesc.[[Writable] to true.
        // j. Let succeeded be the result of calling the default [[DefineOwnProperty]] internal method (8.12.9) on A passing "length", newLenDesc, and Throw as arguments.
        // k. If succeeded is false, return false.
        // l. While newLen < oldLen repeat,
        // l.i. Set oldLen to oldLen – 1.
        // l.ii. Let deleteSucceeded be the result of calling the [[Delete]] internal method of A passing ToString(oldLen) and false as arguments.
        // l.iii. If deleteSucceeded is false, then
        if (!array->setLength(exec, newLen, throwException)) {
            // 1. Set newLenDesc.[[Value] to oldLen+1.
            // 2. If newWritable is false, set newLenDesc.[[Writable] to false.
            // 3. Call the default [[DefineOwnProperty]] internal method (8.12.9) on A passing "length", newLenDesc, and false as arguments.
            // 4. Reject.
            if (descriptor.writablePresent())
                array->setLengthWritable(exec, descriptor.writable());
            return false;
        }

        // m. If newWritable is false, then
        // i. Call the default [[DefineOwnProperty]] internal method (8.12.9) on A passing "length",
        //    Property Descriptor{[[Writable]]: false}, and false as arguments. This call will always
        //    return true.
        if (descriptor.writablePresent())
            array->setLengthWritable(exec, descriptor.writable());
        // n. Return true.
        return true;
    }

    // 4. Else if P is an array index (15.4), then
    // a. Let index be ToUint32(P).
    unsigned index = propertyName.asIndex();
    if (index != PropertyName::NotAnIndex) {
        // b. Reject if index >= oldLen and oldLenDesc.[[Writable]] is false.
        if (index >= array->length() && !array->isLengthWritable())
            return reject(exec, throwException, "Attempting to define numeric property on array with non-writable length property.");
        // c. Let succeeded be the result of calling the default [[DefineOwnProperty]] internal method (8.12.9) on A passing P, Desc, and false as arguments.
        // d. Reject if succeeded is false.
        // e. If index >= oldLen
        // e.i. Set oldLenDesc.[[Value]] to index + 1.
        // e.ii. Call the default [[DefineOwnProperty]] internal method (8.12.9) on A passing "length", oldLenDesc, and false as arguments. This call will always return true.
        // f. Return true.
        return array->defineOwnIndexedProperty(exec, index, descriptor, throwException);
    }

    return array->JSObject::defineOwnNonIndexProperty(exec, propertyName, descriptor, throwException);
}

bool JSArray::getOwnPropertySlot(JSCell* cell, ExecState* exec, PropertyName propertyName, PropertySlot& slot)
{
    JSArray* thisObject = jsCast<JSArray*>(cell);
    if (propertyName == exec->propertyNames().length) {
        slot.setValue(jsNumber(thisObject->length()));
        return true;
    }

    return JSObject::getOwnPropertySlot(thisObject, exec, propertyName, slot);
}

bool JSArray::getOwnPropertyDescriptor(JSObject* object, ExecState* exec, PropertyName propertyName, PropertyDescriptor& descriptor)
{
    JSArray* thisObject = jsCast<JSArray*>(object);
    if (propertyName == exec->propertyNames().length) {
        descriptor.setDescriptor(jsNumber(thisObject->length()), thisObject->isLengthWritable() ? DontDelete | DontEnum : DontDelete | DontEnum | ReadOnly);
        return true;
    }

    return JSObject::getOwnPropertyDescriptor(thisObject, exec, propertyName, descriptor);
}

// ECMA 15.4.5.1
void JSArray::put(JSCell* cell, ExecState* exec, PropertyName propertyName, JSValue value, PutPropertySlot& slot)
{
    JSArray* thisObject = jsCast<JSArray*>(cell);

    if (propertyName == exec->propertyNames().length) {
        unsigned newLength = value.toUInt32(exec);
        if (value.toNumber(exec) != static_cast<double>(newLength)) {
            throwError(exec, createRangeError(exec, ASCIILiteral("Invalid array length")));
            return;
        }
        thisObject->setLength(exec, newLength, slot.isStrictMode());
        return;
    }

    JSObject::put(thisObject, exec, propertyName, value, slot);
}

bool JSArray::deleteProperty(JSCell* cell, ExecState* exec, PropertyName propertyName)
{
    JSArray* thisObject = jsCast<JSArray*>(cell);

    if (propertyName == exec->propertyNames().length)
        return false;

    return JSObject::deleteProperty(thisObject, exec, propertyName);
}

static int compareKeysForQSort(const void* a, const void* b)
{
    unsigned da = *static_cast<const unsigned*>(a);
    unsigned db = *static_cast<const unsigned*>(b);
    return (da > db) - (da < db);
}

void JSArray::getOwnNonIndexPropertyNames(JSObject* object, ExecState* exec, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    JSArray* thisObject = jsCast<JSArray*>(object);

    if (mode == IncludeDontEnumProperties)
        propertyNames.add(exec->propertyNames().length);

    JSObject::getOwnNonIndexPropertyNames(thisObject, exec, propertyNames, mode);
}

// This method makes room in the vector, but leaves the new space for count slots uncleared.
bool JSArray::unshiftCountSlowCase(VM& vm, bool addToFront, unsigned count)
{
    ArrayStorage* storage = ensureArrayStorage(vm);
    Butterfly* butterfly = storage->butterfly();
    unsigned propertyCapacity = structure()->outOfLineCapacity();
    unsigned propertySize = structure()->outOfLineSize();

    // If not, we should have handled this on the fast path.
    ASSERT(!addToFront || count > storage->m_indexBias);

    // Step 1:
    // Gather 4 key metrics:
    //  * usedVectorLength - how many entries are currently in the vector (conservative estimate - fewer may be in use in sparse vectors).
    //  * requiredVectorLength - how many entries are will there be in the vector, after allocating space for 'count' more.
    //  * currentCapacity - what is the current size of the vector, including any pre-capacity.
    //  * desiredCapacity - how large should we like to grow the vector to - based on 2x requiredVectorLength.

    unsigned length = storage->length();
    unsigned usedVectorLength = min(storage->vectorLength(), length);
    ASSERT(usedVectorLength <= MAX_STORAGE_VECTOR_LENGTH);
    // Check that required vector length is possible, in an overflow-safe fashion.
    if (count > MAX_STORAGE_VECTOR_LENGTH - usedVectorLength)
        return false;
    unsigned requiredVectorLength = usedVectorLength + count;
    ASSERT(requiredVectorLength <= MAX_STORAGE_VECTOR_LENGTH);
    // The sum of m_vectorLength and m_indexBias will never exceed MAX_STORAGE_VECTOR_LENGTH.
    ASSERT(storage->vectorLength() <= MAX_STORAGE_VECTOR_LENGTH && (MAX_STORAGE_VECTOR_LENGTH - storage->vectorLength()) >= storage->m_indexBias);
    unsigned currentCapacity = storage->vectorLength() + storage->m_indexBias;
    // The calculation of desiredCapacity won't overflow, due to the range of MAX_STORAGE_VECTOR_LENGTH.
    unsigned desiredCapacity = min(MAX_STORAGE_VECTOR_LENGTH, max(BASE_VECTOR_LEN, requiredVectorLength) << 1);

    // Step 2:
    // We're either going to choose to allocate a new ArrayStorage, or we're going to reuse the existing one.

    void* newAllocBase = 0;
    unsigned newStorageCapacity;
    // If the current storage array is sufficiently large (but not too large!) then just keep using it.
    if (currentCapacity > desiredCapacity && isDenseEnoughForVector(currentCapacity, requiredVectorLength)) {
        newAllocBase = butterfly->base(structure());
        newStorageCapacity = currentCapacity;
    } else {
        size_t newSize = Butterfly::totalSize(0, propertyCapacity, true, ArrayStorage::sizeFor(desiredCapacity));
        if (!vm.heap.tryAllocateStorage(newSize, &newAllocBase))
            return false;
        newStorageCapacity = desiredCapacity;
    }

    // Step 3:
    // Work out where we're going to move things to.

    // Determine how much of the vector to use as pre-capacity, and how much as post-capacity.
    // If we're adding to the end, we'll add all the new space to the end.
    // If the vector had no free post-capacity (length >= m_vectorLength), don't give it any.
    // If it did, we calculate the amount that will remain based on an atomic decay - leave the
    // vector with half the post-capacity it had previously.
    unsigned postCapacity = 0;
    if (!addToFront)
        postCapacity = max(newStorageCapacity - requiredVectorLength, count);
    else if (length < storage->vectorLength()) {
        // Atomic decay, + the post-capacity cannot be greater than what is available.
        postCapacity = min((storage->vectorLength() - length) >> 1, newStorageCapacity - requiredVectorLength);
        // If we're moving contents within the same allocation, the post-capacity is being reduced.
        ASSERT(newAllocBase != butterfly->base(structure()) || postCapacity < storage->vectorLength() - length);
    }

    unsigned newVectorLength = requiredVectorLength + postCapacity;
    unsigned newIndexBias = newStorageCapacity - newVectorLength;

    Butterfly* newButterfly = Butterfly::fromBase(newAllocBase, newIndexBias, propertyCapacity);

    if (addToFront) {
        ASSERT(count + usedVectorLength <= newVectorLength);
        memmove(newButterfly->arrayStorage()->m_vector + count, storage->m_vector, sizeof(JSValue) * usedVectorLength);
        memmove(newButterfly->propertyStorage() - propertySize, butterfly->propertyStorage() - propertySize, sizeof(JSValue) * propertySize + sizeof(IndexingHeader) + ArrayStorage::sizeFor(0));
    } else if ((newAllocBase != butterfly->base(structure())) || (newIndexBias != storage->m_indexBias)) {
        memmove(newButterfly->propertyStorage() - propertySize, butterfly->propertyStorage() - propertySize, sizeof(JSValue) * propertySize + sizeof(IndexingHeader) + ArrayStorage::sizeFor(0));
        memmove(newButterfly->arrayStorage()->m_vector, storage->m_vector, sizeof(JSValue) * usedVectorLength);

        WriteBarrier<Unknown>* newVector = newButterfly->arrayStorage()->m_vector;
        for (unsigned i = requiredVectorLength; i < newVectorLength; i++)
            newVector[i].clear();
    }

    newButterfly->arrayStorage()->setVectorLength(newVectorLength);
    newButterfly->arrayStorage()->m_indexBias = newIndexBias;

    m_butterfly = newButterfly;

    return true;
}

bool JSArray::setLengthWithArrayStorage(ExecState* exec, unsigned newLength, bool throwException, ArrayStorage* storage)
{
    unsigned length = storage->length();

    // If the length is read only then we enter sparse mode, so should enter the following 'if'.
    ASSERT(isLengthWritable() || storage->m_sparseMap);

    if (SparseArrayValueMap* map = storage->m_sparseMap.get()) {
        // Fail if the length is not writable.
        if (map->lengthIsReadOnly())
            return reject(exec, throwException, StrictModeReadonlyPropertyWriteError);

        if (newLength < length) {
            // Copy any keys we might be interested in into a vector.
            Vector<unsigned, 0, UnsafeVectorOverflow> keys;
            keys.reserveInitialCapacity(min(map->size(), static_cast<size_t>(length - newLength)));
            SparseArrayValueMap::const_iterator end = map->end();
            for (SparseArrayValueMap::const_iterator it = map->begin(); it != end; ++it) {
                unsigned index = static_cast<unsigned>(it->key);
                if (index < length && index >= newLength)
                    keys.append(index);
            }

            // Check if the array is in sparse mode. If so there may be non-configurable
            // properties, so we have to perform deletion with caution, if not we can
            // delete values in any order.
            if (map->sparseMode()) {
                qsort(keys.begin(), keys.size(), sizeof(unsigned), compareKeysForQSort);
                unsigned i = keys.size();
                while (i) {
                    unsigned index = keys[--i];
                    SparseArrayValueMap::iterator it = map->find(index);
                    ASSERT(it != map->notFound());
                    if (it->value.attributes & DontDelete) {
                        storage->setLength(index + 1);
                        return reject(exec, throwException, "Unable to delete property.");
                    }
                    map->remove(it);
                }
            } else {
                for (unsigned i = 0; i < keys.size(); ++i)
                    map->remove(keys[i]);
                if (map->isEmpty())
                    deallocateSparseIndexMap();
            }
        }
    }

    if (newLength < length) {
        // Delete properties from the vector.
        unsigned usedVectorLength = min(length, storage->vectorLength());
        for (unsigned i = newLength; i < usedVectorLength; ++i) {
            WriteBarrier<Unknown>& valueSlot = storage->m_vector[i];
            bool hadValue = valueSlot;
            valueSlot.clear();
            storage->m_numValuesInVector -= hadValue;
        }
    }

    storage->setLength(newLength);

    return true;
}

bool JSArray::setLength(ExecState* exec, unsigned newLength, bool throwException)
{
    switch (structure()->indexingType()) {
    case ArrayClass:
        if (!newLength)
            return true;
        if (newLength >= MIN_SPARSE_ARRAY_INDEX) {
            return setLengthWithArrayStorage(
                exec, newLength, throwException,
                convertContiguousToArrayStorage(exec->vm()));
        }
        createInitialUndecided(exec->vm(), newLength);
        return true;
        
    case ArrayWithUndecided:
    case ArrayWithInt32:
    case ArrayWithDouble:
    case ArrayWithContiguous:
        if (newLength == m_butterfly->publicLength())
            return true;
        if (newLength >= MAX_ARRAY_INDEX // This case ensures that we can do fast push.
            || (newLength >= MIN_SPARSE_ARRAY_INDEX
                && !isDenseEnoughForVector(newLength, countElements()))) {
            return setLengthWithArrayStorage(
                exec, newLength, throwException,
                ensureArrayStorage(exec->vm()));
        }
        if (newLength > m_butterfly->publicLength()) {
            ensureLength(exec->vm(), newLength);
            return true;
        }
        if (structure()->indexingType() == ArrayWithDouble) {
            for (unsigned i = m_butterfly->publicLength(); i-- > newLength;)
                m_butterfly->contiguousDouble()[i] = QNaN;
        } else {
            for (unsigned i = m_butterfly->publicLength(); i-- > newLength;)
                m_butterfly->contiguous()[i].clear();
        }
        m_butterfly->setPublicLength(newLength);
        return true;
        
    case ArrayWithArrayStorage:
    case ArrayWithSlowPutArrayStorage:
        return setLengthWithArrayStorage(exec, newLength, throwException, arrayStorage());
        
    default:
        CRASH();
        return false;
    }
}

JSValue JSArray::pop(ExecState* exec)
{
    switch (structure()->indexingType()) {
    case ArrayClass:
        return jsUndefined();
        
    case ArrayWithUndecided:
        if (!m_butterfly->publicLength())
            return jsUndefined();
        // We have nothing but holes. So, drop down to the slow version.
        break;
        
    case ArrayWithInt32:
    case ArrayWithContiguous: {
        unsigned length = m_butterfly->publicLength();
        
        if (!length--)
            return jsUndefined();
        
        RELEASE_ASSERT(length < m_butterfly->vectorLength());
        JSValue value = m_butterfly->contiguous()[length].get();
        if (value) {
            m_butterfly->contiguous()[length].clear();
            m_butterfly->setPublicLength(length);
            return value;
        }
        break;
    }
        
    case ArrayWithDouble: {
        unsigned length = m_butterfly->publicLength();
        
        if (!length--)
            return jsUndefined();
        
        RELEASE_ASSERT(length < m_butterfly->vectorLength());
        double value = m_butterfly->contiguousDouble()[length];
        if (value == value) {
            m_butterfly->contiguousDouble()[length] = QNaN;
            m_butterfly->setPublicLength(length);
            return JSValue(JSValue::EncodeAsDouble, value);
        }
        break;
    }
        
    case ARRAY_WITH_ARRAY_STORAGE_INDEXING_TYPES: {
        ArrayStorage* storage = m_butterfly->arrayStorage();
    
        unsigned length = storage->length();
        if (!length) {
            if (!isLengthWritable())
                throwTypeError(exec, StrictModeReadonlyPropertyWriteError);
            return jsUndefined();
        }

        unsigned index = length - 1;
        if (index < storage->vectorLength()) {
            WriteBarrier<Unknown>& valueSlot = storage->m_vector[index];
            if (valueSlot) {
                --storage->m_numValuesInVector;
                JSValue element = valueSlot.get();
                valueSlot.clear();
            
                RELEASE_ASSERT(isLengthWritable());
                storage->setLength(index);
                return element;
            }
        }
        break;
    }
        
    default:
        CRASH();
        return JSValue();
    }
    
    unsigned index = getArrayLength() - 1;
    // Let element be the result of calling the [[Get]] internal method of O with argument indx.
    JSValue element = get(exec, index);
    if (exec->hadException())
        return jsUndefined();
    // Call the [[Delete]] internal method of O with arguments indx and true.
    if (!deletePropertyByIndex(this, exec, index)) {
        throwTypeError(exec, "Unable to delete property.");
        return jsUndefined();
    }
    // Call the [[Put]] internal method of O with arguments "length", indx, and true.
    setLength(exec, index, true);
    // Return element.
    return element;
}

// Push & putIndex are almost identical, with two small differences.
//  - we always are writing beyond the current array bounds, so it is always necessary to update m_length & m_numValuesInVector.
//  - pushing to an array of length 2^32-1 stores the property, but throws a range error.
void JSArray::push(ExecState* exec, JSValue value)
{
    switch (structure()->indexingType()) {
    case ArrayClass: {
        createInitialUndecided(exec->vm(), 0);
        // Fall through.
    }
        
    case ArrayWithUndecided: {
        convertUndecidedForValue(exec->vm(), value);
        push(exec, value);
        return;
    }
        
    case ArrayWithInt32: {
        if (!value.isInt32()) {
            convertInt32ForValue(exec->vm(), value);
            push(exec, value);
            return;
        }

        unsigned length = m_butterfly->publicLength();
        ASSERT(length <= m_butterfly->vectorLength());
        if (length < m_butterfly->vectorLength()) {
            m_butterfly->contiguousInt32()[length].setWithoutWriteBarrier(value);
            m_butterfly->setPublicLength(length + 1);
            return;
        }
        
        if (length > MAX_ARRAY_INDEX) {
            methodTable()->putByIndex(this, exec, length, value, true);
            if (!exec->hadException())
                throwError(exec, createRangeError(exec, "Invalid array length"));
            return;
        }
        
        putByIndexBeyondVectorLengthWithoutAttributes<Int32Shape>(exec, length, value);
        return;
    }

    case ArrayWithContiguous: {
        unsigned length = m_butterfly->publicLength();
        ASSERT(length <= m_butterfly->vectorLength());
        if (length < m_butterfly->vectorLength()) {
            m_butterfly->contiguous()[length].set(exec->vm(), this, value);
            m_butterfly->setPublicLength(length + 1);
            return;
        }
        
        if (length > MAX_ARRAY_INDEX) {
            methodTable()->putByIndex(this, exec, length, value, true);
            if (!exec->hadException())
                throwError(exec, createRangeError(exec, "Invalid array length"));
            return;
        }
        
        putByIndexBeyondVectorLengthWithoutAttributes<ContiguousShape>(exec, length, value);
        return;
    }
        
    case ArrayWithDouble: {
        if (!value.isNumber()) {
            convertDoubleToContiguous(exec->vm());
            push(exec, value);
            return;
        }
        double valueAsDouble = value.asNumber();
        if (valueAsDouble != valueAsDouble) {
            convertDoubleToContiguous(exec->vm());
            push(exec, value);
            return;
        }

        unsigned length = m_butterfly->publicLength();
        ASSERT(length <= m_butterfly->vectorLength());
        if (length < m_butterfly->vectorLength()) {
            m_butterfly->contiguousDouble()[length] = valueAsDouble;
            m_butterfly->setPublicLength(length + 1);
            return;
        }
        
        if (length > MAX_ARRAY_INDEX) {
            methodTable()->putByIndex(this, exec, length, value, true);
            if (!exec->hadException())
                throwError(exec, createRangeError(exec, "Invalid array length"));
            return;
        }
        
        putByIndexBeyondVectorLengthWithoutAttributes<DoubleShape>(exec, length, value);
        break;
    }
        
    case ArrayWithSlowPutArrayStorage: {
        unsigned oldLength = length();
        if (attemptToInterceptPutByIndexOnHole(exec, oldLength, value, true)) {
            if (!exec->hadException() && oldLength < 0xFFFFFFFFu)
                setLength(exec, oldLength + 1, true);
            return;
        }
        // Fall through.
    }
        
    case ArrayWithArrayStorage: {
        ArrayStorage* storage = m_butterfly->arrayStorage();

        // Fast case - push within vector, always update m_length & m_numValuesInVector.
        unsigned length = storage->length();
        if (length < storage->vectorLength()) {
            storage->m_vector[length].set(exec->vm(), this, value);
            storage->setLength(length + 1);
            ++storage->m_numValuesInVector;
            return;
        }

        // Pushing to an array of invalid length (2^31-1) stores the property, but throws a range error.
        if (storage->length() > MAX_ARRAY_INDEX) {
            methodTable()->putByIndex(this, exec, storage->length(), value, true);
            // Per ES5.1 15.4.4.7 step 6 & 15.4.5.1 step 3.d.
            if (!exec->hadException())
                throwError(exec, createRangeError(exec, "Invalid array length"));
            return;
        }

        // Handled the same as putIndex.
        putByIndexBeyondVectorLengthWithArrayStorage(exec, storage->length(), value, true, storage);
        break;
    }
        
    default:
        RELEASE_ASSERT_NOT_REACHED();
    }
}

bool JSArray::shiftCountWithArrayStorage(unsigned startIndex, unsigned count, ArrayStorage* storage)
{
    unsigned oldLength = storage->length();
    RELEASE_ASSERT(count <= oldLength);
    
    // If the array contains holes or is otherwise in an abnormal state,
    // use the generic algorithm in ArrayPrototype.
    if (oldLength != storage->m_numValuesInVector || inSparseIndexingMode() || shouldUseSlowPut(structure()->indexingType()))
        return false;

    if (!oldLength)
        return true;
    
    unsigned length = oldLength - count;
    
    storage->m_numValuesInVector -= count;
    storage->setLength(length);
    
    unsigned vectorLength = storage->vectorLength();
    if (!vectorLength)
        return true;
    
    if (startIndex >= vectorLength)
        return true;
    
    if (startIndex + count > vectorLength)
        count = vectorLength - startIndex;
    
    unsigned usedVectorLength = min(vectorLength, oldLength);
    
    vectorLength -= count;
    storage->setVectorLength(vectorLength);
    
    if (vectorLength) {
        if (startIndex < usedVectorLength - (startIndex + count)) {
            if (startIndex) {
                memmove(
                    storage->m_vector + count,
                    storage->m_vector,
                    sizeof(JSValue) * startIndex);
            }
            m_butterfly = m_butterfly->shift(structure(), count);
            storage = m_butterfly->arrayStorage();
            storage->m_indexBias += count;
        } else {
            memmove(
                storage->m_vector + startIndex,
                storage->m_vector + startIndex + count,
                sizeof(JSValue) * (usedVectorLength - (startIndex + count)));
            for (unsigned i = usedVectorLength - count; i < usedVectorLength; ++i)
                storage->m_vector[i].clear();
        }
    }
    return true;
}

bool JSArray::shiftCountWithAnyIndexingType(ExecState* exec, unsigned startIndex, unsigned count)
{
    RELEASE_ASSERT(count > 0);
    
    switch (structure()->indexingType()) {
    case ArrayClass:
        return true;
        
    case ArrayWithUndecided:
        // Don't handle this because it's confusing and it shouldn't come up.
        return false;
        
    case ArrayWithInt32:
    case ArrayWithContiguous: {
        unsigned oldLength = m_butterfly->publicLength();
        RELEASE_ASSERT(count <= oldLength);
        
        // We may have to walk the entire array to do the shift. We're willing to do
        // so only if it's not horribly slow.
        if (oldLength - (startIndex + count) >= MIN_SPARSE_ARRAY_INDEX)
            return shiftCountWithArrayStorage(startIndex, count, ensureArrayStorage(exec->vm()));
        
        unsigned end = oldLength - count;
        for (unsigned i = startIndex; i < end; ++i) {
            // Storing to a hole is fine since we're still having a good time. But reading
            // from a hole is totally not fine, since we might have to read from the proto
            // chain.
            JSValue v = m_butterfly->contiguous()[i + count].get();
            if (UNLIKELY(!v)) {
                // The purpose of this path is to ensure that we don't make the same
                // mistake in the future: shiftCountWithArrayStorage() can't do anything
                // about holes (at least for now), but it can detect them quickly. So
                // we convert to array storage and then allow the array storage path to
                // figure it out.
                return shiftCountWithArrayStorage(startIndex, count, ensureArrayStorage(exec->vm()));
            }
            // No need for a barrier since we're just moving data around in the same vector.
            // This is in line with our standing assumption that we won't have a deletion
            // barrier.
            m_butterfly->contiguous()[i].setWithoutWriteBarrier(v);
        }
        for (unsigned i = end; i < oldLength; ++i)
            m_butterfly->contiguous()[i].clear();
        
        m_butterfly->setPublicLength(oldLength - count);
        return true;
    }
        
    case ArrayWithDouble: {
        unsigned oldLength = m_butterfly->publicLength();
        RELEASE_ASSERT(count <= oldLength);
        
        // We may have to walk the entire array to do the shift. We're willing to do
        // so only if it's not horribly slow.
        if (oldLength - (startIndex + count) >= MIN_SPARSE_ARRAY_INDEX)
            return shiftCountWithArrayStorage(startIndex, count, ensureArrayStorage(exec->vm()));
        
        unsigned end = oldLength - count;
        for (unsigned i = startIndex; i < end; ++i) {
            // Storing to a hole is fine since we're still having a good time. But reading
            // from a hole is totally not fine, since we might have to read from the proto
            // chain.
            double v = m_butterfly->contiguousDouble()[i + count];
            if (UNLIKELY(v != v)) {
                // The purpose of this path is to ensure that we don't make the same
                // mistake in the future: shiftCountWithArrayStorage() can't do anything
                // about holes (at least for now), but it can detect them quickly. So
                // we convert to array storage and then allow the array storage path to
                // figure it out.
                return shiftCountWithArrayStorage(startIndex, count, ensureArrayStorage(exec->vm()));
            }
            // No need for a barrier since we're just moving data around in the same vector.
            // This is in line with our standing assumption that we won't have a deletion
            // barrier.
            m_butterfly->contiguousDouble()[i] = v;
        }
        for (unsigned i = end; i < oldLength; ++i)
            m_butterfly->contiguousDouble()[i] = QNaN;
        
        m_butterfly->setPublicLength(oldLength - count);
        return true;
    }
        
    case ArrayWithArrayStorage:
    case ArrayWithSlowPutArrayStorage:
        return shiftCountWithArrayStorage(startIndex, count, arrayStorage());
        
    default:
        CRASH();
        return false;
    }
}

// Returns true if the unshift can be handled, false to fallback.    
bool JSArray::unshiftCountWithArrayStorage(ExecState* exec, unsigned startIndex, unsigned count, ArrayStorage* storage)
{
    unsigned length = storage->length();

    RELEASE_ASSERT(startIndex <= length);

    // If the array contains holes or is otherwise in an abnormal state,
    // use the generic algorithm in ArrayPrototype.
    if (length != storage->m_numValuesInVector || storage->inSparseMode() || shouldUseSlowPut(structure()->indexingType()))
        return false;

    bool moveFront = !startIndex || startIndex < length / 2;

    unsigned vectorLength = storage->vectorLength();

    if (moveFront && storage->m_indexBias >= count) {
        m_butterfly = storage->butterfly()->unshift(structure(), count);
        storage = m_butterfly->arrayStorage();
        storage->m_indexBias -= count;
        storage->setVectorLength(vectorLength + count);
    } else if (!moveFront && vectorLength - length >= count)
        storage = storage->butterfly()->arrayStorage();
    else if (unshiftCountSlowCase(exec->vm(), moveFront, count))
        storage = arrayStorage();
    else {
        throwOutOfMemoryError(exec);
        return true;
    }

    WriteBarrier<Unknown>* vector = storage->m_vector;

    if (startIndex) {
        if (moveFront)
            memmove(vector, vector + count, startIndex * sizeof(JSValue));
        else if (length - startIndex)
            memmove(vector + startIndex + count, vector + startIndex, (length - startIndex) * sizeof(JSValue));
    }

    for (unsigned i = 0; i < count; i++)
        vector[i + startIndex].clear();
    return true;
}

bool JSArray::unshiftCountWithAnyIndexingType(ExecState* exec, unsigned startIndex, unsigned count)
{
    switch (structure()->indexingType()) {
    case ArrayClass:
    case ArrayWithUndecided:
        // We could handle this. But it shouldn't ever come up, so we won't.
        return false;

    case ArrayWithInt32:
    case ArrayWithContiguous: {
        unsigned oldLength = m_butterfly->publicLength();
        
        // We may have to walk the entire array to do the unshift. We're willing to do so
        // only if it's not horribly slow.
        if (oldLength - startIndex >= MIN_SPARSE_ARRAY_INDEX)
            return unshiftCountWithArrayStorage(exec, startIndex, count, ensureArrayStorage(exec->vm()));
        
        ensureLength(exec->vm(), oldLength + count);
        
        for (unsigned i = oldLength; i-- > startIndex;) {
            JSValue v = m_butterfly->contiguous()[i].get();
            if (UNLIKELY(!v))
                return unshiftCountWithArrayStorage(exec, startIndex, count, ensureArrayStorage(exec->vm()));
            m_butterfly->contiguous()[i + count].setWithoutWriteBarrier(v);
        }
        
        // NOTE: we're leaving being garbage in the part of the array that we shifted out
        // of. This is fine because the caller is required to store over that area, and
        // in contiguous mode storing into a hole is guaranteed to behave exactly the same
        // as storing over an existing element.
        
        return true;
    }
        
    case ArrayWithDouble: {
        unsigned oldLength = m_butterfly->publicLength();
        
        // We may have to walk the entire array to do the unshift. We're willing to do so
        // only if it's not horribly slow.
        if (oldLength - startIndex >= MIN_SPARSE_ARRAY_INDEX)
            return unshiftCountWithArrayStorage(exec, startIndex, count, ensureArrayStorage(exec->vm()));
        
        ensureLength(exec->vm(), oldLength + count);
        
        for (unsigned i = oldLength; i-- > startIndex;) {
            double v = m_butterfly->contiguousDouble()[i];
            if (UNLIKELY(v != v))
                return unshiftCountWithArrayStorage(exec, startIndex, count, ensureArrayStorage(exec->vm()));
            m_butterfly->contiguousDouble()[i + count] = v;
        }
        
        // NOTE: we're leaving being garbage in the part of the array that we shifted out
        // of. This is fine because the caller is required to store over that area, and
        // in contiguous mode storing into a hole is guaranteed to behave exactly the same
        // as storing over an existing element.
        
        return true;
    }
        
    case ArrayWithArrayStorage:
    case ArrayWithSlowPutArrayStorage:
        return unshiftCountWithArrayStorage(exec, startIndex, count, arrayStorage());
        
    default:
        CRASH();
        return false;
    }
}

static int compareNumbersForQSortWithInt32(const void* a, const void* b)
{
    int32_t ia = static_cast<const JSValue*>(a)->asInt32();
    int32_t ib = static_cast<const JSValue*>(b)->asInt32();
    return ia - ib;
}

static int compareNumbersForQSortWithDouble(const void* a, const void* b)
{
    double da = *static_cast<const double*>(a);
    double db = *static_cast<const double*>(b);
    return (da > db) - (da < db);
}

static int compareNumbersForQSort(const void* a, const void* b)
{
    double da = static_cast<const JSValue*>(a)->asNumber();
    double db = static_cast<const JSValue*>(b)->asNumber();
    return (da > db) - (da < db);
}

static int compareByStringPairForQSort(const void* a, const void* b)
{
    const ValueStringPair* va = static_cast<const ValueStringPair*>(a);
    const ValueStringPair* vb = static_cast<const ValueStringPair*>(b);
    return codePointCompare(va->second, vb->second);
}

template<IndexingType indexingType>
void JSArray::sortNumericVector(ExecState* exec, JSValue compareFunction, CallType callType, const CallData& callData)
{
    ASSERT(indexingType == ArrayWithInt32 || indexingType == ArrayWithDouble || indexingType == ArrayWithContiguous || indexingType == ArrayWithArrayStorage);
    
    unsigned lengthNotIncludingUndefined;
    unsigned newRelevantLength;
    compactForSorting<indexingType>(
        lengthNotIncludingUndefined,
        newRelevantLength);
    
    ContiguousJSValues data = indexingData<indexingType>();
    
    if (indexingType == ArrayWithArrayStorage && arrayStorage()->m_sparseMap.get()) {
        throwOutOfMemoryError(exec);
        return;
    }
    
    if (!lengthNotIncludingUndefined)
        return;
    
    bool allValuesAreNumbers = true;
    switch (indexingType) {
    case ArrayWithInt32:
    case ArrayWithDouble:
        break;
        
    default:
        for (size_t i = 0; i < newRelevantLength; ++i) {
            if (!data[i].isNumber()) {
                allValuesAreNumbers = false;
                break;
            }
        }
        break;
    }
    
    if (!allValuesAreNumbers)
        return sort(exec, compareFunction, callType, callData);
    
    // For numeric comparison, which is fast, qsort is faster than mergesort. We
    // also don't require mergesort's stability, since there's no user visible
    // side-effect from swapping the order of equal primitive values.
    int (*compare)(const void*, const void*);
    switch (indexingType) {
    case ArrayWithInt32:
        compare = compareNumbersForQSortWithInt32;
        break;
        
    case ArrayWithDouble:
        compare = compareNumbersForQSortWithDouble;
        ASSERT(sizeof(WriteBarrier<Unknown>) == sizeof(double));
        break;
        
    default:
        compare = compareNumbersForQSort;
        break;
    }
    ASSERT(data.length() >= newRelevantLength);
    qsort(data.data(), newRelevantLength, sizeof(WriteBarrier<Unknown>), compare);
    return;
}

void JSArray::sortNumeric(ExecState* exec, JSValue compareFunction, CallType callType, const CallData& callData)
{
    ASSERT(!inSparseIndexingMode());

    switch (structure()->indexingType()) {
    case ArrayClass:
        return;
        
    case ArrayWithInt32:
        sortNumericVector<ArrayWithInt32>(exec, compareFunction, callType, callData);
        break;
        
    case ArrayWithDouble:
        sortNumericVector<ArrayWithDouble>(exec, compareFunction, callType, callData);
        break;
        
    case ArrayWithContiguous:
        sortNumericVector<ArrayWithContiguous>(exec, compareFunction, callType, callData);
        return;

    case ArrayWithArrayStorage:
        sortNumericVector<ArrayWithArrayStorage>(exec, compareFunction, callType, callData);
        return;
        
    default:
        CRASH();
        return;
    }
}

template <IndexingType> struct ContiguousTypeAccessor {
    typedef WriteBarrier<Unknown> Type;
    static JSValue getAsValue(ContiguousData<Type> data, size_t i) { return data[i].get(); }
    static void setWithValue(VM& vm, JSArray* thisValue, ContiguousData<Type> data, size_t i, JSValue value)
    {
        data[i].set(vm, thisValue, value);
    }
    static void replaceDataReference(ContiguousData<Type>* outData, ContiguousJSValues inData)
    {
        *outData = inData;
    }
};

template <> struct ContiguousTypeAccessor<ArrayWithDouble> {
    typedef double Type;
    static JSValue getAsValue(ContiguousData<Type> data, size_t i) { ASSERT(data[i] == data[i]); return JSValue(JSValue::EncodeAsDouble, data[i]); }
    static void setWithValue(VM&, JSArray*, ContiguousData<Type> data, size_t i, JSValue value)
    {
        data[i] = value.asNumber();
    }
    static NO_RETURN_DUE_TO_CRASH void replaceDataReference(ContiguousData<Type>*, ContiguousJSValues)
    {
        RELEASE_ASSERT_WITH_MESSAGE(0, "Inconsistent indexing types during compact array sort.");
    }
};


template<IndexingType indexingType, typename StorageType>
void JSArray::sortCompactedVector(ExecState* exec, ContiguousData<StorageType> data, unsigned relevantLength)
{
    if (!relevantLength)
        return;
    
    VM& vm = exec->vm();

    // Converting JavaScript values to strings can be expensive, so we do it once up front and sort based on that.
    // This is a considerable improvement over doing it twice per comparison, though it requires a large temporary
    // buffer. Besides, this protects us from crashing if some objects have custom toString methods that return
    // random or otherwise changing results, effectively making compare function inconsistent.
        
    Vector<ValueStringPair, 0, UnsafeVectorOverflow> values(relevantLength);
    if (!values.begin()) {
        throwOutOfMemoryError(exec);
        return;
    }
        
    Heap::heap(this)->pushTempSortVector(&values);
        
    bool isSortingPrimitiveValues = true;

    for (size_t i = 0; i < relevantLength; i++) {
        JSValue value = ContiguousTypeAccessor<indexingType>::getAsValue(data, i);
        ASSERT(indexingType != ArrayWithInt32 || value.isInt32());
        ASSERT(!value.isUndefined());
        values[i].first = value;
        if (indexingType != ArrayWithDouble && indexingType != ArrayWithInt32)
            isSortingPrimitiveValues = isSortingPrimitiveValues && value.isPrimitive();
    }
        
    // FIXME: The following loop continues to call toString on subsequent values even after
    // a toString call raises an exception.
        
    for (size_t i = 0; i < relevantLength; i++)
        values[i].second = values[i].first.toWTFStringInline(exec);
        
    if (exec->hadException()) {
        Heap::heap(this)->popTempSortVector(&values);
        return;
    }
        
    // FIXME: Since we sort by string value, a fast algorithm might be to use a radix sort. That would be O(N) rather
    // than O(N log N).
        
#if HAVE(MERGESORT)
    if (isSortingPrimitiveValues)
        qsort(values.begin(), values.size(), sizeof(ValueStringPair), compareByStringPairForQSort);
    else
        mergesort(values.begin(), values.size(), sizeof(ValueStringPair), compareByStringPairForQSort);
#else
    // FIXME: The qsort library function is likely to not be a stable sort.
    // ECMAScript-262 does not specify a stable sort, but in practice, browsers perform a stable sort.
    qsort(values.begin(), values.size(), sizeof(ValueStringPair), compareByStringPairForQSort);
#endif
    
    // If the toString function changed the length of the array or vector storage,
    // increase the length to handle the orignal number of actual values.
    switch (indexingType) {
    case ArrayWithInt32:
    case ArrayWithDouble:
    case ArrayWithContiguous:
        ensureLength(vm, relevantLength);
        break;
        
    case ArrayWithArrayStorage:
        if (arrayStorage()->vectorLength() < relevantLength) {
            increaseVectorLength(exec->vm(), relevantLength);
            ContiguousTypeAccessor<indexingType>::replaceDataReference(&data, arrayStorage()->vector());
        }
        if (arrayStorage()->length() < relevantLength)
            arrayStorage()->setLength(relevantLength);
        break;
        
    default:
        CRASH();
    }

    for (size_t i = 0; i < relevantLength; i++)
        ContiguousTypeAccessor<indexingType>::setWithValue(vm, this, data, i, values[i].first);
    
    Heap::heap(this)->popTempSortVector(&values);
}

void JSArray::sort(ExecState* exec)
{
    ASSERT(!inSparseIndexingMode());
    
    switch (structure()->indexingType()) {
    case ArrayClass:
    case ArrayWithUndecided:
        return;
        
    case ArrayWithInt32: {
        unsigned lengthNotIncludingUndefined;
        unsigned newRelevantLength;
        compactForSorting<ArrayWithInt32>(
            lengthNotIncludingUndefined, newRelevantLength);
        
        sortCompactedVector<ArrayWithInt32>(
            exec, m_butterfly->contiguousInt32(), lengthNotIncludingUndefined);
        return;
    }
        
    case ArrayWithDouble: {
        unsigned lengthNotIncludingUndefined;
        unsigned newRelevantLength;
        compactForSorting<ArrayWithDouble>(
            lengthNotIncludingUndefined, newRelevantLength);
        
        sortCompactedVector<ArrayWithDouble>(
            exec, m_butterfly->contiguousDouble(), lengthNotIncludingUndefined);
        return;
    }
        
    case ArrayWithContiguous: {
        unsigned lengthNotIncludingUndefined;
        unsigned newRelevantLength;
        compactForSorting<ArrayWithContiguous>(
            lengthNotIncludingUndefined, newRelevantLength);
        
        sortCompactedVector<ArrayWithContiguous>(
            exec, m_butterfly->contiguous(), lengthNotIncludingUndefined);
        return;
    }
        
    case ArrayWithArrayStorage: {
        unsigned lengthNotIncludingUndefined;
        unsigned newRelevantLength;
        compactForSorting<ArrayWithArrayStorage>(
            lengthNotIncludingUndefined, newRelevantLength);
        ArrayStorage* storage = m_butterfly->arrayStorage();
        ASSERT(!storage->m_sparseMap);
        
        sortCompactedVector<ArrayWithArrayStorage>(exec, storage->vector(), lengthNotIncludingUndefined);
        return;
    }
        
    default:
        RELEASE_ASSERT_NOT_REACHED();
    }
}

struct AVLTreeNodeForArrayCompare {
    JSValue value;

    // Child pointers.  The high bit of gt is robbed and used as the
    // balance factor sign.  The high bit of lt is robbed and used as
    // the magnitude of the balance factor.
    int32_t gt;
    int32_t lt;
};

struct AVLTreeAbstractorForArrayCompare {
    typedef int32_t handle; // Handle is an index into m_nodes vector.
    typedef JSValue key;
    typedef int32_t size;

    Vector<AVLTreeNodeForArrayCompare, 0, UnsafeVectorOverflow> m_nodes;
    ExecState* m_exec;
    JSValue m_compareFunction;
    CallType m_compareCallType;
    const CallData* m_compareCallData;
    OwnPtr<CachedCall> m_cachedCall;

    handle get_less(handle h) { return m_nodes[h].lt & 0x7FFFFFFF; }
    void set_less(handle h, handle lh) { m_nodes[h].lt &= 0x80000000; m_nodes[h].lt |= lh; }
    handle get_greater(handle h) { return m_nodes[h].gt & 0x7FFFFFFF; }
    void set_greater(handle h, handle gh) { m_nodes[h].gt &= 0x80000000; m_nodes[h].gt |= gh; }

    int get_balance_factor(handle h)
    {
        if (m_nodes[h].gt & 0x80000000)
            return -1;
        return static_cast<unsigned>(m_nodes[h].lt) >> 31;
    }

    void set_balance_factor(handle h, int bf)
    {
        if (bf == 0) {
            m_nodes[h].lt &= 0x7FFFFFFF;
            m_nodes[h].gt &= 0x7FFFFFFF;
        } else {
            m_nodes[h].lt |= 0x80000000;
            if (bf < 0)
                m_nodes[h].gt |= 0x80000000;
            else
                m_nodes[h].gt &= 0x7FFFFFFF;
        }
    }

    int compare_key_key(key va, key vb)
    {
        ASSERT(!va.isUndefined());
        ASSERT(!vb.isUndefined());

        if (m_exec->hadException())
            return 1;

        double compareResult;
        if (m_cachedCall) {
            m_cachedCall->setThis(jsUndefined());
            m_cachedCall->setArgument(0, va);
            m_cachedCall->setArgument(1, vb);
            compareResult = m_cachedCall->call().toNumber(m_cachedCall->newCallFrame(m_exec));
        } else {
            MarkedArgumentBuffer arguments;
            arguments.append(va);
            arguments.append(vb);
            compareResult = call(m_exec, m_compareFunction, m_compareCallType, *m_compareCallData, jsUndefined(), arguments).toNumber(m_exec);
        }
        return (compareResult < 0) ? -1 : 1; // Not passing equality through, because we need to store all values, even if equivalent.
    }

    int compare_key_node(key k, handle h) { return compare_key_key(k, m_nodes[h].value); }
    int compare_node_node(handle h1, handle h2) { return compare_key_key(m_nodes[h1].value, m_nodes[h2].value); }

    static handle null() { return 0x7FFFFFFF; }
};

template<IndexingType indexingType>
void JSArray::sortVector(ExecState* exec, JSValue compareFunction, CallType callType, const CallData& callData)
{
    ASSERT(!inSparseIndexingMode());
    ASSERT(indexingType == structure()->indexingType());
    
    // FIXME: This ignores exceptions raised in the compare function or in toNumber.
        
    // The maximum tree depth is compiled in - but the caller is clearly up to no good
    // if a larger array is passed.
    ASSERT(m_butterfly->publicLength() <= static_cast<unsigned>(std::numeric_limits<int>::max()));
    if (m_butterfly->publicLength() > static_cast<unsigned>(std::numeric_limits<int>::max()))
        return;
        
    unsigned usedVectorLength = relevantLength<indexingType>();
    unsigned nodeCount = usedVectorLength;
        
    if (!nodeCount)
        return;
        
    AVLTree<AVLTreeAbstractorForArrayCompare, 44> tree; // Depth 44 is enough for 2^31 items
    tree.abstractor().m_exec = exec;
    tree.abstractor().m_compareFunction = compareFunction;
    tree.abstractor().m_compareCallType = callType;
    tree.abstractor().m_compareCallData = &callData;
    tree.abstractor().m_nodes.grow(nodeCount);
        
    if (callType == CallTypeJS)
        tree.abstractor().m_cachedCall = adoptPtr(new CachedCall(exec, jsCast<JSFunction*>(compareFunction), 2));
        
    if (!tree.abstractor().m_nodes.begin()) {
        throwOutOfMemoryError(exec);
        return;
    }
        
    // FIXME: If the compare function modifies the array, the vector, map, etc. could be modified
    // right out from under us while we're building the tree here.
        
    unsigned numDefined = 0;
    unsigned numUndefined = 0;
    
    // Iterate over the array, ignoring missing values, counting undefined ones, and inserting all other ones into the tree.
    for (; numDefined < usedVectorLength; ++numDefined) {
        if (numDefined >= m_butterfly->vectorLength())
            break;
        JSValue v = getHolyIndexQuickly(numDefined);
        if (!v || v.isUndefined())
            break;
        tree.abstractor().m_nodes[numDefined].value = v;
        tree.insert(numDefined);
    }
    for (unsigned i = numDefined; i < usedVectorLength; ++i) {
        if (i >= m_butterfly->vectorLength())
            break;
        JSValue v = getHolyIndexQuickly(i);
        if (v) {
            if (v.isUndefined())
                ++numUndefined;
            else {
                tree.abstractor().m_nodes[numDefined].value = v;
                tree.insert(numDefined);
                ++numDefined;
            }
        }
    }
    
    unsigned newUsedVectorLength = numDefined + numUndefined;
        
    // The array size may have changed. Figure out the new bounds.
    unsigned newestUsedVectorLength = currentRelevantLength();
        
    unsigned elementsToExtractThreshold = min(min(newestUsedVectorLength, numDefined), static_cast<unsigned>(tree.abstractor().m_nodes.size()));
    unsigned undefinedElementsThreshold = min(newestUsedVectorLength, newUsedVectorLength);
    unsigned clearElementsThreshold = min(newestUsedVectorLength, usedVectorLength);
        
    // Copy the values back into m_storage.
    AVLTree<AVLTreeAbstractorForArrayCompare, 44>::Iterator iter;
    iter.start_iter_least(tree);
    VM& vm = exec->vm();
    for (unsigned i = 0; i < elementsToExtractThreshold; ++i) {
        ASSERT(i < butterfly()->vectorLength());
        if (structure()->indexingType() == ArrayWithDouble)
            butterfly()->contiguousDouble()[i] = tree.abstractor().m_nodes[*iter].value.asNumber();
        else
            currentIndexingData()[i].set(vm, this, tree.abstractor().m_nodes[*iter].value);
        ++iter;
    }
    // Put undefined values back in.
    switch (structure()->indexingType()) {
    case ArrayWithInt32:
    case ArrayWithDouble:
        ASSERT(elementsToExtractThreshold == undefinedElementsThreshold);
        break;
        
    default:
        for (unsigned i = elementsToExtractThreshold; i < undefinedElementsThreshold; ++i) {
            ASSERT(i < butterfly()->vectorLength());
            currentIndexingData()[i].setUndefined();
        }
    }

    // Ensure that unused values in the vector are zeroed out.
    for (unsigned i = undefinedElementsThreshold; i < clearElementsThreshold; ++i) {
        ASSERT(i < butterfly()->vectorLength());
        if (structure()->indexingType() == ArrayWithDouble)
            butterfly()->contiguousDouble()[i] = QNaN;
        else
            currentIndexingData()[i].clear();
    }
    
    if (hasArrayStorage(structure()->indexingType()))
        arrayStorage()->m_numValuesInVector = newUsedVectorLength;
}

void JSArray::sort(ExecState* exec, JSValue compareFunction, CallType callType, const CallData& callData)
{
    ASSERT(!inSparseIndexingMode());
    
    switch (structure()->indexingType()) {
    case ArrayClass:
    case ArrayWithUndecided:
        return;
        
    case ArrayWithInt32:
        sortVector<ArrayWithInt32>(exec, compareFunction, callType, callData);
        return;

    case ArrayWithDouble:
        sortVector<ArrayWithDouble>(exec, compareFunction, callType, callData);
        return;

    case ArrayWithContiguous:
        sortVector<ArrayWithContiguous>(exec, compareFunction, callType, callData);
        return;

    case ArrayWithArrayStorage:
        sortVector<ArrayWithArrayStorage>(exec, compareFunction, callType, callData);
        return;
        
    default:
        RELEASE_ASSERT_NOT_REACHED();
    }
}

void JSArray::fillArgList(ExecState* exec, MarkedArgumentBuffer& args)
{
    unsigned i = 0;
    unsigned vectorEnd;
    WriteBarrier<Unknown>* vector;
    
    switch (structure()->indexingType()) {
    case ArrayClass:
        return;
        
    case ArrayWithUndecided: {
        vector = 0;
        vectorEnd = 0;
        break;
    }
        
    case ArrayWithInt32:
    case ArrayWithContiguous: {
        vectorEnd = m_butterfly->publicLength();
        vector = m_butterfly->contiguous().data();
        break;
    }
        
    case ArrayWithDouble: {
        vector = 0;
        vectorEnd = 0;
        for (; i < m_butterfly->publicLength(); ++i) {
            double v = butterfly()->contiguousDouble()[i];
            if (v != v)
                break;
            args.append(JSValue(JSValue::EncodeAsDouble, v));
        }
        break;
    }
    
    case ARRAY_WITH_ARRAY_STORAGE_INDEXING_TYPES: {
        ArrayStorage* storage = m_butterfly->arrayStorage();
        
        vector = storage->m_vector;
        vectorEnd = min(storage->length(), storage->vectorLength());
        break;
    }
        
    default:
        CRASH();
        vector = 0;
        vectorEnd = 0;
        break;
    }
    
    for (; i < vectorEnd; ++i) {
        WriteBarrier<Unknown>& v = vector[i];
        if (!v)
            break;
        args.append(v.get());
    }
    
    for (; i < length(); ++i)
        args.append(get(exec, i));
}

void JSArray::copyToArguments(ExecState* exec, CallFrame* callFrame, uint32_t length)
{
    unsigned i = 0;
    WriteBarrier<Unknown>* vector;
    unsigned vectorEnd;
    
    ASSERT(length == this->length());
    switch (structure()->indexingType()) {
    case ArrayClass:
        return;
        
    case ArrayWithUndecided: {
        vector = 0;
        vectorEnd = 0;
        break;
    }
        
    case ArrayWithInt32:
    case ArrayWithContiguous: {
        vector = m_butterfly->contiguous().data();
        vectorEnd = m_butterfly->publicLength();
        break;
    }
        
    case ArrayWithDouble: {
        vector = 0;
        vectorEnd = 0;
        for (; i < m_butterfly->publicLength(); ++i) {
            ASSERT(i < butterfly()->vectorLength());
            double v = m_butterfly->contiguousDouble()[i];
            if (v != v)
                break;
            callFrame->setArgument(i, JSValue(JSValue::EncodeAsDouble, v));
        }
        break;
    }
        
    case ARRAY_WITH_ARRAY_STORAGE_INDEXING_TYPES: {
        ArrayStorage* storage = m_butterfly->arrayStorage();
        vector = storage->m_vector;
        vectorEnd = min(length, storage->vectorLength());
        break;
    }
        
    default:
        CRASH();
        vector = 0;
        vectorEnd = 0;
        break;
    }
    
    for (; i < vectorEnd; ++i) {
        WriteBarrier<Unknown>& v = vector[i];
        if (!v)
            break;
        callFrame->setArgument(i, v.get());
    }
    
    for (; i < length; ++i)
        callFrame->setArgument(i, get(exec, i));
}

template<IndexingType indexingType>
void JSArray::compactForSorting(unsigned& numDefined, unsigned& newRelevantLength)
{
    ASSERT(!inSparseIndexingMode());
    ASSERT(indexingType == structure()->indexingType());

    unsigned myRelevantLength = relevantLength<indexingType>();
    
    numDefined = 0;
    unsigned numUndefined = 0;
        
    for (; numDefined < myRelevantLength; ++numDefined) {
        ASSERT(numDefined < m_butterfly->vectorLength());
        if (indexingType == ArrayWithInt32) {
            JSValue v = m_butterfly->contiguousInt32()[numDefined].get();
            if (!v)
                break;
            ASSERT(v.isInt32());
            continue;
        }
        if (indexingType == ArrayWithDouble) {
            double v = m_butterfly->contiguousDouble()[numDefined];
            if (v != v)
                break;
            continue;
        }
        JSValue v = indexingData<indexingType>()[numDefined].get();
        if (!v || v.isUndefined())
            break;
    }
        
    for (unsigned i = numDefined; i < myRelevantLength; ++i) {
        ASSERT(i < m_butterfly->vectorLength());
        if (indexingType == ArrayWithInt32) {
            JSValue v = m_butterfly->contiguousInt32()[i].get();
            if (!v)
                continue;
            ASSERT(v.isInt32());
            ASSERT(numDefined < m_butterfly->vectorLength());
            m_butterfly->contiguousInt32()[numDefined++].setWithoutWriteBarrier(v);
            continue;
        }
        if (indexingType == ArrayWithDouble) {
            double v = m_butterfly->contiguousDouble()[i];
            if (v != v)
                continue;
            ASSERT(numDefined < m_butterfly->vectorLength());
            m_butterfly->contiguousDouble()[numDefined++] = v;
            continue;
        }
        JSValue v = indexingData<indexingType>()[i].get();
        if (v) {
            if (v.isUndefined())
                ++numUndefined;
            else {
                ASSERT(numDefined < m_butterfly->vectorLength());
                indexingData<indexingType>()[numDefined++].setWithoutWriteBarrier(v);
            }
        }
    }
        
    newRelevantLength = numDefined + numUndefined;
    
    if (hasArrayStorage(indexingType))
        RELEASE_ASSERT(!arrayStorage()->m_sparseMap);
    
    switch (indexingType) {
    case ArrayWithInt32:
    case ArrayWithDouble:
        RELEASE_ASSERT(numDefined == newRelevantLength);
        break;
        
    default:
        for (unsigned i = numDefined; i < newRelevantLength; ++i) {
            ASSERT(i < m_butterfly->vectorLength());
            indexingData<indexingType>()[i].setUndefined();
        }
        break;
    }
    for (unsigned i = newRelevantLength; i < myRelevantLength; ++i) {
        ASSERT(i < m_butterfly->vectorLength());
        if (indexingType == ArrayWithDouble)
            m_butterfly->contiguousDouble()[i] = QNaN;
        else
            indexingData<indexingType>()[i].clear();
    }

    if (hasArrayStorage(indexingType))
        arrayStorage()->m_numValuesInVector = newRelevantLength;
}

} // namespace JSC
