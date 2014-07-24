/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#ifndef ObjectAllocationProfile_h
#define ObjectAllocationProfile_h

#include "VM.h"
#include "JSGlobalObject.h"
#include "ObjectPrototype.h"
#include "SlotVisitor.h"
#include "WriteBarrier.h"

namespace JSC {

class ObjectAllocationProfile {
    friend class LLIntOffsetsExtractor;
public:
    static ptrdiff_t offsetOfAllocator() { return OBJECT_OFFSETOF(ObjectAllocationProfile, m_allocator); }
    static ptrdiff_t offsetOfStructure() { return OBJECT_OFFSETOF(ObjectAllocationProfile, m_structure); }

    ObjectAllocationProfile()
        : m_allocator(0)
    {
    }

    bool isNull() { return !m_allocator; }

    void initialize(VM& vm, JSCell* owner, JSObject* prototype, unsigned inferredInlineCapacity)
    {
        ASSERT(!m_allocator);
        ASSERT(!m_structure);

        unsigned inlineCapacity = 0;
        if (inferredInlineCapacity < JSFinalObject::defaultInlineCapacity()) {
            // Try to shrink the object based on static analysis.
            inferredInlineCapacity += possibleDefaultPropertyCount(vm, prototype);

            if (!inferredInlineCapacity) {
                // Empty objects are rare, so most likely the static analyzer just didn't
                // see the real initializer function. This can happen with helper functions.
                inferredInlineCapacity = JSFinalObject::defaultInlineCapacity();
            } else if (inferredInlineCapacity > JSFinalObject::defaultInlineCapacity()) {
                // Default properties are weak guesses, so don't allow them to turn a small
                // object into a large object.
                inferredInlineCapacity = JSFinalObject::defaultInlineCapacity();
            }

            inlineCapacity = inferredInlineCapacity;
            ASSERT(inlineCapacity < JSFinalObject::maxInlineCapacity());
        } else {
            // Normal or large object.
            inlineCapacity = inferredInlineCapacity;
            if (inlineCapacity > JSFinalObject::maxInlineCapacity())
                inlineCapacity = JSFinalObject::maxInlineCapacity();
        }

        ASSERT(inlineCapacity > 0);
        ASSERT(inlineCapacity <= JSFinalObject::maxInlineCapacity());

        size_t allocationSize = JSFinalObject::allocationSize(inlineCapacity);
        MarkedAllocator* allocator = &vm.heap.allocatorForObjectWithoutDestructor(allocationSize);
        ASSERT(allocator->cellSize());

        // Take advantage of extra inline capacity available in the size class.
        size_t slop = (allocator->cellSize() - allocationSize) / sizeof(WriteBarrier<Unknown>);
        inlineCapacity += slop;
        if (inlineCapacity > JSFinalObject::maxInlineCapacity())
            inlineCapacity = JSFinalObject::maxInlineCapacity();

        m_allocator = allocator;
        m_structure.set(vm, owner,
            vm.prototypeMap.emptyObjectStructureForPrototype(prototype, inlineCapacity));
    }

    Structure* structure() { return m_structure.get(); }
    unsigned inlineCapacity() { return m_structure->inlineCapacity(); }

    void clear()
    {
        m_allocator = 0;
        m_structure.clear();
        ASSERT(isNull());
    }

    void visitAggregate(SlotVisitor& visitor)
    {
        visitor.append(&m_structure);
    }

private:

    unsigned possibleDefaultPropertyCount(VM& vm, JSObject* prototype)
    {
        if (prototype == prototype->globalObject()->objectPrototype())
            return 0;

        size_t count = 0;
        PropertyNameArray propertyNameArray(&vm);
        prototype->structure()->getPropertyNamesFromStructure(vm, propertyNameArray, ExcludeDontEnumProperties);
        PropertyNameArrayData::PropertyNameVector& propertyNameVector = propertyNameArray.data()->propertyNameVector();
        for (size_t i = 0; i < propertyNameVector.size(); ++i) {
            JSValue value = prototype->getDirect(vm, propertyNameVector[i]);

            // Functions are common, and are usually class-level objects that are not overridden.
            if (jsDynamicCast<JSFunction*>(value))
                continue;

            ++count;

        }
        return count;
    }

    MarkedAllocator* m_allocator; // Precomputed to make things easier for generated code.
    WriteBarrier<Structure> m_structure;
};

} // namespace JSC

#endif // ObjectAllocationProfile_h
