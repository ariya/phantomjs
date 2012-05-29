/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef DFGJITCompiler_h
#define DFGJITCompiler_h

#if ENABLE(DFG_JIT)

#include <assembler/MacroAssembler.h>
#include <bytecode/CodeBlock.h>
#include <dfg/DFGGraph.h>
#include <dfg/DFGRegisterBank.h>
#include <jit/JITCode.h>

#include <dfg/DFGFPRInfo.h>
#include <dfg/DFGGPRInfo.h>

namespace JSC {

class AbstractSamplingCounter;
class CodeBlock;
class JSGlobalData;

namespace DFG {

class JITCodeGenerator;
class NonSpeculativeJIT;
class SpeculativeJIT;
class SpeculationRecovery;

struct EntryLocation;
struct SpeculationCheck;

// === CallRecord ===
//
// A record of a call out from JIT code to a helper function.
// Every CallRecord contains a reference to the call instruction & the function
// that it needs to be linked to. Calls that might throw an exception also record
// the Jump taken on exception (unset if not present), and ExceptionInfo (presently
// an unsigned, bytecode index) used to recover handler/source info.
struct CallRecord {
    // Constructor for a call with no exception handler.
    CallRecord(MacroAssembler::Call call, FunctionPtr function)
        : m_call(call)
        , m_function(function)
    {
    }

    // Constructor for a call with an exception handler.
    CallRecord(MacroAssembler::Call call, FunctionPtr function, MacroAssembler::Jump exceptionCheck, ExceptionInfo exceptionInfo)
        : m_call(call)
        , m_function(function)
        , m_exceptionCheck(exceptionCheck)
        , m_exceptionInfo(exceptionInfo)
    {
    }

    MacroAssembler::Call m_call;
    FunctionPtr m_function;
    MacroAssembler::Jump m_exceptionCheck;
    ExceptionInfo m_exceptionInfo;
};

// === JITCompiler ===
//
// DFG::JITCompiler is responsible for generating JIT code from the dataflow graph.
// It does so by delegating to the speculative & non-speculative JITs, which
// generate to a MacroAssembler (which the JITCompiler owns through an inheritance
// relationship). The JITCompiler holds references to information required during
// compilation, and also records information used in linking (e.g. a list of all
// call to be linked).
class JITCompiler : public MacroAssembler {
public:
    JITCompiler(JSGlobalData* globalData, Graph& dfg, CodeBlock* codeBlock)
        : m_globalData(globalData)
        , m_graph(dfg)
        , m_codeBlock(codeBlock)
    {
    }

    void compileFunction(JITCode& entry, MacroAssemblerCodePtr& entryWithArityCheck);

    // Accessors for properties.
    Graph& graph() { return m_graph; }
    CodeBlock* codeBlock() { return m_codeBlock; }
    JSGlobalData* globalData() { return m_globalData; }

#if CPU(X86_64)
    void preserveReturnAddressAfterCall(GPRReg reg)
    {
        pop(reg);
    }

    void restoreReturnAddressBeforeReturn(GPRReg reg)
    {
        push(reg);
    }

    void restoreReturnAddressBeforeReturn(Address address)
    {
        push(address);
    }

    void emitGetFromCallFrameHeaderPtr(RegisterFile::CallFrameHeaderEntry entry, GPRReg to)
    {
        loadPtr(Address(GPRInfo::callFrameRegister, entry * sizeof(Register)), to);
    }
    void emitPutToCallFrameHeader(GPRReg from, RegisterFile::CallFrameHeaderEntry entry)
    {
        storePtr(from, Address(GPRInfo::callFrameRegister, entry * sizeof(Register)));
    }

    void emitPutImmediateToCallFrameHeader(void* value, RegisterFile::CallFrameHeaderEntry entry)
    {
        storePtr(TrustedImmPtr(value), Address(GPRInfo::callFrameRegister, entry * sizeof(Register)));
    }
#endif

    static Address addressForGlobalVar(GPRReg global, int32_t varNumber)
    {
        return Address(global, varNumber * sizeof(Register));
    }

    static Address addressFor(VirtualRegister virtualRegister)
    {
        return Address(GPRInfo::callFrameRegister, virtualRegister * sizeof(Register));
    }

    static Address tagFor(VirtualRegister virtualRegister)
    {
        return Address(GPRInfo::callFrameRegister, virtualRegister * sizeof(Register) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag));
    }

    static Address payloadFor(VirtualRegister virtualRegister)
    {
        return Address(GPRInfo::callFrameRegister, virtualRegister * sizeof(Register) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload));
    }

    // Add a call out from JIT code, without an exception check.
    void appendCall(const FunctionPtr& function)
    {
        m_calls.append(CallRecord(call(), function));
        // FIXME: should be able to JIT_ASSERT here that globalData->exception is null on return back to JIT code.
    }

    // Add a call out from JIT code, with an exception check.
    void appendCallWithExceptionCheck(const FunctionPtr& function, unsigned exceptionInfo)
    {
        Call functionCall = call();
        Jump exceptionCheck = branchTestPtr(NonZero, AbsoluteAddress(&globalData()->exception));
        m_calls.append(CallRecord(functionCall, function, exceptionCheck, exceptionInfo));
    }

    // Helper methods to check nodes for constants.
    bool isConstant(NodeIndex nodeIndex)
    {
        return graph()[nodeIndex].isConstant();
    }
    bool isInt32Constant(NodeIndex nodeIndex)
    {
        return graph()[nodeIndex].op == Int32Constant;
    }
    bool isDoubleConstant(NodeIndex nodeIndex)
    {
        return graph()[nodeIndex].op == DoubleConstant;
    }
    bool isJSConstant(NodeIndex nodeIndex)
    {
        return graph()[nodeIndex].op == JSConstant;
    }

    // Helper methods get constant values from nodes.
    int32_t valueOfInt32Constant(NodeIndex nodeIndex)
    {
        ASSERT(isInt32Constant(nodeIndex));
        return graph()[nodeIndex].int32Constant();
    }
    double valueOfDoubleConstant(NodeIndex nodeIndex)
    {
        ASSERT(isDoubleConstant(nodeIndex));
        return graph()[nodeIndex].numericConstant();
    }
    JSValue valueOfJSConstant(NodeIndex nodeIndex)
    {
        ASSERT(isJSConstant(nodeIndex));
        unsigned constantIndex = graph()[nodeIndex].constantNumber();
        return codeBlock()->constantRegister(FirstConstantRegisterIndex + constantIndex).get();
    }

    // These methods JIT generate dynamic, debug-only checks - akin to ASSERTs.
#if DFG_JIT_ASSERT
    void jitAssertIsInt32(GPRReg);
    void jitAssertIsJSInt32(GPRReg);
    void jitAssertIsJSNumber(GPRReg);
    void jitAssertIsJSDouble(GPRReg);
#else
    void jitAssertIsInt32(GPRReg) {}
    void jitAssertIsJSInt32(GPRReg) {}
    void jitAssertIsJSNumber(GPRReg) {}
    void jitAssertIsJSDouble(GPRReg) {}
#endif

#if ENABLE(SAMPLING_COUNTERS)
    // Debug profiling tool.
    void emitCount(AbstractSamplingCounter&, uint32_t increment = 1);
#endif

#if ENABLE(SAMPLING_FLAGS)
    void setSamplingFlag(int32_t flag);
    void clearSamplingFlag(int32_t flag);
#endif

private:
    // These methods used in linking the speculative & non-speculative paths together.
    void fillNumericToDouble(NodeIndex, FPRReg, GPRReg temporary);
    void fillInt32ToInteger(NodeIndex, GPRReg);
    void fillToJS(NodeIndex, GPRReg);
    void jumpFromSpeculativeToNonSpeculative(const SpeculationCheck&, const EntryLocation&, SpeculationRecovery*);
    void linkSpeculationChecks(SpeculativeJIT&, NonSpeculativeJIT&);

    // The globalData, used to access constants such as the vPtrs.
    JSGlobalData* m_globalData;

    // The dataflow graph currently being generated.
    Graph& m_graph;

    // The codeBlock currently being generated, used to access information such as constant values, immediates.
    CodeBlock* m_codeBlock;

    // Vector of calls out from JIT code, including exception handler information.
    Vector<CallRecord> m_calls;
};

} } // namespace JSC::DFG

#endif
#endif

