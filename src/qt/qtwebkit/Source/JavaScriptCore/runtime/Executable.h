/*
 * Copyright (C) 2009, 2010, 2013 Apple Inc. All rights reserved.
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

#ifndef Executable_h
#define Executable_h

#include "CallData.h"
#include "CodeBlockHash.h"
#include "CodeSpecializationKind.h"
#include "HandlerInfo.h"
#include "JSFunction.h"
#include "Interpreter.h"
#include "JITCode.h"
#include "JSGlobalObject.h"
#include "LLIntCLoop.h"
#include "SamplingTool.h"
#include "SourceCode.h"
#include "UnlinkedCodeBlock.h"
#include <wtf/PassOwnPtr.h>

namespace JSC {

    class CodeBlock;
    class Debugger;
    class EvalCodeBlock;
    class FunctionCodeBlock;
    class LLIntOffsetsExtractor;
    class ProgramCodeBlock;
    class JSScope;
    
    enum CompilationKind { FirstCompilation, OptimizingCompilation };

    inline bool isCall(CodeSpecializationKind kind)
    {
        if (kind == CodeForCall)
            return true;
        ASSERT(kind == CodeForConstruct);
        return false;
    }

    class ExecutableBase : public JSCell, public DoublyLinkedListNode<ExecutableBase> {
        friend class WTF::DoublyLinkedListNode<ExecutableBase>;
        friend class JIT;

    protected:
        static const int NUM_PARAMETERS_IS_HOST = 0;
        static const int NUM_PARAMETERS_NOT_COMPILED = -1;

        ExecutableBase(VM& vm, Structure* structure, int numParameters)
            : JSCell(vm, structure)
            , m_numParametersForCall(numParameters)
            , m_numParametersForConstruct(numParameters)
        {
        }

        void finishCreation(VM& vm)
        {
            Base::finishCreation(vm);
        }

    public:
        typedef JSCell Base;

#if ENABLE(JIT)
        static const bool needsDestruction = true;
        static const bool hasImmortalStructure = true;
        static void destroy(JSCell*);
#endif
        
        CodeBlockHash hashFor(CodeSpecializationKind) const;

        bool isFunctionExecutable()
        {
            return structure()->typeInfo().type() == FunctionExecutableType;
        }

        bool isHostFunction() const
        {
            ASSERT((m_numParametersForCall == NUM_PARAMETERS_IS_HOST) == (m_numParametersForConstruct == NUM_PARAMETERS_IS_HOST));
            return m_numParametersForCall == NUM_PARAMETERS_IS_HOST;
        }

        static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue proto) { return Structure::create(vm, globalObject, proto, TypeInfo(CompoundType, StructureFlags), &s_info); }
        
        void clearCode();

        static JS_EXPORTDATA const ClassInfo s_info;

    protected:
        static const unsigned StructureFlags = 0;
        int m_numParametersForCall;
        int m_numParametersForConstruct;

    public:
        static void clearCodeVirtual(ExecutableBase*);

#if ENABLE(JIT)
        JITCode& generatedJITCodeForCall()
        {
            ASSERT(m_jitCodeForCall);
            return m_jitCodeForCall;
        }

        JITCode& generatedJITCodeForConstruct()
        {
            ASSERT(m_jitCodeForConstruct);
            return m_jitCodeForConstruct;
        }
        
        JITCode& generatedJITCodeFor(CodeSpecializationKind kind)
        {
            if (kind == CodeForCall)
                return generatedJITCodeForCall();
            ASSERT(kind == CodeForConstruct);
            return generatedJITCodeForConstruct();
        }

        MacroAssemblerCodePtr generatedJITCodeForCallWithArityCheck()
        {
            ASSERT(m_jitCodeForCall);
            ASSERT(m_jitCodeForCallWithArityCheck);
            return m_jitCodeForCallWithArityCheck;
        }

        MacroAssemblerCodePtr generatedJITCodeForConstructWithArityCheck()
        {
            ASSERT(m_jitCodeForConstruct);
            ASSERT(m_jitCodeForConstructWithArityCheck);
            return m_jitCodeForConstructWithArityCheck;
        }
        
        MacroAssemblerCodePtr generatedJITCodeWithArityCheckFor(CodeSpecializationKind kind)
        {
            if (kind == CodeForCall)
                return generatedJITCodeForCallWithArityCheck();
            ASSERT(kind == CodeForConstruct);
            return generatedJITCodeForConstructWithArityCheck();
        }
        
        bool hasJITCodeForCall() const
        {
            return m_numParametersForCall >= 0;
        }
        
        bool hasJITCodeForConstruct() const
        {
            return m_numParametersForConstruct >= 0;
        }
        
        bool hasJITCodeFor(CodeSpecializationKind kind) const
        {
            if (kind == CodeForCall)
                return hasJITCodeForCall();
            ASSERT(kind == CodeForConstruct);
            return hasJITCodeForConstruct();
        }

        static ptrdiff_t offsetOfJITCodeFor(CodeSpecializationKind kind)
        {
            if (kind == CodeForCall)
                return OBJECT_OFFSETOF(ExecutableBase, m_jitCodeForCall);
            ASSERT(kind == CodeForConstruct);
            return OBJECT_OFFSETOF(ExecutableBase, m_jitCodeForConstruct);
        }
        
        static ptrdiff_t offsetOfJITCodeWithArityCheckFor(CodeSpecializationKind kind)
        {
            if (kind == CodeForCall)
                return OBJECT_OFFSETOF(ExecutableBase, m_jitCodeForCallWithArityCheck);
            ASSERT(kind == CodeForConstruct);
            return OBJECT_OFFSETOF(ExecutableBase, m_jitCodeForConstructWithArityCheck);
        }
        
        static ptrdiff_t offsetOfNumParametersFor(CodeSpecializationKind kind)
        {
            if (kind == CodeForCall)
                return OBJECT_OFFSETOF(ExecutableBase, m_numParametersForCall);
            ASSERT(kind == CodeForConstruct);
            return OBJECT_OFFSETOF(ExecutableBase, m_numParametersForConstruct);
        }
#endif // ENABLE(JIT)

        // Intrinsics are only for calls, currently.
        Intrinsic intrinsic() const;
        
        Intrinsic intrinsicFor(CodeSpecializationKind kind) const
        {
            if (isCall(kind))
                return intrinsic();
            return NoIntrinsic;
        }
        
#if ENABLE(JIT) || ENABLE(LLINT_C_LOOP)
        MacroAssemblerCodePtr hostCodeEntryFor(CodeSpecializationKind kind)
        {
            #if ENABLE(JIT)
            return generatedJITCodeFor(kind).addressForCall();
            #else
            return LLInt::CLoop::hostCodeEntryFor(kind);
            #endif
        }

        MacroAssemblerCodePtr jsCodeEntryFor(CodeSpecializationKind kind)
        {
            #if ENABLE(JIT)
            return generatedJITCodeFor(kind).addressForCall();
            #else
            return LLInt::CLoop::jsCodeEntryFor(kind);
            #endif
        }

        MacroAssemblerCodePtr jsCodeWithArityCheckEntryFor(CodeSpecializationKind kind)
        {
            #if ENABLE(JIT)
            return generatedJITCodeWithArityCheckFor(kind);
            #else
            return LLInt::CLoop::jsCodeEntryWithArityCheckFor(kind);
            #endif
        }

        static void* catchRoutineFor(HandlerInfo* handler, Instruction* catchPCForInterpreter)
        {
            #if ENABLE(JIT)
            UNUSED_PARAM(catchPCForInterpreter);
            return handler->nativeCode.executableAddress();
            #else
            UNUSED_PARAM(handler);
            return LLInt::CLoop::catchRoutineFor(catchPCForInterpreter);
            #endif
        }
#endif // ENABLE(JIT || ENABLE(LLINT_C_LOOP)

    protected:
        ExecutableBase* m_prev;
        ExecutableBase* m_next;

#if ENABLE(JIT)
        JITCode m_jitCodeForCall;
        JITCode m_jitCodeForConstruct;
        MacroAssemblerCodePtr m_jitCodeForCallWithArityCheck;
        MacroAssemblerCodePtr m_jitCodeForConstructWithArityCheck;
#endif
    };

    class NativeExecutable : public ExecutableBase {
        friend class JIT;
        friend class LLIntOffsetsExtractor;
    public:
        typedef ExecutableBase Base;

#if ENABLE(JIT)
        static NativeExecutable* create(VM& vm, MacroAssemblerCodeRef callThunk, NativeFunction function, MacroAssemblerCodeRef constructThunk, NativeFunction constructor, Intrinsic intrinsic)
        {
            NativeExecutable* executable;
            if (!callThunk) {
                executable = new (NotNull, allocateCell<NativeExecutable>(vm.heap)) NativeExecutable(vm, function, constructor);
                executable->finishCreation(vm, JITCode(), JITCode(), intrinsic);
            } else {
                executable = new (NotNull, allocateCell<NativeExecutable>(vm.heap)) NativeExecutable(vm, function, constructor);
                executable->finishCreation(vm, JITCode::HostFunction(callThunk), JITCode::HostFunction(constructThunk), intrinsic);
            }
            return executable;
        }
#endif

#if ENABLE(LLINT_C_LOOP)
        static NativeExecutable* create(VM& vm, NativeFunction function, NativeFunction constructor)
        {
            ASSERT(!vm.canUseJIT());
            NativeExecutable* executable = new (NotNull, allocateCell<NativeExecutable>(vm.heap)) NativeExecutable(vm, function, constructor);
            executable->finishCreation(vm);
            return executable;
        }
#endif

#if ENABLE(JIT)
        static void destroy(JSCell*);
#endif

        CodeBlockHash hashFor(CodeSpecializationKind) const;

        NativeFunction function() { return m_function; }
        NativeFunction constructor() { return m_constructor; }
        
        NativeFunction nativeFunctionFor(CodeSpecializationKind kind)
        {
            if (kind == CodeForCall)
                return function();
            ASSERT(kind == CodeForConstruct);
            return constructor();
        }
        
        static ptrdiff_t offsetOfNativeFunctionFor(CodeSpecializationKind kind)
        {
            if (kind == CodeForCall)
                return OBJECT_OFFSETOF(NativeExecutable, m_function);
            ASSERT(kind == CodeForConstruct);
            return OBJECT_OFFSETOF(NativeExecutable, m_constructor);
        }

        static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue proto) { return Structure::create(vm, globalObject, proto, TypeInfo(LeafType, StructureFlags), &s_info); }
        
        static const ClassInfo s_info;

        Intrinsic intrinsic() const;

    protected:
#if ENABLE(JIT)
        void finishCreation(VM& vm, JITCode callThunk, JITCode constructThunk, Intrinsic intrinsic)
        {
            Base::finishCreation(vm);
            m_jitCodeForCall = callThunk;
            m_jitCodeForConstruct = constructThunk;
            m_jitCodeForCallWithArityCheck = callThunk.addressForCall();
            m_jitCodeForConstructWithArityCheck = constructThunk.addressForCall();
            m_intrinsic = intrinsic;
        }
#endif

    private:
        NativeExecutable(VM& vm, NativeFunction function, NativeFunction constructor)
            : ExecutableBase(vm, vm.nativeExecutableStructure.get(), NUM_PARAMETERS_IS_HOST)
            , m_function(function)
            , m_constructor(constructor)
        {
        }

        NativeFunction m_function;
        NativeFunction m_constructor;
        
        Intrinsic m_intrinsic;
    };

    class ScriptExecutable : public ExecutableBase {
    public:
        typedef ExecutableBase Base;

        ScriptExecutable(Structure* structure, VM& vm, const SourceCode& source, bool isInStrictContext)
            : ExecutableBase(vm, structure, NUM_PARAMETERS_NOT_COMPILED)
            , m_source(source)
            , m_features(isInStrictContext ? StrictModeFeature : 0)
            , m_neverInline(false)
        {
        }

        ScriptExecutable(Structure* structure, ExecState* exec, const SourceCode& source, bool isInStrictContext)
            : ExecutableBase(exec->vm(), structure, NUM_PARAMETERS_NOT_COMPILED)
            , m_source(source)
            , m_features(isInStrictContext ? StrictModeFeature : 0)
            , m_neverInline(false)
        {
        }

#if ENABLE(JIT)
        static void destroy(JSCell*);
#endif
        
        CodeBlockHash hashFor(CodeSpecializationKind) const;

        const SourceCode& source() const { return m_source; }
        intptr_t sourceID() const { return m_source.providerID(); }
        const String& sourceURL() const { return m_source.provider()->url(); }
        int lineNo() const { return m_firstLine; }
        int lastLine() const { return m_lastLine; }
        unsigned startColumn() const { return m_startColumn; }

        bool usesEval() const { return m_features & EvalFeature; }
        bool usesArguments() const { return m_features & ArgumentsFeature; }
        bool needsActivation() const { return m_hasCapturedVariables || m_features & (EvalFeature | WithFeature | CatchFeature); }
        bool isStrictMode() const { return m_features & StrictModeFeature; }
        
        void setNeverInline(bool value) { m_neverInline = value; }
        bool neverInline() const { return m_neverInline; }
        bool isInliningCandidate() const { return !neverInline(); }

        void unlinkCalls();

        CodeFeatures features() const { return m_features; }
        
        static const ClassInfo s_info;

        void recordParse(CodeFeatures features, bool hasCapturedVariables, int firstLine, int lastLine, unsigned startColumn)
        {
            m_features = features;
            m_hasCapturedVariables = hasCapturedVariables;
            m_firstLine = firstLine;
            m_lastLine = lastLine;
            m_startColumn = startColumn;
        }

    protected:
        void finishCreation(VM& vm)
        {
            Base::finishCreation(vm);
            vm.heap.addCompiledCode(this); // Balanced by Heap::deleteUnmarkedCompiledCode().

#if ENABLE(CODEBLOCK_SAMPLING)
            if (SamplingTool* sampler = vm.interpreter->sampler())
                sampler->notifyOfScope(vm, this);
#endif
        }

        SourceCode m_source;
        CodeFeatures m_features;
        bool m_hasCapturedVariables;
        bool m_neverInline;
        int m_firstLine;
        int m_lastLine;
        unsigned m_startColumn;
    };

    class EvalExecutable : public ScriptExecutable {
        friend class LLIntOffsetsExtractor;
    public:
        typedef ScriptExecutable Base;

        static void destroy(JSCell*);

        JSObject* compile(ExecState* exec, JSScope* scope)
        {
            RELEASE_ASSERT(exec->vm().dynamicGlobalObject);
            JSObject* error = 0;
            if (!m_evalCodeBlock)
                error = compileInternal(exec, scope, JITCode::bottomTierJIT());
            ASSERT(!error == !!m_evalCodeBlock);
            return error;
        }
        
        JSObject* compileOptimized(ExecState*, JSScope*, unsigned bytecodeIndex);
        
#if ENABLE(JIT)
        void jettisonOptimizedCode(VM&);
        bool jitCompile(ExecState*);
#endif

        EvalCodeBlock& generatedBytecode()
        {
            ASSERT(m_evalCodeBlock);
            return *m_evalCodeBlock;
        }

        static EvalExecutable* create(ExecState* exec, PassRefPtr<CodeCache> cache, const SourceCode& source, bool isInStrictContext)
        {
            EvalExecutable* executable = new (NotNull, allocateCell<EvalExecutable>(*exec->heap())) EvalExecutable(exec, cache, source, isInStrictContext);
            executable->finishCreation(exec->vm());
            return executable;
        }

#if ENABLE(JIT)
        JITCode& generatedJITCode()
        {
            return generatedJITCodeForCall();
        }
#endif
        static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue proto)
        {
            return Structure::create(vm, globalObject, proto, TypeInfo(EvalExecutableType, StructureFlags), &s_info);
        }
        
        static const ClassInfo s_info;

        void unlinkCalls();

        void clearCode();

        ExecutableInfo executableInfo() const { return ExecutableInfo(needsActivation(), usesEval(), isStrictMode(), false); }

    private:
        static const unsigned StructureFlags = OverridesVisitChildren | ScriptExecutable::StructureFlags;
        EvalExecutable(ExecState*, PassRefPtr<CodeCache>, const SourceCode&, bool);

        JSObject* compileInternal(ExecState*, JSScope*, JITCode::JITType, unsigned bytecodeIndex = UINT_MAX);
        static void visitChildren(JSCell*, SlotVisitor&);

        OwnPtr<EvalCodeBlock> m_evalCodeBlock;
        WriteBarrier<UnlinkedEvalCodeBlock> m_unlinkedEvalCodeBlock;
        RefPtr<CodeCache> m_codeCache;
    };

    class ProgramExecutable : public ScriptExecutable {
        friend class LLIntOffsetsExtractor;
    public:
        typedef ScriptExecutable Base;

        static ProgramExecutable* create(ExecState* exec, const SourceCode& source)
        {
            ProgramExecutable* executable = new (NotNull, allocateCell<ProgramExecutable>(*exec->heap())) ProgramExecutable(exec, source);
            executable->finishCreation(exec->vm());
            return executable;
        }


        JSObject* initializeGlobalProperties(VM&, CallFrame*, JSScope*);

        static void destroy(JSCell*);

        JSObject* compile(ExecState* exec, JSScope* scope)
        {
            RELEASE_ASSERT(exec->vm().dynamicGlobalObject);
            JSObject* error = 0;
            if (!m_programCodeBlock)
                error = compileInternal(exec, scope, JITCode::bottomTierJIT());
            ASSERT(!error == !!m_programCodeBlock);
            return error;
        }

        JSObject* compileOptimized(ExecState*, JSScope*, unsigned bytecodeIndex);
        
#if ENABLE(JIT)
        void jettisonOptimizedCode(VM&);
        bool jitCompile(ExecState*);
#endif

        ProgramCodeBlock& generatedBytecode()
        {
            ASSERT(m_programCodeBlock);
            return *m_programCodeBlock;
        }

        JSObject* checkSyntax(ExecState*);

#if ENABLE(JIT)
        JITCode& generatedJITCode()
        {
            return generatedJITCodeForCall();
        }
#endif
        
        static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue proto)
        {
            return Structure::create(vm, globalObject, proto, TypeInfo(ProgramExecutableType, StructureFlags), &s_info);
        }
        
        static const ClassInfo s_info;
        
        void unlinkCalls();

        void clearCode();

        ExecutableInfo executableInfo() const { return ExecutableInfo(needsActivation(), usesEval(), isStrictMode(), false); }

    private:
        static const unsigned StructureFlags = OverridesVisitChildren | ScriptExecutable::StructureFlags;

        ProgramExecutable(ExecState*, const SourceCode&);

        enum ConstantMode { IsConstant, IsVariable };
        enum FunctionMode { IsFunctionToSpecialize, NotFunctionOrNotSpecializable };
        int addGlobalVar(JSGlobalObject*, const Identifier&, ConstantMode, FunctionMode);

        JSObject* compileInternal(ExecState*, JSScope*, JITCode::JITType, unsigned bytecodeIndex = UINT_MAX);
        static void visitChildren(JSCell*, SlotVisitor&);

        WriteBarrier<UnlinkedProgramCodeBlock> m_unlinkedProgramCodeBlock;
        OwnPtr<ProgramCodeBlock> m_programCodeBlock;
    };

    class FunctionExecutable : public ScriptExecutable {
        friend class JIT;
        friend class LLIntOffsetsExtractor;
    public:
        typedef ScriptExecutable Base;

        static FunctionExecutable* create(VM& vm, const SourceCode& source, UnlinkedFunctionExecutable* unlinkedExecutable, unsigned firstLine, unsigned lastLine, unsigned startColumn)
        {
            FunctionExecutable* executable = new (NotNull, allocateCell<FunctionExecutable>(vm.heap)) FunctionExecutable(vm, source, unlinkedExecutable, firstLine, lastLine, startColumn);
            executable->finishCreation(vm);
            return executable;
        }
        static FunctionExecutable* fromGlobalCode(const Identifier& name, ExecState*, Debugger*, const SourceCode&, JSObject** exception);

        static void destroy(JSCell*);
        
        UnlinkedFunctionExecutable* unlinkedExecutable()
        {
            return m_unlinkedExecutable.get();
        }

        // Returns either call or construct bytecode. This can be appropriate
        // for answering questions that that don't vary between call and construct --
        // for example, argumentsRegister().
        FunctionCodeBlock& generatedBytecode()
        {
            if (m_codeBlockForCall)
                return *m_codeBlockForCall;
            ASSERT(m_codeBlockForConstruct);
            return *m_codeBlockForConstruct;
        }
        
        PassOwnPtr<FunctionCodeBlock> produceCodeBlockFor(JSScope*, CodeSpecializationKind, JSObject*& exception);

        JSObject* compileForCall(ExecState* exec, JSScope* scope)
        {
            RELEASE_ASSERT(exec->vm().dynamicGlobalObject);
            JSObject* error = 0;
            if (!m_codeBlockForCall)
                error = compileForCallInternal(exec, scope, JITCode::bottomTierJIT());
            ASSERT(!error == !!m_codeBlockForCall);
            return error;
        }

        JSObject* compileOptimizedForCall(ExecState*, JSScope*, unsigned bytecodeIndex);
        
#if ENABLE(JIT)
        void jettisonOptimizedCodeForCall(VM&);
        bool jitCompileForCall(ExecState*);
#endif

        bool isGeneratedForCall() const
        {
            return m_codeBlockForCall;
        }

        FunctionCodeBlock& generatedBytecodeForCall()
        {
            ASSERT(m_codeBlockForCall);
            return *m_codeBlockForCall;
        }

        JSObject* compileForConstruct(ExecState* exec, JSScope* scope)
        {
            RELEASE_ASSERT(exec->vm().dynamicGlobalObject);
            JSObject* error = 0;
            if (!m_codeBlockForConstruct)
                error = compileForConstructInternal(exec, scope, JITCode::bottomTierJIT());
            ASSERT(!error == !!m_codeBlockForConstruct);
            return error;
        }

        JSObject* compileOptimizedForConstruct(ExecState*, JSScope*, unsigned bytecodeIndex);
        
#if ENABLE(JIT)
        void jettisonOptimizedCodeForConstruct(VM&);
        bool jitCompileForConstruct(ExecState*);
#endif

        bool isGeneratedForConstruct() const
        {
            return m_codeBlockForConstruct;
        }

        FunctionCodeBlock& generatedBytecodeForConstruct()
        {
            ASSERT(m_codeBlockForConstruct);
            return *m_codeBlockForConstruct;
        }
        
        JSObject* compileFor(ExecState* exec, JSScope* scope, CodeSpecializationKind kind)
        {
            ASSERT(exec->callee());
            ASSERT(exec->callee()->inherits(&JSFunction::s_info));
            ASSERT(jsCast<JSFunction*>(exec->callee())->jsExecutable() == this);

            if (kind == CodeForCall)
                return compileForCall(exec, scope);
            ASSERT(kind == CodeForConstruct);
            return compileForConstruct(exec, scope);
        }
        
        JSObject* compileOptimizedFor(ExecState* exec, JSScope* scope, unsigned bytecodeIndex, CodeSpecializationKind kind)
        {
            ASSERT(exec->callee());
            ASSERT(exec->callee()->inherits(&JSFunction::s_info));
            ASSERT(jsCast<JSFunction*>(exec->callee())->jsExecutable() == this);
            
            if (kind == CodeForCall)
                return compileOptimizedForCall(exec, scope, bytecodeIndex);
            ASSERT(kind == CodeForConstruct);
            return compileOptimizedForConstruct(exec, scope, bytecodeIndex);
        }
        
#if ENABLE(JIT)
        void jettisonOptimizedCodeFor(VM& vm, CodeSpecializationKind kind)
        {
            if (kind == CodeForCall) 
                jettisonOptimizedCodeForCall(vm);
            else {
                ASSERT(kind == CodeForConstruct);
                jettisonOptimizedCodeForConstruct(vm);
            }
        }
        
        bool jitCompileFor(ExecState* exec, CodeSpecializationKind kind)
        {
            if (kind == CodeForCall)
                return jitCompileForCall(exec);
            ASSERT(kind == CodeForConstruct);
            return jitCompileForConstruct(exec);
        }
#endif
        
        bool isGeneratedFor(CodeSpecializationKind kind)
        {
            if (kind == CodeForCall)
                return isGeneratedForCall();
            ASSERT(kind == CodeForConstruct);
            return isGeneratedForConstruct();
        }
        
        FunctionCodeBlock& generatedBytecodeFor(CodeSpecializationKind kind)
        {
            if (kind == CodeForCall)
                return generatedBytecodeForCall();
            ASSERT(kind == CodeForConstruct);
            return generatedBytecodeForConstruct();
        }

        FunctionCodeBlock* baselineCodeBlockFor(CodeSpecializationKind);
        
        FunctionCodeBlock* profiledCodeBlockFor(CodeSpecializationKind kind)
        {
            return baselineCodeBlockFor(kind);
        }
        
        const Identifier& name() { return m_unlinkedExecutable->name(); }
        const Identifier& inferredName() { return m_unlinkedExecutable->inferredName(); }
        JSString* nameValue() const { return m_unlinkedExecutable->nameValue(); }
        size_t parameterCount() const { return m_unlinkedExecutable->parameterCount(); } // Excluding 'this'!
        String paramString() const;
        SharedSymbolTable* symbolTable(CodeSpecializationKind kind) const { return m_unlinkedExecutable->symbolTable(kind); }

        void clearCodeIfNotCompiling();
        void clearUnlinkedCodeForRecompilationIfNotCompiling();
        static void visitChildren(JSCell*, SlotVisitor&);
        static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue proto)
        {
            return Structure::create(vm, globalObject, proto, TypeInfo(FunctionExecutableType, StructureFlags), &s_info);
        }
        
        static const ClassInfo s_info;
        
        void unlinkCalls();

        void clearCode();

    private:
        FunctionExecutable(VM&, const SourceCode&, UnlinkedFunctionExecutable*, unsigned firstLine, unsigned lastLine, unsigned startColumn);

        JSObject* compileForCallInternal(ExecState*, JSScope*, JITCode::JITType, unsigned bytecodeIndex = UINT_MAX);
        JSObject* compileForConstructInternal(ExecState*, JSScope*, JITCode::JITType, unsigned bytecodeIndex = UINT_MAX);
        
        OwnPtr<FunctionCodeBlock>& codeBlockFor(CodeSpecializationKind kind)
        {
            if (kind == CodeForCall)
                return m_codeBlockForCall;
            ASSERT(kind == CodeForConstruct);
            return m_codeBlockForConstruct;
        }
 
        bool isCompiling()
        {
#if ENABLE(JIT)
            if (!m_jitCodeForCall && m_codeBlockForCall)
                return true;
            if (!m_jitCodeForConstruct && m_codeBlockForConstruct)
                return true;
#endif
            return false;
        }

        static const unsigned StructureFlags = OverridesVisitChildren | ScriptExecutable::StructureFlags;
        WriteBarrier<UnlinkedFunctionExecutable> m_unlinkedExecutable;
        OwnPtr<FunctionCodeBlock> m_codeBlockForCall;
        OwnPtr<FunctionCodeBlock> m_codeBlockForConstruct;
    };

    inline bool isHostFunction(JSValue value, NativeFunction nativeFunction)
    {
        JSFunction* function = jsCast<JSFunction*>(getJSFunction(value));
        if (!function || !function->isHostFunction())
            return false;
        return function->nativeFunction() == nativeFunction;
    }

    inline void ExecutableBase::clearCodeVirtual(ExecutableBase* executable)
    {
        switch (executable->structure()->typeInfo().type()) {
        case EvalExecutableType:
            return jsCast<EvalExecutable*>(executable)->clearCode();
        case ProgramExecutableType:
            return jsCast<ProgramExecutable*>(executable)->clearCode();
        case FunctionExecutableType:
            return jsCast<FunctionExecutable*>(executable)->clearCode();
        default:
            return jsCast<NativeExecutable*>(executable)->clearCode();
        }
    }

    inline void ScriptExecutable::unlinkCalls()
    {
        switch (structure()->typeInfo().type()) {
        case EvalExecutableType:
            return jsCast<EvalExecutable*>(this)->unlinkCalls();
        case ProgramExecutableType:
            return jsCast<ProgramExecutable*>(this)->unlinkCalls();
        case FunctionExecutableType:
            return jsCast<FunctionExecutable*>(this)->unlinkCalls();
        default:
            RELEASE_ASSERT_NOT_REACHED();
        }
    }

}

#include "JSFunctionInlines.h"

#endif
