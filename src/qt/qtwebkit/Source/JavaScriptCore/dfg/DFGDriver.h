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

#ifndef DFGDriver_h
#define DFGDriver_h

#include "CallFrame.h"
#include <wtf/Platform.h>

namespace JSC {

class CodeBlock;
class JITCode;
class VM;
class MacroAssemblerCodePtr;

namespace DFG {

JS_EXPORT_PRIVATE unsigned getNumCompilations();

#if ENABLE(DFG_JIT)
bool tryCompile(ExecState*, CodeBlock*, JITCode&, unsigned bytecodeIndex);
bool tryCompileFunction(ExecState*, CodeBlock*, JITCode&, MacroAssemblerCodePtr& jitCodeWithArityCheck, unsigned bytecodeIndex);
#else
inline bool tryCompile(ExecState*, CodeBlock*, JITCode&, unsigned) { return false; }
inline bool tryCompileFunction(ExecState*, CodeBlock*, JITCode&, MacroAssemblerCodePtr&, unsigned) { return false; }
#endif

} } // namespace JSC::DFG

#endif

