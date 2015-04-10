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

#ifndef GetByIdStatus_h
#define GetByIdStatus_h

#include "PropertyOffset.h"
#include "StructureSet.h"
#include <wtf/NotFound.h>

namespace JSC {

class CodeBlock;
class Identifier;

class GetByIdStatus {
public:
    enum State {
        NoInformation,  // It's uncached so we have no information.
        Simple,         // It's cached for a simple access to a known object property with
                        // a possible structure chain and a possible specific value.
        TakesSlowPath,  // It's known to often take slow path.
        MakesCalls      // It's known to take paths that make calls.
    };

    GetByIdStatus()
        : m_state(NoInformation)
        , m_offset(invalidOffset)
    {
    }
    
    explicit GetByIdStatus(State state)
        : m_state(state)
        , m_offset(invalidOffset)
    {
        ASSERT(state == NoInformation || state == TakesSlowPath || state == MakesCalls);
    }
    
    GetByIdStatus(
        State state, bool wasSeenInJIT, const StructureSet& structureSet = StructureSet(),
        PropertyOffset offset = invalidOffset, JSValue specificValue = JSValue(), Vector<Structure*> chain = Vector<Structure*>())
        : m_state(state)
        , m_structureSet(structureSet)
        , m_chain(chain)
        , m_specificValue(specificValue)
        , m_offset(offset)
        , m_wasSeenInJIT(wasSeenInJIT)
    {
        ASSERT((state == Simple) == (offset != invalidOffset));
    }
    
    static GetByIdStatus computeFor(CodeBlock*, unsigned bytecodeIndex, Identifier&);
    static GetByIdStatus computeFor(VM&, Structure*, Identifier&);
    
    State state() const { return m_state; }
    
    bool isSet() const { return m_state != NoInformation; }
    bool operator!() const { return !isSet(); }
    bool isSimple() const { return m_state == Simple; }
    bool takesSlowPath() const { return m_state == TakesSlowPath || m_state == MakesCalls; }
    bool makesCalls() const { return m_state == MakesCalls; }
    
    const StructureSet& structureSet() const { return m_structureSet; }
    const Vector<Structure*>& chain() const { return m_chain; } // Returns empty vector if this is a direct access.
    JSValue specificValue() const { return m_specificValue; } // Returns JSValue() if there is no specific value.
    PropertyOffset offset() const { return m_offset; }
    
    bool wasSeenInJIT() const { return m_wasSeenInJIT; }
    
private:
    static void computeForChain(GetByIdStatus& result, CodeBlock*, Identifier&, Structure*);
    static GetByIdStatus computeFromLLInt(CodeBlock*, unsigned bytecodeIndex, Identifier&);
    
    State m_state;
    StructureSet m_structureSet;
    Vector<Structure*> m_chain;
    JSValue m_specificValue;
    PropertyOffset m_offset;
    bool m_wasSeenInJIT;
};

} // namespace JSC

#endif // PropertyAccessStatus_h

