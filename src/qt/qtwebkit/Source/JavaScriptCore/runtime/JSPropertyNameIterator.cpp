/*
 * Copyright (C) 2008, 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "JSPropertyNameIterator.h"

#include "JSGlobalObject.h"

namespace JSC {

const ClassInfo JSPropertyNameIterator::s_info = { "JSPropertyNameIterator", 0, 0, 0, CREATE_METHOD_TABLE(JSPropertyNameIterator) };

inline JSPropertyNameIterator::JSPropertyNameIterator(ExecState* exec, PropertyNameArrayData* propertyNameArrayData, size_t numCacheableSlots)
    : JSCell(exec->vm(), exec->vm().propertyNameIteratorStructure.get())
    , m_numCacheableSlots(numCacheableSlots)
    , m_jsStringsSize(propertyNameArrayData->propertyNameVector().size())
    , m_jsStrings(adoptArrayPtr(new WriteBarrier<Unknown>[m_jsStringsSize]))
{
}

JSPropertyNameIterator* JSPropertyNameIterator::create(ExecState* exec, JSObject* o)
{
    ASSERT(!o->structure()->enumerationCache() ||
            o->structure()->enumerationCache()->cachedStructure() != o->structure() ||
            o->structure()->enumerationCache()->cachedPrototypeChain() != o->structure()->prototypeChain(exec));

    PropertyNameArray propertyNames(exec);
    o->methodTable()->getPropertyNames(o, exec, propertyNames, ExcludeDontEnumProperties);
    size_t numCacheableSlots = 0;
    if (!o->structure()->hasNonEnumerableProperties() && !o->structure()->hasGetterSetterProperties()
        && !o->structure()->isUncacheableDictionary() && !o->structure()->typeInfo().overridesGetPropertyNames())
        numCacheableSlots = propertyNames.numCacheableSlots();
    
    JSPropertyNameIterator* jsPropertyNameIterator = new (NotNull, allocateCell<JSPropertyNameIterator>(*exec->heap())) JSPropertyNameIterator(exec, propertyNames.data(), numCacheableSlots);
    jsPropertyNameIterator->finishCreation(exec, propertyNames.data(), o);

    if (o->structure()->isDictionary())
        return jsPropertyNameIterator;

    if (o->structure()->typeInfo().overridesGetPropertyNames())
        return jsPropertyNameIterator;
    
    if (hasIndexingHeader(o->structure()->indexingType()))
        return jsPropertyNameIterator;
    
    size_t count = normalizePrototypeChain(exec, o);
    StructureChain* structureChain = o->structure()->prototypeChain(exec);
    WriteBarrier<Structure>* structure = structureChain->head();
    for (size_t i = 0; i < count; ++i) {
        if (structure[i]->typeInfo().overridesGetPropertyNames())
            return jsPropertyNameIterator;
    }

    jsPropertyNameIterator->setCachedPrototypeChain(exec->vm(), structureChain);
    jsPropertyNameIterator->setCachedStructure(exec->vm(), o->structure());
    o->structure()->setEnumerationCache(exec->vm(), jsPropertyNameIterator);
    return jsPropertyNameIterator;
}

void JSPropertyNameIterator::destroy(JSCell* cell)
{
    static_cast<JSPropertyNameIterator*>(cell)->JSPropertyNameIterator::~JSPropertyNameIterator();
}

JSValue JSPropertyNameIterator::get(ExecState* exec, JSObject* base, size_t i)
{
    JSValue identifier = m_jsStrings[i].get();
    if (m_cachedStructure.get() == base->structure() && m_cachedPrototypeChain.get() == base->structure()->prototypeChain(exec))
        return identifier;

    if (!base->hasProperty(exec, Identifier(exec, asString(identifier)->value(exec))))
        return JSValue();
    return identifier;
}

void JSPropertyNameIterator::visitChildren(JSCell* cell, SlotVisitor& visitor)
{
    JSPropertyNameIterator* thisObject = jsCast<JSPropertyNameIterator*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, &s_info);
    ASSERT(thisObject->structure()->typeInfo().overridesVisitChildren());
    visitor.appendValues(thisObject->m_jsStrings.get(), thisObject->m_jsStringsSize);
    visitor.append(&thisObject->m_cachedPrototypeChain);
}

} // namespace JSC
