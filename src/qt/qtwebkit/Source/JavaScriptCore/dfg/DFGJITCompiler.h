/*
 * Copyright (C) 2011, 2013 Apple Inc. All rights reserved.
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

#include "CodeBlock.h"
#include "DFGCCallHelpers.h"
#include "DFGDisassembler.h"
#include "DFGFPRInfo.h"
#include "DFGGPRInfo.h"
#include "DFGGraph.h"
#include "DFGOSRExitCompilationInfo.h"
#include "DFGRegisterBank.h"
#include "DFGRegisterSet.h"
#include "JITCode.h"
#include "LinkBuffer.h"
#include "MacroAssembler.h"

namespace JSC {

class AbstractSamplingCounter;
class CodeBlock;
class VM;

namespace DFG {

class JITCodeGenerator;
class NodeToRegisterMap;
class OSRExitJumpPlaceholder;
class SlowPathGenerator;
class SpeculativeJIT;
class SpeculationRecovery;

struct EntryLocation;
struct OSRExit;

// === CallLinkRecord ===
//
// A record of a call out from JIT code that needs linking to a helper function.
// Every CallLinkRecord contains a reference to the call instruction & the function
// that it needs to be linked to.
struct CallLinkRecord {
    CallLinkRecord(MacroAssembler::Call call, FunctionPtr function)
        : m_call(call)
        , m_function(function)
    {
    }

    MacroAssembler::Call m_call;
    FunctionPtr m_function;
};

class CallBeginToken {
public:
    CallBeginToken()
#if !ASSERT_DISABLED
        : m_registered(false)
        , m_exceptionCheckIndex(std::numeric_limits<unsigned>::max())
#endif
    {
    }
    
    ~CallBeginToken()
    {
        ASSERT(m_registered || !m_codeOrigin.isSet());
        ASSERT(m_codeOrigin.isSet() == (m_exceptionCheckIndex != std::numeric_limits<unsigned>::max()));
    }
    
    void set(CodeOrigin codeOrigin, unsigned index)
    {
#if !ASSERT_DISABLED
        ASSERT(m_registered || !m_codeOrigin.isSet());
        ASSERT(m_codeOrigin.isSet() == (m_exceptionCheckIndex != std::numeric_limits<unsigned>::max()));
        m_codeOrigin = codeOrigin;
        m_registered = false;
        m_exceptionCheckIndex = index;
#else
        UNUSED_PARAM(codeOrigin);
        UNUSED_PARAM(index);
#endif
    }
    
    void registerWithExceptionCheck(CodeOrigin codeOrigin, unsigned index)
    {
#if !ASSERT_DISABLED
        ASSERT(m_codeOrigin == codeOrigin);
        if (m_registered)
            return;
        ASSERT(m_exceptionCheckIndex == index);
        m_registered = true;
#else
        UNUSED_PARAM(codeOrigin);
        UNUSED_PARAM(index);
#endif
    }

#if !ASSERT_DISABLED
    const CodeOrigin& codeOrigin() const
    {
        return m_codeOrigin;
    }
#endif
    
private:
#if !ASSERT_DISABLED
    CodeOrigin m_codeOrigin;
    bool m_registered;
    unsigned m_exceptionCheckIndex;
#endif
};

// === CallExceptionRecord ===
//
// A record of a call out from JIT code that might throw an exception.
// Calls that might throw an exception also record the Jump taken on exception
// (unset if not present) and code origin used to recover handler/source info.
struct CallExceptionRecord {
    CallExceptionRecord(MacroAssembler::Call call, CodeOrigin codeOrigin)
        : m_call(call)
        , m_codeOrigin(codeOrigin)
    {
    }

    CallExceptionRecord(MacroAssembler::Call call, MacroAssembler::Jump exceptionCheck, CodeOrigin codeOrigin)
        : m_call(call)
        , m_exceptionCheck(exceptionCheck)
        , m_codeOrigin(codeOrigin)
    {
    }

    MacroAssembler::Call m_call;
    MacroAssembler::Jump m_exceptionCheck;
    CodeOrigin m_codeOrigin;
};

struct PropertyAccessRecord {
    enum RegisterMode { RegistersFlushed, RegistersInUse };
    
#if USE(JSVALUE64)
    PropertyAccessRecord(
        CodeOrigin codeOrigin,
        MacroAssembler::DataLabelPtr structureImm,
        MacroAssembler::PatchableJump structureCheck,
        MacroAssembler::ConvertibleLoadLabel propertyStorageLoad,
        MacroAssembler::DataLabelCompact loadOrStore,
        SlowPathGenerator* slowPathGenerator,
        MacroAssembler::Label done,
        int8_t baseGPR,
        int8_t valueGPR,
        const RegisterSet& usedRegisters,
        RegisterMode registerMode = RegistersInUse)
#elif USE(JSVALUE32_64)
    PropertyAccessRecord(
        CodeOrigin codeOrigin,
        MacroAssembler::DataLabelPtr structureImm,
        MacroAssembler::PatchableJump structureCheck,
        MacroAssembler::ConvertibleLoadLabel propertyStorageLoad,
        MacroAssembler::DataLabelCompact tagLoadOrStore,
        MacroAssembler::DataLabelCompact payloadLoadOrStore,
        SlowPathGenerator* slowPathGenerator,
        MacroAssembler::Label done,
        int8_t baseGPR,
        int8_t valueTagGPR,
        int8_t valueGPR,
        const RegisterSet& usedRegisters,
        RegisterMode registerMode = RegistersInUse)
#endif
        : m_codeOrigin(codeOrigin)
        , m_structureImm(structureImm)
        , m_structureCheck(structureCheck)
        , m_propertyStorageLoad(propertyStorageLoad)
#if USE(JSVALUE64)
        , m_loadOrStore(loadOrStore)
#elif USE(JSVALUE32_64)
        , m_tagLoadOrStore(tagLoadOrStore)
        , m_payloadLoadOrStore(payloadLoadOrStore)
#endif
        , m_slowPathGenerator(slowPathGenerator)
        , m_done(done)
        , m_baseGPR(baseGPR)
#if USE(JSVALUE32_64)
        , m_valueTagGPR(valueTagGPR)
#endif
        , m_valueGPR(valueGPR)
        , m_usedRegisters(usedRegisters)
        , m_registerMode(registerMode)
    {
    }

    CodeOrigin m_codeOrigin;
    MacroAssembler::DataLabelPtr m_structureImm;
    MacroAssembler::PatchableJump m_structureCheck;
    MacroAssembler::ConvertibleLoadLabel m_propertyStorageLoad;
#if USE(JSVALUE64)
    MacroAssembler::DataLabelCompact m_loadOrStore;
#elif USE(JSVALUE32_64)
    MacroAssembler::DataLabelCompact m_tagLoadOrStore;
    MacroAssembler::DataLabelCompact m_payloadLoadOrStore;
#endif
    SlowPathGenerator* m_slowPathGenerator;
    MacroAssembler::Label m_done;
    int8_t m_baseGPR;
#if USE(JSVALUE32_64)
    int8_t m_valueTagGPR;
#endif
    int8_t m_valueGPR;
    RegisterSet m_usedRegisters;
    RegisterMode m_registerMode;
};

// === JITCompiler ===
//
// DFG::JITCompiler is responsible for generating JIT code from the dataflow graph.
// It does so by delegating to the speculative & non-speculative JITs, which
// generate to a MacroAssembler (which the JITCompiler owns through an inheritance
// relationship). The JITCompiler holds references to information required during
// compilation, and also records information used in linking (e.g. a list of all
// call to be linked).
class JITCompiler : public CCallHelpers {
public:
    JITCompiler(Graph& dfg);
    
    bool compile(JITCode& entry);
    bool compileFunction(JITCode& entry, MacroAssemblerCodePtr& entryWithArityCheck);

    // Accessors for properties.
    Graph& graph() { return m_graph; }
    
    // Methods to set labels for the disassembler.
    void setStartOfCode()
    {
        if (LIKELY(!m_disassembler))
            return;
        m_disassembler->setStartOfCode(labelIgnoringWatchpoints());
    }
    
    void setForBlock(BlockIndex blockIndex)
    {
        if (LIKELY(!m_disassembler))
            return;
        m_disassembler->setForBlock(blockIndex, labelIgnoringWatchpoints());
    }
    
    void setForNode(Node* node)
    {
        if (LIKELY(!m_disassembler))
            return;
        m_disassembler->setForNode(node, labelIgnoringWatchpoints());
    }
    
    void setEndOfMainPath()
    {
        if (LIKELY(!m_disassembler))
            return;
        m_disassembler->setEndOfMainPath(labelIgnoringWatchpoints());
    }
    
    void setEndOfCode()
    {
        if (LIKELY(!m_disassembler))
            return;
        m_disassembler->setEndOfCode(labelIgnoringWatchpoints());
    }
    
    unsigned currentCodeOriginIndex() const
    {
        return m_currentCodeOriginIndex;
    }
    
    // Get a token for beginning a call, and set the current code origin index in
    // the call frame. For each beginCall() there must be at least one exception
    // check, and all of the exception checks must have the same CodeOrigin as the
    // beginCall().
    void beginCall(CodeOrigin codeOrigin, CallBeginToken& token)
    {
        unsigned index = m_exceptionChecks.size();
        store32(TrustedImm32(index), tagFor(static_cast<VirtualRegister>(JSStack::ArgumentCount)));
        token.set(codeOrigin, index);
    }

    // Notify the JIT of a call that does not require linking.
    void notifyCall(Call functionCall, CodeOrigin codeOrigin, CallBeginToken& token)
    {
        token.registerWithExceptionCheck(codeOrigin, m_exceptionChecks.size());
        m_exceptionChecks.append(CallExceptionRecord(functionCall, codeOrigin));
    }

    // Add a call out from JIT code, without an exception check.
    Call appendCall(const FunctionPtr& function)
    {
        Call functionCall = call();
        m_calls.append(CallLinkRecord(functionCall, function));
        return functionCall;
    }
    
    void prepareForExceptionCheck()
    {
        move(TrustedImm32(m_exceptionChecks.size()), GPRInfo::nonPreservedNonReturnGPR);
    }

    // Add a call out from JIT code, with an exception check.
    void addExceptionCheck(Call functionCall, CodeOrigin codeOrigin, CallBeginToken& token)
    {
        prepareForExceptionCheck();
        token.registerWithExceptionCheck(codeOrigin, m_exceptionChecks.size());
        m_exceptionChecks.append(CallExceptionRecord(functionCall, emitExceptionCheck(), codeOrigin));
    }
    
    // Add a call out from JIT code, with a fast exception check that tests if the return value is zero.
    void addFastExceptionCheck(Call functionCall, CodeOrigin codeOrigin, CallBeginToken& token)
    {
        prepareForExceptionCheck();
        Jump exceptionCheck = branchTestPtr(Zero, GPRInfo::returnValueGPR);
        token.registerWithExceptionCheck(codeOrigin, m_exceptionChecks.size());
        m_exceptionChecks.append(CallExceptionRecord(functionCall, exceptionCheck, codeOrigin));
    }
    
    void appendExitInfo(MacroAssembler::JumpList jumpsToFail = MacroAssembler::JumpList())
    {
        OSRExitCompilationInfo info;
        info.m_failureJumps = jumpsToFail;
        m_exitCompilationInfo.append(info);
    }

#if USE(JSVALUE32_64)
    void* addressOfDoubleConstant(Node* node)
    {
        ASSERT(m_graph.isNumberConstant(node));
        unsigned constantIndex = node->constantNumber();
        return &(codeBlock()->constantRegister(FirstConstantRegisterIndex + constantIndex));
    }
#endif

    void addPropertyAccess(const PropertyAccessRecord& record)
    {
        m_propertyAccesses.append(record);
    }

    void addJSCall(Call fastCall, Call slowCall, DataLabelPtr targetToCheck, CallLinkInfo::CallType callType, GPRReg callee, CodeOrigin codeOrigin)
    {
        m_jsCalls.append(JSCallRecord(fastCall, slowCall, targetToCheck, callType, callee, codeOrigin));
    }
    
    void addWeakReference(JSCell* target)
    {
        m_codeBlock->appendWeakReference(target);
    }
    
    void addWeakReferences(const StructureSet& structureSet)
    {
        for (unsigned i = structureSet.size(); i--;)
            addWeakReference(structureSet[i]);
    }
    
    void addWeakReferenceTransition(JSCell* codeOrigin, JSCell* from, JSCell* to)
    {
        m_codeBlock->appendWeakReferenceTransition(codeOrigin, from, to);
    }
    
    template<typename T>
    Jump branchWeakPtr(RelationalCondition cond, T left, JSCell* weakPtr)
    {
        Jump result = branchPtr(cond, left, TrustedImmPtr(weakPtr));
        addWeakReference(weakPtr);
        return result;
    }
    
    void noticeOSREntry(BasicBlock& basicBlock, JITCompiler::Label blockHead, LinkBuffer& linkBuffer)
    {
#if DFG_ENABLE(OSR_ENTRY)
        // OSR entry is not allowed into blocks deemed unreachable by control flow analysis.
        if (!basicBlock.cfaHasVisited)
            return;
        
        OSREntryData* entry = codeBlock()->appendDFGOSREntryData(basicBlock.bytecodeBegin, linkBuffer.offsetOf(blockHead));
        
        entry->m_expectedValues = basicBlock.valuesAtHead;
        
        // Fix the expected values: in our protocol, a dead variable will have an expected
        // value of (None, []). But the old JIT may stash some values there. So we really
        // need (Top, TOP).
        for (size_t argument = 0; argument < basicBlock.variablesAtHead.numberOfArguments(); ++argument) {
            Node* node = basicBlock.variablesAtHead.argument(argument);
            if (!node || !node->shouldGenerate())
                entry->m_expectedValues.argument(argument).makeTop();
        }
        for (size_t local = 0; local < basicBlock.variablesAtHead.numberOfLocals(); ++local) {
            Node* node = basicBlock.variablesAtHead.local(local);
            if (!node || !node->shouldGenerate())
                entry->m_expectedValues.local(local).makeTop();
            else if (node->variableAccessData()->shouldUseDoubleFormat())
                entry->m_localsForcedDouble.set(local);
        }
#else
        UNUSED_PARAM(basicBlock);
        UNUSED_PARAM(blockHead);
        UNUSED_PARAM(linkBuffer);
#endif
    }

private:
    friend class OSRExitJumpPlaceholder;
    
    // Internal implementation to compile.
    void compileEntry();
    void compileBody(SpeculativeJIT&);
    void link(LinkBuffer&);

    void exitSpeculativeWithOSR(const OSRExit&, SpeculationRecovery*);
    void compileExceptionHandlers();
    void linkOSRExits();
    
    // The dataflow graph currently being generated.
    Graph& m_graph;

    OwnPtr<Disassembler> m_disassembler;
    
    // Vector of calls out from JIT code, including exception handler information.
    // Count of the number of CallRecords with exception handlers.
    Vector<CallLinkRecord> m_calls;
    Vector<CallExceptionRecord> m_exceptionChecks;
    
    struct JSCallRecord {
        JSCallRecord(Call fastCall, Call slowCall, DataLabelPtr targetToCheck, CallLinkInfo::CallType callType, GPRReg callee, CodeOrigin codeOrigin)
            : m_fastCall(fastCall)
            , m_slowCall(slowCall)
            , m_targetToCheck(targetToCheck)
            , m_callType(callType)
            , m_callee(callee)
            , m_codeOrigin(codeOrigin)
        {
        }
        
        Call m_fastCall;
        Call m_slowCall;
        DataLabelPtr m_targetToCheck;
        CallLinkInfo::CallType m_callType;
        GPRReg m_callee;
        CodeOrigin m_codeOrigin;
    };
    
    Vector<PropertyAccessRecord, 4> m_propertyAccesses;
    Vector<JSCallRecord, 4> m_jsCalls;
    Vector<OSRExitCompilationInfo> m_exitCompilationInfo;
    Vector<Vector<Label> > m_exitSiteLabels;
    unsigned m_currentCodeOriginIndex;
};

} } // namespace JSC::DFG

#endif
#endif

