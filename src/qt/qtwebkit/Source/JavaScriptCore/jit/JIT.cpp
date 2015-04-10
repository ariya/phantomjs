/*
 * Copyright (C) 2008, 2009, 2012, 2013 Apple Inc. All rights reserved.
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

#if ENABLE(JIT)
#include "JIT.h"

// This probably does not belong here; adding here for now as a quick Windows build fix.
#if ENABLE(ASSEMBLER) && CPU(X86) && !OS(MAC_OS_X)
#include "MacroAssembler.h"
JSC::MacroAssemblerX86Common::SSE2CheckState JSC::MacroAssemblerX86Common::s_sse2CheckState = NotCheckedSSE2;
#endif

#include "CodeBlock.h"
#include <wtf/CryptographicallyRandomNumber.h>
#include "DFGNode.h" // for DFG_SUCCESS_STATS
#include "Interpreter.h"
#include "JITInlines.h"
#include "JITStubCall.h"
#include "JSArray.h"
#include "JSFunction.h"
#include "LinkBuffer.h"
#include "Operations.h"
#include "RepatchBuffer.h"
#include "ResultType.h"
#include "SamplingTool.h"

using namespace std;

namespace JSC {

void ctiPatchNearCallByReturnAddress(CodeBlock* codeblock, ReturnAddressPtr returnAddress, MacroAssemblerCodePtr newCalleeFunction)
{
    RepatchBuffer repatchBuffer(codeblock);
    repatchBuffer.relinkNearCallerToTrampoline(returnAddress, newCalleeFunction);
}

void ctiPatchCallByReturnAddress(CodeBlock* codeblock, ReturnAddressPtr returnAddress, MacroAssemblerCodePtr newCalleeFunction)
{
    RepatchBuffer repatchBuffer(codeblock);
    repatchBuffer.relinkCallerToTrampoline(returnAddress, newCalleeFunction);
}

void ctiPatchCallByReturnAddress(CodeBlock* codeblock, ReturnAddressPtr returnAddress, FunctionPtr newCalleeFunction)
{
    RepatchBuffer repatchBuffer(codeblock);
    repatchBuffer.relinkCallerToFunction(returnAddress, newCalleeFunction);
}

JIT::JIT(VM* vm, CodeBlock* codeBlock)
    : m_interpreter(vm->interpreter)
    , m_vm(vm)
    , m_codeBlock(codeBlock)
    , m_labels(codeBlock ? codeBlock->numberOfInstructions() : 0)
    , m_bytecodeOffset((unsigned)-1)
    , m_propertyAccessInstructionIndex(UINT_MAX)
    , m_byValInstructionIndex(UINT_MAX)
    , m_globalResolveInfoIndex(UINT_MAX)
    , m_callLinkInfoIndex(UINT_MAX)
#if USE(JSVALUE32_64)
    , m_jumpTargetIndex(0)
    , m_mappedBytecodeOffset((unsigned)-1)
    , m_mappedVirtualRegisterIndex(JSStack::ReturnPC)
    , m_mappedTag((RegisterID)-1)
    , m_mappedPayload((RegisterID)-1)
#else
    , m_lastResultBytecodeRegister(std::numeric_limits<int>::max())
    , m_jumpTargetsPosition(0)
#endif
    , m_randomGenerator(cryptographicallyRandomNumber())
#if ENABLE(VALUE_PROFILER)
    , m_canBeOptimized(false)
    , m_shouldEmitProfiling(false)
#endif
{
}

#if ENABLE(DFG_JIT)
void JIT::emitEnterOptimizationCheck()
{
    if (!canBeOptimized())
        return;

    Jump skipOptimize = branchAdd32(Signed, TrustedImm32(Options::executionCounterIncrementForReturn()), AbsoluteAddress(m_codeBlock->addressOfJITExecuteCounter()));
    JITStubCall stubCall(this, cti_optimize);
    stubCall.addArgument(TrustedImm32(m_bytecodeOffset));
    ASSERT(!m_bytecodeOffset);
    stubCall.call();
    skipOptimize.link(this);
}
#endif

#define NEXT_OPCODE(name) \
    m_bytecodeOffset += OPCODE_LENGTH(name); \
    break;

#if USE(JSVALUE32_64)
#define DEFINE_BINARY_OP(name) \
    case name: { \
        JITStubCall stubCall(this, cti_##name); \
        stubCall.addArgument(currentInstruction[2].u.operand); \
        stubCall.addArgument(currentInstruction[3].u.operand); \
        stubCall.call(currentInstruction[1].u.operand); \
        NEXT_OPCODE(name); \
    }

#define DEFINE_UNARY_OP(name) \
    case name: { \
        JITStubCall stubCall(this, cti_##name); \
        stubCall.addArgument(currentInstruction[2].u.operand); \
        stubCall.call(currentInstruction[1].u.operand); \
        NEXT_OPCODE(name); \
    }

#else // USE(JSVALUE32_64)

#define DEFINE_BINARY_OP(name) \
    case name: { \
        JITStubCall stubCall(this, cti_##name); \
        stubCall.addArgument(currentInstruction[2].u.operand, regT2); \
        stubCall.addArgument(currentInstruction[3].u.operand, regT2); \
        stubCall.call(currentInstruction[1].u.operand); \
        NEXT_OPCODE(name); \
    }

#define DEFINE_UNARY_OP(name) \
    case name: { \
        JITStubCall stubCall(this, cti_##name); \
        stubCall.addArgument(currentInstruction[2].u.operand, regT2); \
        stubCall.call(currentInstruction[1].u.operand); \
        NEXT_OPCODE(name); \
    }
#endif // USE(JSVALUE32_64)

#define DEFINE_OP(name) \
    case name: { \
        emit_##name(currentInstruction); \
        NEXT_OPCODE(name); \
    }

#define DEFINE_SLOWCASE_OP(name) \
    case name: { \
        emitSlow_##name(currentInstruction, iter); \
        NEXT_OPCODE(name); \
    }

void JIT::privateCompileMainPass()
{
    Instruction* instructionsBegin = m_codeBlock->instructions().begin();
    unsigned instructionCount = m_codeBlock->instructions().size();

    m_globalResolveInfoIndex = 0;
    m_callLinkInfoIndex = 0;

    for (m_bytecodeOffset = 0; m_bytecodeOffset < instructionCount; ) {
        if (m_disassembler)
            m_disassembler->setForBytecodeMainPath(m_bytecodeOffset, label());
        Instruction* currentInstruction = instructionsBegin + m_bytecodeOffset;
        ASSERT_WITH_MESSAGE(m_interpreter->isOpcode(currentInstruction->u.opcode), "privateCompileMainPass gone bad @ %d", m_bytecodeOffset);

#if ENABLE(OPCODE_SAMPLING)
        if (m_bytecodeOffset > 0) // Avoid the overhead of sampling op_enter twice.
            sampleInstruction(currentInstruction);
#endif

#if USE(JSVALUE64)
        if (atJumpTarget())
            killLastResultRegister();
#endif

        m_labels[m_bytecodeOffset] = label();

#if ENABLE(JIT_VERBOSE)
        dataLogF("Old JIT emitting code for bc#%u at offset 0x%lx.\n", m_bytecodeOffset, (long)debugOffset());
#endif
        
        OpcodeID opcodeID = m_interpreter->getOpcodeID(currentInstruction->u.opcode);

        if (m_compilation && opcodeID != op_call_put_result) {
            add64(
                TrustedImm32(1),
                AbsoluteAddress(m_compilation->executionCounterFor(Profiler::OriginStack(Profiler::Origin(
                    m_compilation->bytecodes(), m_bytecodeOffset)))->address()));
        }

        switch (opcodeID) {
        DEFINE_BINARY_OP(op_del_by_val)
        DEFINE_BINARY_OP(op_in)
        DEFINE_BINARY_OP(op_less)
        DEFINE_BINARY_OP(op_lesseq)
        DEFINE_BINARY_OP(op_greater)
        DEFINE_BINARY_OP(op_greatereq)
        DEFINE_UNARY_OP(op_is_function)
        DEFINE_UNARY_OP(op_is_object)
        DEFINE_UNARY_OP(op_typeof)

        DEFINE_OP(op_add)
        DEFINE_OP(op_bitand)
        DEFINE_OP(op_bitor)
        DEFINE_OP(op_bitxor)
        DEFINE_OP(op_call)
        DEFINE_OP(op_call_eval)
        DEFINE_OP(op_call_varargs)
        DEFINE_OP(op_catch)
        DEFINE_OP(op_construct)
        DEFINE_OP(op_get_callee)
        DEFINE_OP(op_create_this)
        DEFINE_OP(op_convert_this)
        DEFINE_OP(op_init_lazy_reg)
        DEFINE_OP(op_create_arguments)
        DEFINE_OP(op_debug)
        DEFINE_OP(op_del_by_id)
        DEFINE_OP(op_div)
        DEFINE_OP(op_end)
        DEFINE_OP(op_enter)
        DEFINE_OP(op_create_activation)
        DEFINE_OP(op_eq)
        DEFINE_OP(op_eq_null)
        case op_get_by_id_out_of_line:
        case op_get_array_length:
        DEFINE_OP(op_get_by_id)
        DEFINE_OP(op_get_arguments_length)
        DEFINE_OP(op_get_by_val)
        DEFINE_OP(op_get_argument_by_val)
        DEFINE_OP(op_get_by_pname)
        DEFINE_OP(op_get_pnames)
        DEFINE_OP(op_check_has_instance)
        DEFINE_OP(op_instanceof)
        DEFINE_OP(op_is_undefined)
        DEFINE_OP(op_is_boolean)
        DEFINE_OP(op_is_number)
        DEFINE_OP(op_is_string)
        DEFINE_OP(op_jeq_null)
        DEFINE_OP(op_jfalse)
        DEFINE_OP(op_jmp)
        DEFINE_OP(op_jneq_null)
        DEFINE_OP(op_jneq_ptr)
        DEFINE_OP(op_jless)
        DEFINE_OP(op_jlesseq)
        DEFINE_OP(op_jgreater)
        DEFINE_OP(op_jgreatereq)
        DEFINE_OP(op_jnless)
        DEFINE_OP(op_jnlesseq)
        DEFINE_OP(op_jngreater)
        DEFINE_OP(op_jngreatereq)
        DEFINE_OP(op_jtrue)
        DEFINE_OP(op_loop_hint)
        DEFINE_OP(op_lshift)
        DEFINE_OP(op_mod)
        DEFINE_OP(op_mov)
        DEFINE_OP(op_mul)
        DEFINE_OP(op_negate)
        DEFINE_OP(op_neq)
        DEFINE_OP(op_neq_null)
        DEFINE_OP(op_new_array)
        DEFINE_OP(op_new_array_with_size)
        DEFINE_OP(op_new_array_buffer)
        DEFINE_OP(op_new_func)
        DEFINE_OP(op_new_func_exp)
        DEFINE_OP(op_new_object)
        DEFINE_OP(op_new_regexp)
        DEFINE_OP(op_next_pname)
        DEFINE_OP(op_not)
        DEFINE_OP(op_nstricteq)
        DEFINE_OP(op_pop_scope)
        DEFINE_OP(op_dec)
        DEFINE_OP(op_inc)
        DEFINE_OP(op_profile_did_call)
        DEFINE_OP(op_profile_will_call)
        DEFINE_OP(op_push_name_scope)
        DEFINE_OP(op_push_with_scope)
        case op_put_by_id_out_of_line:
        case op_put_by_id_transition_direct:
        case op_put_by_id_transition_normal:
        case op_put_by_id_transition_direct_out_of_line:
        case op_put_by_id_transition_normal_out_of_line:
        DEFINE_OP(op_put_by_id)
        DEFINE_OP(op_put_by_index)
        DEFINE_OP(op_put_by_val)
        DEFINE_OP(op_put_getter_setter)
        case op_init_global_const_nop:
            NEXT_OPCODE(op_init_global_const_nop);
        DEFINE_OP(op_init_global_const)
        DEFINE_OP(op_init_global_const_check)

        case op_resolve_global_property:
        case op_resolve_global_var:
        case op_resolve_scoped_var:
        case op_resolve_scoped_var_on_top_scope:
        case op_resolve_scoped_var_with_top_scope_check:
        DEFINE_OP(op_resolve)

        case op_resolve_base_to_global:
        case op_resolve_base_to_global_dynamic:
        case op_resolve_base_to_scope:
        case op_resolve_base_to_scope_with_top_scope_check:
        DEFINE_OP(op_resolve_base)

        case op_put_to_base_variable:
        DEFINE_OP(op_put_to_base)

        DEFINE_OP(op_resolve_with_base)
        DEFINE_OP(op_resolve_with_this)
        DEFINE_OP(op_ret)
        DEFINE_OP(op_call_put_result)
        DEFINE_OP(op_ret_object_or_this)
        DEFINE_OP(op_rshift)
        DEFINE_OP(op_urshift)
        DEFINE_OP(op_strcat)
        DEFINE_OP(op_stricteq)
        DEFINE_OP(op_sub)
        DEFINE_OP(op_switch_char)
        DEFINE_OP(op_switch_imm)
        DEFINE_OP(op_switch_string)
        DEFINE_OP(op_tear_off_activation)
        DEFINE_OP(op_tear_off_arguments)
        DEFINE_OP(op_throw)
        DEFINE_OP(op_throw_static_error)
        DEFINE_OP(op_to_number)
        DEFINE_OP(op_to_primitive)

        DEFINE_OP(op_get_scoped_var)
        DEFINE_OP(op_put_scoped_var)

        case op_get_by_id_chain:
        case op_get_by_id_generic:
        case op_get_by_id_proto:
        case op_get_by_id_self:
        case op_get_by_id_getter_chain:
        case op_get_by_id_getter_proto:
        case op_get_by_id_getter_self:
        case op_get_by_id_custom_chain:
        case op_get_by_id_custom_proto:
        case op_get_by_id_custom_self:
        case op_get_string_length:
        case op_put_by_id_generic:
        case op_put_by_id_replace:
        case op_put_by_id_transition:
            RELEASE_ASSERT_NOT_REACHED();
        }
    }

    RELEASE_ASSERT(m_callLinkInfoIndex == m_callStructureStubCompilationInfo.size());

#ifndef NDEBUG
    // Reset this, in order to guard its use with ASSERTs.
    m_bytecodeOffset = (unsigned)-1;
#endif
}

void JIT::privateCompileLinkPass()
{
    unsigned jmpTableCount = m_jmpTable.size();
    for (unsigned i = 0; i < jmpTableCount; ++i)
        m_jmpTable[i].from.linkTo(m_labels[m_jmpTable[i].toBytecodeOffset], this);
    m_jmpTable.clear();
}

void JIT::privateCompileSlowCases()
{
    Instruction* instructionsBegin = m_codeBlock->instructions().begin();

    m_propertyAccessInstructionIndex = 0;
    m_byValInstructionIndex = 0;
    m_globalResolveInfoIndex = 0;
    m_callLinkInfoIndex = 0;
    
#if ENABLE(VALUE_PROFILER)
    // Use this to assert that slow-path code associates new profiling sites with existing
    // ValueProfiles rather than creating new ones. This ensures that for a given instruction
    // (say, get_by_id) we get combined statistics for both the fast-path executions of that
    // instructions and the slow-path executions. Furthermore, if the slow-path code created
    // new ValueProfiles then the ValueProfiles would no longer be sorted by bytecode offset,
    // which would break the invariant necessary to use CodeBlock::valueProfileForBytecodeOffset().
    unsigned numberOfValueProfiles = m_codeBlock->numberOfValueProfiles();
#endif

    for (Vector<SlowCaseEntry>::iterator iter = m_slowCases.begin(); iter != m_slowCases.end();) {
#if USE(JSVALUE64)
        killLastResultRegister();
#endif

        m_bytecodeOffset = iter->to;

        unsigned firstTo = m_bytecodeOffset;

        Instruction* currentInstruction = instructionsBegin + m_bytecodeOffset;
        
#if ENABLE(VALUE_PROFILER)
        RareCaseProfile* rareCaseProfile = 0;
        if (shouldEmitProfiling())
            rareCaseProfile = m_codeBlock->addRareCaseProfile(m_bytecodeOffset);
#endif

#if ENABLE(JIT_VERBOSE)
        dataLogF("Old JIT emitting slow code for bc#%u at offset 0x%lx.\n", m_bytecodeOffset, (long)debugOffset());
#endif
        
        if (m_disassembler)
            m_disassembler->setForBytecodeSlowPath(m_bytecodeOffset, label());

        switch (m_interpreter->getOpcodeID(currentInstruction->u.opcode)) {
        DEFINE_SLOWCASE_OP(op_add)
        DEFINE_SLOWCASE_OP(op_bitand)
        DEFINE_SLOWCASE_OP(op_bitor)
        DEFINE_SLOWCASE_OP(op_bitxor)
        DEFINE_SLOWCASE_OP(op_call)
        DEFINE_SLOWCASE_OP(op_call_eval)
        DEFINE_SLOWCASE_OP(op_call_varargs)
        DEFINE_SLOWCASE_OP(op_construct)
        DEFINE_SLOWCASE_OP(op_convert_this)
        DEFINE_SLOWCASE_OP(op_create_this)
        DEFINE_SLOWCASE_OP(op_div)
        DEFINE_SLOWCASE_OP(op_eq)
        case op_get_by_id_out_of_line:
        case op_get_array_length:
        DEFINE_SLOWCASE_OP(op_get_by_id)
        DEFINE_SLOWCASE_OP(op_get_arguments_length)
        DEFINE_SLOWCASE_OP(op_get_by_val)
        DEFINE_SLOWCASE_OP(op_get_argument_by_val)
        DEFINE_SLOWCASE_OP(op_get_by_pname)
        DEFINE_SLOWCASE_OP(op_check_has_instance)
        DEFINE_SLOWCASE_OP(op_instanceof)
        DEFINE_SLOWCASE_OP(op_jfalse)
        DEFINE_SLOWCASE_OP(op_jless)
        DEFINE_SLOWCASE_OP(op_jlesseq)
        DEFINE_SLOWCASE_OP(op_jgreater)
        DEFINE_SLOWCASE_OP(op_jgreatereq)
        DEFINE_SLOWCASE_OP(op_jnless)
        DEFINE_SLOWCASE_OP(op_jnlesseq)
        DEFINE_SLOWCASE_OP(op_jngreater)
        DEFINE_SLOWCASE_OP(op_jngreatereq)
        DEFINE_SLOWCASE_OP(op_jtrue)
        DEFINE_SLOWCASE_OP(op_loop_hint)
        DEFINE_SLOWCASE_OP(op_lshift)
        DEFINE_SLOWCASE_OP(op_mod)
        DEFINE_SLOWCASE_OP(op_mul)
        DEFINE_SLOWCASE_OP(op_negate)
        DEFINE_SLOWCASE_OP(op_neq)
        DEFINE_SLOWCASE_OP(op_new_object)
        DEFINE_SLOWCASE_OP(op_not)
        DEFINE_SLOWCASE_OP(op_nstricteq)
        DEFINE_SLOWCASE_OP(op_dec)
        DEFINE_SLOWCASE_OP(op_inc)
        case op_put_by_id_out_of_line:
        case op_put_by_id_transition_direct:
        case op_put_by_id_transition_normal:
        case op_put_by_id_transition_direct_out_of_line:
        case op_put_by_id_transition_normal_out_of_line:
        DEFINE_SLOWCASE_OP(op_put_by_id)
        DEFINE_SLOWCASE_OP(op_put_by_val)
        DEFINE_SLOWCASE_OP(op_init_global_const_check);
        DEFINE_SLOWCASE_OP(op_rshift)
        DEFINE_SLOWCASE_OP(op_urshift)
        DEFINE_SLOWCASE_OP(op_stricteq)
        DEFINE_SLOWCASE_OP(op_sub)
        DEFINE_SLOWCASE_OP(op_to_number)
        DEFINE_SLOWCASE_OP(op_to_primitive)

        case op_resolve_global_property:
        case op_resolve_global_var:
        case op_resolve_scoped_var:
        case op_resolve_scoped_var_on_top_scope:
        case op_resolve_scoped_var_with_top_scope_check:
        DEFINE_SLOWCASE_OP(op_resolve)

        case op_resolve_base_to_global:
        case op_resolve_base_to_global_dynamic:
        case op_resolve_base_to_scope:
        case op_resolve_base_to_scope_with_top_scope_check:
        DEFINE_SLOWCASE_OP(op_resolve_base)
        DEFINE_SLOWCASE_OP(op_resolve_with_base)
        DEFINE_SLOWCASE_OP(op_resolve_with_this)

        case op_put_to_base_variable:
        DEFINE_SLOWCASE_OP(op_put_to_base)

        default:
            RELEASE_ASSERT_NOT_REACHED();
        }

        RELEASE_ASSERT_WITH_MESSAGE(iter == m_slowCases.end() || firstTo != iter->to, "Not enough jumps linked in slow case codegen.");
        RELEASE_ASSERT_WITH_MESSAGE(firstTo == (iter - 1)->to, "Too many jumps linked in slow case codegen.");
        
#if ENABLE(VALUE_PROFILER)
        if (shouldEmitProfiling())
            add32(TrustedImm32(1), AbsoluteAddress(&rareCaseProfile->m_counter));
#endif

        emitJumpSlowToHot(jump(), 0);
    }

    RELEASE_ASSERT(m_propertyAccessInstructionIndex == m_propertyAccessCompilationInfo.size());
    RELEASE_ASSERT(m_callLinkInfoIndex == m_callStructureStubCompilationInfo.size());
#if ENABLE(VALUE_PROFILER)
    RELEASE_ASSERT(numberOfValueProfiles == m_codeBlock->numberOfValueProfiles());
#endif

#ifndef NDEBUG
    // Reset this, in order to guard its use with ASSERTs.
    m_bytecodeOffset = (unsigned)-1;
#endif
}

ALWAYS_INLINE void PropertyStubCompilationInfo::copyToStubInfo(StructureStubInfo& info, LinkBuffer &linkBuffer)
{
    ASSERT(bytecodeIndex != std::numeric_limits<unsigned>::max());
    info.bytecodeIndex = bytecodeIndex;
    info.callReturnLocation = linkBuffer.locationOf(callReturnLocation);
    info.hotPathBegin = linkBuffer.locationOf(hotPathBegin);

    switch (m_type) {
    case GetById: {
        CodeLocationLabel hotPathBeginLocation = linkBuffer.locationOf(hotPathBegin);
        info.patch.baseline.u.get.structureToCompare = MacroAssembler::differenceBetweenCodePtr(hotPathBeginLocation, linkBuffer.locationOf(getStructureToCompare));
        info.patch.baseline.u.get.structureCheck = MacroAssembler::differenceBetweenCodePtr(hotPathBeginLocation, linkBuffer.locationOf(getStructureCheck));
        info.patch.baseline.u.get.propertyStorageLoad = MacroAssembler::differenceBetweenCodePtr(hotPathBeginLocation, linkBuffer.locationOf(propertyStorageLoad));
#if USE(JSVALUE64)
        info.patch.baseline.u.get.displacementLabel = MacroAssembler::differenceBetweenCodePtr(hotPathBeginLocation, linkBuffer.locationOf(getDisplacementLabel));
#else
        info.patch.baseline.u.get.displacementLabel1 = MacroAssembler::differenceBetweenCodePtr(hotPathBeginLocation, linkBuffer.locationOf(getDisplacementLabel1));
        info.patch.baseline.u.get.displacementLabel2 = MacroAssembler::differenceBetweenCodePtr(hotPathBeginLocation, linkBuffer.locationOf(getDisplacementLabel2));
#endif
        info.patch.baseline.u.get.putResult = MacroAssembler::differenceBetweenCodePtr(hotPathBeginLocation, linkBuffer.locationOf(getPutResult));
        info.patch.baseline.u.get.coldPathBegin = MacroAssembler::differenceBetweenCodePtr(linkBuffer.locationOf(getColdPathBegin), linkBuffer.locationOf(callReturnLocation));
        break;
    }
    case PutById:
        CodeLocationLabel hotPathBeginLocation = linkBuffer.locationOf(hotPathBegin);
        info.patch.baseline.u.put.structureToCompare = MacroAssembler::differenceBetweenCodePtr(hotPathBeginLocation, linkBuffer.locationOf(putStructureToCompare));
        info.patch.baseline.u.put.propertyStorageLoad = MacroAssembler::differenceBetweenCodePtr(hotPathBeginLocation, linkBuffer.locationOf(propertyStorageLoad));
#if USE(JSVALUE64)
        info.patch.baseline.u.put.displacementLabel = MacroAssembler::differenceBetweenCodePtr(hotPathBeginLocation, linkBuffer.locationOf(putDisplacementLabel));
#else
        info.patch.baseline.u.put.displacementLabel1 = MacroAssembler::differenceBetweenCodePtr(hotPathBeginLocation, linkBuffer.locationOf(putDisplacementLabel1));
        info.patch.baseline.u.put.displacementLabel2 = MacroAssembler::differenceBetweenCodePtr(hotPathBeginLocation, linkBuffer.locationOf(putDisplacementLabel2));
#endif
        break;
    }
}

JITCode JIT::privateCompile(CodePtr* functionEntryArityCheck, JITCompilationEffort effort)
{
#if ENABLE(JIT_VERBOSE_OSR)
    printf("Compiling JIT code!\n");
#endif
    
#if ENABLE(VALUE_PROFILER)
    DFG::CapabilityLevel level = m_codeBlock->canCompileWithDFG();
    switch (level) {
    case DFG::CannotCompile:
        m_canBeOptimized = false;
        m_shouldEmitProfiling = false;
        break;
    case DFG::MayInline:
        m_canBeOptimized = false;
        m_canBeOptimizedOrInlined = true;
        m_shouldEmitProfiling = true;
        break;
    case DFG::CanCompile:
        m_canBeOptimized = true;
        m_canBeOptimizedOrInlined = true;
        m_shouldEmitProfiling = true;
        break;
    default:
        RELEASE_ASSERT_NOT_REACHED();
        break;
    }
#endif
    
    if (Options::showDisassembly() || m_vm->m_perBytecodeProfiler)
        m_disassembler = adoptPtr(new JITDisassembler(m_codeBlock));
    if (m_vm->m_perBytecodeProfiler) {
        m_compilation = m_vm->m_perBytecodeProfiler->newCompilation(m_codeBlock, Profiler::Baseline);
        m_compilation->addProfiledBytecodes(*m_vm->m_perBytecodeProfiler, m_codeBlock);
    }
    
    if (m_disassembler)
        m_disassembler->setStartOfCode(label());

    // Just add a little bit of randomness to the codegen
    if (m_randomGenerator.getUint32() & 1)
        nop();

    preserveReturnAddressAfterCall(regT2);
    emitPutToCallFrameHeader(regT2, JSStack::ReturnPC);
    emitPutImmediateToCallFrameHeader(m_codeBlock, JSStack::CodeBlock);

    Label beginLabel(this);

    sampleCodeBlock(m_codeBlock);
#if ENABLE(OPCODE_SAMPLING)
    sampleInstruction(m_codeBlock->instructions().begin());
#endif

    Jump stackCheck;
    if (m_codeBlock->codeType() == FunctionCode) {
#if ENABLE(DFG_JIT)
#if DFG_ENABLE(SUCCESS_STATS)
        static SamplingCounter counter("orignalJIT");
        emitCount(counter);
#endif
#endif

#if ENABLE(VALUE_PROFILER)
        ASSERT(m_bytecodeOffset == (unsigned)-1);
        if (shouldEmitProfiling()) {
            for (int argument = 0; argument < m_codeBlock->numParameters(); ++argument) {
                // If this is a constructor, then we want to put in a dummy profiling site (to
                // keep things consistent) but we don't actually want to record the dummy value.
                if (m_codeBlock->m_isConstructor && !argument)
                    continue;
                int offset = CallFrame::argumentOffsetIncludingThis(argument) * static_cast<int>(sizeof(Register));
#if USE(JSVALUE64)
                load64(Address(callFrameRegister, offset), regT0);
#elif USE(JSVALUE32_64)
                load32(Address(callFrameRegister, offset + OBJECT_OFFSETOF(JSValue, u.asBits.payload)), regT0);
                load32(Address(callFrameRegister, offset + OBJECT_OFFSETOF(JSValue, u.asBits.tag)), regT1);
#endif
                emitValueProfilingSite(m_codeBlock->valueProfileForArgument(argument));
            }
        }
#endif

        addPtr(TrustedImm32(m_codeBlock->m_numCalleeRegisters * sizeof(Register)), callFrameRegister, regT1);
        stackCheck = branchPtr(Below, AbsoluteAddress(m_vm->interpreter->stack().addressOfEnd()), regT1);
    }

    Label functionBody = label();
    
    privateCompileMainPass();
    privateCompileLinkPass();
    privateCompileSlowCases();
    
    if (m_disassembler)
        m_disassembler->setEndOfSlowPath(label());

    Label arityCheck;
    if (m_codeBlock->codeType() == FunctionCode) {
        stackCheck.link(this);
        m_bytecodeOffset = 0;
        JITStubCall(this, cti_stack_check).call();
#ifndef NDEBUG
        m_bytecodeOffset = (unsigned)-1; // Reset this, in order to guard its use with ASSERTs.
#endif
        jump(functionBody);

        arityCheck = label();
        preserveReturnAddressAfterCall(regT2);
        emitPutToCallFrameHeader(regT2, JSStack::ReturnPC);
        emitPutImmediateToCallFrameHeader(m_codeBlock, JSStack::CodeBlock);

        load32(payloadFor(JSStack::ArgumentCount), regT1);
        branch32(AboveOrEqual, regT1, TrustedImm32(m_codeBlock->m_numParameters)).linkTo(beginLabel, this);

        m_bytecodeOffset = 0;
        JITStubCall(this, m_codeBlock->m_isConstructor ? cti_op_construct_arityCheck : cti_op_call_arityCheck).call(callFrameRegister);
#if !ASSERT_DISABLED
        m_bytecodeOffset = (unsigned)-1; // Reset this, in order to guard its use with ASSERTs.
#endif

        jump(beginLabel);
    }

    ASSERT(m_jmpTable.isEmpty());
    
    if (m_disassembler)
        m_disassembler->setEndOfCode(label());

    LinkBuffer patchBuffer(*m_vm, this, m_codeBlock, effort);
    if (patchBuffer.didFailToAllocate())
        return JITCode();

    // Translate vPC offsets into addresses in JIT generated code, for switch tables.
    for (unsigned i = 0; i < m_switches.size(); ++i) {
        SwitchRecord record = m_switches[i];
        unsigned bytecodeOffset = record.bytecodeOffset;

        if (record.type != SwitchRecord::String) {
            ASSERT(record.type == SwitchRecord::Immediate || record.type == SwitchRecord::Character); 
            ASSERT(record.jumpTable.simpleJumpTable->branchOffsets.size() == record.jumpTable.simpleJumpTable->ctiOffsets.size());

            record.jumpTable.simpleJumpTable->ctiDefault = patchBuffer.locationOf(m_labels[bytecodeOffset + record.defaultOffset]);

            for (unsigned j = 0; j < record.jumpTable.simpleJumpTable->branchOffsets.size(); ++j) {
                unsigned offset = record.jumpTable.simpleJumpTable->branchOffsets[j];
                record.jumpTable.simpleJumpTable->ctiOffsets[j] = offset ? patchBuffer.locationOf(m_labels[bytecodeOffset + offset]) : record.jumpTable.simpleJumpTable->ctiDefault;
            }
        } else {
            ASSERT(record.type == SwitchRecord::String);

            record.jumpTable.stringJumpTable->ctiDefault = patchBuffer.locationOf(m_labels[bytecodeOffset + record.defaultOffset]);

            StringJumpTable::StringOffsetTable::iterator end = record.jumpTable.stringJumpTable->offsetTable.end();            
            for (StringJumpTable::StringOffsetTable::iterator it = record.jumpTable.stringJumpTable->offsetTable.begin(); it != end; ++it) {
                unsigned offset = it->value.branchOffset;
                it->value.ctiOffset = offset ? patchBuffer.locationOf(m_labels[bytecodeOffset + offset]) : record.jumpTable.stringJumpTable->ctiDefault;
            }
        }
    }

    for (size_t i = 0; i < m_codeBlock->numberOfExceptionHandlers(); ++i) {
        HandlerInfo& handler = m_codeBlock->exceptionHandler(i);
        handler.nativeCode = patchBuffer.locationOf(m_labels[handler.target]);
    }

    for (Vector<CallRecord>::iterator iter = m_calls.begin(); iter != m_calls.end(); ++iter) {
        if (iter->to)
            patchBuffer.link(iter->from, FunctionPtr(iter->to));
    }

    m_codeBlock->callReturnIndexVector().reserveCapacity(m_calls.size());
    for (Vector<CallRecord>::iterator iter = m_calls.begin(); iter != m_calls.end(); ++iter)
        m_codeBlock->callReturnIndexVector().append(CallReturnOffsetToBytecodeOffset(patchBuffer.returnAddressOffset(iter->from), iter->bytecodeOffset));

    m_codeBlock->setNumberOfStructureStubInfos(m_propertyAccessCompilationInfo.size());
    for (unsigned i = 0; i < m_propertyAccessCompilationInfo.size(); ++i)
        m_propertyAccessCompilationInfo[i].copyToStubInfo(m_codeBlock->structureStubInfo(i), patchBuffer);
    m_codeBlock->setNumberOfByValInfos(m_byValCompilationInfo.size());
    for (unsigned i = 0; i < m_byValCompilationInfo.size(); ++i) {
        CodeLocationJump badTypeJump = CodeLocationJump(patchBuffer.locationOf(m_byValCompilationInfo[i].badTypeJump));
        CodeLocationLabel doneTarget = patchBuffer.locationOf(m_byValCompilationInfo[i].doneTarget);
        CodeLocationLabel slowPathTarget = patchBuffer.locationOf(m_byValCompilationInfo[i].slowPathTarget);
        CodeLocationCall returnAddress = patchBuffer.locationOf(m_byValCompilationInfo[i].returnAddress);
        
        m_codeBlock->byValInfo(i) = ByValInfo(
            m_byValCompilationInfo[i].bytecodeIndex,
            badTypeJump,
            m_byValCompilationInfo[i].arrayMode,
            differenceBetweenCodePtr(badTypeJump, doneTarget),
            differenceBetweenCodePtr(returnAddress, slowPathTarget));
    }
    m_codeBlock->setNumberOfCallLinkInfos(m_callStructureStubCompilationInfo.size());
    for (unsigned i = 0; i < m_codeBlock->numberOfCallLinkInfos(); ++i) {
        CallLinkInfo& info = m_codeBlock->callLinkInfo(i);
        info.callType = m_callStructureStubCompilationInfo[i].callType;
        info.codeOrigin = CodeOrigin(m_callStructureStubCompilationInfo[i].bytecodeIndex);
        info.callReturnLocation = patchBuffer.locationOfNearCall(m_callStructureStubCompilationInfo[i].callReturnLocation);
        info.hotPathBegin = patchBuffer.locationOf(m_callStructureStubCompilationInfo[i].hotPathBegin);
        info.hotPathOther = patchBuffer.locationOfNearCall(m_callStructureStubCompilationInfo[i].hotPathOther);
        info.calleeGPR = regT0;
    }

#if ENABLE(DFG_JIT) || ENABLE(LLINT)
    if (canBeOptimizedOrInlined()
#if ENABLE(LLINT)
        || true
#endif
        ) {
        CompactJITCodeMap::Encoder jitCodeMapEncoder;
        for (unsigned bytecodeOffset = 0; bytecodeOffset < m_labels.size(); ++bytecodeOffset) {
            if (m_labels[bytecodeOffset].isSet())
                jitCodeMapEncoder.append(bytecodeOffset, patchBuffer.offsetOf(m_labels[bytecodeOffset]));
        }
        m_codeBlock->setJITCodeMap(jitCodeMapEncoder.finish());
    }
#endif

    if (m_codeBlock->codeType() == FunctionCode && functionEntryArityCheck)
        *functionEntryArityCheck = patchBuffer.locationOf(arityCheck);

    if (Options::showDisassembly())
        m_disassembler->dump(patchBuffer);
    if (m_compilation)
        m_disassembler->reportToProfiler(m_compilation.get(), patchBuffer);
    
    CodeRef result = patchBuffer.finalizeCodeWithoutDisassembly();
    
    m_vm->machineCodeBytesPerBytecodeWordForBaselineJIT.add(
        static_cast<double>(result.size()) /
        static_cast<double>(m_codeBlock->instructions().size()));
    
    m_codeBlock->shrinkToFit(CodeBlock::LateShrink);
    
#if ENABLE(JIT_VERBOSE)
    dataLogF("JIT generated code for %p at [%p, %p).\n", m_codeBlock, result.executableMemory()->start(), result.executableMemory()->end());
#endif
    
    return JITCode(result, JITCode::BaselineJIT);
}

void JIT::linkFor(JSFunction* callee, CodeBlock* callerCodeBlock, CodeBlock* calleeCodeBlock, JIT::CodePtr code, CallLinkInfo* callLinkInfo, VM* vm, CodeSpecializationKind kind)
{
    RepatchBuffer repatchBuffer(callerCodeBlock);

    ASSERT(!callLinkInfo->isLinked());
    callLinkInfo->callee.set(*vm, callLinkInfo->hotPathBegin, callerCodeBlock->ownerExecutable(), callee);
    callLinkInfo->lastSeenCallee.set(*vm, callerCodeBlock->ownerExecutable(), callee);
    repatchBuffer.relink(callLinkInfo->hotPathOther, code);

    if (calleeCodeBlock)
        calleeCodeBlock->linkIncomingCall(callLinkInfo);

    // Patch the slow patch so we do not continue to try to link.
    if (kind == CodeForCall) {
        ASSERT(callLinkInfo->callType == CallLinkInfo::Call
               || callLinkInfo->callType == CallLinkInfo::CallVarargs);
        if (callLinkInfo->callType == CallLinkInfo::Call) {
            repatchBuffer.relink(callLinkInfo->callReturnLocation, vm->getCTIStub(linkClosureCallGenerator).code());
            return;
        }

        repatchBuffer.relink(callLinkInfo->callReturnLocation, vm->getCTIStub(virtualCallGenerator).code());
        return;
    }

    ASSERT(kind == CodeForConstruct);
    repatchBuffer.relink(callLinkInfo->callReturnLocation, vm->getCTIStub(virtualConstructGenerator).code());
}

void JIT::linkSlowCall(CodeBlock* callerCodeBlock, CallLinkInfo* callLinkInfo)
{
    RepatchBuffer repatchBuffer(callerCodeBlock);

    repatchBuffer.relink(callLinkInfo->callReturnLocation, callerCodeBlock->vm()->getCTIStub(virtualCallGenerator).code());
}

} // namespace JSC

#endif // ENABLE(JIT)
