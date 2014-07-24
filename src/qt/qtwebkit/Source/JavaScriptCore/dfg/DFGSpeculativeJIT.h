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

#ifndef DFGSpeculativeJIT_h
#define DFGSpeculativeJIT_h

#include <wtf/Platform.h>

#if ENABLE(DFG_JIT)

#include "DFGAbstractState.h"
#include "DFGGenerationInfo.h"
#include "DFGJITCompiler.h"
#include "DFGOSRExit.h"
#include "DFGOSRExitJumpPlaceholder.h"
#include "DFGOperations.h"
#include "DFGSilentRegisterSavePlan.h"
#include "DFGValueSource.h"
#include "MarkedAllocator.h"
#include "ValueRecovery.h"

namespace JSC { namespace DFG {

class GPRTemporary;
class JSValueOperand;
class SlowPathGenerator;
class SpeculativeJIT;
class SpeculateIntegerOperand;
class SpeculateStrictInt32Operand;
class SpeculateDoubleOperand;
class SpeculateCellOperand;
class SpeculateBooleanOperand;

enum GeneratedOperandType { GeneratedOperandTypeUnknown, GeneratedOperandInteger, GeneratedOperandDouble, GeneratedOperandJSValue};

// === SpeculativeJIT ===
//
// The SpeculativeJIT is used to generate a fast, but potentially
// incomplete code path for the dataflow. When code generating
// we may make assumptions about operand types, dynamically check,
// and bail-out to an alternate code path if these checks fail.
// Importantly, the speculative code path cannot be reentered once
// a speculative check has failed. This allows the SpeculativeJIT
// to propagate type information (including information that has
// only speculatively been asserted) through the dataflow.
class SpeculativeJIT {
    friend struct OSRExit;
private:
    typedef JITCompiler::TrustedImm32 TrustedImm32;
    typedef JITCompiler::Imm32 Imm32;
    typedef JITCompiler::TrustedImmPtr TrustedImmPtr;
    typedef JITCompiler::ImmPtr ImmPtr;
    typedef JITCompiler::TrustedImm64 TrustedImm64;
    typedef JITCompiler::Imm64 Imm64;

    // These constants are used to set priorities for spill order for
    // the register allocator.
#if USE(JSVALUE64)
    enum SpillOrder {
        SpillOrderConstant = 1, // no spill, and cheap fill
        SpillOrderSpilled  = 2, // no spill
        SpillOrderJS       = 4, // needs spill
        SpillOrderCell     = 4, // needs spill
        SpillOrderStorage  = 4, // needs spill
        SpillOrderInteger  = 5, // needs spill and box
        SpillOrderBoolean  = 5, // needs spill and box
        SpillOrderDouble   = 6, // needs spill and convert
    };
#elif USE(JSVALUE32_64)
    enum SpillOrder {
        SpillOrderConstant = 1, // no spill, and cheap fill
        SpillOrderSpilled  = 2, // no spill
        SpillOrderJS       = 4, // needs spill
        SpillOrderStorage  = 4, // needs spill
        SpillOrderDouble   = 4, // needs spill
        SpillOrderInteger  = 5, // needs spill and box
        SpillOrderCell     = 5, // needs spill and box
        SpillOrderBoolean  = 5, // needs spill and box
    };
#endif

    enum UseChildrenMode { CallUseChildren, UseChildrenCalledExplicitly };
    
public:
    SpeculativeJIT(JITCompiler&);
    ~SpeculativeJIT();

    bool compile();
    void createOSREntries();
    void linkOSREntries(LinkBuffer&);

    BlockIndex nextBlock()
    {
        for (BlockIndex result = m_block + 1; ; result++) {
            if (result >= m_jit.graph().m_blocks.size())
                return NoBlock;
            if (m_jit.graph().m_blocks[result])
                return result;
        }
    }
    
    GPRReg fillInteger(Edge, DataFormat& returnFormat);
#if USE(JSVALUE64)
    GPRReg fillJSValue(Edge);
#elif USE(JSVALUE32_64)
    bool fillJSValue(Edge, GPRReg&, GPRReg&, FPRReg&);
#endif
    GPRReg fillStorage(Edge);

    // lock and unlock GPR & FPR registers.
    void lock(GPRReg reg)
    {
        m_gprs.lock(reg);
    }
    void lock(FPRReg reg)
    {
        m_fprs.lock(reg);
    }
    void unlock(GPRReg reg)
    {
        m_gprs.unlock(reg);
    }
    void unlock(FPRReg reg)
    {
        m_fprs.unlock(reg);
    }

    // Used to check whether a child node is on its last use,
    // and its machine registers may be reused.
    bool canReuse(Node* node)
    {
        VirtualRegister virtualRegister = node->virtualRegister();
        GenerationInfo& info = m_generationInfo[virtualRegister];
        return info.canReuse();
    }
    bool canReuse(Edge nodeUse)
    {
        return canReuse(nodeUse.node());
    }
    GPRReg reuse(GPRReg reg)
    {
        m_gprs.lock(reg);
        return reg;
    }
    FPRReg reuse(FPRReg reg)
    {
        m_fprs.lock(reg);
        return reg;
    }

    // Allocate a gpr/fpr.
    GPRReg allocate()
    {
#if ENABLE(DFG_REGISTER_ALLOCATION_VALIDATION)
        m_jit.addRegisterAllocationAtOffset(m_jit.debugOffset());
#endif
        VirtualRegister spillMe;
        GPRReg gpr = m_gprs.allocate(spillMe);
        if (spillMe != InvalidVirtualRegister) {
#if USE(JSVALUE32_64)
            GenerationInfo& info = m_generationInfo[spillMe];
            RELEASE_ASSERT(info.registerFormat() != DataFormatJSDouble);
            if ((info.registerFormat() & DataFormatJS))
                m_gprs.release(info.tagGPR() == gpr ? info.payloadGPR() : info.tagGPR());
#endif
            spill(spillMe);
        }
        return gpr;
    }
    GPRReg allocate(GPRReg specific)
    {
#if ENABLE(DFG_REGISTER_ALLOCATION_VALIDATION)
        m_jit.addRegisterAllocationAtOffset(m_jit.debugOffset());
#endif
        VirtualRegister spillMe = m_gprs.allocateSpecific(specific);
        if (spillMe != InvalidVirtualRegister) {
#if USE(JSVALUE32_64)
            GenerationInfo& info = m_generationInfo[spillMe];
            RELEASE_ASSERT(info.registerFormat() != DataFormatJSDouble);
            if ((info.registerFormat() & DataFormatJS))
                m_gprs.release(info.tagGPR() == specific ? info.payloadGPR() : info.tagGPR());
#endif
            spill(spillMe);
        }
        return specific;
    }
    GPRReg tryAllocate()
    {
        return m_gprs.tryAllocate();
    }
    FPRReg fprAllocate()
    {
#if ENABLE(DFG_REGISTER_ALLOCATION_VALIDATION)
        m_jit.addRegisterAllocationAtOffset(m_jit.debugOffset());
#endif
        VirtualRegister spillMe;
        FPRReg fpr = m_fprs.allocate(spillMe);
        if (spillMe != InvalidVirtualRegister)
            spill(spillMe);
        return fpr;
    }

    // Check whether a VirtualRegsiter is currently in a machine register.
    // We use this when filling operands to fill those that are already in
    // machine registers first (by locking VirtualRegsiters that are already
    // in machine register before filling those that are not we attempt to
    // avoid spilling values we will need immediately).
    bool isFilled(Node* node)
    {
        VirtualRegister virtualRegister = node->virtualRegister();
        GenerationInfo& info = m_generationInfo[virtualRegister];
        return info.registerFormat() != DataFormatNone;
    }
    bool isFilledDouble(Node* node)
    {
        VirtualRegister virtualRegister = node->virtualRegister();
        GenerationInfo& info = m_generationInfo[virtualRegister];
        return info.registerFormat() == DataFormatDouble;
    }

    // Called on an operand once it has been consumed by a parent node.
    void use(Node* node)
    {
        if (!node->hasResult())
            return;
        VirtualRegister virtualRegister = node->virtualRegister();
        GenerationInfo& info = m_generationInfo[virtualRegister];

        // use() returns true when the value becomes dead, and any
        // associated resources may be freed.
        if (!info.use(*m_stream))
            return;

        // Release the associated machine registers.
        DataFormat registerFormat = info.registerFormat();
#if USE(JSVALUE64)
        if (registerFormat == DataFormatDouble)
            m_fprs.release(info.fpr());
        else if (registerFormat != DataFormatNone)
            m_gprs.release(info.gpr());
#elif USE(JSVALUE32_64)
        if (registerFormat == DataFormatDouble || registerFormat == DataFormatJSDouble)
            m_fprs.release(info.fpr());
        else if (registerFormat & DataFormatJS) {
            m_gprs.release(info.tagGPR());
            m_gprs.release(info.payloadGPR());
        } else if (registerFormat != DataFormatNone)
            m_gprs.release(info.gpr());
#endif
    }
    void use(Edge nodeUse)
    {
        use(nodeUse.node());
    }
    
    RegisterSet usedRegisters()
    {
        RegisterSet result;
        for (unsigned i = GPRInfo::numberOfRegisters; i--;) {
            GPRReg gpr = GPRInfo::toRegister(i);
            if (m_gprs.isInUse(gpr))
                result.set(gpr);
        }
        for (unsigned i = FPRInfo::numberOfRegisters; i--;) {
            FPRReg fpr = FPRInfo::toRegister(i);
            if (m_fprs.isInUse(fpr))
                result.set(fpr);
        }
        return result;
    }

    static void writeBarrier(MacroAssembler&, GPRReg ownerGPR, GPRReg scratchGPR1, GPRReg scratchGPR2, WriteBarrierUseKind);

    void writeBarrier(GPRReg ownerGPR, GPRReg valueGPR, Edge valueUse, WriteBarrierUseKind, GPRReg scratchGPR1 = InvalidGPRReg, GPRReg scratchGPR2 = InvalidGPRReg);
    void writeBarrier(GPRReg ownerGPR, JSCell* value, WriteBarrierUseKind, GPRReg scratchGPR1 = InvalidGPRReg, GPRReg scratchGPR2 = InvalidGPRReg);
    void writeBarrier(JSCell* owner, GPRReg valueGPR, Edge valueUse, WriteBarrierUseKind, GPRReg scratchGPR1 = InvalidGPRReg);

    static GPRReg selectScratchGPR(GPRReg preserve1 = InvalidGPRReg, GPRReg preserve2 = InvalidGPRReg, GPRReg preserve3 = InvalidGPRReg, GPRReg preserve4 = InvalidGPRReg)
    {
        return AssemblyHelpers::selectScratchGPR(preserve1, preserve2, preserve3, preserve4);
    }

    // Called by the speculative operand types, below, to fill operand to
    // machine registers, implicitly generating speculation checks as needed.
    GPRReg fillSpeculateInt(Edge, DataFormat& returnFormat);
    GPRReg fillSpeculateIntStrict(Edge);
    FPRReg fillSpeculateDouble(Edge);
    GPRReg fillSpeculateCell(Edge);
    GPRReg fillSpeculateBoolean(Edge);
    GeneratedOperandType checkGeneratedTypeForToInt32(Node*);

    void addSlowPathGenerator(PassOwnPtr<SlowPathGenerator>);
    void runSlowPathGenerators();
    
    void compile(Node*);
    void noticeOSRBirth(Node*);
    void compile(BasicBlock&);

    void checkArgumentTypes();

    void clearGenerationInfo();

    // These methods are used when generating 'unexpected'
    // calls out from JIT code to C++ helper routines -
    // they spill all live values to the appropriate
    // slots in the JSStack without changing any state
    // in the GenerationInfo.
    SilentRegisterSavePlan silentSavePlanForGPR(VirtualRegister spillMe, GPRReg source);
    SilentRegisterSavePlan silentSavePlanForFPR(VirtualRegister spillMe, FPRReg source);
    void silentSpill(const SilentRegisterSavePlan&);
    void silentFill(const SilentRegisterSavePlan&, GPRReg canTrample);
    
    template<typename CollectionType>
    void silentSpillAllRegistersImpl(bool doSpill, CollectionType& plans, GPRReg exclude, GPRReg exclude2 = InvalidGPRReg, FPRReg fprExclude = InvalidFPRReg)
    {
        ASSERT(plans.isEmpty());
        for (gpr_iterator iter = m_gprs.begin(); iter != m_gprs.end(); ++iter) {
            GPRReg gpr = iter.regID();
            if (iter.name() != InvalidVirtualRegister && gpr != exclude && gpr != exclude2) {
                SilentRegisterSavePlan plan = silentSavePlanForGPR(iter.name(), gpr);
                if (doSpill)
                    silentSpill(plan);
                plans.append(plan);
            }
        }
        for (fpr_iterator iter = m_fprs.begin(); iter != m_fprs.end(); ++iter) {
            if (iter.name() != InvalidVirtualRegister && iter.regID() != fprExclude) {
                SilentRegisterSavePlan plan = silentSavePlanForFPR(iter.name(), iter.regID());
                if (doSpill)
                    silentSpill(plan);
                plans.append(plan);
            }
        }
    }
    template<typename CollectionType>
    void silentSpillAllRegistersImpl(bool doSpill, CollectionType& plans, NoResultTag)
    {
        silentSpillAllRegistersImpl(doSpill, plans, InvalidGPRReg, InvalidGPRReg, InvalidFPRReg);
    }
    template<typename CollectionType>
    void silentSpillAllRegistersImpl(bool doSpill, CollectionType& plans, FPRReg exclude)
    {
        silentSpillAllRegistersImpl(doSpill, plans, InvalidGPRReg, InvalidGPRReg, exclude);
    }
#if USE(JSVALUE32_64)
    template<typename CollectionType>
    void silentSpillAllRegistersImpl(bool doSpill, CollectionType& plans, JSValueRegs exclude)
    {
        silentSpillAllRegistersImpl(doSpill, plans, exclude.tagGPR(), exclude.payloadGPR());
    }
#endif
    
    void silentSpillAllRegisters(GPRReg exclude, GPRReg exclude2 = InvalidGPRReg, FPRReg fprExclude = InvalidFPRReg)
    {
        silentSpillAllRegistersImpl(true, m_plans, exclude, exclude2, fprExclude);
    }
    void silentSpillAllRegisters(FPRReg exclude)
    {
        silentSpillAllRegisters(InvalidGPRReg, InvalidGPRReg, exclude);
    }
    
    static GPRReg pickCanTrample(GPRReg exclude)
    {
        GPRReg result = GPRInfo::regT0;
        if (result == exclude)
            result = GPRInfo::regT1;
        return result;
    }
    static GPRReg pickCanTrample(FPRReg)
    {
        return GPRInfo::regT0;
    }
    static GPRReg pickCanTrample(NoResultTag)
    {
        return GPRInfo::regT0;
    }

#if USE(JSVALUE32_64)
    static GPRReg pickCanTrample(JSValueRegs exclude)
    {
        GPRReg result = GPRInfo::regT0;
        if (result == exclude.tagGPR()) {
            result = GPRInfo::regT1;
            if (result == exclude.payloadGPR())
                result = GPRInfo::regT2;
        } else if (result == exclude.payloadGPR()) {
            result = GPRInfo::regT1;
            if (result == exclude.tagGPR())
                result = GPRInfo::regT2;
        }
        return result;
    }
#endif
    
    template<typename RegisterType>
    void silentFillAllRegisters(RegisterType exclude)
    {
        GPRReg canTrample = pickCanTrample(exclude);
        
        while (!m_plans.isEmpty()) {
            SilentRegisterSavePlan& plan = m_plans.last();
            silentFill(plan, canTrample);
            m_plans.removeLast();
        }
    }

    // These methods convert between doubles, and doubles boxed and JSValues.
#if USE(JSVALUE64)
    GPRReg boxDouble(FPRReg fpr, GPRReg gpr)
    {
        return m_jit.boxDouble(fpr, gpr);
    }
    FPRReg unboxDouble(GPRReg gpr, FPRReg fpr)
    {
        return m_jit.unboxDouble(gpr, fpr);
    }
    GPRReg boxDouble(FPRReg fpr)
    {
        return boxDouble(fpr, allocate());
    }
#elif USE(JSVALUE32_64)
    void boxDouble(FPRReg fpr, GPRReg tagGPR, GPRReg payloadGPR)
    {
        m_jit.boxDouble(fpr, tagGPR, payloadGPR);
    }
    void unboxDouble(GPRReg tagGPR, GPRReg payloadGPR, FPRReg fpr, FPRReg scratchFPR)
    {
        m_jit.unboxDouble(tagGPR, payloadGPR, fpr, scratchFPR);
    }
#endif

    // Spill a VirtualRegister to the JSStack.
    void spill(VirtualRegister spillMe)
    {
        GenerationInfo& info = m_generationInfo[spillMe];

#if USE(JSVALUE32_64)
        if (info.registerFormat() == DataFormatNone) // it has been spilled. JS values which have two GPRs can reach here
            return;
#endif
        // Check the GenerationInfo to see if this value need writing
        // to the JSStack - if not, mark it as spilled & return.
        if (!info.needsSpill()) {
            info.setSpilled(*m_stream, spillMe);
            return;
        }

        DataFormat spillFormat = info.registerFormat();
        switch (spillFormat) {
        case DataFormatStorage: {
            // This is special, since it's not a JS value - as in it's not visible to JS
            // code.
            m_jit.storePtr(info.gpr(), JITCompiler::addressFor(spillMe));
            info.spill(*m_stream, spillMe, DataFormatStorage);
            return;
        }

        case DataFormatInteger: {
            m_jit.store32(info.gpr(), JITCompiler::payloadFor(spillMe));
            info.spill(*m_stream, spillMe, DataFormatInteger);
            return;
        }

#if USE(JSVALUE64)
        case DataFormatDouble: {
            m_jit.storeDouble(info.fpr(), JITCompiler::addressFor(spillMe));
            info.spill(*m_stream, spillMe, DataFormatDouble);
            return;
        }
            
        default:
            // The following code handles JSValues, int32s, and cells.
            RELEASE_ASSERT(spillFormat == DataFormatCell || spillFormat & DataFormatJS);
            
            GPRReg reg = info.gpr();
            // We need to box int32 and cell values ...
            // but on JSVALUE64 boxing a cell is a no-op!
            if (spillFormat == DataFormatInteger)
                m_jit.or64(GPRInfo::tagTypeNumberRegister, reg);
            
            // Spill the value, and record it as spilled in its boxed form.
            m_jit.store64(reg, JITCompiler::addressFor(spillMe));
            info.spill(*m_stream, spillMe, (DataFormat)(spillFormat | DataFormatJS));
            return;
#elif USE(JSVALUE32_64)
        case DataFormatCell:
        case DataFormatBoolean: {
            m_jit.store32(info.gpr(), JITCompiler::payloadFor(spillMe));
            info.spill(*m_stream, spillMe, spillFormat);
            return;
        }

        case DataFormatDouble:
        case DataFormatJSDouble: {
            // On JSVALUE32_64 boxing a double is a no-op.
            m_jit.storeDouble(info.fpr(), JITCompiler::addressFor(spillMe));
            info.spill(*m_stream, spillMe, DataFormatJSDouble);
            return;
        }

        default:
            // The following code handles JSValues.
            RELEASE_ASSERT(spillFormat & DataFormatJS);
            m_jit.store32(info.tagGPR(), JITCompiler::tagFor(spillMe));
            m_jit.store32(info.payloadGPR(), JITCompiler::payloadFor(spillMe));
            info.spill(*m_stream, spillMe, spillFormat);
            return;
#endif
        }
    }
    
    bool isKnownInteger(Node* node) { return !(m_state.forNode(node).m_type & ~SpecInt32); }
    bool isKnownCell(Node* node) { return !(m_state.forNode(node).m_type & ~SpecCell); }
    
    bool isKnownNotInteger(Node* node) { return !(m_state.forNode(node).m_type & SpecInt32); }
    bool isKnownNotNumber(Node* node) { return !(m_state.forNode(node).m_type & SpecNumber); }
    bool isKnownNotCell(Node* node) { return !(m_state.forNode(node).m_type & SpecCell); }
    
    // Checks/accessors for constant values.
    bool isConstant(Node* node) { return m_jit.graph().isConstant(node); }
    bool isJSConstant(Node* node) { return m_jit.graph().isJSConstant(node); }
    bool isInt32Constant(Node* node) { return m_jit.graph().isInt32Constant(node); }
    bool isDoubleConstant(Node* node) { return m_jit.graph().isDoubleConstant(node); }
    bool isNumberConstant(Node* node) { return m_jit.graph().isNumberConstant(node); }
    bool isBooleanConstant(Node* node) { return m_jit.graph().isBooleanConstant(node); }
    bool isFunctionConstant(Node* node) { return m_jit.graph().isFunctionConstant(node); }
    int32_t valueOfInt32Constant(Node* node) { return m_jit.graph().valueOfInt32Constant(node); }
    double valueOfNumberConstant(Node* node) { return m_jit.graph().valueOfNumberConstant(node); }
#if USE(JSVALUE32_64)
    void* addressOfDoubleConstant(Node* node) { return m_jit.addressOfDoubleConstant(node); }
#endif
    JSValue valueOfJSConstant(Node* node) { return m_jit.graph().valueOfJSConstant(node); }
    bool valueOfBooleanConstant(Node* node) { return m_jit.graph().valueOfBooleanConstant(node); }
    JSFunction* valueOfFunctionConstant(Node* node) { return m_jit.graph().valueOfFunctionConstant(node); }
    bool isNullConstant(Node* node)
    {
        if (!isConstant(node))
            return false;
        return valueOfJSConstant(node).isNull();
    }

    Identifier* identifier(unsigned index)
    {
        return &m_jit.codeBlock()->identifier(index);
    }

    // Spill all VirtualRegisters back to the JSStack.
    void flushRegisters()
    {
        for (gpr_iterator iter = m_gprs.begin(); iter != m_gprs.end(); ++iter) {
            if (iter.name() != InvalidVirtualRegister) {
                spill(iter.name());
                iter.release();
            }
        }
        for (fpr_iterator iter = m_fprs.begin(); iter != m_fprs.end(); ++iter) {
            if (iter.name() != InvalidVirtualRegister) {
                spill(iter.name());
                iter.release();
            }
        }
    }

#ifndef NDEBUG
    // Used to ASSERT flushRegisters() has been called prior to
    // calling out from JIT code to a C helper function.
    bool isFlushed()
    {
        for (gpr_iterator iter = m_gprs.begin(); iter != m_gprs.end(); ++iter) {
            if (iter.name() != InvalidVirtualRegister)
                return false;
        }
        for (fpr_iterator iter = m_fprs.begin(); iter != m_fprs.end(); ++iter) {
            if (iter.name() != InvalidVirtualRegister)
                return false;
        }
        return true;
    }
#endif

#if USE(JSVALUE64)
    MacroAssembler::Imm64 valueOfJSConstantAsImm64(Node* node)
    {
        return MacroAssembler::Imm64(JSValue::encode(valueOfJSConstant(node)));
    }
#endif

    // Helper functions to enable code sharing in implementations of bit/shift ops.
    void bitOp(NodeType op, int32_t imm, GPRReg op1, GPRReg result)
    {
        switch (op) {
        case BitAnd:
            m_jit.and32(Imm32(imm), op1, result);
            break;
        case BitOr:
            m_jit.or32(Imm32(imm), op1, result);
            break;
        case BitXor:
            m_jit.xor32(Imm32(imm), op1, result);
            break;
        default:
            RELEASE_ASSERT_NOT_REACHED();
        }
    }
    void bitOp(NodeType op, GPRReg op1, GPRReg op2, GPRReg result)
    {
        switch (op) {
        case BitAnd:
            m_jit.and32(op1, op2, result);
            break;
        case BitOr:
            m_jit.or32(op1, op2, result);
            break;
        case BitXor:
            m_jit.xor32(op1, op2, result);
            break;
        default:
            RELEASE_ASSERT_NOT_REACHED();
        }
    }
    void shiftOp(NodeType op, GPRReg op1, int32_t shiftAmount, GPRReg result)
    {
        switch (op) {
        case BitRShift:
            m_jit.rshift32(op1, Imm32(shiftAmount), result);
            break;
        case BitLShift:
            m_jit.lshift32(op1, Imm32(shiftAmount), result);
            break;
        case BitURShift:
            m_jit.urshift32(op1, Imm32(shiftAmount), result);
            break;
        default:
            RELEASE_ASSERT_NOT_REACHED();
        }
    }
    void shiftOp(NodeType op, GPRReg op1, GPRReg shiftAmount, GPRReg result)
    {
        switch (op) {
        case BitRShift:
            m_jit.rshift32(op1, shiftAmount, result);
            break;
        case BitLShift:
            m_jit.lshift32(op1, shiftAmount, result);
            break;
        case BitURShift:
            m_jit.urshift32(op1, shiftAmount, result);
            break;
        default:
            RELEASE_ASSERT_NOT_REACHED();
        }
    }
    
    // Returns the index of the branch node if peephole is okay, UINT_MAX otherwise.
    unsigned detectPeepHoleBranch()
    {
        BasicBlock* block = m_jit.graph().m_blocks[m_block].get();

        // Check that no intervening nodes will be generated.
        for (unsigned index = m_indexInBlock + 1; index < block->size() - 1; ++index) {
            Node* node = block->at(index);
            if (node->shouldGenerate())
                return UINT_MAX;
        }

        // Check if the lastNode is a branch on this node.
        Node* lastNode = block->last();
        return lastNode->op() == Branch && lastNode->child1() == m_currentNode ? block->size() - 1 : UINT_MAX;
    }
    
    void compileMovHint(Node*);
    void compileMovHintAndCheck(Node*);
    void compileInlineStart(Node*);

    void nonSpeculativeUInt32ToNumber(Node*);

#if USE(JSVALUE64)
    void cachedGetById(CodeOrigin, GPRReg baseGPR, GPRReg resultGPR, unsigned identifierNumber, JITCompiler::Jump slowPathTarget = JITCompiler::Jump(), SpillRegistersMode = NeedToSpill);
    void cachedPutById(CodeOrigin, GPRReg base, GPRReg value, Edge valueUse, GPRReg scratchGPR, unsigned identifierNumber, PutKind, JITCompiler::Jump slowPathTarget = JITCompiler::Jump());
#elif USE(JSVALUE32_64)
    void cachedGetById(CodeOrigin, GPRReg baseTagGPROrNone, GPRReg basePayloadGPR, GPRReg resultTagGPR, GPRReg resultPayloadGPR, unsigned identifierNumber, JITCompiler::Jump slowPathTarget = JITCompiler::Jump(), SpillRegistersMode = NeedToSpill);
    void cachedPutById(CodeOrigin, GPRReg basePayloadGPR, GPRReg valueTagGPR, GPRReg valuePayloadGPR, Edge valueUse, GPRReg scratchGPR, unsigned identifierNumber, PutKind, JITCompiler::Jump slowPathTarget = JITCompiler::Jump());
#endif

    void nonSpeculativeNonPeepholeCompareNull(Edge operand, bool invert = false);
    void nonSpeculativePeepholeBranchNull(Edge operand, Node* branchNode, bool invert = false);
    bool nonSpeculativeCompareNull(Node*, Edge operand, bool invert = false);
    
    void nonSpeculativePeepholeBranch(Node*, Node* branchNode, MacroAssembler::RelationalCondition, S_DFGOperation_EJJ helperFunction);
    void nonSpeculativeNonPeepholeCompare(Node*, MacroAssembler::RelationalCondition, S_DFGOperation_EJJ helperFunction);
    bool nonSpeculativeCompare(Node*, MacroAssembler::RelationalCondition, S_DFGOperation_EJJ helperFunction);
    
    void nonSpeculativePeepholeStrictEq(Node*, Node* branchNode, bool invert = false);
    void nonSpeculativeNonPeepholeStrictEq(Node*, bool invert = false);
    bool nonSpeculativeStrictEq(Node*, bool invert = false);
    
    void compileInstanceOfForObject(Node*, GPRReg valueReg, GPRReg prototypeReg, GPRReg scratchAndResultReg);
    void compileInstanceOf(Node*);
    
    // Access to our fixed callee CallFrame.
    MacroAssembler::Address callFrameSlot(int slot)
    {
        return MacroAssembler::Address(GPRInfo::callFrameRegister, (m_jit.codeBlock()->m_numCalleeRegisters + slot) * static_cast<int>(sizeof(Register)));
    }

    // Access to our fixed callee CallFrame.
    MacroAssembler::Address argumentSlot(int argument)
    {
        return MacroAssembler::Address(GPRInfo::callFrameRegister, (m_jit.codeBlock()->m_numCalleeRegisters + argumentToOperand(argument)) * static_cast<int>(sizeof(Register)));
    }

    MacroAssembler::Address callFrameTagSlot(int slot)
    {
        return MacroAssembler::Address(GPRInfo::callFrameRegister, (m_jit.codeBlock()->m_numCalleeRegisters + slot) * static_cast<int>(sizeof(Register)) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag));
    }

    MacroAssembler::Address callFramePayloadSlot(int slot)
    {
        return MacroAssembler::Address(GPRInfo::callFrameRegister, (m_jit.codeBlock()->m_numCalleeRegisters + slot) * static_cast<int>(sizeof(Register)) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload));
    }

    MacroAssembler::Address argumentTagSlot(int argument)
    {
        return MacroAssembler::Address(GPRInfo::callFrameRegister, (m_jit.codeBlock()->m_numCalleeRegisters + argumentToOperand(argument)) * static_cast<int>(sizeof(Register)) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag));
    }

    MacroAssembler::Address argumentPayloadSlot(int argument)
    {
        return MacroAssembler::Address(GPRInfo::callFrameRegister, (m_jit.codeBlock()->m_numCalleeRegisters + argumentToOperand(argument)) * static_cast<int>(sizeof(Register)) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload));
    }

    void emitCall(Node*);
    
    // Called once a node has completed code generation but prior to setting
    // its result, to free up its children. (This must happen prior to setting
    // the nodes result, since the node may have the same VirtualRegister as
    // a child, and as such will use the same GeneratioInfo).
    void useChildren(Node*);

    // These method called to initialize the the GenerationInfo
    // to describe the result of an operation.
    void integerResult(GPRReg reg, Node* node, DataFormat format = DataFormatInteger, UseChildrenMode mode = CallUseChildren)
    {
        if (mode == CallUseChildren)
            useChildren(node);

        VirtualRegister virtualRegister = node->virtualRegister();
        GenerationInfo& info = m_generationInfo[virtualRegister];

        if (format == DataFormatInteger) {
            m_jit.jitAssertIsInt32(reg);
            m_gprs.retain(reg, virtualRegister, SpillOrderInteger);
            info.initInteger(node, node->refCount(), reg);
        } else {
#if USE(JSVALUE64)
            RELEASE_ASSERT(format == DataFormatJSInteger);
            m_jit.jitAssertIsJSInt32(reg);
            m_gprs.retain(reg, virtualRegister, SpillOrderJS);
            info.initJSValue(node, node->refCount(), reg, format);
#elif USE(JSVALUE32_64)
            RELEASE_ASSERT_NOT_REACHED();
#endif
        }
    }
    void integerResult(GPRReg reg, Node* node, UseChildrenMode mode)
    {
        integerResult(reg, node, DataFormatInteger, mode);
    }
    void noResult(Node* node, UseChildrenMode mode = CallUseChildren)
    {
        if (mode == UseChildrenCalledExplicitly)
            return;
        useChildren(node);
    }
    void cellResult(GPRReg reg, Node* node, UseChildrenMode mode = CallUseChildren)
    {
        if (mode == CallUseChildren)
            useChildren(node);

        VirtualRegister virtualRegister = node->virtualRegister();
        m_gprs.retain(reg, virtualRegister, SpillOrderCell);
        GenerationInfo& info = m_generationInfo[virtualRegister];
        info.initCell(node, node->refCount(), reg);
    }
    void booleanResult(GPRReg reg, Node* node, UseChildrenMode mode = CallUseChildren)
    {
        if (mode == CallUseChildren)
            useChildren(node);

        VirtualRegister virtualRegister = node->virtualRegister();
        m_gprs.retain(reg, virtualRegister, SpillOrderBoolean);
        GenerationInfo& info = m_generationInfo[virtualRegister];
        info.initBoolean(node, node->refCount(), reg);
    }
#if USE(JSVALUE64)
    void jsValueResult(GPRReg reg, Node* node, DataFormat format = DataFormatJS, UseChildrenMode mode = CallUseChildren)
    {
        if (format == DataFormatJSInteger)
            m_jit.jitAssertIsJSInt32(reg);
        
        if (mode == CallUseChildren)
            useChildren(node);

        VirtualRegister virtualRegister = node->virtualRegister();
        m_gprs.retain(reg, virtualRegister, SpillOrderJS);
        GenerationInfo& info = m_generationInfo[virtualRegister];
        info.initJSValue(node, node->refCount(), reg, format);
    }
    void jsValueResult(GPRReg reg, Node* node, UseChildrenMode mode)
    {
        jsValueResult(reg, node, DataFormatJS, mode);
    }
#elif USE(JSVALUE32_64)
    void jsValueResult(GPRReg tag, GPRReg payload, Node* node, DataFormat format = DataFormatJS, UseChildrenMode mode = CallUseChildren)
    {
        if (mode == CallUseChildren)
            useChildren(node);

        VirtualRegister virtualRegister = node->virtualRegister();
        m_gprs.retain(tag, virtualRegister, SpillOrderJS);
        m_gprs.retain(payload, virtualRegister, SpillOrderJS);
        GenerationInfo& info = m_generationInfo[virtualRegister];
        info.initJSValue(node, node->refCount(), tag, payload, format);
    }
    void jsValueResult(GPRReg tag, GPRReg payload, Node* node, UseChildrenMode mode)
    {
        jsValueResult(tag, payload, node, DataFormatJS, mode);
    }
#endif
    void storageResult(GPRReg reg, Node* node, UseChildrenMode mode = CallUseChildren)
    {
        if (mode == CallUseChildren)
            useChildren(node);
        
        VirtualRegister virtualRegister = node->virtualRegister();
        m_gprs.retain(reg, virtualRegister, SpillOrderStorage);
        GenerationInfo& info = m_generationInfo[virtualRegister];
        info.initStorage(node, node->refCount(), reg);
    }
    void doubleResult(FPRReg reg, Node* node, UseChildrenMode mode = CallUseChildren)
    {
        if (mode == CallUseChildren)
            useChildren(node);

        VirtualRegister virtualRegister = node->virtualRegister();
        m_fprs.retain(reg, virtualRegister, SpillOrderDouble);
        GenerationInfo& info = m_generationInfo[virtualRegister];
        info.initDouble(node, node->refCount(), reg);
    }
    void initConstantInfo(Node* node)
    {
        ASSERT(isInt32Constant(node) || isNumberConstant(node) || isJSConstant(node));
        m_generationInfo[node->virtualRegister()].initConstant(node, node->refCount());
    }
    
    // These methods add calls to C++ helper functions.
    // These methods are broadly value representation specific (i.e.
    // deal with the fact that a JSValue may be passed in one or two
    // machine registers, and delegate the calling convention specific
    // decision as to how to fill the regsiters to setupArguments* methods.

    JITCompiler::Call callOperation(P_DFGOperation_E operation, GPRReg result)
    {
        m_jit.setupArgumentsExecState();
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(P_DFGOperation_EC operation, GPRReg result, GPRReg cell)
    {
        m_jit.setupArgumentsWithExecState(cell);
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(P_DFGOperation_EO operation, GPRReg result, GPRReg object)
    {
        m_jit.setupArgumentsWithExecState(object);
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(P_DFGOperation_EOS operation, GPRReg result, GPRReg object, size_t size)
    {
        m_jit.setupArgumentsWithExecState(object, TrustedImmPtr(size));
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(P_DFGOperation_EOZ operation, GPRReg result, GPRReg object, int32_t size)
    {
        m_jit.setupArgumentsWithExecState(object, TrustedImmPtr(size));
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(C_DFGOperation_EOZ operation, GPRReg result, GPRReg object, int32_t size)
    {
        m_jit.setupArgumentsWithExecState(object, TrustedImmPtr(static_cast<size_t>(size)));
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(P_DFGOperation_EPS operation, GPRReg result, GPRReg old, size_t size)
    {
        m_jit.setupArgumentsWithExecState(old, TrustedImmPtr(size));
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(P_DFGOperation_ES operation, GPRReg result, size_t size)
    {
        m_jit.setupArgumentsWithExecState(TrustedImmPtr(size));
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(P_DFGOperation_ESt operation, GPRReg result, Structure* structure)
    {
        m_jit.setupArgumentsWithExecState(TrustedImmPtr(structure));
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(P_DFGOperation_EStZ operation, GPRReg result, Structure* structure, GPRReg arg2)
    {
        m_jit.setupArgumentsWithExecState(TrustedImmPtr(structure), arg2);
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(P_DFGOperation_EStZ operation, GPRReg result, Structure* structure, size_t arg2)
    {
        m_jit.setupArgumentsWithExecState(TrustedImmPtr(structure), TrustedImm32(arg2));
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(P_DFGOperation_EStZ operation, GPRReg result, GPRReg arg1, GPRReg arg2)
    {
        m_jit.setupArgumentsWithExecState(arg1, arg2);
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(P_DFGOperation_EStPS operation, GPRReg result, Structure* structure, void* pointer, size_t size)
    {
        m_jit.setupArgumentsWithExecState(TrustedImmPtr(structure), TrustedImmPtr(pointer), TrustedImmPtr(size));
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(P_DFGOperation_EStSS operation, GPRReg result, Structure* structure, size_t index, size_t size)
    {
        m_jit.setupArgumentsWithExecState(TrustedImmPtr(structure), TrustedImmPtr(index), TrustedImmPtr(size));
        return appendCallWithExceptionCheckSetResult(operation, result);
    }

    JITCompiler::Call callOperation(C_DFGOperation_E operation, GPRReg result)
    {
        m_jit.setupArgumentsExecState();
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(C_DFGOperation_EC operation, GPRReg result, GPRReg arg1)
    {
        m_jit.setupArgumentsWithExecState(arg1);
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(C_DFGOperation_EC operation, GPRReg result, JSCell* cell)
    {
        m_jit.setupArgumentsWithExecState(TrustedImmPtr(cell));
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(C_DFGOperation_ECC operation, GPRReg result, GPRReg arg1, JSCell* cell)
    {
        m_jit.setupArgumentsWithExecState(arg1, TrustedImmPtr(cell));
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(C_DFGOperation_EIcf operation, GPRReg result, InlineCallFrame* inlineCallFrame)
    {
        m_jit.setupArgumentsWithExecState(TrustedImmPtr(inlineCallFrame));
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(C_DFGOperation_ESt operation, GPRReg result, Structure* structure)
    {
        m_jit.setupArgumentsWithExecState(TrustedImmPtr(structure));
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(C_DFGOperation_EJssSt operation, GPRReg result, GPRReg arg1, Structure* structure)
    {
        m_jit.setupArgumentsWithExecState(arg1, TrustedImmPtr(structure));
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(C_DFGOperation_EJssJss operation, GPRReg result, GPRReg arg1, GPRReg arg2)
    {
        m_jit.setupArgumentsWithExecState(arg1, arg2);
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(C_DFGOperation_EJssJssJss operation, GPRReg result, GPRReg arg1, GPRReg arg2, GPRReg arg3)
    {
        m_jit.setupArgumentsWithExecState(arg1, arg2, arg3);
        return appendCallWithExceptionCheckSetResult(operation, result);
    }

    JITCompiler::Call callOperation(S_DFGOperation_ECC operation, GPRReg result, GPRReg arg1, GPRReg arg2)
    {
        m_jit.setupArgumentsWithExecState(arg1, arg2);
        return appendCallWithExceptionCheckSetResult(operation, result);
    }

    JITCompiler::Call callOperation(V_DFGOperation_EC operation, GPRReg arg1)
    {
        m_jit.setupArgumentsWithExecState(arg1);
        return appendCallWithExceptionCheck(operation);
    }

    JITCompiler::Call callOperation(V_DFGOperation_ECIcf operation, GPRReg arg1, InlineCallFrame* inlineCallFrame)
    {
        m_jit.setupArgumentsWithExecState(arg1, TrustedImmPtr(inlineCallFrame));
        return appendCallWithExceptionCheck(operation);
    }
    JITCompiler::Call callOperation(V_DFGOperation_ECCIcf operation, GPRReg arg1, GPRReg arg2, InlineCallFrame* inlineCallFrame)
    {
        m_jit.setupArgumentsWithExecState(arg1, arg2, TrustedImmPtr(inlineCallFrame));
        return appendCallWithExceptionCheck(operation);
    }

    JITCompiler::Call callOperation(V_DFGOperation_ECZ operation, GPRReg arg1, int arg2)
    {
        m_jit.setupArgumentsWithExecState(arg1, TrustedImm32(arg2));
        return appendCallWithExceptionCheck(operation);
    }

    JITCompiler::Call callOperation(V_DFGOperation_ECC operation, GPRReg arg1, GPRReg arg2)
    {
        m_jit.setupArgumentsWithExecState(arg1, arg2);
        return appendCallWithExceptionCheck(operation);
    }

    JITCompiler::Call callOperation(V_DFGOperation_EOZD operation, GPRReg arg1, GPRReg arg2, FPRReg arg3)
    {
        m_jit.setupArgumentsWithExecState(arg1, arg2, arg3);
        return appendCallWithExceptionCheck(operation);
    }

    JITCompiler::Call callOperation(V_DFGOperation_W operation, WatchpointSet* watchpointSet)
    {
        m_jit.setupArguments(TrustedImmPtr(watchpointSet));
        return appendCall(operation);
    }

    template<typename FunctionType, typename ArgumentType1>
    JITCompiler::Call callOperation(FunctionType operation, NoResultTag, ArgumentType1 arg1)
    {
        return callOperation(operation, arg1);
    }
    template<typename FunctionType, typename ArgumentType1, typename ArgumentType2>
    JITCompiler::Call callOperation(FunctionType operation, NoResultTag, ArgumentType1 arg1, ArgumentType2 arg2)
    {
        return callOperation(operation, arg1, arg2);
    }
    template<typename FunctionType, typename ArgumentType1, typename ArgumentType2, typename ArgumentType3>
    JITCompiler::Call callOperation(FunctionType operation, NoResultTag, ArgumentType1 arg1, ArgumentType2 arg2, ArgumentType3 arg3)
    {
        return callOperation(operation, arg1, arg2, arg3);
    }
    template<typename FunctionType, typename ArgumentType1, typename ArgumentType2, typename ArgumentType3, typename ArgumentType4>
    JITCompiler::Call callOperation(FunctionType operation, NoResultTag, ArgumentType1 arg1, ArgumentType2 arg2, ArgumentType3 arg3, ArgumentType4 arg4)
    {
        return callOperation(operation, arg1, arg2, arg3, arg4);
    }
    template<typename FunctionType, typename ArgumentType1, typename ArgumentType2, typename ArgumentType3, typename ArgumentType4, typename ArgumentType5>
    JITCompiler::Call callOperation(FunctionType operation, NoResultTag, ArgumentType1 arg1, ArgumentType2 arg2, ArgumentType3 arg3, ArgumentType4 arg4, ArgumentType5 arg5)
    {
        return callOperation(operation, arg1, arg2, arg3, arg4, arg5);
    }

    JITCompiler::Call callOperation(D_DFGOperation_ZZ operation, FPRReg result, GPRReg arg1, GPRReg arg2)
    {
        m_jit.setupArguments(arg1, arg2);
        return appendCallSetResult(operation, result);
    }
    JITCompiler::Call callOperation(D_DFGOperation_DD operation, FPRReg result, FPRReg arg1, FPRReg arg2)
    {
        m_jit.setupArguments(arg1, arg2);
        return appendCallSetResult(operation, result);
    }
    JITCompiler::Call callOperation(Str_DFGOperation_EJss operation, GPRReg result, GPRReg arg1)
    {
        m_jit.setupArgumentsWithExecState(arg1);
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(C_DFGOperation_EZ operation, GPRReg result, GPRReg arg1)
    {
        m_jit.setupArgumentsWithExecState(arg1);
        return appendCallWithExceptionCheckSetResult(operation, result);
    }

#if USE(JSVALUE64)
    JITCompiler::Call callOperation(J_DFGOperation_E operation, GPRReg result)
    {
        m_jit.setupArgumentsExecState();
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EP operation, GPRReg result, void* pointer)
    {
        m_jit.setupArgumentsWithExecState(TrustedImmPtr(pointer));
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(Z_DFGOperation_D operation, GPRReg result, FPRReg arg1)
    {
        m_jit.setupArguments(arg1);
        JITCompiler::Call call = m_jit.appendCall(operation);
        m_jit.zeroExtend32ToPtr(GPRInfo::returnValueGPR, result);
        return call;
    }
    JITCompiler::Call callOperation(J_DFGOperation_EGriJsgI operation, GPRReg result, GPRReg arg1, GPRReg arg2, Identifier* identifier)
    {
        m_jit.setupArgumentsWithExecState(arg1, arg2, TrustedImmPtr(identifier));
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EI operation, GPRReg result, Identifier* identifier)
    {
        m_jit.setupArgumentsWithExecState(TrustedImmPtr(identifier));
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EIRo operation, GPRReg result, Identifier* identifier, ResolveOperations* operations)
    {
        m_jit.setupArgumentsWithExecState(TrustedImmPtr(identifier), TrustedImmPtr(operations));
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EIRoPtbo operation, GPRReg result, Identifier* identifier, ResolveOperations* operations, PutToBaseOperation* putToBaseOperations)
    {
        m_jit.setupArgumentsWithExecState(TrustedImmPtr(identifier), TrustedImmPtr(operations), TrustedImmPtr(putToBaseOperations));
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EA operation, GPRReg result, GPRReg arg1)
    {
        m_jit.setupArgumentsWithExecState(arg1);
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EAZ operation, GPRReg result, GPRReg arg1, GPRReg arg2)
    {
        m_jit.setupArgumentsWithExecState(arg1, arg2);
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EPS operation, GPRReg result, void* pointer, size_t size)
    {
        m_jit.setupArgumentsWithExecState(TrustedImmPtr(pointer), TrustedImmPtr(size));
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(J_DFGOperation_ESS operation, GPRReg result, int startConstant, int numConstants)
    {
        m_jit.setupArgumentsWithExecState(TrustedImm32(startConstant), TrustedImm32(numConstants));
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EPP operation, GPRReg result, GPRReg arg1, void* pointer)
    {
        m_jit.setupArgumentsWithExecState(arg1, TrustedImmPtr(pointer));
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EC operation, GPRReg result, JSCell* cell)
    {
        m_jit.setupArgumentsWithExecState(TrustedImmPtr(cell));
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(J_DFGOperation_ECI operation, GPRReg result, GPRReg arg1, Identifier* identifier)
    {
        m_jit.setupArgumentsWithExecState(arg1, TrustedImmPtr(identifier));
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EJI operation, GPRReg result, GPRReg arg1, Identifier* identifier)
    {
        m_jit.setupArgumentsWithExecState(arg1, TrustedImmPtr(identifier));
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EDA operation, GPRReg result, FPRReg arg1, GPRReg arg2)
    {
        m_jit.setupArgumentsWithExecState(arg1, arg2);
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EJA operation, GPRReg result, GPRReg arg1, GPRReg arg2)
    {
        m_jit.setupArgumentsWithExecState(arg1, arg2);
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EP operation, GPRReg result, GPRReg arg1)
    {
        m_jit.setupArgumentsWithExecState(arg1);
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EZ operation, GPRReg result, GPRReg arg1)
    {
        m_jit.setupArgumentsWithExecState(arg1);
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EZ operation, GPRReg result, int32_t arg1)
    {
        m_jit.setupArgumentsWithExecState(TrustedImm32(arg1));
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EZZ operation, GPRReg result, int32_t arg1, GPRReg arg2)
    {
        m_jit.setupArgumentsWithExecState(TrustedImm32(arg1), arg2);
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EZIcfZ operation, GPRReg result, int32_t arg1, InlineCallFrame* inlineCallFrame, GPRReg arg2)
    {
        m_jit.setupArgumentsWithExecState(TrustedImm32(arg1), TrustedImmPtr(inlineCallFrame), arg2);
        return appendCallWithExceptionCheckSetResult(operation, result);
    }


    JITCompiler::Call callOperation(C_DFGOperation_EJ operation, GPRReg result, GPRReg arg1)
    {
        m_jit.setupArgumentsWithExecState(arg1);
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(S_DFGOperation_J operation, GPRReg result, GPRReg arg1)
    {
        m_jit.setupArguments(arg1);
        return appendCallSetResult(operation, result);
    }
    JITCompiler::Call callOperation(S_DFGOperation_EJ operation, GPRReg result, GPRReg arg1)
    {
        m_jit.setupArgumentsWithExecState(arg1);
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EJ operation, GPRReg result, GPRReg arg1)
    {
        m_jit.setupArgumentsWithExecState(arg1);
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(S_DFGOperation_EJJ operation, GPRReg result, GPRReg arg1, GPRReg arg2)
    {
        m_jit.setupArgumentsWithExecState(arg1, arg2);
        return appendCallWithExceptionCheckSetResult(operation, result);
    }

    JITCompiler::Call callOperation(J_DFGOperation_EPP operation, GPRReg result, GPRReg arg1, GPRReg arg2)
    {
        m_jit.setupArgumentsWithExecState(arg1, arg2);
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EJJ operation, GPRReg result, GPRReg arg1, GPRReg arg2)
    {
        m_jit.setupArgumentsWithExecState(arg1, arg2);
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EJJ operation, GPRReg result, GPRReg arg1, MacroAssembler::TrustedImm32 imm)
    {
        m_jit.setupArgumentsWithExecState(arg1, MacroAssembler::TrustedImm64(JSValue::encode(jsNumber(imm.m_value))));
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EJJ operation, GPRReg result, MacroAssembler::TrustedImm32 imm, GPRReg arg2)
    {
        m_jit.setupArgumentsWithExecState(MacroAssembler::TrustedImm64(JSValue::encode(jsNumber(imm.m_value))), arg2);
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(J_DFGOperation_ECC operation, GPRReg result, GPRReg arg1, GPRReg arg2)
    {
        m_jit.setupArgumentsWithExecState(arg1, arg2);
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(J_DFGOperation_ECJ operation, GPRReg result, GPRReg arg1, GPRReg arg2)
    {
        m_jit.setupArgumentsWithExecState(arg1, arg2);
        return appendCallWithExceptionCheckSetResult(operation, result);
    }

    JITCompiler::Call callOperation(V_DFGOperation_EJPP operation, GPRReg arg1, GPRReg arg2, void* pointer)
    {
        m_jit.setupArgumentsWithExecState(arg1, arg2, TrustedImmPtr(pointer));
        return appendCallWithExceptionCheck(operation);
    }
    JITCompiler::Call callOperation(V_DFGOperation_EJCI operation, GPRReg arg1, GPRReg arg2, Identifier* identifier)
    {
        m_jit.setupArgumentsWithExecState(arg1, arg2, TrustedImmPtr(identifier));
        return appendCallWithExceptionCheck(operation);
    }
    JITCompiler::Call callOperation(V_DFGOperation_EJJJ operation, GPRReg arg1, GPRReg arg2, GPRReg arg3)
    {
        m_jit.setupArgumentsWithExecState(arg1, arg2, arg3);
        return appendCallWithExceptionCheck(operation);
    }
    JITCompiler::Call callOperation(V_DFGOperation_EPZJ operation, GPRReg arg1, GPRReg arg2, GPRReg arg3)
    {
        m_jit.setupArgumentsWithExecState(arg1, arg2, arg3);
        return appendCallWithExceptionCheck(operation);
    }

    JITCompiler::Call callOperation(V_DFGOperation_EOZJ operation, GPRReg arg1, GPRReg arg2, GPRReg arg3)
    {
        m_jit.setupArgumentsWithExecState(arg1, arg2, arg3);
        return appendCallWithExceptionCheck(operation);
    }
    JITCompiler::Call callOperation(V_DFGOperation_ECJJ operation, GPRReg arg1, GPRReg arg2, GPRReg arg3)
    {
        m_jit.setupArgumentsWithExecState(arg1, arg2, arg3);
        return appendCallWithExceptionCheck(operation);
    }

    JITCompiler::Call callOperation(D_DFGOperation_EJ operation, FPRReg result, GPRReg arg1)
    {
        m_jit.setupArgumentsWithExecState(arg1);
        return appendCallWithExceptionCheckSetResult(operation, result);
    }

#else // USE(JSVALUE32_64)

// EncodedJSValue in JSVALUE32_64 is a 64-bit integer. When being compiled in ARM EABI, it must be aligned even-numbered register (r0, r2 or [sp]).
// To avoid assemblies from using wrong registers, let's occupy r1 or r3 with a dummy argument when necessary.
#if (COMPILER_SUPPORTS(EABI) && CPU(ARM)) || CPU(MIPS)
#define EABI_32BIT_DUMMY_ARG      TrustedImm32(0),
#else
#define EABI_32BIT_DUMMY_ARG
#endif

// JSVALUE32_64 is a 64-bit integer that cannot be put half in an argument register and half on stack when using SH4 architecture.
// To avoid this, let's occupy the 4th argument register (r7) with a dummy argument when necessary.
#if CPU(SH4)
#define SH4_32BIT_DUMMY_ARG      TrustedImm32(0),
#else
#define SH4_32BIT_DUMMY_ARG
#endif

    JITCompiler::Call callOperation(Z_DFGOperation_D operation, GPRReg result, FPRReg arg1)
    {
        prepareForExternalCall();
        m_jit.setupArguments(arg1);
        JITCompiler::Call call = m_jit.appendCall(operation);
        m_jit.zeroExtend32ToPtr(GPRInfo::returnValueGPR, result);
        return call;
    }
    JITCompiler::Call callOperation(J_DFGOperation_E operation, GPRReg resultTag, GPRReg resultPayload)
    {
        m_jit.setupArgumentsExecState();
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EP operation, GPRReg resultTag, GPRReg resultPayload, void* pointer)
    {
        m_jit.setupArgumentsWithExecState(TrustedImmPtr(pointer));
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EPP operation, GPRReg resultTag, GPRReg resultPayload, GPRReg arg1, void* pointer)
    {
        m_jit.setupArgumentsWithExecState(arg1, TrustedImmPtr(pointer));
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EGriJsgI operation, GPRReg resultTag, GPRReg resultPayload, GPRReg arg1, GPRReg arg2, Identifier* identifier)
    {
        m_jit.setupArgumentsWithExecState(arg1, arg2, TrustedImmPtr(identifier));
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EP operation, GPRReg resultTag, GPRReg resultPayload, GPRReg arg1)
    {
        m_jit.setupArgumentsWithExecState(arg1);
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EI operation, GPRReg resultTag, GPRReg resultPayload, Identifier* identifier)
    {
        m_jit.setupArgumentsWithExecState(TrustedImmPtr(identifier));
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EA operation, GPRReg resultTag, GPRReg resultPayload, GPRReg arg1)
    {
        m_jit.setupArgumentsWithExecState(arg1);
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EAZ operation, GPRReg resultTag, GPRReg resultPayload, GPRReg arg1, GPRReg arg2)
    {
        m_jit.setupArgumentsWithExecState(arg1, arg2);
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EPS operation, GPRReg resultTag, GPRReg resultPayload, void* pointer, size_t size)
    {
        m_jit.setupArgumentsWithExecState(TrustedImmPtr(pointer), TrustedImmPtr(size));
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }
    JITCompiler::Call callOperation(J_DFGOperation_ESS operation, GPRReg resultTag, GPRReg resultPayload, int startConstant, int numConstants)
    {
        m_jit.setupArgumentsWithExecState(TrustedImm32(startConstant), TrustedImm32(numConstants));
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EJP operation, GPRReg resultTag, GPRReg resultPayload, GPRReg arg1Tag, GPRReg arg1Payload, void* pointer)
    {
        m_jit.setupArgumentsWithExecState(EABI_32BIT_DUMMY_ARG arg1Payload, arg1Tag, TrustedImmPtr(pointer));
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EJP operation, GPRReg resultTag, GPRReg resultPayload, GPRReg arg1Tag, GPRReg arg1Payload, GPRReg arg2)
    {
        m_jit.setupArgumentsWithExecState(EABI_32BIT_DUMMY_ARG arg1Payload, arg1Tag, arg2);
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }

    JITCompiler::Call callOperation(J_DFGOperation_EC operation, GPRReg resultTag, GPRReg resultPayload, JSCell* cell)
    {
        m_jit.setupArgumentsWithExecState(TrustedImmPtr(cell));
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }
    JITCompiler::Call callOperation(J_DFGOperation_ECI operation, GPRReg resultTag, GPRReg resultPayload, GPRReg arg1, Identifier* identifier)
    {
        m_jit.setupArgumentsWithExecState(arg1, TrustedImmPtr(identifier));
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EJI operation, GPRReg resultTag, GPRReg resultPayload, GPRReg arg1Tag, GPRReg arg1Payload, Identifier* identifier)
    {
        m_jit.setupArgumentsWithExecState(EABI_32BIT_DUMMY_ARG arg1Payload, arg1Tag, TrustedImmPtr(identifier));
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EJI operation, GPRReg resultTag, GPRReg resultPayload, int32_t arg1Tag, GPRReg arg1Payload, Identifier* identifier)
    {
        m_jit.setupArgumentsWithExecState(EABI_32BIT_DUMMY_ARG arg1Payload, TrustedImm32(arg1Tag), TrustedImmPtr(identifier));
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EDA operation, GPRReg resultTag, GPRReg resultPayload, FPRReg arg1, GPRReg arg2)
    {
        m_jit.setupArgumentsWithExecState(arg1, arg2);
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EJA operation, GPRReg resultTag, GPRReg resultPayload, GPRReg arg1Tag, GPRReg arg1Payload, GPRReg arg2)
    {
        m_jit.setupArgumentsWithExecState(EABI_32BIT_DUMMY_ARG arg1Payload, arg1Tag, arg2);
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EJA operation, GPRReg resultTag, GPRReg resultPayload, TrustedImm32 arg1Tag, GPRReg arg1Payload, GPRReg arg2)
    {
        m_jit.setupArgumentsWithExecState(EABI_32BIT_DUMMY_ARG arg1Payload, arg1Tag, arg2);
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EJ operation, GPRReg resultTag, GPRReg resultPayload, GPRReg arg1Tag, GPRReg arg1Payload)
    {
        m_jit.setupArgumentsWithExecState(EABI_32BIT_DUMMY_ARG arg1Payload, arg1Tag);
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EZ operation, GPRReg resultTag, GPRReg resultPayload, GPRReg arg1)
    {
        m_jit.setupArgumentsWithExecState(arg1);
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EZ operation, GPRReg resultTag, GPRReg resultPayload, int32_t arg1)
    {
        m_jit.setupArgumentsWithExecState(TrustedImm32(arg1));
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EZIcfZ operation, GPRReg resultTag, GPRReg resultPayload, int32_t arg1, InlineCallFrame* inlineCallFrame, GPRReg arg2)
    {
        m_jit.setupArgumentsWithExecState(TrustedImm32(arg1), TrustedImmPtr(inlineCallFrame), arg2);
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EZZ operation, GPRReg resultTag, GPRReg resultPayload, int32_t arg1, GPRReg arg2)
    {
        m_jit.setupArgumentsWithExecState(TrustedImm32(arg1), arg2);
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }


    JITCompiler::Call callOperation(C_DFGOperation_EJ operation, GPRReg result, GPRReg arg1Tag, GPRReg arg1Payload)
    {
        m_jit.setupArgumentsWithExecState(EABI_32BIT_DUMMY_ARG arg1Payload, arg1Tag);
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(S_DFGOperation_J operation, GPRReg result, GPRReg arg1Tag, GPRReg arg1Payload)
    {
        m_jit.setupArguments(arg1Payload, arg1Tag);
        return appendCallSetResult(operation, result);
    }
    JITCompiler::Call callOperation(S_DFGOperation_EJ operation, GPRReg result, GPRReg arg1Tag, GPRReg arg1Payload)
    {
        m_jit.setupArgumentsWithExecState(EABI_32BIT_DUMMY_ARG arg1Payload, arg1Tag);
        return appendCallWithExceptionCheckSetResult(operation, result);
    }

    JITCompiler::Call callOperation(S_DFGOperation_EJJ operation, GPRReg result, GPRReg arg1Tag, GPRReg arg1Payload, GPRReg arg2Tag, GPRReg arg2Payload)
    {
        m_jit.setupArgumentsWithExecState(EABI_32BIT_DUMMY_ARG arg1Payload, arg1Tag, SH4_32BIT_DUMMY_ARG arg2Payload, arg2Tag);
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EJJ operation, GPRReg resultTag, GPRReg resultPayload, GPRReg arg1Tag, GPRReg arg1Payload, GPRReg arg2Tag, GPRReg arg2Payload)
    {
        m_jit.setupArgumentsWithExecState(EABI_32BIT_DUMMY_ARG arg1Payload, arg1Tag, SH4_32BIT_DUMMY_ARG arg2Payload, arg2Tag);
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EJJ operation, GPRReg resultTag, GPRReg resultPayload, GPRReg arg1Tag, GPRReg arg1Payload, MacroAssembler::TrustedImm32 imm)
    {
        m_jit.setupArgumentsWithExecState(EABI_32BIT_DUMMY_ARG arg1Payload, arg1Tag, SH4_32BIT_DUMMY_ARG imm, TrustedImm32(JSValue::Int32Tag));
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EJJ operation, GPRReg resultTag, GPRReg resultPayload, MacroAssembler::TrustedImm32 imm, GPRReg arg2Tag, GPRReg arg2Payload)
    {
        m_jit.setupArgumentsWithExecState(EABI_32BIT_DUMMY_ARG imm, TrustedImm32(JSValue::Int32Tag), SH4_32BIT_DUMMY_ARG arg2Payload, arg2Tag);
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }

    JITCompiler::Call callOperation(J_DFGOperation_EIRo operation, GPRReg resultTag, GPRReg resultPayload, Identifier* identifier, ResolveOperations* operations)
    {
        m_jit.setupArgumentsWithExecState(TrustedImmPtr(identifier), TrustedImmPtr(operations));
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }

    JITCompiler::Call callOperation(J_DFGOperation_EIRoPtbo operation, GPRReg resultTag, GPRReg resultPayload, Identifier* identifier, ResolveOperations* operations, PutToBaseOperation* putToBaseOperations)
    {
        m_jit.setupArgumentsWithExecState(TrustedImmPtr(identifier), TrustedImmPtr(operations), TrustedImmPtr(putToBaseOperations));
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }

    JITCompiler::Call callOperation(J_DFGOperation_ECJ operation, GPRReg resultTag, GPRReg resultPayload, GPRReg arg1, GPRReg arg2Tag, GPRReg arg2Payload)
    {
        m_jit.setupArgumentsWithExecState(arg1, arg2Payload, arg2Tag);
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }
    JITCompiler::Call callOperation(J_DFGOperation_ECC operation, GPRReg resultTag, GPRReg resultPayload, GPRReg arg1, GPRReg arg2)
    {
        m_jit.setupArgumentsWithExecState(arg1, arg2);
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }

    JITCompiler::Call callOperation(V_DFGOperation_EJPP operation, GPRReg arg1Tag, GPRReg arg1Payload, GPRReg arg2, void* pointer)
    {
        m_jit.setupArgumentsWithExecState(EABI_32BIT_DUMMY_ARG arg1Payload, arg1Tag, arg2, TrustedImmPtr(pointer));
        return appendCallWithExceptionCheck(operation);
    }
    JITCompiler::Call callOperation(V_DFGOperation_EJCI operation, GPRReg arg1Tag, GPRReg arg1Payload, GPRReg arg2, Identifier* identifier)
    {
        m_jit.setupArgumentsWithExecState(EABI_32BIT_DUMMY_ARG arg1Payload, arg1Tag, arg2, TrustedImmPtr(identifier));
        return appendCallWithExceptionCheck(operation);
    }
    JITCompiler::Call callOperation(V_DFGOperation_ECJJ operation, GPRReg arg1, GPRReg arg2Tag, GPRReg arg2Payload, GPRReg arg3Tag, GPRReg arg3Payload)
    {
        m_jit.setupArgumentsWithExecState(arg1, arg2Payload, arg2Tag, arg3Payload, arg3Tag);
        return appendCallWithExceptionCheck(operation);
    }

    JITCompiler::Call callOperation(V_DFGOperation_EPZJ operation, GPRReg arg1, GPRReg arg2, GPRReg arg3Tag, GPRReg arg3Payload)
    {
        m_jit.setupArgumentsWithExecState(arg1, arg2, EABI_32BIT_DUMMY_ARG SH4_32BIT_DUMMY_ARG arg3Payload, arg3Tag);
        return appendCallWithExceptionCheck(operation);
    }

    JITCompiler::Call callOperation(V_DFGOperation_EOZJ operation, GPRReg arg1, GPRReg arg2, GPRReg arg3Tag, GPRReg arg3Payload)
    {
        m_jit.setupArgumentsWithExecState(arg1, arg2, EABI_32BIT_DUMMY_ARG SH4_32BIT_DUMMY_ARG arg3Payload, arg3Tag);
        return appendCallWithExceptionCheck(operation);
    }
    JITCompiler::Call callOperation(V_DFGOperation_EOZJ operation, GPRReg arg1, GPRReg arg2, TrustedImm32 arg3Tag, GPRReg arg3Payload)
    {
        m_jit.setupArgumentsWithExecState(arg1, arg2, EABI_32BIT_DUMMY_ARG SH4_32BIT_DUMMY_ARG arg3Payload, arg3Tag);
        return appendCallWithExceptionCheck(operation);
    }

    JITCompiler::Call callOperation(D_DFGOperation_EJ operation, FPRReg result, GPRReg arg1Tag, GPRReg arg1Payload)
    {
        m_jit.setupArgumentsWithExecState(EABI_32BIT_DUMMY_ARG arg1Payload, arg1Tag);
        return appendCallWithExceptionCheckSetResult(operation, result);
    }

#undef EABI_32BIT_DUMMY_ARG
#undef SH4_32BIT_DUMMY_ARG
    
    template<typename FunctionType>
    JITCompiler::Call callOperation(
        FunctionType operation, JSValueRegs result)
    {
        return callOperation(operation, result.tagGPR(), result.payloadGPR());
    }
    template<typename FunctionType, typename ArgumentType1>
    JITCompiler::Call callOperation(
        FunctionType operation, JSValueRegs result, ArgumentType1 arg1)
    {
        return callOperation(operation, result.tagGPR(), result.payloadGPR(), arg1);
    }
    template<typename FunctionType, typename ArgumentType1, typename ArgumentType2>
    JITCompiler::Call callOperation(
        FunctionType operation, JSValueRegs result, ArgumentType1 arg1, ArgumentType2 arg2)
    {
        return callOperation(operation, result.tagGPR(), result.payloadGPR(), arg1, arg2);
    }
    template<
        typename FunctionType, typename ArgumentType1, typename ArgumentType2,
        typename ArgumentType3>
    JITCompiler::Call callOperation(
        FunctionType operation, JSValueRegs result, ArgumentType1 arg1, ArgumentType2 arg2,
        ArgumentType3 arg3)
    {
        return callOperation(operation, result.tagGPR(), result.payloadGPR(), arg1, arg2, arg3);
    }
    template<
        typename FunctionType, typename ArgumentType1, typename ArgumentType2,
        typename ArgumentType3, typename ArgumentType4>
    JITCompiler::Call callOperation(
        FunctionType operation, JSValueRegs result, ArgumentType1 arg1, ArgumentType2 arg2,
        ArgumentType3 arg3, ArgumentType4 arg4)
    {
        return callOperation(operation, result.tagGPR(), result.payloadGPR(), arg1, arg2, arg3, arg4);
    }
    template<
        typename FunctionType, typename ArgumentType1, typename ArgumentType2,
        typename ArgumentType3, typename ArgumentType4, typename ArgumentType5>
    JITCompiler::Call callOperation(
        FunctionType operation, JSValueRegs result, ArgumentType1 arg1, ArgumentType2 arg2,
        ArgumentType3 arg3, ArgumentType4 arg4, ArgumentType5 arg5)
    {
        return callOperation(
            operation, result.tagGPR(), result.payloadGPR(), arg1, arg2, arg3, arg4, arg5);
    }
#endif // USE(JSVALUE32_64)
    
#if !defined(NDEBUG) && !CPU(ARM) && !CPU(MIPS) && !CPU(SH4)
    void prepareForExternalCall()
    {
        // We're about to call out to a "native" helper function. The helper
        // function is expected to set topCallFrame itself with the ExecState
        // that is passed to it.
        //
        // We explicitly trash topCallFrame here so that we'll know if some of
        // the helper functions are not setting topCallFrame when they should
        // be doing so. Note: the previous value in topcallFrame was not valid
        // anyway since it was not being updated by JIT'ed code by design.

        for (unsigned i = 0; i < sizeof(void*) / 4; i++)
            m_jit.store32(TrustedImm32(0xbadbeef), reinterpret_cast<char*>(&m_jit.vm()->topCallFrame) + i * 4);
    }
#else
    void prepareForExternalCall() { }
#endif

    // These methods add call instructions, with optional exception checks & setting results.
    JITCompiler::Call appendCallWithExceptionCheck(const FunctionPtr& function)
    {
        prepareForExternalCall();
        CodeOrigin codeOrigin = m_currentNode->codeOrigin;
        CallBeginToken token;
        m_jit.beginCall(codeOrigin, token);
        JITCompiler::Call call = m_jit.appendCall(function);
        m_jit.addExceptionCheck(call, codeOrigin, token);
        return call;
    }
    JITCompiler::Call appendCallWithExceptionCheckSetResult(const FunctionPtr& function, GPRReg result)
    {
        JITCompiler::Call call = appendCallWithExceptionCheck(function);
        m_jit.move(GPRInfo::returnValueGPR, result);
        return call;
    }
    JITCompiler::Call appendCallSetResult(const FunctionPtr& function, GPRReg result)
    {
        prepareForExternalCall();
        JITCompiler::Call call = m_jit.appendCall(function);
        m_jit.move(GPRInfo::returnValueGPR, result);
        return call;
    }
    JITCompiler::Call appendCall(const FunctionPtr& function)
    {
        prepareForExternalCall();
        return m_jit.appendCall(function);
    }
    JITCompiler::Call appendCallWithExceptionCheckSetResult(const FunctionPtr& function, GPRReg result1, GPRReg result2)
    {
        JITCompiler::Call call = appendCallWithExceptionCheck(function);
        m_jit.setupResults(result1, result2);
        return call;
    }
#if CPU(X86)
    JITCompiler::Call appendCallWithExceptionCheckSetResult(const FunctionPtr& function, FPRReg result)
    {
        JITCompiler::Call call = appendCallWithExceptionCheck(function);
        m_jit.assembler().fstpl(0, JITCompiler::stackPointerRegister);
        m_jit.loadDouble(JITCompiler::stackPointerRegister, result);
        return call;
    }
    JITCompiler::Call appendCallSetResult(const FunctionPtr& function, FPRReg result)
    {
        JITCompiler::Call call = m_jit.appendCall(function);
        m_jit.assembler().fstpl(0, JITCompiler::stackPointerRegister);
        m_jit.loadDouble(JITCompiler::stackPointerRegister, result);
        return call;
    }
#elif CPU(ARM)
#if CPU(ARM_HARDFP)
    JITCompiler::Call appendCallWithExceptionCheckSetResult(const FunctionPtr& function, FPRReg result)
    {
        JITCompiler::Call call = appendCallWithExceptionCheck(function);
        if (result != InvalidFPRReg)
            m_jit.moveDouble(FPRInfo::argumentFPR0, result);
        return call;
    }
    JITCompiler::Call appendCallSetResult(const FunctionPtr& function, FPRReg result)
    {
        JITCompiler::Call call = m_jit.appendCall(function);
        if (result != InvalidFPRReg)
            m_jit.moveDouble(FPRInfo::argumentFPR0, result);
        return call;
    }
#else
        JITCompiler::Call appendCallWithExceptionCheckSetResult(const FunctionPtr& function, FPRReg result)
    {
        JITCompiler::Call call = appendCallWithExceptionCheck(function);
        m_jit.assembler().vmov(result, GPRInfo::returnValueGPR, GPRInfo::returnValueGPR2);
        return call;
    }
    JITCompiler::Call appendCallSetResult(const FunctionPtr& function, FPRReg result)
    {
        JITCompiler::Call call = m_jit.appendCall(function);
        m_jit.assembler().vmov(result, GPRInfo::returnValueGPR, GPRInfo::returnValueGPR2);
        return call;
    }
#endif // CPU(ARM_HARDFP)
#else
    JITCompiler::Call appendCallWithExceptionCheckSetResult(const FunctionPtr& function, FPRReg result)
    {
        JITCompiler::Call call = appendCallWithExceptionCheck(function);
        m_jit.moveDouble(FPRInfo::returnValueFPR, result);
        return call;
    }
    JITCompiler::Call appendCallSetResult(const FunctionPtr& function, FPRReg result)
    {
        JITCompiler::Call call = m_jit.appendCall(function);
        m_jit.moveDouble(FPRInfo::returnValueFPR, result);
        return call;
    }
#endif
    
    void branchDouble(JITCompiler::DoubleCondition cond, FPRReg left, FPRReg right, BlockIndex destination)
    {
        if (!haveEdgeCodeToEmit(destination))
            return addBranch(m_jit.branchDouble(cond, left, right), destination);
        
        JITCompiler::Jump notTaken = m_jit.branchDouble(JITCompiler::invert(cond), left, right);
        emitEdgeCode(destination);
        addBranch(m_jit.jump(), destination);
        notTaken.link(&m_jit);
    }
    
    void branchDoubleNonZero(FPRReg value, FPRReg scratch, BlockIndex destination)
    {
        if (!haveEdgeCodeToEmit(destination))
            return addBranch(m_jit.branchDoubleNonZero(value, scratch), destination);
        
        JITCompiler::Jump notTaken = m_jit.branchDoubleZeroOrNaN(value, scratch);
        emitEdgeCode(destination);
        addBranch(m_jit.jump(), destination);
        notTaken.link(&m_jit);
    }
    
    template<typename T, typename U>
    void branch32(JITCompiler::RelationalCondition cond, T left, U right, BlockIndex destination)
    {
        if (!haveEdgeCodeToEmit(destination))
            return addBranch(m_jit.branch32(cond, left, right), destination);
        
        JITCompiler::Jump notTaken = m_jit.branch32(JITCompiler::invert(cond), left, right);
        emitEdgeCode(destination);
        addBranch(m_jit.jump(), destination);
        notTaken.link(&m_jit);
    }
    
    template<typename T, typename U>
    void branchTest32(JITCompiler::ResultCondition cond, T value, U mask, BlockIndex destination)
    {
        ASSERT(JITCompiler::isInvertible(cond));
        
        if (!haveEdgeCodeToEmit(destination))
            return addBranch(m_jit.branchTest32(cond, value, mask), destination);
        
        JITCompiler::Jump notTaken = m_jit.branchTest32(JITCompiler::invert(cond), value, mask);
        emitEdgeCode(destination);
        addBranch(m_jit.jump(), destination);
        notTaken.link(&m_jit);
    }
    
    template<typename T>
    void branchTest32(JITCompiler::ResultCondition cond, T value, BlockIndex destination)
    {
        ASSERT(JITCompiler::isInvertible(cond));
        
        if (!haveEdgeCodeToEmit(destination))
            return addBranch(m_jit.branchTest32(cond, value), destination);
        
        JITCompiler::Jump notTaken = m_jit.branchTest32(JITCompiler::invert(cond), value);
        emitEdgeCode(destination);
        addBranch(m_jit.jump(), destination);
        notTaken.link(&m_jit);
    }
    
#if USE(JSVALUE64)
    template<typename T, typename U>
    void branch64(JITCompiler::RelationalCondition cond, T left, U right, BlockIndex destination)
    {
        if (!haveEdgeCodeToEmit(destination))
            return addBranch(m_jit.branch64(cond, left, right), destination);
        
        JITCompiler::Jump notTaken = m_jit.branch64(JITCompiler::invert(cond), left, right);
        emitEdgeCode(destination);
        addBranch(m_jit.jump(), destination);
        notTaken.link(&m_jit);
    }
#endif
    
    template<typename T, typename U>
    void branchPtr(JITCompiler::RelationalCondition cond, T left, U right, BlockIndex destination)
    {
        if (!haveEdgeCodeToEmit(destination))
            return addBranch(m_jit.branchPtr(cond, left, right), destination);
        
        JITCompiler::Jump notTaken = m_jit.branchPtr(JITCompiler::invert(cond), left, right);
        emitEdgeCode(destination);
        addBranch(m_jit.jump(), destination);
        notTaken.link(&m_jit);
    }
    
    template<typename T, typename U>
    void branchTestPtr(JITCompiler::ResultCondition cond, T value, U mask, BlockIndex destination)
    {
        ASSERT(JITCompiler::isInvertible(cond));
        
        if (!haveEdgeCodeToEmit(destination))
            return addBranch(m_jit.branchTestPtr(cond, value, mask), destination);
        
        JITCompiler::Jump notTaken = m_jit.branchTestPtr(JITCompiler::invert(cond), value, mask);
        emitEdgeCode(destination);
        addBranch(m_jit.jump(), destination);
        notTaken.link(&m_jit);
    }
    
    template<typename T>
    void branchTestPtr(JITCompiler::ResultCondition cond, T value, BlockIndex destination)
    {
        ASSERT(JITCompiler::isInvertible(cond));
        
        if (!haveEdgeCodeToEmit(destination))
            return addBranch(m_jit.branchTestPtr(cond, value), destination);
        
        JITCompiler::Jump notTaken = m_jit.branchTestPtr(JITCompiler::invert(cond), value);
        emitEdgeCode(destination);
        addBranch(m_jit.jump(), destination);
        notTaken.link(&m_jit);
    }
    
    template<typename T, typename U>
    void branchTest8(JITCompiler::ResultCondition cond, T value, U mask, BlockIndex destination)
    {
        ASSERT(JITCompiler::isInvertible(cond));
        
        if (!haveEdgeCodeToEmit(destination))
            return addBranch(m_jit.branchTest8(cond, value, mask), destination);
        
        JITCompiler::Jump notTaken = m_jit.branchTest8(JITCompiler::invert(cond), value, mask);
        emitEdgeCode(destination);
        addBranch(m_jit.jump(), destination);
        notTaken.link(&m_jit);
    }
    
    template<typename T>
    void branchTest8(JITCompiler::ResultCondition cond, T value, BlockIndex destination)
    {
        ASSERT(JITCompiler::isInvertible(cond));
        
        if (!haveEdgeCodeToEmit(destination))
            return addBranch(m_jit.branchTest8(cond, value), destination);
        
        JITCompiler::Jump notTaken = m_jit.branchTest8(JITCompiler::invert(cond), value);
        emitEdgeCode(destination);
        addBranch(m_jit.jump(), destination);
        notTaken.link(&m_jit);
    }
    
    enum FallThroughMode {
        AtFallThroughPoint,
        ForceJump
    };
    void jump(BlockIndex destination, FallThroughMode fallThroughMode = AtFallThroughPoint)
    {
        if (haveEdgeCodeToEmit(destination))
            emitEdgeCode(destination);
        if (destination == nextBlock()
            && fallThroughMode == AtFallThroughPoint)
            return;
        addBranch(m_jit.jump(), destination);
    }
    
    inline bool haveEdgeCodeToEmit(BlockIndex)
    {
        return DFG_ENABLE_EDGE_CODE_VERIFICATION;
    }
    void emitEdgeCode(BlockIndex destination)
    {
        if (!DFG_ENABLE_EDGE_CODE_VERIFICATION)
            return;
        m_jit.move(TrustedImm32(destination), GPRInfo::regT0);
    }

    void addBranch(const MacroAssembler::Jump& jump, BlockIndex destination)
    {
        m_branches.append(BranchRecord(jump, destination));
    }

    void linkBranches()
    {
        for (size_t i = 0; i < m_branches.size(); ++i) {
            BranchRecord& branch = m_branches[i];
            branch.jump.linkTo(m_blockHeads[branch.destination], &m_jit);
        }
    }

    BasicBlock* block()
    {
        return m_jit.graph().m_blocks[m_block].get();
    }

#ifndef NDEBUG
    void dump(const char* label = 0);
#endif

#if DFG_ENABLE(CONSISTENCY_CHECK)
    void checkConsistency();
#else
    void checkConsistency() { }
#endif

    bool isInteger(Node* node)
    {
        if (node->hasInt32Result())
            return true;

        if (isInt32Constant(node))
            return true;

        VirtualRegister virtualRegister = node->virtualRegister();
        GenerationInfo& info = m_generationInfo[virtualRegister];
        
        return info.isJSInteger();
    }
    
    bool compare(Node*, MacroAssembler::RelationalCondition, MacroAssembler::DoubleCondition, S_DFGOperation_EJJ);
    bool compilePeepHoleBranch(Node*, MacroAssembler::RelationalCondition, MacroAssembler::DoubleCondition, S_DFGOperation_EJJ);
    void compilePeepHoleIntegerBranch(Node*, Node* branchNode, JITCompiler::RelationalCondition);
    void compilePeepHoleBooleanBranch(Node*, Node* branchNode, JITCompiler::RelationalCondition);
    void compilePeepHoleDoubleBranch(Node*, Node* branchNode, JITCompiler::DoubleCondition);
    void compilePeepHoleObjectEquality(Node*, Node* branchNode);
    void compilePeepHoleObjectToObjectOrOtherEquality(Edge leftChild, Edge rightChild, Node* branchNode);
    void compileObjectEquality(Node*);
    void compileObjectToObjectOrOtherEquality(Edge leftChild, Edge rightChild);
    void compileValueAdd(Node*);
    void compileObjectOrOtherLogicalNot(Edge value);
    void compileLogicalNot(Node*);
    void compileStringEquality(Node*);
    void emitObjectOrOtherBranch(Edge value, BlockIndex taken, BlockIndex notTaken);
    void emitBranch(Node*);
    
    void compileToStringOnCell(Node*);
    void compileNewStringObject(Node*);
    
    void compileIntegerCompare(Node*, MacroAssembler::RelationalCondition);
    void compileBooleanCompare(Node*, MacroAssembler::RelationalCondition);
    void compileDoubleCompare(Node*, MacroAssembler::DoubleCondition);
    
    bool compileStrictEqForConstant(Node*, Edge value, JSValue constant);
    
    bool compileStrictEq(Node*);
    
    void compileAllocatePropertyStorage(Node*);
    void compileReallocatePropertyStorage(Node*);
    
#if USE(JSVALUE32_64)
    template<typename BaseOperandType, typename PropertyOperandType, typename ValueOperandType, typename TagType>
    void compileContiguousPutByVal(Node*, BaseOperandType&, PropertyOperandType&, ValueOperandType&, GPRReg valuePayloadReg, TagType valueTag);
#endif
    void compileDoublePutByVal(Node*, SpeculateCellOperand& base, SpeculateStrictInt32Operand& property);
    bool putByValWillNeedExtraRegister(ArrayMode arrayMode)
    {
        return arrayMode.mayStoreToHole();
    }
    GPRReg temporaryRegisterForPutByVal(GPRTemporary&, ArrayMode);
    GPRReg temporaryRegisterForPutByVal(GPRTemporary& temporary, Node* node)
    {
        return temporaryRegisterForPutByVal(temporary, node->arrayMode());
    }
    
    void compileGetCharCodeAt(Node*);
    void compileGetByValOnString(Node*);
    void compileFromCharCode(Node*); 

    void compileGetByValOnArguments(Node*);
    void compileGetArgumentsLength(Node*);
    
    void compileGetArrayLength(Node*);
    
    void compileValueToInt32(Node*);
    void compileUInt32ToNumber(Node*);
    void compileDoubleAsInt32(Node*);
    void compileInt32ToDouble(Node*);
    void compileAdd(Node*);
    void compileMakeRope(Node*);
    void compileArithSub(Node*);
    void compileArithNegate(Node*);
    void compileArithMul(Node*);
    void compileArithIMul(Node*);
#if CPU(X86) || CPU(X86_64)
    void compileIntegerArithDivForX86(Node*);
#elif CPU(APPLE_ARMV7S)
    void compileIntegerArithDivForARMv7s(Node*);
#endif
    void compileArithMod(Node*);
    void compileSoftModulo(Node*);
    void compileGetIndexedPropertyStorage(Node*);
    void compileGetByValOnIntTypedArray(const TypedArrayDescriptor&, Node*, size_t elementSize, TypedArraySignedness);
    void compilePutByValForIntTypedArray(const TypedArrayDescriptor&, GPRReg base, GPRReg property, Node*, size_t elementSize, TypedArraySignedness, TypedArrayRounding = TruncateRounding);
    void compileGetByValOnFloatTypedArray(const TypedArrayDescriptor&, Node*, size_t elementSize);
    void compilePutByValForFloatTypedArray(const TypedArrayDescriptor&, GPRReg base, GPRReg property, Node*, size_t elementSize);
    void compileNewFunctionNoCheck(Node*);
    void compileNewFunctionExpression(Node*);
    bool compileRegExpExec(Node*);
    
    // size can be an immediate or a register, and must be in bytes. If size is a register,
    // it must be a different register than resultGPR. Emits code that place a pointer to
    // the end of the allocation. The returned jump is the jump to the slow path.
    template<typename SizeType>
    MacroAssembler::Jump emitAllocateBasicStorage(SizeType size, GPRReg resultGPR)
    {
        CopiedAllocator* copiedAllocator = &m_jit.vm()->heap.storageAllocator();
        
        m_jit.loadPtr(&copiedAllocator->m_currentRemaining, resultGPR);
        MacroAssembler::Jump slowPath = m_jit.branchSubPtr(JITCompiler::Signed, size, resultGPR);
        m_jit.storePtr(resultGPR, &copiedAllocator->m_currentRemaining);
        m_jit.negPtr(resultGPR);
        m_jit.addPtr(JITCompiler::AbsoluteAddress(&copiedAllocator->m_currentPayloadEnd), resultGPR);
        
        return slowPath;
    }
    
    // Allocator for a cell of a specific size.
    template <typename StructureType> // StructureType can be GPR or ImmPtr.
    void emitAllocateJSCell(GPRReg resultGPR, GPRReg allocatorGPR, StructureType structure,
        GPRReg scratchGPR, MacroAssembler::JumpList& slowPath)
    {
        m_jit.loadPtr(MacroAssembler::Address(allocatorGPR, MarkedAllocator::offsetOfFreeListHead()), resultGPR);
        slowPath.append(m_jit.branchTestPtr(MacroAssembler::Zero, resultGPR));
        
        // The object is half-allocated: we have what we know is a fresh object, but
        // it's still on the GC's free list.
        m_jit.loadPtr(MacroAssembler::Address(resultGPR), scratchGPR);
        m_jit.storePtr(scratchGPR, MacroAssembler::Address(allocatorGPR, MarkedAllocator::offsetOfFreeListHead()));

        // Initialize the object's Structure.
        m_jit.storePtr(structure, MacroAssembler::Address(resultGPR, JSCell::structureOffset()));
    }

    // Allocator for an object of a specific size.
    template <typename StructureType, typename StorageType> // StructureType and StorageType can be GPR or ImmPtr.
    void emitAllocateJSObject(GPRReg resultGPR, GPRReg allocatorGPR, StructureType structure,
        StorageType storage, GPRReg scratchGPR, MacroAssembler::JumpList& slowPath)
    {
        emitAllocateJSCell(resultGPR, allocatorGPR, structure, scratchGPR, slowPath);
        
        // Initialize the object's property storage pointer.
        m_jit.storePtr(storage, MacroAssembler::Address(resultGPR, JSObject::butterflyOffset()));
    }

    // Convenience allocator for a buit-in object.
    template <typename ClassType, typename StructureType, typename StorageType> // StructureType and StorageType can be GPR or ImmPtr.
    void emitAllocateJSObject(GPRReg resultGPR, StructureType structure, StorageType storage,
        GPRReg scratchGPR1, GPRReg scratchGPR2, MacroAssembler::JumpList& slowPath)
    {
        MarkedAllocator* allocator = 0;
        size_t size = ClassType::allocationSize(0);
        if (ClassType::needsDestruction && ClassType::hasImmortalStructure)
            allocator = &m_jit.vm()->heap.allocatorForObjectWithImmortalStructureDestructor(size);
        else if (ClassType::needsDestruction)
            allocator = &m_jit.vm()->heap.allocatorForObjectWithNormalDestructor(size);
        else
            allocator = &m_jit.vm()->heap.allocatorForObjectWithoutDestructor(size);
        m_jit.move(TrustedImmPtr(allocator), scratchGPR1);
        emitAllocateJSObject(resultGPR, scratchGPR1, structure, storage, scratchGPR2, slowPath);
    }

    void emitAllocateJSArray(GPRReg resultGPR, Structure*, GPRReg storageGPR, unsigned numElements);

#if USE(JSVALUE64) 
    JITCompiler::Jump convertToDouble(GPRReg value, FPRReg result, GPRReg tmp);
#elif USE(JSVALUE32_64)
    JITCompiler::Jump convertToDouble(JSValueOperand&, FPRReg result);
#endif
    
    // Add a backward speculation check.
    void backwardSpeculationCheck(ExitKind, JSValueSource, Node*, MacroAssembler::Jump jumpToFail);
    void backwardSpeculationCheck(ExitKind, JSValueSource, Node*, const MacroAssembler::JumpList& jumpsToFail);

    // Add a speculation check without additional recovery.
    void speculationCheck(ExitKind, JSValueSource, Node*, MacroAssembler::Jump jumpToFail);
    void speculationCheck(ExitKind, JSValueSource, Edge, MacroAssembler::Jump jumpToFail);
    // Add a speculation check without additional recovery, and with a promise to supply a jump later.
    OSRExitJumpPlaceholder backwardSpeculationCheck(ExitKind, JSValueSource, Node*);
    OSRExitJumpPlaceholder backwardSpeculationCheck(ExitKind, JSValueSource, Edge);
    // Add a set of speculation checks without additional recovery.
    void speculationCheck(ExitKind, JSValueSource, Node*, const MacroAssembler::JumpList& jumpsToFail);
    void speculationCheck(ExitKind, JSValueSource, Edge, const MacroAssembler::JumpList& jumpsToFail);
    // Add a speculation check with additional recovery.
    void backwardSpeculationCheck(ExitKind, JSValueSource, Node*, MacroAssembler::Jump jumpToFail, const SpeculationRecovery&);
    void backwardSpeculationCheck(ExitKind, JSValueSource, Edge, MacroAssembler::Jump jumpToFail, const SpeculationRecovery&);
    // Use this like you would use speculationCheck(), except that you don't pass it a jump
    // (because you don't have to execute a branch; that's kind of the whole point), and you
    // must register the returned Watchpoint with something relevant. In general, this should
    // be used with extreme care. Use speculationCheck() unless you've got an amazing reason
    // not to.
    JumpReplacementWatchpoint* speculationWatchpoint(ExitKind, JSValueSource, Node*);
    // The default for speculation watchpoints is that they're uncounted, because the
    // act of firing a watchpoint invalidates it. So, future recompilations will not
    // attempt to set this watchpoint again.
    JumpReplacementWatchpoint* speculationWatchpoint(ExitKind = UncountableWatchpoint);
    
    // It is generally a good idea to not use this directly.
    void convertLastOSRExitToForward(const ValueRecovery& = ValueRecovery());
    
    // Note: not specifying the valueRecovery argument (leaving it as ValueRecovery()) implies
    // that you've ensured that there exists a MovHint prior to your use of forwardSpeculationCheck().
    void forwardSpeculationCheck(ExitKind, JSValueSource, Node*, MacroAssembler::Jump jumpToFail, const ValueRecovery& = ValueRecovery());
    void forwardSpeculationCheck(ExitKind, JSValueSource, Node*, const MacroAssembler::JumpList& jumpsToFail, const ValueRecovery& = ValueRecovery());
    void speculationCheck(ExitKind, JSValueSource, Node*, MacroAssembler::Jump jumpToFail, const SpeculationRecovery&);
    void speculationCheck(ExitKind, JSValueSource, Edge, MacroAssembler::Jump jumpToFail, const SpeculationRecovery&);
    // Called when we statically determine that a speculation will fail.
    void terminateSpeculativeExecution(ExitKind, JSValueRegs, Node*);
    void terminateSpeculativeExecution(ExitKind, JSValueRegs, Edge);
    
    // Helpers for performing type checks on an edge stored in the given registers.
    bool needsTypeCheck(Edge edge, SpeculatedType typesPassedThrough) { return m_state.forNode(edge).m_type & ~typesPassedThrough; }
    void backwardTypeCheck(JSValueSource, Edge, SpeculatedType typesPassedThrough, MacroAssembler::Jump jumpToFail);
    void typeCheck(JSValueSource, Edge, SpeculatedType typesPassedThrough, MacroAssembler::Jump jumpToFail);
    void forwardTypeCheck(JSValueSource, Edge, SpeculatedType typesPassedThrough, MacroAssembler::Jump jumpToFail, const ValueRecovery&);

    void speculateInt32(Edge);
    void speculateNumber(Edge);
    void speculateRealNumber(Edge);
    void speculateBoolean(Edge);
    void speculateCell(Edge);
    void speculateObject(Edge);
    void speculateObjectOrOther(Edge);
    void speculateString(Edge);
    template<typename StructureLocationType>
    void speculateStringObjectForStructure(Edge, StructureLocationType);
    void speculateStringObject(Edge, GPRReg);
    void speculateStringObject(Edge);
    void speculateStringOrStringObject(Edge);
    void speculateNotCell(Edge);
    void speculateOther(Edge);
    void speculate(Node*, Edge);
    
    const TypedArrayDescriptor* typedArrayDescriptor(ArrayMode);
    
    JITCompiler::Jump jumpSlowForUnwantedArrayMode(GPRReg tempWithIndexingTypeReg, ArrayMode, IndexingType);
    JITCompiler::JumpList jumpSlowForUnwantedArrayMode(GPRReg tempWithIndexingTypeReg, ArrayMode);
    void checkArray(Node*);
    void arrayify(Node*, GPRReg baseReg, GPRReg propertyReg);
    void arrayify(Node*);
    
    template<bool strict>
    GPRReg fillSpeculateIntInternal(Edge, DataFormat& returnFormat);
    
    // It is possible, during speculative generation, to reach a situation in which we
    // can statically determine a speculation will fail (for example, when two nodes
    // will make conflicting speculations about the same operand). In such cases this
    // flag is cleared, indicating no further code generation should take place.
    bool m_compileOkay;
    
    // Tracking for which nodes are currently holding the values of arguments and bytecode
    // operand-indexed variables.
    
    ValueSource valueSourceForOperand(int operand)
    {
        return valueSourceReferenceForOperand(operand);
    }
    
    void setNodeForOperand(Node* node, int operand)
    {
        valueSourceReferenceForOperand(operand) = ValueSource(MinifiedID(node));
    }
    
    // Call this with care, since it both returns a reference into an array
    // and potentially resizes the array. So it would not be right to call this
    // twice and then perform operands on both references, since the one from
    // the first call may no longer be valid.
    ValueSource& valueSourceReferenceForOperand(int operand)
    {
        if (operandIsArgument(operand)) {
            int argument = operandToArgument(operand);
            return m_arguments[argument];
        }
        
        if ((unsigned)operand >= m_variables.size())
            m_variables.resize(operand + 1);
        
        return m_variables[operand];
    }
    
    void recordSetLocal(int operand, ValueSource valueSource)
    {
        valueSourceReferenceForOperand(operand) = valueSource;
        m_stream->appendAndLog(VariableEvent::setLocal(operand, valueSource.dataFormat()));
    }
    
    // The JIT, while also provides MacroAssembler functionality.
    JITCompiler& m_jit;

    // The current node being generated.
    BlockIndex m_block;
    Node* m_currentNode;
    SpeculationDirection m_speculationDirection;
#if !ASSERT_DISABLED
    bool m_canExit;
#endif
    unsigned m_indexInBlock;
    // Virtual and physical register maps.
    Vector<GenerationInfo, 32> m_generationInfo;
    RegisterBank<GPRInfo> m_gprs;
    RegisterBank<FPRInfo> m_fprs;

    Vector<MacroAssembler::Label> m_blockHeads;
    Vector<MacroAssembler::Label> m_osrEntryHeads;
    
    struct BranchRecord {
        BranchRecord(MacroAssembler::Jump jump, BlockIndex destination)
            : jump(jump)
            , destination(destination)
        {
        }

        MacroAssembler::Jump jump;
        BlockIndex destination;
    };
    Vector<BranchRecord, 8> m_branches;

    Vector<ValueSource, 0> m_arguments;
    Vector<ValueSource, 0> m_variables;
    int m_lastSetOperand;
    CodeOrigin m_codeOriginForOSR;
    
    AbstractState m_state;
    
    VariableEventStream* m_stream;
    MinifiedGraph* m_minifiedGraph;
    
    bool m_isCheckingArgumentTypes;
    
    Vector<OwnPtr<SlowPathGenerator>, 8> m_slowPathGenerators;
    Vector<SilentRegisterSavePlan> m_plans;
    
    ValueRecovery computeValueRecoveryFor(const ValueSource&);

    ValueRecovery computeValueRecoveryFor(int operand)
    {
        return computeValueRecoveryFor(valueSourceForOperand(operand));
    }
};


// === Operand types ===
//
// IntegerOperand and JSValueOperand.
//
// These classes are used to lock the operands to a node into machine
// registers. These classes implement of pattern of locking a value
// into register at the point of construction only if it is already in
// registers, and otherwise loading it lazily at the point it is first
// used. We do so in order to attempt to avoid spilling one operand
// in order to make space available for another.

class IntegerOperand {
public:
    explicit IntegerOperand(SpeculativeJIT* jit, Edge edge, OperandSpeculationMode mode = AutomaticOperandSpeculation)
        : m_jit(jit)
        , m_edge(edge)
        , m_gprOrInvalid(InvalidGPRReg)
#ifndef NDEBUG
        , m_format(DataFormatNone)
#endif
    {
        ASSERT(m_jit);
        ASSERT_UNUSED(mode, mode == ManualOperandSpeculation || edge.useKind() == KnownInt32Use);
        if (jit->isFilled(edge.node()))
            gpr();
    }

    ~IntegerOperand()
    {
        ASSERT(m_gprOrInvalid != InvalidGPRReg);
        m_jit->unlock(m_gprOrInvalid);
    }

    Edge edge() const
    {
        return m_edge;
    }
    
    Node* node() const
    {
        return edge().node();
    }

    DataFormat format()
    {
        gpr(); // m_format is set when m_gpr is locked.
        ASSERT(m_format == DataFormatInteger || m_format == DataFormatJSInteger);
        return m_format;
    }

    GPRReg gpr()
    {
        if (m_gprOrInvalid == InvalidGPRReg)
            m_gprOrInvalid = m_jit->fillInteger(m_edge, m_format);
        return m_gprOrInvalid;
    }
    
    void use()
    {
        m_jit->use(node());
    }

private:
    SpeculativeJIT* m_jit;
    Edge m_edge;
    GPRReg m_gprOrInvalid;
    DataFormat m_format;
};

class JSValueOperand {
public:
    explicit JSValueOperand(SpeculativeJIT* jit, Edge edge, OperandSpeculationMode mode = AutomaticOperandSpeculation)
        : m_jit(jit)
        , m_edge(edge)
#if USE(JSVALUE64)
        , m_gprOrInvalid(InvalidGPRReg)
#elif USE(JSVALUE32_64)
        , m_isDouble(false)
#endif
    {
        ASSERT(m_jit);
        ASSERT_UNUSED(mode, mode == ManualOperandSpeculation || edge.useKind() == UntypedUse);
#if USE(JSVALUE64)
        if (jit->isFilled(node()))
            gpr();
#elif USE(JSVALUE32_64)
        m_register.pair.tagGPR = InvalidGPRReg;
        m_register.pair.payloadGPR = InvalidGPRReg;
        if (jit->isFilled(node()))
            fill();
#endif
    }

    ~JSValueOperand()
    {
#if USE(JSVALUE64)
        ASSERT(m_gprOrInvalid != InvalidGPRReg);
        m_jit->unlock(m_gprOrInvalid);
#elif USE(JSVALUE32_64)
        if (m_isDouble) {
            ASSERT(m_register.fpr != InvalidFPRReg);
            m_jit->unlock(m_register.fpr);
        } else {
            ASSERT(m_register.pair.tagGPR != InvalidGPRReg && m_register.pair.payloadGPR != InvalidGPRReg);
            m_jit->unlock(m_register.pair.tagGPR);
            m_jit->unlock(m_register.pair.payloadGPR);
        }
#endif
    }
    
    Edge edge() const
    {
        return m_edge;
    }

    Node* node() const
    {
        return edge().node();
    }

#if USE(JSVALUE64)
    GPRReg gpr()
    {
        if (m_gprOrInvalid == InvalidGPRReg)
            m_gprOrInvalid = m_jit->fillJSValue(m_edge);
        return m_gprOrInvalid;
    }
    JSValueRegs jsValueRegs()
    {
        return JSValueRegs(gpr());
    }
#elif USE(JSVALUE32_64)
    bool isDouble() { return m_isDouble; }

    void fill()
    {
        if (m_register.pair.tagGPR == InvalidGPRReg && m_register.pair.payloadGPR == InvalidGPRReg)
            m_isDouble = !m_jit->fillJSValue(m_edge, m_register.pair.tagGPR, m_register.pair.payloadGPR, m_register.fpr);
    }

    GPRReg tagGPR()
    {
        fill();
        ASSERT(!m_isDouble);
        return m_register.pair.tagGPR;
    }

    GPRReg payloadGPR()
    {
        fill();
        ASSERT(!m_isDouble);
        return m_register.pair.payloadGPR;
    }

    JSValueRegs jsValueRegs()
    {
        return JSValueRegs(tagGPR(), payloadGPR());
    }

    FPRReg fpr()
    {
        fill();
        ASSERT(m_isDouble);
        return m_register.fpr;
    }
#endif

    void use()
    {
        m_jit->use(node());
    }

private:
    SpeculativeJIT* m_jit;
    Edge m_edge;
#if USE(JSVALUE64)
    GPRReg m_gprOrInvalid;
#elif USE(JSVALUE32_64)
    union {
        struct {
            GPRReg tagGPR;
            GPRReg payloadGPR;
        } pair;
        FPRReg fpr;
    } m_register;
    bool m_isDouble;
#endif
};

class StorageOperand {
public:
    explicit StorageOperand(SpeculativeJIT* jit, Edge edge)
        : m_jit(jit)
        , m_edge(edge)
        , m_gprOrInvalid(InvalidGPRReg)
    {
        ASSERT(m_jit);
        ASSERT(edge.useKind() == UntypedUse || edge.useKind() == KnownCellUse);
        if (jit->isFilled(node()))
            gpr();
    }
    
    ~StorageOperand()
    {
        ASSERT(m_gprOrInvalid != InvalidGPRReg);
        m_jit->unlock(m_gprOrInvalid);
    }
    
    Edge edge() const
    {
        return m_edge;
    }
    
    Node* node() const
    {
        return edge().node();
    }
    
    GPRReg gpr()
    {
        if (m_gprOrInvalid == InvalidGPRReg)
            m_gprOrInvalid = m_jit->fillStorage(edge());
        return m_gprOrInvalid;
    }
    
    void use()
    {
        m_jit->use(node());
    }
    
private:
    SpeculativeJIT* m_jit;
    Edge m_edge;
    GPRReg m_gprOrInvalid;
};


// === Temporaries ===
//
// These classes are used to allocate temporary registers.
// A mechanism is provided to attempt to reuse the registers
// currently allocated to child nodes whose value is consumed
// by, and not live after, this operation.

class GPRTemporary {
public:
    GPRTemporary();
    GPRTemporary(SpeculativeJIT*);
    GPRTemporary(SpeculativeJIT*, GPRReg specific);
    GPRTemporary(SpeculativeJIT*, SpeculateIntegerOperand&);
    GPRTemporary(SpeculativeJIT*, SpeculateIntegerOperand&, SpeculateIntegerOperand&);
    GPRTemporary(SpeculativeJIT*, SpeculateStrictInt32Operand&);
    GPRTemporary(SpeculativeJIT*, IntegerOperand&);
    GPRTemporary(SpeculativeJIT*, IntegerOperand&, IntegerOperand&);
    GPRTemporary(SpeculativeJIT*, SpeculateCellOperand&);
    GPRTemporary(SpeculativeJIT*, SpeculateBooleanOperand&);
#if USE(JSVALUE64)
    GPRTemporary(SpeculativeJIT*, JSValueOperand&);
#elif USE(JSVALUE32_64)
    GPRTemporary(SpeculativeJIT*, JSValueOperand&, bool tag = true);
#endif
    GPRTemporary(SpeculativeJIT*, StorageOperand&);

    void adopt(GPRTemporary&);

    ~GPRTemporary()
    {
        if (m_jit && m_gpr != InvalidGPRReg)
            m_jit->unlock(gpr());
    }

    GPRReg gpr()
    {
        return m_gpr;
    }

private:
    SpeculativeJIT* m_jit;
    GPRReg m_gpr;
};

class FPRTemporary {
public:
    FPRTemporary(SpeculativeJIT*);
    FPRTemporary(SpeculativeJIT*, SpeculateDoubleOperand&);
    FPRTemporary(SpeculativeJIT*, SpeculateDoubleOperand&, SpeculateDoubleOperand&);
#if USE(JSVALUE32_64)
    FPRTemporary(SpeculativeJIT*, JSValueOperand&);
#endif

    ~FPRTemporary()
    {
        m_jit->unlock(fpr());
    }

    FPRReg fpr() const
    {
        ASSERT(m_fpr != InvalidFPRReg);
        return m_fpr;
    }

protected:
    FPRTemporary(SpeculativeJIT* jit, FPRReg lockedFPR)
        : m_jit(jit)
        , m_fpr(lockedFPR)
    {
    }

private:
    SpeculativeJIT* m_jit;
    FPRReg m_fpr;
};


// === Results ===
//
// These classes lock the result of a call to a C++ helper function.

class GPRResult : public GPRTemporary {
public:
    GPRResult(SpeculativeJIT* jit)
        : GPRTemporary(jit, GPRInfo::returnValueGPR)
    {
    }
};

#if USE(JSVALUE32_64)
class GPRResult2 : public GPRTemporary {
public:
    GPRResult2(SpeculativeJIT* jit)
        : GPRTemporary(jit, GPRInfo::returnValueGPR2)
    {
    }
};
#endif

class FPRResult : public FPRTemporary {
public:
    FPRResult(SpeculativeJIT* jit)
        : FPRTemporary(jit, lockedResult(jit))
    {
    }

private:
    static FPRReg lockedResult(SpeculativeJIT* jit)
    {
        jit->lock(FPRInfo::returnValueFPR);
        return FPRInfo::returnValueFPR;
    }
};


// === Speculative Operand types ===
//
// SpeculateIntegerOperand, SpeculateStrictInt32Operand and SpeculateCellOperand.
//
// These are used to lock the operands to a node into machine registers within the
// SpeculativeJIT. The classes operate like those above, however these will
// perform a speculative check for a more restrictive type than we can statically
// determine the operand to have. If the operand does not have the requested type,
// a bail-out to the non-speculative path will be taken.

class SpeculateIntegerOperand {
public:
    explicit SpeculateIntegerOperand(SpeculativeJIT* jit, Edge edge, OperandSpeculationMode mode = AutomaticOperandSpeculation)
        : m_jit(jit)
        , m_edge(edge)
        , m_gprOrInvalid(InvalidGPRReg)
#ifndef NDEBUG
        , m_format(DataFormatNone)
#endif
    {
        ASSERT(m_jit);
        ASSERT_UNUSED(mode, mode == ManualOperandSpeculation || (edge.useKind() == Int32Use || edge.useKind() == KnownInt32Use));
        if (jit->isFilled(node()))
            gpr();
    }

    ~SpeculateIntegerOperand()
    {
        ASSERT(m_gprOrInvalid != InvalidGPRReg);
        m_jit->unlock(m_gprOrInvalid);
    }
    
    Edge edge() const
    {
        return m_edge;
    }

    Node* node() const
    {
        return edge().node();
    }

    DataFormat format()
    {
        gpr(); // m_format is set when m_gpr is locked.
        ASSERT(m_format == DataFormatInteger || m_format == DataFormatJSInteger);
        return m_format;
    }

    GPRReg gpr()
    {
        if (m_gprOrInvalid == InvalidGPRReg)
            m_gprOrInvalid = m_jit->fillSpeculateInt(edge(), m_format);
        return m_gprOrInvalid;
    }
    
    void use()
    {
        m_jit->use(node());
    }

private:
    SpeculativeJIT* m_jit;
    Edge m_edge;
    GPRReg m_gprOrInvalid;
    DataFormat m_format;
};

class SpeculateStrictInt32Operand {
public:
    explicit SpeculateStrictInt32Operand(SpeculativeJIT* jit, Edge edge, OperandSpeculationMode mode = AutomaticOperandSpeculation)
        : m_jit(jit)
        , m_edge(edge)
        , m_gprOrInvalid(InvalidGPRReg)
    {
        ASSERT(m_jit);
        ASSERT_UNUSED(mode, mode == ManualOperandSpeculation || (edge.useKind() == Int32Use || edge.useKind() == KnownInt32Use));
        if (jit->isFilled(node()))
            gpr();
    }

    ~SpeculateStrictInt32Operand()
    {
        ASSERT(m_gprOrInvalid != InvalidGPRReg);
        m_jit->unlock(m_gprOrInvalid);
    }
    
    Edge edge() const
    {
        return m_edge;
    }

    Node* node() const
    {
        return edge().node();
    }

    GPRReg gpr()
    {
        if (m_gprOrInvalid == InvalidGPRReg)
            m_gprOrInvalid = m_jit->fillSpeculateIntStrict(edge());
        return m_gprOrInvalid;
    }
    
    void use()
    {
        m_jit->use(node());
    }

private:
    SpeculativeJIT* m_jit;
    Edge m_edge;
    GPRReg m_gprOrInvalid;
};

class SpeculateDoubleOperand {
public:
    explicit SpeculateDoubleOperand(SpeculativeJIT* jit, Edge edge, OperandSpeculationMode mode = AutomaticOperandSpeculation)
        : m_jit(jit)
        , m_edge(edge)
        , m_fprOrInvalid(InvalidFPRReg)
    {
        ASSERT(m_jit);
        ASSERT_UNUSED(mode, mode == ManualOperandSpeculation || (edge.useKind() == NumberUse || edge.useKind() == KnownNumberUse || edge.useKind() == RealNumberUse));
        if (jit->isFilled(node()))
            fpr();
    }

    ~SpeculateDoubleOperand()
    {
        ASSERT(m_fprOrInvalid != InvalidFPRReg);
        m_jit->unlock(m_fprOrInvalid);
    }
    
    Edge edge() const
    {
        return m_edge;
    }

    Node* node() const
    {
        return edge().node();
    }

    FPRReg fpr()
    {
        if (m_fprOrInvalid == InvalidFPRReg)
            m_fprOrInvalid = m_jit->fillSpeculateDouble(edge());
        return m_fprOrInvalid;
    }
    
    void use()
    {
        m_jit->use(node());
    }

private:
    SpeculativeJIT* m_jit;
    Edge m_edge;
    FPRReg m_fprOrInvalid;
};

class SpeculateCellOperand {
public:
    explicit SpeculateCellOperand(SpeculativeJIT* jit, Edge edge, OperandSpeculationMode mode = AutomaticOperandSpeculation)
        : m_jit(jit)
        , m_edge(edge)
        , m_gprOrInvalid(InvalidGPRReg)
    {
        ASSERT(m_jit);
        if (!edge)
            return;
        ASSERT_UNUSED(mode, mode == ManualOperandSpeculation || (edge.useKind() == CellUse || edge.useKind() == KnownCellUse || edge.useKind() == ObjectUse || edge.useKind() == StringUse || edge.useKind() == KnownStringUse || edge.useKind() == StringObjectUse || edge.useKind() == StringOrStringObjectUse));
        if (jit->isFilled(node()))
            gpr();
    }

    ~SpeculateCellOperand()
    {
        if (!m_edge)
            return;
        ASSERT(m_gprOrInvalid != InvalidGPRReg);
        m_jit->unlock(m_gprOrInvalid);
    }
    
    Edge edge() const
    {
        return m_edge;
    }

    Node* node() const
    {
        return edge().node();
    }

    GPRReg gpr()
    {
        ASSERT(m_edge);
        if (m_gprOrInvalid == InvalidGPRReg)
            m_gprOrInvalid = m_jit->fillSpeculateCell(edge());
        return m_gprOrInvalid;
    }
    
    void use()
    {
        ASSERT(m_edge);
        m_jit->use(node());
    }

private:
    SpeculativeJIT* m_jit;
    Edge m_edge;
    GPRReg m_gprOrInvalid;
};

class SpeculateBooleanOperand {
public:
    explicit SpeculateBooleanOperand(SpeculativeJIT* jit, Edge edge, OperandSpeculationMode mode = AutomaticOperandSpeculation)
        : m_jit(jit)
        , m_edge(edge)
        , m_gprOrInvalid(InvalidGPRReg)
    {
        ASSERT(m_jit);
        ASSERT_UNUSED(mode, mode == ManualOperandSpeculation || edge.useKind() == BooleanUse);
        if (jit->isFilled(node()))
            gpr();
    }
    
    ~SpeculateBooleanOperand()
    {
        ASSERT(m_gprOrInvalid != InvalidGPRReg);
        m_jit->unlock(m_gprOrInvalid);
    }
    
    Edge edge() const
    {
        return m_edge;
    }
    
    Node* node() const
    {
        return edge().node();
    }
    
    GPRReg gpr()
    {
        if (m_gprOrInvalid == InvalidGPRReg)
            m_gprOrInvalid = m_jit->fillSpeculateBoolean(edge());
        return m_gprOrInvalid;
    }
    
    void use()
    {
        m_jit->use(node());
    }

private:
    SpeculativeJIT* m_jit;
    Edge m_edge;
    GPRReg m_gprOrInvalid;
};

template<typename StructureLocationType>
void SpeculativeJIT::speculateStringObjectForStructure(Edge edge, StructureLocationType structureLocation)
{
    Structure* stringObjectStructure =
        m_jit.globalObjectFor(m_currentNode->codeOrigin)->stringObjectStructure();
    Structure* stringPrototypeStructure = stringObjectStructure->storedPrototype().asCell()->structure();
    ASSERT(stringPrototypeStructure->transitionWatchpointSetIsStillValid());
    
    if (!m_state.forNode(edge).m_currentKnownStructure.isSubsetOf(StructureSet(m_jit.globalObjectFor(m_currentNode->codeOrigin)->stringObjectStructure()))) {
        speculationCheck(
            NotStringObject, JSValueRegs(), 0,
            m_jit.branchPtr(
                JITCompiler::NotEqual, structureLocation, TrustedImmPtr(stringObjectStructure)));
    }
    stringPrototypeStructure->addTransitionWatchpoint(speculationWatchpoint(NotStringObject));
}

#define DFG_TYPE_CHECK(source, edge, typesPassedThrough, jumpToFail) do { \
        if (!needsTypeCheck((edge), (typesPassedThrough)))              \
            break;                                                      \
        typeCheck((source), (edge), (typesPassedThrough), (jumpToFail)); \
    } while (0)

} } // namespace JSC::DFG

#endif
#endif

