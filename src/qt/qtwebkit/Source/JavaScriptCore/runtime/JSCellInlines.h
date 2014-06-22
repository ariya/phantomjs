/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#ifndef JSCellInlines_h
#define JSCellInlines_h

#include "CallFrame.h"
#include "Handle.h"
#include "JSCell.h"
#include "JSObject.h"
#include "JSString.h"
#include "Structure.h"
#include "StructureInlines.h"

namespace JSC {

inline JSCell::JSCell(CreatingEarlyCellTag)
{
}

inline JSCell::JSCell(VM& vm, Structure* structure)
    : m_structure(vm, this, structure)
{
}

inline void JSCell::finishCreation(VM& vm)
{
#if ENABLE(GC_VALIDATION)
    ASSERT(vm.isInitializingObject());
    vm.setInitializingObjectClass(0);
#else
    UNUSED_PARAM(vm);
#endif
    ASSERT(m_structure);
}

inline void JSCell::finishCreation(VM& vm, Structure* structure, CreatingEarlyCellTag)
{
#if ENABLE(GC_VALIDATION)
    ASSERT(vm.isInitializingObject());
    vm.setInitializingObjectClass(0);
    if (structure)
#endif
        m_structure.setEarlyValue(vm, this, structure);
    // Very first set of allocations won't have a real structure.
    ASSERT(m_structure || !vm.structureStructure);
}

inline Structure* JSCell::structure() const
{
    return m_structure.get();
}

inline void JSCell::visitChildren(JSCell* cell, SlotVisitor& visitor)
{
    MARK_LOG_PARENT(visitor, cell);

    visitor.append(&cell->m_structure);
}

template<typename T>
void* allocateCell(Heap& heap, size_t size)
{
    ASSERT(size >= sizeof(T));
#if ENABLE(GC_VALIDATION)
    ASSERT(!heap.vm()->isInitializingObject());
    heap.vm()->setInitializingObjectClass(&T::s_info);
#endif
    JSCell* result = 0;
    if (T::needsDestruction && T::hasImmortalStructure)
        result = static_cast<JSCell*>(heap.allocateWithImmortalStructureDestructor(size));
    else if (T::needsDestruction)
        result = static_cast<JSCell*>(heap.allocateWithNormalDestructor(size));
    else 
        result = static_cast<JSCell*>(heap.allocateWithoutDestructor(size));
    result->clearStructure();
    return result;
}
    
template<typename T>
void* allocateCell(Heap& heap)
{
    return allocateCell<T>(heap, sizeof(T));
}
    
inline bool isZapped(const JSCell* cell)
{
    return cell->isZapped();
}

inline bool JSCell::isObject() const
{
    return m_structure->isObject();
}

inline bool JSCell::isString() const
{
    return m_structure->typeInfo().type() == StringType;
}

inline bool JSCell::isGetterSetter() const
{
    return m_structure->typeInfo().type() == GetterSetterType;
}

inline bool JSCell::isProxy() const
{
    return structure()->typeInfo().type() == ProxyType;
}

inline bool JSCell::isAPIValueWrapper() const
{
    return m_structure->typeInfo().type() == APIValueWrapperType;
}

inline void JSCell::setStructure(VM& vm, Structure* structure)
{
    ASSERT(structure->typeInfo().overridesVisitChildren() == this->structure()->typeInfo().overridesVisitChildren());
    ASSERT(structure->classInfo() == m_structure->classInfo());
    ASSERT(!m_structure
        || m_structure->transitionWatchpointSetHasBeenInvalidated()
        || m_structure.get() == structure);
    m_structure.set(vm, this, structure);
}

inline const MethodTable* JSCell::methodTableForDestruction() const
{
    return &classInfo()->methodTable;
}

inline const MethodTable* JSCell::methodTable() const
{
    if (Structure* rootStructure = m_structure->structure())
        RELEASE_ASSERT(rootStructure == rootStructure->structure());

    return &classInfo()->methodTable;
}

inline bool JSCell::inherits(const ClassInfo* info) const
{
    return classInfo()->isSubClassOf(info);
}

ALWAYS_INLINE bool JSCell::fastGetOwnPropertySlot(ExecState* exec, PropertyName propertyName, PropertySlot& slot)
{
    if (!structure()->typeInfo().overridesGetOwnPropertySlot())
        return asObject(this)->inlineGetOwnPropertySlot(exec, propertyName, slot);
    return methodTable()->getOwnPropertySlot(this, exec, propertyName, slot);
}

// Fast call to get a property where we may not yet have converted the string to an
// identifier. The first time we perform a property access with a given string, try
// performing the property map lookup without forming an identifier. We detect this
// case by checking whether the hash has yet been set for this string.
ALWAYS_INLINE JSValue JSCell::fastGetOwnProperty(ExecState* exec, const String& name)
{
    if (!structure()->typeInfo().overridesGetOwnPropertySlot() && !structure()->hasGetterSetterProperties()) {
        PropertyOffset offset = name.impl()->hasHash()
            ? structure()->get(exec->vm(), Identifier(exec, name))
            : structure()->get(exec->vm(), name);
        if (offset != invalidOffset)
            return asObject(this)->locationForOffset(offset)->get();
    }
    return JSValue();
}

inline bool JSCell::toBoolean(ExecState* exec) const
{
    if (isString()) 
        return static_cast<const JSString*>(this)->toBoolean();
    return !structure()->masqueradesAsUndefined(exec->lexicalGlobalObject());
}

inline TriState JSCell::pureToBoolean() const
{
    if (isString()) 
        return static_cast<const JSString*>(this)->toBoolean() ? TrueTriState : FalseTriState;
    return MixedTriState;
}

} // namespace JSC

#endif // JSCellInlines_h
