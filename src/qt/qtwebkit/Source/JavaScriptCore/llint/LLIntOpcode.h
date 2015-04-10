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

#ifndef LLIntOpcode_h
#define LLIntOpcode_h

#include <wtf/Platform.h>

#if ENABLE(LLINT)

#if ENABLE(LLINT_C_LOOP)

#define FOR_EACH_LLINT_NOJIT_NATIVE_HELPER(macro) \
    macro(getHostCallReturnValue, 1) \
    macro(ctiOpThrowNotCaught, 1)

#else // !ENABLE(LLINT_C_LOOP)

#define FOR_EACH_LLINT_NOJIT_NATIVE_HELPER(macro) \
    // Nothing to do here. Use the JIT impl instead.

#endif // !ENABLE(LLINT_C_LOOP)


#define FOR_EACH_LLINT_NATIVE_HELPER(macro) \
    FOR_EACH_LLINT_NOJIT_NATIVE_HELPER(macro) \
    \
    macro(llint_begin, 1) \
    \
    macro(llint_program_prologue, 1) \
    macro(llint_eval_prologue, 1) \
    macro(llint_function_for_call_prologue, 1) \
    macro(llint_function_for_construct_prologue, 1) \
    macro(llint_function_for_call_arity_check, 1) \
    macro(llint_function_for_construct_arity_check, 1) \
    macro(llint_generic_return_point, 1) \
    macro(llint_throw_from_slow_path_trampoline, 1) \
    macro(llint_throw_during_call_trampoline, 1) \
    \
    /* Native call trampolines */ \
    macro(llint_native_call_trampoline, 1) \
    macro(llint_native_construct_trampoline, 1) \
    \
    macro(llint_end, 1)


#if ENABLE(LLINT_C_LOOP)
#define FOR_EACH_LLINT_OPCODE_EXTENSION(macro) FOR_EACH_LLINT_NATIVE_HELPER(macro)
#else
#define FOR_EACH_LLINT_OPCODE_EXTENSION(macro) // Nothing to add.
#endif

#else // !ENABLE(LLINT)

#define FOR_EACH_LLINT_OPCODE_EXTENSION(macro) // Nothing to add.

#endif // !ENABLE(LLINT)

#endif // LLIntOpcode_h
