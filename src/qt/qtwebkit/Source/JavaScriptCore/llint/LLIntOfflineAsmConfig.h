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

#ifndef LLIntOfflineAsmConfig_h
#define LLIntOfflineAsmConfig_h

#include "LLIntCommon.h"
#include <wtf/Assertions.h>
#include <wtf/InlineASM.h>
#include <wtf/Platform.h>


#if ENABLE(LLINT_C_LOOP)
#define OFFLINE_ASM_C_LOOP 1
#define OFFLINE_ASM_X86 0
#define OFFLINE_ASM_ARM 0
#define OFFLINE_ASM_ARMv7 0
#define OFFLINE_ASM_ARMv7_TRADITIONAL 0
#define OFFLINE_ASM_X86_64 0
#define OFFLINE_ASM_ARMv7s 0
#define OFFLINE_ASM_MIPS 0
#define OFFLINE_ASM_SH4 0

#else // !ENABLE(LLINT_C_LOOP)

#define OFFLINE_ASM_C_LOOP 0

#if CPU(X86)
#define OFFLINE_ASM_X86 1
#else
#define OFFLINE_ASM_X86 0
#endif

#ifdef __ARM_ARCH_7S__
#define OFFLINE_ASM_ARMv7s 1
#else
#define OFFLINE_ASM_ARMv7s 0
#endif

#if CPU(ARM_THUMB2)
#define OFFLINE_ASM_ARMv7 1
#else
#define OFFLINE_ASM_ARMv7 0
#endif

#if CPU(ARM_TRADITIONAL)
#if WTF_ARM_ARCH_AT_LEAST(7)
#define OFFLINE_ASM_ARMv7_TRADITIONAL 1
#define OFFLINE_ASM_ARM 0
#else
#define OFFLINE_ASM_ARM 1
#define OFFLINE_ASM_ARMv7_TRADITIONAL 0
#endif
#else
#define OFFLINE_ASM_ARMv7_TRADITIONAL 0
#define OFFLINE_ASM_ARM 0
#endif

#if CPU(X86_64)
#define OFFLINE_ASM_X86_64 1
#else
#define OFFLINE_ASM_X86_64 0
#endif

#if CPU(MIPS)
#define OFFLINE_ASM_MIPS 1
#else
#define OFFLINE_ASM_MIPS 0
#endif

#if CPU(SH4)
#define OFFLINE_ASM_SH4 1
#else
#define OFFLINE_ASM_SH4 0
#endif

#endif // !ENABLE(LLINT_C_LOOP)

#if USE(JSVALUE64)
#define OFFLINE_ASM_JSVALUE64 1
#else
#define OFFLINE_ASM_JSVALUE64 0
#endif

#if !ASSERT_DISABLED
#define OFFLINE_ASM_ASSERT_ENABLED 1
#else
#define OFFLINE_ASM_ASSERT_ENABLED 0
#endif

#if CPU(BIG_ENDIAN)
#define OFFLINE_ASM_BIG_ENDIAN 1
#else
#define OFFLINE_ASM_BIG_ENDIAN 0
#endif

#if LLINT_OSR_TO_JIT
#define OFFLINE_ASM_JIT_ENABLED 1
#else
#define OFFLINE_ASM_JIT_ENABLED 0
#endif

#if LLINT_EXECUTION_TRACING
#define OFFLINE_ASM_EXECUTION_TRACING 1
#else
#define OFFLINE_ASM_EXECUTION_TRACING 0
#endif

#if LLINT_ALWAYS_ALLOCATE_SLOW
#define OFFLINE_ASM_ALWAYS_ALLOCATE_SLOW 1
#else
#define OFFLINE_ASM_ALWAYS_ALLOCATE_SLOW 0
#endif

#if ENABLE(VALUE_PROFILER)
#define OFFLINE_ASM_VALUE_PROFILER 1
#else
#define OFFLINE_ASM_VALUE_PROFILER 0
#endif

#if CPU(MIPS)
#ifdef WTF_MIPS_PIC
#define S(x) #x
#define SX(x) S(x)
#define OFFLINE_ASM_CPLOAD(reg) \
    ".set noreorder\n" \
    ".cpload " SX(reg) "\n" \
    ".set reorder\n"
#else
#define OFFLINE_ASM_CPLOAD(reg)
#endif
#endif

#endif // LLIntOfflineAsmConfig_h
