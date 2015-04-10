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

#include "config.h"
#include "MethodOfGettingAValueProfile.h"

#if ENABLE(DFG_JIT)

#include "CodeBlock.h"

namespace JSC {

MethodOfGettingAValueProfile MethodOfGettingAValueProfile::fromLazyOperand(
    CodeBlock* codeBlock, const LazyOperandValueProfileKey& key)
{
    MethodOfGettingAValueProfile result;
    result.m_kind = LazyOperand;
    result.u.lazyOperand.codeBlock = codeBlock;
    result.u.lazyOperand.bytecodeOffset = key.bytecodeOffset();
    result.u.lazyOperand.operand = key.operand();
    return result;
}

EncodedJSValue* MethodOfGettingAValueProfile::getSpecFailBucket(unsigned index) const
{
    switch (m_kind) {
    case None:
        return 0;
        
    case Ready:
        return u.profile->specFailBucket(index);
        
    case LazyOperand:
        return u.lazyOperand.codeBlock->lazyOperandValueProfiles().add(
            LazyOperandValueProfileKey(
                u.lazyOperand.bytecodeOffset, u.lazyOperand.operand))->specFailBucket(index);
        
    default:
        RELEASE_ASSERT_NOT_REACHED();
        return 0;
    }
}

} // namespace JSC

#endif // ENABLE(DFG_JIT)

