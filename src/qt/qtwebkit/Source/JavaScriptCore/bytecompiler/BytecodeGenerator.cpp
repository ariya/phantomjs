/*
 * Copyright (C) 2008, 2009, 2012, 2013 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Cameron Zwarich <cwzwarich@uwaterloo.ca>
 * Copyright (C) 2012 Igalia, S.L.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "BytecodeGenerator.h"

#include "BatchedTransitionOptimizer.h"
#include "Interpreter.h"
#include "JSActivation.h"
#include "JSFunction.h"
#include "JSNameScope.h"
#include "LowLevelInterpreter.h"
#include "Operations.h"
#include "Options.h"
#include "StrongInlines.h"
#include "UnlinkedCodeBlock.h"
#include <wtf/text/WTFString.h>

using namespace std;

namespace JSC {

void Label::setLocation(unsigned location)
{
    m_location = location;
    
    unsigned size = m_unresolvedJumps.size();
    for (unsigned i = 0; i < size; ++i)
        m_generator->m_instructions[m_unresolvedJumps[i].second].u.operand = m_location - m_unresolvedJumps[i].first;
}

#ifndef NDEBUG
void ResolveResult::checkValidity()
{
    switch (m_type) {
    case Register:
    case ReadOnlyRegister:
        ASSERT(m_local);
        return;
    case Dynamic:
        ASSERT(!m_local);
        return;
    case Lexical:
    case ReadOnlyLexical:
        ASSERT(!m_local);
        return;
    default:
        RELEASE_ASSERT_NOT_REACHED();
    }
}
#endif

ParserError BytecodeGenerator::generate()
{
    SamplingRegion samplingRegion("Bytecode Generation");
    
    m_codeBlock->setThisRegister(m_thisRegister.index());

    m_scopeNode->emitBytecode(*this);

    m_staticPropertyAnalyzer.kill();

    for (unsigned i = 0; i < m_tryRanges.size(); ++i) {
        TryRange& range = m_tryRanges[i];
        int start = range.start->bind();
        int end = range.end->bind();
        
        // This will happen for empty try blocks and for some cases of finally blocks:
        //
        // try {
        //    try {
        //    } finally {
        //        return 42;
        //        // *HERE*
        //    }
        // } finally {
        //    print("things");
        // }
        //
        // The return will pop scopes to execute the outer finally block. But this includes
        // popping the try context for the inner try. The try context is live in the fall-through
        // part of the finally block not because we will emit a handler that overlaps the finally,
        // but because we haven't yet had a chance to plant the catch target. Then when we finish
        // emitting code for the outer finally block, we repush the try contex, this time with a
        // new start index. But that means that the start index for the try range corresponding
        // to the inner-finally-following-the-return (marked as "*HERE*" above) will be greater
        // than the end index of the try block. This is harmless since end < start handlers will
        // never get matched in our logic, but we do the runtime a favor and choose to not emit
        // such handlers at all.
        if (end <= start)
            continue;
        
        ASSERT(range.tryData->targetScopeDepth != UINT_MAX);
        UnlinkedHandlerInfo info = {
            static_cast<uint32_t>(start), static_cast<uint32_t>(end),
            static_cast<uint32_t>(range.tryData->target->bind()),
            range.tryData->targetScopeDepth
        };
        m_codeBlock->addExceptionHandler(info);
    }
    
    m_codeBlock->instructions() = RefCountedArray<UnlinkedInstruction>(m_instructions);

    m_codeBlock->shrinkToFit();

    if (m_expressionTooDeep)
        return ParserError(ParserError::OutOfMemory);
    return ParserError(ParserError::ErrorNone);
}

bool BytecodeGenerator::addVar(const Identifier& ident, bool isConstant, RegisterID*& r0)
{
    int index = m_calleeRegisters.size();
    SymbolTableEntry newEntry(index, isConstant ? ReadOnly : 0);
    SymbolTable::AddResult result = symbolTable().add(ident.impl(), newEntry);

    if (!result.isNewEntry) {
        r0 = &registerFor(result.iterator->value.getIndex());
        return false;
    }

    r0 = addVar();
    return true;
}

void BytecodeGenerator::preserveLastVar()
{
    if ((m_firstConstantIndex = m_calleeRegisters.size()) != 0)
        m_lastVar = &m_calleeRegisters.last();
}

BytecodeGenerator::BytecodeGenerator(VM& vm, JSScope*, ProgramNode* programNode, UnlinkedProgramCodeBlock* codeBlock, DebuggerMode debuggerMode, ProfilerMode profilerMode)
    : m_shouldEmitDebugHooks(debuggerMode == DebuggerOn)
    , m_shouldEmitProfileHooks(profilerMode == ProfilerOn)
    , m_symbolTable(0)
    , m_scopeNode(programNode)
    , m_codeBlock(vm, codeBlock)
    , m_thisRegister(CallFrame::thisArgumentOffset())
    , m_emptyValueRegister(0)
    , m_globalObjectRegister(0)
    , m_finallyDepth(0)
    , m_dynamicScopeDepth(0)
    , m_codeType(GlobalCode)
    , m_nextConstantOffset(0)
    , m_globalConstantIndex(0)
    , m_hasCreatedActivation(true)
    , m_firstLazyFunction(0)
    , m_lastLazyFunction(0)
    , m_staticPropertyAnalyzer(&m_instructions)
    , m_vm(&vm)
    , m_lastOpcodeID(op_end)
#ifndef NDEBUG
    , m_lastOpcodePosition(0)
#endif
    , m_stack(vm, wtfThreadData().stack())
    , m_usesExceptions(false)
    , m_expressionTooDeep(false)
{
    if (m_shouldEmitDebugHooks)
        m_codeBlock->setNeedsFullScopeChain(true);

    m_codeBlock->setNumParameters(1); // Allocate space for "this"

    emitOpcode(op_enter);

    const VarStack& varStack = programNode->varStack();
    const FunctionStack& functionStack = programNode->functionStack();

    for (size_t i = 0; i < functionStack.size(); ++i) {
        FunctionBodyNode* function = functionStack[i];
        UnlinkedFunctionExecutable* unlinkedFunction = makeFunction(function);
        codeBlock->addFunctionDeclaration(*m_vm, function->ident(), unlinkedFunction);
    }

    for (size_t i = 0; i < varStack.size(); ++i)
        codeBlock->addVariableDeclaration(*varStack[i].first, !!(varStack[i].second & DeclarationStacks::IsConstant));

}

BytecodeGenerator::BytecodeGenerator(VM& vm, JSScope* scope, FunctionBodyNode* functionBody, UnlinkedFunctionCodeBlock* codeBlock, DebuggerMode debuggerMode, ProfilerMode profilerMode)
    : m_shouldEmitDebugHooks(debuggerMode == DebuggerOn)
    , m_shouldEmitProfileHooks(profilerMode == ProfilerOn)
    , m_symbolTable(codeBlock->symbolTable())
    , m_scopeNode(functionBody)
    , m_scope(vm, scope)
    , m_codeBlock(vm, codeBlock)
    , m_activationRegister(0)
    , m_emptyValueRegister(0)
    , m_globalObjectRegister(0)
    , m_finallyDepth(0)
    , m_dynamicScopeDepth(0)
    , m_codeType(FunctionCode)
    , m_nextConstantOffset(0)
    , m_globalConstantIndex(0)
    , m_hasCreatedActivation(false)
    , m_firstLazyFunction(0)
    , m_lastLazyFunction(0)
    , m_staticPropertyAnalyzer(&m_instructions)
    , m_vm(&vm)
    , m_lastOpcodeID(op_end)
#ifndef NDEBUG
    , m_lastOpcodePosition(0)
#endif
    , m_stack(vm, wtfThreadData().stack())
    , m_usesExceptions(false)
    , m_expressionTooDeep(false)
{
    if (m_shouldEmitDebugHooks)
        m_codeBlock->setNeedsFullScopeChain(true);

    m_symbolTable->setUsesNonStrictEval(codeBlock->usesEval() && !codeBlock->isStrictMode());
    m_symbolTable->setParameterCountIncludingThis(functionBody->parameters()->size() + 1);

    emitOpcode(op_enter);
    if (m_codeBlock->needsFullScopeChain()) {
        m_activationRegister = addVar();
        emitInitLazyRegister(m_activationRegister);
        m_codeBlock->setActivationRegister(m_activationRegister->index());
    }

    m_symbolTable->setCaptureStart(m_codeBlock->m_numVars);

    if (functionBody->usesArguments() || codeBlock->usesEval() || m_shouldEmitDebugHooks) { // May reify arguments object.
        RegisterID* unmodifiedArgumentsRegister = addVar(); // Anonymous, so it can't be modified by user code.
        RegisterID* argumentsRegister = addVar(propertyNames().arguments, false); // Can be changed by assigning to 'arguments'.

        // We can save a little space by hard-coding the knowledge that the two
        // 'arguments' values are stored in consecutive registers, and storing
        // only the index of the assignable one.
        codeBlock->setArgumentsRegister(argumentsRegister->index());
        ASSERT_UNUSED(unmodifiedArgumentsRegister, unmodifiedArgumentsRegister->index() == JSC::unmodifiedArgumentsRegister(codeBlock->argumentsRegister()));

        emitInitLazyRegister(argumentsRegister);
        emitInitLazyRegister(unmodifiedArgumentsRegister);
        
        if (m_codeBlock->isStrictMode()) {
            emitOpcode(op_create_arguments);
            instructions().append(argumentsRegister->index());
        }

        // The debugger currently retrieves the arguments object from an activation rather than pulling
        // it from a call frame.  In the long-term it should stop doing that (<rdar://problem/6911886>),
        // but for now we force eager creation of the arguments object when debugging.
        if (m_shouldEmitDebugHooks) {
            emitOpcode(op_create_arguments);
            instructions().append(argumentsRegister->index());
        }
    }

    bool shouldCaptureAllTheThings = m_shouldEmitDebugHooks || codeBlock->usesEval();

    bool capturesAnyArgumentByName = false;
    Vector<RegisterID*, 0, UnsafeVectorOverflow> capturedArguments;
    if (functionBody->hasCapturedVariables() || shouldCaptureAllTheThings) {
        FunctionParameters& parameters = *functionBody->parameters();
        capturedArguments.resize(parameters.size());
        for (size_t i = 0; i < parameters.size(); ++i) {
            capturedArguments[i] = 0;
            if (!functionBody->captures(parameters.at(i)) && !shouldCaptureAllTheThings)
                continue;
            capturesAnyArgumentByName = true;
            capturedArguments[i] = addVar();
        }
    }

    if (capturesAnyArgumentByName && !codeBlock->isStrictMode()) {
        size_t parameterCount = m_symbolTable->parameterCount();
        OwnArrayPtr<SlowArgument> slowArguments = adoptArrayPtr(new SlowArgument[parameterCount]);
        for (size_t i = 0; i < parameterCount; ++i) {
            if (!capturedArguments[i]) {
                ASSERT(slowArguments[i].status == SlowArgument::Normal);
                slowArguments[i].index = CallFrame::argumentOffset(i);
                continue;
            }
            slowArguments[i].status = SlowArgument::Captured;
            slowArguments[i].index = capturedArguments[i]->index();
        }
        m_symbolTable->setSlowArguments(slowArguments.release());
    }

    RegisterID* calleeRegister = resolveCallee(functionBody); // May push to the scope chain and/or add a captured var.

    const DeclarationStacks::FunctionStack& functionStack = functionBody->functionStack();
    const DeclarationStacks::VarStack& varStack = functionBody->varStack();

    // Captured variables and functions go first so that activations don't have
    // to step over the non-captured locals to mark them.
    m_hasCreatedActivation = false;
    if (functionBody->hasCapturedVariables()) {
        for (size_t i = 0; i < functionStack.size(); ++i) {
            FunctionBodyNode* function = functionStack[i];
            const Identifier& ident = function->ident();
            if (functionBody->captures(ident)) {
                if (!m_hasCreatedActivation) {
                    m_hasCreatedActivation = true;
                    emitOpcode(op_create_activation);
                    instructions().append(m_activationRegister->index());
                }
                m_functions.add(ident.impl());
                emitNewFunction(addVar(ident, false), function);
            }
        }
        for (size_t i = 0; i < varStack.size(); ++i) {
            const Identifier& ident = *varStack[i].first;
            if (functionBody->captures(ident))
                addVar(ident, varStack[i].second & DeclarationStacks::IsConstant);
        }
    }
    bool canLazilyCreateFunctions = !functionBody->needsActivationForMoreThanVariables() && !m_shouldEmitDebugHooks;
    if (!canLazilyCreateFunctions && !m_hasCreatedActivation) {
        m_hasCreatedActivation = true;
        emitOpcode(op_create_activation);
        instructions().append(m_activationRegister->index());
    }

    m_symbolTable->setCaptureEnd(codeBlock->m_numVars);

    m_firstLazyFunction = codeBlock->m_numVars;
    for (size_t i = 0; i < functionStack.size(); ++i) {
        FunctionBodyNode* function = functionStack[i];
        const Identifier& ident = function->ident();
        if (!functionBody->captures(ident)) {
            m_functions.add(ident.impl());
            RefPtr<RegisterID> reg = addVar(ident, false);
            // Don't lazily create functions that override the name 'arguments'
            // as this would complicate lazy instantiation of actual arguments.
            if (!canLazilyCreateFunctions || ident == propertyNames().arguments)
                emitNewFunction(reg.get(), function);
            else {
                emitInitLazyRegister(reg.get());
                m_lazyFunctions.set(reg->index(), function);
            }
        }
    }
    m_lastLazyFunction = canLazilyCreateFunctions ? codeBlock->m_numVars : m_firstLazyFunction;
    for (size_t i = 0; i < varStack.size(); ++i) {
        const Identifier& ident = *varStack[i].first;
        if (!functionBody->captures(ident))
            addVar(ident, varStack[i].second & DeclarationStacks::IsConstant);
    }

    if (shouldCaptureAllTheThings)
        m_symbolTable->setCaptureEnd(codeBlock->m_numVars);

    FunctionParameters& parameters = *functionBody->parameters();
    m_parameters.grow(parameters.size() + 1); // reserve space for "this"

    // Add "this" as a parameter
    int nextParameterIndex = CallFrame::thisArgumentOffset();
    m_thisRegister.setIndex(nextParameterIndex--);
    m_codeBlock->addParameter();
    
    for (size_t i = 0; i < parameters.size(); ++i, --nextParameterIndex) {
        int index = nextParameterIndex;
        if (capturedArguments.size() && capturedArguments[i]) {
            ASSERT((functionBody->hasCapturedVariables() && functionBody->captures(parameters.at(i))) || shouldCaptureAllTheThings);
            index = capturedArguments[i]->index();
            RegisterID original(nextParameterIndex);
            emitMove(capturedArguments[i], &original);
        }
        addParameter(parameters.at(i), index);
    }
    preserveLastVar();

    // We declare the callee's name last because it should lose to a var, function, and/or parameter declaration.
    addCallee(functionBody, calleeRegister);

    if (isConstructor()) {
        emitCreateThis(&m_thisRegister);
    } else if (!codeBlock->isStrictMode() && (functionBody->usesThis() || codeBlock->usesEval() || m_shouldEmitDebugHooks)) {
        UnlinkedValueProfile profile = emitProfiledOpcode(op_convert_this);
        instructions().append(kill(&m_thisRegister));
        instructions().append(profile);
    }
}

BytecodeGenerator::BytecodeGenerator(VM& vm, JSScope* scope, EvalNode* evalNode, UnlinkedEvalCodeBlock* codeBlock, DebuggerMode debuggerMode, ProfilerMode profilerMode)
    : m_shouldEmitDebugHooks(debuggerMode == DebuggerOn)
    , m_shouldEmitProfileHooks(profilerMode == ProfilerOn)
    , m_symbolTable(codeBlock->symbolTable())
    , m_scopeNode(evalNode)
    , m_scope(vm, scope)
    , m_codeBlock(vm, codeBlock)
    , m_thisRegister(CallFrame::thisArgumentOffset())
    , m_emptyValueRegister(0)
    , m_globalObjectRegister(0)
    , m_finallyDepth(0)
    , m_dynamicScopeDepth(0)
    , m_codeType(EvalCode)
    , m_nextConstantOffset(0)
    , m_globalConstantIndex(0)
    , m_hasCreatedActivation(true)
    , m_firstLazyFunction(0)
    , m_lastLazyFunction(0)
    , m_staticPropertyAnalyzer(&m_instructions)
    , m_vm(&vm)
    , m_lastOpcodeID(op_end)
#ifndef NDEBUG
    , m_lastOpcodePosition(0)
#endif
    , m_stack(vm, wtfThreadData().stack())
    , m_usesExceptions(false)
    , m_expressionTooDeep(false)
{
    m_codeBlock->setNeedsFullScopeChain(true);

    m_symbolTable->setUsesNonStrictEval(codeBlock->usesEval() && !codeBlock->isStrictMode());
    m_codeBlock->setNumParameters(1);

    emitOpcode(op_enter);

    const DeclarationStacks::FunctionStack& functionStack = evalNode->functionStack();
    for (size_t i = 0; i < functionStack.size(); ++i)
        m_codeBlock->addFunctionDecl(makeFunction(functionStack[i]));

    const DeclarationStacks::VarStack& varStack = evalNode->varStack();
    unsigned numVariables = varStack.size();
    Vector<Identifier, 0, UnsafeVectorOverflow> variables;
    variables.reserveCapacity(numVariables);
    for (size_t i = 0; i < numVariables; ++i)
        variables.append(*varStack[i].first);
    codeBlock->adoptVariables(variables);
    preserveLastVar();
}

BytecodeGenerator::~BytecodeGenerator()
{
}

RegisterID* BytecodeGenerator::emitInitLazyRegister(RegisterID* reg)
{
    emitOpcode(op_init_lazy_reg);
    instructions().append(reg->index());
    return reg;
}

RegisterID* BytecodeGenerator::resolveCallee(FunctionBodyNode* functionBodyNode)
{
    if (functionBodyNode->ident().isNull() || !functionBodyNode->functionNameIsInScope())
        return 0;

    m_calleeRegister.setIndex(JSStack::Callee);

    // If non-strict eval is in play, we use a separate object in the scope chain for the callee's name.
    if ((m_codeBlock->usesEval() && !m_codeBlock->isStrictMode()) || m_shouldEmitDebugHooks) {
        emitOpcode(op_push_name_scope);
        instructions().append(addConstant(functionBodyNode->ident()));
        instructions().append(m_calleeRegister.index());
        instructions().append(ReadOnly | DontDelete);
        return 0;
    }

    if (!functionBodyNode->captures(functionBodyNode->ident()))
        return &m_calleeRegister;

    // Move the callee into the captured section of the stack.
    return emitMove(addVar(), &m_calleeRegister);
}

void BytecodeGenerator::addCallee(FunctionBodyNode* functionBodyNode, RegisterID* calleeRegister)
{
    if (functionBodyNode->ident().isNull() || !functionBodyNode->functionNameIsInScope())
        return;

    // If non-strict eval is in play, we use a separate object in the scope chain for the callee's name.
    if ((m_codeBlock->usesEval() && !m_codeBlock->isStrictMode()) || m_shouldEmitDebugHooks)
        return;

    ASSERT(calleeRegister);
    symbolTable().add(functionBodyNode->ident().impl(), SymbolTableEntry(calleeRegister->index(), ReadOnly));
}

void BytecodeGenerator::addParameter(const Identifier& ident, int parameterIndex)
{
    // Parameters overwrite var declarations, but not function declarations.
    StringImpl* rep = ident.impl();
    if (!m_functions.contains(rep)) {
        symbolTable().set(rep, parameterIndex);
        RegisterID& parameter = registerFor(parameterIndex);
        parameter.setIndex(parameterIndex);
    }

    // To maintain the calling convention, we have to allocate unique space for
    // each parameter, even if the parameter doesn't make it into the symbol table.
    m_codeBlock->addParameter();
}

bool BytecodeGenerator::willResolveToArguments(const Identifier& ident)
{
    if (ident != propertyNames().arguments)
        return false;
    
    if (!shouldOptimizeLocals())
        return false;
    
    SymbolTableEntry entry = symbolTable().get(ident.impl());
    if (entry.isNull())
        return false;

    if (m_codeBlock->usesArguments() && m_codeType == FunctionCode)
        return true;
    
    return false;
}

RegisterID* BytecodeGenerator::uncheckedRegisterForArguments()
{
    ASSERT(willResolveToArguments(propertyNames().arguments));

    SymbolTableEntry entry = symbolTable().get(propertyNames().arguments.impl());
    ASSERT(!entry.isNull());
    return &registerFor(entry.getIndex());
}

RegisterID* BytecodeGenerator::createLazyRegisterIfNecessary(RegisterID* reg)
{
    if (m_lastLazyFunction <= reg->index() || reg->index() < m_firstLazyFunction)
        return reg;
    emitLazyNewFunction(reg, m_lazyFunctions.get(reg->index()));
    return reg;
}

RegisterID* BytecodeGenerator::newRegister()
{
    m_calleeRegisters.append(m_calleeRegisters.size());
    m_codeBlock->m_numCalleeRegisters = max<int>(m_codeBlock->m_numCalleeRegisters, m_calleeRegisters.size());
    return &m_calleeRegisters.last();
}

RegisterID* BytecodeGenerator::newTemporary()
{
    // Reclaim free register IDs.
    while (m_calleeRegisters.size() && !m_calleeRegisters.last().refCount())
        m_calleeRegisters.removeLast();
        
    RegisterID* result = newRegister();
    result->setTemporary();
    return result;
}

LabelScopePtr BytecodeGenerator::newLabelScope(LabelScope::Type type, const Identifier* name)
{
    // Reclaim free label scopes.
    while (m_labelScopes.size() && !m_labelScopes.last().refCount())
        m_labelScopes.removeLast();

    // Allocate new label scope.
    LabelScope scope(type, name, scopeDepth(), newLabel(), type == LabelScope::Loop ? newLabel() : PassRefPtr<Label>()); // Only loops have continue targets.
    m_labelScopes.append(scope);
    return LabelScopePtr(&m_labelScopes, m_labelScopes.size() - 1);
}

PassRefPtr<Label> BytecodeGenerator::newLabel()
{
    // Reclaim free label IDs.
    while (m_labels.size() && !m_labels.last().refCount())
        m_labels.removeLast();

    // Allocate new label ID.
    m_labels.append(this);
    return &m_labels.last();
}

PassRefPtr<Label> BytecodeGenerator::emitLabel(Label* l0)
{
    unsigned newLabelIndex = instructions().size();
    l0->setLocation(newLabelIndex);

    if (m_codeBlock->numberOfJumpTargets()) {
        unsigned lastLabelIndex = m_codeBlock->lastJumpTarget();
        ASSERT(lastLabelIndex <= newLabelIndex);
        if (newLabelIndex == lastLabelIndex) {
            // Peephole optimizations have already been disabled by emitting the last label
            return l0;
        }
    }

    m_codeBlock->addJumpTarget(newLabelIndex);

    // This disables peephole optimizations when an instruction is a jump target
    m_lastOpcodeID = op_end;
    return l0;
}

void BytecodeGenerator::emitOpcode(OpcodeID opcodeID)
{
#ifndef NDEBUG
    size_t opcodePosition = instructions().size();
    ASSERT(opcodePosition - m_lastOpcodePosition == opcodeLength(m_lastOpcodeID) || m_lastOpcodeID == op_end);
    m_lastOpcodePosition = opcodePosition;
#endif
    instructions().append(opcodeID);
    m_lastOpcodeID = opcodeID;
}

UnlinkedArrayProfile BytecodeGenerator::newArrayProfile()
{
#if ENABLE(VALUE_PROFILER)
    return m_codeBlock->addArrayProfile();
#else
    return 0;
#endif
}

UnlinkedArrayAllocationProfile BytecodeGenerator::newArrayAllocationProfile()
{
#if ENABLE(VALUE_PROFILER)
    return m_codeBlock->addArrayAllocationProfile();
#else
    return 0;
#endif
}

UnlinkedObjectAllocationProfile BytecodeGenerator::newObjectAllocationProfile()
{
    return m_codeBlock->addObjectAllocationProfile();
}

UnlinkedValueProfile BytecodeGenerator::emitProfiledOpcode(OpcodeID opcodeID)
{
#if ENABLE(VALUE_PROFILER)
    UnlinkedValueProfile result = m_codeBlock->addValueProfile();
#else
    UnlinkedValueProfile result = 0;
#endif
    emitOpcode(opcodeID);
    return result;
}

void BytecodeGenerator::emitLoopHint()
{
    emitOpcode(op_loop_hint);
}

void BytecodeGenerator::retrieveLastBinaryOp(int& dstIndex, int& src1Index, int& src2Index)
{
    ASSERT(instructions().size() >= 4);
    size_t size = instructions().size();
    dstIndex = instructions().at(size - 3).u.operand;
    src1Index = instructions().at(size - 2).u.operand;
    src2Index = instructions().at(size - 1).u.operand;
}

void BytecodeGenerator::retrieveLastUnaryOp(int& dstIndex, int& srcIndex)
{
    ASSERT(instructions().size() >= 3);
    size_t size = instructions().size();
    dstIndex = instructions().at(size - 2).u.operand;
    srcIndex = instructions().at(size - 1).u.operand;
}

void ALWAYS_INLINE BytecodeGenerator::rewindBinaryOp()
{
    ASSERT(instructions().size() >= 4);
    instructions().shrink(instructions().size() - 4);
    m_lastOpcodeID = op_end;
}

void ALWAYS_INLINE BytecodeGenerator::rewindUnaryOp()
{
    ASSERT(instructions().size() >= 3);
    instructions().shrink(instructions().size() - 3);
    m_lastOpcodeID = op_end;
}

PassRefPtr<Label> BytecodeGenerator::emitJump(Label* target)
{
    size_t begin = instructions().size();
    emitOpcode(op_jmp);
    instructions().append(target->bind(begin, instructions().size()));
    return target;
}

PassRefPtr<Label> BytecodeGenerator::emitJumpIfTrue(RegisterID* cond, Label* target)
{
    if (m_lastOpcodeID == op_less) {
        int dstIndex;
        int src1Index;
        int src2Index;

        retrieveLastBinaryOp(dstIndex, src1Index, src2Index);

        if (cond->index() == dstIndex && cond->isTemporary() && !cond->refCount()) {
            rewindBinaryOp();

            size_t begin = instructions().size();
            emitOpcode(op_jless);
            instructions().append(src1Index);
            instructions().append(src2Index);
            instructions().append(target->bind(begin, instructions().size()));
            return target;
        }
    } else if (m_lastOpcodeID == op_lesseq) {
        int dstIndex;
        int src1Index;
        int src2Index;

        retrieveLastBinaryOp(dstIndex, src1Index, src2Index);

        if (cond->index() == dstIndex && cond->isTemporary() && !cond->refCount()) {
            rewindBinaryOp();

            size_t begin = instructions().size();
            emitOpcode(op_jlesseq);
            instructions().append(src1Index);
            instructions().append(src2Index);
            instructions().append(target->bind(begin, instructions().size()));
            return target;
        }
    } else if (m_lastOpcodeID == op_greater) {
        int dstIndex;
        int src1Index;
        int src2Index;

        retrieveLastBinaryOp(dstIndex, src1Index, src2Index);

        if (cond->index() == dstIndex && cond->isTemporary() && !cond->refCount()) {
            rewindBinaryOp();

            size_t begin = instructions().size();
            emitOpcode(op_jgreater);
            instructions().append(src1Index);
            instructions().append(src2Index);
            instructions().append(target->bind(begin, instructions().size()));
            return target;
        }
    } else if (m_lastOpcodeID == op_greatereq) {
        int dstIndex;
        int src1Index;
        int src2Index;

        retrieveLastBinaryOp(dstIndex, src1Index, src2Index);

        if (cond->index() == dstIndex && cond->isTemporary() && !cond->refCount()) {
            rewindBinaryOp();

            size_t begin = instructions().size();
            emitOpcode(op_jgreatereq);
            instructions().append(src1Index);
            instructions().append(src2Index);
            instructions().append(target->bind(begin, instructions().size()));
            return target;
        }
    } else if (m_lastOpcodeID == op_eq_null && target->isForward()) {
        int dstIndex;
        int srcIndex;

        retrieveLastUnaryOp(dstIndex, srcIndex);

        if (cond->index() == dstIndex && cond->isTemporary() && !cond->refCount()) {
            rewindUnaryOp();

            size_t begin = instructions().size();
            emitOpcode(op_jeq_null);
            instructions().append(srcIndex);
            instructions().append(target->bind(begin, instructions().size()));
            return target;
        }
    } else if (m_lastOpcodeID == op_neq_null && target->isForward()) {
        int dstIndex;
        int srcIndex;

        retrieveLastUnaryOp(dstIndex, srcIndex);

        if (cond->index() == dstIndex && cond->isTemporary() && !cond->refCount()) {
            rewindUnaryOp();

            size_t begin = instructions().size();
            emitOpcode(op_jneq_null);
            instructions().append(srcIndex);
            instructions().append(target->bind(begin, instructions().size()));
            return target;
        }
    }

    size_t begin = instructions().size();

    emitOpcode(op_jtrue);
    instructions().append(cond->index());
    instructions().append(target->bind(begin, instructions().size()));
    return target;
}

PassRefPtr<Label> BytecodeGenerator::emitJumpIfFalse(RegisterID* cond, Label* target)
{
    if (m_lastOpcodeID == op_less && target->isForward()) {
        int dstIndex;
        int src1Index;
        int src2Index;

        retrieveLastBinaryOp(dstIndex, src1Index, src2Index);

        if (cond->index() == dstIndex && cond->isTemporary() && !cond->refCount()) {
            rewindBinaryOp();

            size_t begin = instructions().size();
            emitOpcode(op_jnless);
            instructions().append(src1Index);
            instructions().append(src2Index);
            instructions().append(target->bind(begin, instructions().size()));
            return target;
        }
    } else if (m_lastOpcodeID == op_lesseq && target->isForward()) {
        int dstIndex;
        int src1Index;
        int src2Index;

        retrieveLastBinaryOp(dstIndex, src1Index, src2Index);

        if (cond->index() == dstIndex && cond->isTemporary() && !cond->refCount()) {
            rewindBinaryOp();

            size_t begin = instructions().size();
            emitOpcode(op_jnlesseq);
            instructions().append(src1Index);
            instructions().append(src2Index);
            instructions().append(target->bind(begin, instructions().size()));
            return target;
        }
    } else if (m_lastOpcodeID == op_greater && target->isForward()) {
        int dstIndex;
        int src1Index;
        int src2Index;

        retrieveLastBinaryOp(dstIndex, src1Index, src2Index);

        if (cond->index() == dstIndex && cond->isTemporary() && !cond->refCount()) {
            rewindBinaryOp();

            size_t begin = instructions().size();
            emitOpcode(op_jngreater);
            instructions().append(src1Index);
            instructions().append(src2Index);
            instructions().append(target->bind(begin, instructions().size()));
            return target;
        }
    } else if (m_lastOpcodeID == op_greatereq && target->isForward()) {
        int dstIndex;
        int src1Index;
        int src2Index;

        retrieveLastBinaryOp(dstIndex, src1Index, src2Index);

        if (cond->index() == dstIndex && cond->isTemporary() && !cond->refCount()) {
            rewindBinaryOp();

            size_t begin = instructions().size();
            emitOpcode(op_jngreatereq);
            instructions().append(src1Index);
            instructions().append(src2Index);
            instructions().append(target->bind(begin, instructions().size()));
            return target;
        }
    } else if (m_lastOpcodeID == op_not) {
        int dstIndex;
        int srcIndex;

        retrieveLastUnaryOp(dstIndex, srcIndex);

        if (cond->index() == dstIndex && cond->isTemporary() && !cond->refCount()) {
            rewindUnaryOp();

            size_t begin = instructions().size();
            emitOpcode(op_jtrue);
            instructions().append(srcIndex);
            instructions().append(target->bind(begin, instructions().size()));
            return target;
        }
    } else if (m_lastOpcodeID == op_eq_null && target->isForward()) {
        int dstIndex;
        int srcIndex;

        retrieveLastUnaryOp(dstIndex, srcIndex);

        if (cond->index() == dstIndex && cond->isTemporary() && !cond->refCount()) {
            rewindUnaryOp();

            size_t begin = instructions().size();
            emitOpcode(op_jneq_null);
            instructions().append(srcIndex);
            instructions().append(target->bind(begin, instructions().size()));
            return target;
        }
    } else if (m_lastOpcodeID == op_neq_null && target->isForward()) {
        int dstIndex;
        int srcIndex;

        retrieveLastUnaryOp(dstIndex, srcIndex);

        if (cond->index() == dstIndex && cond->isTemporary() && !cond->refCount()) {
            rewindUnaryOp();

            size_t begin = instructions().size();
            emitOpcode(op_jeq_null);
            instructions().append(srcIndex);
            instructions().append(target->bind(begin, instructions().size()));
            return target;
        }
    }

    size_t begin = instructions().size();
    emitOpcode(op_jfalse);
    instructions().append(cond->index());
    instructions().append(target->bind(begin, instructions().size()));
    return target;
}

PassRefPtr<Label> BytecodeGenerator::emitJumpIfNotFunctionCall(RegisterID* cond, Label* target)
{
    size_t begin = instructions().size();

    emitOpcode(op_jneq_ptr);
    instructions().append(cond->index());
    instructions().append(Special::CallFunction);
    instructions().append(target->bind(begin, instructions().size()));
    return target;
}

PassRefPtr<Label> BytecodeGenerator::emitJumpIfNotFunctionApply(RegisterID* cond, Label* target)
{
    size_t begin = instructions().size();

    emitOpcode(op_jneq_ptr);
    instructions().append(cond->index());
    instructions().append(Special::ApplyFunction);
    instructions().append(target->bind(begin, instructions().size()));
    return target;
}

unsigned BytecodeGenerator::addConstant(const Identifier& ident)
{
    StringImpl* rep = ident.impl();
    IdentifierMap::AddResult result = m_identifierMap.add(rep, m_codeBlock->numberOfIdentifiers());
    if (result.isNewEntry)
        m_codeBlock->addIdentifier(Identifier(m_vm, rep));

    return result.iterator->value;
}

// We can't hash JSValue(), so we use a dedicated data member to cache it.
RegisterID* BytecodeGenerator::addConstantEmptyValue()
{
    if (!m_emptyValueRegister) {
        int index = m_nextConstantOffset;
        m_constantPoolRegisters.append(FirstConstantRegisterIndex + m_nextConstantOffset);
        ++m_nextConstantOffset;
        m_codeBlock->addConstant(JSValue());
        m_emptyValueRegister = &m_constantPoolRegisters[index];
    }

    return m_emptyValueRegister;
}

RegisterID* BytecodeGenerator::addConstantValue(JSValue v)
{
    if (!v)
        return addConstantEmptyValue();

    int index = m_nextConstantOffset;
    JSValueMap::AddResult result = m_jsValueMap.add(JSValue::encode(v), m_nextConstantOffset);
    if (result.isNewEntry) {
        m_constantPoolRegisters.append(FirstConstantRegisterIndex + m_nextConstantOffset);
        ++m_nextConstantOffset;
        m_codeBlock->addConstant(v);
    } else
        index = result.iterator->value;
    return &m_constantPoolRegisters[index];
}

unsigned BytecodeGenerator::addRegExp(RegExp* r)
{
    return m_codeBlock->addRegExp(r);
}

RegisterID* BytecodeGenerator::emitMove(RegisterID* dst, RegisterID* src)
{
    m_staticPropertyAnalyzer.mov(dst->index(), src->index());

    emitOpcode(op_mov);
    instructions().append(dst->index());
    instructions().append(src->index());
    return dst;
}

RegisterID* BytecodeGenerator::emitUnaryOp(OpcodeID opcodeID, RegisterID* dst, RegisterID* src)
{
    emitOpcode(opcodeID);
    instructions().append(dst->index());
    instructions().append(src->index());
    return dst;
}

RegisterID* BytecodeGenerator::emitInc(RegisterID* srcDst)
{
    emitOpcode(op_inc);
    instructions().append(srcDst->index());
    return srcDst;
}

RegisterID* BytecodeGenerator::emitDec(RegisterID* srcDst)
{
    emitOpcode(op_dec);
    instructions().append(srcDst->index());
    return srcDst;
}

RegisterID* BytecodeGenerator::emitBinaryOp(OpcodeID opcodeID, RegisterID* dst, RegisterID* src1, RegisterID* src2, OperandTypes types)
{
    emitOpcode(opcodeID);
    instructions().append(dst->index());
    instructions().append(src1->index());
    instructions().append(src2->index());

    if (opcodeID == op_bitor || opcodeID == op_bitand || opcodeID == op_bitxor ||
        opcodeID == op_add || opcodeID == op_mul || opcodeID == op_sub || opcodeID == op_div)
        instructions().append(types.toInt());

    return dst;
}

RegisterID* BytecodeGenerator::emitEqualityOp(OpcodeID opcodeID, RegisterID* dst, RegisterID* src1, RegisterID* src2)
{
    if (m_lastOpcodeID == op_typeof) {
        int dstIndex;
        int srcIndex;

        retrieveLastUnaryOp(dstIndex, srcIndex);

        if (src1->index() == dstIndex
            && src1->isTemporary()
            && m_codeBlock->isConstantRegisterIndex(src2->index())
            && m_codeBlock->constantRegister(src2->index()).get().isString()) {
            const String& value = asString(m_codeBlock->constantRegister(src2->index()).get())->tryGetValue();
            if (value == "undefined") {
                rewindUnaryOp();
                emitOpcode(op_is_undefined);
                instructions().append(dst->index());
                instructions().append(srcIndex);
                return dst;
            }
            if (value == "boolean") {
                rewindUnaryOp();
                emitOpcode(op_is_boolean);
                instructions().append(dst->index());
                instructions().append(srcIndex);
                return dst;
            }
            if (value == "number") {
                rewindUnaryOp();
                emitOpcode(op_is_number);
                instructions().append(dst->index());
                instructions().append(srcIndex);
                return dst;
            }
            if (value == "string") {
                rewindUnaryOp();
                emitOpcode(op_is_string);
                instructions().append(dst->index());
                instructions().append(srcIndex);
                return dst;
            }
            if (value == "object") {
                rewindUnaryOp();
                emitOpcode(op_is_object);
                instructions().append(dst->index());
                instructions().append(srcIndex);
                return dst;
            }
            if (value == "function") {
                rewindUnaryOp();
                emitOpcode(op_is_function);
                instructions().append(dst->index());
                instructions().append(srcIndex);
                return dst;
            }
        }
    }

    emitOpcode(opcodeID);
    instructions().append(dst->index());
    instructions().append(src1->index());
    instructions().append(src2->index());
    return dst;
}

RegisterID* BytecodeGenerator::emitLoad(RegisterID* dst, bool b)
{
    return emitLoad(dst, jsBoolean(b));
}

RegisterID* BytecodeGenerator::emitLoad(RegisterID* dst, double number)
{
    // FIXME: Our hash tables won't hold infinity, so we make a new JSValue each time.
    // Later we can do the extra work to handle that like the other cases.  They also don't
    // work correctly with NaN as a key.
    if (std::isnan(number) || number == HashTraits<double>::emptyValue() || HashTraits<double>::isDeletedValue(number))
        return emitLoad(dst, jsNumber(number));
    JSValue& valueInMap = m_numberMap.add(number, JSValue()).iterator->value;
    if (!valueInMap)
        valueInMap = jsNumber(number);
    return emitLoad(dst, valueInMap);
}

RegisterID* BytecodeGenerator::emitLoad(RegisterID* dst, const Identifier& identifier)
{
    JSString*& stringInMap = m_stringMap.add(identifier.impl(), 0).iterator->value;
    if (!stringInMap)
        stringInMap = jsOwnedString(vm(), identifier.string());
    return emitLoad(dst, JSValue(stringInMap));
}

RegisterID* BytecodeGenerator::emitLoad(RegisterID* dst, JSValue v)
{
    RegisterID* constantID = addConstantValue(v);
    if (dst)
        return emitMove(dst, constantID);
    return constantID;
}

RegisterID* BytecodeGenerator::emitLoadGlobalObject(RegisterID* dst)
{
    if (!m_globalObjectRegister) {
        int index = m_nextConstantOffset;
        m_constantPoolRegisters.append(FirstConstantRegisterIndex + m_nextConstantOffset);
        ++m_nextConstantOffset;
        m_codeBlock->addConstant(JSValue());
        m_globalObjectRegister = &m_constantPoolRegisters[index];
        m_codeBlock->setGlobalObjectRegister(index);
    }
    if (dst)
        emitMove(dst, m_globalObjectRegister);
    return m_globalObjectRegister;
}

ResolveResult BytecodeGenerator::resolve(const Identifier& property)
{
    if (property == propertyNames().thisIdentifier)
        return ResolveResult::registerResolve(thisRegister(), ResolveResult::ReadOnlyFlag);

    // Check if the property should be allocated in a register.
    if (m_codeType != GlobalCode && shouldOptimizeLocals() && m_symbolTable) {
        SymbolTableEntry entry = symbolTable().get(property.impl());
        if (!entry.isNull()) {
            if (property == propertyNames().arguments)
                createArgumentsIfNecessary();
            unsigned flags = entry.isReadOnly() ? ResolveResult::ReadOnlyFlag : 0;
            RegisterID* local = createLazyRegisterIfNecessary(&registerFor(entry.getIndex()));
            return ResolveResult::registerResolve(local, flags);
        }
    }
    // Cases where we cannot statically optimize the lookup.
    if (property == propertyNames().arguments || !canOptimizeNonLocals())
        return ResolveResult::dynamicResolve();

    if (!m_scope || m_codeType != FunctionCode || m_shouldEmitDebugHooks)
        return ResolveResult::dynamicResolve();

    ScopeChainIterator iter = m_scope->begin();
    ScopeChainIterator end = m_scope->end();
    size_t depth = m_codeBlock->needsFullScopeChain();
    unsigned flags = 0;
    for (; iter != end; ++iter, ++depth) {
        JSObject* currentScope = iter.get();
        if (!currentScope->isStaticScopeObject())
            return ResolveResult::dynamicResolve();

        JSSymbolTableObject* currentVariableObject = jsCast<JSSymbolTableObject*>(currentScope);
        SymbolTableEntry entry = currentVariableObject->symbolTable()->get(property.impl());

        // Found the property
        if (!entry.isNull()) {
            if (entry.isReadOnly())
                flags |= ResolveResult::ReadOnlyFlag;
            if (++iter == end)
                return ResolveResult::dynamicResolve();
#if !ASSERT_DISABLED
            if (JSActivation* activation = jsDynamicCast<JSActivation*>(currentVariableObject))
                ASSERT(activation->isValid(entry));
#endif
            return ResolveResult::lexicalResolve(entry.getIndex(), depth, flags);
        }
        bool scopeRequiresDynamicChecks = false;
        if (currentVariableObject->isDynamicScope(scopeRequiresDynamicChecks))
            break;
        if (scopeRequiresDynamicChecks)
            flags |= ResolveResult::DynamicFlag;
    }

    return ResolveResult::dynamicResolve();
}

ResolveResult BytecodeGenerator::resolveConstDecl(const Identifier& property)
{
    // Register-allocated const declarations.
    if (m_codeType == FunctionCode && m_symbolTable) {
        SymbolTableEntry entry = symbolTable().get(property.impl());
        if (!entry.isNull()) {
            unsigned flags = entry.isReadOnly() ? ResolveResult::ReadOnlyFlag : 0;
            RegisterID* local = createLazyRegisterIfNecessary(&registerFor(entry.getIndex()));
            return ResolveResult::registerResolve(local, flags);
        }
    }

    return ResolveResult::dynamicResolve();
}

void BytecodeGenerator::emitCheckHasInstance(RegisterID* dst, RegisterID* value, RegisterID* base, Label* target)
{
    size_t begin = instructions().size();
    emitOpcode(op_check_has_instance);
    instructions().append(dst->index());
    instructions().append(value->index());
    instructions().append(base->index());
    instructions().append(target->bind(begin, instructions().size()));
}

RegisterID* BytecodeGenerator::emitInstanceOf(RegisterID* dst, RegisterID* value, RegisterID* basePrototype)
{ 
    emitOpcode(op_instanceof);
    instructions().append(dst->index());
    instructions().append(value->index());
    instructions().append(basePrototype->index());
    return dst;
}

bool BytecodeGenerator::shouldAvoidResolveGlobal()
{
    return !m_labelScopes.size();
}

RegisterID* BytecodeGenerator::emitResolve(RegisterID* dst, const ResolveResult& resolveResult, const Identifier& property)
{

    if (resolveResult.isStatic())
        return emitGetStaticVar(dst, resolveResult, property);

    UnlinkedValueProfile profile = emitProfiledOpcode(op_resolve);
    instructions().append(kill(dst));
    instructions().append(addConstant(property));
    instructions().append(getResolveOperations(property));
    instructions().append(profile);
    return dst;
}

RegisterID* BytecodeGenerator::emitResolveBase(RegisterID* dst, const ResolveResult& resolveResult, const Identifier& property)
{
    if (!resolveResult.isDynamic()) {
        // Global object is the base
        return emitLoadGlobalObject(dst);
    }

    // We can't optimise at all :-(
    UnlinkedValueProfile profile = emitProfiledOpcode(op_resolve_base);
    instructions().append(kill(dst));
    instructions().append(addConstant(property));
    instructions().append(false);
    instructions().append(getResolveBaseOperations(property));
    instructions().append(0);
    instructions().append(profile);
    return dst;
}

RegisterID* BytecodeGenerator::emitResolveBaseForPut(RegisterID* dst, const ResolveResult&, const Identifier& property, NonlocalResolveInfo& verifier)
{
    // We can't optimise at all :-(
    UnlinkedValueProfile profile = emitProfiledOpcode(op_resolve_base);
    instructions().append(kill(dst));
    instructions().append(addConstant(property));
    instructions().append(m_codeBlock->isStrictMode());
    uint32_t putToBaseIndex = 0;
    instructions().append(getResolveBaseForPutOperations(property, putToBaseIndex));
    verifier.resolved(putToBaseIndex);
    instructions().append(putToBaseIndex);
    instructions().append(profile);
    return dst;
}

RegisterID* BytecodeGenerator::emitResolveWithBaseForPut(RegisterID* baseDst, RegisterID* propDst, const ResolveResult& resolveResult, const Identifier& property, NonlocalResolveInfo& verifier)
{
    ASSERT_UNUSED(resolveResult, !resolveResult.isStatic());
    UnlinkedValueProfile profile = emitProfiledOpcode(op_resolve_with_base);
    instructions().append(kill(baseDst));
    instructions().append(propDst->index());
    instructions().append(addConstant(property));
    uint32_t putToBaseIndex = 0;
    instructions().append(getResolveWithBaseForPutOperations(property, putToBaseIndex));
    verifier.resolved(putToBaseIndex);
    instructions().append(putToBaseIndex);
    instructions().append(profile);
    return baseDst;
}

RegisterID* BytecodeGenerator::emitResolveWithThis(RegisterID* baseDst, RegisterID* propDst, const ResolveResult& resolveResult, const Identifier& property)
{
    if (resolveResult.isStatic()) {
        emitLoad(baseDst, jsUndefined());
        emitGetStaticVar(propDst, resolveResult, property);
        return baseDst;
    }

    UnlinkedValueProfile profile = emitProfiledOpcode(op_resolve_with_this);
    instructions().append(kill(baseDst));
    instructions().append(propDst->index());
    instructions().append(addConstant(property));
    instructions().append(getResolveWithThisOperations(property));
    instructions().append(profile);
    return baseDst;
}

RegisterID* BytecodeGenerator::emitGetStaticVar(RegisterID* dst, const ResolveResult& resolveResult, const Identifier&)
{
    ASSERT(m_codeType == FunctionCode);
    switch (resolveResult.type()) {
    case ResolveResult::Register:
    case ResolveResult::ReadOnlyRegister:
        if (dst == ignoredResult())
            return 0;
        return moveToDestinationIfNeeded(dst, resolveResult.local());

    case ResolveResult::Lexical:
    case ResolveResult::ReadOnlyLexical: {
        UnlinkedValueProfile profile = emitProfiledOpcode(op_get_scoped_var);
        instructions().append(dst->index());
        instructions().append(resolveResult.index());
        instructions().append(resolveResult.depth());
        instructions().append(profile);
        return dst;
    }

    default:
        RELEASE_ASSERT_NOT_REACHED();
        return 0;
    }
}

RegisterID* BytecodeGenerator::emitInitGlobalConst(const Identifier& identifier, RegisterID* value)
{
    ASSERT(m_codeType == GlobalCode);
    emitOpcode(op_init_global_const_nop);
    instructions().append(0);
    instructions().append(value->index());
    instructions().append(0);
    instructions().append(addConstant(identifier));
    return value;
}

RegisterID* BytecodeGenerator::emitPutStaticVar(const ResolveResult& resolveResult, const Identifier&, RegisterID* value)
{
    ASSERT(m_codeType == FunctionCode);
    switch (resolveResult.type()) {
    case ResolveResult::Register:
    case ResolveResult::ReadOnlyRegister:
        return moveToDestinationIfNeeded(resolveResult.local(), value);

    case ResolveResult::Lexical:
    case ResolveResult::ReadOnlyLexical:
        emitOpcode(op_put_scoped_var);
        instructions().append(resolveResult.index());
        instructions().append(resolveResult.depth());
        instructions().append(value->index());
        return value;

    default:
        RELEASE_ASSERT_NOT_REACHED();
        return 0;
    }
}

RegisterID* BytecodeGenerator::emitGetById(RegisterID* dst, RegisterID* base, const Identifier& property)
{
    m_codeBlock->addPropertyAccessInstruction(instructions().size());

    UnlinkedValueProfile profile = emitProfiledOpcode(op_get_by_id);
    instructions().append(kill(dst));
    instructions().append(base->index());
    instructions().append(addConstant(property));
    instructions().append(0);
    instructions().append(0);
    instructions().append(0);
    instructions().append(0);
    instructions().append(profile);
    return dst;
}

RegisterID* BytecodeGenerator::emitGetArgumentsLength(RegisterID* dst, RegisterID* base)
{
    emitOpcode(op_get_arguments_length);
    instructions().append(dst->index());
    ASSERT(base->index() == m_codeBlock->argumentsRegister());
    instructions().append(base->index());
    instructions().append(addConstant(propertyNames().length));
    return dst;
}

RegisterID* BytecodeGenerator::emitPutById(RegisterID* base, const Identifier& property, RegisterID* value)
{
    unsigned propertyIndex = addConstant(property);

    m_staticPropertyAnalyzer.putById(base->index(), propertyIndex);

    m_codeBlock->addPropertyAccessInstruction(instructions().size());

    emitOpcode(op_put_by_id);
    instructions().append(base->index());
    instructions().append(propertyIndex);
    instructions().append(value->index());
    instructions().append(0);
    instructions().append(0);
    instructions().append(0);
    instructions().append(0);
    instructions().append(0);
    return value;
}

RegisterID* BytecodeGenerator::emitPutToBase(RegisterID* base, const Identifier& property, RegisterID* value, NonlocalResolveInfo& resolveInfo)
{
    emitOpcode(op_put_to_base);
    instructions().append(base->index());
    instructions().append(addConstant(property));
    instructions().append(value->index());
    instructions().append(resolveInfo.put());
    return value;
}

RegisterID* BytecodeGenerator::emitDirectPutById(RegisterID* base, const Identifier& property, RegisterID* value)
{
    unsigned propertyIndex = addConstant(property);

    m_staticPropertyAnalyzer.putById(base->index(), propertyIndex);

    m_codeBlock->addPropertyAccessInstruction(instructions().size());
    
    emitOpcode(op_put_by_id);
    instructions().append(base->index());
    instructions().append(propertyIndex);
    instructions().append(value->index());
    instructions().append(0);
    instructions().append(0);
    instructions().append(0);
    instructions().append(0);
    instructions().append(
        property != m_vm->propertyNames->underscoreProto
        && PropertyName(property).asIndex() == PropertyName::NotAnIndex);
    return value;
}

void BytecodeGenerator::emitPutGetterSetter(RegisterID* base, const Identifier& property, RegisterID* getter, RegisterID* setter)
{
    unsigned propertyIndex = addConstant(property);

    m_staticPropertyAnalyzer.putById(base->index(), propertyIndex);

    emitOpcode(op_put_getter_setter);
    instructions().append(base->index());
    instructions().append(propertyIndex);
    instructions().append(getter->index());
    instructions().append(setter->index());
}

RegisterID* BytecodeGenerator::emitDeleteById(RegisterID* dst, RegisterID* base, const Identifier& property)
{
    emitOpcode(op_del_by_id);
    instructions().append(dst->index());
    instructions().append(base->index());
    instructions().append(addConstant(property));
    return dst;
}

RegisterID* BytecodeGenerator::emitGetArgumentByVal(RegisterID* dst, RegisterID* base, RegisterID* property)
{
    UnlinkedArrayProfile arrayProfile = newArrayProfile();
    UnlinkedValueProfile profile = emitProfiledOpcode(op_get_argument_by_val);
    instructions().append(kill(dst));
    ASSERT(base->index() == m_codeBlock->argumentsRegister());
    instructions().append(base->index());
    instructions().append(property->index());
    instructions().append(arrayProfile);
    instructions().append(profile);
    return dst;
}

RegisterID* BytecodeGenerator::emitGetByVal(RegisterID* dst, RegisterID* base, RegisterID* property)
{
    for (size_t i = m_forInContextStack.size(); i > 0; i--) {
        ForInContext& context = m_forInContextStack[i - 1];
        if (context.propertyRegister == property) {
            emitOpcode(op_get_by_pname);
            instructions().append(dst->index());
            instructions().append(base->index());
            instructions().append(property->index());
            instructions().append(context.expectedSubscriptRegister->index());
            instructions().append(context.iterRegister->index());
            instructions().append(context.indexRegister->index());
            return dst;
        }
    }
    UnlinkedArrayProfile arrayProfile = newArrayProfile();
    UnlinkedValueProfile profile = emitProfiledOpcode(op_get_by_val);
    instructions().append(kill(dst));
    instructions().append(base->index());
    instructions().append(property->index());
    instructions().append(arrayProfile);
    instructions().append(profile);
    return dst;
}

RegisterID* BytecodeGenerator::emitPutByVal(RegisterID* base, RegisterID* property, RegisterID* value)
{
    UnlinkedArrayProfile arrayProfile = newArrayProfile();
    emitOpcode(op_put_by_val);
    instructions().append(base->index());
    instructions().append(property->index());
    instructions().append(value->index());
    instructions().append(arrayProfile);
    return value;
}

RegisterID* BytecodeGenerator::emitDeleteByVal(RegisterID* dst, RegisterID* base, RegisterID* property)
{
    emitOpcode(op_del_by_val);
    instructions().append(dst->index());
    instructions().append(base->index());
    instructions().append(property->index());
    return dst;
}

RegisterID* BytecodeGenerator::emitPutByIndex(RegisterID* base, unsigned index, RegisterID* value)
{
    emitOpcode(op_put_by_index);
    instructions().append(base->index());
    instructions().append(index);
    instructions().append(value->index());
    return value;
}

RegisterID* BytecodeGenerator::emitCreateThis(RegisterID* dst)
{
    RefPtr<RegisterID> func = newTemporary(); 

    UnlinkedValueProfile profile = emitProfiledOpcode(op_get_callee);
    instructions().append(func->index());
    instructions().append(profile);

    size_t begin = instructions().size();
    m_staticPropertyAnalyzer.createThis(m_thisRegister.index(), begin + 3);

    emitOpcode(op_create_this); 
    instructions().append(m_thisRegister.index()); 
    instructions().append(func->index()); 
    instructions().append(0);
    return dst;
}

RegisterID* BytecodeGenerator::emitNewObject(RegisterID* dst)
{
    size_t begin = instructions().size();
    m_staticPropertyAnalyzer.newObject(dst->index(), begin + 2);

    emitOpcode(op_new_object);
    instructions().append(dst->index());
    instructions().append(0);
    instructions().append(newObjectAllocationProfile());
    return dst;
}

unsigned BytecodeGenerator::addConstantBuffer(unsigned length)
{
    return m_codeBlock->addConstantBuffer(length);
}

JSString* BytecodeGenerator::addStringConstant(const Identifier& identifier)
{
    JSString*& stringInMap = m_stringMap.add(identifier.impl(), 0).iterator->value;
    if (!stringInMap) {
        stringInMap = jsString(vm(), identifier.string());
        addConstantValue(stringInMap);
    }
    return stringInMap;
}

RegisterID* BytecodeGenerator::emitNewArray(RegisterID* dst, ElementNode* elements, unsigned length)
{
#if !ASSERT_DISABLED
    unsigned checkLength = 0;
#endif
    bool hadVariableExpression = false;
    if (length) {
        for (ElementNode* n = elements; n; n = n->next()) {
            if (!n->value()->isConstant()) {
                hadVariableExpression = true;
                break;
            }
            if (n->elision())
                break;
#if !ASSERT_DISABLED
            checkLength++;
#endif
        }
        if (!hadVariableExpression) {
            ASSERT(length == checkLength);
            unsigned constantBufferIndex = addConstantBuffer(length);
            JSValue* constantBuffer = m_codeBlock->constantBuffer(constantBufferIndex).data();
            unsigned index = 0;
            for (ElementNode* n = elements; index < length; n = n->next()) {
                ASSERT(n->value()->isConstant());
                constantBuffer[index++] = static_cast<ConstantNode*>(n->value())->jsValue(*this);
            }
            emitOpcode(op_new_array_buffer);
            instructions().append(dst->index());
            instructions().append(constantBufferIndex);
            instructions().append(length);
            instructions().append(newArrayAllocationProfile());
            return dst;
        }
    }

    Vector<RefPtr<RegisterID>, 16, UnsafeVectorOverflow> argv;
    for (ElementNode* n = elements; n; n = n->next()) {
        if (n->elision())
            break;
        argv.append(newTemporary());
        // op_new_array requires the initial values to be a sequential range of registers
        ASSERT(argv.size() == 1 || argv[argv.size() - 1]->index() == argv[argv.size() - 2]->index() + 1);
        emitNode(argv.last().get(), n->value());
    }
    emitOpcode(op_new_array);
    instructions().append(dst->index());
    instructions().append(argv.size() ? argv[0]->index() : 0); // argv
    instructions().append(argv.size()); // argc
    instructions().append(newArrayAllocationProfile());
    return dst;
}

RegisterID* BytecodeGenerator::emitNewFunction(RegisterID* dst, FunctionBodyNode* function)
{
    return emitNewFunctionInternal(dst, m_codeBlock->addFunctionDecl(makeFunction(function)), false);
}

RegisterID* BytecodeGenerator::emitLazyNewFunction(RegisterID* dst, FunctionBodyNode* function)
{
    FunctionOffsetMap::AddResult ptr = m_functionOffsets.add(function, 0);
    if (ptr.isNewEntry)
        ptr.iterator->value = m_codeBlock->addFunctionDecl(makeFunction(function));
    return emitNewFunctionInternal(dst, ptr.iterator->value, true);
}

RegisterID* BytecodeGenerator::emitNewFunctionInternal(RegisterID* dst, unsigned index, bool doNullCheck)
{
    createActivationIfNecessary();
    emitOpcode(op_new_func);
    instructions().append(dst->index());
    instructions().append(index);
    instructions().append(doNullCheck);
    return dst;
}

RegisterID* BytecodeGenerator::emitNewRegExp(RegisterID* dst, RegExp* regExp)
{
    emitOpcode(op_new_regexp);
    instructions().append(dst->index());
    instructions().append(addRegExp(regExp));
    return dst;
}

RegisterID* BytecodeGenerator::emitNewFunctionExpression(RegisterID* r0, FuncExprNode* n)
{
    FunctionBodyNode* function = n->body();
    unsigned index = m_codeBlock->addFunctionExpr(makeFunction(function));
    
    createActivationIfNecessary();
    emitOpcode(op_new_func_exp);
    instructions().append(r0->index());
    instructions().append(index);
    return r0;
}

RegisterID* BytecodeGenerator::emitCall(RegisterID* dst, RegisterID* func, ExpectedFunction expectedFunction, CallArguments& callArguments, const JSTextPosition& divot, const JSTextPosition& divotStart, const JSTextPosition& divotEnd)
{
    return emitCall(op_call, dst, func, expectedFunction, callArguments, divot, divotStart, divotEnd);
}

void BytecodeGenerator::createArgumentsIfNecessary()
{
    if (m_codeType != FunctionCode)
        return;
    
    if (!m_codeBlock->usesArguments())
        return;

    // If we're in strict mode we tear off the arguments on function
    // entry, so there's no need to check if we need to create them
    // now
    if (m_codeBlock->isStrictMode())
        return;

    emitOpcode(op_create_arguments);
    instructions().append(m_codeBlock->argumentsRegister());
}

void BytecodeGenerator::createActivationIfNecessary()
{
    if (m_hasCreatedActivation)
        return;
    if (!m_codeBlock->needsFullScopeChain())
        return;
    emitOpcode(op_create_activation);
    instructions().append(m_activationRegister->index());
}

RegisterID* BytecodeGenerator::emitCallEval(RegisterID* dst, RegisterID* func, CallArguments& callArguments, const JSTextPosition& divot, const JSTextPosition& divotStart, const JSTextPosition& divotEnd)
{
    return emitCall(op_call_eval, dst, func, NoExpectedFunction, callArguments, divot, divotStart, divotEnd);
}

ExpectedFunction BytecodeGenerator::expectedFunctionForIdentifier(const Identifier& identifier)
{
    if (identifier == m_vm->propertyNames->Object)
        return ExpectObjectConstructor;
    if (identifier == m_vm->propertyNames->Array)
        return ExpectArrayConstructor;
    return NoExpectedFunction;
}

ExpectedFunction BytecodeGenerator::emitExpectedFunctionSnippet(RegisterID* dst, RegisterID* func, ExpectedFunction expectedFunction, CallArguments& callArguments, Label* done)
{
    RefPtr<Label> realCall = newLabel();
    switch (expectedFunction) {
    case ExpectObjectConstructor: {
        // If the number of arguments is non-zero, then we can't do anything interesting.
        if (callArguments.argumentCountIncludingThis() >= 2)
            return NoExpectedFunction;
        
        size_t begin = instructions().size();
        emitOpcode(op_jneq_ptr);
        instructions().append(func->index());
        instructions().append(Special::ObjectConstructor);
        instructions().append(realCall->bind(begin, instructions().size()));
        
        if (dst != ignoredResult())
            emitNewObject(dst);
        break;
    }
        
    case ExpectArrayConstructor: {
        // If you're doing anything other than "new Array()" or "new Array(foo)" then we
        // don't do inline it, for now. The only reason is that call arguments are in
        // the opposite order of what op_new_array expects, so we'd either need to change
        // how op_new_array works or we'd need an op_new_array_reverse. Neither of these
        // things sounds like it's worth it.
        if (callArguments.argumentCountIncludingThis() > 2)
            return NoExpectedFunction;
        
        size_t begin = instructions().size();
        emitOpcode(op_jneq_ptr);
        instructions().append(func->index());
        instructions().append(Special::ArrayConstructor);
        instructions().append(realCall->bind(begin, instructions().size()));
        
        if (dst != ignoredResult()) {
            if (callArguments.argumentCountIncludingThis() == 2) {
                emitOpcode(op_new_array_with_size);
                instructions().append(dst->index());
                instructions().append(callArguments.argumentRegister(0)->index());
                instructions().append(newArrayAllocationProfile());
            } else {
                ASSERT(callArguments.argumentCountIncludingThis() == 1);
                emitOpcode(op_new_array);
                instructions().append(dst->index());
                instructions().append(0);
                instructions().append(0);
                instructions().append(newArrayAllocationProfile());
            }
        }
        break;
    }
        
    default:
        ASSERT(expectedFunction == NoExpectedFunction);
        return NoExpectedFunction;
    }
    
    size_t begin = instructions().size();
    emitOpcode(op_jmp);
    instructions().append(done->bind(begin, instructions().size()));
    emitLabel(realCall.get());
    
    return expectedFunction;
}

RegisterID* BytecodeGenerator::emitCall(OpcodeID opcodeID, RegisterID* dst, RegisterID* func, ExpectedFunction expectedFunction, CallArguments& callArguments, const JSTextPosition& divot, const JSTextPosition& divotStart, const JSTextPosition& divotEnd)
{
    ASSERT(opcodeID == op_call || opcodeID == op_call_eval);
    ASSERT(func->refCount());

    if (m_shouldEmitProfileHooks)
        emitMove(callArguments.profileHookRegister(), func);

    // Generate code for arguments.
    unsigned argument = 0;
    for (ArgumentListNode* n = callArguments.argumentsNode()->m_listNode; n; n = n->m_next)
        emitNode(callArguments.argumentRegister(argument++), n);

    // Reserve space for call frame.
    Vector<RefPtr<RegisterID>, JSStack::CallFrameHeaderSize, UnsafeVectorOverflow> callFrame;
    for (int i = 0; i < JSStack::CallFrameHeaderSize; ++i)
        callFrame.append(newTemporary());

    if (m_shouldEmitProfileHooks) {
        emitOpcode(op_profile_will_call);
        instructions().append(callArguments.profileHookRegister()->index());
    }

    emitExpressionInfo(divot, divotStart, divotEnd);

    RefPtr<Label> done = newLabel();
    expectedFunction = emitExpectedFunctionSnippet(dst, func, expectedFunction, callArguments, done.get());
    
    // Emit call.
    UnlinkedArrayProfile arrayProfile = newArrayProfile();
    emitOpcode(opcodeID);
    instructions().append(func->index()); // func
    instructions().append(callArguments.argumentCountIncludingThis()); // argCount
    instructions().append(callArguments.registerOffset()); // registerOffset
#if ENABLE(LLINT)
    instructions().append(m_codeBlock->addLLIntCallLinkInfo());
#else
    instructions().append(0);
#endif
    instructions().append(arrayProfile);
    if (dst != ignoredResult()) {
        UnlinkedValueProfile profile = emitProfiledOpcode(op_call_put_result);
        instructions().append(kill(dst));
        instructions().append(profile);
    }
    
    if (expectedFunction != NoExpectedFunction)
        emitLabel(done.get());

    if (m_shouldEmitProfileHooks) {
        emitOpcode(op_profile_did_call);
        instructions().append(callArguments.profileHookRegister()->index());
    }

    return dst;
}

RegisterID* BytecodeGenerator::emitCallVarargs(RegisterID* dst, RegisterID* func, RegisterID* thisRegister, RegisterID* arguments, RegisterID* firstFreeRegister, RegisterID* profileHookRegister, const JSTextPosition& divot, const JSTextPosition& divotStart, const JSTextPosition& divotEnd)
{
    if (m_shouldEmitProfileHooks) {
        emitMove(profileHookRegister, func);
        emitOpcode(op_profile_will_call);
        instructions().append(profileHookRegister->index());
    }
    
    emitExpressionInfo(divot, divotStart, divotEnd);

    // Emit call.
    emitOpcode(op_call_varargs);
    instructions().append(func->index());
    instructions().append(thisRegister->index());
    instructions().append(arguments->index());
    instructions().append(firstFreeRegister->index());
    if (dst != ignoredResult()) {
        UnlinkedValueProfile profile = emitProfiledOpcode(op_call_put_result);
        instructions().append(kill(dst));
        instructions().append(profile);
    }
    if (m_shouldEmitProfileHooks) {
        emitOpcode(op_profile_did_call);
        instructions().append(profileHookRegister->index());
    }
    return dst;
}

RegisterID* BytecodeGenerator::emitReturn(RegisterID* src)
{
    if (m_codeBlock->needsFullScopeChain()) {
        emitOpcode(op_tear_off_activation);
        instructions().append(m_activationRegister->index());
    }

    if (m_codeBlock->usesArguments() && m_codeBlock->numParameters() != 1 && !m_codeBlock->isStrictMode()) {
        emitOpcode(op_tear_off_arguments);
        instructions().append(m_codeBlock->argumentsRegister());
        instructions().append(m_activationRegister ? m_activationRegister->index() : emitLoad(0, JSValue())->index());
    }

    // Constructors use op_ret_object_or_this to check the result is an
    // object, unless we can trivially determine the check is not
    // necessary (currently, if the return value is 'this').
    if (isConstructor() && (src->index() != m_thisRegister.index())) {
        emitOpcode(op_ret_object_or_this);
        instructions().append(src->index());
        instructions().append(m_thisRegister.index());
        return src;
    }
    return emitUnaryNoDstOp(op_ret, src);
}

RegisterID* BytecodeGenerator::emitUnaryNoDstOp(OpcodeID opcodeID, RegisterID* src)
{
    emitOpcode(opcodeID);
    instructions().append(src->index());
    return src;
}

RegisterID* BytecodeGenerator::emitConstruct(RegisterID* dst, RegisterID* func, ExpectedFunction expectedFunction, CallArguments& callArguments, const JSTextPosition& divot, const JSTextPosition& divotStart, const JSTextPosition& divotEnd)
{
    ASSERT(func->refCount());

    if (m_shouldEmitProfileHooks)
        emitMove(callArguments.profileHookRegister(), func);

    // Generate code for arguments.
    unsigned argument = 0;
    if (ArgumentsNode* argumentsNode = callArguments.argumentsNode()) {
        for (ArgumentListNode* n = argumentsNode->m_listNode; n; n = n->m_next)
            emitNode(callArguments.argumentRegister(argument++), n);
    }

    if (m_shouldEmitProfileHooks) {
        emitOpcode(op_profile_will_call);
        instructions().append(callArguments.profileHookRegister()->index());
    }

    // Reserve space for call frame.
    Vector<RefPtr<RegisterID>, JSStack::CallFrameHeaderSize, UnsafeVectorOverflow> callFrame;
    for (int i = 0; i < JSStack::CallFrameHeaderSize; ++i)
        callFrame.append(newTemporary());

    emitExpressionInfo(divot, divotStart, divotEnd);
    
    RefPtr<Label> done = newLabel();
    expectedFunction = emitExpectedFunctionSnippet(dst, func, expectedFunction, callArguments, done.get());

    emitOpcode(op_construct);
    instructions().append(func->index()); // func
    instructions().append(callArguments.argumentCountIncludingThis()); // argCount
    instructions().append(callArguments.registerOffset()); // registerOffset
#if ENABLE(LLINT)
    instructions().append(m_codeBlock->addLLIntCallLinkInfo());
#else
    instructions().append(0);
#endif
    instructions().append(0);
    if (dst != ignoredResult()) {
        UnlinkedValueProfile profile = emitProfiledOpcode(op_call_put_result);
        instructions().append(kill(dst));
        instructions().append(profile);
    }

    if (expectedFunction != NoExpectedFunction)
        emitLabel(done.get());

    if (m_shouldEmitProfileHooks) {
        emitOpcode(op_profile_did_call);
        instructions().append(callArguments.profileHookRegister()->index());
    }

    return dst;
}

RegisterID* BytecodeGenerator::emitStrcat(RegisterID* dst, RegisterID* src, int count)
{
    emitOpcode(op_strcat);
    instructions().append(dst->index());
    instructions().append(src->index());
    instructions().append(count);

    return dst;
}

void BytecodeGenerator::emitToPrimitive(RegisterID* dst, RegisterID* src)
{
    emitOpcode(op_to_primitive);
    instructions().append(dst->index());
    instructions().append(src->index());
}

RegisterID* BytecodeGenerator::emitPushWithScope(RegisterID* scope)
{
    ControlFlowContext context;
    context.isFinallyBlock = false;
    m_scopeContextStack.append(context);
    m_dynamicScopeDepth++;

    return emitUnaryNoDstOp(op_push_with_scope, scope);
}

void BytecodeGenerator::emitPopScope()
{
    ASSERT(m_scopeContextStack.size());
    ASSERT(!m_scopeContextStack.last().isFinallyBlock);

    emitOpcode(op_pop_scope);

    m_scopeContextStack.removeLast();
    m_dynamicScopeDepth--;
}

void BytecodeGenerator::emitDebugHook(DebugHookID debugHookID, unsigned firstLine, unsigned lastLine, unsigned charOffset, unsigned lineStart)
{
#if ENABLE(DEBUG_WITH_BREAKPOINT)
    if (debugHookID != DidReachBreakpoint)
        return;
#else
    if (!m_shouldEmitDebugHooks)
        return;
#endif
    JSTextPosition divot(firstLine, charOffset, lineStart);
    emitExpressionInfo(divot, divot, divot);
    unsigned charPosition = charOffset - m_scopeNode->source().startOffset();
    emitOpcode(op_debug);
    instructions().append(debugHookID);
    instructions().append(firstLine);
    instructions().append(lastLine);
    instructions().append(charPosition);
}

void BytecodeGenerator::pushFinallyContext(StatementNode* finallyBlock)
{
    ControlFlowContext scope;
    scope.isFinallyBlock = true;
    FinallyContext context = {
        finallyBlock,
        static_cast<unsigned>(m_scopeContextStack.size()),
        static_cast<unsigned>(m_switchContextStack.size()),
        static_cast<unsigned>(m_forInContextStack.size()),
        static_cast<unsigned>(m_tryContextStack.size()),
        static_cast<unsigned>(m_labelScopes.size()),
        m_finallyDepth,
        m_dynamicScopeDepth
    };
    scope.finallyContext = context;
    m_scopeContextStack.append(scope);
    m_finallyDepth++;
}

void BytecodeGenerator::popFinallyContext()
{
    ASSERT(m_scopeContextStack.size());
    ASSERT(m_scopeContextStack.last().isFinallyBlock);
    ASSERT(m_finallyDepth > 0);
    m_scopeContextStack.removeLast();
    m_finallyDepth--;
}

LabelScope* BytecodeGenerator::breakTarget(const Identifier& name)
{
    // Reclaim free label scopes.
    //
    // The condition was previously coded as 'm_labelScopes.size() && !m_labelScopes.last().refCount()',
    // however sometimes this appears to lead to GCC going a little haywire and entering the loop with
    // size 0, leading to segfaulty badness.  We are yet to identify a valid cause within our code to
    // cause the GCC codegen to misbehave in this fashion, and as such the following refactoring of the
    // loop condition is a workaround.
    while (m_labelScopes.size()) {
        if  (m_labelScopes.last().refCount())
            break;
        m_labelScopes.removeLast();
    }

    if (!m_labelScopes.size())
        return 0;

    // We special-case the following, which is a syntax error in Firefox:
    // label:
    //     break;
    if (name.isEmpty()) {
        for (int i = m_labelScopes.size() - 1; i >= 0; --i) {
            LabelScope* scope = &m_labelScopes[i];
            if (scope->type() != LabelScope::NamedLabel) {
                ASSERT(scope->breakTarget());
                return scope;
            }
        }
        return 0;
    }

    for (int i = m_labelScopes.size() - 1; i >= 0; --i) {
        LabelScope* scope = &m_labelScopes[i];
        if (scope->name() && *scope->name() == name) {
            ASSERT(scope->breakTarget());
            return scope;
        }
    }
    return 0;
}

LabelScope* BytecodeGenerator::continueTarget(const Identifier& name)
{
    // Reclaim free label scopes.
    while (m_labelScopes.size() && !m_labelScopes.last().refCount())
        m_labelScopes.removeLast();

    if (!m_labelScopes.size())
        return 0;

    if (name.isEmpty()) {
        for (int i = m_labelScopes.size() - 1; i >= 0; --i) {
            LabelScope* scope = &m_labelScopes[i];
            if (scope->type() == LabelScope::Loop) {
                ASSERT(scope->continueTarget());
                return scope;
            }
        }
        return 0;
    }

    // Continue to the loop nested nearest to the label scope that matches
    // 'name'.
    LabelScope* result = 0;
    for (int i = m_labelScopes.size() - 1; i >= 0; --i) {
        LabelScope* scope = &m_labelScopes[i];
        if (scope->type() == LabelScope::Loop) {
            ASSERT(scope->continueTarget());
            result = scope;
        }
        if (scope->name() && *scope->name() == name)
            return result; // may be 0
    }
    return 0;
}

void BytecodeGenerator::emitComplexPopScopes(ControlFlowContext* topScope, ControlFlowContext* bottomScope)
{
    while (topScope > bottomScope) {
        // First we count the number of dynamic scopes we need to remove to get
        // to a finally block.
        int nNormalScopes = 0;
        while (topScope > bottomScope) {
            if (topScope->isFinallyBlock)
                break;
            ++nNormalScopes;
            --topScope;
        }

        if (nNormalScopes) {
            // We need to remove a number of dynamic scopes to get to the next
            // finally block
            while (nNormalScopes--)
                emitOpcode(op_pop_scope);

            // If topScope == bottomScope then there isn't a finally block left to emit.
            if (topScope == bottomScope)
                return;
        }
        
        Vector<ControlFlowContext> savedScopeContextStack;
        Vector<SwitchInfo> savedSwitchContextStack;
        Vector<ForInContext> savedForInContextStack;
        Vector<TryContext> poppedTryContexts;
        LabelScopeStore savedLabelScopes;
        while (topScope > bottomScope && topScope->isFinallyBlock) {
            RefPtr<Label> beforeFinally = emitLabel(newLabel().get());
            
            // Save the current state of the world while instating the state of the world
            // for the finally block.
            FinallyContext finallyContext = topScope->finallyContext;
            bool flipScopes = finallyContext.scopeContextStackSize != m_scopeContextStack.size();
            bool flipSwitches = finallyContext.switchContextStackSize != m_switchContextStack.size();
            bool flipForIns = finallyContext.forInContextStackSize != m_forInContextStack.size();
            bool flipTries = finallyContext.tryContextStackSize != m_tryContextStack.size();
            bool flipLabelScopes = finallyContext.labelScopesSize != m_labelScopes.size();
            int topScopeIndex = -1;
            int bottomScopeIndex = -1;
            if (flipScopes) {
                topScopeIndex = topScope - m_scopeContextStack.begin();
                bottomScopeIndex = bottomScope - m_scopeContextStack.begin();
                savedScopeContextStack = m_scopeContextStack;
                m_scopeContextStack.shrink(finallyContext.scopeContextStackSize);
            }
            if (flipSwitches) {
                savedSwitchContextStack = m_switchContextStack;
                m_switchContextStack.shrink(finallyContext.switchContextStackSize);
            }
            if (flipForIns) {
                savedForInContextStack = m_forInContextStack;
                m_forInContextStack.shrink(finallyContext.forInContextStackSize);
            }
            if (flipTries) {
                while (m_tryContextStack.size() != finallyContext.tryContextStackSize) {
                    ASSERT(m_tryContextStack.size() > finallyContext.tryContextStackSize);
                    TryContext context = m_tryContextStack.last();
                    m_tryContextStack.removeLast();
                    TryRange range;
                    range.start = context.start;
                    range.end = beforeFinally;
                    range.tryData = context.tryData;
                    m_tryRanges.append(range);
                    poppedTryContexts.append(context);
                }
            }
            if (flipLabelScopes) {
                savedLabelScopes = m_labelScopes;
                while (m_labelScopes.size() > finallyContext.labelScopesSize)
                    m_labelScopes.removeLast();
            }
            int savedFinallyDepth = m_finallyDepth;
            m_finallyDepth = finallyContext.finallyDepth;
            int savedDynamicScopeDepth = m_dynamicScopeDepth;
            m_dynamicScopeDepth = finallyContext.dynamicScopeDepth;
            
            // Emit the finally block.
            emitNode(finallyContext.finallyBlock);
            
            RefPtr<Label> afterFinally = emitLabel(newLabel().get());
            
            // Restore the state of the world.
            if (flipScopes) {
                m_scopeContextStack = savedScopeContextStack;
                topScope = &m_scopeContextStack[topScopeIndex]; // assert it's within bounds
                bottomScope = m_scopeContextStack.begin() + bottomScopeIndex; // don't assert, since it the index might be -1.
            }
            if (flipSwitches)
                m_switchContextStack = savedSwitchContextStack;
            if (flipForIns)
                m_forInContextStack = savedForInContextStack;
            if (flipTries) {
                ASSERT(m_tryContextStack.size() == finallyContext.tryContextStackSize);
                for (unsigned i = poppedTryContexts.size(); i--;) {
                    TryContext context = poppedTryContexts[i];
                    context.start = afterFinally;
                    m_tryContextStack.append(context);
                }
                poppedTryContexts.clear();
            }
            if (flipLabelScopes)
                m_labelScopes = savedLabelScopes;
            m_finallyDepth = savedFinallyDepth;
            m_dynamicScopeDepth = savedDynamicScopeDepth;
            
            --topScope;
        }
    }
}

void BytecodeGenerator::emitPopScopes(int targetScopeDepth)
{
    ASSERT(scopeDepth() - targetScopeDepth >= 0);

    size_t scopeDelta = scopeDepth() - targetScopeDepth;
    ASSERT(scopeDelta <= m_scopeContextStack.size());
    if (!scopeDelta)
        return;

    if (!m_finallyDepth) {
        while (scopeDelta--)
            emitOpcode(op_pop_scope);
        return;
    }

    emitComplexPopScopes(&m_scopeContextStack.last(), &m_scopeContextStack.last() - scopeDelta);
}

RegisterID* BytecodeGenerator::emitGetPropertyNames(RegisterID* dst, RegisterID* base, RegisterID* i, RegisterID* size, Label* breakTarget)
{
    size_t begin = instructions().size();

    emitOpcode(op_get_pnames);
    instructions().append(dst->index());
    instructions().append(base->index());
    instructions().append(i->index());
    instructions().append(size->index());
    instructions().append(breakTarget->bind(begin, instructions().size()));
    return dst;
}

RegisterID* BytecodeGenerator::emitNextPropertyName(RegisterID* dst, RegisterID* base, RegisterID* i, RegisterID* size, RegisterID* iter, Label* target)
{
    size_t begin = instructions().size();

    emitOpcode(op_next_pname);
    instructions().append(dst->index());
    instructions().append(base->index());
    instructions().append(i->index());
    instructions().append(size->index());
    instructions().append(iter->index());
    instructions().append(target->bind(begin, instructions().size()));
    return dst;
}

TryData* BytecodeGenerator::pushTry(Label* start)
{
    TryData tryData;
    tryData.target = newLabel();
    tryData.targetScopeDepth = UINT_MAX;
    m_tryData.append(tryData);
    TryData* result = &m_tryData.last();
    
    TryContext tryContext;
    tryContext.start = start;
    tryContext.tryData = result;
    
    m_tryContextStack.append(tryContext);
    
    return result;
}

RegisterID* BytecodeGenerator::popTryAndEmitCatch(TryData* tryData, RegisterID* targetRegister, Label* end)
{
    m_usesExceptions = true;
    
    ASSERT_UNUSED(tryData, m_tryContextStack.last().tryData == tryData);
    
    TryRange tryRange;
    tryRange.start = m_tryContextStack.last().start;
    tryRange.end = end;
    tryRange.tryData = m_tryContextStack.last().tryData;
    m_tryRanges.append(tryRange);
    m_tryContextStack.removeLast();
    
    emitLabel(tryRange.tryData->target.get());
    tryRange.tryData->targetScopeDepth = m_dynamicScopeDepth;

    emitOpcode(op_catch);
    instructions().append(targetRegister->index());
    return targetRegister;
}

void BytecodeGenerator::emitThrowReferenceError(const String& message)
{
    emitOpcode(op_throw_static_error);
    instructions().append(addConstantValue(addStringConstant(Identifier(m_vm, message)))->index());
    instructions().append(true);
}

void BytecodeGenerator::emitPushNameScope(const Identifier& property, RegisterID* value, unsigned attributes)
{
    ControlFlowContext context;
    context.isFinallyBlock = false;
    m_scopeContextStack.append(context);
    m_dynamicScopeDepth++;

    emitOpcode(op_push_name_scope);
    instructions().append(addConstant(property));
    instructions().append(value->index());
    instructions().append(attributes);
}

void BytecodeGenerator::beginSwitch(RegisterID* scrutineeRegister, SwitchInfo::SwitchType type)
{
    SwitchInfo info = { static_cast<uint32_t>(instructions().size()), type };
    switch (type) {
        case SwitchInfo::SwitchImmediate:
            emitOpcode(op_switch_imm);
            break;
        case SwitchInfo::SwitchCharacter:
            emitOpcode(op_switch_char);
            break;
        case SwitchInfo::SwitchString:
            emitOpcode(op_switch_string);
            break;
        default:
            RELEASE_ASSERT_NOT_REACHED();
    }

    instructions().append(0); // place holder for table index
    instructions().append(0); // place holder for default target    
    instructions().append(scrutineeRegister->index());
    m_switchContextStack.append(info);
}

static int32_t keyForImmediateSwitch(ExpressionNode* node, int32_t min, int32_t max)
{
    UNUSED_PARAM(max);
    ASSERT(node->isNumber());
    double value = static_cast<NumberNode*>(node)->value();
    int32_t key = static_cast<int32_t>(value);
    ASSERT(key == value);
    ASSERT(key >= min);
    ASSERT(key <= max);
    return key - min;
}

static void prepareJumpTableForImmediateSwitch(UnlinkedSimpleJumpTable& jumpTable, int32_t switchAddress, uint32_t clauseCount, RefPtr<Label>* labels, ExpressionNode** nodes, int32_t min, int32_t max)
{
    jumpTable.min = min;
    jumpTable.branchOffsets.resize(max - min + 1);
    jumpTable.branchOffsets.fill(0);
    for (uint32_t i = 0; i < clauseCount; ++i) {
        // We're emitting this after the clause labels should have been fixed, so 
        // the labels should not be "forward" references
        ASSERT(!labels[i]->isForward());
        jumpTable.add(keyForImmediateSwitch(nodes[i], min, max), labels[i]->bind(switchAddress, switchAddress + 3)); 
    }
}

static int32_t keyForCharacterSwitch(ExpressionNode* node, int32_t min, int32_t max)
{
    UNUSED_PARAM(max);
    ASSERT(node->isString());
    StringImpl* clause = static_cast<StringNode*>(node)->value().impl();
    ASSERT(clause->length() == 1);
    
    int32_t key = (*clause)[0];
    ASSERT(key >= min);
    ASSERT(key <= max);
    return key - min;
}

static void prepareJumpTableForCharacterSwitch(UnlinkedSimpleJumpTable& jumpTable, int32_t switchAddress, uint32_t clauseCount, RefPtr<Label>* labels, ExpressionNode** nodes, int32_t min, int32_t max)
{
    jumpTable.min = min;
    jumpTable.branchOffsets.resize(max - min + 1);
    jumpTable.branchOffsets.fill(0);
    for (uint32_t i = 0; i < clauseCount; ++i) {
        // We're emitting this after the clause labels should have been fixed, so 
        // the labels should not be "forward" references
        ASSERT(!labels[i]->isForward());
        jumpTable.add(keyForCharacterSwitch(nodes[i], min, max), labels[i]->bind(switchAddress, switchAddress + 3)); 
    }
}

static void prepareJumpTableForStringSwitch(UnlinkedStringJumpTable& jumpTable, int32_t switchAddress, uint32_t clauseCount, RefPtr<Label>* labels, ExpressionNode** nodes)
{
    for (uint32_t i = 0; i < clauseCount; ++i) {
        // We're emitting this after the clause labels should have been fixed, so 
        // the labels should not be "forward" references
        ASSERT(!labels[i]->isForward());
        
        ASSERT(nodes[i]->isString());
        StringImpl* clause = static_cast<StringNode*>(nodes[i])->value().impl();
        jumpTable.offsetTable.add(clause, labels[i]->bind(switchAddress, switchAddress + 3));
    }
}

void BytecodeGenerator::endSwitch(uint32_t clauseCount, RefPtr<Label>* labels, ExpressionNode** nodes, Label* defaultLabel, int32_t min, int32_t max)
{
    SwitchInfo switchInfo = m_switchContextStack.last();
    m_switchContextStack.removeLast();
    if (switchInfo.switchType == SwitchInfo::SwitchImmediate) {
        instructions()[switchInfo.bytecodeOffset + 1] = m_codeBlock->numberOfImmediateSwitchJumpTables();
        instructions()[switchInfo.bytecodeOffset + 2] = defaultLabel->bind(switchInfo.bytecodeOffset, switchInfo.bytecodeOffset + 3);

        UnlinkedSimpleJumpTable& jumpTable = m_codeBlock->addImmediateSwitchJumpTable();
        prepareJumpTableForImmediateSwitch(jumpTable, switchInfo.bytecodeOffset, clauseCount, labels, nodes, min, max);
    } else if (switchInfo.switchType == SwitchInfo::SwitchCharacter) {
        instructions()[switchInfo.bytecodeOffset + 1] = m_codeBlock->numberOfCharacterSwitchJumpTables();
        instructions()[switchInfo.bytecodeOffset + 2] = defaultLabel->bind(switchInfo.bytecodeOffset, switchInfo.bytecodeOffset + 3);
        
        UnlinkedSimpleJumpTable& jumpTable = m_codeBlock->addCharacterSwitchJumpTable();
        prepareJumpTableForCharacterSwitch(jumpTable, switchInfo.bytecodeOffset, clauseCount, labels, nodes, min, max);
    } else {
        ASSERT(switchInfo.switchType == SwitchInfo::SwitchString);
        instructions()[switchInfo.bytecodeOffset + 1] = m_codeBlock->numberOfStringSwitchJumpTables();
        instructions()[switchInfo.bytecodeOffset + 2] = defaultLabel->bind(switchInfo.bytecodeOffset, switchInfo.bytecodeOffset + 3);

        UnlinkedStringJumpTable& jumpTable = m_codeBlock->addStringSwitchJumpTable();
        prepareJumpTableForStringSwitch(jumpTable, switchInfo.bytecodeOffset, clauseCount, labels, nodes);
    }
}

RegisterID* BytecodeGenerator::emitThrowExpressionTooDeepException()
{
    // It would be nice to do an even better job of identifying exactly where the expression is.
    // And we could make the caller pass the node pointer in, if there was some way of getting
    // that from an arbitrary node. However, calling emitExpressionInfo without any useful data
    // is still good enough to get us an accurate line number.
    m_expressionTooDeep = true;
    return newTemporary();
}

void BytecodeGenerator::setIsNumericCompareFunction(bool isNumericCompareFunction)
{
    m_codeBlock->setIsNumericCompareFunction(isNumericCompareFunction);
}

bool BytecodeGenerator::isArgumentNumber(const Identifier& ident, int argumentNumber)
{
    RegisterID* registerID = resolve(ident).local();
    if (!registerID || registerID->index() >= 0)
         return 0;
    return registerID->index() == CallFrame::argumentOffset(argumentNumber);
}

void BytecodeGenerator::emitReadOnlyExceptionIfNeeded()
{
    if (!isStrictMode())
        return;
    emitOpcode(op_throw_static_error);
    instructions().append(addConstantValue(addStringConstant(Identifier(m_vm, StrictModeReadonlyPropertyWriteError)))->index());
    instructions().append(false);
}

} // namespace JSC
