/*
 * Copyright (C) 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Cameron Zwarich <cwzwarich@uwaterloo.ca>
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
#include "JSFunction.h"
#include "Interpreter.h"
#include "ScopeChain.h"
#include "UString.h"

using namespace std;

namespace JSC {

/*
    The layout of a register frame looks like this:

    For

    function f(x, y) {
        var v1;
        function g() { }
        var v2;
        return (x) * (y);
    }

    assuming (x) and (y) generated temporaries t1 and t2, you would have

    ------------------------------------
    |  x |  y |  g | v2 | v1 | t1 | t2 | <-- value held
    ------------------------------------
    | -5 | -4 | -3 | -2 | -1 | +0 | +1 | <-- register index
    ------------------------------------
    | params->|<-locals      | temps->

    Because temporary registers are allocated in a stack-like fashion, we
    can reclaim them with a simple popping algorithm. The same goes for labels.
    (We never reclaim parameter or local registers, because parameters and
    locals are DontDelete.)

    The register layout before a function call looks like this:

    For

    function f(x, y)
    {
    }

    f(1);

    >                        <------------------------------
    <                        >  reserved: call frame  |  1 | <-- value held
    >         >snip<         <------------------------------
    <                        > +0 | +1 | +2 | +3 | +4 | +5 | <-- register index
    >                        <------------------------------
    | params->|<-locals      | temps->

    The call instruction fills in the "call frame" registers. It also pads
    missing arguments at the end of the call:

    >                        <-----------------------------------
    <                        >  reserved: call frame  |  1 |  ? | <-- value held ("?" stands for "undefined")
    >         >snip<         <-----------------------------------
    <                        > +0 | +1 | +2 | +3 | +4 | +5 | +6 | <-- register index
    >                        <-----------------------------------
    | params->|<-locals      | temps->

    After filling in missing arguments, the call instruction sets up the new
    stack frame to overlap the end of the old stack frame:

                             |---------------------------------->                        <
                             |  reserved: call frame  |  1 |  ? <                        > <-- value held ("?" stands for "undefined")
                             |---------------------------------->         >snip<         <
                             | -7 | -6 | -5 | -4 | -3 | -2 | -1 <                        > <-- register index
                             |---------------------------------->                        <
                             |                        | params->|<-locals       | temps->

    That way, arguments are "copied" into the callee's stack frame for free.

    If the caller supplies too many arguments, this trick doesn't work. The
    extra arguments protrude into space reserved for locals and temporaries.
    In that case, the call instruction makes a real copy of the call frame header,
    along with just the arguments expected by the callee, leaving the original
    call frame header and arguments behind. (The call instruction can't just discard
    extra arguments, because the "arguments" object may access them later.)
    This copying strategy ensures that all named values will be at the indices
    expected by the callee.
*/

#ifndef NDEBUG
static bool s_dumpsGeneratedCode = false;
#endif

void BytecodeGenerator::setDumpsGeneratedCode(bool dumpsGeneratedCode)
{
#ifndef NDEBUG
    s_dumpsGeneratedCode = dumpsGeneratedCode;
#else
    UNUSED_PARAM(dumpsGeneratedCode);
#endif
}

bool BytecodeGenerator::dumpsGeneratedCode()
{
#ifndef NDEBUG
    return s_dumpsGeneratedCode;
#else
    return false;
#endif
}

JSObject* BytecodeGenerator::generate()
{
    m_codeBlock->setThisRegister(m_thisRegister.index());

    m_scopeNode->emitBytecode(*this);

#ifndef NDEBUG
    m_codeBlock->setInstructionCount(m_codeBlock->instructions().size());

    if (s_dumpsGeneratedCode)
        m_codeBlock->dump(m_scopeChain->globalObject->globalExec());
#endif

    if ((m_codeType == FunctionCode && !m_codeBlock->needsFullScopeChain() && !m_codeBlock->usesArguments()) || m_codeType == EvalCode)
        symbolTable().clear();

    m_codeBlock->shrinkToFit();

    if (m_expressionTooDeep)
        return createOutOfMemoryError(m_scopeChain->globalObject.get());
    return 0;
}

bool BytecodeGenerator::addVar(const Identifier& ident, bool isConstant, RegisterID*& r0)
{
    int index = m_calleeRegisters.size();
    SymbolTableEntry newEntry(index, isConstant ? ReadOnly : 0);
    pair<SymbolTable::iterator, bool> result = symbolTable().add(ident.impl(), newEntry);

    if (!result.second) {
        r0 = &registerFor(result.first->second.getIndex());
        return false;
    }

    r0 = addVar();
    return true;
}

bool BytecodeGenerator::addGlobalVar(const Identifier& ident, bool isConstant, RegisterID*& r0)
{
    int index = m_nextGlobalIndex;
    SymbolTableEntry newEntry(index, isConstant ? ReadOnly : 0);
    pair<SymbolTable::iterator, bool> result = symbolTable().add(ident.impl(), newEntry);

    if (!result.second)
        index = result.first->second.getIndex();
    else {
        --m_nextGlobalIndex;
        m_globals.append(index + m_globalVarStorageOffset);
    }

    r0 = &registerFor(index);
    return result.second;
}

void BytecodeGenerator::preserveLastVar()
{
    if ((m_firstConstantIndex = m_calleeRegisters.size()) != 0)
        m_lastVar = &m_calleeRegisters.last();
}

BytecodeGenerator::BytecodeGenerator(ProgramNode* programNode, ScopeChainNode* scopeChain, SymbolTable* symbolTable, ProgramCodeBlock* codeBlock)
    : m_shouldEmitDebugHooks(scopeChain->globalObject->debugger())
    , m_shouldEmitProfileHooks(scopeChain->globalObject->supportsProfiling())
    , m_shouldEmitRichSourceInfo(scopeChain->globalObject->supportsRichSourceInfo())
    , m_scopeChain(*scopeChain->globalData, scopeChain)
    , m_symbolTable(symbolTable)
    , m_scopeNode(programNode)
    , m_codeBlock(codeBlock)
    , m_thisRegister(RegisterFile::ProgramCodeThisRegister)
    , m_finallyDepth(0)
    , m_dynamicScopeDepth(0)
    , m_baseScopeDepth(0)
    , m_codeType(GlobalCode)
    , m_nextGlobalIndex(-1)
    , m_nextConstantOffset(0)
    , m_globalConstantIndex(0)
    , m_hasCreatedActivation(true)
    , m_firstLazyFunction(0)
    , m_lastLazyFunction(0)
    , m_globalData(scopeChain->globalData)
    , m_lastOpcodeID(op_end)
#ifndef NDEBUG
    , m_lastOpcodePosition(0)
#endif
    , m_stack(m_globalData->stack())
    , m_usesExceptions(false)
    , m_expressionTooDeep(false)
{
    if (m_shouldEmitDebugHooks)
        m_codeBlock->setNeedsFullScopeChain(true);

    emitOpcode(op_enter);
    codeBlock->setGlobalData(m_globalData);

    // FIXME: Move code that modifies the global object to Interpreter::execute.
    
    m_codeBlock->m_numParameters = 1; // Allocate space for "this"

    JSGlobalObject* globalObject = scopeChain->globalObject.get();
    ExecState* exec = globalObject->globalExec();
    RegisterFile* registerFile = &exec->globalData().interpreter->registerFile();
    
    // Shift register indexes in generated code to elide registers allocated by intermediate stack frames.
    m_globalVarStorageOffset = -RegisterFile::CallFrameHeaderSize - m_codeBlock->m_numParameters - registerFile->size();

    // Add previously defined symbols to bookkeeping.
    m_globals.grow(symbolTable->size());
    SymbolTable::iterator end = symbolTable->end();
    for (SymbolTable::iterator it = symbolTable->begin(); it != end; ++it)
        registerFor(it->second.getIndex()).setIndex(it->second.getIndex() + m_globalVarStorageOffset);

    BatchedTransitionOptimizer optimizer(*m_globalData, globalObject);

    const VarStack& varStack = programNode->varStack();
    const FunctionStack& functionStack = programNode->functionStack();
    bool canOptimizeNewGlobals = symbolTable->size() + functionStack.size() + varStack.size() < registerFile->maxGlobals();
    if (canOptimizeNewGlobals) {
        // Shift new symbols so they get stored prior to existing symbols.
        m_nextGlobalIndex -= symbolTable->size();

        HashSet<StringImpl*, IdentifierRepHash> newGlobals;
        Vector<std::pair<int, bool>, 16> functionInfo(functionStack.size());
        for (size_t i = 0; i < functionStack.size(); ++i) {
            FunctionBodyNode* function = functionStack[i];
            globalObject->removeDirect(*m_globalData, function->ident()); // Make sure our new function is not shadowed by an old property.
            SymbolTableEntry entry = symbolTable->inlineGet(function->ident().impl());
            
            if (entry.isNull())
                newGlobals.add(function->ident().impl());
            functionInfo[i] = make_pair(entry.getIndex(), entry.isReadOnly());
        }
        
        Vector<bool, 16> shouldCreateVar(varStack.size());
        for (size_t i = 0; i < varStack.size(); ++i) {
            if (newGlobals.contains(varStack[i].first->impl()) || globalObject->hasProperty(exec, *varStack[i].first)) {
                shouldCreateVar[i] = false;
                continue;
            }
            shouldCreateVar[i] = true;
            newGlobals.add(varStack[i].first->impl());
        }

        int expectedSize = symbolTable->size() + newGlobals.size();
        globalObject->resizeRegisters(symbolTable->size(), expectedSize);
    
        for (size_t i = 0; i < functionStack.size(); ++i) {
            FunctionBodyNode* function = functionStack[i];
            if (functionInfo[i].second)
                continue;
            RegisterID* dst = addGlobalVar(function->ident(), false);
            JSValue value = new (exec) JSFunction(exec, makeFunction(exec, function), scopeChain);
            globalObject->registerAt(dst->index() - m_globalVarStorageOffset).set(*m_globalData, globalObject, value);
        }

        for (size_t i = 0; i < varStack.size(); ++i) {
            if (!shouldCreateVar[i])
                continue;
            addGlobalVar(*varStack[i].first, varStack[i].second & DeclarationStacks::IsConstant);
        }
        if (symbolTable->size() != expectedSize)
            CRASH();

        preserveLastVar();
    } else {
        for (size_t i = 0; i < functionStack.size(); ++i) {
            FunctionBodyNode* function = functionStack[i];
            globalObject->putWithAttributes(exec, function->ident(), new (exec) JSFunction(exec, makeFunction(exec, function), scopeChain), DontDelete);
        }
        for (size_t i = 0; i < varStack.size(); ++i) {
            if (globalObject->symbolTableHasProperty(*varStack[i].first) || globalObject->hasProperty(exec, *varStack[i].first))
                continue;
            int attributes = DontDelete;
            if (varStack[i].second & DeclarationStacks::IsConstant)
                attributes |= ReadOnly;
            globalObject->putWithAttributes(exec, *varStack[i].first, jsUndefined(), attributes);
        }

        preserveLastVar();
    }
    codeBlock->m_numCapturedVars = codeBlock->m_numVars;
}

BytecodeGenerator::BytecodeGenerator(FunctionBodyNode* functionBody, ScopeChainNode* scopeChain, SymbolTable* symbolTable, CodeBlock* codeBlock)
    : m_shouldEmitDebugHooks(scopeChain->globalObject->debugger())
    , m_shouldEmitProfileHooks(scopeChain->globalObject->supportsProfiling())
    , m_shouldEmitRichSourceInfo(scopeChain->globalObject->supportsRichSourceInfo())
    , m_scopeChain(*scopeChain->globalData, scopeChain)
    , m_symbolTable(symbolTable)
    , m_scopeNode(functionBody)
    , m_codeBlock(codeBlock)
    , m_activationRegister(0)
    , m_finallyDepth(0)
    , m_dynamicScopeDepth(0)
    , m_baseScopeDepth(0)
    , m_codeType(FunctionCode)
    , m_nextConstantOffset(0)
    , m_globalConstantIndex(0)
    , m_hasCreatedActivation(false)
    , m_firstLazyFunction(0)
    , m_lastLazyFunction(0)
    , m_globalData(scopeChain->globalData)
    , m_lastOpcodeID(op_end)
#ifndef NDEBUG
    , m_lastOpcodePosition(0)
#endif
    , m_stack(m_globalData->stack())
    , m_usesExceptions(false)
    , m_expressionTooDeep(false)
{
    if (m_shouldEmitDebugHooks)
        m_codeBlock->setNeedsFullScopeChain(true);

    codeBlock->setGlobalData(m_globalData);
    
    emitOpcode(op_enter);
    if (m_codeBlock->needsFullScopeChain()) {
        m_activationRegister = addVar();
        emitInitLazyRegister(m_activationRegister);
        m_codeBlock->setActivationRegister(m_activationRegister->index());
    }

    // Both op_tear_off_activation and op_tear_off_arguments tear off the 'arguments'
    // object, if created.
    if (m_codeBlock->needsFullScopeChain() || functionBody->usesArguments()) {
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

    codeBlock->m_numCapturedVars = codeBlock->m_numVars;
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
    
    if (m_shouldEmitDebugHooks)
        codeBlock->m_numCapturedVars = codeBlock->m_numVars;

    FunctionParameters& parameters = *functionBody->parameters();
    size_t parameterCount = parameters.size();
    int nextParameterIndex = -RegisterFile::CallFrameHeaderSize - parameterCount - 1;
    m_parameters.grow(1 + parameterCount); // reserve space for "this"

    // Add "this" as a parameter
    m_thisRegister.setIndex(nextParameterIndex);
    ++m_codeBlock->m_numParameters;
    
    for (size_t i = 0; i < parameterCount; ++i)
        addParameter(parameters[i], ++nextParameterIndex);

    preserveLastVar();

    if (isConstructor()) {
        RefPtr<RegisterID> func = newTemporary();
        RefPtr<RegisterID> funcProto = newTemporary();

        emitOpcode(op_get_callee);
        instructions().append(func->index());
        // Load prototype.
        emitGetById(funcProto.get(), func.get(), globalData()->propertyNames->prototype);

        emitOpcode(op_create_this);
        instructions().append(m_thisRegister.index());
        instructions().append(funcProto->index());
    } else if (functionBody->usesThis() || m_shouldEmitDebugHooks) {
        if (codeBlock->isStrictMode())
            emitOpcode(op_convert_this_strict);
        else
            emitOpcode(op_convert_this);
        instructions().append(m_thisRegister.index());
    }
}

BytecodeGenerator::BytecodeGenerator(EvalNode* evalNode, ScopeChainNode* scopeChain, SymbolTable* symbolTable, EvalCodeBlock* codeBlock)
    : m_shouldEmitDebugHooks(scopeChain->globalObject->debugger())
    , m_shouldEmitProfileHooks(scopeChain->globalObject->supportsProfiling())
    , m_shouldEmitRichSourceInfo(scopeChain->globalObject->supportsRichSourceInfo())
    , m_scopeChain(*scopeChain->globalData, scopeChain)
    , m_symbolTable(symbolTable)
    , m_scopeNode(evalNode)
    , m_codeBlock(codeBlock)
    , m_thisRegister(RegisterFile::ProgramCodeThisRegister)
    , m_finallyDepth(0)
    , m_dynamicScopeDepth(0)
    , m_baseScopeDepth(codeBlock->baseScopeDepth())
    , m_codeType(EvalCode)
    , m_nextConstantOffset(0)
    , m_globalConstantIndex(0)
    , m_hasCreatedActivation(true)
    , m_firstLazyFunction(0)
    , m_lastLazyFunction(0)
    , m_globalData(scopeChain->globalData)
    , m_lastOpcodeID(op_end)
#ifndef NDEBUG
    , m_lastOpcodePosition(0)
#endif
    , m_stack(m_globalData->stack())
    , m_usesExceptions(false)
    , m_expressionTooDeep(false)
{
    if (m_shouldEmitDebugHooks || m_baseScopeDepth)
        m_codeBlock->setNeedsFullScopeChain(true);

    emitOpcode(op_enter);
    codeBlock->setGlobalData(m_globalData);
    m_codeBlock->m_numParameters = 1; // Allocate space for "this"

    const DeclarationStacks::FunctionStack& functionStack = evalNode->functionStack();
    for (size_t i = 0; i < functionStack.size(); ++i)
        m_codeBlock->addFunctionDecl(makeFunction(m_globalData, functionStack[i]));

    const DeclarationStacks::VarStack& varStack = evalNode->varStack();
    unsigned numVariables = varStack.size();
    Vector<Identifier> variables;
    variables.reserveCapacity(numVariables);
    for (size_t i = 0; i < numVariables; ++i)
        variables.append(*varStack[i].first);
    codeBlock->adoptVariables(variables);
    codeBlock->m_numCapturedVars = codeBlock->m_numVars;
    preserveLastVar();
}

RegisterID* BytecodeGenerator::emitInitLazyRegister(RegisterID* reg)
{
    emitOpcode(op_init_lazy_reg);
    instructions().append(reg->index());
    return reg;
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
    ++m_codeBlock->m_numParameters;
}

RegisterID* BytecodeGenerator::registerFor(const Identifier& ident)
{
    if (ident == propertyNames().thisIdentifier)
        return &m_thisRegister;

    if (!shouldOptimizeLocals())
        return 0;

    SymbolTableEntry entry = symbolTable().get(ident.impl());
    if (entry.isNull())
        return 0;

    if (ident == propertyNames().arguments)
        createArgumentsIfNecessary();

    return createLazyRegisterIfNecessary(&registerFor(entry.getIndex()));
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

RegisterID* BytecodeGenerator::constRegisterFor(const Identifier& ident)
{
    if (m_codeType == EvalCode)
        return 0;

    SymbolTableEntry entry = symbolTable().get(ident.impl());
    if (entry.isNull())
        return 0;

    return createLazyRegisterIfNecessary(&registerFor(entry.getIndex()));
}

bool BytecodeGenerator::isLocal(const Identifier& ident)
{
    if (ident == propertyNames().thisIdentifier)
        return true;
    
    return shouldOptimizeLocals() && symbolTable().contains(ident.impl());
}

bool BytecodeGenerator::isLocalConstant(const Identifier& ident)
{
    return symbolTable().get(ident.impl()).isReadOnly();
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

RegisterID* BytecodeGenerator::highestUsedRegister()
{
    size_t count = m_codeBlock->m_numCalleeRegisters;
    while (m_calleeRegisters.size() < count)
        newRegister();
    return &m_calleeRegisters.last();
}

PassRefPtr<LabelScope> BytecodeGenerator::newLabelScope(LabelScope::Type type, const Identifier* name)
{
    // Reclaim free label scopes.
    while (m_labelScopes.size() && !m_labelScopes.last().refCount())
        m_labelScopes.removeLast();

    // Allocate new label scope.
    LabelScope scope(type, name, scopeDepth(), newLabel(), type == LabelScope::Loop ? newLabel() : PassRefPtr<Label>()); // Only loops have continue targets.
    m_labelScopes.append(scope);
    return &m_labelScopes.last();
}

PassRefPtr<Label> BytecodeGenerator::newLabel()
{
    // Reclaim free label IDs.
    while (m_labels.size() && !m_labels.last().refCount())
        m_labels.removeLast();

    // Allocate new label ID.
    m_labels.append(m_codeBlock);
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
    instructions().append(globalData()->interpreter->getOpcode(opcodeID));
    m_lastOpcodeID = opcodeID;
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
    emitOpcode(target->isForward() ? op_jmp : op_loop);
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
            emitOpcode(target->isForward() ? op_jless : op_loop_if_less);
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
            emitOpcode(target->isForward() ? op_jlesseq : op_loop_if_lesseq);
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

    emitOpcode(target->isForward() ? op_jtrue : op_loop_if_true);
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
    } else if (m_lastOpcodeID == op_not) {
        int dstIndex;
        int srcIndex;

        retrieveLastUnaryOp(dstIndex, srcIndex);

        if (cond->index() == dstIndex && cond->isTemporary() && !cond->refCount()) {
            rewindUnaryOp();

            size_t begin = instructions().size();
            emitOpcode(target->isForward() ? op_jtrue : op_loop_if_true);
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
    emitOpcode(target->isForward() ? op_jfalse : op_loop_if_false);
    instructions().append(cond->index());
    instructions().append(target->bind(begin, instructions().size()));
    return target;
}

PassRefPtr<Label> BytecodeGenerator::emitJumpIfNotFunctionCall(RegisterID* cond, Label* target)
{
    size_t begin = instructions().size();

    emitOpcode(op_jneq_ptr);
    instructions().append(cond->index());
    instructions().append(Instruction(*m_globalData, m_codeBlock->ownerExecutable(), m_scopeChain->globalObject->callFunction()));
    instructions().append(target->bind(begin, instructions().size()));
    return target;
}

PassRefPtr<Label> BytecodeGenerator::emitJumpIfNotFunctionApply(RegisterID* cond, Label* target)
{
    size_t begin = instructions().size();

    emitOpcode(op_jneq_ptr);
    instructions().append(cond->index());
    instructions().append(Instruction(*m_globalData, m_codeBlock->ownerExecutable(), m_scopeChain->globalObject->applyFunction()));
    instructions().append(target->bind(begin, instructions().size()));
    return target;
}

unsigned BytecodeGenerator::addConstant(const Identifier& ident)
{
    StringImpl* rep = ident.impl();
    pair<IdentifierMap::iterator, bool> result = m_identifierMap.add(rep, m_codeBlock->numberOfIdentifiers());
    if (result.second) // new entry
        m_codeBlock->addIdentifier(Identifier(m_globalData, rep));

    return result.first->second;
}

RegisterID* BytecodeGenerator::addConstantValue(JSValue v)
{
    int index = m_nextConstantOffset;

    pair<JSValueMap::iterator, bool> result = m_jsValueMap.add(JSValue::encode(v), m_nextConstantOffset);
    if (result.second) {
        m_constantPoolRegisters.append(FirstConstantRegisterIndex + m_nextConstantOffset);
        ++m_nextConstantOffset;
        m_codeBlock->addConstant(JSValue(v));
    } else
        index = result.first->second;

    return &m_constantPoolRegisters[index];
}

unsigned BytecodeGenerator::addRegExp(PassRefPtr<RegExp> r)
{
    return m_codeBlock->addRegExp(r);
}

RegisterID* BytecodeGenerator::emitMove(RegisterID* dst, RegisterID* src)
{
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

RegisterID* BytecodeGenerator::emitPreInc(RegisterID* srcDst)
{
    emitOpcode(op_pre_inc);
    instructions().append(srcDst->index());
    return srcDst;
}

RegisterID* BytecodeGenerator::emitPreDec(RegisterID* srcDst)
{
    emitOpcode(op_pre_dec);
    instructions().append(srcDst->index());
    return srcDst;
}

RegisterID* BytecodeGenerator::emitPostInc(RegisterID* dst, RegisterID* srcDst)
{
    emitOpcode(op_post_inc);
    instructions().append(dst->index());
    instructions().append(srcDst->index());
    return dst;
}

RegisterID* BytecodeGenerator::emitPostDec(RegisterID* dst, RegisterID* srcDst)
{
    emitOpcode(op_post_dec);
    instructions().append(dst->index());
    instructions().append(srcDst->index());
    return dst;
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
            const UString& value = asString(m_codeBlock->constantRegister(src2->index()).get())->tryGetValue();
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
    if (isnan(number) || number == HashTraits<double>::emptyValue() || HashTraits<double>::isDeletedValue(number))
        return emitLoad(dst, jsNumber(number));
    JSValue& valueInMap = m_numberMap.add(number, JSValue()).first->second;
    if (!valueInMap)
        valueInMap = jsNumber(number);
    return emitLoad(dst, valueInMap);
}

RegisterID* BytecodeGenerator::emitLoad(RegisterID* dst, const Identifier& identifier)
{
    JSString*& stringInMap = m_stringMap.add(identifier.impl(), 0).first->second;
    if (!stringInMap)
        stringInMap = jsOwnedString(globalData(), identifier.ustring());
    return emitLoad(dst, JSValue(stringInMap));
}

RegisterID* BytecodeGenerator::emitLoad(RegisterID* dst, JSValue v)
{
    RegisterID* constantID = addConstantValue(v);
    if (dst)
        return emitMove(dst, constantID);
    return constantID;
}

bool BytecodeGenerator::findScopedProperty(const Identifier& property, int& index, size_t& stackDepth, bool forWriting, bool& requiresDynamicChecks, JSObject*& globalObject)
{
    // Cases where we cannot statically optimize the lookup.
    if (property == propertyNames().arguments || !canOptimizeNonLocals()) {
        stackDepth = 0;
        index = missingSymbolMarker();

        if (shouldOptimizeLocals() && m_codeType == GlobalCode) {
            ScopeChainIterator iter = m_scopeChain->begin();
            globalObject = iter->get();
            ASSERT((++iter) == m_scopeChain->end());
        }
        return false;
    }

    size_t depth = 0;
    requiresDynamicChecks = false;
    ScopeChainIterator iter = m_scopeChain->begin();
    ScopeChainIterator end = m_scopeChain->end();
    for (; iter != end; ++iter, ++depth) {
        JSObject* currentScope = iter->get();
        if (!currentScope->isVariableObject())
            break;
        JSVariableObject* currentVariableObject = static_cast<JSVariableObject*>(currentScope);
        SymbolTableEntry entry = currentVariableObject->symbolTable().get(property.impl());

        // Found the property
        if (!entry.isNull()) {
            if (entry.isReadOnly() && forWriting) {
                stackDepth = 0;
                index = missingSymbolMarker();
                if (++iter == end)
                    globalObject = currentVariableObject;
                return false;
            }
            stackDepth = depth + m_codeBlock->needsFullScopeChain();
            index = entry.getIndex();
            if (++iter == end)
                globalObject = currentVariableObject;
            return true;
        }
        bool scopeRequiresDynamicChecks = false;
        if (currentVariableObject->isDynamicScope(scopeRequiresDynamicChecks))
            break;
        requiresDynamicChecks |= scopeRequiresDynamicChecks;
    }
    // Can't locate the property but we're able to avoid a few lookups.
    stackDepth = depth + m_codeBlock->needsFullScopeChain();
    index = missingSymbolMarker();
    JSObject* scope = iter->get();
    if (++iter == end)
        globalObject = scope;
    return true;
}

void BytecodeGenerator::emitCheckHasInstance(RegisterID* base)
{ 
    emitOpcode(op_check_has_instance);
    instructions().append(base->index());
}

RegisterID* BytecodeGenerator::emitInstanceOf(RegisterID* dst, RegisterID* value, RegisterID* base, RegisterID* basePrototype)
{ 
    emitOpcode(op_instanceof);
    instructions().append(dst->index());
    instructions().append(value->index());
    instructions().append(base->index());
    instructions().append(basePrototype->index());
    return dst;
}

RegisterID* BytecodeGenerator::emitResolve(RegisterID* dst, const Identifier& property)
{
    size_t depth = 0;
    int index = 0;
    JSObject* globalObject = 0;
    bool requiresDynamicChecks = false;
    if (!findScopedProperty(property, index, depth, false, requiresDynamicChecks, globalObject) && !globalObject) {
        // We can't optimise at all :-(
        emitOpcode(op_resolve);
        instructions().append(dst->index());
        instructions().append(addConstant(property));
        return dst;
    }

    if (globalObject) {
        bool forceGlobalResolve = false;

        if (index != missingSymbolMarker() && !forceGlobalResolve && !requiresDynamicChecks) {
            // Directly index the property lookup across multiple scopes.
            return emitGetScopedVar(dst, depth, index, globalObject);
        }

#if ENABLE(JIT)
        m_codeBlock->addGlobalResolveInfo(instructions().size());
#endif
#if ENABLE(INTERPRETER)
        m_codeBlock->addGlobalResolveInstruction(instructions().size());
#endif
        emitOpcode(requiresDynamicChecks ? op_resolve_global_dynamic : op_resolve_global);
        instructions().append(dst->index());
        instructions().append(addConstant(property));
        instructions().append(0);
        instructions().append(0);
        if (requiresDynamicChecks)
            instructions().append(depth);
        return dst;
    }

    if (requiresDynamicChecks) {
        // If we get here we have eval nested inside a |with| just give up
        emitOpcode(op_resolve);
        instructions().append(dst->index());
        instructions().append(addConstant(property));
        return dst;
    }

    if (index != missingSymbolMarker()) {
        // Directly index the property lookup across multiple scopes.
        return emitGetScopedVar(dst, depth, index, globalObject);
    }

    // In this case we are at least able to drop a few scope chains from the
    // lookup chain, although we still need to hash from then on.
    emitOpcode(op_resolve_skip);
    instructions().append(dst->index());
    instructions().append(addConstant(property));
    instructions().append(depth);
    return dst;
}

RegisterID* BytecodeGenerator::emitGetScopedVar(RegisterID* dst, size_t depth, int index, JSValue globalObject)
{
    if (globalObject) {
        emitOpcode(op_get_global_var);
        instructions().append(dst->index());
        instructions().append(index);
        return dst;
    }

    emitOpcode(op_get_scoped_var);
    instructions().append(dst->index());
    instructions().append(index);
    instructions().append(depth);
    return dst;
}

RegisterID* BytecodeGenerator::emitPutScopedVar(size_t depth, int index, RegisterID* value, JSValue globalObject)
{
    if (globalObject) {
        emitOpcode(op_put_global_var);
        instructions().append(index);
        instructions().append(value->index());
        return value;
    }
    emitOpcode(op_put_scoped_var);
    instructions().append(index);
    instructions().append(depth);
    instructions().append(value->index());
    return value;
}

RegisterID* BytecodeGenerator::emitResolveBase(RegisterID* dst, const Identifier& property)
{
    size_t depth = 0;
    int index = 0;
    JSObject* globalObject = 0;
    bool requiresDynamicChecks = false;
    findScopedProperty(property, index, depth, false, requiresDynamicChecks, globalObject);
    if (!globalObject || requiresDynamicChecks) {
        // We can't optimise at all :-(
        emitOpcode(op_resolve_base);
        instructions().append(dst->index());
        instructions().append(addConstant(property));
        instructions().append(false);
        return dst;
    }

    // Global object is the base
    return emitLoad(dst, JSValue(globalObject));
}

RegisterID* BytecodeGenerator::emitResolveBaseForPut(RegisterID* dst, const Identifier& property)
{
    if (!m_codeBlock->isStrictMode())
        return emitResolveBase(dst, property);
    size_t depth = 0;
    int index = 0;
    JSObject* globalObject = 0;
    bool requiresDynamicChecks = false;
    findScopedProperty(property, index, depth, false, requiresDynamicChecks, globalObject);
    if (!globalObject || requiresDynamicChecks) {
        // We can't optimise at all :-(
        emitOpcode(op_resolve_base);
        instructions().append(dst->index());
        instructions().append(addConstant(property));
        instructions().append(true);
        return dst;
    }
    
    // Global object is the base
    RefPtr<RegisterID> result = emitLoad(dst, JSValue(globalObject));
    emitOpcode(op_ensure_property_exists);
    instructions().append(dst->index());
    instructions().append(addConstant(property));
    return result.get();
}

RegisterID* BytecodeGenerator::emitResolveWithBase(RegisterID* baseDst, RegisterID* propDst, const Identifier& property)
{
    size_t depth = 0;
    int index = 0;
    JSObject* globalObject = 0;
    bool requiresDynamicChecks = false;
    if (!findScopedProperty(property, index, depth, false, requiresDynamicChecks, globalObject) || !globalObject || requiresDynamicChecks) {
        // We can't optimise at all :-(
        emitOpcode(op_resolve_with_base);
        instructions().append(baseDst->index());
        instructions().append(propDst->index());
        instructions().append(addConstant(property));
        return baseDst;
    }

    bool forceGlobalResolve = false;

    // Global object is the base
    emitLoad(baseDst, JSValue(globalObject));

    if (index != missingSymbolMarker() && !forceGlobalResolve) {
        // Directly index the property lookup across multiple scopes.
        emitGetScopedVar(propDst, depth, index, globalObject);
        return baseDst;
    }

#if ENABLE(JIT)
    m_codeBlock->addGlobalResolveInfo(instructions().size());
#endif
#if ENABLE(INTERPRETER)
    m_codeBlock->addGlobalResolveInstruction(instructions().size());
#endif
    emitOpcode(requiresDynamicChecks ? op_resolve_global_dynamic : op_resolve_global);
    instructions().append(propDst->index());
    instructions().append(addConstant(property));
    instructions().append(0);
    instructions().append(0);
    if (requiresDynamicChecks)
        instructions().append(depth);
    return baseDst;
}

void BytecodeGenerator::emitMethodCheck()
{
    emitOpcode(op_method_check);
}

RegisterID* BytecodeGenerator::emitGetById(RegisterID* dst, RegisterID* base, const Identifier& property)
{
#if ENABLE(JIT)
    m_codeBlock->addStructureStubInfo(StructureStubInfo(access_get_by_id));
#endif
    
#if ENABLE(INTERPRETER)
    m_codeBlock->addPropertyAccessInstruction(instructions().size());
#endif

    emitOpcode(op_get_by_id);
    instructions().append(dst->index());
    instructions().append(base->index());
    instructions().append(addConstant(property));
    instructions().append(0);
    instructions().append(0);
    instructions().append(0);
    instructions().append(0);
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
#if ENABLE(JIT)
    m_codeBlock->addStructureStubInfo(StructureStubInfo(access_put_by_id));
#endif
#if ENABLE(INTERPRETER)
    m_codeBlock->addPropertyAccessInstruction(instructions().size());
#endif

    emitOpcode(op_put_by_id);
    instructions().append(base->index());
    instructions().append(addConstant(property));
    instructions().append(value->index());
    instructions().append(0);
    instructions().append(0);
    instructions().append(0);
    instructions().append(0);
    instructions().append(0);
    return value;
}

RegisterID* BytecodeGenerator::emitDirectPutById(RegisterID* base, const Identifier& property, RegisterID* value)
{
#if ENABLE(JIT)
    m_codeBlock->addStructureStubInfo(StructureStubInfo(access_put_by_id));
#endif
#if ENABLE(INTERPRETER)
    m_codeBlock->addPropertyAccessInstruction(instructions().size());
#endif
    
    emitOpcode(op_put_by_id);
    instructions().append(base->index());
    instructions().append(addConstant(property));
    instructions().append(value->index());
    instructions().append(0);
    instructions().append(0);
    instructions().append(0);
    instructions().append(0);
    instructions().append(property != m_globalData->propertyNames->underscoreProto);
    return value;
}

RegisterID* BytecodeGenerator::emitPutGetter(RegisterID* base, const Identifier& property, RegisterID* value)
{
    emitOpcode(op_put_getter);
    instructions().append(base->index());
    instructions().append(addConstant(property));
    instructions().append(value->index());
    return value;
}

RegisterID* BytecodeGenerator::emitPutSetter(RegisterID* base, const Identifier& property, RegisterID* value)
{
    emitOpcode(op_put_setter);
    instructions().append(base->index());
    instructions().append(addConstant(property));
    instructions().append(value->index());
    return value;
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
    emitOpcode(op_get_argument_by_val);
    instructions().append(dst->index());
    ASSERT(base->index() == m_codeBlock->argumentsRegister());
    instructions().append(base->index());
    instructions().append(property->index());
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
    emitOpcode(op_get_by_val);
    instructions().append(dst->index());
    instructions().append(base->index());
    instructions().append(property->index());
    return dst;
}

RegisterID* BytecodeGenerator::emitPutByVal(RegisterID* base, RegisterID* property, RegisterID* value)
{
    emitOpcode(op_put_by_val);
    instructions().append(base->index());
    instructions().append(property->index());
    instructions().append(value->index());
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

RegisterID* BytecodeGenerator::emitNewObject(RegisterID* dst)
{
    emitOpcode(op_new_object);
    instructions().append(dst->index());
    return dst;
}

RegisterID* BytecodeGenerator::emitNewArray(RegisterID* dst, ElementNode* elements)
{
    Vector<RefPtr<RegisterID>, 16> argv;
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
    return dst;
}

RegisterID* BytecodeGenerator::emitNewFunction(RegisterID* dst, FunctionBodyNode* function)
{
    return emitNewFunctionInternal(dst, m_codeBlock->addFunctionDecl(makeFunction(m_globalData, function)), false);
}

RegisterID* BytecodeGenerator::emitLazyNewFunction(RegisterID* dst, FunctionBodyNode* function)
{
    std::pair<FunctionOffsetMap::iterator, bool> ptr = m_functionOffsets.add(function, 0);
    if (ptr.second)
        ptr.first->second = m_codeBlock->addFunctionDecl(makeFunction(m_globalData, function));
    return emitNewFunctionInternal(dst, ptr.first->second, true);
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

RegisterID* BytecodeGenerator::emitNewRegExp(RegisterID* dst, PassRefPtr<RegExp> regExp)
{
    emitOpcode(op_new_regexp);
    instructions().append(dst->index());
    instructions().append(addRegExp(regExp));
    return dst;
}

RegisterID* BytecodeGenerator::emitNewFunctionExpression(RegisterID* r0, FuncExprNode* n)
{
    FunctionBodyNode* function = n->body();
    unsigned index = m_codeBlock->addFunctionExpr(makeFunction(m_globalData, function));
    
    createActivationIfNecessary();
    emitOpcode(op_new_func_exp);
    instructions().append(r0->index());
    instructions().append(index);
    return r0;
}

RegisterID* BytecodeGenerator::emitCall(RegisterID* dst, RegisterID* func, CallArguments& callArguments, unsigned divot, unsigned startOffset, unsigned endOffset)
{
    return emitCall(op_call, dst, func, callArguments, divot, startOffset, endOffset);
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

RegisterID* BytecodeGenerator::emitCallEval(RegisterID* dst, RegisterID* func, CallArguments& callArguments, unsigned divot, unsigned startOffset, unsigned endOffset)
{
    return emitCall(op_call_eval, dst, func, callArguments, divot, startOffset, endOffset);
}

RegisterID* BytecodeGenerator::emitCall(OpcodeID opcodeID, RegisterID* dst, RegisterID* func, CallArguments& callArguments, unsigned divot, unsigned startOffset, unsigned endOffset)
{
    ASSERT(opcodeID == op_call || opcodeID == op_call_eval);
    ASSERT(func->refCount());

    if (m_shouldEmitProfileHooks)
        emitMove(callArguments.profileHookRegister(), func);

    // Generate code for arguments.
    unsigned argumentIndex = 0;
    for (ArgumentListNode* n = callArguments.argumentsNode()->m_listNode; n; n = n->m_next)
        emitNode(callArguments.argumentRegister(argumentIndex++), n);

    // Reserve space for call frame.
    Vector<RefPtr<RegisterID>, RegisterFile::CallFrameHeaderSize> callFrame;
    for (int i = 0; i < RegisterFile::CallFrameHeaderSize; ++i)
        callFrame.append(newTemporary());

    if (m_shouldEmitProfileHooks) {
        emitOpcode(op_profile_will_call);
        instructions().append(callArguments.profileHookRegister()->index());
    }

    emitExpressionInfo(divot, startOffset, endOffset);

#if ENABLE(JIT)
    m_codeBlock->addCallLinkInfo();
#endif

    // Emit call.
    emitOpcode(opcodeID);
    instructions().append(func->index()); // func
    instructions().append(callArguments.count()); // argCount
    instructions().append(callArguments.callFrame()); // registerOffset
    if (dst != ignoredResult()) {
        emitOpcode(op_call_put_result);
        instructions().append(dst->index()); // dst
    }

    if (m_shouldEmitProfileHooks) {
        emitOpcode(op_profile_did_call);
        instructions().append(callArguments.profileHookRegister()->index());
    }

    return dst;
}

RegisterID* BytecodeGenerator::emitLoadVarargs(RegisterID* argCountDst, RegisterID* thisRegister, RegisterID* arguments)
{
    ASSERT(argCountDst->index() < arguments->index());
    emitOpcode(op_load_varargs);
    instructions().append(argCountDst->index());
    instructions().append(arguments->index());
    instructions().append(thisRegister->index() + RegisterFile::CallFrameHeaderSize); // initial registerOffset
    return argCountDst;
}

RegisterID* BytecodeGenerator::emitCallVarargs(RegisterID* dst, RegisterID* func, RegisterID* thisRegister, RegisterID* argCountRegister, unsigned divot, unsigned startOffset, unsigned endOffset)
{
    ASSERT(func->refCount());
    ASSERT(thisRegister->refCount());
    ASSERT(dst != func);
    if (m_shouldEmitProfileHooks) {
        emitOpcode(op_profile_will_call);
        instructions().append(func->index());
    }
    
    emitExpressionInfo(divot, startOffset, endOffset);
    
    // Emit call.
    emitOpcode(op_call_varargs);
    instructions().append(func->index()); // func
    instructions().append(argCountRegister->index()); // arg count
    instructions().append(thisRegister->index() + RegisterFile::CallFrameHeaderSize); // initial registerOffset
    if (dst != ignoredResult()) {
        emitOpcode(op_call_put_result);
        instructions().append(dst->index()); // dst
    }
    if (m_shouldEmitProfileHooks) {
        emitOpcode(op_profile_did_call);
        instructions().append(func->index());
    }
    return dst;
}

RegisterID* BytecodeGenerator::emitReturn(RegisterID* src)
{
    if (m_codeBlock->needsFullScopeChain()) {
        emitOpcode(op_tear_off_activation);
        instructions().append(m_activationRegister->index());
        instructions().append(m_codeBlock->argumentsRegister());
    } else if (m_codeBlock->usesArguments() && m_codeBlock->m_numParameters > 1
               && !m_codeBlock->isStrictMode()) { // If there are no named parameters, there's nothing to tear off, since extra / unnamed parameters get copied to the arguments object at construct time.
        emitOpcode(op_tear_off_arguments);
        instructions().append(m_codeBlock->argumentsRegister());
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

RegisterID* BytecodeGenerator::emitConstruct(RegisterID* dst, RegisterID* func, CallArguments& callArguments, unsigned divot, unsigned startOffset, unsigned endOffset)
{
    ASSERT(func->refCount());

    if (m_shouldEmitProfileHooks)
        emitMove(callArguments.profileHookRegister(), func);

    // Generate code for arguments.
    unsigned argumentIndex = 0;
    if (ArgumentsNode* argumentsNode = callArguments.argumentsNode()) {
        for (ArgumentListNode* n = argumentsNode->m_listNode; n; n = n->m_next)
            emitNode(callArguments.argumentRegister(argumentIndex++), n);
    }

    if (m_shouldEmitProfileHooks) {
        emitOpcode(op_profile_will_call);
        instructions().append(callArguments.profileHookRegister()->index());
    }

    // Reserve space for call frame.
    Vector<RefPtr<RegisterID>, RegisterFile::CallFrameHeaderSize> callFrame;
    for (int i = 0; i < RegisterFile::CallFrameHeaderSize; ++i)
        callFrame.append(newTemporary());

    emitExpressionInfo(divot, startOffset, endOffset);

#if ENABLE(JIT)
    m_codeBlock->addCallLinkInfo();
#endif

    emitOpcode(op_construct);
    instructions().append(func->index()); // func
    instructions().append(callArguments.count()); // argCount
    instructions().append(callArguments.callFrame()); // registerOffset
    if (dst != ignoredResult()) {
        emitOpcode(op_call_put_result);
        instructions().append(dst->index()); // dst
    }

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

RegisterID* BytecodeGenerator::emitPushScope(RegisterID* scope)
{
    ASSERT(scope->isTemporary());
    ControlFlowContext context;
    context.isFinallyBlock = false;
    m_scopeContextStack.append(context);
    m_dynamicScopeDepth++;

    return emitUnaryNoDstOp(op_push_scope, scope);
}

void BytecodeGenerator::emitPopScope()
{
    ASSERT(m_scopeContextStack.size());
    ASSERT(!m_scopeContextStack.last().isFinallyBlock);

    emitOpcode(op_pop_scope);

    m_scopeContextStack.removeLast();
    m_dynamicScopeDepth--;
}

void BytecodeGenerator::emitDebugHook(DebugHookID debugHookID, int firstLine, int lastLine)
{
#if ENABLE(DEBUG_WITH_BREAKPOINT)
    if (debugHookID != DidReachBreakpoint)
        return;
#else
    if (!m_shouldEmitDebugHooks)
        return;
#endif
    emitOpcode(op_debug);
    instructions().append(debugHookID);
    instructions().append(firstLine);
    instructions().append(lastLine);
}

void BytecodeGenerator::pushFinallyContext(Label* target, RegisterID* retAddrDst)
{
    ControlFlowContext scope;
    scope.isFinallyBlock = true;
    FinallyContext context = { target, retAddrDst };
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

PassRefPtr<Label> BytecodeGenerator::emitComplexJumpScopes(Label* target, ControlFlowContext* topScope, ControlFlowContext* bottomScope)
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
            size_t begin = instructions().size();

            // We need to remove a number of dynamic scopes to get to the next
            // finally block
            emitOpcode(op_jmp_scopes);
            instructions().append(nNormalScopes);

            // If topScope == bottomScope then there isn't actually a finally block
            // left to emit, so make the jmp_scopes jump directly to the target label
            if (topScope == bottomScope) {
                instructions().append(target->bind(begin, instructions().size()));
                return target;
            }

            // Otherwise we just use jmp_scopes to pop a group of scopes and go
            // to the next instruction
            RefPtr<Label> nextInsn = newLabel();
            instructions().append(nextInsn->bind(begin, instructions().size()));
            emitLabel(nextInsn.get());
        }

        while (topScope > bottomScope && topScope->isFinallyBlock) {
            emitJumpSubroutine(topScope->finallyContext.retAddrDst, topScope->finallyContext.finallyAddr);
            --topScope;
        }
    }
    return emitJump(target);
}

PassRefPtr<Label> BytecodeGenerator::emitJumpScopes(Label* target, int targetScopeDepth)
{
    ASSERT(scopeDepth() - targetScopeDepth >= 0);
    ASSERT(target->isForward());

    size_t scopeDelta = scopeDepth() - targetScopeDepth;
    ASSERT(scopeDelta <= m_scopeContextStack.size());
    if (!scopeDelta)
        return emitJump(target);

    if (m_finallyDepth)
        return emitComplexJumpScopes(target, &m_scopeContextStack.last(), &m_scopeContextStack.last() - scopeDelta);

    size_t begin = instructions().size();

    emitOpcode(op_jmp_scopes);
    instructions().append(scopeDelta);
    instructions().append(target->bind(begin, instructions().size()));
    return target;
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

RegisterID* BytecodeGenerator::emitCatch(RegisterID* targetRegister, Label* start, Label* end)
{
    m_usesExceptions = true;
#if ENABLE(JIT)
    HandlerInfo info = { start->bind(0, 0), end->bind(0, 0), instructions().size(), m_dynamicScopeDepth + m_baseScopeDepth, CodeLocationLabel() };
#else
    HandlerInfo info = { start->bind(0, 0), end->bind(0, 0), instructions().size(), m_dynamicScopeDepth + m_baseScopeDepth };
#endif

    m_codeBlock->addExceptionHandler(info);
    emitOpcode(op_catch);
    instructions().append(targetRegister->index());
    return targetRegister;
}

void BytecodeGenerator::emitThrowReferenceError(const UString& message)
{
    emitOpcode(op_throw_reference_error);
    instructions().append(addConstantValue(jsString(globalData(), message))->index());
}

PassRefPtr<Label> BytecodeGenerator::emitJumpSubroutine(RegisterID* retAddrDst, Label* finally)
{
    size_t begin = instructions().size();

    emitOpcode(op_jsr);
    instructions().append(retAddrDst->index());
    instructions().append(finally->bind(begin, instructions().size()));
    emitLabel(newLabel().get()); // Record the fact that the next instruction is implicitly labeled, because op_sret will return to it.
    return finally;
}

void BytecodeGenerator::emitSubroutineReturn(RegisterID* retAddrSrc)
{
    emitOpcode(op_sret);
    instructions().append(retAddrSrc->index());
}

void BytecodeGenerator::emitPushNewScope(RegisterID* dst, const Identifier& property, RegisterID* value)
{
    ControlFlowContext context;
    context.isFinallyBlock = false;
    m_scopeContextStack.append(context);
    m_dynamicScopeDepth++;

    emitOpcode(op_push_new_scope);
    instructions().append(dst->index());
    instructions().append(addConstant(property));
    instructions().append(value->index());
}

void BytecodeGenerator::beginSwitch(RegisterID* scrutineeRegister, SwitchInfo::SwitchType type)
{
    SwitchInfo info = { instructions().size(), type };
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
            ASSERT_NOT_REACHED();
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

static void prepareJumpTableForImmediateSwitch(SimpleJumpTable& jumpTable, int32_t switchAddress, uint32_t clauseCount, RefPtr<Label>* labels, ExpressionNode** nodes, int32_t min, int32_t max)
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
    
    int32_t key = clause->characters()[0];
    ASSERT(key >= min);
    ASSERT(key <= max);
    return key - min;
}

static void prepareJumpTableForCharacterSwitch(SimpleJumpTable& jumpTable, int32_t switchAddress, uint32_t clauseCount, RefPtr<Label>* labels, ExpressionNode** nodes, int32_t min, int32_t max)
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

static void prepareJumpTableForStringSwitch(StringJumpTable& jumpTable, int32_t switchAddress, uint32_t clauseCount, RefPtr<Label>* labels, ExpressionNode** nodes)
{
    for (uint32_t i = 0; i < clauseCount; ++i) {
        // We're emitting this after the clause labels should have been fixed, so 
        // the labels should not be "forward" references
        ASSERT(!labels[i]->isForward());
        
        ASSERT(nodes[i]->isString());
        StringImpl* clause = static_cast<StringNode*>(nodes[i])->value().impl();
        OffsetLocation location;
        location.branchOffset = labels[i]->bind(switchAddress, switchAddress + 3);
        jumpTable.offsetTable.add(clause, location);
    }
}

void BytecodeGenerator::endSwitch(uint32_t clauseCount, RefPtr<Label>* labels, ExpressionNode** nodes, Label* defaultLabel, int32_t min, int32_t max)
{
    SwitchInfo switchInfo = m_switchContextStack.last();
    m_switchContextStack.removeLast();
    if (switchInfo.switchType == SwitchInfo::SwitchImmediate) {
        instructions()[switchInfo.bytecodeOffset + 1] = m_codeBlock->numberOfImmediateSwitchJumpTables();
        instructions()[switchInfo.bytecodeOffset + 2] = defaultLabel->bind(switchInfo.bytecodeOffset, switchInfo.bytecodeOffset + 3);

        SimpleJumpTable& jumpTable = m_codeBlock->addImmediateSwitchJumpTable();
        prepareJumpTableForImmediateSwitch(jumpTable, switchInfo.bytecodeOffset, clauseCount, labels, nodes, min, max);
    } else if (switchInfo.switchType == SwitchInfo::SwitchCharacter) {
        instructions()[switchInfo.bytecodeOffset + 1] = m_codeBlock->numberOfCharacterSwitchJumpTables();
        instructions()[switchInfo.bytecodeOffset + 2] = defaultLabel->bind(switchInfo.bytecodeOffset, switchInfo.bytecodeOffset + 3);
        
        SimpleJumpTable& jumpTable = m_codeBlock->addCharacterSwitchJumpTable();
        prepareJumpTableForCharacterSwitch(jumpTable, switchInfo.bytecodeOffset, clauseCount, labels, nodes, min, max);
    } else {
        ASSERT(switchInfo.switchType == SwitchInfo::SwitchString);
        instructions()[switchInfo.bytecodeOffset + 1] = m_codeBlock->numberOfStringSwitchJumpTables();
        instructions()[switchInfo.bytecodeOffset + 2] = defaultLabel->bind(switchInfo.bytecodeOffset, switchInfo.bytecodeOffset + 3);

        StringJumpTable& jumpTable = m_codeBlock->addStringSwitchJumpTable();
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

int BytecodeGenerator::argumentNumberFor(const Identifier& ident)
{
    int parameterCount = m_parameters.size(); // includes 'this'
    RegisterID* registerID = registerFor(ident);
    if (!registerID)
        return 0;
    int index = registerID->index() + RegisterFile::CallFrameHeaderSize + parameterCount;
    return (index > 0 && index < parameterCount) ? index : 0;
}

} // namespace JSC
