/*
 * Copyright (C) 2008, 2009, 2010, 2012, 2013 Apple Inc. All rights reserved.
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
#include "Interpreter.h"

#include "Arguments.h"
#include "BatchedTransitionOptimizer.h"
#include "CallFrame.h"
#include "CallFrameClosure.h"
#include "CodeBlock.h"
#include "Heap.h"
#include "Debugger.h"
#include "DebuggerCallFrame.h"
#include "ErrorInstance.h"
#include "EvalCodeCache.h"
#include "ExceptionHelpers.h"
#include "GetterSetter.h"
#include "JSActivation.h"
#include "JSArray.h"
#include "JSBoundFunction.h"
#include "JSNameScope.h"
#include "JSNotAnObject.h"
#include "JSPropertyNameIterator.h"
#include "JSStackInlines.h"
#include "JSString.h"
#include "JSWithScope.h"
#include "LLIntCLoop.h"
#include "LegacyProfiler.h"
#include "LiteralParser.h"
#include "NameInstance.h"
#include "ObjectPrototype.h"
#include "Operations.h"
#include "Parser.h"
#include "RegExpObject.h"
#include "RegExpPrototype.h"
#include "Register.h"
#include "SamplingTool.h"
#include "StrictEvalActivation.h"
#include "StrongInlines.h"
#include "VMStackBounds.h"
#include <limits.h>
#include <stdio.h>
#include <wtf/StackStats.h>
#include <wtf/StringPrintStream.h>
#include <wtf/Threading.h>
#include <wtf/WTFThreadData.h>
#include <wtf/text/StringBuilder.h>

#if ENABLE(JIT)
#include "JIT.h"
#endif

#define WTF_USE_GCC_COMPUTED_GOTO_WORKAROUND (ENABLE(LLINT) && !defined(__llvm__))

using namespace std;

namespace JSC {

Interpreter::ErrorHandlingMode::ErrorHandlingMode(ExecState *exec)
    : m_interpreter(*exec->interpreter())
{
    if (!m_interpreter.m_errorHandlingModeReentry)
        m_interpreter.stack().enableErrorStackReserve();
    m_interpreter.m_errorHandlingModeReentry++;
}

Interpreter::ErrorHandlingMode::~ErrorHandlingMode()
{
    m_interpreter.m_errorHandlingModeReentry--;
    ASSERT(m_interpreter.m_errorHandlingModeReentry >= 0);
    if (!m_interpreter.m_errorHandlingModeReentry)
        m_interpreter.stack().disableErrorStackReserve();
}

static CallFrame* getCallerInfo(VM*, CallFrame*, unsigned& bytecodeOffset, CodeBlock*& callerOut);

// Returns the depth of the scope chain within a given call frame.
static int depth(CodeBlock* codeBlock, JSScope* sc)
{
    if (!codeBlock->needsFullScopeChain())
        return 0;
    return sc->localDepth();
}

JSValue eval(CallFrame* callFrame)
{
    if (!callFrame->argumentCount())
        return jsUndefined();

    JSValue program = callFrame->argument(0);
    if (!program.isString())
        return program;
    
    TopCallFrameSetter topCallFrame(callFrame->vm(), callFrame);
    String programSource = asString(program)->value(callFrame);
    if (callFrame->hadException())
        return JSValue();
    
    CallFrame* callerFrame = callFrame->callerFrame();
    CodeBlock* callerCodeBlock = callerFrame->codeBlock();
    JSScope* callerScopeChain = callerFrame->scope();
    EvalExecutable* eval = callerCodeBlock->evalCodeCache().tryGet(callerCodeBlock->isStrictMode(), programSource, callerScopeChain);

    if (!eval) {
        if (!callerCodeBlock->isStrictMode()) {
            // FIXME: We can use the preparser in strict mode, we just need additional logic
            // to prevent duplicates.
            if (programSource.is8Bit()) {
                LiteralParser<LChar> preparser(callFrame, programSource.characters8(), programSource.length(), NonStrictJSON);
                if (JSValue parsedObject = preparser.tryLiteralParse())
                    return parsedObject;
            } else {
                LiteralParser<UChar> preparser(callFrame, programSource.characters16(), programSource.length(), NonStrictJSON);
                if (JSValue parsedObject = preparser.tryLiteralParse())
                    return parsedObject;                
            }
        }
        
        // If the literal parser bailed, it should not have thrown exceptions.
        ASSERT(!callFrame->vm().exception);

        JSValue exceptionValue;
        eval = callerCodeBlock->evalCodeCache().getSlow(callFrame, callerCodeBlock->unlinkedCodeBlock()->codeCacheForEval().get(), callerCodeBlock->ownerExecutable(), callerCodeBlock->isStrictMode(), programSource, callerScopeChain, exceptionValue);
        
        ASSERT(!eval == exceptionValue);
        if (UNLIKELY(!eval))
            return throwError(callFrame, exceptionValue);
    }

    JSValue thisValue = callerFrame->thisValue();
    ASSERT(isValidThisObject(thisValue, callFrame));
    Interpreter* interpreter = callFrame->vm().interpreter;
    return interpreter->execute(eval, callFrame, thisValue, callerScopeChain);
}

CallFrame* loadVarargs(CallFrame* callFrame, JSStack* stack, JSValue thisValue, JSValue arguments, int firstFreeRegister)
{
    if (!arguments) { // f.apply(x, arguments), with arguments unmodified.
        unsigned argumentCountIncludingThis = callFrame->argumentCountIncludingThis();
        CallFrame* newCallFrame = CallFrame::create(callFrame->registers() + firstFreeRegister + argumentCountIncludingThis + JSStack::CallFrameHeaderSize);
        if (argumentCountIncludingThis > Arguments::MaxArguments + 1 || !stack->grow(newCallFrame->registers())) {
            callFrame->vm().exception = createStackOverflowError(callFrame);
            return 0;
        }

        newCallFrame->setArgumentCountIncludingThis(argumentCountIncludingThis);
        newCallFrame->setThisValue(thisValue);
        for (size_t i = 0; i < callFrame->argumentCount(); ++i)
            newCallFrame->setArgument(i, callFrame->argumentAfterCapture(i));
        return newCallFrame;
    }

    if (arguments.isUndefinedOrNull()) {
        CallFrame* newCallFrame = CallFrame::create(callFrame->registers() + firstFreeRegister + 1 + JSStack::CallFrameHeaderSize);
        if (!stack->grow(newCallFrame->registers())) {
            callFrame->vm().exception = createStackOverflowError(callFrame);
            return 0;
        }
        newCallFrame->setArgumentCountIncludingThis(1);
        newCallFrame->setThisValue(thisValue);
        return newCallFrame;
    }

    if (!arguments.isObject()) {
        callFrame->vm().exception = createInvalidParameterError(callFrame, "Function.prototype.apply", arguments);
        return 0;
    }

    if (asObject(arguments)->classInfo() == &Arguments::s_info) {
        Arguments* argsObject = asArguments(arguments);
        unsigned argCount = argsObject->length(callFrame);
        CallFrame* newCallFrame = CallFrame::create(callFrame->registers() + firstFreeRegister + CallFrame::offsetFor(argCount + 1));
        if (argCount > Arguments::MaxArguments || !stack->grow(newCallFrame->registers())) {
            callFrame->vm().exception = createStackOverflowError(callFrame);
            return 0;
        }
        newCallFrame->setArgumentCountIncludingThis(argCount + 1);
        newCallFrame->setThisValue(thisValue);
        argsObject->copyToArguments(callFrame, newCallFrame, argCount);
        return newCallFrame;
    }

    if (isJSArray(arguments)) {
        JSArray* array = asArray(arguments);
        unsigned argCount = array->length();
        CallFrame* newCallFrame = CallFrame::create(callFrame->registers() + firstFreeRegister + CallFrame::offsetFor(argCount + 1));
        if (argCount > Arguments::MaxArguments || !stack->grow(newCallFrame->registers())) {
            callFrame->vm().exception = createStackOverflowError(callFrame);
            return 0;
        }
        newCallFrame->setArgumentCountIncludingThis(argCount + 1);
        newCallFrame->setThisValue(thisValue);
        array->copyToArguments(callFrame, newCallFrame, argCount);
        return newCallFrame;
    }

    JSObject* argObject = asObject(arguments);
    unsigned argCount = argObject->get(callFrame, callFrame->propertyNames().length).toUInt32(callFrame);
    CallFrame* newCallFrame = CallFrame::create(callFrame->registers() + firstFreeRegister + CallFrame::offsetFor(argCount + 1));
    if (argCount > Arguments::MaxArguments || !stack->grow(newCallFrame->registers())) {
        callFrame->vm().exception = createStackOverflowError(callFrame);
        return 0;
    }
    newCallFrame->setArgumentCountIncludingThis(argCount + 1);
    newCallFrame->setThisValue(thisValue);
    for (size_t i = 0; i < argCount; ++i) {
        newCallFrame->setArgument(i, asObject(arguments)->get(callFrame, i));
        if (UNLIKELY(callFrame->vm().exception))
            return 0;
    }
    return newCallFrame;
}

Interpreter::Interpreter(VM& vm)
    : m_sampleEntryDepth(0)
    , m_stack(vm)
    , m_errorHandlingModeReentry(0)
#if !ASSERT_DISABLED
    , m_initialized(false)
#endif
{
}

Interpreter::~Interpreter()
{
}

void Interpreter::initialize(bool canUseJIT)
{
    UNUSED_PARAM(canUseJIT);

#if ENABLE(COMPUTED_GOTO_OPCODES) && ENABLE(LLINT)
    m_opcodeTable = LLInt::opcodeMap();
    for (int i = 0; i < numOpcodeIDs; ++i)
        m_opcodeIDTable.add(m_opcodeTable[i], static_cast<OpcodeID>(i));
#endif

#if !ASSERT_DISABLED
    m_initialized = true;
#endif

#if ENABLE(OPCODE_SAMPLING)
    enableSampler();
#endif
}

#ifdef NDEBUG

void Interpreter::dumpCallFrame(CallFrame*)
{
}

#else

void Interpreter::dumpCallFrame(CallFrame* callFrame)
{
    callFrame->codeBlock()->dumpBytecode();
    dumpRegisters(callFrame);
}

void Interpreter::dumpRegisters(CallFrame* callFrame)
{
    dataLogF("Register frame: \n\n");
    dataLogF("-----------------------------------------------------------------------------\n");
    dataLogF("            use            |   address  |                value               \n");
    dataLogF("-----------------------------------------------------------------------------\n");

    CodeBlock* codeBlock = callFrame->codeBlock();
    const Register* it;
    const Register* end;

    it = callFrame->registers() - JSStack::CallFrameHeaderSize - callFrame->argumentCountIncludingThis();
    end = callFrame->registers() - JSStack::CallFrameHeaderSize;
    while (it < end) {
        JSValue v = it->jsValue();
        int registerNumber = it - callFrame->registers();
        String name = codeBlock->nameForRegister(registerNumber);
        dataLogF("[r% 3d %14s]      | %10p | %-16s 0x%lld \n", registerNumber, name.ascii().data(), it, toCString(v).data(), (long long)JSValue::encode(v));
        it++;
    }
    
    dataLogF("-----------------------------------------------------------------------------\n");
    dataLogF("[ArgumentCount]            | %10p | %lu \n", it, (unsigned long) callFrame->argumentCount());
    ++it;
    dataLogF("[CallerFrame]              | %10p | %p \n", it, callFrame->callerFrame());
    ++it;
    dataLogF("[Callee]                   | %10p | %p \n", it, callFrame->callee());
    ++it;
    dataLogF("[ScopeChain]               | %10p | %p \n", it, callFrame->scope());
    ++it;
#if ENABLE(JIT)
    AbstractPC pc = callFrame->abstractReturnPC(callFrame->vm());
    if (pc.hasJITReturnAddress())
        dataLogF("[ReturnJITPC]              | %10p | %p \n", it, pc.jitReturnAddress().value());
#endif
    unsigned bytecodeOffset = 0;
    int line = 0;
    CodeBlock* callerCodeBlock = 0;
    getCallerInfo(&callFrame->vm(), callFrame, bytecodeOffset, callerCodeBlock);
    line = callerCodeBlock->lineNumberForBytecodeOffset(bytecodeOffset);
    dataLogF("[ReturnVPC]                | %10p | %d (line %d)\n", it, bytecodeOffset, line);
    ++it;
    dataLogF("[CodeBlock]                | %10p | %p \n", it, callFrame->codeBlock());
    ++it;
    dataLogF("-----------------------------------------------------------------------------\n");

    int registerCount = 0;

    end = it + codeBlock->m_numVars;
    if (it != end) {
        do {
            JSValue v = it->jsValue();
            int registerNumber = it - callFrame->registers();
            String name = codeBlock->nameForRegister(registerNumber);
            dataLogF("[r% 3d %14s]      | %10p | %-16s 0x%lld \n", registerNumber, name.ascii().data(), it, toCString(v).data(), (long long)JSValue::encode(v));
            ++it;
            ++registerCount;
        } while (it != end);
    }
    dataLogF("-----------------------------------------------------------------------------\n");

    end = it + codeBlock->m_numCalleeRegisters - codeBlock->m_numVars;
    if (it != end) {
        do {
            JSValue v = (*it).jsValue();
            dataLogF("[r% 3d]                     | %10p | %-16s 0x%lld \n", registerCount, it, toCString(v).data(), (long long)JSValue::encode(v));
            ++it;
            ++registerCount;
        } while (it != end);
    }
    dataLogF("-----------------------------------------------------------------------------\n");
}

#endif

bool Interpreter::isOpcode(Opcode opcode)
{
#if ENABLE(COMPUTED_GOTO_OPCODES)
#if !ENABLE(LLINT)
    return static_cast<OpcodeID>(bitwise_cast<uintptr_t>(opcode)) <= op_end;
#else
    return opcode != HashTraits<Opcode>::emptyValue()
        && !HashTraits<Opcode>::isDeletedValue(opcode)
        && m_opcodeIDTable.contains(opcode);
#endif
#else
    return opcode >= 0 && opcode <= op_end;
#endif
}

NEVER_INLINE bool Interpreter::unwindCallFrame(CallFrame*& callFrame, JSValue exceptionValue, unsigned& bytecodeOffset, CodeBlock*& codeBlock)
{
    CodeBlock* oldCodeBlock = codeBlock;
    JSScope* scope = callFrame->scope();

    if (Debugger* debugger = callFrame->dynamicGlobalObject()->debugger()) {
        DebuggerCallFrame debuggerCallFrame(callFrame, exceptionValue);
        if (callFrame->callee())
            debugger->returnEvent(debuggerCallFrame, codeBlock->ownerExecutable()->sourceID(), codeBlock->ownerExecutable()->lastLine(), 0);
        else
            debugger->didExecuteProgram(debuggerCallFrame, codeBlock->ownerExecutable()->sourceID(), codeBlock->ownerExecutable()->lastLine(), 0);
    }

    JSValue activation;
    if (oldCodeBlock->codeType() == FunctionCode && oldCodeBlock->needsActivation()) {
        activation = callFrame->uncheckedR(oldCodeBlock->activationRegister()).jsValue();
        if (activation)
            jsCast<JSActivation*>(activation)->tearOff(*scope->vm());
    }

    if (oldCodeBlock->codeType() == FunctionCode && oldCodeBlock->usesArguments()) {
        if (JSValue arguments = callFrame->uncheckedR(unmodifiedArgumentsRegister(oldCodeBlock->argumentsRegister())).jsValue()) {
            if (activation)
                jsCast<Arguments*>(arguments)->didTearOffActivation(callFrame, jsCast<JSActivation*>(activation));
            else
                jsCast<Arguments*>(arguments)->tearOff(callFrame);
        }
    }

    CallFrame* callerFrame = callFrame->callerFrame();
    callFrame->vm().topCallFrame = callerFrame;
    if (callerFrame->hasHostCallFrameFlag())
        return false;
    callFrame = getCallerInfo(&callFrame->vm(), callFrame, bytecodeOffset, codeBlock);
    return true;
}

static void appendSourceToError(CallFrame* callFrame, ErrorInstance* exception, unsigned bytecodeOffset)
{
    exception->clearAppendSourceToMessage();

    if (!callFrame->codeBlock()->hasExpressionInfo())
        return;

    int startOffset = 0;
    int endOffset = 0;
    int divotPoint = 0;
    unsigned line = 0;
    unsigned column = 0;

    CodeBlock* codeBlock = callFrame->codeBlock();
    codeBlock->expressionRangeForBytecodeOffset(bytecodeOffset, divotPoint, startOffset, endOffset, line, column);

    int expressionStart = divotPoint - startOffset;
    int expressionStop = divotPoint + endOffset;

    const String& sourceString = codeBlock->source()->source();
    if (!expressionStop || expressionStart > static_cast<int>(sourceString.length()))
        return;

    VM* vm = &callFrame->vm();
    JSValue jsMessage = exception->getDirect(*vm, vm->propertyNames->message);
    if (!jsMessage || !jsMessage.isString())
        return;

    String message = asString(jsMessage)->value(callFrame);

    if (expressionStart < expressionStop)
        message =  makeString(message, " (evaluating '", codeBlock->source()->getRange(expressionStart, expressionStop), "')");
    else {
        // No range information, so give a few characters of context
        const StringImpl* data = sourceString.impl();
        int dataLength = sourceString.length();
        int start = expressionStart;
        int stop = expressionStart;
        // Get up to 20 characters of context to the left and right of the divot, clamping to the line.
        // then strip whitespace.
        while (start > 0 && (expressionStart - start < 20) && (*data)[start - 1] != '\n')
            start--;
        while (start < (expressionStart - 1) && isStrWhiteSpace((*data)[start]))
            start++;
        while (stop < dataLength && (stop - expressionStart < 20) && (*data)[stop] != '\n')
            stop++;
        while (stop > expressionStart && isStrWhiteSpace((*data)[stop - 1]))
            stop--;
        message = makeString(message, " (near '...", codeBlock->source()->getRange(start, stop), "...')");
    }

    exception->putDirect(*vm, vm->propertyNames->message, jsString(vm, message));
}

static unsigned getBytecodeOffsetForCallFrame(CallFrame* callFrame)
{
    callFrame = callFrame->removeHostCallFrameFlag();
    CodeBlock* codeBlock = callFrame->codeBlock();
    if (!codeBlock)
        return 0;
#if ENABLE(DFG_JIT)
    if (codeBlock->getJITType() == JITCode::DFGJIT)
        return codeBlock->codeOrigin(callFrame->codeOriginIndexForDFG()).bytecodeIndex;
#endif
    return callFrame->bytecodeOffsetForNonDFGCode();
}

static CallFrame* getCallerInfo(VM* vm, CallFrame* callFrame, unsigned& bytecodeOffset, CodeBlock*& caller)
{
    ASSERT_UNUSED(vm, vm);
    bytecodeOffset = 0;
    ASSERT(!callFrame->hasHostCallFrameFlag());
    CallFrame* trueCallerFrame = callFrame->trueCallerFrame();
    bool wasCalledByHost = callFrame->callerFrame()->hasHostCallFrameFlag();
    ASSERT(!trueCallerFrame->hasHostCallFrameFlag());

    if (trueCallerFrame == CallFrame::noCaller() || !trueCallerFrame || !trueCallerFrame->codeBlock()) {
        caller = 0;
        return trueCallerFrame;
    }
    
    CodeBlock* callerCodeBlock = trueCallerFrame->codeBlock();
    
    if (!callFrame->hasReturnPC())
        wasCalledByHost = true;

    if (wasCalledByHost) {
#if ENABLE(DFG_JIT)
        if (callerCodeBlock && callerCodeBlock->getJITType() == JITCode::DFGJIT) {
            unsigned codeOriginIndex = callFrame->callerFrame()->removeHostCallFrameFlag()->codeOriginIndexForDFG();
            CodeOrigin origin = callerCodeBlock->codeOrigin(codeOriginIndex);
            bytecodeOffset = origin.bytecodeIndex;
            if (InlineCallFrame* inlineCallFrame = origin.inlineCallFrame)
                callerCodeBlock = inlineCallFrame->baselineCodeBlock();
        } else
#endif
            bytecodeOffset = trueCallerFrame->bytecodeOffsetForNonDFGCode();
    } else {
#if ENABLE(DFG_JIT)
        if (callFrame->isInlineCallFrame()) {
            InlineCallFrame* icf = callFrame->inlineCallFrame();
            bytecodeOffset = icf->caller.bytecodeIndex;
            if (InlineCallFrame* parentCallFrame = icf->caller.inlineCallFrame) {
                FunctionExecutable* executable = static_cast<FunctionExecutable*>(parentCallFrame->executable.get());
                CodeBlock* newCodeBlock = executable->baselineCodeBlockFor(parentCallFrame->isCall ? CodeForCall : CodeForConstruct);
                ASSERT(newCodeBlock);
                ASSERT(newCodeBlock->instructionCount() > bytecodeOffset);
                callerCodeBlock = newCodeBlock;
            }
        } else if (callerCodeBlock && callerCodeBlock->getJITType() == JITCode::DFGJIT) {
            CodeOrigin origin;
            if (!callerCodeBlock->codeOriginForReturn(callFrame->returnPC(), origin)) {
                // This should not be possible, but we're seeing cases where it does happen
                // CallFrame already has robustness against bogus stack walks, so
                // we'll extend that to here as well.
                ASSERT_NOT_REACHED();
                caller = 0;
                return 0;
            }
            bytecodeOffset = origin.bytecodeIndex;
            if (InlineCallFrame* icf = origin.inlineCallFrame) {
                FunctionExecutable* executable = static_cast<FunctionExecutable*>(icf->executable.get());
                CodeBlock* newCodeBlock = executable->baselineCodeBlockFor(icf->isCall ? CodeForCall : CodeForConstruct);
                ASSERT(newCodeBlock);
                ASSERT(newCodeBlock->instructionCount() > bytecodeOffset);
                callerCodeBlock = newCodeBlock;
            }
        } else
#endif
        {
            RELEASE_ASSERT(callerCodeBlock);
            bytecodeOffset = callerCodeBlock->bytecodeOffset(trueCallerFrame, callFrame->returnPC());
        }
    }

    RELEASE_ASSERT(callerCodeBlock);
    caller = callerCodeBlock;
    return trueCallerFrame;
}

static ALWAYS_INLINE const String getSourceURLFromCallFrame(CallFrame* callFrame)
{
    ASSERT(!callFrame->hasHostCallFrameFlag());
    return callFrame->codeBlock()->ownerExecutable()->sourceURL();
}

static StackFrameCodeType getStackFrameCodeType(CallFrame* callFrame)
{
    ASSERT(!callFrame->hasHostCallFrameFlag());

    switch (callFrame->codeBlock()->codeType()) {
    case EvalCode:
        return StackFrameEvalCode;
    case FunctionCode:
        return StackFrameFunctionCode;
    case GlobalCode:
        return StackFrameGlobalCode;
    }
    RELEASE_ASSERT_NOT_REACHED();
    return StackFrameGlobalCode;
}

void StackFrame::computeLineAndColumn(unsigned& line, unsigned& column)
{
    if (!codeBlock) {
        line = 0;
        column = 0;
        return;
    }

    int divot = 0;
    int unusedStartOffset = 0;
    int unusedEndOffset = 0;
    unsigned divotLine = 0;
    unsigned divotColumn = 0;
    expressionInfo(divot, unusedStartOffset, unusedEndOffset, divotLine, divotColumn);

    line = divotLine + lineOffset;
    column = divotColumn + (divotLine ? 1 : firstLineColumnOffset);
}

void StackFrame::expressionInfo(int& divot, int& startOffset, int& endOffset, unsigned& line, unsigned& column)
{
    codeBlock->expressionRangeForBytecodeOffset(bytecodeOffset, divot, startOffset, endOffset, line, column);
    divot += characterOffset;
}

String StackFrame::toString(CallFrame* callFrame)
{
    StringBuilder traceBuild;
    String functionName = friendlyFunctionName(callFrame);
    String sourceURL = friendlySourceURL();
    traceBuild.append("\tat ");
    if (!sourceURL.isEmpty()) {
        if (!functionName.isEmpty()) {
            traceBuild.append(functionName);
            traceBuild.append(" (");
        }
        traceBuild.append(sourceURL);
        if (codeType != StackFrameNativeCode) {
            unsigned line;
            unsigned column;
            computeLineAndColumn(line, column);

            traceBuild.append(':');
            traceBuild.appendNumber(line);
            traceBuild.append(':');
            traceBuild.appendNumber(column);
        }
        if (!functionName.isEmpty()) {
            traceBuild.append(")");
        }
    }
    return traceBuild.toString().impl();
}

void Interpreter::getStackTrace(VM* vm, Vector<StackFrame>& results, size_t maxStackSize)
{
    CallFrame* callFrame = vm->topCallFrame->removeHostCallFrameFlag();
    if (!callFrame || callFrame == CallFrame::noCaller()) 
        return;
    unsigned bytecodeOffset = getBytecodeOffsetForCallFrame(callFrame);
    callFrame = callFrame->trueCallFrameFromVMCode();
    if (!callFrame)
        return;
    CodeBlock* callerCodeBlock = callFrame->codeBlock();

    while (callFrame && callFrame != CallFrame::noCaller() && maxStackSize--) {
        String sourceURL;
        if (callerCodeBlock) {
            sourceURL = getSourceURLFromCallFrame(callFrame);
            StackFrame s = {
                Strong<JSObject>(*vm, callFrame->callee()),
                getStackFrameCodeType(callFrame),
                Strong<ExecutableBase>(*vm, callerCodeBlock->ownerExecutable()),
                Strong<UnlinkedCodeBlock>(*vm, callerCodeBlock->unlinkedCodeBlock()),
                callerCodeBlock->source(),
                callerCodeBlock->ownerExecutable()->lineNo(),
                callerCodeBlock->firstLineColumnOffset(),
                callerCodeBlock->sourceOffset(),
                bytecodeOffset,
                sourceURL
            };

            results.append(s);
        } else {
            StackFrame s = { Strong<JSObject>(*vm, callFrame->callee()), StackFrameNativeCode, Strong<ExecutableBase>(), Strong<UnlinkedCodeBlock>(), 0, 0, 0, 0, 0, String()};
            results.append(s);
        }
        callFrame = getCallerInfo(vm, callFrame, bytecodeOffset, callerCodeBlock);
    }
}

JSString* Interpreter:: stackTraceAsString(ExecState* exec, Vector<StackFrame> stackTrace)
{
    // FIXME: JSStringJoiner could be more efficient than StringBuilder here.
    StringBuilder builder;
    for (unsigned i = 0; i < stackTrace.size(); i++) {
        builder.append(String(stackTrace[i].toString(exec)));
        if (i != stackTrace.size() - 1)
            builder.append('\n');
    }
    return jsString(&exec->vm(), builder.toString());
}

void Interpreter::addStackTraceIfNecessary(CallFrame* callFrame, JSValue error)
{
    VM* vm = &callFrame->vm();
    ASSERT(callFrame == vm->topCallFrame || callFrame == callFrame->lexicalGlobalObject()->globalExec() || callFrame == callFrame->dynamicGlobalObject()->globalExec());

    if (error.isObject()) {
        if (asObject(error)->hasProperty(callFrame, vm->propertyNames->stack))
            return;
    }
    
    Vector<StackFrame> stackTrace;
    getStackTrace(&callFrame->vm(), stackTrace);
    vm->exceptionStack() = RefCountedArray<StackFrame>(stackTrace);
    if (stackTrace.isEmpty() || !error.isObject())
        return;

    asObject(error)->putDirect(*vm, vm->propertyNames->stack, vm->interpreter->stackTraceAsString(vm->topCallFrame, stackTrace), ReadOnly | DontDelete);
}

NEVER_INLINE HandlerInfo* Interpreter::throwException(CallFrame*& callFrame, JSValue& exceptionValue, unsigned bytecodeOffset)
{
    CodeBlock* codeBlock = callFrame->codeBlock();
    bool isTermination = false;

    ASSERT(!exceptionValue.isEmpty());
    ASSERT(!exceptionValue.isCell() || exceptionValue.asCell());
    // This shouldn't be possible (hence the assertions), but we're already in the slowest of
    // slow cases, so let's harden against it anyway to be safe.
    if (exceptionValue.isEmpty() || (exceptionValue.isCell() && !exceptionValue.asCell()))
        exceptionValue = jsNull();

    // Set up the exception object
    if (exceptionValue.isObject()) {
        JSObject* exception = asObject(exceptionValue);

        if (exception->isErrorInstance() && static_cast<ErrorInstance*>(exception)->appendSourceToMessage())
            appendSourceToError(callFrame, static_cast<ErrorInstance*>(exception), bytecodeOffset);

        if (!hasErrorInfo(callFrame, exception)) {
            // FIXME: should only really be adding these properties to VM generated exceptions,
            // but the inspector currently requires these for all thrown objects.
            addErrorInfo(callFrame, exception, codeBlock->lineNumberForBytecodeOffset(bytecodeOffset), codeBlock->ownerExecutable()->source());
        }

        isTermination = isTerminatedExecutionException(exception);
    } else {
        if (!callFrame->vm().exceptionStack().size()) {
            Vector<StackFrame> stack;
            Interpreter::getStackTrace(&callFrame->vm(), stack);
            callFrame->vm().exceptionStack() = RefCountedArray<StackFrame>(stack);
        }
    }

    if (Debugger* debugger = callFrame->dynamicGlobalObject()->debugger()) {
        DebuggerCallFrame debuggerCallFrame(callFrame, exceptionValue);
        bool hasHandler = codeBlock->handlerForBytecodeOffset(bytecodeOffset);
        debugger->exception(debuggerCallFrame, codeBlock->ownerExecutable()->sourceID(), codeBlock->lineNumberForBytecodeOffset(bytecodeOffset), 0, hasHandler);
    }

    // Calculate an exception handler vPC, unwinding call frames as necessary.
    HandlerInfo* handler = 0;
    while (isTermination || !(handler = codeBlock->handlerForBytecodeOffset(bytecodeOffset))) {
        if (!unwindCallFrame(callFrame, exceptionValue, bytecodeOffset, codeBlock)) {
            if (LegacyProfiler* profiler = callFrame->vm().enabledProfiler())
                profiler->exceptionUnwind(callFrame);
            return 0;
        }
    }

    if (LegacyProfiler* profiler = callFrame->vm().enabledProfiler())
        profiler->exceptionUnwind(callFrame);

    // Unwind the scope chain within the exception handler's call frame.
    JSScope* scope = callFrame->scope();
    int scopeDelta = 0;
    if (!codeBlock->needsFullScopeChain() || codeBlock->codeType() != FunctionCode 
        || callFrame->uncheckedR(codeBlock->activationRegister()).jsValue()) {
        int currentDepth = depth(codeBlock, scope);
        int targetDepth = handler->scopeDepth;
        scopeDelta = currentDepth - targetDepth;
        RELEASE_ASSERT(scopeDelta >= 0);
    }
    while (scopeDelta--)
        scope = scope->next();
    callFrame->setScope(scope);

    return handler;
}

static inline JSValue checkedReturn(JSValue returnValue)
{
    ASSERT(returnValue);
    return returnValue;
}

static inline JSObject* checkedReturn(JSObject* returnValue)
{
    ASSERT(returnValue);
    return returnValue;
}

class SamplingScope {
public:
    SamplingScope(Interpreter* interpreter)
        : m_interpreter(interpreter)
    {
        interpreter->startSampling();
    }
    ~SamplingScope()
    {
        m_interpreter->stopSampling();
    }
private:
    Interpreter* m_interpreter;
};

JSValue Interpreter::execute(ProgramExecutable* program, CallFrame* callFrame, JSObject* thisObj)
{
    SamplingScope samplingScope(this);
    
    JSScope* scope = callFrame->scope();
    VM& vm = *scope->vm();

    ASSERT(isValidThisObject(thisObj, callFrame));
    ASSERT(!vm.exception);
    ASSERT(!vm.isCollectorBusy());
    if (vm.isCollectorBusy())
        return jsNull();

    StackStats::CheckPoint stackCheckPoint;
    const VMStackBounds vmStackBounds(vm, wtfThreadData().stack());
    if (!vmStackBounds.isSafeToRecurse())
        return checkedReturn(throwStackOverflowError(callFrame));

    // First check if the "program" is actually just a JSON object. If so,
    // we'll handle the JSON object here. Else, we'll handle real JS code
    // below at failedJSONP.
    DynamicGlobalObjectScope globalObjectScope(vm, scope->globalObject());
    Vector<JSONPData> JSONPData;
    bool parseResult;
    const String programSource = program->source().toString();
    if (programSource.isNull())
        return jsUndefined();
    if (programSource.is8Bit()) {
        LiteralParser<LChar> literalParser(callFrame, programSource.characters8(), programSource.length(), JSONP);
        parseResult = literalParser.tryJSONPParse(JSONPData, scope->globalObject()->globalObjectMethodTable()->supportsRichSourceInfo(scope->globalObject()));
    } else {
        LiteralParser<UChar> literalParser(callFrame, programSource.characters16(), programSource.length(), JSONP);
        parseResult = literalParser.tryJSONPParse(JSONPData, scope->globalObject()->globalObjectMethodTable()->supportsRichSourceInfo(scope->globalObject()));
    }

    if (parseResult) {
        JSGlobalObject* globalObject = scope->globalObject();
        JSValue result;
        for (unsigned entry = 0; entry < JSONPData.size(); entry++) {
            Vector<JSONPPathEntry> JSONPPath;
            JSONPPath.swap(JSONPData[entry].m_path);
            JSValue JSONPValue = JSONPData[entry].m_value.get();
            if (JSONPPath.size() == 1 && JSONPPath[0].m_type == JSONPPathEntryTypeDeclare) {
                if (globalObject->hasProperty(callFrame, JSONPPath[0].m_pathEntryName)) {
                    PutPropertySlot slot;
                    globalObject->methodTable()->put(globalObject, callFrame, JSONPPath[0].m_pathEntryName, JSONPValue, slot);
                } else
                    globalObject->methodTable()->putDirectVirtual(globalObject, callFrame, JSONPPath[0].m_pathEntryName, JSONPValue, DontEnum | DontDelete);
                // var declarations return undefined
                result = jsUndefined();
                continue;
            }
            JSValue baseObject(globalObject);
            for (unsigned i = 0; i < JSONPPath.size() - 1; i++) {
                ASSERT(JSONPPath[i].m_type != JSONPPathEntryTypeDeclare);
                switch (JSONPPath[i].m_type) {
                case JSONPPathEntryTypeDot: {
                    if (i == 0) {
                        PropertySlot slot(globalObject);
                        if (!globalObject->getPropertySlot(callFrame, JSONPPath[i].m_pathEntryName, slot)) {
                            if (entry)
                                return throwError(callFrame, createUndefinedVariableError(globalObject->globalExec(), JSONPPath[i].m_pathEntryName));
                            goto failedJSONP;
                        }
                        baseObject = slot.getValue(callFrame, JSONPPath[i].m_pathEntryName);
                    } else
                        baseObject = baseObject.get(callFrame, JSONPPath[i].m_pathEntryName);
                    if (callFrame->hadException())
                        return jsUndefined();
                    continue;
                }
                case JSONPPathEntryTypeLookup: {
                    baseObject = baseObject.get(callFrame, JSONPPath[i].m_pathIndex);
                    if (callFrame->hadException())
                        return jsUndefined();
                    continue;
                }
                default:
                    RELEASE_ASSERT_NOT_REACHED();
                    return jsUndefined();
                }
            }
            PutPropertySlot slot;
            switch (JSONPPath.last().m_type) {
            case JSONPPathEntryTypeCall: {
                JSValue function = baseObject.get(callFrame, JSONPPath.last().m_pathEntryName);
                if (callFrame->hadException())
                    return jsUndefined();
                CallData callData;
                CallType callType = getCallData(function, callData);
                if (callType == CallTypeNone)
                    return throwError(callFrame, createNotAFunctionError(callFrame, function));
                MarkedArgumentBuffer jsonArg;
                jsonArg.append(JSONPValue);
                JSValue thisValue = JSONPPath.size() == 1 ? jsUndefined(): baseObject;
                JSONPValue = JSC::call(callFrame, function, callType, callData, thisValue, jsonArg);
                if (callFrame->hadException())
                    return jsUndefined();
                break;
            }
            case JSONPPathEntryTypeDot: {
                baseObject.put(callFrame, JSONPPath.last().m_pathEntryName, JSONPValue, slot);
                if (callFrame->hadException())
                    return jsUndefined();
                break;
            }
            case JSONPPathEntryTypeLookup: {
                baseObject.putByIndex(callFrame, JSONPPath.last().m_pathIndex, JSONPValue, slot.isStrictMode());
                if (callFrame->hadException())
                    return jsUndefined();
                break;
            }
            default:
                RELEASE_ASSERT_NOT_REACHED();
                    return jsUndefined();
            }
            result = JSONPValue;
        }
        return result;
    }
failedJSONP:
    // If we get here, then we have already proven that the script is not a JSON
    // object.

    // Compile source to bytecode if necessary:
    if (JSObject* error = program->initializeGlobalProperties(vm, callFrame, scope))
        return checkedReturn(throwError(callFrame, error));

    if (JSObject* error = program->compile(callFrame, scope))
        return checkedReturn(throwError(callFrame, error));

    ProgramCodeBlock* codeBlock = &program->generatedBytecode();

    if (UNLIKELY(vm.watchdog.didFire(callFrame)))
        return throwTerminatedExecutionException(callFrame);

    // Push the call frame for this invocation:
    ASSERT(codeBlock->numParameters() == 1); // 1 parameter for 'this'.
    CallFrame* newCallFrame = m_stack.pushFrame(callFrame, codeBlock, scope, 1, 0);
    if (UNLIKELY(!newCallFrame))
        return checkedReturn(throwStackOverflowError(callFrame));

    // Set the arguments for the callee:
    newCallFrame->setThisValue(thisObj);

    if (LegacyProfiler* profiler = vm.enabledProfiler())
        profiler->willExecute(callFrame, program->sourceURL(), program->lineNo());

    // Execute the code:
    JSValue result;
    {
        SamplingTool::CallRecord callRecord(m_sampler.get());
        Watchdog::Scope watchdogScope(vm.watchdog);

#if ENABLE(LLINT_C_LOOP)
        result = LLInt::CLoop::execute(newCallFrame, llint_program_prologue);
#elif ENABLE(JIT)
        result = program->generatedJITCode().execute(&m_stack, newCallFrame, &vm);
#endif // ENABLE(JIT)
    }

    if (LegacyProfiler* profiler = vm.enabledProfiler())
        profiler->didExecute(callFrame, program->sourceURL(), program->lineNo());

    m_stack.popFrame(newCallFrame);

    return checkedReturn(result);
}

JSValue Interpreter::executeCall(CallFrame* callFrame, JSObject* function, CallType callType, const CallData& callData, JSValue thisValue, const ArgList& args)
{
    VM& vm = callFrame->vm();
    ASSERT(isValidThisObject(thisValue, callFrame));
    ASSERT(!callFrame->hadException());
    ASSERT(!vm.isCollectorBusy());
    if (vm.isCollectorBusy())
        return jsNull();

    StackStats::CheckPoint stackCheckPoint;
    const VMStackBounds vmStackBounds(vm, wtfThreadData().stack());
    if (!vmStackBounds.isSafeToRecurse())
        return checkedReturn(throwStackOverflowError(callFrame));

    bool isJSCall = (callType == CallTypeJS);
    JSScope* scope;
    CodeBlock* newCodeBlock;
    size_t argsCount = 1 + args.size(); // implicit "this" parameter

    if (isJSCall)
        scope = callData.js.scope;
    else {
        ASSERT(callType == CallTypeHost);
        scope = callFrame->scope();
    }
    DynamicGlobalObjectScope globalObjectScope(vm, scope->globalObject());

    if (isJSCall) {
        // Compile the callee:
        JSObject* compileError = callData.js.functionExecutable->compileForCall(callFrame, scope);
        if (UNLIKELY(!!compileError)) {
            return checkedReturn(throwError(callFrame, compileError));
        }
        newCodeBlock = &callData.js.functionExecutable->generatedBytecodeForCall();
        ASSERT(!!newCodeBlock);
    } else
        newCodeBlock = 0;

    if (UNLIKELY(vm.watchdog.didFire(callFrame)))
        return throwTerminatedExecutionException(callFrame);

    CallFrame* newCallFrame = m_stack.pushFrame(callFrame, newCodeBlock, scope, argsCount, function);
    if (UNLIKELY(!newCallFrame))
        return checkedReturn(throwStackOverflowError(callFrame));

    // Set the arguments for the callee:
    newCallFrame->setThisValue(thisValue);
    for (size_t i = 0; i < args.size(); ++i)
        newCallFrame->setArgument(i, args.at(i));

    if (LegacyProfiler* profiler = vm.enabledProfiler())
        profiler->willExecute(callFrame, function);

    JSValue result;
    {
        SamplingTool::CallRecord callRecord(m_sampler.get(), !isJSCall);
        Watchdog::Scope watchdogScope(vm.watchdog);

        // Execute the code:
        if (isJSCall) {
#if ENABLE(LLINT_C_LOOP)
            result = LLInt::CLoop::execute(newCallFrame, llint_function_for_call_prologue);
#elif ENABLE(JIT)
            result = callData.js.functionExecutable->generatedJITCodeForCall().execute(&m_stack, newCallFrame, &vm);
#endif // ENABLE(JIT)
        } else
            result = JSValue::decode(callData.native.function(newCallFrame));
    }

    if (LegacyProfiler* profiler = vm.enabledProfiler())
        profiler->didExecute(callFrame, function);

    m_stack.popFrame(newCallFrame);
    return checkedReturn(result);
}

JSObject* Interpreter::executeConstruct(CallFrame* callFrame, JSObject* constructor, ConstructType constructType, const ConstructData& constructData, const ArgList& args)
{
    VM& vm = callFrame->vm();
    ASSERT(!callFrame->hadException());
    ASSERT(!vm.isCollectorBusy());
    // We throw in this case because we have to return something "valid" but we're
    // already in an invalid state.
    if (vm.isCollectorBusy())
        return checkedReturn(throwStackOverflowError(callFrame));

    StackStats::CheckPoint stackCheckPoint;
    const VMStackBounds vmStackBounds(vm, wtfThreadData().stack());
    if (!vmStackBounds.isSafeToRecurse())
        return checkedReturn(throwStackOverflowError(callFrame));

    bool isJSConstruct = (constructType == ConstructTypeJS);
    JSScope* scope;
    CodeBlock* newCodeBlock;
    size_t argsCount = 1 + args.size(); // implicit "this" parameter

    if (isJSConstruct)
        scope = constructData.js.scope;
    else {
        ASSERT(constructType == ConstructTypeHost);
        scope = callFrame->scope();
    }

    DynamicGlobalObjectScope globalObjectScope(vm, scope->globalObject());

    if (isJSConstruct) {
        // Compile the callee:
        JSObject* compileError = constructData.js.functionExecutable->compileForConstruct(callFrame, scope);
        if (UNLIKELY(!!compileError)) {
            return checkedReturn(throwError(callFrame, compileError));
        }
        newCodeBlock = &constructData.js.functionExecutable->generatedBytecodeForConstruct();
        ASSERT(!!newCodeBlock);
    } else
        newCodeBlock = 0;

    if (UNLIKELY(vm.watchdog.didFire(callFrame)))
        return throwTerminatedExecutionException(callFrame);

    CallFrame* newCallFrame = m_stack.pushFrame(callFrame, newCodeBlock, scope, argsCount, constructor);
    if (UNLIKELY(!newCallFrame))
        return checkedReturn(throwStackOverflowError(callFrame));

    // Set the arguments for the callee:
    newCallFrame->setThisValue(jsUndefined());
    for (size_t i = 0; i < args.size(); ++i)
        newCallFrame->setArgument(i, args.at(i));

    if (LegacyProfiler* profiler = vm.enabledProfiler())
        profiler->willExecute(callFrame, constructor);

    JSValue result;
    {
        SamplingTool::CallRecord callRecord(m_sampler.get(), !isJSConstruct);
        Watchdog::Scope watchdogScope(vm.watchdog);

        // Execute the code.
        if (isJSConstruct) {
#if ENABLE(LLINT_C_LOOP)
            result = LLInt::CLoop::execute(newCallFrame, llint_function_for_construct_prologue);
#elif ENABLE(JIT)
            result = constructData.js.functionExecutable->generatedJITCodeForConstruct().execute(&m_stack, newCallFrame, &vm);
#endif // ENABLE(JIT)
        } else
            result = JSValue::decode(constructData.native.function(newCallFrame));
    }

    if (LegacyProfiler* profiler = vm.enabledProfiler())
        profiler->didExecute(callFrame, constructor);

    m_stack.popFrame(newCallFrame);

    if (callFrame->hadException())
        return 0;
    ASSERT(result.isObject());
    return checkedReturn(asObject(result));
}

CallFrameClosure Interpreter::prepareForRepeatCall(FunctionExecutable* functionExecutable, CallFrame* callFrame, JSFunction* function, int argumentCountIncludingThis, JSScope* scope)
{
    VM& vm = *scope->vm();
    ASSERT(!vm.exception);
    
    if (vm.isCollectorBusy())
        return CallFrameClosure();

    StackStats::CheckPoint stackCheckPoint;
    const VMStackBounds vmStackBounds(vm, wtfThreadData().stack());
    if (!vmStackBounds.isSafeToRecurse()) {
        throwStackOverflowError(callFrame);
        return CallFrameClosure();
    }

    // Compile the callee:
    JSObject* error = functionExecutable->compileForCall(callFrame, scope);
    if (error) {
        throwError(callFrame, error);
        return CallFrameClosure();
    }
    CodeBlock* newCodeBlock = &functionExecutable->generatedBytecodeForCall();

    size_t argsCount = argumentCountIncludingThis;

    CallFrame* newCallFrame = m_stack.pushFrame(callFrame, newCodeBlock, scope, argsCount, function);  
    if (UNLIKELY(!newCallFrame)) {
        throwStackOverflowError(callFrame);
        return CallFrameClosure();
    }

    if (UNLIKELY(!newCallFrame)) {
        throwStackOverflowError(callFrame);
        return CallFrameClosure();
    }

    // Return the successful closure:
    CallFrameClosure result = { callFrame, newCallFrame, function, functionExecutable, &vm, scope, newCodeBlock->numParameters(), argumentCountIncludingThis };
    return result;
}

JSValue Interpreter::execute(CallFrameClosure& closure) 
{
    VM& vm = *closure.vm;
    SamplingScope samplingScope(this);
    
    ASSERT(!vm.isCollectorBusy());
    if (vm.isCollectorBusy())
        return jsNull();

    StackStats::CheckPoint stackCheckPoint;
    m_stack.validateFence(closure.newCallFrame, "BEFORE");
    closure.resetCallFrame();
    m_stack.validateFence(closure.newCallFrame, "STEP 1");

    if (LegacyProfiler* profiler = vm.enabledProfiler())
        profiler->willExecute(closure.oldCallFrame, closure.function);

    if (UNLIKELY(vm.watchdog.didFire(closure.oldCallFrame)))
        return throwTerminatedExecutionException(closure.oldCallFrame);

    // The code execution below may push more frames and point the topCallFrame
    // to those newer frames, or it may pop to the top frame to the caller of
    // the current repeat frame, or it may leave the top frame pointing to the
    // current repeat frame.
    //
    // Hence, we need to preserve the topCallFrame here ourselves before
    // repeating this call on a second callback function.

    TopCallFrameSetter topCallFrame(vm, closure.newCallFrame);

    // Execute the code:
    JSValue result;
    {
        SamplingTool::CallRecord callRecord(m_sampler.get());
        Watchdog::Scope watchdogScope(vm.watchdog);

#if ENABLE(LLINT_C_LOOP)
        result = LLInt::CLoop::execute(closure.newCallFrame, llint_function_for_call_prologue);
#elif ENABLE(JIT)
        result = closure.functionExecutable->generatedJITCodeForCall().execute(&m_stack, closure.newCallFrame, &vm);
#endif // ENABLE(JIT)
    }

    if (LegacyProfiler* profiler = vm.enabledProfiler())
        profiler->didExecute(closure.oldCallFrame, closure.function);

    m_stack.validateFence(closure.newCallFrame, "AFTER");
    return checkedReturn(result);
}

void Interpreter::endRepeatCall(CallFrameClosure& closure)
{
    m_stack.popFrame(closure.newCallFrame);
}

JSValue Interpreter::execute(EvalExecutable* eval, CallFrame* callFrame, JSValue thisValue, JSScope* scope)
{
    VM& vm = *scope->vm();
    SamplingScope samplingScope(this);
    
    ASSERT(scope->vm() == &callFrame->vm());
    ASSERT(isValidThisObject(thisValue, callFrame));
    ASSERT(!vm.exception);
    ASSERT(!vm.isCollectorBusy());
    if (vm.isCollectorBusy())
        return jsNull();

    DynamicGlobalObjectScope globalObjectScope(vm, scope->globalObject());

    StackStats::CheckPoint stackCheckPoint;
    const VMStackBounds vmStackBounds(vm, wtfThreadData().stack());
    if (!vmStackBounds.isSafeToRecurse())
        return checkedReturn(throwStackOverflowError(callFrame));

    // Compile the callee:
    JSObject* compileError = eval->compile(callFrame, scope);
    if (UNLIKELY(!!compileError))
        return checkedReturn(throwError(callFrame, compileError));
    EvalCodeBlock* codeBlock = &eval->generatedBytecode();

    JSObject* variableObject;
    for (JSScope* node = scope; ; node = node->next()) {
        RELEASE_ASSERT(node);
        if (node->isVariableObject() && !node->isNameScopeObject()) {
            variableObject = node;
            break;
        }
    }

    unsigned numVariables = codeBlock->numVariables();
    int numFunctions = codeBlock->numberOfFunctionDecls();
    if (numVariables || numFunctions) {
        if (codeBlock->isStrictMode()) {
            scope = StrictEvalActivation::create(callFrame);
            variableObject = scope;
        }
        // Scope for BatchedTransitionOptimizer
        BatchedTransitionOptimizer optimizer(vm, variableObject);

        for (unsigned i = 0; i < numVariables; ++i) {
            const Identifier& ident = codeBlock->variable(i);
            if (!variableObject->hasProperty(callFrame, ident)) {
                PutPropertySlot slot;
                variableObject->methodTable()->put(variableObject, callFrame, ident, jsUndefined(), slot);
            }
        }

        for (int i = 0; i < numFunctions; ++i) {
            FunctionExecutable* function = codeBlock->functionDecl(i);
            PutPropertySlot slot;
            variableObject->methodTable()->put(variableObject, callFrame, function->name(), JSFunction::create(callFrame, function, scope), slot);
        }
    }

    if (UNLIKELY(vm.watchdog.didFire(callFrame)))
        return throwTerminatedExecutionException(callFrame);

    // Push the frame:
    ASSERT(codeBlock->numParameters() == 1); // 1 parameter for 'this'.
    CallFrame* newCallFrame = m_stack.pushFrame(callFrame, codeBlock, scope, 1, 0);
    if (UNLIKELY(!newCallFrame))
        return checkedReturn(throwStackOverflowError(callFrame));

    // Set the arguments for the callee:
    newCallFrame->setThisValue(thisValue);

    if (LegacyProfiler* profiler = vm.enabledProfiler())
        profiler->willExecute(callFrame, eval->sourceURL(), eval->lineNo());

    // Execute the code:
    JSValue result;
    {
        SamplingTool::CallRecord callRecord(m_sampler.get());
        Watchdog::Scope watchdogScope(vm.watchdog);

#if ENABLE(LLINT_C_LOOP)
        result = LLInt::CLoop::execute(newCallFrame, llint_eval_prologue);
#elif ENABLE(JIT)
        result = eval->generatedJITCode().execute(&m_stack, newCallFrame, &vm);
#endif // ENABLE(JIT)
    }

    if (LegacyProfiler* profiler = vm.enabledProfiler())
        profiler->didExecute(callFrame, eval->sourceURL(), eval->lineNo());

    m_stack.popFrame(newCallFrame);
    return checkedReturn(result);
}

NEVER_INLINE void Interpreter::debug(CallFrame* callFrame, DebugHookID debugHookID, int firstLine, int lastLine, int column)
{
    Debugger* debugger = callFrame->dynamicGlobalObject()->debugger();
    if (!debugger)
        return;

    switch (debugHookID) {
        case DidEnterCallFrame:
            debugger->callEvent(callFrame, callFrame->codeBlock()->ownerExecutable()->sourceID(), firstLine, column);
            return;
        case WillLeaveCallFrame:
            debugger->returnEvent(callFrame, callFrame->codeBlock()->ownerExecutable()->sourceID(), lastLine, column);
            return;
        case WillExecuteStatement:
            debugger->atStatement(callFrame, callFrame->codeBlock()->ownerExecutable()->sourceID(), firstLine, column);
            return;
        case WillExecuteProgram:
            debugger->willExecuteProgram(callFrame, callFrame->codeBlock()->ownerExecutable()->sourceID(), firstLine, column);
            return;
        case DidExecuteProgram:
            debugger->didExecuteProgram(callFrame, callFrame->codeBlock()->ownerExecutable()->sourceID(), lastLine, column);
            return;
        case DidReachBreakpoint:
            debugger->didReachBreakpoint(callFrame, callFrame->codeBlock()->ownerExecutable()->sourceID(), lastLine, column);
            return;
    }
}
    
JSValue Interpreter::retrieveArgumentsFromVMCode(CallFrame* callFrame, JSFunction* function) const
{
    CallFrame* functionCallFrame = findFunctionCallFrameFromVMCode(callFrame, function);
    if (!functionCallFrame)
        return jsNull();

    Arguments* arguments = Arguments::create(functionCallFrame->vm(), functionCallFrame);
    arguments->tearOff(functionCallFrame);
    return JSValue(arguments);
}

JSValue Interpreter::retrieveCallerFromVMCode(CallFrame* callFrame, JSFunction* function) const
{
    CallFrame* functionCallFrame = findFunctionCallFrameFromVMCode(callFrame, function);

    if (!functionCallFrame)
        return jsNull();
    
    unsigned bytecodeOffset;
    CodeBlock* unusedCallerCodeBlock = 0;
    CallFrame* callerFrame = getCallerInfo(&callFrame->vm(), functionCallFrame, bytecodeOffset, unusedCallerCodeBlock);
    if (!callerFrame)
        return jsNull();
    JSValue caller = callerFrame->callee();
    if (!caller)
        return jsNull();

    // Skip over function bindings.
    ASSERT(caller.isObject());
    while (asObject(caller)->inherits(&JSBoundFunction::s_info)) {
        callerFrame = getCallerInfo(&callFrame->vm(), callerFrame, bytecodeOffset, unusedCallerCodeBlock);
        if (!callerFrame)
            return jsNull();
        caller = callerFrame->callee();
        if (!caller)
            return jsNull();
    }

    return caller;
}

void Interpreter::retrieveLastCaller(CallFrame* callFrame, int& lineNumber, intptr_t& sourceID, String& sourceURL, JSValue& function) const
{
    function = JSValue();
    lineNumber = -1;
    sourceURL = String();

    CallFrame* callerFrame = callFrame->callerFrame();
    if (callerFrame->hasHostCallFrameFlag())
        return;

    CodeBlock* callerCodeBlock = callerFrame->codeBlock();
    if (!callerCodeBlock)
        return;
    unsigned bytecodeOffset = 0;
    bytecodeOffset = callerCodeBlock->bytecodeOffset(callerFrame, callFrame->returnPC());
    lineNumber = callerCodeBlock->lineNumberForBytecodeOffset(bytecodeOffset - 1);
    sourceID = callerCodeBlock->ownerExecutable()->sourceID();
    sourceURL = callerCodeBlock->ownerExecutable()->sourceURL();
    function = callerFrame->callee();
}

CallFrame* Interpreter::findFunctionCallFrameFromVMCode(CallFrame* callFrame, JSFunction* function)
{
    for (CallFrame* candidate = callFrame->trueCallFrameFromVMCode(); candidate; candidate = candidate->trueCallerFrame()) {
        if (candidate->callee() == function)
            return candidate;
    }
    return 0;
}

void Interpreter::enableSampler()
{
#if ENABLE(OPCODE_SAMPLING)
    if (!m_sampler) {
        m_sampler = adoptPtr(new SamplingTool(this));
        m_sampler->setup();
    }
#endif
}
void Interpreter::dumpSampleData(ExecState* exec)
{
#if ENABLE(OPCODE_SAMPLING)
    if (m_sampler)
        m_sampler->dump(exec);
#else
    UNUSED_PARAM(exec);
#endif
}
void Interpreter::startSampling()
{
#if ENABLE(SAMPLING_THREAD)
    if (!m_sampleEntryDepth)
        SamplingThread::start();

    m_sampleEntryDepth++;
#endif
}
void Interpreter::stopSampling()
{
#if ENABLE(SAMPLING_THREAD)
    m_sampleEntryDepth--;
    if (!m_sampleEntryDepth)
        SamplingThread::stop();
#endif
}

} // namespace JSC
