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

#include "config.h"
#include "LLIntSlowPaths.h"

#if ENABLE(LLINT)

#include "Arguments.h"
#include "ArrayConstructor.h"
#include "CallFrame.h"
#include "CommonSlowPaths.h"
#include "GetterSetter.h"
#include "HostCallReturnValue.h"
#include "Interpreter.h"
#include "JIT.h"
#include "JITDriver.h"
#include "JSActivation.h"
#include "JSCJSValue.h"
#include "JSGlobalObjectFunctions.h"
#include "JSNameScope.h"
#include "JSPropertyNameIterator.h"
#include "JSString.h"
#include "JSWithScope.h"
#include "LLIntCommon.h"
#include "LLIntExceptions.h"
#include "LowLevelInterpreter.h"
#include "ObjectConstructor.h"
#include "Operations.h"
#include "StructureRareDataInlines.h"
#include <wtf/StringPrintStream.h>

namespace JSC { namespace LLInt {

#define LLINT_BEGIN_NO_SET_PC() \
    VM& vm = exec->vm();      \
    NativeCallFrameTracer tracer(&vm, exec)

#ifndef NDEBUG
#define LLINT_SET_PC_FOR_STUBS() do { \
        exec->codeBlock()->bytecodeOffset(pc); \
        exec->setCurrentVPC(pc + 1); \
    } while (false)
#else
#define LLINT_SET_PC_FOR_STUBS() do { \
        exec->setCurrentVPC(pc + 1); \
    } while (false)
#endif

#define LLINT_BEGIN()                           \
    LLINT_BEGIN_NO_SET_PC();                    \
    LLINT_SET_PC_FOR_STUBS()

#define LLINT_OP(index) (exec->uncheckedR(pc[index].u.operand))
#define LLINT_OP_C(index) (exec->r(pc[index].u.operand))

#define LLINT_RETURN_TWO(first, second) do {       \
        return encodeResult(first, second);        \
    } while (false)

#define LLINT_END_IMPL() LLINT_RETURN_TWO(pc, exec)

#define LLINT_THROW(exceptionToThrow) do {                        \
        vm.exception = (exceptionToThrow);                \
        pc = returnToThrow(exec, pc);                             \
        LLINT_END_IMPL();                                         \
    } while (false)

#define LLINT_CHECK_EXCEPTION() do {                    \
        if (UNLIKELY(vm.exception)) {           \
            pc = returnToThrow(exec, pc);               \
            LLINT_END_IMPL();                           \
        }                                               \
    } while (false)

#define LLINT_END() do {                        \
        LLINT_CHECK_EXCEPTION();                \
        LLINT_END_IMPL();                       \
    } while (false)

#define LLINT_BRANCH(opcode, condition) do {                      \
        bool __b_condition = (condition);                         \
        LLINT_CHECK_EXCEPTION();                                  \
        if (__b_condition)                                        \
            pc += pc[OPCODE_LENGTH(opcode) - 1].u.operand;        \
        else                                                      \
            pc += OPCODE_LENGTH(opcode);                          \
        LLINT_END_IMPL();                                         \
    } while (false)

#define LLINT_RETURN(value) do {                \
        JSValue __r_returnValue = (value);      \
        LLINT_CHECK_EXCEPTION();                \
        LLINT_OP(1) = __r_returnValue;          \
        LLINT_END_IMPL();                       \
    } while (false)

#if ENABLE(VALUE_PROFILER)
#define LLINT_RETURN_PROFILED(opcode, value) do {               \
        JSValue __rp_returnValue = (value);                     \
        LLINT_CHECK_EXCEPTION();                                \
        LLINT_OP(1) = __rp_returnValue;                         \
        LLINT_PROFILE_VALUE(opcode, __rp_returnValue);          \
        LLINT_END_IMPL();                                       \
    } while (false)

#define LLINT_PROFILE_VALUE(opcode, value) do { \
        pc[OPCODE_LENGTH(opcode) - 1].u.profile->m_buckets[0] = \
        JSValue::encode(value);                  \
    } while (false)

#else // ENABLE(VALUE_PROFILER)
#define LLINT_RETURN_PROFILED(opcode, value) LLINT_RETURN(value)

#define LLINT_PROFILE_VALUE(opcode, value) do { } while (false)

#endif // ENABLE(VALUE_PROFILER)

#define LLINT_CALL_END_IMPL(exec, callTarget) LLINT_RETURN_TWO((callTarget), (exec))

#define LLINT_CALL_THROW(exec, pc, exceptionToThrow) do {               \
        ExecState* __ct_exec = (exec);                                  \
        Instruction* __ct_pc = (pc);                                    \
        vm.exception = (exceptionToThrow);                      \
        LLINT_CALL_END_IMPL(__ct_exec, callToThrow(__ct_exec, __ct_pc)); \
    } while (false)

#define LLINT_CALL_CHECK_EXCEPTION(exec, pc) do {                       \
        ExecState* __cce_exec = (exec);                                 \
        Instruction* __cce_pc = (pc);                                   \
        if (UNLIKELY(vm.exception))                              \
            LLINT_CALL_END_IMPL(__cce_exec, callToThrow(__cce_exec, __cce_pc)); \
    } while (false)

#define LLINT_CALL_RETURN(exec, pc, callTarget) do {                    \
        ExecState* __cr_exec = (exec);                                  \
        Instruction* __cr_pc = (pc);                                    \
        void* __cr_callTarget = (callTarget);                           \
        LLINT_CALL_CHECK_EXCEPTION(__cr_exec->callerFrame(), __cr_pc);  \
        LLINT_CALL_END_IMPL(__cr_exec, __cr_callTarget);                \
    } while (false)

extern "C" SlowPathReturnType llint_trace_operand(ExecState* exec, Instruction* pc, int fromWhere, int operand)
{
    LLINT_BEGIN();
    dataLogF("%p / %p: executing bc#%zu, op#%u: Trace(%d): %d: %d\n",
            exec->codeBlock(),
            exec,
            static_cast<intptr_t>(pc - exec->codeBlock()->instructions().begin()),
            exec->vm().interpreter->getOpcodeID(pc[0].u.opcode),
            fromWhere,
            operand,
            pc[operand].u.operand);
    LLINT_END();
}

extern "C" SlowPathReturnType llint_trace_value(ExecState* exec, Instruction* pc, int fromWhere, int operand)
{
    JSValue value = LLINT_OP_C(operand).jsValue();
    union {
        struct {
            uint32_t tag;
            uint32_t payload;
        } bits;
        EncodedJSValue asValue;
    } u;
    u.asValue = JSValue::encode(value);
    dataLogF(
        "%p / %p: executing bc#%zu, op#%u: Trace(%d): %d: %d: %08x:%08x: %s\n",
        exec->codeBlock(),
        exec,
        static_cast<intptr_t>(pc - exec->codeBlock()->instructions().begin()),
        exec->vm().interpreter->getOpcodeID(pc[0].u.opcode),
        fromWhere,
        operand,
        pc[operand].u.operand,
        u.bits.tag,
        u.bits.payload,
        toCString(value).data());
    LLINT_END_IMPL();
}

LLINT_SLOW_PATH_DECL(trace_prologue)
{
    dataLogF("%p / %p: in prologue.\n", exec->codeBlock(), exec);
    LLINT_END_IMPL();
}

static void traceFunctionPrologue(ExecState* exec, const char* comment, CodeSpecializationKind kind)
{
    JSFunction* callee = jsCast<JSFunction*>(exec->callee());
    FunctionExecutable* executable = callee->jsExecutable();
    CodeBlock* codeBlock = &executable->generatedBytecodeFor(kind);
    dataLogF("%p / %p: in %s of function %p, executable %p; numVars = %u, numParameters = %u, numCalleeRegisters = %u, caller = %p.\n",
            codeBlock, exec, comment, callee, executable,
            codeBlock->m_numVars, codeBlock->numParameters(), codeBlock->m_numCalleeRegisters,
            exec->callerFrame());
}

LLINT_SLOW_PATH_DECL(trace_prologue_function_for_call)
{
    traceFunctionPrologue(exec, "call prologue", CodeForCall);
    LLINT_END_IMPL();
}

LLINT_SLOW_PATH_DECL(trace_prologue_function_for_construct)
{
    traceFunctionPrologue(exec, "construct prologue", CodeForConstruct);
    LLINT_END_IMPL();
}

LLINT_SLOW_PATH_DECL(trace_arityCheck_for_call)
{
    traceFunctionPrologue(exec, "call arity check", CodeForCall);
    LLINT_END_IMPL();
}

LLINT_SLOW_PATH_DECL(trace_arityCheck_for_construct)
{
    traceFunctionPrologue(exec, "construct arity check", CodeForConstruct);
    LLINT_END_IMPL();
}

LLINT_SLOW_PATH_DECL(trace)
{
    dataLogF("%p / %p: executing bc#%zu, %s, scope %p\n",
            exec->codeBlock(),
            exec,
            static_cast<intptr_t>(pc - exec->codeBlock()->instructions().begin()),
            opcodeNames[exec->vm().interpreter->getOpcodeID(pc[0].u.opcode)],
            exec->scope());
    if (exec->vm().interpreter->getOpcodeID(pc[0].u.opcode) == op_ret) {
        dataLogF("Will be returning to %p\n", exec->returnPC().value());
        dataLogF("The new cfr will be %p\n", exec->callerFrame());
    }
    LLINT_END_IMPL();
}

LLINT_SLOW_PATH_DECL(special_trace)
{
    dataLogF("%p / %p: executing special case bc#%zu, op#%u, return PC is %p\n",
            exec->codeBlock(),
            exec,
            static_cast<intptr_t>(pc - exec->codeBlock()->instructions().begin()),
            exec->vm().interpreter->getOpcodeID(pc[0].u.opcode),
            exec->returnPC().value());
    LLINT_END_IMPL();
}

#if ENABLE(JIT)
inline bool shouldJIT(ExecState* exec)
{
    // You can modify this to turn off JITting without rebuilding the world.
    return exec->vm().canUseJIT();
}

// Returns true if we should try to OSR.
inline bool jitCompileAndSetHeuristics(CodeBlock* codeBlock, ExecState* exec)
{
    codeBlock->updateAllValueProfilePredictions();
    
    if (!codeBlock->checkIfJITThresholdReached()) {
#if ENABLE(JIT_VERBOSE_OSR)
        dataLogF("    JIT threshold should be lifted.\n");
#endif
        return false;
    }
        
    CodeBlock::JITCompilationResult result = codeBlock->jitCompile(exec);
    switch (result) {
    case CodeBlock::AlreadyCompiled:
#if ENABLE(JIT_VERBOSE_OSR)
        dataLogF("    Code was already compiled.\n");
#endif
        codeBlock->jitSoon();
        return true;
    case CodeBlock::CouldNotCompile:
#if ENABLE(JIT_VERBOSE_OSR)
        dataLogF("    JIT compilation failed.\n");
#endif
        codeBlock->dontJITAnytimeSoon();
        return false;
    case CodeBlock::CompiledSuccessfully:
#if ENABLE(JIT_VERBOSE_OSR)
        dataLogF("    JIT compilation successful.\n");
#endif
        codeBlock->jitSoon();
        return true;
    }
    RELEASE_ASSERT_NOT_REACHED();
    return false;
}

enum EntryKind { Prologue, ArityCheck };
static SlowPathReturnType entryOSR(ExecState* exec, Instruction*, CodeBlock* codeBlock, const char *name, EntryKind kind)
{
#if ENABLE(JIT_VERBOSE_OSR)
    dataLog(*codeBlock, ": Entered ", name, " with executeCounter = ", codeBlock->llintExecuteCounter(), "\n");
#else
    UNUSED_PARAM(name);
#endif
    
    if (!shouldJIT(exec)) {
        codeBlock->dontJITAnytimeSoon();
        LLINT_RETURN_TWO(0, exec);
    }
    if (!jitCompileAndSetHeuristics(codeBlock, exec))
        LLINT_RETURN_TWO(0, exec);
    
    if (kind == Prologue)
        LLINT_RETURN_TWO(codeBlock->getJITCode().executableAddressAtOffset(0), exec);
    ASSERT(kind == ArityCheck);
    LLINT_RETURN_TWO(codeBlock->getJITCodeWithArityCheck().executableAddress(), exec);
}

LLINT_SLOW_PATH_DECL(entry_osr)
{
    return entryOSR(exec, pc, exec->codeBlock(), "entry_osr", Prologue);
}

LLINT_SLOW_PATH_DECL(entry_osr_function_for_call)
{
    return entryOSR(exec, pc, &jsCast<JSFunction*>(exec->callee())->jsExecutable()->generatedBytecodeFor(CodeForCall), "entry_osr_function_for_call", Prologue);
}

LLINT_SLOW_PATH_DECL(entry_osr_function_for_construct)
{
    return entryOSR(exec, pc, &jsCast<JSFunction*>(exec->callee())->jsExecutable()->generatedBytecodeFor(CodeForConstruct), "entry_osr_function_for_construct", Prologue);
}

LLINT_SLOW_PATH_DECL(entry_osr_function_for_call_arityCheck)
{
    return entryOSR(exec, pc, &jsCast<JSFunction*>(exec->callee())->jsExecutable()->generatedBytecodeFor(CodeForCall), "entry_osr_function_for_call_arityCheck", ArityCheck);
}

LLINT_SLOW_PATH_DECL(entry_osr_function_for_construct_arityCheck)
{
    return entryOSR(exec, pc, &jsCast<JSFunction*>(exec->callee())->jsExecutable()->generatedBytecodeFor(CodeForConstruct), "entry_osr_function_for_construct_arityCheck", ArityCheck);
}

LLINT_SLOW_PATH_DECL(loop_osr)
{
    CodeBlock* codeBlock = exec->codeBlock();
    
#if ENABLE(JIT_VERBOSE_OSR)
    dataLog(*codeBlock, ": Entered loop_osr with executeCounter = ", codeBlock->llintExecuteCounter(), "\n");
#endif
    
    if (!shouldJIT(exec)) {
        codeBlock->dontJITAnytimeSoon();
        LLINT_RETURN_TWO(0, exec);
    }
    
    if (!jitCompileAndSetHeuristics(codeBlock, exec))
        LLINT_RETURN_TWO(0, exec);
    
    ASSERT(codeBlock->getJITType() == JITCode::BaselineJIT);
    
    Vector<BytecodeAndMachineOffset> map;
    codeBlock->jitCodeMap()->decode(map);
    BytecodeAndMachineOffset* mapping = binarySearch<BytecodeAndMachineOffset, unsigned>(map, map.size(), pc - codeBlock->instructions().begin(), BytecodeAndMachineOffset::getBytecodeIndex);
    ASSERT(mapping);
    ASSERT(mapping->m_bytecodeIndex == static_cast<unsigned>(pc - codeBlock->instructions().begin()));
    
    void* jumpTarget = codeBlock->getJITCode().executableAddressAtOffset(mapping->m_machineCodeOffset);
    ASSERT(jumpTarget);
    
    LLINT_RETURN_TWO(jumpTarget, exec);
}

LLINT_SLOW_PATH_DECL(replace)
{
    CodeBlock* codeBlock = exec->codeBlock();
    
#if ENABLE(JIT_VERBOSE_OSR)
    dataLog(*codeBlock, ": Entered replace with executeCounter = ", codeBlock->llintExecuteCounter(), "\n");
#endif
    
    if (shouldJIT(exec))
        jitCompileAndSetHeuristics(codeBlock, exec);
    else
        codeBlock->dontJITAnytimeSoon();
    LLINT_END_IMPL();
}
#endif // ENABLE(JIT)

LLINT_SLOW_PATH_DECL(stack_check)
{
    LLINT_BEGIN();
#if LLINT_SLOW_PATH_TRACING
    dataLogF("Checking stack height with exec = %p.\n", exec);
    dataLogF("CodeBlock = %p.\n", exec->codeBlock());
    dataLogF("Num callee registers = %u.\n", exec->codeBlock()->m_numCalleeRegisters);
    dataLogF("Num vars = %u.\n", exec->codeBlock()->m_numVars);
    dataLogF("Current end is at %p.\n", exec->vm().interpreter->stack().end());
#endif
    ASSERT(&exec->registers()[exec->codeBlock()->m_numCalleeRegisters] > exec->vm().interpreter->stack().end());
    if (UNLIKELY(!vm.interpreter->stack().grow(&exec->registers()[exec->codeBlock()->m_numCalleeRegisters]))) {
        ReturnAddressPtr returnPC = exec->returnPC();
        exec = exec->callerFrame();
        vm.exception = createStackOverflowError(exec);
        interpreterThrowInCaller(exec, returnPC);
        pc = returnToThrowForThrownException(exec);
    }
    LLINT_END_IMPL();
}

LLINT_SLOW_PATH_DECL(slow_path_call_arityCheck)
{
    LLINT_BEGIN();
    ExecState* newExec = CommonSlowPaths::arityCheckFor(exec, &vm.interpreter->stack(), CodeForCall);
    if (!newExec) {
        ReturnAddressPtr returnPC = exec->returnPC();
        exec = exec->callerFrame();
        vm.exception = createStackOverflowError(exec);
        interpreterThrowInCaller(exec, returnPC);
        LLINT_RETURN_TWO(bitwise_cast<void*>(static_cast<uintptr_t>(1)), exec);
    }
    LLINT_RETURN_TWO(0, newExec);
}

LLINT_SLOW_PATH_DECL(slow_path_construct_arityCheck)
{
    LLINT_BEGIN();
    ExecState* newExec = CommonSlowPaths::arityCheckFor(exec, &vm.interpreter->stack(), CodeForConstruct);
    if (!newExec) {
        ReturnAddressPtr returnPC = exec->returnPC();
        exec = exec->callerFrame();
        vm.exception = createStackOverflowError(exec);
        interpreterThrowInCaller(exec, returnPC);
        LLINT_RETURN_TWO(bitwise_cast<void*>(static_cast<uintptr_t>(1)), exec);
    }
    LLINT_RETURN_TWO(0, newExec);
}

LLINT_SLOW_PATH_DECL(slow_path_create_activation)
{
    LLINT_BEGIN();
#if LLINT_SLOW_PATH_TRACING
    dataLogF("Creating an activation, exec = %p!\n", exec);
#endif
    JSActivation* activation = JSActivation::create(vm, exec, exec->codeBlock());
    exec->setScope(activation);
    LLINT_RETURN(JSValue(activation));
}

LLINT_SLOW_PATH_DECL(slow_path_create_arguments)
{
    LLINT_BEGIN();
    JSValue arguments = JSValue(Arguments::create(vm, exec));
    LLINT_CHECK_EXCEPTION();
    exec->uncheckedR(pc[1].u.operand) = arguments;
    exec->uncheckedR(unmodifiedArgumentsRegister(pc[1].u.operand)) = arguments;
    LLINT_END();
}

LLINT_SLOW_PATH_DECL(slow_path_create_this)
{
    LLINT_BEGIN();
    JSFunction* constructor = jsCast<JSFunction*>(LLINT_OP(2).jsValue().asCell());
    
#if !ASSERT_DISABLED
    ConstructData constructData;
    ASSERT(constructor->methodTable()->getConstructData(constructor, constructData) == ConstructTypeJS);
#endif

    size_t inlineCapacity = pc[3].u.operand;
    Structure* structure = constructor->allocationProfile(exec, inlineCapacity)->structure();
    LLINT_RETURN(constructEmptyObject(exec, structure));
}

LLINT_SLOW_PATH_DECL(slow_path_convert_this)
{
    LLINT_BEGIN();
    JSValue v1 = LLINT_OP(1).jsValue();
    ASSERT(v1.isPrimitive());
#if ENABLE(VALUE_PROFILER)
    pc[OPCODE_LENGTH(op_convert_this) - 1].u.profile->m_buckets[0] =
        JSValue::encode(v1.structureOrUndefined());
#endif
    LLINT_RETURN(v1.toThisObject(exec));
}

LLINT_SLOW_PATH_DECL(slow_path_new_object)
{
    LLINT_BEGIN();
    LLINT_RETURN(constructEmptyObject(exec, pc[3].u.objectAllocationProfile->structure()));
}

LLINT_SLOW_PATH_DECL(slow_path_new_array)
{
    LLINT_BEGIN();
    LLINT_RETURN(constructArray(exec, pc[4].u.arrayAllocationProfile, bitwise_cast<JSValue*>(&LLINT_OP(2)), pc[3].u.operand));
}

LLINT_SLOW_PATH_DECL(slow_path_new_array_with_size)
{
    LLINT_BEGIN();
    LLINT_RETURN(constructArrayWithSizeQuirk(exec, pc[3].u.arrayAllocationProfile, exec->lexicalGlobalObject(), LLINT_OP_C(2).jsValue()));
}

LLINT_SLOW_PATH_DECL(slow_path_new_array_buffer)
{
    LLINT_BEGIN();
    LLINT_RETURN(constructArray(exec, pc[4].u.arrayAllocationProfile, exec->codeBlock()->constantBuffer(pc[2].u.operand), pc[3].u.operand));
}

LLINT_SLOW_PATH_DECL(slow_path_new_regexp)
{
    LLINT_BEGIN();
    RegExp* regExp = exec->codeBlock()->regexp(pc[2].u.operand);
    if (!regExp->isValid())
        LLINT_THROW(createSyntaxError(exec, "Invalid flag supplied to RegExp constructor."));
    LLINT_RETURN(RegExpObject::create(vm, exec->lexicalGlobalObject(), exec->lexicalGlobalObject()->regExpStructure(), regExp));
}

LLINT_SLOW_PATH_DECL(slow_path_not)
{
    LLINT_BEGIN();
    LLINT_RETURN(jsBoolean(!LLINT_OP_C(2).jsValue().toBoolean(exec)));
}

LLINT_SLOW_PATH_DECL(slow_path_eq)
{
    LLINT_BEGIN();
    LLINT_RETURN(jsBoolean(JSValue::equal(exec, LLINT_OP_C(2).jsValue(), LLINT_OP_C(3).jsValue())));
}

LLINT_SLOW_PATH_DECL(slow_path_neq)
{
    LLINT_BEGIN();
    LLINT_RETURN(jsBoolean(!JSValue::equal(exec, LLINT_OP_C(2).jsValue(), LLINT_OP_C(3).jsValue())));
}

LLINT_SLOW_PATH_DECL(slow_path_stricteq)
{
    LLINT_BEGIN();
    LLINT_RETURN(jsBoolean(JSValue::strictEqual(exec, LLINT_OP_C(2).jsValue(), LLINT_OP_C(3).jsValue())));
}

LLINT_SLOW_PATH_DECL(slow_path_nstricteq)
{
    LLINT_BEGIN();
    LLINT_RETURN(jsBoolean(!JSValue::strictEqual(exec, LLINT_OP_C(2).jsValue(), LLINT_OP_C(3).jsValue())));
}

LLINT_SLOW_PATH_DECL(slow_path_less)
{
    LLINT_BEGIN();
    LLINT_RETURN(jsBoolean(jsLess<true>(exec, LLINT_OP_C(2).jsValue(), LLINT_OP_C(3).jsValue())));
}

LLINT_SLOW_PATH_DECL(slow_path_lesseq)
{
    LLINT_BEGIN();
    LLINT_RETURN(jsBoolean(jsLessEq<true>(exec, LLINT_OP_C(2).jsValue(), LLINT_OP_C(3).jsValue())));
}

LLINT_SLOW_PATH_DECL(slow_path_greater)
{
    LLINT_BEGIN();
    LLINT_RETURN(jsBoolean(jsLess<false>(exec, LLINT_OP_C(3).jsValue(), LLINT_OP_C(2).jsValue())));
}

LLINT_SLOW_PATH_DECL(slow_path_greatereq)
{
    LLINT_BEGIN();
    LLINT_RETURN(jsBoolean(jsLessEq<false>(exec, LLINT_OP_C(3).jsValue(), LLINT_OP_C(2).jsValue())));
}

LLINT_SLOW_PATH_DECL(slow_path_pre_inc)
{
    LLINT_BEGIN();
    LLINT_RETURN(jsNumber(LLINT_OP(1).jsValue().toNumber(exec) + 1));
}

LLINT_SLOW_PATH_DECL(slow_path_pre_dec)
{
    LLINT_BEGIN();
    LLINT_RETURN(jsNumber(LLINT_OP(1).jsValue().toNumber(exec) - 1));
}

LLINT_SLOW_PATH_DECL(slow_path_to_number)
{
    LLINT_BEGIN();
    LLINT_RETURN(jsNumber(LLINT_OP_C(2).jsValue().toNumber(exec)));
}

LLINT_SLOW_PATH_DECL(slow_path_negate)
{
    LLINT_BEGIN();
    LLINT_RETURN(jsNumber(-LLINT_OP_C(2).jsValue().toNumber(exec)));
}

LLINT_SLOW_PATH_DECL(slow_path_add)
{
    LLINT_BEGIN();
    JSValue v1 = LLINT_OP_C(2).jsValue();
    JSValue v2 = LLINT_OP_C(3).jsValue();
    
#if LLINT_SLOW_PATH_TRACING
    dataLog("Trying to add ", v1, " to ", v2, ".\n");
#endif
    
    if (v1.isString() && !v2.isObject())
        LLINT_RETURN(jsString(exec, asString(v1), v2.toString(exec)));
    
    if (v1.isNumber() && v2.isNumber())
        LLINT_RETURN(jsNumber(v1.asNumber() + v2.asNumber()));
    
    LLINT_RETURN(jsAddSlowCase(exec, v1, v2));
}

// The following arithmetic and bitwise operations need to be sure to run
// toNumber() on their operands in order.  (A call to toNumber() is idempotent
// if an exception is already set on the ExecState.)

LLINT_SLOW_PATH_DECL(slow_path_mul)
{
    LLINT_BEGIN();
    double a = LLINT_OP_C(2).jsValue().toNumber(exec);
    double b = LLINT_OP_C(3).jsValue().toNumber(exec);
    LLINT_RETURN(jsNumber(a * b));
}

LLINT_SLOW_PATH_DECL(slow_path_sub)
{
    LLINT_BEGIN();
    double a = LLINT_OP_C(2).jsValue().toNumber(exec);
    double b = LLINT_OP_C(3).jsValue().toNumber(exec);
    LLINT_RETURN(jsNumber(a - b));
}

LLINT_SLOW_PATH_DECL(slow_path_div)
{
    LLINT_BEGIN();
    double a = LLINT_OP_C(2).jsValue().toNumber(exec);
    double b = LLINT_OP_C(3).jsValue().toNumber(exec);
    LLINT_RETURN(jsNumber(a / b));
}

LLINT_SLOW_PATH_DECL(slow_path_mod)
{
    LLINT_BEGIN();
    double a = LLINT_OP_C(2).jsValue().toNumber(exec);
    double b = LLINT_OP_C(3).jsValue().toNumber(exec);
    LLINT_RETURN(jsNumber(fmod(a, b)));
}

LLINT_SLOW_PATH_DECL(slow_path_lshift)
{
    LLINT_BEGIN();
    int32_t a = LLINT_OP_C(2).jsValue().toInt32(exec);
    uint32_t b = LLINT_OP_C(3).jsValue().toUInt32(exec);
    LLINT_RETURN(jsNumber(a << (b & 31)));
}

LLINT_SLOW_PATH_DECL(slow_path_rshift)
{
    LLINT_BEGIN();
    int32_t a = LLINT_OP_C(2).jsValue().toInt32(exec);
    uint32_t b = LLINT_OP_C(3).jsValue().toUInt32(exec);
    LLINT_RETURN(jsNumber(a >> (b & 31)));
}

LLINT_SLOW_PATH_DECL(slow_path_urshift)
{
    LLINT_BEGIN();
    uint32_t a = LLINT_OP_C(2).jsValue().toUInt32(exec);
    uint32_t b = LLINT_OP_C(3).jsValue().toUInt32(exec);
    LLINT_RETURN(jsNumber(a >> (b & 31)));
}

LLINT_SLOW_PATH_DECL(slow_path_bitand)
{
    LLINT_BEGIN();
    int32_t a = LLINT_OP_C(2).jsValue().toInt32(exec);
    int32_t b = LLINT_OP_C(3).jsValue().toInt32(exec);
    LLINT_RETURN(jsNumber(a & b));
}

LLINT_SLOW_PATH_DECL(slow_path_bitor)
{
    LLINT_BEGIN();
    int32_t a = LLINT_OP_C(2).jsValue().toInt32(exec);
    int32_t b = LLINT_OP_C(3).jsValue().toInt32(exec);
    LLINT_RETURN(jsNumber(a | b));
}

LLINT_SLOW_PATH_DECL(slow_path_bitxor)
{
    LLINT_BEGIN();
    int32_t a = LLINT_OP_C(2).jsValue().toInt32(exec);
    int32_t b = LLINT_OP_C(3).jsValue().toInt32(exec);
    LLINT_RETURN(jsNumber(a ^ b));
}

LLINT_SLOW_PATH_DECL(slow_path_check_has_instance)
{
    LLINT_BEGIN();
    
    JSValue value = LLINT_OP_C(2).jsValue();
    JSValue baseVal = LLINT_OP_C(3).jsValue();
    if (baseVal.isObject()) {
        JSObject* baseObject = asObject(baseVal);
        ASSERT(!baseObject->structure()->typeInfo().implementsDefaultHasInstance());
        if (baseObject->structure()->typeInfo().implementsHasInstance()) {
            pc += pc[4].u.operand;
            LLINT_RETURN(jsBoolean(baseObject->methodTable()->customHasInstance(baseObject, exec, value)));
        }
    }
    LLINT_THROW(createInvalidParameterError(exec, "instanceof", baseVal));
}

LLINT_SLOW_PATH_DECL(slow_path_instanceof)
{
    LLINT_BEGIN();
    JSValue value = LLINT_OP_C(2).jsValue();
    JSValue proto = LLINT_OP_C(3).jsValue();
    ASSERT(!value.isObject() || !proto.isObject());
    LLINT_RETURN(jsBoolean(JSObject::defaultHasInstance(exec, value, proto)));
}

LLINT_SLOW_PATH_DECL(slow_path_typeof)
{
    LLINT_BEGIN();
    LLINT_RETURN(jsTypeStringForValue(exec, LLINT_OP_C(2).jsValue()));
}

LLINT_SLOW_PATH_DECL(slow_path_is_object)
{
    LLINT_BEGIN();
    LLINT_RETURN(jsBoolean(jsIsObjectType(exec, LLINT_OP_C(2).jsValue())));
}

LLINT_SLOW_PATH_DECL(slow_path_is_function)
{
    LLINT_BEGIN();
    LLINT_RETURN(jsBoolean(jsIsFunctionType(LLINT_OP_C(2).jsValue())));
}

LLINT_SLOW_PATH_DECL(slow_path_in)
{
    LLINT_BEGIN();
    LLINT_RETURN(jsBoolean(CommonSlowPaths::opIn(exec, LLINT_OP_C(2).jsValue(), LLINT_OP_C(3).jsValue())));
}

LLINT_SLOW_PATH_DECL(slow_path_resolve)
{
    LLINT_BEGIN();
    Identifier ident = exec->codeBlock()->identifier(pc[2].u.operand);
    ResolveOperations* operations = pc[3].u.resolveOperations;
    JSValue result = JSScope::resolve(exec, ident, operations);
    ASSERT(operations->size());
    if (operations->isEmpty())
        LLINT_RETURN_PROFILED(op_resolve, result);

    switch (operations->data()[0].m_operation) {
    case ResolveOperation::GetAndReturnGlobalProperty:
        pc[0].u.opcode = LLInt::getOpcode(llint_op_resolve_global_property);
        break;

    case ResolveOperation::GetAndReturnGlobalVar:
        pc[0].u.opcode = LLInt::getOpcode(llint_op_resolve_global_var);
        break;

    case ResolveOperation::SkipTopScopeNode:
        pc[0].u.opcode = LLInt::getOpcode(llint_op_resolve_scoped_var_with_top_scope_check);
        break;

    case ResolveOperation::SkipScopes:
        if (operations->data()[0].m_scopesToSkip)
            pc[0].u.opcode = LLInt::getOpcode(llint_op_resolve_scoped_var);
        else
            pc[0].u.opcode = LLInt::getOpcode(llint_op_resolve_scoped_var_on_top_scope);
        break;

    default:
        break;
    }
    LLINT_RETURN_PROFILED(op_resolve, result);
}

LLINT_SLOW_PATH_DECL(slow_path_put_to_base)
{
    LLINT_BEGIN();
    PutToBaseOperation* operation = pc[4].u.putToBaseOperation;
    JSScope::resolvePut(exec, LLINT_OP_C(1).jsValue(), exec->codeBlock()->identifier(pc[2].u.operand), LLINT_OP_C(3).jsValue(), operation);
    switch (operation->m_kind) {
    case PutToBaseOperation::VariablePut:
        pc[0].u.opcode = LLInt::getOpcode(llint_op_put_to_base_variable);
        break;

    default:
        break;
    }
    LLINT_END();
}

LLINT_SLOW_PATH_DECL(slow_path_resolve_base)
{
    LLINT_BEGIN();
    Identifier& ident = exec->codeBlock()->identifier(pc[2].u.operand);
    ResolveOperations* operations = pc[4].u.resolveOperations;
    JSValue result;
    if (pc[3].u.operand) {
        result = JSScope::resolveBase(exec, ident, true, operations, pc[5].u.putToBaseOperation);
        if (!result)
            LLINT_THROW(vm.exception);
    } else
        result = JSScope::resolveBase(exec, ident, false, operations, pc[5].u.putToBaseOperation);

    ASSERT(operations->size());
    if (operations->isEmpty()) {
        LLINT_PROFILE_VALUE(op_resolve_base, result);
        LLINT_RETURN(result);
    }

    switch (operations->data()[0].m_operation) {
    case ResolveOperation::ReturnGlobalObjectAsBase:
        pc[0].u.opcode = LLInt::getOpcode(llint_op_resolve_base_to_global);
        break;

    case ResolveOperation::SkipTopScopeNode:
        pc[0].u.opcode = LLInt::getOpcode(llint_op_resolve_base_to_scope_with_top_scope_check);
        break;

    case ResolveOperation::SkipScopes:
        pc[0].u.opcode = LLInt::getOpcode(llint_op_resolve_base_to_scope);
        break;

    default:
        break;
    }
    LLINT_PROFILE_VALUE(op_resolve_base, result);
    LLINT_RETURN(result);
}

LLINT_SLOW_PATH_DECL(slow_path_resolve_with_base)
{
    LLINT_BEGIN();
    ResolveOperations* operations = pc[4].u.resolveOperations;
    JSValue result = JSScope::resolveWithBase(exec, exec->codeBlock()->identifier(pc[3].u.operand), &LLINT_OP(1), operations, pc[5].u.putToBaseOperation);
    LLINT_CHECK_EXCEPTION();
    LLINT_OP(2) = result;
    LLINT_PROFILE_VALUE(op_resolve_with_base, result);
    LLINT_END();
}

LLINT_SLOW_PATH_DECL(slow_path_resolve_with_this)
{
    LLINT_BEGIN();
    ResolveOperations* operations = pc[4].u.resolveOperations;
    JSValue result = JSScope::resolveWithThis(exec, exec->codeBlock()->identifier(pc[3].u.operand), &LLINT_OP(1), operations);
    LLINT_CHECK_EXCEPTION();
    LLINT_OP(2) = result;
    LLINT_PROFILE_VALUE(op_resolve_with_this, result);
    LLINT_END();
}

LLINT_SLOW_PATH_DECL(slow_path_init_global_const_check)
{
    LLINT_BEGIN();
    CodeBlock* codeBlock = exec->codeBlock();
    symbolTablePut(codeBlock->globalObject(), exec, codeBlock->identifier(pc[4].u.operand), LLINT_OP_C(2).jsValue(), true);
    LLINT_END();
}

LLINT_SLOW_PATH_DECL(slow_path_get_by_id)
{
    LLINT_BEGIN();
    CodeBlock* codeBlock = exec->codeBlock();
    Identifier& ident = codeBlock->identifier(pc[3].u.operand);
    JSValue baseValue = LLINT_OP_C(2).jsValue();
    PropertySlot slot(baseValue);

    JSValue result = baseValue.get(exec, ident, slot);
    LLINT_CHECK_EXCEPTION();
    LLINT_OP(1) = result;
    
    if (!LLINT_ALWAYS_ACCESS_SLOW
        && baseValue.isCell()
        && slot.isCacheable()
        && slot.slotBase() == baseValue
        && slot.cachedPropertyType() == PropertySlot::Value) {
        
        JSCell* baseCell = baseValue.asCell();
        Structure* structure = baseCell->structure();
        
        if (!structure->isUncacheableDictionary()
            && !structure->typeInfo().prohibitsPropertyCaching()) {
            pc[4].u.structure.set(
                vm, codeBlock->ownerExecutable(), structure);
            if (isInlineOffset(slot.cachedOffset())) {
                pc[0].u.opcode = LLInt::getOpcode(llint_op_get_by_id);
                pc[5].u.operand = offsetInInlineStorage(slot.cachedOffset()) * sizeof(JSValue) + JSObject::offsetOfInlineStorage();
            } else {
                pc[0].u.opcode = LLInt::getOpcode(llint_op_get_by_id_out_of_line);
                pc[5].u.operand = offsetInButterfly(slot.cachedOffset()) * sizeof(JSValue);
            }
        }
    }

    if (!LLINT_ALWAYS_ACCESS_SLOW
        && isJSArray(baseValue)
        && ident == exec->propertyNames().length) {
        pc[0].u.opcode = LLInt::getOpcode(llint_op_get_array_length);
#if ENABLE(VALUE_PROFILER)
        ArrayProfile* arrayProfile = codeBlock->getOrAddArrayProfile(pc - codeBlock->instructions().begin());
        arrayProfile->observeStructure(baseValue.asCell()->structure());
        pc[4].u.arrayProfile = arrayProfile;
#endif
    }

#if ENABLE(VALUE_PROFILER)    
    pc[OPCODE_LENGTH(op_get_by_id) - 1].u.profile->m_buckets[0] = JSValue::encode(result);
#endif
    LLINT_END();
}

LLINT_SLOW_PATH_DECL(slow_path_get_arguments_length)
{
    LLINT_BEGIN();
    CodeBlock* codeBlock = exec->codeBlock();
    Identifier& ident = codeBlock->identifier(pc[3].u.operand);
    JSValue baseValue = LLINT_OP(2).jsValue();
    PropertySlot slot(baseValue);
    LLINT_RETURN(baseValue.get(exec, ident, slot));
}

LLINT_SLOW_PATH_DECL(slow_path_put_by_id)
{
    LLINT_BEGIN();
    CodeBlock* codeBlock = exec->codeBlock();
    Identifier& ident = codeBlock->identifier(pc[2].u.operand);
    
    JSValue baseValue = LLINT_OP_C(1).jsValue();
    PutPropertySlot slot(codeBlock->isStrictMode());
    if (pc[8].u.operand)
        asObject(baseValue)->putDirect(vm, ident, LLINT_OP_C(3).jsValue(), slot);
    else
        baseValue.put(exec, ident, LLINT_OP_C(3).jsValue(), slot);
    LLINT_CHECK_EXCEPTION();
    
    if (!LLINT_ALWAYS_ACCESS_SLOW
        && baseValue.isCell()
        && slot.isCacheable()) {
        
        JSCell* baseCell = baseValue.asCell();
        Structure* structure = baseCell->structure();
        
        if (!structure->isUncacheableDictionary()
            && !structure->typeInfo().prohibitsPropertyCaching()
            && baseCell == slot.base()) {
            
            if (slot.type() == PutPropertySlot::NewProperty) {
                if (!structure->isDictionary() && structure->previousID()->outOfLineCapacity() == structure->outOfLineCapacity()) {
                    ASSERT(structure->previousID()->transitionWatchpointSetHasBeenInvalidated());
                    
                    // This is needed because some of the methods we call
                    // below may GC.
                    pc[0].u.opcode = LLInt::getOpcode(llint_op_put_by_id);

                    if (normalizePrototypeChain(exec, baseCell) != InvalidPrototypeChain) {
                        ASSERT(structure->previousID()->isObject());
                        pc[4].u.structure.set(
                            vm, codeBlock->ownerExecutable(), structure->previousID());
                        if (isInlineOffset(slot.cachedOffset()))
                            pc[5].u.operand = offsetInInlineStorage(slot.cachedOffset()) * sizeof(JSValue) + JSObject::offsetOfInlineStorage();
                        else
                            pc[5].u.operand = offsetInButterfly(slot.cachedOffset()) * sizeof(JSValue);
                        pc[6].u.structure.set(
                            vm, codeBlock->ownerExecutable(), structure);
                        StructureChain* chain = structure->prototypeChain(exec);
                        ASSERT(chain);
                        pc[7].u.structureChain.set(
                            vm, codeBlock->ownerExecutable(), chain);
                    
                        if (pc[8].u.operand) {
                            if (isInlineOffset(slot.cachedOffset()))
                                pc[0].u.opcode = LLInt::getOpcode(llint_op_put_by_id_transition_direct);
                            else
                                pc[0].u.opcode = LLInt::getOpcode(llint_op_put_by_id_transition_direct_out_of_line);
                        } else {
                            if (isInlineOffset(slot.cachedOffset()))
                                pc[0].u.opcode = LLInt::getOpcode(llint_op_put_by_id_transition_normal);
                            else
                                pc[0].u.opcode = LLInt::getOpcode(llint_op_put_by_id_transition_normal_out_of_line);
                        }
                    }
                }
            } else {
                pc[4].u.structure.set(
                    vm, codeBlock->ownerExecutable(), structure);
                if (isInlineOffset(slot.cachedOffset())) {
                    pc[0].u.opcode = LLInt::getOpcode(llint_op_put_by_id);
                    pc[5].u.operand = offsetInInlineStorage(slot.cachedOffset()) * sizeof(JSValue) + JSObject::offsetOfInlineStorage();
                } else {
                    pc[0].u.opcode = LLInt::getOpcode(llint_op_put_by_id_out_of_line);
                    pc[5].u.operand = offsetInButterfly(slot.cachedOffset()) * sizeof(JSValue);
                }
            }
        }
    }
    
    LLINT_END();
}

LLINT_SLOW_PATH_DECL(slow_path_del_by_id)
{
    LLINT_BEGIN();
    CodeBlock* codeBlock = exec->codeBlock();
    JSObject* baseObject = LLINT_OP_C(2).jsValue().toObject(exec);
    bool couldDelete = baseObject->methodTable()->deleteProperty(baseObject, exec, codeBlock->identifier(pc[3].u.operand));
    LLINT_CHECK_EXCEPTION();
    if (!couldDelete && codeBlock->isStrictMode())
        LLINT_THROW(createTypeError(exec, "Unable to delete property."));
    LLINT_RETURN(jsBoolean(couldDelete));
}

inline JSValue getByVal(ExecState* exec, JSValue baseValue, JSValue subscript)
{
    if (LIKELY(baseValue.isCell() && subscript.isString())) {
        if (JSValue result = baseValue.asCell()->fastGetOwnProperty(exec, asString(subscript)->value(exec)))
            return result;
    }
    
    if (subscript.isUInt32()) {
        uint32_t i = subscript.asUInt32();
        if (isJSString(baseValue) && asString(baseValue)->canGetIndex(i))
            return asString(baseValue)->getIndex(exec, i);
        
        return baseValue.get(exec, i);
    }

    if (isName(subscript))
        return baseValue.get(exec, jsCast<NameInstance*>(subscript.asCell())->privateName());
    
    Identifier property(exec, subscript.toString(exec)->value(exec));
    return baseValue.get(exec, property);
}

LLINT_SLOW_PATH_DECL(slow_path_get_by_val)
{
    LLINT_BEGIN();
    LLINT_RETURN_PROFILED(op_get_by_val, getByVal(exec, LLINT_OP_C(2).jsValue(), LLINT_OP_C(3).jsValue()));
}

LLINT_SLOW_PATH_DECL(slow_path_get_argument_by_val)
{
    LLINT_BEGIN();
    JSValue arguments = LLINT_OP(2).jsValue();
    if (!arguments) {
        arguments = Arguments::create(vm, exec);
        LLINT_CHECK_EXCEPTION();
        LLINT_OP(2) = arguments;
        exec->uncheckedR(unmodifiedArgumentsRegister(pc[2].u.operand)) = arguments;
    }
    
    LLINT_RETURN_PROFILED(op_get_argument_by_val, getByVal(exec, arguments, LLINT_OP_C(3).jsValue()));
}

LLINT_SLOW_PATH_DECL(slow_path_get_by_pname)
{
    LLINT_BEGIN();
    LLINT_RETURN(getByVal(exec, LLINT_OP_C(2).jsValue(), LLINT_OP_C(3).jsValue()));
}

LLINT_SLOW_PATH_DECL(slow_path_put_by_val)
{
    LLINT_BEGIN();
    
    JSValue baseValue = LLINT_OP_C(1).jsValue();
    JSValue subscript = LLINT_OP_C(2).jsValue();
    JSValue value = LLINT_OP_C(3).jsValue();
    
    if (LIKELY(subscript.isUInt32())) {
        uint32_t i = subscript.asUInt32();
        if (baseValue.isObject()) {
            JSObject* object = asObject(baseValue);
            if (object->canSetIndexQuickly(i))
                object->setIndexQuickly(vm, i, value);
            else
                object->methodTable()->putByIndex(object, exec, i, value, exec->codeBlock()->isStrictMode());
            LLINT_END();
        }
        baseValue.putByIndex(exec, i, value, exec->codeBlock()->isStrictMode());
        LLINT_END();
    }

    if (isName(subscript)) {
        PutPropertySlot slot(exec->codeBlock()->isStrictMode());
        baseValue.put(exec, jsCast<NameInstance*>(subscript.asCell())->privateName(), value, slot);
        LLINT_END();
    }

    Identifier property(exec, subscript.toString(exec)->value(exec));
    LLINT_CHECK_EXCEPTION();
    PutPropertySlot slot(exec->codeBlock()->isStrictMode());
    baseValue.put(exec, property, value, slot);
    LLINT_END();
}

LLINT_SLOW_PATH_DECL(slow_path_del_by_val)
{
    LLINT_BEGIN();
    JSValue baseValue = LLINT_OP_C(2).jsValue();
    JSObject* baseObject = baseValue.toObject(exec);
    
    JSValue subscript = LLINT_OP_C(3).jsValue();
    
    bool couldDelete;
    
    uint32_t i;
    if (subscript.getUInt32(i))
        couldDelete = baseObject->methodTable()->deletePropertyByIndex(baseObject, exec, i);
    else if (isName(subscript))
        couldDelete = baseObject->methodTable()->deleteProperty(baseObject, exec, jsCast<NameInstance*>(subscript.asCell())->privateName());
    else {
        LLINT_CHECK_EXCEPTION();
        Identifier property(exec, subscript.toString(exec)->value(exec));
        LLINT_CHECK_EXCEPTION();
        couldDelete = baseObject->methodTable()->deleteProperty(baseObject, exec, property);
    }
    
    if (!couldDelete && exec->codeBlock()->isStrictMode())
        LLINT_THROW(createTypeError(exec, "Unable to delete property."));
    
    LLINT_RETURN(jsBoolean(couldDelete));
}

LLINT_SLOW_PATH_DECL(slow_path_put_by_index)
{
    LLINT_BEGIN();
    JSValue arrayValue = LLINT_OP_C(1).jsValue();
    ASSERT(isJSArray(arrayValue));
    asArray(arrayValue)->putDirectIndex(exec, pc[2].u.operand, LLINT_OP_C(3).jsValue());
    LLINT_END();
}

LLINT_SLOW_PATH_DECL(slow_path_put_getter_setter)
{
    LLINT_BEGIN();
    ASSERT(LLINT_OP(1).jsValue().isObject());
    JSObject* baseObj = asObject(LLINT_OP(1).jsValue());
    
    GetterSetter* accessor = GetterSetter::create(exec);
    LLINT_CHECK_EXCEPTION();
    
    JSValue getter = LLINT_OP(3).jsValue();
    JSValue setter = LLINT_OP(4).jsValue();
    ASSERT(getter.isObject() || getter.isUndefined());
    ASSERT(setter.isObject() || setter.isUndefined());
    ASSERT(getter.isObject() || setter.isObject());
    
    if (!getter.isUndefined())
        accessor->setGetter(vm, asObject(getter));
    if (!setter.isUndefined())
        accessor->setSetter(vm, asObject(setter));
    baseObj->putDirectAccessor(
        exec,
        exec->codeBlock()->identifier(pc[2].u.operand),
        accessor, Accessor);
    LLINT_END();
}

LLINT_SLOW_PATH_DECL(slow_path_jtrue)
{
    LLINT_BEGIN();
    LLINT_BRANCH(op_jtrue, LLINT_OP_C(1).jsValue().toBoolean(exec));
}

LLINT_SLOW_PATH_DECL(slow_path_jfalse)
{
    LLINT_BEGIN();
    LLINT_BRANCH(op_jfalse, !LLINT_OP_C(1).jsValue().toBoolean(exec));
}

LLINT_SLOW_PATH_DECL(slow_path_jless)
{
    LLINT_BEGIN();
    LLINT_BRANCH(op_jless, jsLess<true>(exec, LLINT_OP_C(1).jsValue(), LLINT_OP_C(2).jsValue()));
}

LLINT_SLOW_PATH_DECL(slow_path_jnless)
{
    LLINT_BEGIN();
    LLINT_BRANCH(op_jnless, !jsLess<true>(exec, LLINT_OP_C(1).jsValue(), LLINT_OP_C(2).jsValue()));
}

LLINT_SLOW_PATH_DECL(slow_path_jgreater)
{
    LLINT_BEGIN();
    LLINT_BRANCH(op_jgreater, jsLess<false>(exec, LLINT_OP_C(2).jsValue(), LLINT_OP_C(1).jsValue()));
}

LLINT_SLOW_PATH_DECL(slow_path_jngreater)
{
    LLINT_BEGIN();
    LLINT_BRANCH(op_jngreater, !jsLess<false>(exec, LLINT_OP_C(2).jsValue(), LLINT_OP_C(1).jsValue()));
}

LLINT_SLOW_PATH_DECL(slow_path_jlesseq)
{
    LLINT_BEGIN();
    LLINT_BRANCH(op_jlesseq, jsLessEq<true>(exec, LLINT_OP_C(1).jsValue(), LLINT_OP_C(2).jsValue()));
}

LLINT_SLOW_PATH_DECL(slow_path_jnlesseq)
{
    LLINT_BEGIN();
    LLINT_BRANCH(op_jnlesseq, !jsLessEq<true>(exec, LLINT_OP_C(1).jsValue(), LLINT_OP_C(2).jsValue()));
}

LLINT_SLOW_PATH_DECL(slow_path_jgreatereq)
{
    LLINT_BEGIN();
    LLINT_BRANCH(op_jgreatereq, jsLessEq<false>(exec, LLINT_OP_C(2).jsValue(), LLINT_OP_C(1).jsValue()));
}

LLINT_SLOW_PATH_DECL(slow_path_jngreatereq)
{
    LLINT_BEGIN();
    LLINT_BRANCH(op_jngreatereq, !jsLessEq<false>(exec, LLINT_OP_C(2).jsValue(), LLINT_OP_C(1).jsValue()));
}

LLINT_SLOW_PATH_DECL(slow_path_switch_imm)
{
    LLINT_BEGIN();
    JSValue scrutinee = LLINT_OP_C(3).jsValue();
    ASSERT(scrutinee.isDouble());
    double value = scrutinee.asDouble();
    int32_t intValue = static_cast<int32_t>(value);
    int defaultOffset = pc[2].u.operand;
    if (value == intValue) {
        CodeBlock* codeBlock = exec->codeBlock();
        pc += codeBlock->immediateSwitchJumpTable(pc[1].u.operand).offsetForValue(intValue, defaultOffset);
    } else
        pc += defaultOffset;
    LLINT_END();
}

LLINT_SLOW_PATH_DECL(slow_path_switch_char)
{
    LLINT_BEGIN();
    JSValue scrutinee = LLINT_OP_C(3).jsValue();
    ASSERT(scrutinee.isString());
    JSString* string = asString(scrutinee);
    ASSERT(string->length() == 1);
    int defaultOffset = pc[2].u.operand;
    StringImpl* impl = string->value(exec).impl();
    CodeBlock* codeBlock = exec->codeBlock();
    pc += codeBlock->characterSwitchJumpTable(pc[1].u.operand).offsetForValue((*impl)[0], defaultOffset);
    LLINT_END();
}

LLINT_SLOW_PATH_DECL(slow_path_switch_string)
{
    LLINT_BEGIN();
    JSValue scrutinee = LLINT_OP_C(3).jsValue();
    int defaultOffset = pc[2].u.operand;
    if (!scrutinee.isString())
        pc += defaultOffset;
    else {
        CodeBlock* codeBlock = exec->codeBlock();
        pc += codeBlock->stringSwitchJumpTable(pc[1].u.operand).offsetForValue(asString(scrutinee)->value(exec).impl(), defaultOffset);
    }
    LLINT_END();
}

LLINT_SLOW_PATH_DECL(slow_path_new_func)
{
    LLINT_BEGIN();
    CodeBlock* codeBlock = exec->codeBlock();
    ASSERT(codeBlock->codeType() != FunctionCode
           || !codeBlock->needsFullScopeChain()
           || exec->uncheckedR(codeBlock->activationRegister()).jsValue());
#if LLINT_SLOW_PATH_TRACING
    dataLogF("Creating function!\n");
#endif
    LLINT_RETURN(JSFunction::create(exec, codeBlock->functionDecl(pc[2].u.operand), exec->scope()));
}

LLINT_SLOW_PATH_DECL(slow_path_new_func_exp)
{
    LLINT_BEGIN();
    CodeBlock* codeBlock = exec->codeBlock();
    FunctionExecutable* function = codeBlock->functionExpr(pc[2].u.operand);
    JSFunction* func = JSFunction::create(exec, function, exec->scope());
    
    LLINT_RETURN(func);
}

static SlowPathReturnType handleHostCall(ExecState* execCallee, Instruction* pc, JSValue callee, CodeSpecializationKind kind)
{
    ExecState* exec = execCallee->callerFrame();
    VM& vm = exec->vm();

    execCallee->setScope(exec->scope());
    execCallee->setCodeBlock(0);
    execCallee->clearReturnPC();

    if (kind == CodeForCall) {
        CallData callData;
        CallType callType = getCallData(callee, callData);
    
        ASSERT(callType != CallTypeJS);
    
        if (callType == CallTypeHost) {
            NativeCallFrameTracer tracer(&vm, execCallee);
            execCallee->setCallee(asObject(callee));
            vm.hostCallReturnValue = JSValue::decode(callData.native.function(execCallee));
            
            LLINT_CALL_RETURN(execCallee, pc, LLInt::getCodePtr(getHostCallReturnValue));
        }
        
#if LLINT_SLOW_PATH_TRACING
        dataLog("Call callee is not a function: ", callee, "\n");
#endif

        ASSERT(callType == CallTypeNone);
        LLINT_CALL_THROW(exec, pc, createNotAFunctionError(exec, callee));
    }

    ASSERT(kind == CodeForConstruct);
    
    ConstructData constructData;
    ConstructType constructType = getConstructData(callee, constructData);
    
    ASSERT(constructType != ConstructTypeJS);
    
    if (constructType == ConstructTypeHost) {
        NativeCallFrameTracer tracer(&vm, execCallee);
        execCallee->setCallee(asObject(callee));
        vm.hostCallReturnValue = JSValue::decode(constructData.native.function(execCallee));

        LLINT_CALL_RETURN(execCallee, pc, LLInt::getCodePtr(getHostCallReturnValue));
    }
    
#if LLINT_SLOW_PATH_TRACING
    dataLog("Constructor callee is not a function: ", callee, "\n");
#endif

    ASSERT(constructType == ConstructTypeNone);
    LLINT_CALL_THROW(exec, pc, createNotAConstructorError(exec, callee));
}

inline SlowPathReturnType setUpCall(ExecState* execCallee, Instruction* pc, CodeSpecializationKind kind, JSValue calleeAsValue, LLIntCallLinkInfo* callLinkInfo = 0)
{
#if LLINT_SLOW_PATH_TRACING
    dataLogF("Performing call with recorded PC = %p\n", execCallee->callerFrame()->currentVPC());
#endif

    JSCell* calleeAsFunctionCell = getJSFunction(calleeAsValue);
    if (!calleeAsFunctionCell)
        return handleHostCall(execCallee, pc, calleeAsValue, kind);
    
    JSFunction* callee = jsCast<JSFunction*>(calleeAsFunctionCell);
    JSScope* scope = callee->scopeUnchecked();
    VM& vm = *scope->vm();
    execCallee->setScope(scope);
    ExecutableBase* executable = callee->executable();
    
    MacroAssemblerCodePtr codePtr;
    CodeBlock* codeBlock = 0;
    if (executable->isHostFunction())
        codePtr = executable->hostCodeEntryFor(kind);
    else {
        FunctionExecutable* functionExecutable = static_cast<FunctionExecutable*>(executable);
        JSObject* error = functionExecutable->compileFor(execCallee, callee->scope(), kind);
        if (error)
            LLINT_CALL_THROW(execCallee->callerFrame(), pc, error);
        codeBlock = &functionExecutable->generatedBytecodeFor(kind);
        ASSERT(codeBlock);
        if (execCallee->argumentCountIncludingThis() < static_cast<size_t>(codeBlock->numParameters()))
            codePtr = functionExecutable->jsCodeWithArityCheckEntryFor(kind);
        else
            codePtr = functionExecutable->jsCodeEntryFor(kind);
    }
    
    if (!LLINT_ALWAYS_ACCESS_SLOW && callLinkInfo) {
        if (callLinkInfo->isOnList())
            callLinkInfo->remove();
        ExecState* execCaller = execCallee->callerFrame();
        callLinkInfo->callee.set(vm, execCaller->codeBlock()->ownerExecutable(), callee);
        callLinkInfo->lastSeenCallee.set(vm, execCaller->codeBlock()->ownerExecutable(), callee);
        callLinkInfo->machineCodeTarget = codePtr;
        if (codeBlock)
            codeBlock->linkIncomingCall(callLinkInfo);
    }

    LLINT_CALL_RETURN(execCallee, pc, codePtr.executableAddress());
}

inline SlowPathReturnType genericCall(ExecState* exec, Instruction* pc, CodeSpecializationKind kind)
{
    // This needs to:
    // - Set up a call frame.
    // - Figure out what to call and compile it if necessary.
    // - If possible, link the call's inline cache.
    // - Return a tuple of machine code address to call and the new call frame.
    
    JSValue calleeAsValue = LLINT_OP_C(1).jsValue();
    
    ExecState* execCallee = exec + pc[3].u.operand;
    
    execCallee->setArgumentCountIncludingThis(pc[2].u.operand);
    execCallee->uncheckedR(JSStack::Callee) = calleeAsValue;
    execCallee->setCallerFrame(exec);
    
    ASSERT(pc[4].u.callLinkInfo);
    return setUpCall(execCallee, pc, kind, calleeAsValue, pc[4].u.callLinkInfo);
}

LLINT_SLOW_PATH_DECL(slow_path_call)
{
    LLINT_BEGIN_NO_SET_PC();
    return genericCall(exec, pc, CodeForCall);
}

LLINT_SLOW_PATH_DECL(slow_path_construct)
{
    LLINT_BEGIN_NO_SET_PC();
    return genericCall(exec, pc, CodeForConstruct);
}

LLINT_SLOW_PATH_DECL(slow_path_call_varargs)
{
    LLINT_BEGIN();
    // This needs to:
    // - Set up a call frame while respecting the variable arguments.
    // - Figure out what to call and compile it if necessary.
    // - Return a tuple of machine code address to call and the new call frame.
    
    JSValue calleeAsValue = LLINT_OP_C(1).jsValue();
    
    ExecState* execCallee = loadVarargs(
        exec, &vm.interpreter->stack(),
        LLINT_OP_C(2).jsValue(), LLINT_OP_C(3).jsValue(), pc[4].u.operand);
    LLINT_CALL_CHECK_EXCEPTION(exec, pc);
    
    execCallee->uncheckedR(JSStack::Callee) = calleeAsValue;
    execCallee->setCallerFrame(exec);
    exec->setCurrentVPC(pc + OPCODE_LENGTH(op_call_varargs));
    
    return setUpCall(execCallee, pc, CodeForCall, calleeAsValue);
}

LLINT_SLOW_PATH_DECL(slow_path_call_eval)
{
    LLINT_BEGIN_NO_SET_PC();
    JSValue calleeAsValue = LLINT_OP(1).jsValue();
    
    ExecState* execCallee = exec + pc[3].u.operand;
    
    execCallee->setArgumentCountIncludingThis(pc[2].u.operand);
    execCallee->setCallerFrame(exec);
    execCallee->uncheckedR(JSStack::Callee) = calleeAsValue;
    execCallee->setScope(exec->scope());
    execCallee->setReturnPC(LLInt::getCodePtr(llint_generic_return_point));
    execCallee->setCodeBlock(0);
    exec->setCurrentVPC(pc + OPCODE_LENGTH(op_call_eval));
    
    if (!isHostFunction(calleeAsValue, globalFuncEval))
        return setUpCall(execCallee, pc, CodeForCall, calleeAsValue);
    
    vm.hostCallReturnValue = eval(execCallee);
    LLINT_CALL_RETURN(execCallee, pc, LLInt::getCodePtr(getHostCallReturnValue));
}

LLINT_SLOW_PATH_DECL(slow_path_tear_off_activation)
{
    LLINT_BEGIN();
    ASSERT(exec->codeBlock()->needsFullScopeChain());
    jsCast<JSActivation*>(LLINT_OP(1).jsValue())->tearOff(vm);
    LLINT_END();
}

LLINT_SLOW_PATH_DECL(slow_path_tear_off_arguments)
{
    LLINT_BEGIN();
    ASSERT(exec->codeBlock()->usesArguments());
    Arguments* arguments = jsCast<Arguments*>(exec->uncheckedR(unmodifiedArgumentsRegister(pc[1].u.operand)).jsValue());
    if (JSValue activationValue = LLINT_OP_C(2).jsValue())
        arguments->didTearOffActivation(exec, jsCast<JSActivation*>(activationValue));
    else
        arguments->tearOff(exec);
    LLINT_END();
}

LLINT_SLOW_PATH_DECL(slow_path_strcat)
{
    LLINT_BEGIN();
    LLINT_RETURN(jsString(exec, &LLINT_OP(2), pc[3].u.operand));
}

LLINT_SLOW_PATH_DECL(slow_path_to_primitive)
{
    LLINT_BEGIN();
    LLINT_RETURN(LLINT_OP_C(2).jsValue().toPrimitive(exec));
}

LLINT_SLOW_PATH_DECL(slow_path_get_pnames)
{
    LLINT_BEGIN();
    JSValue v = LLINT_OP(2).jsValue();
    if (v.isUndefinedOrNull()) {
        pc += pc[5].u.operand;
        LLINT_END();
    }
    
    JSObject* o = v.toObject(exec);
    Structure* structure = o->structure();
    JSPropertyNameIterator* jsPropertyNameIterator = structure->enumerationCache();
    if (!jsPropertyNameIterator || jsPropertyNameIterator->cachedPrototypeChain() != structure->prototypeChain(exec))
        jsPropertyNameIterator = JSPropertyNameIterator::create(exec, o);
    
    LLINT_OP(1) = JSValue(jsPropertyNameIterator);
    LLINT_OP(2) = JSValue(o);
    LLINT_OP(3) = Register::withInt(0);
    LLINT_OP(4) = Register::withInt(jsPropertyNameIterator->size());
    
    pc += OPCODE_LENGTH(op_get_pnames);
    LLINT_END();
}

LLINT_SLOW_PATH_DECL(slow_path_next_pname)
{
    LLINT_BEGIN();
    JSObject* base = asObject(LLINT_OP(2).jsValue());
    JSString* property = asString(LLINT_OP(1).jsValue());
    if (base->hasProperty(exec, Identifier(exec, property->value(exec)))) {
        // Go to target.
        pc += pc[6].u.operand;
    } // Else, don't change the PC, so the interpreter will reloop.
    LLINT_END();
}

LLINT_SLOW_PATH_DECL(slow_path_push_with_scope)
{
    LLINT_BEGIN();
    JSValue v = LLINT_OP_C(1).jsValue();
    JSObject* o = v.toObject(exec);
    LLINT_CHECK_EXCEPTION();
    
    exec->setScope(JSWithScope::create(exec, o));
    
    LLINT_END();
}

LLINT_SLOW_PATH_DECL(slow_path_pop_scope)
{
    LLINT_BEGIN();
    exec->setScope(exec->scope()->next());
    LLINT_END();
}

LLINT_SLOW_PATH_DECL(slow_path_push_name_scope)
{
    LLINT_BEGIN();
    CodeBlock* codeBlock = exec->codeBlock();
    JSNameScope* scope = JSNameScope::create(exec, codeBlock->identifier(pc[1].u.operand), LLINT_OP(2).jsValue(), pc[3].u.operand);
    exec->setScope(scope);
    LLINT_END();
}

LLINT_SLOW_PATH_DECL(slow_path_throw)
{
    LLINT_BEGIN();
    LLINT_THROW(LLINT_OP_C(1).jsValue());
}

LLINT_SLOW_PATH_DECL(slow_path_throw_static_error)
{
    LLINT_BEGIN();
    if (pc[2].u.operand)
        LLINT_THROW(createReferenceError(exec, errorDescriptionForValue(exec, LLINT_OP_C(1).jsValue())->value(exec)));
    else
        LLINT_THROW(createTypeError(exec, errorDescriptionForValue(exec, LLINT_OP_C(1).jsValue())->value(exec)));
}

LLINT_SLOW_PATH_DECL(slow_path_handle_watchdog_timer)
{
    LLINT_BEGIN_NO_SET_PC();
    if (UNLIKELY(vm.watchdog.didFire(exec)))
        LLINT_THROW(createTerminatedExecutionException(&vm));
    LLINT_RETURN_TWO(0, exec);
}

LLINT_SLOW_PATH_DECL(slow_path_debug)
{
    LLINT_BEGIN();
    int debugHookID = pc[1].u.operand;
    int firstLine = pc[2].u.operand;
    int lastLine = pc[3].u.operand;
    int column = pc[4].u.operand;

    vm.interpreter->debug(exec, static_cast<DebugHookID>(debugHookID), firstLine, lastLine, column);
    
    LLINT_END();
}

LLINT_SLOW_PATH_DECL(slow_path_profile_will_call)
{
    LLINT_BEGIN();
    if (LegacyProfiler* profiler = vm.enabledProfiler())
        profiler->willExecute(exec, LLINT_OP(1).jsValue());
    LLINT_END();
}

LLINT_SLOW_PATH_DECL(slow_path_profile_did_call)
{
    LLINT_BEGIN();
    if (LegacyProfiler* profiler = vm.enabledProfiler())
        profiler->didExecute(exec, LLINT_OP(1).jsValue());
    LLINT_END();
}

LLINT_SLOW_PATH_DECL(throw_from_native_call)
{
    LLINT_BEGIN();
    ASSERT(vm.exception);
    LLINT_END();
}

} } // namespace JSC::LLInt

#endif // ENABLE(LLINT)
