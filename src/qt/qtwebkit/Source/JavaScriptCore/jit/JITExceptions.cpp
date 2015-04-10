/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
#include "JITExceptions.h"

#include "CallFrame.h"
#include "CodeBlock.h"
#include "Interpreter.h"
#include "JSCJSValue.h"
#include "VM.h"
#include "Operations.h"

#if ENABLE(JIT) || ENABLE(LLINT)

namespace JSC {

ExceptionHandler genericThrow(VM* vm, ExecState* callFrame, JSValue exceptionValue, unsigned vPCIndex)
{
    RELEASE_ASSERT(exceptionValue);
    
    vm->exception = JSValue();
    HandlerInfo* handler = vm->interpreter->throwException(callFrame, exceptionValue, vPCIndex); // This may update callFrame & exceptionValue!
    vm->exception = exceptionValue;

    void* catchRoutine;
    Instruction* catchPCForInterpreter = 0;
    if (handler) {
        catchPCForInterpreter = &callFrame->codeBlock()->instructions()[handler->target];
        catchRoutine = ExecutableBase::catchRoutineFor(handler, catchPCForInterpreter);
    } else
        catchRoutine = FunctionPtr(LLInt::getCodePtr(ctiOpThrowNotCaught)).value();
    
    vm->callFrameForThrow = callFrame;
    vm->targetMachinePCForThrow = catchRoutine;
    vm->targetInterpreterPCForThrow = catchPCForInterpreter;
    
    RELEASE_ASSERT(catchRoutine);
    ExceptionHandler exceptionHandler = { catchRoutine, callFrame };
    return exceptionHandler;
}

ExceptionHandler jitThrow(VM* vm, ExecState* callFrame, JSValue exceptionValue, ReturnAddressPtr faultLocation)
{
    return genericThrow(vm, callFrame, exceptionValue, callFrame->codeBlock()->bytecodeOffset(callFrame, faultLocation));
}

}

#endif
