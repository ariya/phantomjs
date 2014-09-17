/*
 * Copyright (C) 2008, 2009, 2010 Apple Inc. All rights reserved.
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

#ifndef CodeBlock_h
#define CodeBlock_h

#include "EvalCodeCache.h"
#include "Instruction.h"
#include "JITCode.h"
#include "JSGlobalObject.h"
#include "JumpTable.h"
#include "Nodes.h"
#include "RegExp.h"
#include "UString.h"
#include <wtf/FastAllocBase.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>

#if ENABLE(JIT)
#include "StructureStubInfo.h"
#endif

// Register numbers used in bytecode operations have different meaning according to their ranges:
//      0x80000000-0xFFFFFFFF  Negative indices from the CallFrame pointer are entries in the call frame, see RegisterFile.h.
//      0x00000000-0x3FFFFFFF  Forwards indices from the CallFrame pointer are local vars and temporaries with the function's callframe.
//      0x40000000-0x7FFFFFFF  Positive indices from 0x40000000 specify entries in the constant pool on the CodeBlock.
static const int FirstConstantRegisterIndex = 0x40000000;

namespace JSC {

    enum HasSeenShouldRepatch {
        hasSeenShouldRepatch
    };

    class ExecState;

    enum CodeType { GlobalCode, EvalCode, FunctionCode };

    inline int unmodifiedArgumentsRegister(int argumentsRegister) { return argumentsRegister - 1; }

    static ALWAYS_INLINE int missingThisObjectMarker() { return std::numeric_limits<int>::max(); }

    struct HandlerInfo {
        uint32_t start;
        uint32_t end;
        uint32_t target;
        uint32_t scopeDepth;
#if ENABLE(JIT)
        CodeLocationLabel nativeCode;
#endif
    };

    struct ExpressionRangeInfo {
        enum {
            MaxOffset = (1 << 7) - 1, 
            MaxDivot = (1 << 25) - 1
        };
        uint32_t instructionOffset : 25;
        uint32_t divotPoint : 25;
        uint32_t startOffset : 7;
        uint32_t endOffset : 7;
    };

    struct LineInfo {
        uint32_t instructionOffset;
        int32_t lineNumber;
    };

#if ENABLE(JIT)
    struct CallLinkInfo {
        CallLinkInfo()
            : hasSeenShouldRepatch(false)
        {
        }

        CodeLocationNearCall callReturnLocation;
        CodeLocationDataLabelPtr hotPathBegin;
        CodeLocationNearCall hotPathOther;
        WriteBarrier<JSFunction> callee;
        bool hasSeenShouldRepatch;
        
        void setUnlinked() { callee.clear(); }
        bool isLinked() { return callee; }

        bool seenOnce()
        {
            return hasSeenShouldRepatch;
        }

        void setSeen()
        {
            hasSeenShouldRepatch = true;
        }
    };

    struct MethodCallLinkInfo {
        MethodCallLinkInfo()
        {
        }

        bool seenOnce()
        {
            ASSERT(!cachedStructure);
            return cachedPrototypeStructure;
        }

        void setSeen()
        {
            ASSERT(!cachedStructure && !cachedPrototypeStructure);
            // We use the values of cachedStructure & cachedPrototypeStructure to indicate the
            // current state.
            //     - In the initial state, both are null.
            //     - Once this transition has been taken once, cachedStructure is
            //       null and cachedPrototypeStructure is set to a nun-null value.
            //     - Once the call is linked both structures are set to non-null values.
            cachedPrototypeStructure.setWithoutWriteBarrier((Structure*)1);
        }

        CodeLocationCall callReturnLocation;
        CodeLocationDataLabelPtr structureLabel;
        WriteBarrier<Structure> cachedStructure;
        WriteBarrier<Structure> cachedPrototypeStructure;
    };

    struct GlobalResolveInfo {
        GlobalResolveInfo(unsigned bytecodeOffset)
            : offset(0)
            , bytecodeOffset(bytecodeOffset)
        {
        }

        WriteBarrier<Structure> structure;
        unsigned offset;
        unsigned bytecodeOffset;
    };

    // This structure is used to map from a call return location
    // (given as an offset in bytes into the JIT code) back to
    // the bytecode index of the corresponding bytecode operation.
    // This is then used to look up the corresponding handler.
    struct CallReturnOffsetToBytecodeOffset {
        CallReturnOffsetToBytecodeOffset(unsigned callReturnOffset, unsigned bytecodeOffset)
            : callReturnOffset(callReturnOffset)
            , bytecodeOffset(bytecodeOffset)
        {
        }

        unsigned callReturnOffset;
        unsigned bytecodeOffset;
    };

    // valueAtPosition helpers for the binarySearch algorithm.

    inline void* getStructureStubInfoReturnLocation(StructureStubInfo* structureStubInfo)
    {
        return structureStubInfo->callReturnLocation.executableAddress();
    }

    inline void* getCallLinkInfoReturnLocation(CallLinkInfo* callLinkInfo)
    {
        return callLinkInfo->callReturnLocation.executableAddress();
    }

    inline void* getMethodCallLinkInfoReturnLocation(MethodCallLinkInfo* methodCallLinkInfo)
    {
        return methodCallLinkInfo->callReturnLocation.executableAddress();
    }

    inline unsigned getCallReturnOffset(CallReturnOffsetToBytecodeOffset* pc)
    {
        return pc->callReturnOffset;
    }
#endif

    class CodeBlock {
        WTF_MAKE_FAST_ALLOCATED;
        friend class JIT;
    protected:
        CodeBlock(ScriptExecutable* ownerExecutable, CodeType, JSGlobalObject*, PassRefPtr<SourceProvider>, unsigned sourceOffset, SymbolTable* symbolTable, bool isConstructor);

        WriteBarrier<JSGlobalObject> m_globalObject;
        Heap* m_heap;

    public:
        virtual ~CodeBlock();

        void visitAggregate(SlotVisitor&);

        static void dumpStatistics();

#if !defined(NDEBUG) || ENABLE_OPCODE_SAMPLING
        void dump(ExecState*) const;
        void printStructures(const Instruction*) const;
        void printStructure(const char* name, const Instruction*, int operand) const;
#endif

        bool isStrictMode() const { return m_isStrictMode; }

        inline bool isKnownNotImmediate(int index)
        {
            if (index == m_thisRegister && !m_isStrictMode)
                return true;

            if (isConstantRegisterIndex(index))
                return getConstant(index).isCell();

            return false;
        }

        ALWAYS_INLINE bool isTemporaryRegisterIndex(int index)
        {
            return index >= m_numVars;
        }

        HandlerInfo* handlerForBytecodeOffset(unsigned bytecodeOffset);
        int lineNumberForBytecodeOffset(unsigned bytecodeOffset);
        void expressionRangeForBytecodeOffset(unsigned bytecodeOffset, int& divot, int& startOffset, int& endOffset);

#if ENABLE(JIT)

        StructureStubInfo& getStubInfo(ReturnAddressPtr returnAddress)
        {
            return *(binarySearch<StructureStubInfo, void*, getStructureStubInfoReturnLocation>(m_structureStubInfos.begin(), m_structureStubInfos.size(), returnAddress.value()));
        }

        CallLinkInfo& getCallLinkInfo(ReturnAddressPtr returnAddress)
        {
            return *(binarySearch<CallLinkInfo, void*, getCallLinkInfoReturnLocation>(m_callLinkInfos.begin(), m_callLinkInfos.size(), returnAddress.value()));
        }

        MethodCallLinkInfo& getMethodCallLinkInfo(ReturnAddressPtr returnAddress)
        {
            return *(binarySearch<MethodCallLinkInfo, void*, getMethodCallLinkInfoReturnLocation>(m_methodCallLinkInfos.begin(), m_methodCallLinkInfos.size(), returnAddress.value()));
        }

        unsigned bytecodeOffset(ReturnAddressPtr returnAddress)
        {
            if (!m_rareData)
                return 1;
            Vector<CallReturnOffsetToBytecodeOffset>& callIndices = m_rareData->m_callReturnIndexVector;
            if (!callIndices.size())
                return 1;
            return binarySearch<CallReturnOffsetToBytecodeOffset, unsigned, getCallReturnOffset>(callIndices.begin(), callIndices.size(), getJITCode().offsetOf(returnAddress.value()))->bytecodeOffset;
        }
#endif
#if ENABLE(INTERPRETER)
        unsigned bytecodeOffset(Instruction* returnAddress)
        {
            return static_cast<Instruction*>(returnAddress) - instructions().begin();
        }
#endif

        void setIsNumericCompareFunction(bool isNumericCompareFunction) { m_isNumericCompareFunction = isNumericCompareFunction; }
        bool isNumericCompareFunction() { return m_isNumericCompareFunction; }

        Vector<Instruction>& instructions() { return m_instructions; }
        void discardBytecode() { m_instructions.clear(); }

#ifndef NDEBUG
        unsigned instructionCount() { return m_instructionCount; }
        void setInstructionCount(unsigned instructionCount) { m_instructionCount = instructionCount; }
#endif

#if ENABLE(JIT)
        JITCode& getJITCode() { return m_isConstructor ? ownerExecutable()->generatedJITCodeForConstruct() : ownerExecutable()->generatedJITCodeForCall(); }
        ExecutablePool* executablePool() { return getJITCode().getExecutablePool(); }
#endif

        ScriptExecutable* ownerExecutable() const { return m_ownerExecutable.get(); }

        void setGlobalData(JSGlobalData* globalData) { m_globalData = globalData; }

        void setThisRegister(int thisRegister) { m_thisRegister = thisRegister; }
        int thisRegister() const { return m_thisRegister; }

        void setNeedsFullScopeChain(bool needsFullScopeChain) { m_needsFullScopeChain = needsFullScopeChain; }
        bool needsFullScopeChain() const { return m_needsFullScopeChain; }
        void setUsesEval(bool usesEval) { m_usesEval = usesEval; }
        bool usesEval() const { return m_usesEval; }
        
        void setArgumentsRegister(int argumentsRegister)
        {
            ASSERT(argumentsRegister != -1);
            m_argumentsRegister = argumentsRegister;
            ASSERT(usesArguments());
        }
        int argumentsRegister()
        {
            ASSERT(usesArguments());
            return m_argumentsRegister;
        }
        void setActivationRegister(int activationRegister)
        {
            m_activationRegister = activationRegister;
        }
        int activationRegister()
        {
            ASSERT(needsFullScopeChain());
            return m_activationRegister;
        }
        bool usesArguments() const { return m_argumentsRegister != -1; }

        CodeType codeType() const { return m_codeType; }

        SourceProvider* source() const { return m_source.get(); }
        unsigned sourceOffset() const { return m_sourceOffset; }

        size_t numberOfJumpTargets() const { return m_jumpTargets.size(); }
        void addJumpTarget(unsigned jumpTarget) { m_jumpTargets.append(jumpTarget); }
        unsigned jumpTarget(int index) const { return m_jumpTargets[index]; }
        unsigned lastJumpTarget() const { return m_jumpTargets.last(); }

        void createActivation(CallFrame*);

#if ENABLE(INTERPRETER)
        void addPropertyAccessInstruction(unsigned propertyAccessInstruction)
        {
            if (!m_globalData->canUseJIT())
                m_propertyAccessInstructions.append(propertyAccessInstruction);
        }
        void addGlobalResolveInstruction(unsigned globalResolveInstruction)
        {
            if (!m_globalData->canUseJIT())
                m_globalResolveInstructions.append(globalResolveInstruction);
        }
        bool hasGlobalResolveInstructionAtBytecodeOffset(unsigned bytecodeOffset);
#endif
#if ENABLE(JIT)
        size_t numberOfStructureStubInfos() const { return m_structureStubInfos.size(); }
        void addStructureStubInfo(const StructureStubInfo& stubInfo)
        {
            if (m_globalData->canUseJIT())
                m_structureStubInfos.append(stubInfo);
        }
        StructureStubInfo& structureStubInfo(int index) { return m_structureStubInfos[index]; }

        void addGlobalResolveInfo(unsigned globalResolveInstruction)
        {
            if (m_globalData->canUseJIT())
                m_globalResolveInfos.append(GlobalResolveInfo(globalResolveInstruction));
        }
        GlobalResolveInfo& globalResolveInfo(int index) { return m_globalResolveInfos[index]; }
        bool hasGlobalResolveInfoAtBytecodeOffset(unsigned bytecodeOffset);

        size_t numberOfCallLinkInfos() const { return m_callLinkInfos.size(); }
        void addCallLinkInfo() { m_callLinkInfos.append(CallLinkInfo()); }
        CallLinkInfo& callLinkInfo(int index) { return m_callLinkInfos[index]; }

        void addMethodCallLinkInfos(unsigned n) { m_methodCallLinkInfos.grow(n); }
        MethodCallLinkInfo& methodCallLinkInfo(int index) { return m_methodCallLinkInfos[index]; }
#endif

        // Exception handling support

        size_t numberOfExceptionHandlers() const { return m_rareData ? m_rareData->m_exceptionHandlers.size() : 0; }
        void addExceptionHandler(const HandlerInfo& hanler) { createRareDataIfNecessary(); return m_rareData->m_exceptionHandlers.append(hanler); }
        HandlerInfo& exceptionHandler(int index) { ASSERT(m_rareData); return m_rareData->m_exceptionHandlers[index]; }

        void addExpressionInfo(const ExpressionRangeInfo& expressionInfo)
        {
            createRareDataIfNecessary();
            m_rareData->m_expressionInfo.append(expressionInfo);
        }

        void addLineInfo(unsigned bytecodeOffset, int lineNo)
        {
            createRareDataIfNecessary();
            Vector<LineInfo>& lineInfo = m_rareData->m_lineInfo;
            if (!lineInfo.size() || lineInfo.last().lineNumber != lineNo) {
                LineInfo info = { bytecodeOffset, lineNo };
                lineInfo.append(info);
            }
        }

        bool hasExpressionInfo() { return m_rareData && m_rareData->m_expressionInfo.size(); }
        bool hasLineInfo() { return m_rareData && m_rareData->m_lineInfo.size(); }
        //  We only generate exception handling info if the user is debugging
        // (and may want line number info), or if the function contains exception handler.
        bool needsCallReturnIndices()
        {
            return m_rareData &&
                (m_rareData->m_expressionInfo.size() || m_rareData->m_lineInfo.size() || m_rareData->m_exceptionHandlers.size());
        }

#if ENABLE(JIT)
        Vector<CallReturnOffsetToBytecodeOffset>& callReturnIndexVector()
        {
            createRareDataIfNecessary();
            return m_rareData->m_callReturnIndexVector;
        }
#endif

        // Constant Pool

        size_t numberOfIdentifiers() const { return m_identifiers.size(); }
        void addIdentifier(const Identifier& i) { return m_identifiers.append(i); }
        Identifier& identifier(int index) { return m_identifiers[index]; }

        size_t numberOfConstantRegisters() const { return m_constantRegisters.size(); }
        void addConstant(JSValue v)
        {
            m_constantRegisters.append(WriteBarrier<Unknown>());
            m_constantRegisters.last().set(m_globalObject->globalData(), m_ownerExecutable.get(), v);
        }
        WriteBarrier<Unknown>& constantRegister(int index) { return m_constantRegisters[index - FirstConstantRegisterIndex]; }
        ALWAYS_INLINE bool isConstantRegisterIndex(int index) const { return index >= FirstConstantRegisterIndex; }
        ALWAYS_INLINE JSValue getConstant(int index) const { return m_constantRegisters[index - FirstConstantRegisterIndex].get(); }

        unsigned addFunctionDecl(FunctionExecutable* n)
        {
            unsigned size = m_functionDecls.size();
            m_functionDecls.append(WriteBarrier<FunctionExecutable>());
            m_functionDecls.last().set(m_globalObject->globalData(), m_ownerExecutable.get(), n);
            return size;
        }
        FunctionExecutable* functionDecl(int index) { return m_functionDecls[index].get(); }
        int numberOfFunctionDecls() { return m_functionDecls.size(); }
        unsigned addFunctionExpr(FunctionExecutable* n)
        {
            unsigned size = m_functionExprs.size();
            m_functionExprs.append(WriteBarrier<FunctionExecutable>());
            m_functionExprs.last().set(m_globalObject->globalData(), m_ownerExecutable.get(), n);
            return size;
        }
        FunctionExecutable* functionExpr(int index) { return m_functionExprs[index].get(); }

        unsigned addRegExp(PassRefPtr<RegExp> r) { createRareDataIfNecessary(); unsigned size = m_rareData->m_regexps.size(); m_rareData->m_regexps.append(r); return size; }
        RegExp* regexp(int index) const { ASSERT(m_rareData); return m_rareData->m_regexps[index].get(); }

        JSGlobalObject* globalObject() { return m_globalObject.get(); }

        // Jump Tables

        size_t numberOfImmediateSwitchJumpTables() const { return m_rareData ? m_rareData->m_immediateSwitchJumpTables.size() : 0; }
        SimpleJumpTable& addImmediateSwitchJumpTable() { createRareDataIfNecessary(); m_rareData->m_immediateSwitchJumpTables.append(SimpleJumpTable()); return m_rareData->m_immediateSwitchJumpTables.last(); }
        SimpleJumpTable& immediateSwitchJumpTable(int tableIndex) { ASSERT(m_rareData); return m_rareData->m_immediateSwitchJumpTables[tableIndex]; }

        size_t numberOfCharacterSwitchJumpTables() const { return m_rareData ? m_rareData->m_characterSwitchJumpTables.size() : 0; }
        SimpleJumpTable& addCharacterSwitchJumpTable() { createRareDataIfNecessary(); m_rareData->m_characterSwitchJumpTables.append(SimpleJumpTable()); return m_rareData->m_characterSwitchJumpTables.last(); }
        SimpleJumpTable& characterSwitchJumpTable(int tableIndex) { ASSERT(m_rareData); return m_rareData->m_characterSwitchJumpTables[tableIndex]; }

        size_t numberOfStringSwitchJumpTables() const { return m_rareData ? m_rareData->m_stringSwitchJumpTables.size() : 0; }
        StringJumpTable& addStringSwitchJumpTable() { createRareDataIfNecessary(); m_rareData->m_stringSwitchJumpTables.append(StringJumpTable()); return m_rareData->m_stringSwitchJumpTables.last(); }
        StringJumpTable& stringSwitchJumpTable(int tableIndex) { ASSERT(m_rareData); return m_rareData->m_stringSwitchJumpTables[tableIndex]; }


        SymbolTable* symbolTable() { return m_symbolTable; }
        SharedSymbolTable* sharedSymbolTable() { ASSERT(m_codeType == FunctionCode); return static_cast<SharedSymbolTable*>(m_symbolTable); }

        EvalCodeCache& evalCodeCache() { createRareDataIfNecessary(); return m_rareData->m_evalCodeCache; }

        void shrinkToFit();

        // FIXME: Make these remaining members private.

        int m_numCalleeRegisters;
        int m_numVars;
        int m_numCapturedVars;
        int m_numParameters;
        bool m_isConstructor;

    private:
#if !defined(NDEBUG) || ENABLE(OPCODE_SAMPLING)
        void dump(ExecState*, const Vector<Instruction>::const_iterator& begin, Vector<Instruction>::const_iterator&) const;

        CString registerName(ExecState*, int r) const;
        void printUnaryOp(ExecState*, int location, Vector<Instruction>::const_iterator&, const char* op) const;
        void printBinaryOp(ExecState*, int location, Vector<Instruction>::const_iterator&, const char* op) const;
        void printConditionalJump(ExecState*, const Vector<Instruction>::const_iterator&, Vector<Instruction>::const_iterator&, int location, const char* op) const;
        void printGetByIdOp(ExecState*, int location, Vector<Instruction>::const_iterator&, const char* op) const;
        void printPutByIdOp(ExecState*, int location, Vector<Instruction>::const_iterator&, const char* op) const;
#endif
        void visitStructures(SlotVisitor&, Instruction* vPC) const;

        void createRareDataIfNecessary()
        {
            if (!m_rareData)
                m_rareData = adoptPtr(new RareData);
        }

        WriteBarrier<ScriptExecutable> m_ownerExecutable;
        JSGlobalData* m_globalData;

        Vector<Instruction> m_instructions;
#ifndef NDEBUG
        unsigned m_instructionCount;
#endif

        int m_thisRegister;
        int m_argumentsRegister;
        int m_activationRegister;

        bool m_needsFullScopeChain;
        bool m_usesEval;
        bool m_isNumericCompareFunction;
        bool m_isStrictMode;

        CodeType m_codeType;

        RefPtr<SourceProvider> m_source;
        unsigned m_sourceOffset;

#if ENABLE(INTERPRETER)
        Vector<unsigned> m_propertyAccessInstructions;
        Vector<unsigned> m_globalResolveInstructions;
#endif
#if ENABLE(JIT)
        Vector<StructureStubInfo> m_structureStubInfos;
        Vector<GlobalResolveInfo> m_globalResolveInfos;
        Vector<CallLinkInfo> m_callLinkInfos;
        Vector<MethodCallLinkInfo> m_methodCallLinkInfos;
#endif

        Vector<unsigned> m_jumpTargets;

        // Constant Pool
        Vector<Identifier> m_identifiers;
        COMPILE_ASSERT(sizeof(Register) == sizeof(WriteBarrier<Unknown>), Register_must_be_same_size_as_WriteBarrier_Unknown);
        Vector<WriteBarrier<Unknown> > m_constantRegisters;
        Vector<WriteBarrier<FunctionExecutable> > m_functionDecls;
        Vector<WriteBarrier<FunctionExecutable> > m_functionExprs;

        SymbolTable* m_symbolTable;

        struct RareData {
           WTF_MAKE_FAST_ALLOCATED;
        public:
            Vector<HandlerInfo> m_exceptionHandlers;

            // Rare Constants
            Vector<RefPtr<RegExp> > m_regexps;

            // Jump Tables
            Vector<SimpleJumpTable> m_immediateSwitchJumpTables;
            Vector<SimpleJumpTable> m_characterSwitchJumpTables;
            Vector<StringJumpTable> m_stringSwitchJumpTables;

            EvalCodeCache m_evalCodeCache;

            // Expression info - present if debugging.
            Vector<ExpressionRangeInfo> m_expressionInfo;
            // Line info - present if profiling or debugging.
            Vector<LineInfo> m_lineInfo;
#if ENABLE(JIT)
            Vector<CallReturnOffsetToBytecodeOffset> m_callReturnIndexVector;
#endif
        };
#if COMPILER(MSVC)
        friend void WTF::deleteOwnedPtr<RareData>(RareData*);
#endif
        OwnPtr<RareData> m_rareData;
    };

    // Program code is not marked by any function, so we make the global object
    // responsible for marking it.

    class GlobalCodeBlock : public CodeBlock {
    public:
        GlobalCodeBlock(ScriptExecutable* ownerExecutable, CodeType codeType, JSGlobalObject* globalObject, PassRefPtr<SourceProvider> sourceProvider, unsigned sourceOffset)
            : CodeBlock(ownerExecutable, codeType, globalObject, sourceProvider, sourceOffset, &m_unsharedSymbolTable, false)
        {
        }

    private:
        SymbolTable m_unsharedSymbolTable;
    };

    class ProgramCodeBlock : public GlobalCodeBlock {
    public:
        ProgramCodeBlock(ProgramExecutable* ownerExecutable, CodeType codeType, JSGlobalObject* globalObject, PassRefPtr<SourceProvider> sourceProvider)
            : GlobalCodeBlock(ownerExecutable, codeType, globalObject, sourceProvider, 0)
        {
        }
    };

    class EvalCodeBlock : public GlobalCodeBlock {
    public:
        EvalCodeBlock(EvalExecutable* ownerExecutable, JSGlobalObject* globalObject, PassRefPtr<SourceProvider> sourceProvider, int baseScopeDepth)
            : GlobalCodeBlock(ownerExecutable, EvalCode, globalObject, sourceProvider, 0)
            , m_baseScopeDepth(baseScopeDepth)
        {
        }

        int baseScopeDepth() const { return m_baseScopeDepth; }

        const Identifier& variable(unsigned index) { return m_variables[index]; }
        unsigned numVariables() { return m_variables.size(); }
        void adoptVariables(Vector<Identifier>& variables)
        {
            ASSERT(m_variables.isEmpty());
            m_variables.swap(variables);
        }

    private:
        int m_baseScopeDepth;
        Vector<Identifier> m_variables;
    };

    class FunctionCodeBlock : public CodeBlock {
    public:
        // Rather than using the usual RefCounted::create idiom for SharedSymbolTable we just use new
        // as we need to initialise the CodeBlock before we could initialise any RefPtr to hold the shared
        // symbol table, so we just pass as a raw pointer with a ref count of 1.  We then manually deref
        // in the destructor.
        FunctionCodeBlock(FunctionExecutable* ownerExecutable, CodeType codeType, JSGlobalObject* globalObject, PassRefPtr<SourceProvider> sourceProvider, unsigned sourceOffset, bool isConstructor)
            : CodeBlock(ownerExecutable, codeType, globalObject, sourceProvider, sourceOffset, SharedSymbolTable::create().leakRef(), isConstructor)
        {
        }
        ~FunctionCodeBlock()
        {
            sharedSymbolTable()->deref();
        }
    };

    inline Register& ExecState::r(int index)
    {
        CodeBlock* codeBlock = this->codeBlock();
        if (codeBlock->isConstantRegisterIndex(index))
            return *reinterpret_cast<Register*>(&codeBlock->constantRegister(index));
        return this[index];
    }

    inline Register& ExecState::uncheckedR(int index)
    {
        ASSERT(index < FirstConstantRegisterIndex);
        return this[index];
    }
    
} // namespace JSC

#endif // CodeBlock_h
