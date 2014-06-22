/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#ifndef DFGVariadicFunction_h
#define DFGVariadicFunction_h

#define DFG_COMMA ,

// The signature of v is (templatePre, templatePost, typeParams, valueParams, valueArgs)
//
// You would use it like:
// #define DEFINE_FUNCTION(templatePre, templatePost, typeParams, valueParamsComma, valueParams, valueArgs)
//     templatePre typeParams templatePost void f(valueParams) { g(valueArgs); }
// DFG_VARIADIC_TEMPLATE_FUNCTION(DEFINE_FUNCTION)
// #undef DEFINE_FUNCTION
//
// Or if you wanted the defined function to take an additional template arg, you would do:
// #define DEFINE_FUNCTION(templatePre, templatePost, typeParams, valueParamsComma, valueParams, valueArgs)
//     template<typename T valueParamsComma typeParams> void f(T value valueParamsComma valueParams) { g(value, valueArgs); }
// DFG_VARIADIC_TEMPLATE_FUNCTION(DEFINE_FUNCTION)
// #undef DEFINE_FUNCTION

#define DFG_VARIADIC_TEMPLATE_FUNCTION(v) \
    v(, , , , , ) \
    v(template<, >, typename _DFG_T1, DFG_COMMA, const _DFG_T1& _DFG_value1, _DFG_value1) \
    v(template<, >, typename _DFG_T1 DFG_COMMA typename _DFG_T2, DFG_COMMA, const _DFG_T1& _DFG_value1 DFG_COMMA const _DFG_T2& _DFG_value2, _DFG_value1 DFG_COMMA _DFG_value2) \
    v(template<, >, typename _DFG_T1 DFG_COMMA typename _DFG_T2 DFG_COMMA typename _DFG_T3, DFG_COMMA, const _DFG_T1& _DFG_value1 DFG_COMMA const _DFG_T2& _DFG_value2 DFG_COMMA const _DFG_T3& _DFG_value3, _DFG_value1 DFG_COMMA _DFG_value2 DFG_COMMA _DFG_value3) \
    v(template<, >, typename _DFG_T1 DFG_COMMA typename _DFG_T2 DFG_COMMA typename _DFG_T3 DFG_COMMA typename _DFG_T4, DFG_COMMA, const _DFG_T1& _DFG_value1 DFG_COMMA const _DFG_T2& _DFG_value2 DFG_COMMA const _DFG_T3& _DFG_value3 DFG_COMMA const _DFG_T4& _DFG_value4, _DFG_value1 DFG_COMMA _DFG_value2 DFG_COMMA _DFG_value3 DFG_COMMA _DFG_value4) \
    v(template<, >, typename _DFG_T1 DFG_COMMA typename _DFG_T2 DFG_COMMA typename _DFG_T3 DFG_COMMA typename _DFG_T4 DFG_COMMA typename _DFG_T5, DFG_COMMA, const _DFG_T1& _DFG_value1 DFG_COMMA const _DFG_T2& _DFG_value2 DFG_COMMA const _DFG_T3& _DFG_value3 DFG_COMMA const _DFG_T4& _DFG_value4 DFG_COMMA const _DFG_T5& _DFG_value5, _DFG_value1 DFG_COMMA _DFG_value2 DFG_COMMA _DFG_value3 DFG_COMMA _DFG_value4 DFG_COMMA _DFG_value5) \
    v(template<, >, typename _DFG_T1 DFG_COMMA typename _DFG_T2 DFG_COMMA typename _DFG_T3 DFG_COMMA typename _DFG_T4 DFG_COMMA typename _DFG_T5 DFG_COMMA typename _DFG_T6, DFG_COMMA, const _DFG_T1& _DFG_value1 DFG_COMMA const _DFG_T2& _DFG_value2 DFG_COMMA const _DFG_T3& _DFG_value3 DFG_COMMA const _DFG_T4& _DFG_value4 DFG_COMMA const _DFG_T5& _DFG_value5 DFG_COMMA const _DFG_T6& _DFG_value6, _DFG_value1 DFG_COMMA _DFG_value2 DFG_COMMA _DFG_value3 DFG_COMMA _DFG_value4 DFG_COMMA _DFG_value5 DFG_COMMA _DFG_value6) \
    v(template<, >, typename _DFG_T1 DFG_COMMA typename _DFG_T2 DFG_COMMA typename _DFG_T3 DFG_COMMA typename _DFG_T4 DFG_COMMA typename _DFG_T5 DFG_COMMA typename _DFG_T6 DFG_COMMA typename _DFG_T7, DFG_COMMA, const _DFG_T1& _DFG_value1 DFG_COMMA const _DFG_T2& _DFG_value2 DFG_COMMA const _DFG_T3& _DFG_value3 DFG_COMMA const _DFG_T4& _DFG_value4 DFG_COMMA const _DFG_T5& _DFG_value5 DFG_COMMA const _DFG_T6& _DFG_value6 DFG_COMMA const _DFG_T7& _DFG_value7, _DFG_value1 DFG_COMMA _DFG_value2 DFG_COMMA _DFG_value3 DFG_COMMA _DFG_value4 DFG_COMMA _DFG_value5 DFG_COMMA _DFG_value6 DFG_COMMA _DFG_value7) \
    v(template<, >, typename _DFG_T1 DFG_COMMA typename _DFG_T2 DFG_COMMA typename _DFG_T3 DFG_COMMA typename _DFG_T4 DFG_COMMA typename _DFG_T5 DFG_COMMA typename _DFG_T6 DFG_COMMA typename _DFG_T7 DFG_COMMA typename _DFG_T8, DFG_COMMA, const _DFG_T1& _DFG_value1 DFG_COMMA const _DFG_T2& _DFG_value2 DFG_COMMA const _DFG_T3& _DFG_value3 DFG_COMMA const _DFG_T4& _DFG_value4 DFG_COMMA const _DFG_T5& _DFG_value5 DFG_COMMA const _DFG_T6& _DFG_value6 DFG_COMMA const _DFG_T7& _DFG_value7 DFG_COMMA _DFG_T8& _DFG_value8, _DFG_value1 DFG_COMMA _DFG_value2 DFG_COMMA _DFG_value3 DFG_COMMA _DFG_value4 DFG_COMMA _DFG_value5 DFG_COMMA _DFG_value6 DFG_COMMA _DFG_value7 DFG_COMMA _DFG_value8)

#endif // DFGVariadicFunction_h

