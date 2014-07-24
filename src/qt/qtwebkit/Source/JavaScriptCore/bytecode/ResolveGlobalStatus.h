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

#ifndef ResolveGlobalStatus_h
#define ResolveGlobalStatus_h

#include "JSCJSValue.h"
#include "PropertyOffset.h"
#include <wtf/NotFound.h>

namespace JSC {

class CodeBlock;
class Identifier;
struct ResolveOperation;
class Structure;

class ResolveGlobalStatus {
public:
    enum State {
        NoInformation,
        Simple,
        TakesSlowPath
    };
    
    ResolveGlobalStatus()
        : m_state(NoInformation)
        , m_structure(0)
        , m_offset(invalidOffset)
    {
    }
    
    ResolveGlobalStatus(
        State state, Structure* structure = 0, PropertyOffset offset = invalidOffset,
        JSValue specificValue = JSValue())
        : m_state(state)
        , m_structure(structure)
        , m_offset(offset)
        , m_specificValue(specificValue)
    {
    }
    
    static ResolveGlobalStatus computeFor(CodeBlock*, int bytecodeIndex, ResolveOperation*, Identifier&);
    
    State state() const { return m_state; }
    
    bool isSet() const { return m_state != NoInformation; }
    bool operator!() const { return !isSet(); }
    bool isSimple() const { return m_state == Simple; }
    bool takesSlowPath() const { return m_state == TakesSlowPath; }
    
    Structure* structure() const { return m_structure; }
    PropertyOffset offset() const { return m_offset; }
    JSValue specificValue() const { return m_specificValue; }

private:
    State m_state;
    Structure* m_structure;
    PropertyOffset m_offset;
    JSValue m_specificValue;
}; // class ResolveGlobalStatus

} // namespace JSC

#endif // ResolveGlobalStatus_h

