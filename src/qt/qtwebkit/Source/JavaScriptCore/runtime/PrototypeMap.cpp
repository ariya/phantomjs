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

#include "config.h"
#include "PrototypeMap.h"

#include "JSGlobalObject.h"
#include "Operations.h"

namespace JSC {

void PrototypeMap::addPrototype(JSObject* object)
{
    m_prototypes.add(object, object);

    // Note that this method makes the somewhat odd decision to not check if this
    // object currently has indexed accessors. We could do that check here, and if
    // indexed accessors were found, we could tell the global object to have a bad
    // time. But we avoid this, to allow the following to be always fast:
    //
    // 1) Create an object.
    // 2) Give it a setter or read-only property that happens to have a numeric name.
    // 3) Allocate objects that use this object as a prototype.
    //
    // This avoids anyone having a bad time. Even if the instance objects end up
    // having indexed storage, the creation of indexed storage leads to a prototype
    // chain walk that detects the presence of indexed setters and then does the
    // right thing. As a result, having a bad time only happens if you add an
    // indexed setter (or getter, or read-only field) to an object that is already
    // used as a prototype.
}

Structure* PrototypeMap::emptyObjectStructureForPrototype(JSObject* prototype, unsigned inlineCapacity)
{
    StructureMap::AddResult addResult = m_structures.add(std::make_pair(prototype, inlineCapacity), nullptr);
    if (!addResult.isNewEntry) {
        ASSERT(isPrototype(prototype));
        return addResult.iterator->value.get();
    }

    addPrototype(prototype);
    Structure* structure = JSFinalObject::createStructure(
        prototype->globalObject()->vm(), prototype->globalObject(), prototype, inlineCapacity);
    addResult.iterator->value = structure;
    return structure;
}

void PrototypeMap::clearEmptyObjectStructureForPrototype(JSObject* object, unsigned inlineCapacity)
{
    m_structures.remove(std::make_pair(object, inlineCapacity));
}

} // namespace JSC
