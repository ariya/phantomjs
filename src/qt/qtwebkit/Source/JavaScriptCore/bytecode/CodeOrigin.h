/*
 * Copyright (C) 2011, 2012, 2013 Apple Inc. All rights reserved.
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

#ifndef CodeOrigin_h
#define CodeOrigin_h

#include "CodeBlockHash.h"
#include "CodeSpecializationKind.h"
#include "ValueRecovery.h"
#include "WriteBarrier.h"
#include <wtf/BitVector.h>
#include <wtf/PrintStream.h>
#include <wtf/StdLibExtras.h>
#include <wtf/Vector.h>

namespace JSC {

struct InlineCallFrame;
class ExecState;
class ExecutableBase;
class JSFunction;

struct CodeOrigin {
    static const unsigned maximumBytecodeIndex = (1u << 29) - 1;
    
    // Bytecode offset that you'd use to re-execute this instruction.
    unsigned bytecodeIndex : 29;
    // Bytecode offset corresponding to the opcode that gives the result (needed to handle
    // op_call/op_call_put_result and op_method_check/op_get_by_id).
    unsigned valueProfileOffset : 3;
    
    InlineCallFrame* inlineCallFrame;
    
    CodeOrigin()
        : bytecodeIndex(maximumBytecodeIndex)
        , valueProfileOffset(0)
        , inlineCallFrame(0)
    {
    }
    
    explicit CodeOrigin(unsigned bytecodeIndex, InlineCallFrame* inlineCallFrame = 0, unsigned valueProfileOffset = 0)
        : bytecodeIndex(bytecodeIndex)
        , valueProfileOffset(valueProfileOffset)
        , inlineCallFrame(inlineCallFrame)
    {
        RELEASE_ASSERT(bytecodeIndex <= maximumBytecodeIndex);
        RELEASE_ASSERT(valueProfileOffset < (1u << 3));
    }
    
    bool isSet() const { return bytecodeIndex != maximumBytecodeIndex; }
    
    unsigned bytecodeIndexForValueProfile() const
    {
        return bytecodeIndex + valueProfileOffset;
    }
    
    // The inline depth is the depth of the inline stack, so 1 = not inlined,
    // 2 = inlined one deep, etc.
    unsigned inlineDepth() const;
    
    // If the code origin corresponds to inlined code, gives you the heap object that
    // would have owned the code if it had not been inlined. Otherwise returns 0.
    ExecutableBase* codeOriginOwner() const;
    
    unsigned stackOffset() const;
    
    static unsigned inlineDepthForCallFrame(InlineCallFrame*);
    
    bool operator==(const CodeOrigin& other) const;
    
    bool operator!=(const CodeOrigin& other) const { return !(*this == other); }
    
    // Get the inline stack. This is slow, and is intended for debugging only.
    Vector<CodeOrigin> inlineStack() const;
    
    void dump(PrintStream&) const;
};

struct InlineCallFrame {
    Vector<ValueRecovery> arguments;
    WriteBarrier<ExecutableBase> executable;
    WriteBarrier<JSFunction> callee; // This may be null, indicating that this is a closure call and that the JSFunction and JSScope are already on the stack.
    CodeOrigin caller;
    BitVector capturedVars; // Indexed by the machine call frame's variable numbering.
    unsigned stackOffset : 31;
    bool isCall : 1;
    
    CodeSpecializationKind specializationKind() const { return specializationFromIsCall(isCall); }
    
    bool isClosureCall() const { return !callee; }
    
    // Get the callee given a machine call frame to which this InlineCallFrame belongs.
    JSFunction* calleeForCallFrame(ExecState*) const;
    
    String inferredName() const;
    CodeBlockHash hash() const;
    
    CodeBlock* baselineCodeBlock() const;
    
    void dumpBriefFunctionInformation(PrintStream&) const;
    void dump(PrintStream&) const;

    MAKE_PRINT_METHOD(InlineCallFrame, dumpBriefFunctionInformation, briefFunctionInformation);
};

struct CodeOriginAtCallReturnOffset {
    CodeOrigin codeOrigin;
    unsigned callReturnOffset;
};

inline unsigned CodeOrigin::stackOffset() const
{
    if (!inlineCallFrame)
        return 0;
    
    return inlineCallFrame->stackOffset;
}

inline bool CodeOrigin::operator==(const CodeOrigin& other) const
{
    return bytecodeIndex == other.bytecodeIndex
        && inlineCallFrame == other.inlineCallFrame;
}
    
inline unsigned getCallReturnOffsetForCodeOrigin(CodeOriginAtCallReturnOffset* data)
{
    return data->callReturnOffset;
}

inline ExecutableBase* CodeOrigin::codeOriginOwner() const
{
    if (!inlineCallFrame)
        return 0;
    return inlineCallFrame->executable.get();
}

} // namespace JSC

#endif // CodeOrigin_h

