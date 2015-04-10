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

#ifndef LLIntSlowPaths_h
#define LLIntSlowPaths_h

#include <wtf/Platform.h>
#include <wtf/StdLibExtras.h>

#if ENABLE(LLINT)

namespace JSC {

class ExecState;
struct Instruction;

namespace LLInt {

#if USE(JSVALUE64)
// According to C++ rules, a type used for the return signature of function with C linkage (i.e.
// 'extern "C"') needs to be POD; hence putting any constructors into it could cause either compiler
// warnings, or worse, a change in the ABI used to return these types.
struct SlowPathReturnType {
    void* a;
    ExecState* b;
};

inline SlowPathReturnType encodeResult(void* a, ExecState* b)
{
    SlowPathReturnType result;
    result.a = a;
    result.b = b;
    return result;
}

inline void decodeResult(SlowPathReturnType result, void*& a, ExecState*& b)
{
    a = result.a;
    b = result.b;
}

#else // USE(JSVALUE32_64)
typedef int64_t SlowPathReturnType;

typedef union {
    struct {
        void* a;
        ExecState* b;
    } pair;
    int64_t i;
} SlowPathReturnTypeEncoding;

inline SlowPathReturnType encodeResult(void* a, ExecState* b)
{
    SlowPathReturnTypeEncoding u;
    u.pair.a = a;
    u.pair.b = b;
    return u.i;
}

inline void decodeResult(SlowPathReturnType result, void*& a, ExecState*& b)
{
    SlowPathReturnTypeEncoding u;
    u.i = result;
    a = u.pair.a;
    b = u.pair.b;
}
#endif // USE(JSVALUE32_64)

extern "C" SlowPathReturnType llint_trace_operand(ExecState*, Instruction*, int fromWhere, int operand);
extern "C" SlowPathReturnType llint_trace_value(ExecState*, Instruction*, int fromWhere, int operand);

#define LLINT_SLOW_PATH_DECL(name) \
    extern "C" SlowPathReturnType llint_##name(ExecState* exec, Instruction* pc)

#define LLINT_SLOW_PATH_HIDDEN_DECL(name) \
    LLINT_SLOW_PATH_DECL(name) WTF_INTERNAL

LLINT_SLOW_PATH_HIDDEN_DECL(trace_prologue);
LLINT_SLOW_PATH_HIDDEN_DECL(trace_prologue_function_for_call);
LLINT_SLOW_PATH_HIDDEN_DECL(trace_prologue_function_for_construct);
LLINT_SLOW_PATH_HIDDEN_DECL(trace_arityCheck_for_call);
LLINT_SLOW_PATH_HIDDEN_DECL(trace_arityCheck_for_construct);
LLINT_SLOW_PATH_HIDDEN_DECL(trace);
LLINT_SLOW_PATH_HIDDEN_DECL(special_trace);
LLINT_SLOW_PATH_HIDDEN_DECL(entry_osr);
LLINT_SLOW_PATH_HIDDEN_DECL(entry_osr_function_for_call);
LLINT_SLOW_PATH_HIDDEN_DECL(entry_osr_function_for_construct);
LLINT_SLOW_PATH_HIDDEN_DECL(entry_osr_function_for_call_arityCheck);
LLINT_SLOW_PATH_HIDDEN_DECL(entry_osr_function_for_construct_arityCheck);
LLINT_SLOW_PATH_HIDDEN_DECL(loop_osr);
LLINT_SLOW_PATH_HIDDEN_DECL(replace);
LLINT_SLOW_PATH_HIDDEN_DECL(stack_check);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_call_arityCheck);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_construct_arityCheck);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_create_activation);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_create_arguments);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_create_this);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_convert_this);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_new_object);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_new_array);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_new_array_with_size);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_new_array_buffer);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_new_regexp);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_not);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_eq);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_neq);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_stricteq);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_nstricteq);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_less);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_lesseq);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_greater);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_greatereq);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_pre_inc);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_pre_dec);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_to_number);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_negate);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_add);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_mul);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_sub);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_div);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_mod);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_lshift);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_rshift);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_urshift);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_bitand);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_bitor);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_bitxor);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_check_has_instance);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_instanceof);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_typeof);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_is_object);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_is_function);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_in);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_resolve);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_put_to_base);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_resolve_base);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_resolve_with_base);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_resolve_with_this);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_init_global_const_check);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_get_by_id);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_get_arguments_length);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_put_by_id);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_del_by_id);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_get_by_val);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_get_argument_by_val);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_get_by_pname);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_put_by_val);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_del_by_val);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_put_by_index);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_put_getter_setter);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_jtrue);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_jfalse);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_jless);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_jnless);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_jgreater);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_jngreater);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_jlesseq);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_jnlesseq);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_jgreatereq);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_jngreatereq);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_switch_imm);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_switch_char);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_switch_string);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_new_func);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_new_func_exp);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_call);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_construct);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_call_varargs);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_call_eval);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_tear_off_activation);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_tear_off_arguments);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_strcat);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_to_primitive);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_get_pnames);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_next_pname);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_push_with_scope);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_pop_scope);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_push_name_scope);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_throw);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_throw_static_error);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_handle_watchdog_timer);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_debug);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_profile_will_call);
LLINT_SLOW_PATH_HIDDEN_DECL(slow_path_profile_did_call);
LLINT_SLOW_PATH_HIDDEN_DECL(throw_from_native_call);

} } // namespace JSC::LLInt

#endif // ENABLE(LLINT)

#endif // LLIntSlowPaths_h

