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

#ifndef MethodOfGettingAValueProfile_h
#define MethodOfGettingAValueProfile_h

#include <wtf/Platform.h>

// This is guarded by ENABLE_DFG_JIT only because it uses some value profiles
// that are currently only used if the DFG is enabled (i.e. they are not
// available in the profile-only configuration). Hopefully someday all of
// these #if's will disappear...
#if ENABLE(DFG_JIT)

#include "JSCJSValue.h"

namespace JSC {

class CodeBlock;
class LazyOperandValueProfileKey;
struct ValueProfile;

class MethodOfGettingAValueProfile {
public:
    MethodOfGettingAValueProfile()
        : m_kind(None)
    {
    }
    
    explicit MethodOfGettingAValueProfile(ValueProfile* profile)
    {
        if (profile) {
            m_kind = Ready;
            u.profile = profile;
        } else
            m_kind = None;
    }
    
    static MethodOfGettingAValueProfile fromLazyOperand(
        CodeBlock*, const LazyOperandValueProfileKey&);
    
    bool operator!() const { return m_kind == None; }
    
    // This logically has a pointer to a "There exists X such that
    // ValueProfileBase<X>". But since C++ does not have existential
    // templates, I cannot return it. So instead, for any methods that
    // users of this class would like to call, we'll just have to provide
    // a method here that does it through an indirection. Or we could
    // possibly just make ValueProfile less template-based. But last I
    // tried that, it felt more yucky than this class.
    
    EncodedJSValue* getSpecFailBucket(unsigned index) const;
    
private:
    enum Kind {
        None,
        Ready,
        LazyOperand
    };
    
    Kind m_kind;
    union {
        ValueProfile* profile;
        struct {
            CodeBlock* codeBlock;
            unsigned bytecodeOffset;
            int operand;
        } lazyOperand;
    } u;
};

} // namespace JSC

#endif // ENABLE(DFG_JIT)

#endif // MethodOfGettingAValueProfile_h

