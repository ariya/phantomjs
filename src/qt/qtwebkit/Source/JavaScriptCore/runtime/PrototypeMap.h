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

#ifndef PrototypeMap_h
#define PrototypeMap_h

#include "WeakGCMap.h"
#include <wtf/TriState.h>

namespace JSC {

class JSObject;
class Structure;

// Tracks the canonical structure an object should be allocated with when inheriting from a given prototype.
class PrototypeMap {
public:
    JS_EXPORT_PRIVATE Structure* emptyObjectStructureForPrototype(JSObject*, unsigned inlineCapacity);
    void clearEmptyObjectStructureForPrototype(JSObject*, unsigned inlineCapacity);
    void addPrototype(JSObject*);
    TriState isPrototype(JSObject*) const; // Returns a conservative estimate.

private:
    WeakGCMap<JSObject*, JSObject> m_prototypes;
    typedef WeakGCMap<std::pair<JSObject*, unsigned>, Structure> StructureMap;
    StructureMap m_structures;
};

inline TriState PrototypeMap::isPrototype(JSObject* object) const
{
    if (!m_prototypes.contains(object))
        return FalseTriState;

    // We know that 'object' was used as a prototype at one time, so be
    // conservative and say that it might still be so. (It would be expensive
    // to find out for sure, and we don't know of any cases where being precise
    // would improve performance.)
    return MixedTriState;
}

} // namespace JSC

#endif // PrototypeMap_h
