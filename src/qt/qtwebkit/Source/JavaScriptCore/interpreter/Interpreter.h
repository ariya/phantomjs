/*
 * Copyright (C) 2008, 2013 Apple Inc. All rights reserved.
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
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

#ifndef Interpreter_h
#define Interpreter_h

#include "ArgList.h"
#include "JSCJSValue.h"
#include "JSCell.h"
#include "JSFunction.h"
#include "JSObject.h"
#include "JSStack.h"
#include "LLIntData.h"
#include "Opcode.h"

#include <wtf/HashMap.h>
#include <wtf/text/StringBuilder.h>

namespace JSC {

    class CodeBlock;
    class EvalExecutable;
    class ExecutableBase;
    class FunctionExecutable;
    class VM;
    class JSGlobalObject;
    class LLIntOffsetsExtractor;
    class ProgramExecutable;
    class Register;
    class JSScope;
    class SamplingTool;
    struct CallFrameClosure;
    struct HandlerInfo;
    struct Instruction;
    
    enum DebugHookID {
        WillExecuteProgram,
        DidExecuteProgram,
        DidEnterCallFrame,
        DidReachBreakpoint,
        WillLeaveCallFrame,
        WillExecuteStatement
    };

    enum StackFrameCodeType {
        StackFrameGlobalCode,
        StackFrameEvalCode,
        StackFrameFunctionCode,
        StackFrameNativeCode
    };

    struct StackFrame {
        Strong<JSObject> callee;
        StackFrameCodeType codeType;
        Strong<ExecutableBase> executable;
        Strong<UnlinkedCodeBlock> codeBlock;
        RefPtr<SourceProvider> code;
        int lineOffset;
        unsigned firstLineColumnOffset;
        unsigned characterOffset;
        unsigned bytecodeOffset;
        String sourceURL;
        JS_EXPORT_PRIVATE String toString(CallFrame*);
        String friendlySourceURL() const
        {
            String traceLine;

            switch (codeType) {
            case StackFrameEvalCode:
            case StackFrameFunctionCode:
            case StackFrameGlobalCode:
                if (!sourceURL.isEmpty())
                    traceLine = sourceURL.impl();
                break;
            case StackFrameNativeCode:
                traceLine = "[native code]";
                break;
            }
            return traceLine.isNull() ? emptyString() : traceLine;
        }
        String friendlyFunctionName(CallFrame* callFrame) const
        {
            String traceLine;
            JSObject* stackFrameCallee = callee.get();

            switch (codeType) {
            case StackFrameEvalCode:
                traceLine = "eval code";
                break;
            case StackFrameNativeCode:
                if (callee)
                    traceLine = getCalculatedDisplayName(callFrame, stackFrameCallee).impl();
                break;
            case StackFrameFunctionCode:
                traceLine = getCalculatedDisplayName(callFrame, stackFrameCallee).impl();
                break;
            case StackFrameGlobalCode:
                traceLine = "global code";
                break;
            }
            return traceLine.isNull() ? emptyString() : traceLine;
        }
        JS_EXPORT_PRIVATE void computeLineAndColumn(unsigned& line, unsigned& column);

    private:
        void expressionInfo(int& divot, int& startOffset, int& endOffset, unsigned& line, unsigned& column);
    };

    class TopCallFrameSetter {
    public:
        TopCallFrameSetter(VM& global, CallFrame* callFrame)
            : vm(global)
            , oldCallFrame(global.topCallFrame) 
        {
            global.topCallFrame = callFrame;
        }
        
        ~TopCallFrameSetter() 
        {
            vm.topCallFrame = oldCallFrame;
        }
    private:
        VM& vm;
        CallFrame* oldCallFrame;
    };
    
    class NativeCallFrameTracer {
    public:
        ALWAYS_INLINE NativeCallFrameTracer(VM* global, CallFrame* callFrame)
        {
            ASSERT(global);
            ASSERT(callFrame);
            global->topCallFrame = callFrame;
        }
    };

    class Interpreter {
        WTF_MAKE_FAST_ALLOCATED;
        friend class CachedCall;
        friend class LLIntOffsetsExtractor;
        friend class JIT;

    public:
        class ErrorHandlingMode {
        public:
            JS_EXPORT_PRIVATE ErrorHandlingMode(ExecState*);
            JS_EXPORT_PRIVATE ~ErrorHandlingMode();
        private:
            Interpreter& m_interpreter;
        };

        Interpreter(VM &);
        ~Interpreter();
        
        void initialize(bool canUseJIT);

        JSStack& stack() { return m_stack; }
        
        Opcode getOpcode(OpcodeID id)
        {
            ASSERT(m_initialized);
#if ENABLE(COMPUTED_GOTO_OPCODES)
            return m_opcodeTable[id];
#else
            return id;
#endif
        }

        OpcodeID getOpcodeID(Opcode opcode)
        {
            ASSERT(m_initialized);
#if ENABLE(COMPUTED_GOTO_OPCODES) && ENABLE(LLINT)
            ASSERT(isOpcode(opcode));
            return m_opcodeIDTable.get(opcode);
#else
            return opcode;
#endif
        }
        
        bool isOpcode(Opcode);

        JSValue execute(ProgramExecutable*, CallFrame*, JSObject* thisObj);
        JSValue executeCall(CallFrame*, JSObject* function, CallType, const CallData&, JSValue thisValue, const ArgList&);
        JSObject* executeConstruct(CallFrame*, JSObject* function, ConstructType, const ConstructData&, const ArgList&);
        JSValue execute(EvalExecutable*, CallFrame*, JSValue thisValue, JSScope*);

        JSValue retrieveArgumentsFromVMCode(CallFrame*, JSFunction*) const;
        JSValue retrieveCallerFromVMCode(CallFrame*, JSFunction*) const;
        JS_EXPORT_PRIVATE void retrieveLastCaller(CallFrame*, int& lineNumber, intptr_t& sourceID, String& sourceURL, JSValue& function) const;
        
        void getArgumentsData(CallFrame*, JSFunction*&, ptrdiff_t& firstParameterIndex, Register*& argv, int& argc);
        
        SamplingTool* sampler() { return m_sampler.get(); }

        bool isInErrorHandlingMode() { return m_errorHandlingModeReentry; }

        NEVER_INLINE HandlerInfo* throwException(CallFrame*&, JSValue&, unsigned bytecodeOffset);
        NEVER_INLINE void debug(CallFrame*, DebugHookID, int firstLine, int lastLine, int column);
        static const String getTraceLine(CallFrame*, StackFrameCodeType, const String&, int);
        JS_EXPORT_PRIVATE static void getStackTrace(VM*, Vector<StackFrame>& results, size_t maxStackSize = std::numeric_limits<size_t>::max());
        static void addStackTraceIfNecessary(CallFrame*, JSValue error);
        JSString* stackTraceAsString(ExecState* exec, Vector<StackFrame> stackTrace);

        void dumpSampleData(ExecState* exec);
        void startSampling();
        void stopSampling();

        JS_EXPORT_PRIVATE void dumpCallFrame(CallFrame*);

    private:
        enum ExecutionFlag { Normal, InitializeAndReturn };

        CallFrameClosure prepareForRepeatCall(FunctionExecutable*, CallFrame*, JSFunction*, int argumentCountIncludingThis, JSScope*);
        void endRepeatCall(CallFrameClosure&);
        JSValue execute(CallFrameClosure&);

        NEVER_INLINE bool unwindCallFrame(CallFrame*&, JSValue, unsigned& bytecodeOffset, CodeBlock*&);

        static CallFrame* findFunctionCallFrameFromVMCode(CallFrame*, JSFunction*);

        void dumpRegisters(CallFrame*);
        
        bool isCallBytecode(Opcode opcode) { return opcode == getOpcode(op_call) || opcode == getOpcode(op_construct) || opcode == getOpcode(op_call_eval); }

        void enableSampler();
        int m_sampleEntryDepth;
        OwnPtr<SamplingTool> m_sampler;

        JSStack m_stack;
        int m_errorHandlingModeReentry;
        
#if ENABLE(COMPUTED_GOTO_OPCODES) && ENABLE(LLINT)
        Opcode* m_opcodeTable; // Maps OpcodeID => Opcode for compiling
        HashMap<Opcode, OpcodeID> m_opcodeIDTable; // Maps Opcode => OpcodeID for decompiling
#endif

#if !ASSERT_DISABLED
        bool m_initialized;
#endif
    };

    // This value must not be an object that would require this conversion (WebCore's global object).
    inline bool isValidThisObject(JSValue thisValue, ExecState* exec)
    {
        return !thisValue.isObject() || thisValue.toThisObject(exec) == thisValue;
    }

    JSValue eval(CallFrame*);
    CallFrame* loadVarargs(CallFrame*, JSStack*, JSValue thisValue, JSValue arguments, int firstFreeRegister);

} // namespace JSC

#endif // Interpreter_h
