/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef LLIntExceptions_h
#define LLIntExceptions_h

#include <wtf/Platform.h>
#include <wtf/StdLibExtras.h>

#if ENABLE(LLINT)

#include "MacroAssemblerCodeRef.h"

namespace JSC {

class ExecState;
struct Instruction;

namespace LLInt {

// Throw the currently active exception in the context of the caller's call frame.
void interpreterThrowInCaller(ExecState* callerFrame, ReturnAddressPtr);

// Tells you where to jump to if you want to return-to-throw, after you've already
// set up all information needed to throw the exception.
Instruction* returnToThrowForThrownException(ExecState*);

// Saves the current PC in the global data for safe-keeping, and gives you a PC
// that you can tell the interpreter to go to, which when advanced between 1
// and 9 slots will give you an "instruction" that threads to the interpreter's
// exception handler. Note that if you give it the PC for exception handling,
// it's smart enough to just return that PC without doing anything else; this
// lets you thread exception handling through common helper functions used by
// other helpers.
Instruction* returnToThrow(ExecState*, Instruction*);

// Use this when you're throwing to a call thunk.
void* callToThrow(ExecState*, Instruction*);

} } // namespace JSC::LLInt

#endif // ENABLE(LLINT)

#endif // LLIntExceptions_h
