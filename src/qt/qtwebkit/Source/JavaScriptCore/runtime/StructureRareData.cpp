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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "StructureRareData.h"

#include "JSString.h"
#include "Operations.h"

namespace JSC {

const ClassInfo StructureRareData::s_info = { "StructureRareData", 0, 0, 0, CREATE_METHOD_TABLE(StructureRareData) };

Structure* StructureRareData::createStructure(VM& vm, JSGlobalObject* globalObject, JSValue prototype)
{
    return Structure::create(vm, globalObject, prototype, TypeInfo(CompoundType, StructureFlags), &s_info);
}

StructureRareData* StructureRareData::create(VM& vm, Structure* previous)
{
    StructureRareData* rareData = new (NotNull, allocateCell<StructureRareData>(vm.heap)) StructureRareData(vm, previous);
    rareData->finishCreation(vm);
    return rareData;
}

StructureRareData* StructureRareData::clone(VM& vm, const StructureRareData* other)
{
    StructureRareData* newRareData = new (NotNull, allocateCell<StructureRareData>(vm.heap)) StructureRareData(vm, other);
    newRareData->finishCreation(vm);
    return newRareData;
}

StructureRareData::StructureRareData(VM& vm, Structure* previous)
    : JSCell(vm, vm.structureRareDataStructure.get())
{
    if (previous)
        m_previous.set(vm, this, previous);
}

StructureRareData::StructureRareData(VM& vm, const StructureRareData* other)
    : JSCell(vm, other->structure())
{
    if (other->previousID())
        m_previous.set(vm, this, other->previousID());
    if (other->objectToStringValue())
        m_objectToStringValue.set(vm, this, other->objectToStringValue());
}

void StructureRareData::visitChildren(JSCell* cell, SlotVisitor& visitor)
{
    StructureRareData* thisObject = jsCast<StructureRareData*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, &s_info);
    ASSERT(thisObject->structure()->typeInfo().overridesVisitChildren());

    JSCell::visitChildren(thisObject, visitor);
    visitor.append(&thisObject->m_previous);
    visitor.append(&thisObject->m_objectToStringValue);
    visitor.append(&thisObject->m_enumerationCache);
}

} // namespace JSC
