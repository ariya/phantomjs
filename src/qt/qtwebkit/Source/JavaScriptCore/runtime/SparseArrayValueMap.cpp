/*
 * Copyright (C) 2011, 2012 Apple Inc. All rights reserved.
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
#include "SparseArrayValueMap.h"

#include "ClassInfo.h"
#include "GetterSetter.h"
#include "JSObject.h"
#include "Operations.h"
#include "PropertySlot.h"
#include "Reject.h"
#include "SlotVisitor.h"
#include "Structure.h"

namespace JSC {

const ClassInfo SparseArrayValueMap::s_info = { "SparseArrayValueMap", 0, 0, 0, CREATE_METHOD_TABLE(SparseArrayValueMap) };

SparseArrayValueMap::SparseArrayValueMap(VM& vm)
    : Base(vm, vm.sparseArrayValueMapStructure.get())
    , m_flags(Normal)
    , m_reportedCapacity(0)
{
}

SparseArrayValueMap::~SparseArrayValueMap()
{
}

void SparseArrayValueMap::finishCreation(VM& vm)
{
    Base::finishCreation(vm);
}

SparseArrayValueMap* SparseArrayValueMap::create(VM& vm)
{
    SparseArrayValueMap* result = new (NotNull, allocateCell<SparseArrayValueMap>(vm.heap)) SparseArrayValueMap(vm);
    result->finishCreation(vm);
    return result;
}

void SparseArrayValueMap::destroy(JSCell* cell)
{
    static_cast<SparseArrayValueMap*>(cell)->SparseArrayValueMap::~SparseArrayValueMap();
}

Structure* SparseArrayValueMap::createStructure(VM& vm, JSGlobalObject* globalObject, JSValue prototype)
{
    return Structure::create(vm, globalObject, prototype, TypeInfo(CompoundType, StructureFlags), &s_info);
}

SparseArrayValueMap::AddResult SparseArrayValueMap::add(JSObject* array, unsigned i)
{
    SparseArrayEntry entry;
    entry.setWithoutWriteBarrier(jsUndefined());

    AddResult result = m_map.add(i, entry);
    size_t capacity = m_map.capacity();
    if (capacity != m_reportedCapacity) {
        Heap::heap(array)->reportExtraMemoryCost((capacity - m_reportedCapacity) * (sizeof(unsigned) + sizeof(WriteBarrier<Unknown>)));
        m_reportedCapacity = capacity;
    }
    return result;
}

void SparseArrayValueMap::putEntry(ExecState* exec, JSObject* array, unsigned i, JSValue value, bool shouldThrow)
{
    AddResult result = add(array, i);
    SparseArrayEntry& entry = result.iterator->value;

    // To save a separate find & add, we first always add to the sparse map.
    // In the uncommon case that this is a new property, and the array is not
    // extensible, this is not the right thing to have done - so remove again.
    if (result.isNewEntry && !array->isExtensible()) {
        remove(result.iterator);
        if (shouldThrow)
            throwTypeError(exec, StrictModeReadonlyPropertyWriteError);
        return;
    }
    
    entry.put(exec, array, this, value, shouldThrow);
}

bool SparseArrayValueMap::putDirect(ExecState* exec, JSObject* array, unsigned i, JSValue value, unsigned attributes, PutDirectIndexMode mode)
{
    AddResult result = add(array, i);
    SparseArrayEntry& entry = result.iterator->value;

    // To save a separate find & add, we first always add to the sparse map.
    // In the uncommon case that this is a new property, and the array is not
    // extensible, this is not the right thing to have done - so remove again.
    if (mode != PutDirectIndexLikePutDirect && result.isNewEntry && !array->isExtensible()) {
        remove(result.iterator);
        return reject(exec, mode == PutDirectIndexShouldThrow, "Attempting to define property on object that is not extensible.");
    }

    entry.attributes = attributes;
    entry.set(exec->vm(), this, value);
    return true;
}

void SparseArrayEntry::get(PropertySlot& slot) const
{
    JSValue value = Base::get();
    ASSERT(value);

    if (LIKELY(!value.isGetterSetter())) {
        slot.setValue(value);
        return;
    }

    JSObject* getter = asGetterSetter(value)->getter();
    if (!getter) {
        slot.setUndefined();
        return;
    }

    slot.setGetterSlot(getter);
}

void SparseArrayEntry::get(PropertyDescriptor& descriptor) const
{
    descriptor.setDescriptor(Base::get(), attributes);
}

JSValue SparseArrayEntry::get(ExecState* exec, JSObject* array) const
{
    JSValue result = Base::get();
    ASSERT(result);

    if (LIKELY(!result.isGetterSetter()))
        return result;

    JSObject* getter = asGetterSetter(result)->getter();
    if (!getter)
        return jsUndefined();

    CallData callData;
    CallType callType = getter->methodTable()->getCallData(getter, callData);
    return call(exec, getter, callType, callData, array->methodTable()->toThisObject(array, exec), exec->emptyList());
}

void SparseArrayEntry::put(ExecState* exec, JSValue thisValue, SparseArrayValueMap* map, JSValue value, bool shouldThrow)
{
    if (!(attributes & Accessor)) {
        if (attributes & ReadOnly) {
            if (shouldThrow)
                throwTypeError(exec, StrictModeReadonlyPropertyWriteError);
            return;
        }

        set(exec->vm(), map, value);
        return;
    }

    JSValue accessor = Base::get();
    ASSERT(accessor.isGetterSetter());
    JSObject* setter = asGetterSetter(accessor)->setter();
    
    if (!setter) {
        if (shouldThrow)
            throwTypeError(exec, StrictModeReadonlyPropertyWriteError);
        return;
    }

    CallData callData;
    CallType callType = setter->methodTable()->getCallData(setter, callData);
    MarkedArgumentBuffer args;
    args.append(value);
    if (thisValue.isObject())
        thisValue = asObject(thisValue)->methodTable()->toThisObject(asObject(thisValue), exec);
    call(exec, setter, callType, callData, thisValue, args);
}

JSValue SparseArrayEntry::getNonSparseMode() const
{
    ASSERT(!attributes);
    return Base::get();
}

void SparseArrayValueMap::visitChildren(JSCell* thisObject, SlotVisitor& visitor)
{
    Base::visitChildren(thisObject, visitor);
    
    SparseArrayValueMap* thisMap = jsCast<SparseArrayValueMap*>(thisObject);
    iterator end = thisMap->m_map.end();
    for (iterator it = thisMap->m_map.begin(); it != end; ++it)
        visitor.append(&it->value);
}

} // namespace JSC

