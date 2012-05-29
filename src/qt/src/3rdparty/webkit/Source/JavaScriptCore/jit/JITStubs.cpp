/*
 * Copyright (C) 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Cameron Zwarich <cwzwarich@uwaterloo.ca>
 * Copyright (C) Research In Motion Limited 2010, 2011. All rights reserved.
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

#if ENABLE(JIT)
#include "JITStubs.h"

#include "Arguments.h"
#include "CallFrame.h"
#include "CodeBlock.h"
#include "Heap.h"
#include "Debugger.h"
#include "ExceptionHelpers.h"
#include "GetterSetter.h"
#include "Strong.h"
#include "JIT.h"
#include "JSActivation.h"
#include "JSArray.h"
#include "JSByteArray.h"
#include "JSFunction.h"
#include "JSGlobalObjectFunctions.h"
#include "JSNotAnObject.h"
#include "JSPropertyNameIterator.h"
#include "JSStaticScopeObject.h"
#include "JSString.h"
#include "ObjectPrototype.h"
#include "Operations.h"
#include "Parser.h"
#include "Profiler.h"
#include "RegExpObject.h"
#include "RegExpPrototype.h"
#include "Register.h"
#include "SamplingTool.h"
#include <wtf/StdLibExtras.h>
#include <stdarg.h>
#include <stdio.h>

using namespace std;

namespace JSC {

#if OS(DARWIN) || (OS(WINDOWS) && CPU(X86))
#define SYMBOL_STRING(name) "_" #name
#else
#define SYMBOL_STRING(name) #name
#endif

#if OS(IOS)
#define THUMB_FUNC_PARAM(name) SYMBOL_STRING(name)
#else
#define THUMB_FUNC_PARAM(name)
#endif

#if (OS(LINUX) || OS(FREEBSD)) && CPU(X86_64)
#define SYMBOL_STRING_RELOCATION(name) #name "@plt"
#elif OS(DARWIN) || (CPU(X86_64) && COMPILER(MINGW) && !GCC_VERSION_AT_LEAST(4, 5, 0))
#define SYMBOL_STRING_RELOCATION(name) "_" #name
#elif CPU(X86) && COMPILER(MINGW)
#define SYMBOL_STRING_RELOCATION(name) "@" #name "@4"
#else
#define SYMBOL_STRING_RELOCATION(name) #name
#endif

#if OS(DARWIN)
    // Mach-O platform
#define HIDE_SYMBOL(name) ".private_extern _" #name
#elif OS(AIX)
    // IBM's own file format
#define HIDE_SYMBOL(name) ".lglobl " #name
#elif   OS(LINUX)               \
     || OS(FREEBSD)             \
     || OS(OPENBSD)             \
     || OS(SOLARIS)             \
     || (OS(HPUX) && CPU(IA64)) \
     || OS(SYMBIAN)             \
     || OS(NETBSD)
    // ELF platform
#define HIDE_SYMBOL(name) ".hidden " #name
#else
#define HIDE_SYMBOL(name)
#endif

#if USE(JSVALUE32_64)

#if COMPILER(GCC) && CPU(X86)

// These ASSERTs remind you that, if you change the layout of JITStackFrame, you
// need to change the assembly trampolines below to match.
COMPILE_ASSERT(offsetof(struct JITStackFrame, code) % 16 == 0x0, JITStackFrame_maintains_16byte_stack_alignment);
COMPILE_ASSERT(offsetof(struct JITStackFrame, savedEBX) == 0x3c, JITStackFrame_stub_argument_space_matches_ctiTrampoline);
COMPILE_ASSERT(offsetof(struct JITStackFrame, callFrame) == 0x58, JITStackFrame_callFrame_offset_matches_ctiTrampoline);
COMPILE_ASSERT(offsetof(struct JITStackFrame, code) == 0x50, JITStackFrame_code_offset_matches_ctiTrampoline);

asm (
".text\n"
".globl " SYMBOL_STRING(ctiTrampoline) "\n"
HIDE_SYMBOL(ctiTrampoline) "\n"
SYMBOL_STRING(ctiTrampoline) ":" "\n"
    "pushl %ebp" "\n"
    "movl %esp, %ebp" "\n"
    "pushl %esi" "\n"
    "pushl %edi" "\n"
    "pushl %ebx" "\n"
    "subl $0x3c, %esp" "\n"
    "movl $512, %esi" "\n"
    "movl 0x58(%esp), %edi" "\n"
    "call *0x50(%esp)" "\n"
    "addl $0x3c, %esp" "\n"
    "popl %ebx" "\n"
    "popl %edi" "\n"
    "popl %esi" "\n"
    "popl %ebp" "\n"
    "ret" "\n"
);

asm (
".globl " SYMBOL_STRING(ctiVMThrowTrampoline) "\n"
HIDE_SYMBOL(ctiVMThrowTrampoline) "\n"
SYMBOL_STRING(ctiVMThrowTrampoline) ":" "\n"
    "movl %esp, %ecx" "\n"
    "call " SYMBOL_STRING_RELOCATION(cti_vm_throw) "\n"
    "int3" "\n"
);
    
asm (
".globl " SYMBOL_STRING(ctiOpThrowNotCaught) "\n"
HIDE_SYMBOL(ctiOpThrowNotCaught) "\n"
SYMBOL_STRING(ctiOpThrowNotCaught) ":" "\n"
    "addl $0x3c, %esp" "\n"
    "popl %ebx" "\n"
    "popl %edi" "\n"
    "popl %esi" "\n"
    "popl %ebp" "\n"
    "ret" "\n"
);
    
#elif COMPILER(GCC) && CPU(X86_64)

// These ASSERTs remind you that, if you change the layout of JITStackFrame, you
// need to change the assembly trampolines below to match.
COMPILE_ASSERT(offsetof(struct JITStackFrame, code) % 32 == 0x0, JITStackFrame_maintains_32byte_stack_alignment);
COMPILE_ASSERT(offsetof(struct JITStackFrame, savedRBX) == 0x48, JITStackFrame_stub_argument_space_matches_ctiTrampoline);
COMPILE_ASSERT(offsetof(struct JITStackFrame, callFrame) == 0x90, JITStackFrame_callFrame_offset_matches_ctiTrampoline);
COMPILE_ASSERT(offsetof(struct JITStackFrame, code) == 0x80, JITStackFrame_code_offset_matches_ctiTrampoline);

asm (
".globl " SYMBOL_STRING(ctiTrampoline) "\n"
HIDE_SYMBOL(ctiTrampoline) "\n"
SYMBOL_STRING(ctiTrampoline) ":" "\n"
    "pushq %rbp" "\n"
    "movq %rsp, %rbp" "\n"
    "pushq %r12" "\n"
    "pushq %r13" "\n"
    "pushq %r14" "\n"
    "pushq %r15" "\n"
    "pushq %rbx" "\n"
    "subq $0x48, %rsp" "\n"
    "movq $512, %r12" "\n"
    "movq $0xFFFF000000000000, %r14" "\n"
    "movq $0xFFFF000000000002, %r15" "\n"
    "movq 0x90(%rsp), %r13" "\n"
    "call *0x80(%rsp)" "\n"
    "addq $0x48, %rsp" "\n"
    "popq %rbx" "\n"
    "popq %r15" "\n"
    "popq %r14" "\n"
    "popq %r13" "\n"
    "popq %r12" "\n"
    "popq %rbp" "\n"
    "ret" "\n"
);

asm (
".globl " SYMBOL_STRING(ctiVMThrowTrampoline) "\n"
HIDE_SYMBOL(ctiVMThrowTrampoline) "\n"
SYMBOL_STRING(ctiVMThrowTrampoline) ":" "\n"
    "movq %rsp, %rdi" "\n"
    "call " SYMBOL_STRING_RELOCATION(cti_vm_throw) "\n"
    "int3" "\n"
);

asm (
".globl " SYMBOL_STRING(ctiOpThrowNotCaught) "\n"
HIDE_SYMBOL(ctiOpThrowNotCaught) "\n"
SYMBOL_STRING(ctiOpThrowNotCaught) ":" "\n"
    "addq $0x48, %rsp" "\n"
    "popq %rbx" "\n"
    "popq %r15" "\n"
    "popq %r14" "\n"
    "popq %r13" "\n"
    "popq %r12" "\n"
    "popq %rbp" "\n"
    "ret" "\n"
);

#elif (COMPILER(GCC) || COMPILER(RVCT)) && CPU(ARM_THUMB2)

#define THUNK_RETURN_ADDRESS_OFFSET      0x38
#define PRESERVED_RETURN_ADDRESS_OFFSET  0x3C
#define PRESERVED_R4_OFFSET              0x40
#define PRESERVED_R5_OFFSET              0x44
#define PRESERVED_R6_OFFSET              0x48
#define REGISTER_FILE_OFFSET             0x4C
#define CALLFRAME_OFFSET                 0x50
#define EXCEPTION_OFFSET                 0x54
#define ENABLE_PROFILER_REFERENCE_OFFSET 0x58

#elif (COMPILER(GCC) || COMPILER(MSVC) || COMPILER(RVCT)) && CPU(ARM_TRADITIONAL)

// Also update the MSVC section (defined at DEFINE_STUB_FUNCTION)
// when changing one of the following values.
#define THUNK_RETURN_ADDRESS_OFFSET 64
#define PRESERVEDR4_OFFSET          68

#elif COMPILER(MSVC) && CPU(X86)

// These ASSERTs remind you that, if you change the layout of JITStackFrame, you
// need to change the assembly trampolines below to match.
COMPILE_ASSERT(offsetof(struct JITStackFrame, code) % 16 == 0x0, JITStackFrame_maintains_16byte_stack_alignment);
COMPILE_ASSERT(offsetof(struct JITStackFrame, savedEBX) == 0x3c, JITStackFrame_stub_argument_space_matches_ctiTrampoline);
COMPILE_ASSERT(offsetof(struct JITStackFrame, callFrame) == 0x58, JITStackFrame_callFrame_offset_matches_ctiTrampoline);
COMPILE_ASSERT(offsetof(struct JITStackFrame, code) == 0x50, JITStackFrame_code_offset_matches_ctiTrampoline);

extern "C" {

    __declspec(naked) EncodedJSValue ctiTrampoline(void* code, RegisterFile*, CallFrame*, void* /*unused1*/, Profiler**, JSGlobalData*)
    {
        __asm {
            push ebp;
            mov ebp, esp;
            push esi;
            push edi;
            push ebx;
            sub esp, 0x3c;
            mov esi, 512;
            mov ecx, esp;
            mov edi, [esp + 0x58];
            call [esp + 0x50];
            add esp, 0x3c;
            pop ebx;
            pop edi;
            pop esi;
            pop ebp;
            ret;
        }
    }

    __declspec(naked) void ctiVMThrowTrampoline()
    {
        __asm {
            mov ecx, esp;
            call cti_vm_throw;
            add esp, 0x3c;
            pop ebx;
            pop edi;
            pop esi;
            pop ebp;
            ret;
        }
    }

    __declspec(naked) void ctiOpThrowNotCaught()
    {
        __asm {
            add esp, 0x3c;
            pop ebx;
            pop edi;
            pop esi;
            pop ebp;
            ret;
        }
    }
}

#elif CPU(MIPS)

#define PRESERVED_GP_OFFSET         60
#define PRESERVED_S0_OFFSET         64
#define PRESERVED_S1_OFFSET         68
#define PRESERVED_S2_OFFSET         72
#define PRESERVED_RETURN_ADDRESS_OFFSET 76
#define THUNK_RETURN_ADDRESS_OFFSET 80
#define REGISTER_FILE_OFFSET        84
#define CALLFRAME_OFFSET            88
#define EXCEPTION_OFFSET            92
#define ENABLE_PROFILER_REFERENCE_OFFSET 96
#define GLOBAL_DATA_OFFSET         100
#define STACK_LENGTH               104
#elif CPU(SH4)
#define SYMBOL_STRING(name) #name
/* code (r4), RegisterFile* (r5), CallFrame* (r6), JSValue* exception (r7), Profiler**(sp), JSGlobalData (sp)*/

asm volatile (
".text\n"
".globl " SYMBOL_STRING(ctiTrampoline) "\n"
HIDE_SYMBOL(ctiTrampoline) "\n"
SYMBOL_STRING(ctiTrampoline) ":" "\n"
    "mov.l r7, @-r15" "\n"
    "mov.l r6, @-r15" "\n"
    "mov.l r5, @-r15" "\n"
    "mov.l r8, @-r15" "\n"
    "mov #127, r8" "\n"
    "mov.l r14, @-r15" "\n"
    "sts.l pr, @-r15" "\n"
    "mov.l r13, @-r15" "\n"
    "mov.l r11, @-r15" "\n"
    "mov.l r10, @-r15" "\n"
    "add #-60, r15" "\n"
    "mov r6, r14" "\n"
    "jsr @r4" "\n"
    "nop" "\n"
    "add #60, r15" "\n"
    "mov.l @r15+,r10" "\n"
    "mov.l @r15+,r11" "\n"
    "mov.l @r15+,r13" "\n"
    "lds.l @r15+,pr" "\n"
    "mov.l @r15+,r14" "\n"
    "mov.l @r15+,r8" "\n"
    "add #12, r15" "\n"
    "rts" "\n"
    "nop" "\n"
);

asm volatile (
".globl " SYMBOL_STRING(ctiVMThrowTrampoline) "\n"
HIDE_SYMBOL(ctiVMThrowTrampoline) "\n"
SYMBOL_STRING(ctiVMThrowTrampoline) ":" "\n"
    "mov.l .L2" SYMBOL_STRING(cti_vm_throw) ",r0" "\n"
    "mov r15, r4" "\n"
    "mov.l @(r0,r12),r11" "\n"
    "jsr @r11" "\n"
    "nop" "\n"
    "add #60, r15" "\n"
    "mov.l @r15+,r10" "\n"
    "mov.l @r15+,r11" "\n"
    "mov.l @r15+,r13" "\n"
    "lds.l @r15+,pr" "\n"
    "mov.l @r15+,r14" "\n"
    "mov.l @r15+,r8" "\n"
    "add #12, r15" "\n"
    "rts" "\n"
    "nop" "\n"
    ".align 2" "\n"
    ".L2" SYMBOL_STRING(cti_vm_throw)":.long " SYMBOL_STRING(cti_vm_throw) "@GOT \n"
);

asm volatile (
".globl " SYMBOL_STRING(ctiOpThrowNotCaught) "\n"
HIDE_SYMBOL(ctiOpThrowNotCaught) "\n"
SYMBOL_STRING(ctiOpThrowNotCaught) ":" "\n"
    "add #60, r15" "\n"
    "mov.l @r15+,r10" "\n"
    "mov.l @r15+,r11" "\n"
    "mov.l @r15+,r13" "\n"
    "lds.l @r15+,pr" "\n"
    "mov.l @r15+,r14" "\n"
    "mov.l @r15+,r8" "\n"
    "add #12, r15" "\n"
    "rts" "\n"
    "nop" "\n"
);
#else
    #error "JIT not supported on this platform."
#endif

#else // USE(JSVALUE32_64)

#if COMPILER(GCC) && CPU(X86_64)

// These ASSERTs remind you that, if you change the layout of JITStackFrame, you
// need to change the assembly trampolines below to match.
COMPILE_ASSERT(offsetof(struct JITStackFrame, callFrame) == 0x58, JITStackFrame_callFrame_offset_matches_ctiTrampoline);
COMPILE_ASSERT(offsetof(struct JITStackFrame, code) == 0x48, JITStackFrame_code_offset_matches_ctiTrampoline);
COMPILE_ASSERT(offsetof(struct JITStackFrame, savedRBX) == 0x78, JITStackFrame_stub_argument_space_matches_ctiTrampoline);

asm (
".text\n"
".globl " SYMBOL_STRING(ctiTrampoline) "\n"
HIDE_SYMBOL(ctiTrampoline) "\n"
SYMBOL_STRING(ctiTrampoline) ":" "\n"
    "pushq %rbp" "\n"
    "movq %rsp, %rbp" "\n"
    "pushq %r12" "\n"
    "pushq %r13" "\n"
    "pushq %r14" "\n"
    "pushq %r15" "\n"
    "pushq %rbx" "\n"
    // Form the JIT stubs area
    "pushq %r9" "\n"
    "pushq %r8" "\n"
    "pushq %rcx" "\n"
    "pushq %rdx" "\n"
    "pushq %rsi" "\n"
    "pushq %rdi" "\n"
    "subq $0x48, %rsp" "\n"
    "movq $512, %r12" "\n"
    "movq $0xFFFF000000000000, %r14" "\n"
    "movq $0xFFFF000000000002, %r15" "\n"
    "movq %rdx, %r13" "\n"
    "call *%rdi" "\n"
    "addq $0x78, %rsp" "\n"
    "popq %rbx" "\n"
    "popq %r15" "\n"
    "popq %r14" "\n"
    "popq %r13" "\n"
    "popq %r12" "\n"
    "popq %rbp" "\n"
    "ret" "\n"
);

asm (
".globl " SYMBOL_STRING(ctiVMThrowTrampoline) "\n"
HIDE_SYMBOL(ctiVMThrowTrampoline) "\n"
SYMBOL_STRING(ctiVMThrowTrampoline) ":" "\n"
    "movq %rsp, %rdi" "\n"
    "call " SYMBOL_STRING_RELOCATION(cti_vm_throw) "\n"
    "int3" "\n"
);

asm (
".globl " SYMBOL_STRING(ctiOpThrowNotCaught) "\n"
HIDE_SYMBOL(ctiOpThrowNotCaught) "\n"
SYMBOL_STRING(ctiOpThrowNotCaught) ":" "\n"
    "addq $0x78, %rsp" "\n"
    "popq %rbx" "\n"
    "popq %r15" "\n"
    "popq %r14" "\n"
    "popq %r13" "\n"
    "popq %r12" "\n"
    "popq %rbp" "\n"
    "ret" "\n"
);

#else
    #error "JIT not supported on this platform."
#endif

#endif // USE(JSVALUE32_64)

#if CPU(MIPS)
asm (
".text" "\n"
".align 2" "\n"
".set noreorder" "\n"
".set nomacro" "\n"
".set nomips16" "\n"
".globl " SYMBOL_STRING(ctiTrampoline) "\n"
".ent " SYMBOL_STRING(ctiTrampoline) "\n"
SYMBOL_STRING(ctiTrampoline) ":" "\n"
    "addiu $29,$29,-" STRINGIZE_VALUE_OF(STACK_LENGTH) "\n"
    "sw    $31," STRINGIZE_VALUE_OF(PRESERVED_RETURN_ADDRESS_OFFSET) "($29)" "\n"
    "sw    $18," STRINGIZE_VALUE_OF(PRESERVED_S2_OFFSET) "($29)" "\n"
    "sw    $17," STRINGIZE_VALUE_OF(PRESERVED_S1_OFFSET) "($29)" "\n"
    "sw    $16," STRINGIZE_VALUE_OF(PRESERVED_S0_OFFSET) "($29)" "\n"
#if WTF_MIPS_PIC
    "sw    $28," STRINGIZE_VALUE_OF(PRESERVED_GP_OFFSET) "($29)" "\n"
#endif
    "move  $16,$6       # set callFrameRegister" "\n"
    "li    $17,512      # set timeoutCheckRegister" "\n"
    "move  $25,$4       # move executableAddress to t9" "\n"
    "sw    $5," STRINGIZE_VALUE_OF(REGISTER_FILE_OFFSET) "($29) # store registerFile to current stack" "\n"
    "sw    $6," STRINGIZE_VALUE_OF(CALLFRAME_OFFSET) "($29)     # store callFrame to curent stack" "\n"
    "sw    $7," STRINGIZE_VALUE_OF(EXCEPTION_OFFSET) "($29)     # store exception to current stack" "\n"
    "lw    $8," STRINGIZE_VALUE_OF(STACK_LENGTH + 16) "($29)    # load enableProfilerReference from previous stack" "\n"
    "lw    $9," STRINGIZE_VALUE_OF(STACK_LENGTH + 20) "($29)    # load globalData from previous stack" "\n"
    "sw    $8," STRINGIZE_VALUE_OF(ENABLE_PROFILER_REFERENCE_OFFSET) "($29)   # store enableProfilerReference to current stack" "\n"
    "jalr  $25" "\n"
    "sw    $9," STRINGIZE_VALUE_OF(GLOBAL_DATA_OFFSET) "($29)   # store globalData to current stack" "\n"
    "lw    $16," STRINGIZE_VALUE_OF(PRESERVED_S0_OFFSET) "($29)" "\n"
    "lw    $17," STRINGIZE_VALUE_OF(PRESERVED_S1_OFFSET) "($29)" "\n"
    "lw    $18," STRINGIZE_VALUE_OF(PRESERVED_S2_OFFSET) "($29)" "\n"
    "lw    $31," STRINGIZE_VALUE_OF(PRESERVED_RETURN_ADDRESS_OFFSET) "($29)" "\n"
    "jr    $31" "\n"
    "addiu $29,$29," STRINGIZE_VALUE_OF(STACK_LENGTH) "\n"
".set reorder" "\n"
".set macro" "\n"
".end " SYMBOL_STRING(ctiTrampoline) "\n"
);

asm (
".text" "\n"
".align 2" "\n"
".set noreorder" "\n"
".set nomacro" "\n"
".set nomips16" "\n"
".globl " SYMBOL_STRING(ctiVMThrowTrampoline) "\n"
".ent " SYMBOL_STRING(ctiVMThrowTrampoline) "\n"
SYMBOL_STRING(ctiVMThrowTrampoline) ":" "\n"
#if WTF_MIPS_PIC
    "lw    $28," STRINGIZE_VALUE_OF(PRESERVED_GP_OFFSET) "($29)" "\n"
".set macro" "\n"
    "la    $25," SYMBOL_STRING(cti_vm_throw) "\n"
".set nomacro" "\n"
    "bal " SYMBOL_STRING(cti_vm_throw) "\n"
    "move  $4,$29" "\n"
#else
    "jal " SYMBOL_STRING(cti_vm_throw) "\n"
    "move  $4,$29" "\n"
#endif
    "lw    $16," STRINGIZE_VALUE_OF(PRESERVED_S0_OFFSET) "($29)" "\n"
    "lw    $17," STRINGIZE_VALUE_OF(PRESERVED_S1_OFFSET) "($29)" "\n"
    "lw    $18," STRINGIZE_VALUE_OF(PRESERVED_S2_OFFSET) "($29)" "\n"
    "lw    $31," STRINGIZE_VALUE_OF(PRESERVED_RETURN_ADDRESS_OFFSET) "($29)" "\n"
    "jr    $31" "\n"
    "addiu $29,$29," STRINGIZE_VALUE_OF(STACK_LENGTH) "\n"
".set reorder" "\n"
".set macro" "\n"
".end " SYMBOL_STRING(ctiVMThrowTrampoline) "\n"
);

asm (
".text" "\n"
".align 2" "\n"
".set noreorder" "\n"
".set nomacro" "\n"
".set nomips16" "\n"
".globl " SYMBOL_STRING(ctiOpThrowNotCaught) "\n"
".ent " SYMBOL_STRING(ctiOpThrowNotCaught) "\n"
SYMBOL_STRING(ctiOpThrowNotCaught) ":" "\n"
    "lw    $16," STRINGIZE_VALUE_OF(PRESERVED_S0_OFFSET) "($29)" "\n"
    "lw    $17," STRINGIZE_VALUE_OF(PRESERVED_S1_OFFSET) "($29)" "\n"
    "lw    $18," STRINGIZE_VALUE_OF(PRESERVED_S2_OFFSET) "($29)" "\n"
    "lw    $31," STRINGIZE_VALUE_OF(PRESERVED_RETURN_ADDRESS_OFFSET) "($29)" "\n"
    "jr    $31" "\n"
    "addiu $29,$29," STRINGIZE_VALUE_OF(STACK_LENGTH) "\n"
".set reorder" "\n"
".set macro" "\n"
".end " SYMBOL_STRING(ctiOpThrowNotCaught) "\n"
);
#endif

#if COMPILER(GCC) && CPU(ARM_THUMB2)

asm (
".text" "\n"
".align 2" "\n"
".globl " SYMBOL_STRING(ctiTrampoline) "\n"
HIDE_SYMBOL(ctiTrampoline) "\n"
".thumb" "\n"
".thumb_func " THUMB_FUNC_PARAM(ctiTrampoline) "\n"
SYMBOL_STRING(ctiTrampoline) ":" "\n"
    "sub sp, sp, #" STRINGIZE_VALUE_OF(ENABLE_PROFILER_REFERENCE_OFFSET) "\n"
    "str lr, [sp, #" STRINGIZE_VALUE_OF(PRESERVED_RETURN_ADDRESS_OFFSET) "]" "\n"
    "str r4, [sp, #" STRINGIZE_VALUE_OF(PRESERVED_R4_OFFSET) "]" "\n"
    "str r5, [sp, #" STRINGIZE_VALUE_OF(PRESERVED_R5_OFFSET) "]" "\n"
    "str r6, [sp, #" STRINGIZE_VALUE_OF(PRESERVED_R6_OFFSET) "]" "\n"
    "str r1, [sp, #" STRINGIZE_VALUE_OF(REGISTER_FILE_OFFSET) "]" "\n"
    "str r2, [sp, #" STRINGIZE_VALUE_OF(CALLFRAME_OFFSET) "]" "\n"
    "str r3, [sp, #" STRINGIZE_VALUE_OF(EXCEPTION_OFFSET) "]" "\n"
    "cpy r5, r2" "\n"
    "mov r6, #512" "\n"
    "blx r0" "\n"
    "ldr r6, [sp, #" STRINGIZE_VALUE_OF(PRESERVED_R6_OFFSET) "]" "\n"
    "ldr r5, [sp, #" STRINGIZE_VALUE_OF(PRESERVED_R5_OFFSET) "]" "\n"
    "ldr r4, [sp, #" STRINGIZE_VALUE_OF(PRESERVED_R4_OFFSET) "]" "\n"
    "ldr lr, [sp, #" STRINGIZE_VALUE_OF(PRESERVED_RETURN_ADDRESS_OFFSET) "]" "\n"
    "add sp, sp, #" STRINGIZE_VALUE_OF(ENABLE_PROFILER_REFERENCE_OFFSET) "\n"
    "bx lr" "\n"
);

asm (
".text" "\n"
".align 2" "\n"
".globl " SYMBOL_STRING(ctiVMThrowTrampoline) "\n"
HIDE_SYMBOL(ctiVMThrowTrampoline) "\n"
".thumb" "\n"
".thumb_func " THUMB_FUNC_PARAM(ctiVMThrowTrampoline) "\n"
SYMBOL_STRING(ctiVMThrowTrampoline) ":" "\n"
    "cpy r0, sp" "\n"
    "bl " SYMBOL_STRING_RELOCATION(cti_vm_throw) "\n"
    "ldr r6, [sp, #" STRINGIZE_VALUE_OF(PRESERVED_R6_OFFSET) "]" "\n"
    "ldr r5, [sp, #" STRINGIZE_VALUE_OF(PRESERVED_R5_OFFSET) "]" "\n"
    "ldr r4, [sp, #" STRINGIZE_VALUE_OF(PRESERVED_R4_OFFSET) "]" "\n"
    "ldr lr, [sp, #" STRINGIZE_VALUE_OF(PRESERVED_RETURN_ADDRESS_OFFSET) "]" "\n"
    "add sp, sp, #" STRINGIZE_VALUE_OF(ENABLE_PROFILER_REFERENCE_OFFSET) "\n"
    "bx lr" "\n"
);

asm (
".text" "\n"
".align 2" "\n"
".globl " SYMBOL_STRING(ctiOpThrowNotCaught) "\n"
HIDE_SYMBOL(ctiOpThrowNotCaught) "\n"
".thumb" "\n"
".thumb_func " THUMB_FUNC_PARAM(ctiOpThrowNotCaught) "\n"
SYMBOL_STRING(ctiOpThrowNotCaught) ":" "\n"
    "ldr r6, [sp, #" STRINGIZE_VALUE_OF(PRESERVED_R6_OFFSET) "]" "\n"
    "ldr r5, [sp, #" STRINGIZE_VALUE_OF(PRESERVED_R5_OFFSET) "]" "\n"
    "ldr r4, [sp, #" STRINGIZE_VALUE_OF(PRESERVED_R4_OFFSET) "]" "\n"
    "ldr lr, [sp, #" STRINGIZE_VALUE_OF(PRESERVED_RETURN_ADDRESS_OFFSET) "]" "\n"
    "add sp, sp, #" STRINGIZE_VALUE_OF(ENABLE_PROFILER_REFERENCE_OFFSET) "\n"
    "bx lr" "\n"
);

#elif COMPILER(GCC) && CPU(ARM_TRADITIONAL)

asm (
".globl " SYMBOL_STRING(ctiTrampoline) "\n"
HIDE_SYMBOL(ctiTrampoline) "\n"
SYMBOL_STRING(ctiTrampoline) ":" "\n"
    "stmdb sp!, {r1-r3}" "\n"
    "stmdb sp!, {r4-r8, lr}" "\n"
    "sub sp, sp, #" STRINGIZE_VALUE_OF(PRESERVEDR4_OFFSET) "\n"
    "mov r4, r2" "\n"
    "mov r5, #512" "\n"
    // r0 contains the code
    "mov lr, pc" "\n"
    "mov pc, r0" "\n"
    "add sp, sp, #" STRINGIZE_VALUE_OF(PRESERVEDR4_OFFSET) "\n"
    "ldmia sp!, {r4-r8, lr}" "\n"
    "add sp, sp, #12" "\n"
    "mov pc, lr" "\n"
);

asm (
".globl " SYMBOL_STRING(ctiVMThrowTrampoline) "\n"
HIDE_SYMBOL(ctiVMThrowTrampoline) "\n"
SYMBOL_STRING(ctiVMThrowTrampoline) ":" "\n"
    "mov r0, sp" "\n"
    "bl " SYMBOL_STRING(cti_vm_throw) "\n"

// Both has the same return sequence
".globl " SYMBOL_STRING(ctiOpThrowNotCaught) "\n"
HIDE_SYMBOL(ctiOpThrowNotCaught) "\n"
SYMBOL_STRING(ctiOpThrowNotCaught) ":" "\n"
    "add sp, sp, #" STRINGIZE_VALUE_OF(PRESERVEDR4_OFFSET) "\n"
    "ldmia sp!, {r4-r8, lr}" "\n"
    "add sp, sp, #12" "\n"
    "mov pc, lr" "\n"
);

#elif COMPILER(RVCT) && CPU(ARM_THUMB2)

__asm EncodedJSValue ctiTrampoline(void*, RegisterFile*, CallFrame*, JSValue*, Profiler**, JSGlobalData*)
{
    PRESERVE8
    sub sp, sp, # ENABLE_PROFILER_REFERENCE_OFFSET
    str lr, [sp, # PRESERVED_RETURN_ADDRESS_OFFSET ]
    str r4, [sp, # PRESERVED_R4_OFFSET ]
    str r5, [sp, # PRESERVED_R5_OFFSET ]
    str r6, [sp, # PRESERVED_R6_OFFSET ]
    str r1, [sp, # REGISTER_FILE_OFFSET ]
    str r2, [sp, # CALLFRAME_OFFSET ]
    str r3, [sp, # EXCEPTION_OFFSET ]
    cpy r5, r2
    mov r6, #512
    blx r0
    ldr r6, [sp, # PRESERVED_R6_OFFSET ]
    ldr r5, [sp, # PRESERVED_R5_OFFSET ]
    ldr r4, [sp, # PRESERVED_R4_OFFSET ]
    ldr lr, [sp, # PRESERVED_RETURN_ADDRESS_OFFSET ]
    add sp, sp, # ENABLE_PROFILER_REFERENCE_OFFSET
    bx lr
}

__asm void ctiVMThrowTrampoline()
{
    PRESERVE8
    cpy r0, sp
    bl cti_vm_throw
    ldr r6, [sp, # PRESERVED_R6_OFFSET ]
    ldr r5, [sp, # PRESERVED_R5_OFFSET ]
    ldr r4, [sp, # PRESERVED_R4_OFFSET ]
    ldr lr, [sp, # PRESERVED_RETURN_ADDRESS_OFFSET ]
    add sp, sp, # ENABLE_PROFILER_REFERENCE_OFFSET
    bx lr
}

__asm void ctiOpThrowNotCaught()
{
    PRESERVE8
    ldr r6, [sp, # PRESERVED_R6_OFFSET ]
    ldr r5, [sp, # PRESERVED_R5_OFFSET ]
    ldr r4, [sp, # PRESERVED_R4_OFFSET ]
    ldr lr, [sp, # PRESERVED_RETURN_ADDRESS_OFFSET ]
    add sp, sp, # ENABLE_PROFILER_REFERENCE_OFFSET
    bx lr
}

#elif COMPILER(RVCT) && CPU(ARM_TRADITIONAL)

__asm EncodedJSValue ctiTrampoline(void*, RegisterFile*, CallFrame*, void* /*unused1*/, Profiler**, JSGlobalData*)
{
    ARM
    stmdb sp!, {r1-r3}
    stmdb sp!, {r4-r8, lr}
    sub sp, sp, # PRESERVEDR4_OFFSET
    mov r4, r2
    mov r5, #512
    mov lr, pc
    bx r0
    add sp, sp, # PRESERVEDR4_OFFSET
    ldmia sp!, {r4-r8, lr}
    add sp, sp, #12
    bx lr
}

__asm void ctiVMThrowTrampoline()
{
    ARM
    PRESERVE8
    mov r0, sp
    bl cti_vm_throw
    add sp, sp, # PRESERVEDR4_OFFSET
    ldmia sp!, {r4-r8, lr}
    add sp, sp, #12
    bx lr
}

__asm void ctiOpThrowNotCaught()
{
    ARM
    add sp, sp, # PRESERVEDR4_OFFSET
    ldmia sp!, {r4-r8, lr}
    add sp, sp, #12
    bx lr
}
#endif

#if ENABLE(OPCODE_SAMPLING)
    #define CTI_SAMPLER stackFrame.globalData->interpreter->sampler()
#else
    #define CTI_SAMPLER 0
#endif

JITThunks::JITThunks(JSGlobalData* globalData)
    : m_hostFunctionStubMap(adoptPtr(new HostFunctionStubMap))
{
    if (!globalData->executableAllocator.isValid())
        return;

    JIT::compileCTIMachineTrampolines(globalData, &m_executablePool, &m_trampolineStructure);
    ASSERT(m_executablePool);
#if CPU(ARM_THUMB2)
    // Unfortunate the arm compiler does not like the use of offsetof on JITStackFrame (since it contains non POD types),
    // and the OBJECT_OFFSETOF macro does not appear constantish enough for it to be happy with its use in COMPILE_ASSERT
    // macros.
    ASSERT(OBJECT_OFFSETOF(struct JITStackFrame, preservedReturnAddress) == PRESERVED_RETURN_ADDRESS_OFFSET);
    ASSERT(OBJECT_OFFSETOF(struct JITStackFrame, preservedR4) == PRESERVED_R4_OFFSET);
    ASSERT(OBJECT_OFFSETOF(struct JITStackFrame, preservedR5) == PRESERVED_R5_OFFSET);
    ASSERT(OBJECT_OFFSETOF(struct JITStackFrame, preservedR6) == PRESERVED_R6_OFFSET);

    ASSERT(OBJECT_OFFSETOF(struct JITStackFrame, registerFile) == REGISTER_FILE_OFFSET);
    ASSERT(OBJECT_OFFSETOF(struct JITStackFrame, callFrame) == CALLFRAME_OFFSET);
    // The fifth argument is the first item already on the stack.
    ASSERT(OBJECT_OFFSETOF(struct JITStackFrame, enabledProfilerReference) == ENABLE_PROFILER_REFERENCE_OFFSET);

    ASSERT(OBJECT_OFFSETOF(struct JITStackFrame, thunkReturnAddress) == THUNK_RETURN_ADDRESS_OFFSET);

#elif CPU(ARM_TRADITIONAL)

    ASSERT(OBJECT_OFFSETOF(struct JITStackFrame, thunkReturnAddress) == THUNK_RETURN_ADDRESS_OFFSET);
    ASSERT(OBJECT_OFFSETOF(struct JITStackFrame, preservedR4) == PRESERVEDR4_OFFSET);


#elif CPU(MIPS)
    ASSERT(OBJECT_OFFSETOF(struct JITStackFrame, preservedGP) == PRESERVED_GP_OFFSET);
    ASSERT(OBJECT_OFFSETOF(struct JITStackFrame, preservedS0) == PRESERVED_S0_OFFSET);
    ASSERT(OBJECT_OFFSETOF(struct JITStackFrame, preservedS1) == PRESERVED_S1_OFFSET);
    ASSERT(OBJECT_OFFSETOF(struct JITStackFrame, preservedS2) == PRESERVED_S2_OFFSET);
    ASSERT(OBJECT_OFFSETOF(struct JITStackFrame, preservedReturnAddress) == PRESERVED_RETURN_ADDRESS_OFFSET);
    ASSERT(OBJECT_OFFSETOF(struct JITStackFrame, thunkReturnAddress) == THUNK_RETURN_ADDRESS_OFFSET);
    ASSERT(OBJECT_OFFSETOF(struct JITStackFrame, registerFile) == REGISTER_FILE_OFFSET);
    ASSERT(OBJECT_OFFSETOF(struct JITStackFrame, callFrame) == CALLFRAME_OFFSET);
    ASSERT(OBJECT_OFFSETOF(struct JITStackFrame, unused1) == EXCEPTION_OFFSET);
    ASSERT(OBJECT_OFFSETOF(struct JITStackFrame, enabledProfilerReference) == ENABLE_PROFILER_REFERENCE_OFFSET);
    ASSERT(OBJECT_OFFSETOF(struct JITStackFrame, globalData) == GLOBAL_DATA_OFFSET);

#endif
}

JITThunks::~JITThunks()
{
}

#if ENABLE(JIT_OPTIMIZE_PROPERTY_ACCESS)

NEVER_INLINE void JITThunks::tryCachePutByID(CallFrame* callFrame, CodeBlock* codeBlock, ReturnAddressPtr returnAddress, JSValue baseValue, const PutPropertySlot& slot, StructureStubInfo* stubInfo, bool direct)
{
    // The interpreter checks for recursion here; I do not believe this can occur in CTI.

    if (!baseValue.isCell())
        return;

    // Uncacheable: give up.
    if (!slot.isCacheable()) {
        ctiPatchCallByReturnAddress(codeBlock, returnAddress, FunctionPtr(direct ? cti_op_put_by_id_direct_generic : cti_op_put_by_id_generic));
        return;
    }
    
    JSCell* baseCell = baseValue.asCell();
    Structure* structure = baseCell->structure();

    if (structure->isUncacheableDictionary()) {
        ctiPatchCallByReturnAddress(codeBlock, returnAddress, FunctionPtr(direct ? cti_op_put_by_id_direct_generic : cti_op_put_by_id_generic));
        return;
    }

    // If baseCell != base, then baseCell must be a proxy for another object.
    if (baseCell != slot.base()) {
        ctiPatchCallByReturnAddress(codeBlock, returnAddress, FunctionPtr(direct ? cti_op_put_by_id_direct_generic : cti_op_put_by_id_generic));
        return;
    }

    // Cache hit: Specialize instruction and ref Structures.

    // Structure transition, cache transition info
    if (slot.type() == PutPropertySlot::NewProperty) {
        if (structure->isDictionary()) {
            ctiPatchCallByReturnAddress(codeBlock, returnAddress, FunctionPtr(direct ? cti_op_put_by_id_direct_generic : cti_op_put_by_id_generic));
            return;
        }

        // put_by_id_transition checks the prototype chain for setters.
        normalizePrototypeChain(callFrame, baseCell);

        StructureChain* prototypeChain = structure->prototypeChain(callFrame);
        stubInfo->initPutByIdTransition(callFrame->globalData(), codeBlock->ownerExecutable(), structure->previousID(), structure, prototypeChain);
        JIT::compilePutByIdTransition(callFrame->scopeChain()->globalData, codeBlock, stubInfo, structure->previousID(), structure, slot.cachedOffset(), prototypeChain, returnAddress, direct);
        return;
    }
    
    stubInfo->initPutByIdReplace(callFrame->globalData(), codeBlock->ownerExecutable(), structure);

    JIT::patchPutByIdReplace(codeBlock, stubInfo, structure, slot.cachedOffset(), returnAddress, direct);
}

NEVER_INLINE void JITThunks::tryCacheGetByID(CallFrame* callFrame, CodeBlock* codeBlock, ReturnAddressPtr returnAddress, JSValue baseValue, const Identifier& propertyName, const PropertySlot& slot, StructureStubInfo* stubInfo)
{
    // FIXME: Write a test that proves we need to check for recursion here just
    // like the interpreter does, then add a check for recursion.

    // FIXME: Cache property access for immediates.
    if (!baseValue.isCell()) {
        ctiPatchCallByReturnAddress(codeBlock, returnAddress, FunctionPtr(cti_op_get_by_id_generic));
        return;
    }
    
    JSGlobalData* globalData = &callFrame->globalData();

    if (isJSArray(globalData, baseValue) && propertyName == callFrame->propertyNames().length) {
        JIT::compilePatchGetArrayLength(callFrame->scopeChain()->globalData, codeBlock, returnAddress);
        return;
    }
    
    if (isJSString(globalData, baseValue) && propertyName == callFrame->propertyNames().length) {
        // The tradeoff of compiling an patched inline string length access routine does not seem
        // to pay off, so we currently only do this for arrays.
        ctiPatchCallByReturnAddress(codeBlock, returnAddress, globalData->jitStubs->ctiStringLengthTrampoline());
        return;
    }

    // Uncacheable: give up.
    if (!slot.isCacheable()) {
        ctiPatchCallByReturnAddress(codeBlock, returnAddress, FunctionPtr(cti_op_get_by_id_generic));
        return;
    }

    JSCell* baseCell = baseValue.asCell();
    Structure* structure = baseCell->structure();

    if (structure->isUncacheableDictionary()) {
        ctiPatchCallByReturnAddress(codeBlock, returnAddress, FunctionPtr(cti_op_get_by_id_generic));
        return;
    }

    // Cache hit: Specialize instruction and ref Structures.

    if (slot.slotBase() == baseValue) {
        // set this up, so derefStructures can do it's job.
        stubInfo->initGetByIdSelf(callFrame->globalData(), codeBlock->ownerExecutable(), structure);
        if (slot.cachedPropertyType() != PropertySlot::Value)
            ctiPatchCallByReturnAddress(codeBlock, returnAddress, FunctionPtr(cti_op_get_by_id_self_fail));
        else
            JIT::patchGetByIdSelf(codeBlock, stubInfo, structure, slot.cachedOffset(), returnAddress);
        return;
    }

    if (structure->isDictionary()) {
        ctiPatchCallByReturnAddress(codeBlock, returnAddress, FunctionPtr(cti_op_get_by_id_generic));
        return;
    }

    if (slot.slotBase() == structure->prototypeForLookup(callFrame)) {
        ASSERT(slot.slotBase().isObject());

        JSObject* slotBaseObject = asObject(slot.slotBase());
        size_t offset = slot.cachedOffset();
        
        // Since we're accessing a prototype in a loop, it's a good bet that it
        // should not be treated as a dictionary.
        if (slotBaseObject->structure()->isDictionary()) {
            slotBaseObject->flattenDictionaryObject(callFrame->globalData());
            offset = slotBaseObject->structure()->get(callFrame->globalData(), propertyName);
        }
        
        stubInfo->initGetByIdProto(callFrame->globalData(), codeBlock->ownerExecutable(), structure, slotBaseObject->structure());

        ASSERT(!structure->isDictionary());
        ASSERT(!slotBaseObject->structure()->isDictionary());
        JIT::compileGetByIdProto(callFrame->scopeChain()->globalData, callFrame, codeBlock, stubInfo, structure, slotBaseObject->structure(), propertyName, slot, offset, returnAddress);
        return;
    }

    size_t offset = slot.cachedOffset();
    size_t count = normalizePrototypeChain(callFrame, baseValue, slot.slotBase(), propertyName, offset);
    if (!count) {
        stubInfo->accessType = access_get_by_id_generic;
        return;
    }

    StructureChain* prototypeChain = structure->prototypeChain(callFrame);
    stubInfo->initGetByIdChain(callFrame->globalData(), codeBlock->ownerExecutable(), structure, prototypeChain);
    JIT::compileGetByIdChain(callFrame->scopeChain()->globalData, callFrame, codeBlock, stubInfo, structure, prototypeChain, count, propertyName, slot, offset, returnAddress);
}

#endif // ENABLE(JIT_OPTIMIZE_PROPERTY_ACCESS)

#ifndef NDEBUG

extern "C" {

static void jscGeneratedNativeCode() 
{
    // When executing a JIT stub function (which might do an allocation), we hack the return address
    // to pretend to be executing this function, to keep stack logging tools from blowing out
    // memory.
}

}

struct StackHack {
    ALWAYS_INLINE StackHack(JITStackFrame& stackFrame) 
        : stackFrame(stackFrame)
        , savedReturnAddress(*stackFrame.returnAddressSlot())
    {
        *stackFrame.returnAddressSlot() = ReturnAddressPtr(FunctionPtr(jscGeneratedNativeCode));
    }

    ALWAYS_INLINE ~StackHack() 
    { 
        *stackFrame.returnAddressSlot() = savedReturnAddress;
    }

    JITStackFrame& stackFrame;
    ReturnAddressPtr savedReturnAddress;
};

#define STUB_INIT_STACK_FRAME(stackFrame) JITStackFrame& stackFrame = *reinterpret_cast_ptr<JITStackFrame*>(STUB_ARGS); StackHack stackHack(stackFrame)
#define STUB_SET_RETURN_ADDRESS(returnAddress) stackHack.savedReturnAddress = ReturnAddressPtr(returnAddress)
#define STUB_RETURN_ADDRESS stackHack.savedReturnAddress

#else

#define STUB_INIT_STACK_FRAME(stackFrame) JITStackFrame& stackFrame = *reinterpret_cast_ptr<JITStackFrame*>(STUB_ARGS)
#define STUB_SET_RETURN_ADDRESS(returnAddress) *stackFrame.returnAddressSlot() = ReturnAddressPtr(returnAddress)
#define STUB_RETURN_ADDRESS *stackFrame.returnAddressSlot()

#endif

// The reason this is not inlined is to avoid having to do a PIC branch
// to get the address of the ctiVMThrowTrampoline function. It's also
// good to keep the code size down by leaving as much of the exception
// handling code out of line as possible.
static NEVER_INLINE void returnToThrowTrampoline(JSGlobalData* globalData, ReturnAddressPtr exceptionLocation, ReturnAddressPtr& returnAddressSlot)
{
    ASSERT(globalData->exception);
    globalData->exceptionLocation = exceptionLocation;
    returnAddressSlot = ReturnAddressPtr(FunctionPtr(ctiVMThrowTrampoline));
}

static NEVER_INLINE void throwStackOverflowError(CallFrame* callFrame, JSGlobalData* globalData, ReturnAddressPtr exceptionLocation, ReturnAddressPtr& returnAddressSlot)
{
    globalData->exception = createStackOverflowError(callFrame);
    returnToThrowTrampoline(globalData, exceptionLocation, returnAddressSlot);
}

#define VM_THROW_EXCEPTION() \
    do { \
        VM_THROW_EXCEPTION_AT_END(); \
        return 0; \
    } while (0)
#define VM_THROW_EXCEPTION_AT_END() \
    do {\
        returnToThrowTrampoline(stackFrame.globalData, STUB_RETURN_ADDRESS, STUB_RETURN_ADDRESS);\
    } while (0)

#define CHECK_FOR_EXCEPTION() \
    do { \
        if (UNLIKELY(stackFrame.globalData->exception)) \
            VM_THROW_EXCEPTION(); \
    } while (0)
#define CHECK_FOR_EXCEPTION_AT_END() \
    do { \
        if (UNLIKELY(stackFrame.globalData->exception)) \
            VM_THROW_EXCEPTION_AT_END(); \
    } while (0)
#define CHECK_FOR_EXCEPTION_VOID() \
    do { \
        if (UNLIKELY(stackFrame.globalData->exception)) { \
            VM_THROW_EXCEPTION_AT_END(); \
            return; \
        } \
    } while (0)

struct ExceptionHandler {
    void* catchRoutine;
    CallFrame* callFrame;
};
static ExceptionHandler jitThrow(JSGlobalData* globalData, CallFrame* callFrame, JSValue exceptionValue, ReturnAddressPtr faultLocation)
{
    ASSERT(exceptionValue);

    unsigned vPCIndex = callFrame->codeBlock()->bytecodeOffset(faultLocation);
    globalData->exception = JSValue();
    HandlerInfo* handler = globalData->interpreter->throwException(callFrame, exceptionValue, vPCIndex); // This may update callFrame & exceptionValue!
    globalData->exception = exceptionValue;

    void* catchRoutine = handler ? handler->nativeCode.executableAddress() : FunctionPtr(ctiOpThrowNotCaught).value();
    ASSERT(catchRoutine);
    ExceptionHandler exceptionHandler = { catchRoutine, callFrame };
    return exceptionHandler;
}

#if CPU(ARM_THUMB2) && COMPILER(GCC)

#define DEFINE_STUB_FUNCTION(rtype, op) \
    extern "C" { \
        rtype JITStubThunked_##op(STUB_ARGS_DECLARATION); \
    }; \
    asm ( \
        ".text" "\n" \
        ".align 2" "\n" \
        ".globl " SYMBOL_STRING(cti_##op) "\n" \
        HIDE_SYMBOL(cti_##op) "\n"             \
        ".thumb" "\n" \
        ".thumb_func " THUMB_FUNC_PARAM(cti_##op) "\n" \
        SYMBOL_STRING(cti_##op) ":" "\n" \
        "str lr, [sp, #" STRINGIZE_VALUE_OF(THUNK_RETURN_ADDRESS_OFFSET) "]" "\n" \
        "bl " SYMBOL_STRING(JITStubThunked_##op) "\n" \
        "ldr lr, [sp, #" STRINGIZE_VALUE_OF(THUNK_RETURN_ADDRESS_OFFSET) "]" "\n" \
        "bx lr" "\n" \
        ); \
    rtype JITStubThunked_##op(STUB_ARGS_DECLARATION) \

#elif CPU(MIPS)
#if WTF_MIPS_PIC
#define DEFINE_STUB_FUNCTION(rtype, op) \
    extern "C" { \
        rtype JITStubThunked_##op(STUB_ARGS_DECLARATION); \
    }; \
    asm ( \
        ".text" "\n" \
        ".align 2" "\n" \
        ".set noreorder" "\n" \
        ".set nomacro" "\n" \
        ".set nomips16" "\n" \
        ".globl " SYMBOL_STRING(cti_##op) "\n" \
        ".ent " SYMBOL_STRING(cti_##op) "\n" \
        SYMBOL_STRING(cti_##op) ":" "\n" \
        "lw    $28," STRINGIZE_VALUE_OF(PRESERVED_GP_OFFSET) "($29)" "\n" \
        "sw    $31," STRINGIZE_VALUE_OF(THUNK_RETURN_ADDRESS_OFFSET) "($29)" "\n" \
        ".set macro" "\n" \
        "la    $25," SYMBOL_STRING(JITStubThunked_##op) "\n" \
        ".set nomacro" "\n" \
        "bal " SYMBOL_STRING(JITStubThunked_##op) "\n" \
        "nop" "\n" \
        "lw    $31," STRINGIZE_VALUE_OF(THUNK_RETURN_ADDRESS_OFFSET) "($29)" "\n" \
        "jr    $31" "\n" \
        "nop" "\n" \
        ".set reorder" "\n" \
        ".set macro" "\n" \
        ".end " SYMBOL_STRING(cti_##op) "\n" \
        ); \
    rtype JITStubThunked_##op(STUB_ARGS_DECLARATION)

#else // WTF_MIPS_PIC
#define DEFINE_STUB_FUNCTION(rtype, op) \
    extern "C" { \
        rtype JITStubThunked_##op(STUB_ARGS_DECLARATION); \
    }; \
    asm ( \
        ".text" "\n" \
        ".align 2" "\n" \
        ".set noreorder" "\n" \
        ".set nomacro" "\n" \
        ".set nomips16" "\n" \
        ".globl " SYMBOL_STRING(cti_##op) "\n" \
        ".ent " SYMBOL_STRING(cti_##op) "\n" \
        SYMBOL_STRING(cti_##op) ":" "\n" \
        "sw    $31," STRINGIZE_VALUE_OF(THUNK_RETURN_ADDRESS_OFFSET) "($29)" "\n" \
        "jal " SYMBOL_STRING(JITStubThunked_##op) "\n" \
        "nop" "\n" \
        "lw    $31," STRINGIZE_VALUE_OF(THUNK_RETURN_ADDRESS_OFFSET) "($29)" "\n" \
        "jr    $31" "\n" \
        "nop" "\n" \
        ".set reorder" "\n" \
        ".set macro" "\n" \
        ".end " SYMBOL_STRING(cti_##op) "\n" \
        ); \
    rtype JITStubThunked_##op(STUB_ARGS_DECLARATION)

#endif

#elif CPU(ARM_TRADITIONAL) && COMPILER(GCC)

#define DEFINE_STUB_FUNCTION(rtype, op) \
    extern "C" { \
        rtype JITStubThunked_##op(STUB_ARGS_DECLARATION); \
    }; \
    asm ( \
        ".globl " SYMBOL_STRING(cti_##op) "\n" \
        SYMBOL_STRING(cti_##op) ":" "\n" \
        "str lr, [sp, #" STRINGIZE_VALUE_OF(THUNK_RETURN_ADDRESS_OFFSET) "]" "\n" \
        "bl " SYMBOL_STRING(JITStubThunked_##op) "\n" \
        "ldr lr, [sp, #" STRINGIZE_VALUE_OF(THUNK_RETURN_ADDRESS_OFFSET) "]" "\n" \
        "mov pc, lr" "\n" \
        ); \
    rtype JITStubThunked_##op(STUB_ARGS_DECLARATION)

#elif (CPU(ARM_THUMB2) || CPU(ARM_TRADITIONAL)) && COMPILER(RVCT)

#define DEFINE_STUB_FUNCTION(rtype, op) rtype JITStubThunked_##op(STUB_ARGS_DECLARATION)

/* The following is a workaround for RVCT toolchain; precompiler macros are not expanded before the code is passed to the assembler */

/* The following section is a template to generate code for GeneratedJITStubs_RVCT.h */
/* The pattern "#xxx#" will be replaced with "xxx" */

/*
RVCT(extern "C" #rtype# JITStubThunked_#op#(STUB_ARGS_DECLARATION);)
RVCT(__asm #rtype# cti_#op#(STUB_ARGS_DECLARATION))
RVCT({)
RVCT(    PRESERVE8)
RVCT(    IMPORT JITStubThunked_#op#)
RVCT(    str lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET])
RVCT(    bl JITStubThunked_#op#)
RVCT(    ldr lr, [sp, # THUNK_RETURN_ADDRESS_OFFSET])
RVCT(    bx lr)
RVCT(})
RVCT()
*/

/* Include the generated file */
#include "GeneratedJITStubs_RVCT.h"

#elif CPU(ARM_TRADITIONAL) && COMPILER(MSVC)

#define DEFINE_STUB_FUNCTION(rtype, op) extern "C" rtype JITStubThunked_##op(STUB_ARGS_DECLARATION)

/* The following is a workaround for MSVC toolchain; inline assembler is not supported */

/* The following section is a template to generate code for GeneratedJITStubs_MSVC.asm */
/* The pattern "#xxx#" will be replaced with "xxx" */

/*
MSVC_BEGIN(    AREA Trampoline, CODE)
MSVC_BEGIN()
MSVC_BEGIN(    EXPORT ctiTrampoline)
MSVC_BEGIN(    EXPORT ctiVMThrowTrampoline)
MSVC_BEGIN(    EXPORT ctiOpThrowNotCaught)
MSVC_BEGIN()
MSVC_BEGIN(ctiTrampoline PROC)
MSVC_BEGIN(    stmdb sp!, {r1-r3})
MSVC_BEGIN(    stmdb sp!, {r4-r8, lr})
MSVC_BEGIN(    sub sp, sp, #68 ; sync with PRESERVEDR4_OFFSET)
MSVC_BEGIN(    mov r4, r2)
MSVC_BEGIN(    mov r5, #512)
MSVC_BEGIN(    ; r0 contains the code)
MSVC_BEGIN(    mov lr, pc)
MSVC_BEGIN(    bx r0)
MSVC_BEGIN(    add sp, sp, #68 ; sync with PRESERVEDR4_OFFSET)
MSVC_BEGIN(    ldmia sp!, {r4-r8, lr})
MSVC_BEGIN(    add sp, sp, #12)
MSVC_BEGIN(    bx lr)
MSVC_BEGIN(ctiTrampoline ENDP)
MSVC_BEGIN()
MSVC_BEGIN(ctiVMThrowTrampoline PROC)
MSVC_BEGIN(    mov r0, sp)
MSVC_BEGIN(    mov lr, pc)
MSVC_BEGIN(    bl cti_vm_throw)
MSVC_BEGIN(ctiOpThrowNotCaught)
MSVC_BEGIN(    add sp, sp, #68 ; sync with PRESERVEDR4_OFFSET)
MSVC_BEGIN(    ldmia sp!, {r4-r8, lr})
MSVC_BEGIN(    add sp, sp, #12)
MSVC_BEGIN(    bx lr)
MSVC_BEGIN(ctiVMThrowTrampoline ENDP)
MSVC_BEGIN()

MSVC(    EXPORT cti_#op#)
MSVC(    IMPORT JITStubThunked_#op#)
MSVC(cti_#op# PROC)
MSVC(    str lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET)
MSVC(    bl JITStubThunked_#op#)
MSVC(    ldr lr, [sp, #64] ; sync with THUNK_RETURN_ADDRESS_OFFSET)
MSVC(    bx lr)
MSVC(cti_#op# ENDP)
MSVC()

MSVC_END(    END)
*/

#elif CPU(SH4)
#define DEFINE_STUB_FUNCTION(rtype, op) \
    extern "C" { \
        rtype JITStubThunked_##op(STUB_ARGS_DECLARATION); \
    }; \
    asm volatile( \
    ".align 2" "\n" \
    ".globl " SYMBOL_STRING(cti_##op) "\n" \
    SYMBOL_STRING(cti_##op) ":" "\n" \
    "sts pr, r11" "\n" \
    "mov.l r11, @(0x38, r15)" "\n" \
    "mov.l .L2" SYMBOL_STRING(JITStubThunked_##op) ",r0" "\n" \
    "mov.l @(r0,r12),r11" "\n" \
    "jsr @r11" "\n" \
    "nop" "\n" \
    "mov.l @(0x38, r15), r11 " "\n" \
    "lds r11, pr " "\n" \
    "rts" "\n" \
    "nop" "\n" \
    ".align 2" "\n" \
    ".L2" SYMBOL_STRING(JITStubThunked_##op) ":.long " SYMBOL_STRING(JITStubThunked_##op)"@GOT \n" \
    ); \
    rtype JITStubThunked_##op(STUB_ARGS_DECLARATION)
#else
#define DEFINE_STUB_FUNCTION(rtype, op) rtype JIT_STUB cti_##op(STUB_ARGS_DECLARATION)
#endif

DEFINE_STUB_FUNCTION(EncodedJSValue, op_create_this)
{
    STUB_INIT_STACK_FRAME(stackFrame);
    CallFrame* callFrame = stackFrame.callFrame;

    JSFunction* constructor = asFunction(callFrame->callee());
#if !ASSERT_DISABLED
    ConstructData constructData;
    ASSERT(constructor->getConstructData(constructData) == ConstructTypeJS);
#endif

    Structure* structure;
    JSValue proto = stackFrame.args[0].jsValue();
    if (proto.isObject())
        structure = asObject(proto)->inheritorID(*stackFrame.globalData);
    else
        structure = constructor->scope()->globalObject->emptyObjectStructure();
    JSValue result = constructEmptyObject(callFrame, structure);

    return JSValue::encode(result);
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_convert_this)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    JSValue v1 = stackFrame.args[0].jsValue();
    CallFrame* callFrame = stackFrame.callFrame;

    JSObject* result = v1.toThisObject(callFrame);
    CHECK_FOR_EXCEPTION_AT_END();
    return JSValue::encode(result);
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_convert_this_strict)
{
    STUB_INIT_STACK_FRAME(stackFrame);
    
    JSValue v1 = stackFrame.args[0].jsValue();
    CallFrame* callFrame = stackFrame.callFrame;
    ASSERT(v1.asCell()->structure()->typeInfo().needsThisConversion());
    JSValue result = v1.toStrictThisObject(callFrame);
    CHECK_FOR_EXCEPTION_AT_END();
    return JSValue::encode(result);
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_add)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    JSValue v1 = stackFrame.args[0].jsValue();
    JSValue v2 = stackFrame.args[1].jsValue();
    CallFrame* callFrame = stackFrame.callFrame;

    if (v1.isString()) {
        JSValue result = v2.isString()
            ? jsString(callFrame, asString(v1), asString(v2))
            : jsString(callFrame, asString(v1), v2.toPrimitiveString(callFrame));
        CHECK_FOR_EXCEPTION_AT_END();
        return JSValue::encode(result);
    }

    double left = 0.0, right;
    if (v1.getNumber(left) && v2.getNumber(right))
        return JSValue::encode(jsNumber(left + right));

    // All other cases are pretty uncommon
    JSValue result = jsAddSlowCase(callFrame, v1, v2);
    CHECK_FOR_EXCEPTION_AT_END();
    return JSValue::encode(result);
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_pre_inc)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    JSValue v = stackFrame.args[0].jsValue();

    CallFrame* callFrame = stackFrame.callFrame;
    JSValue result = jsNumber(v.toNumber(callFrame) + 1);
    CHECK_FOR_EXCEPTION_AT_END();
    return JSValue::encode(result);
}

DEFINE_STUB_FUNCTION(int, timeout_check)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    JSGlobalData* globalData = stackFrame.globalData;
    TimeoutChecker& timeoutChecker = globalData->timeoutChecker;

    if (globalData->terminator.shouldTerminate()) {
        globalData->exception = createTerminatedExecutionException(globalData);
        VM_THROW_EXCEPTION_AT_END();
    } else if (timeoutChecker.didTimeOut(stackFrame.callFrame)) {
        globalData->exception = createInterruptedExecutionException(globalData);
        VM_THROW_EXCEPTION_AT_END();
    }

    return timeoutChecker.ticksUntilNextCheck();
}

DEFINE_STUB_FUNCTION(void*, register_file_check)
{
    STUB_INIT_STACK_FRAME(stackFrame);
    CallFrame* callFrame = stackFrame.callFrame;

    if (UNLIKELY(!stackFrame.registerFile->grow(&callFrame->registers()[callFrame->codeBlock()->m_numCalleeRegisters]))) {
        // Rewind to the previous call frame because op_call already optimistically
        // moved the call frame forward.
        CallFrame* oldCallFrame = callFrame->callerFrame();
        ExceptionHandler handler = jitThrow(stackFrame.globalData, oldCallFrame, createStackOverflowError(oldCallFrame), ReturnAddressPtr(callFrame->returnPC()));
        STUB_SET_RETURN_ADDRESS(handler.catchRoutine);
        callFrame = handler.callFrame;
    }

    return callFrame;
}

DEFINE_STUB_FUNCTION(int, op_loop_if_lesseq)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    JSValue src1 = stackFrame.args[0].jsValue();
    JSValue src2 = stackFrame.args[1].jsValue();
    CallFrame* callFrame = stackFrame.callFrame;

    bool result = jsLessEq(callFrame, src1, src2);
    CHECK_FOR_EXCEPTION_AT_END();
    return result;
}

DEFINE_STUB_FUNCTION(JSObject*, op_new_object)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    return constructEmptyObject(stackFrame.callFrame);
}

DEFINE_STUB_FUNCTION(void, op_put_by_id_generic)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    PutPropertySlot slot(stackFrame.callFrame->codeBlock()->isStrictMode());
    stackFrame.args[0].jsValue().put(stackFrame.callFrame, stackFrame.args[1].identifier(), stackFrame.args[2].jsValue(), slot);
    CHECK_FOR_EXCEPTION_AT_END();
}

DEFINE_STUB_FUNCTION(void, op_put_by_id_direct_generic)
{
    STUB_INIT_STACK_FRAME(stackFrame);
    
    PutPropertySlot slot(stackFrame.callFrame->codeBlock()->isStrictMode());
    stackFrame.args[0].jsValue().putDirect(stackFrame.callFrame, stackFrame.args[1].identifier(), stackFrame.args[2].jsValue(), slot);
    CHECK_FOR_EXCEPTION_AT_END();
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_get_by_id_generic)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    CallFrame* callFrame = stackFrame.callFrame;
    Identifier& ident = stackFrame.args[1].identifier();

    JSValue baseValue = stackFrame.args[0].jsValue();
    PropertySlot slot(baseValue);
    JSValue result = baseValue.get(callFrame, ident, slot);

    CHECK_FOR_EXCEPTION_AT_END();
    return JSValue::encode(result);
}

#if ENABLE(JIT_OPTIMIZE_PROPERTY_ACCESS)

DEFINE_STUB_FUNCTION(void, op_put_by_id)
{
    STUB_INIT_STACK_FRAME(stackFrame);
    CallFrame* callFrame = stackFrame.callFrame;
    Identifier& ident = stackFrame.args[1].identifier();
    
    PutPropertySlot slot(callFrame->codeBlock()->isStrictMode());
    stackFrame.args[0].jsValue().put(callFrame, ident, stackFrame.args[2].jsValue(), slot);
    
    CodeBlock* codeBlock = stackFrame.callFrame->codeBlock();
    StructureStubInfo* stubInfo = &codeBlock->getStubInfo(STUB_RETURN_ADDRESS);
    if (!stubInfo->seenOnce())
        stubInfo->setSeen();
    else
        JITThunks::tryCachePutByID(callFrame, codeBlock, STUB_RETURN_ADDRESS, stackFrame.args[0].jsValue(), slot, stubInfo, false);
    
    CHECK_FOR_EXCEPTION_AT_END();
}

DEFINE_STUB_FUNCTION(void, op_put_by_id_direct)
{
    STUB_INIT_STACK_FRAME(stackFrame);
    CallFrame* callFrame = stackFrame.callFrame;
    Identifier& ident = stackFrame.args[1].identifier();
    
    PutPropertySlot slot(callFrame->codeBlock()->isStrictMode());
    stackFrame.args[0].jsValue().putDirect(callFrame, ident, stackFrame.args[2].jsValue(), slot);
    
    CodeBlock* codeBlock = stackFrame.callFrame->codeBlock();
    StructureStubInfo* stubInfo = &codeBlock->getStubInfo(STUB_RETURN_ADDRESS);
    if (!stubInfo->seenOnce())
        stubInfo->setSeen();
    else
        JITThunks::tryCachePutByID(callFrame, codeBlock, STUB_RETURN_ADDRESS, stackFrame.args[0].jsValue(), slot, stubInfo, true);
    
    CHECK_FOR_EXCEPTION_AT_END();
}

DEFINE_STUB_FUNCTION(void, op_put_by_id_fail)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    CallFrame* callFrame = stackFrame.callFrame;
    Identifier& ident = stackFrame.args[1].identifier();
    
    PutPropertySlot slot(callFrame->codeBlock()->isStrictMode());
    stackFrame.args[0].jsValue().put(callFrame, ident, stackFrame.args[2].jsValue(), slot);

    CHECK_FOR_EXCEPTION_AT_END();
}

DEFINE_STUB_FUNCTION(void, op_put_by_id_direct_fail)
{
    STUB_INIT_STACK_FRAME(stackFrame);
    
    CallFrame* callFrame = stackFrame.callFrame;
    Identifier& ident = stackFrame.args[1].identifier();
    
    PutPropertySlot slot(callFrame->codeBlock()->isStrictMode());
    stackFrame.args[0].jsValue().putDirect(callFrame, ident, stackFrame.args[2].jsValue(), slot);
    
    CHECK_FOR_EXCEPTION_AT_END();
}

DEFINE_STUB_FUNCTION(JSObject*, op_put_by_id_transition_realloc)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    JSValue baseValue = stackFrame.args[0].jsValue();
    int32_t oldSize = stackFrame.args[3].int32();
    int32_t newSize = stackFrame.args[4].int32();

    ASSERT(baseValue.isObject());
    JSObject* base = asObject(baseValue);
    base->allocatePropertyStorage(oldSize, newSize);

    return base;
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_get_by_id_method_check)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    CallFrame* callFrame = stackFrame.callFrame;
    Identifier& ident = stackFrame.args[1].identifier();

    JSValue baseValue = stackFrame.args[0].jsValue();
    PropertySlot slot(baseValue);
    JSValue result = baseValue.get(callFrame, ident, slot);
    CHECK_FOR_EXCEPTION();

    CodeBlock* codeBlock = stackFrame.callFrame->codeBlock();
    MethodCallLinkInfo& methodCallLinkInfo = codeBlock->getMethodCallLinkInfo(STUB_RETURN_ADDRESS);

    if (!methodCallLinkInfo.seenOnce()) {
        methodCallLinkInfo.setSeen();
        return JSValue::encode(result);
    }

    // If we successfully got something, then the base from which it is being accessed must
    // be an object.  (Assertion to ensure asObject() call below is safe, which comes after
    // an isCacheable() chceck.
    ASSERT(!slot.isCacheableValue() || slot.slotBase().isObject());

    // Check that:
    //   * We're dealing with a JSCell,
    //   * the property is cachable,
    //   * it's not a dictionary
    //   * there is a function cached.
    Structure* structure;
    JSCell* specific;
    JSObject* slotBaseObject;
    if (baseValue.isCell()
        && slot.isCacheableValue()
        && !(structure = baseValue.asCell()->structure())->isUncacheableDictionary()
        && (slotBaseObject = asObject(slot.slotBase()))->getPropertySpecificValue(callFrame, ident, specific)
        && specific
        ) {

        JSFunction* callee = (JSFunction*)specific;

        // Since we're accessing a prototype in a loop, it's a good bet that it
        // should not be treated as a dictionary.
        if (slotBaseObject->structure()->isDictionary())
            slotBaseObject->flattenDictionaryObject(callFrame->globalData());

        // The result fetched should always be the callee!
        ASSERT(result == JSValue(callee));

        // Check to see if the function is on the object's prototype.  Patch up the code to optimize.
        if (slot.slotBase() == structure->prototypeForLookup(callFrame)) {
            JIT::patchMethodCallProto(callFrame->globalData(), codeBlock, methodCallLinkInfo, callee, structure, slotBaseObject, STUB_RETURN_ADDRESS);
            return JSValue::encode(result);
        }

        // Check to see if the function is on the object itself.
        // Since we generate the method-check to check both the structure and a prototype-structure (since this
        // is the common case) we have a problem - we need to patch the prototype structure check to do something
        // useful.  We could try to nop it out altogether, but that's a little messy, so lets do something simpler
        // for now.  For now it performs a check on a special object on the global object only used for this
        // purpose.  The object is in no way exposed, and as such the check will always pass.
        if (slot.slotBase() == baseValue) {
            JIT::patchMethodCallProto(callFrame->globalData(), codeBlock, methodCallLinkInfo, callee, structure, callFrame->scopeChain()->globalObject->methodCallDummy(), STUB_RETURN_ADDRESS);
            return JSValue::encode(result);
        }
    }

    // Revert the get_by_id op back to being a regular get_by_id - allow it to cache like normal, if it needs to.
    ctiPatchCallByReturnAddress(codeBlock, STUB_RETURN_ADDRESS, FunctionPtr(cti_op_get_by_id));
    return JSValue::encode(result);
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_get_by_id)
{
    STUB_INIT_STACK_FRAME(stackFrame);
    CallFrame* callFrame = stackFrame.callFrame;
    Identifier& ident = stackFrame.args[1].identifier();

    JSValue baseValue = stackFrame.args[0].jsValue();
    PropertySlot slot(baseValue);
    JSValue result = baseValue.get(callFrame, ident, slot);

    CodeBlock* codeBlock = stackFrame.callFrame->codeBlock();
    StructureStubInfo* stubInfo = &codeBlock->getStubInfo(STUB_RETURN_ADDRESS);
    if (!stubInfo->seenOnce())
        stubInfo->setSeen();
    else
        JITThunks::tryCacheGetByID(callFrame, codeBlock, STUB_RETURN_ADDRESS, baseValue, ident, slot, stubInfo);

    CHECK_FOR_EXCEPTION_AT_END();
    return JSValue::encode(result);
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_get_by_id_self_fail)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    CallFrame* callFrame = stackFrame.callFrame;
    Identifier& ident = stackFrame.args[1].identifier();

    JSValue baseValue = stackFrame.args[0].jsValue();
    PropertySlot slot(baseValue);
    JSValue result = baseValue.get(callFrame, ident, slot);

    CHECK_FOR_EXCEPTION();

    if (baseValue.isCell()
        && slot.isCacheable()
        && !baseValue.asCell()->structure()->isUncacheableDictionary()
        && slot.slotBase() == baseValue) {

        CodeBlock* codeBlock = callFrame->codeBlock();
        StructureStubInfo* stubInfo = &codeBlock->getStubInfo(STUB_RETURN_ADDRESS);

        ASSERT(slot.slotBase().isObject());

        PolymorphicAccessStructureList* polymorphicStructureList;
        int listIndex = 1;

        if (stubInfo->accessType == access_get_by_id_self) {
            ASSERT(!stubInfo->stubRoutine);
            polymorphicStructureList = new PolymorphicAccessStructureList(callFrame->globalData(), codeBlock->ownerExecutable(), CodeLocationLabel(), stubInfo->u.getByIdSelf.baseObjectStructure.get());
            stubInfo->initGetByIdSelfList(polymorphicStructureList, 1);
        } else {
            polymorphicStructureList = stubInfo->u.getByIdSelfList.structureList;
            listIndex = stubInfo->u.getByIdSelfList.listSize;
        }
        if (listIndex < POLYMORPHIC_LIST_CACHE_SIZE) {
            stubInfo->u.getByIdSelfList.listSize++;
            JIT::compileGetByIdSelfList(callFrame->scopeChain()->globalData, codeBlock, stubInfo, polymorphicStructureList, listIndex, baseValue.asCell()->structure(), ident, slot, slot.cachedOffset());

            if (listIndex == (POLYMORPHIC_LIST_CACHE_SIZE - 1))
                ctiPatchCallByReturnAddress(codeBlock, STUB_RETURN_ADDRESS, FunctionPtr(cti_op_get_by_id_generic));
        }
    } else
        ctiPatchCallByReturnAddress(callFrame->codeBlock(), STUB_RETURN_ADDRESS, FunctionPtr(cti_op_get_by_id_generic));
    return JSValue::encode(result);
}

static PolymorphicAccessStructureList* getPolymorphicAccessStructureListSlot(JSGlobalData& globalData, ScriptExecutable* owner, StructureStubInfo* stubInfo, int& listIndex)
{
    PolymorphicAccessStructureList* prototypeStructureList = 0;
    listIndex = 1;

    switch (stubInfo->accessType) {
    case access_get_by_id_proto:
        prototypeStructureList = new PolymorphicAccessStructureList(globalData, owner, stubInfo->stubRoutine, stubInfo->u.getByIdProto.baseObjectStructure.get(), stubInfo->u.getByIdProto.prototypeStructure.get());
        stubInfo->stubRoutine = CodeLocationLabel();
        stubInfo->initGetByIdProtoList(prototypeStructureList, 2);
        break;
    case access_get_by_id_chain:
        prototypeStructureList = new PolymorphicAccessStructureList(globalData, owner, stubInfo->stubRoutine, stubInfo->u.getByIdChain.baseObjectStructure.get(), stubInfo->u.getByIdChain.chain.get());
        stubInfo->stubRoutine = CodeLocationLabel();
        stubInfo->initGetByIdProtoList(prototypeStructureList, 2);
        break;
    case access_get_by_id_proto_list:
        prototypeStructureList = stubInfo->u.getByIdProtoList.structureList;
        listIndex = stubInfo->u.getByIdProtoList.listSize;
        if (listIndex < POLYMORPHIC_LIST_CACHE_SIZE)
            stubInfo->u.getByIdProtoList.listSize++;
        break;
    default:
        ASSERT_NOT_REACHED();
    }
    
    ASSERT(listIndex <= POLYMORPHIC_LIST_CACHE_SIZE);
    return prototypeStructureList;
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_get_by_id_getter_stub)
{
    STUB_INIT_STACK_FRAME(stackFrame);
    CallFrame* callFrame = stackFrame.callFrame;
    GetterSetter* getterSetter = asGetterSetter(stackFrame.args[0].jsObject());
    if (!getterSetter->getter())
        return JSValue::encode(jsUndefined());
    JSObject* getter = asObject(getterSetter->getter());
    CallData callData;
    CallType callType = getter->getCallData(callData);
    JSValue result = call(callFrame, getter, callType, callData, stackFrame.args[1].jsObject(), ArgList());
    if (callFrame->hadException())
        returnToThrowTrampoline(&callFrame->globalData(), stackFrame.args[2].returnAddress(), STUB_RETURN_ADDRESS);

    return JSValue::encode(result);
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_get_by_id_custom_stub)
{
    STUB_INIT_STACK_FRAME(stackFrame);
    CallFrame* callFrame = stackFrame.callFrame;
    JSObject* slotBase = stackFrame.args[0].jsObject();
    PropertySlot::GetValueFunc getter = reinterpret_cast<PropertySlot::GetValueFunc>(stackFrame.args[1].asPointer);
    const Identifier& ident = stackFrame.args[2].identifier();
    JSValue result = getter(callFrame, slotBase, ident);
    if (callFrame->hadException())
        returnToThrowTrampoline(&callFrame->globalData(), stackFrame.args[3].returnAddress(), STUB_RETURN_ADDRESS);
    
    return JSValue::encode(result);
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_get_by_id_proto_list)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    CallFrame* callFrame = stackFrame.callFrame;
    const Identifier& propertyName = stackFrame.args[1].identifier();

    JSValue baseValue = stackFrame.args[0].jsValue();
    PropertySlot slot(baseValue);
    JSValue result = baseValue.get(callFrame, propertyName, slot);

    CHECK_FOR_EXCEPTION();

    if (!baseValue.isCell() || !slot.isCacheable() || baseValue.asCell()->structure()->isDictionary()) {
        ctiPatchCallByReturnAddress(callFrame->codeBlock(), STUB_RETURN_ADDRESS, FunctionPtr(cti_op_get_by_id_proto_fail));
        return JSValue::encode(result);
    }

    Structure* structure = baseValue.asCell()->structure();
    CodeBlock* codeBlock = callFrame->codeBlock();
    StructureStubInfo* stubInfo = &codeBlock->getStubInfo(STUB_RETURN_ADDRESS);

    ASSERT(slot.slotBase().isObject());
    JSObject* slotBaseObject = asObject(slot.slotBase());
    
    size_t offset = slot.cachedOffset();

    if (slot.slotBase() == baseValue)
        ctiPatchCallByReturnAddress(codeBlock, STUB_RETURN_ADDRESS, FunctionPtr(cti_op_get_by_id_proto_fail));
    else if (slot.slotBase() == baseValue.asCell()->structure()->prototypeForLookup(callFrame)) {
        ASSERT(!baseValue.asCell()->structure()->isDictionary());
        // Since we're accessing a prototype in a loop, it's a good bet that it
        // should not be treated as a dictionary.
        if (slotBaseObject->structure()->isDictionary()) {
            slotBaseObject->flattenDictionaryObject(callFrame->globalData());
            offset = slotBaseObject->structure()->get(callFrame->globalData(), propertyName);
        }

        int listIndex;
        PolymorphicAccessStructureList* prototypeStructureList = getPolymorphicAccessStructureListSlot(callFrame->globalData(), codeBlock->ownerExecutable(), stubInfo, listIndex);
        if (listIndex < POLYMORPHIC_LIST_CACHE_SIZE) {
            JIT::compileGetByIdProtoList(callFrame->scopeChain()->globalData, callFrame, codeBlock, stubInfo, prototypeStructureList, listIndex, structure, slotBaseObject->structure(), propertyName, slot, offset);

            if (listIndex == (POLYMORPHIC_LIST_CACHE_SIZE - 1))
                ctiPatchCallByReturnAddress(codeBlock, STUB_RETURN_ADDRESS, FunctionPtr(cti_op_get_by_id_proto_list_full));
        }
    } else if (size_t count = normalizePrototypeChain(callFrame, baseValue, slot.slotBase(), propertyName, offset)) {
        ASSERT(!baseValue.asCell()->structure()->isDictionary());
        int listIndex;
        PolymorphicAccessStructureList* prototypeStructureList = getPolymorphicAccessStructureListSlot(callFrame->globalData(), codeBlock->ownerExecutable(), stubInfo, listIndex);
        
        if (listIndex < POLYMORPHIC_LIST_CACHE_SIZE) {
            StructureChain* protoChain = structure->prototypeChain(callFrame);
            JIT::compileGetByIdChainList(callFrame->scopeChain()->globalData, callFrame, codeBlock, stubInfo, prototypeStructureList, listIndex, structure, protoChain, count, propertyName, slot, offset);

            if (listIndex == (POLYMORPHIC_LIST_CACHE_SIZE - 1))
                ctiPatchCallByReturnAddress(codeBlock, STUB_RETURN_ADDRESS, FunctionPtr(cti_op_get_by_id_proto_list_full));
        }
    } else
        ctiPatchCallByReturnAddress(codeBlock, STUB_RETURN_ADDRESS, FunctionPtr(cti_op_get_by_id_proto_fail));

    return JSValue::encode(result);
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_get_by_id_proto_list_full)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    JSValue baseValue = stackFrame.args[0].jsValue();
    PropertySlot slot(baseValue);
    JSValue result = baseValue.get(stackFrame.callFrame, stackFrame.args[1].identifier(), slot);

    CHECK_FOR_EXCEPTION_AT_END();
    return JSValue::encode(result);
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_get_by_id_proto_fail)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    JSValue baseValue = stackFrame.args[0].jsValue();
    PropertySlot slot(baseValue);
    JSValue result = baseValue.get(stackFrame.callFrame, stackFrame.args[1].identifier(), slot);

    CHECK_FOR_EXCEPTION_AT_END();
    return JSValue::encode(result);
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_get_by_id_array_fail)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    JSValue baseValue = stackFrame.args[0].jsValue();
    PropertySlot slot(baseValue);
    JSValue result = baseValue.get(stackFrame.callFrame, stackFrame.args[1].identifier(), slot);

    CHECK_FOR_EXCEPTION_AT_END();
    return JSValue::encode(result);
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_get_by_id_string_fail)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    JSValue baseValue = stackFrame.args[0].jsValue();
    PropertySlot slot(baseValue);
    JSValue result = baseValue.get(stackFrame.callFrame, stackFrame.args[1].identifier(), slot);

    CHECK_FOR_EXCEPTION_AT_END();
    return JSValue::encode(result);
}

#endif // ENABLE(JIT_OPTIMIZE_PROPERTY_ACCESS)

DEFINE_STUB_FUNCTION(void, op_check_has_instance)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    CallFrame* callFrame = stackFrame.callFrame;
    JSValue baseVal = stackFrame.args[0].jsValue();

    // ECMA-262 15.3.5.3:
    // Throw an exception either if baseVal is not an object, or if it does not implement 'HasInstance' (i.e. is a function).
#ifndef NDEBUG
    TypeInfo typeInfo(UnspecifiedType);
    ASSERT(!baseVal.isObject() || !(typeInfo = asObject(baseVal)->structure()->typeInfo()).implementsHasInstance());
#endif
    stackFrame.globalData->exception = createInvalidParamError(callFrame, "instanceof", baseVal);
    VM_THROW_EXCEPTION_AT_END();
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_instanceof)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    CallFrame* callFrame = stackFrame.callFrame;
    JSValue value = stackFrame.args[0].jsValue();
    JSValue baseVal = stackFrame.args[1].jsValue();
    JSValue proto = stackFrame.args[2].jsValue();

    // At least one of these checks must have failed to get to the slow case.
    ASSERT(!value.isCell() || !baseVal.isCell() || !proto.isCell()
           || !value.isObject() || !baseVal.isObject() || !proto.isObject() 
           || (asObject(baseVal)->structure()->typeInfo().flags() & (ImplementsHasInstance | OverridesHasInstance)) != ImplementsHasInstance);


    // ECMA-262 15.3.5.3:
    // Throw an exception either if baseVal is not an object, or if it does not implement 'HasInstance' (i.e. is a function).
    TypeInfo typeInfo(UnspecifiedType);
    if (!baseVal.isObject() || !(typeInfo = asObject(baseVal)->structure()->typeInfo()).implementsHasInstance()) {
        stackFrame.globalData->exception = createInvalidParamError(stackFrame.callFrame, "instanceof", baseVal);
        VM_THROW_EXCEPTION();
    }
    ASSERT(typeInfo.type() != UnspecifiedType);

    if (!typeInfo.overridesHasInstance()) {
        if (!value.isObject())
            return JSValue::encode(jsBoolean(false));

        if (!proto.isObject()) {
            throwError(callFrame, createTypeError(callFrame, "instanceof called on an object with an invalid prototype property."));
            VM_THROW_EXCEPTION();
        }
    }

    JSValue result = jsBoolean(asObject(baseVal)->hasInstance(callFrame, value, proto));
    CHECK_FOR_EXCEPTION_AT_END();

    return JSValue::encode(result);
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_del_by_id)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    CallFrame* callFrame = stackFrame.callFrame;
    
    JSObject* baseObj = stackFrame.args[0].jsValue().toObject(callFrame);

    bool couldDelete = baseObj->deleteProperty(callFrame, stackFrame.args[1].identifier());
    JSValue result = jsBoolean(couldDelete);
    if (!couldDelete && callFrame->codeBlock()->isStrictMode())
        stackFrame.globalData->exception = createTypeError(stackFrame.callFrame, "Unable to delete property.");

    CHECK_FOR_EXCEPTION_AT_END();
    return JSValue::encode(result);
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_mul)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    JSValue src1 = stackFrame.args[0].jsValue();
    JSValue src2 = stackFrame.args[1].jsValue();

    double left;
    double right;
    if (src1.getNumber(left) && src2.getNumber(right))
        return JSValue::encode(jsNumber(left * right));

    CallFrame* callFrame = stackFrame.callFrame;
    JSValue result = jsNumber(src1.toNumber(callFrame) * src2.toNumber(callFrame));
    CHECK_FOR_EXCEPTION_AT_END();
    return JSValue::encode(result);
}

DEFINE_STUB_FUNCTION(JSObject*, op_new_func)
{
    STUB_INIT_STACK_FRAME(stackFrame);
    
    ASSERT(stackFrame.callFrame->codeBlock()->codeType() != FunctionCode || !stackFrame.callFrame->codeBlock()->needsFullScopeChain() || stackFrame.callFrame->uncheckedR(stackFrame.callFrame->codeBlock()->activationRegister()).jsValue());
    return stackFrame.args[0].function()->make(stackFrame.callFrame, stackFrame.callFrame->scopeChain());
}

DEFINE_STUB_FUNCTION(void*, op_call_jitCompile)
{
    STUB_INIT_STACK_FRAME(stackFrame);

#if !ASSERT_DISABLED
    CallData callData;
    ASSERT(stackFrame.callFrame->callee()->getCallData(callData) == CallTypeJS);
#endif

    JSFunction* function = asFunction(stackFrame.callFrame->callee());
    ASSERT(!function->isHostFunction());
    FunctionExecutable* executable = function->jsExecutable();
    ScopeChainNode* callDataScopeChain = function->scope();
    JSObject* error = executable->compileForCall(stackFrame.callFrame, callDataScopeChain);
    if (error) {
        stackFrame.callFrame->globalData().exception = error;
        return 0;
    }
    return function;
}

DEFINE_STUB_FUNCTION(void*, op_construct_jitCompile)
{
    STUB_INIT_STACK_FRAME(stackFrame);

#if !ASSERT_DISABLED
    ConstructData constructData;
    ASSERT(asFunction(stackFrame.callFrame->callee())->getConstructData(constructData) == ConstructTypeJS);
#endif

    JSFunction* function = asFunction(stackFrame.callFrame->callee());
    ASSERT(!function->isHostFunction());
    FunctionExecutable* executable = function->jsExecutable();
    ScopeChainNode* callDataScopeChain = function->scope();
    JSObject* error = executable->compileForConstruct(stackFrame.callFrame, callDataScopeChain);
    if (error) {
        stackFrame.callFrame->globalData().exception = error;
        return 0;
    }
    return function;
}

DEFINE_STUB_FUNCTION(void*, op_call_arityCheck)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    CallFrame* callFrame = stackFrame.callFrame;
    JSFunction* callee = asFunction(callFrame->callee());
    ASSERT(!callee->isHostFunction());
    CodeBlock* newCodeBlock = &callee->jsExecutable()->generatedBytecodeForCall();
    int argCount = callFrame->argumentCountIncludingThis();
    ReturnAddressPtr pc = callFrame->returnPC();

    ASSERT(argCount != newCodeBlock->m_numParameters);

    CallFrame* oldCallFrame = callFrame->callerFrame();

    Register* r;
    if (argCount > newCodeBlock->m_numParameters) {
        size_t numParameters = newCodeBlock->m_numParameters;
        r = callFrame->registers() + numParameters;
        Register* newEnd = r + newCodeBlock->m_numCalleeRegisters;
        if (!stackFrame.registerFile->grow(newEnd)) {
            // Rewind to the previous call frame because op_call already optimistically
            // moved the call frame forward.
            ExceptionHandler handler = jitThrow(stackFrame.globalData, oldCallFrame, createStackOverflowError(oldCallFrame), pc);
            STUB_SET_RETURN_ADDRESS(handler.catchRoutine);
            return handler.callFrame;
        }

        Register* argv = r - RegisterFile::CallFrameHeaderSize - numParameters - argCount;
        for (size_t i = 0; i < numParameters; ++i)
            argv[i + argCount] = argv[i];
    } else {
        size_t omittedArgCount = newCodeBlock->m_numParameters - argCount;
        r = callFrame->registers() + omittedArgCount;
        Register* newEnd = r + newCodeBlock->m_numCalleeRegisters;
        if (!stackFrame.registerFile->grow(newEnd)) {
            // Rewind to the previous call frame because op_call already optimistically
            // moved the call frame forward.
            ExceptionHandler handler = jitThrow(stackFrame.globalData, oldCallFrame, createStackOverflowError(oldCallFrame), pc);
            STUB_SET_RETURN_ADDRESS(handler.catchRoutine);
            return handler.callFrame;
        }

        Register* argv = r - RegisterFile::CallFrameHeaderSize - omittedArgCount;
        for (size_t i = 0; i < omittedArgCount; ++i)
            argv[i] = jsUndefined();
    }

    callFrame = CallFrame::create(r);
    callFrame->setCallerFrame(oldCallFrame);
    callFrame->setArgumentCountIncludingThis(argCount);
    callFrame->setCallee(callee);
    callFrame->setScopeChain(callee->scope());
    callFrame->setReturnPC(pc.value());

    ASSERT((void*)callFrame <= stackFrame.registerFile->end());
    return callFrame;
}

DEFINE_STUB_FUNCTION(void*, op_construct_arityCheck)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    CallFrame* callFrame = stackFrame.callFrame;
    JSFunction* callee = asFunction(callFrame->callee());
    ASSERT(!callee->isHostFunction());
    CodeBlock* newCodeBlock = &callee->jsExecutable()->generatedBytecodeForConstruct();
    int argCount = callFrame->argumentCountIncludingThis();
    ReturnAddressPtr pc = callFrame->returnPC();

    ASSERT(argCount != newCodeBlock->m_numParameters);

    CallFrame* oldCallFrame = callFrame->callerFrame();

    Register* r;
    if (argCount > newCodeBlock->m_numParameters) {
        size_t numParameters = newCodeBlock->m_numParameters;
        r = callFrame->registers() + numParameters;
        Register* newEnd = r + newCodeBlock->m_numCalleeRegisters;
        if (!stackFrame.registerFile->grow(newEnd)) {
            // Rewind to the previous call frame because op_call already optimistically
            // moved the call frame forward.
            ExceptionHandler handler = jitThrow(stackFrame.globalData, oldCallFrame, createStackOverflowError(oldCallFrame), pc);
            STUB_SET_RETURN_ADDRESS(handler.catchRoutine);
            return handler.callFrame;
        }

        Register* argv = r - RegisterFile::CallFrameHeaderSize - numParameters - argCount;
        for (size_t i = 0; i < numParameters; ++i)
            argv[i + argCount] = argv[i];
    } else {
        size_t omittedArgCount = newCodeBlock->m_numParameters - argCount;
        r = callFrame->registers() + omittedArgCount;
        Register* newEnd = r + newCodeBlock->m_numCalleeRegisters;
        if (!stackFrame.registerFile->grow(newEnd)) {
            // Rewind to the previous call frame because op_call already optimistically
            // moved the call frame forward.
            ExceptionHandler handler = jitThrow(stackFrame.globalData, oldCallFrame, createStackOverflowError(oldCallFrame), pc);
            STUB_SET_RETURN_ADDRESS(handler.catchRoutine);
            return handler.callFrame;
        }

        Register* argv = r - RegisterFile::CallFrameHeaderSize - omittedArgCount;
        for (size_t i = 0; i < omittedArgCount; ++i)
            argv[i] = jsUndefined();
    }

    callFrame = CallFrame::create(r);
    callFrame->setCallerFrame(oldCallFrame);
    callFrame->setArgumentCountIncludingThis(argCount);
    callFrame->setCallee(callee);
    callFrame->setScopeChain(callee->scope());
    callFrame->setReturnPC(pc.value());

    ASSERT((void*)callFrame <= stackFrame.registerFile->end());
    return callFrame;
}

#if ENABLE(JIT_OPTIMIZE_CALL)
DEFINE_STUB_FUNCTION(void*, vm_lazyLinkCall)
{
    STUB_INIT_STACK_FRAME(stackFrame);
    CallFrame* callFrame = stackFrame.callFrame;
    JSFunction* callee = asFunction(callFrame->callee());
    ExecutableBase* executable = callee->executable();

    MacroAssemblerCodePtr codePtr;
    CodeBlock* codeBlock = 0;
    if (executable->isHostFunction())
        codePtr = executable->generatedJITCodeForCall().addressForCall();
    else {
        FunctionExecutable* functionExecutable = static_cast<FunctionExecutable*>(executable);
        JSObject* error = functionExecutable->compileForCall(callFrame, callee->scope());
        if (error) {
            callFrame->globalData().exception = createStackOverflowError(callFrame);
            return 0;
        }
        codeBlock = &functionExecutable->generatedBytecodeForCall();
        if (callFrame->argumentCountIncludingThis() == static_cast<size_t>(codeBlock->m_numParameters))
            codePtr = functionExecutable->generatedJITCodeForCall().addressForCall();
        else
            codePtr = functionExecutable->generatedJITCodeForCallWithArityCheck();
    }
    CallLinkInfo* callLinkInfo = &stackFrame.callFrame->callerFrame()->codeBlock()->getCallLinkInfo(callFrame->returnPC());

    if (!callLinkInfo->seenOnce())
        callLinkInfo->setSeen();
    else
        JIT::linkCall(callee, stackFrame.callFrame->callerFrame()->codeBlock(), codeBlock, codePtr, callLinkInfo, callFrame->argumentCountIncludingThis(), stackFrame.globalData);

    return codePtr.executableAddress();
}

DEFINE_STUB_FUNCTION(void*, vm_lazyLinkConstruct)
{
    STUB_INIT_STACK_FRAME(stackFrame);
    CallFrame* callFrame = stackFrame.callFrame;
    JSFunction* callee = asFunction(callFrame->callee());
    ExecutableBase* executable = callee->executable();

    MacroAssemblerCodePtr codePtr;
    CodeBlock* codeBlock = 0;
    if (executable->isHostFunction())
        codePtr = executable->generatedJITCodeForConstruct().addressForCall();
    else {
        FunctionExecutable* functionExecutable = static_cast<FunctionExecutable*>(executable);
        JSObject* error = functionExecutable->compileForConstruct(callFrame, callee->scope());
        if (error) {
            throwStackOverflowError(callFrame, stackFrame.globalData, ReturnAddressPtr(callFrame->returnPC()), STUB_RETURN_ADDRESS);
            return 0;
        }
        codeBlock = &functionExecutable->generatedBytecodeForConstruct();
        if (callFrame->argumentCountIncludingThis() == static_cast<size_t>(codeBlock->m_numParameters))
            codePtr = functionExecutable->generatedJITCodeForConstruct().addressForCall();
        else
            codePtr = functionExecutable->generatedJITCodeForConstructWithArityCheck();
    }
    CallLinkInfo* callLinkInfo = &stackFrame.callFrame->callerFrame()->codeBlock()->getCallLinkInfo(callFrame->returnPC());

    if (!callLinkInfo->seenOnce())
        callLinkInfo->setSeen();
    else
        JIT::linkConstruct(callee, stackFrame.callFrame->callerFrame()->codeBlock(), codeBlock, codePtr, callLinkInfo, callFrame->argumentCountIncludingThis(), stackFrame.globalData);

    return codePtr.executableAddress();
}
#endif // !ENABLE(JIT_OPTIMIZE_CALL)

DEFINE_STUB_FUNCTION(JSObject*, op_push_activation)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    JSActivation* activation = new (stackFrame.globalData) JSActivation(stackFrame.callFrame, static_cast<FunctionExecutable*>(stackFrame.callFrame->codeBlock()->ownerExecutable()));
    stackFrame.callFrame->setScopeChain(stackFrame.callFrame->scopeChain()->push(activation));
    return activation;
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_call_NotJSFunction)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    JSValue funcVal = stackFrame.args[0].jsValue();

    CallData callData;
    CallType callType = getCallData(funcVal, callData);

    ASSERT(callType != CallTypeJS);

    if (callType == CallTypeHost) {
        int registerOffset = stackFrame.args[1].int32();
        int argCount = stackFrame.args[2].int32();
        CallFrame* previousCallFrame = stackFrame.callFrame;
        CallFrame* callFrame = CallFrame::create(previousCallFrame->registers() + registerOffset);
        if (!stackFrame.registerFile->grow(callFrame->registers())) {
            throwStackOverflowError(previousCallFrame, stackFrame.globalData, callFrame->returnPC(), STUB_RETURN_ADDRESS);
            VM_THROW_EXCEPTION();
        }

        callFrame->init(0, static_cast<Instruction*>((STUB_RETURN_ADDRESS).value()), previousCallFrame->scopeChain(), previousCallFrame, argCount, asObject(funcVal));

        EncodedJSValue returnValue;
        {
            SamplingTool::HostCallRecord callRecord(CTI_SAMPLER);
            returnValue = callData.native.function(callFrame);
        }

        CHECK_FOR_EXCEPTION_AT_END();
        return returnValue;
    }

    ASSERT(callType == CallTypeNone);

    stackFrame.globalData->exception = createNotAFunctionError(stackFrame.callFrame, funcVal);
    VM_THROW_EXCEPTION();
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_create_arguments)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    Arguments* arguments = new (stackFrame.globalData) Arguments(stackFrame.callFrame);
    return JSValue::encode(JSValue(arguments));
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_create_arguments_no_params)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    Arguments* arguments = new (stackFrame.globalData) Arguments(stackFrame.callFrame, Arguments::NoParameters);
    return JSValue::encode(JSValue(arguments));
}

DEFINE_STUB_FUNCTION(void, op_tear_off_activation)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    ASSERT(stackFrame.callFrame->codeBlock()->needsFullScopeChain());
    JSValue activationValue = stackFrame.args[0].jsValue();
    if (!activationValue) {
        if (JSValue v = stackFrame.args[1].jsValue()) {
            if (!stackFrame.callFrame->codeBlock()->isStrictMode())
                asArguments(v)->copyRegisters(*stackFrame.globalData);
        }
        return;
    }
    JSActivation* activation = asActivation(stackFrame.args[0].jsValue());
    activation->copyRegisters(*stackFrame.globalData);
    if (JSValue v = stackFrame.args[1].jsValue()) {
        if (!stackFrame.callFrame->codeBlock()->isStrictMode())
            asArguments(v)->setActivation(*stackFrame.globalData, activation);
    }
}

DEFINE_STUB_FUNCTION(void, op_tear_off_arguments)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    ASSERT(stackFrame.callFrame->codeBlock()->usesArguments() && !stackFrame.callFrame->codeBlock()->needsFullScopeChain());
    asArguments(stackFrame.args[0].jsValue())->copyRegisters(*stackFrame.globalData);
}

DEFINE_STUB_FUNCTION(void, op_profile_will_call)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    ASSERT(*stackFrame.enabledProfilerReference);
    (*stackFrame.enabledProfilerReference)->willExecute(stackFrame.callFrame, stackFrame.args[0].jsValue());
}

DEFINE_STUB_FUNCTION(void, op_profile_did_call)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    ASSERT(*stackFrame.enabledProfilerReference);
    (*stackFrame.enabledProfilerReference)->didExecute(stackFrame.callFrame, stackFrame.args[0].jsValue());
}

DEFINE_STUB_FUNCTION(JSObject*, op_new_array)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    ArgList argList(&stackFrame.callFrame->registers()[stackFrame.args[0].int32()], stackFrame.args[1].int32());
    return constructArray(stackFrame.callFrame, argList);
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_resolve)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    CallFrame* callFrame = stackFrame.callFrame;
    ScopeChainNode* scopeChain = callFrame->scopeChain();

    ScopeChainIterator iter = scopeChain->begin();
    ScopeChainIterator end = scopeChain->end();
    ASSERT(iter != end);

    Identifier& ident = stackFrame.args[0].identifier();
    do {
        JSObject* o = iter->get();
        PropertySlot slot(o);
        if (o->getPropertySlot(callFrame, ident, slot)) {
            JSValue result = slot.getValue(callFrame, ident);
            CHECK_FOR_EXCEPTION_AT_END();
            return JSValue::encode(result);
        }
    } while (++iter != end);

    stackFrame.globalData->exception = createUndefinedVariableError(callFrame, ident);
    VM_THROW_EXCEPTION();
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_construct_NotJSConstruct)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    JSValue constrVal = stackFrame.args[0].jsValue();

    ConstructData constructData;
    ConstructType constructType = getConstructData(constrVal, constructData);

    ASSERT(constructType != ConstructTypeJS);

    if (constructType == ConstructTypeHost) {
        int registerOffset = stackFrame.args[1].int32();
        int argCount = stackFrame.args[2].int32();
        CallFrame* previousCallFrame = stackFrame.callFrame;
        CallFrame* callFrame = CallFrame::create(previousCallFrame->registers() + registerOffset);
        if (!stackFrame.registerFile->grow(callFrame->registers())) {
            throwStackOverflowError(previousCallFrame, stackFrame.globalData, callFrame->returnPC(), STUB_RETURN_ADDRESS);
            VM_THROW_EXCEPTION();
        }

        callFrame->init(0, static_cast<Instruction*>((STUB_RETURN_ADDRESS).value()), previousCallFrame->scopeChain(), previousCallFrame, argCount, asObject(constrVal));

        EncodedJSValue returnValue;
        {
            SamplingTool::HostCallRecord callRecord(CTI_SAMPLER);
            returnValue = constructData.native.function(callFrame);
        }

        CHECK_FOR_EXCEPTION_AT_END();
        return returnValue;
    }

    ASSERT(constructType == ConstructTypeNone);

    stackFrame.globalData->exception = createNotAConstructorError(stackFrame.callFrame, constrVal);
    VM_THROW_EXCEPTION();
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_get_by_val)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    CallFrame* callFrame = stackFrame.callFrame;
    JSGlobalData* globalData = stackFrame.globalData;

    JSValue baseValue = stackFrame.args[0].jsValue();
    JSValue subscript = stackFrame.args[1].jsValue();

    if (LIKELY(baseValue.isCell() && subscript.isString())) {
        Identifier propertyName(callFrame, asString(subscript)->value(callFrame));
        PropertySlot slot(baseValue.asCell());
        // JSString::value may have thrown, but we shouldn't find a property with a null identifier,
        // so we should miss this case and wind up in the CHECK_FOR_EXCEPTION_AT_END, below.
        if (baseValue.asCell()->fastGetOwnPropertySlot(callFrame, propertyName, slot)) {
            JSValue result = slot.getValue(callFrame, propertyName);
            CHECK_FOR_EXCEPTION();
            return JSValue::encode(result);
        }
    }

    if (subscript.isUInt32()) {
        uint32_t i = subscript.asUInt32();
        if (isJSString(globalData, baseValue) && asString(baseValue)->canGetIndex(i)) {
            ctiPatchCallByReturnAddress(callFrame->codeBlock(), STUB_RETURN_ADDRESS, FunctionPtr(cti_op_get_by_val_string));
            JSValue result = asString(baseValue)->getIndex(callFrame, i);
            CHECK_FOR_EXCEPTION();
            return JSValue::encode(result);
        }
        if (isJSByteArray(globalData, baseValue) && asByteArray(baseValue)->canAccessIndex(i)) {
            // All fast byte array accesses are safe from exceptions so return immediately to avoid exception checks.
            ctiPatchCallByReturnAddress(callFrame->codeBlock(), STUB_RETURN_ADDRESS, FunctionPtr(cti_op_get_by_val_byte_array));
            return JSValue::encode(asByteArray(baseValue)->getIndex(callFrame, i));
        }
        JSValue result = baseValue.get(callFrame, i);
        CHECK_FOR_EXCEPTION();
        return JSValue::encode(result);
    }
    
    Identifier property(callFrame, subscript.toString(callFrame));
    JSValue result = baseValue.get(callFrame, property);
    CHECK_FOR_EXCEPTION_AT_END();
    return JSValue::encode(result);
}
    
DEFINE_STUB_FUNCTION(EncodedJSValue, op_get_by_val_string)
{
    STUB_INIT_STACK_FRAME(stackFrame);
    
    CallFrame* callFrame = stackFrame.callFrame;
    JSGlobalData* globalData = stackFrame.globalData;
    
    JSValue baseValue = stackFrame.args[0].jsValue();
    JSValue subscript = stackFrame.args[1].jsValue();
    
    JSValue result;
    
    if (LIKELY(subscript.isUInt32())) {
        uint32_t i = subscript.asUInt32();
        if (isJSString(globalData, baseValue) && asString(baseValue)->canGetIndex(i))
            result = asString(baseValue)->getIndex(callFrame, i);
        else {
            result = baseValue.get(callFrame, i);
            if (!isJSString(globalData, baseValue))
                ctiPatchCallByReturnAddress(callFrame->codeBlock(), STUB_RETURN_ADDRESS, FunctionPtr(cti_op_get_by_val));
        }
    } else {
        Identifier property(callFrame, subscript.toString(callFrame));
        result = baseValue.get(callFrame, property);
    }
    
    CHECK_FOR_EXCEPTION_AT_END();
    return JSValue::encode(result);
}
    
DEFINE_STUB_FUNCTION(EncodedJSValue, op_get_by_val_byte_array)
{
    STUB_INIT_STACK_FRAME(stackFrame);
    
    CallFrame* callFrame = stackFrame.callFrame;
    JSGlobalData* globalData = stackFrame.globalData;
    
    JSValue baseValue = stackFrame.args[0].jsValue();
    JSValue subscript = stackFrame.args[1].jsValue();
    
    JSValue result;

    if (LIKELY(subscript.isUInt32())) {
        uint32_t i = subscript.asUInt32();
        if (isJSByteArray(globalData, baseValue) && asByteArray(baseValue)->canAccessIndex(i)) {
            // All fast byte array accesses are safe from exceptions so return immediately to avoid exception checks.
            return JSValue::encode(asByteArray(baseValue)->getIndex(callFrame, i));
        }

        result = baseValue.get(callFrame, i);
        if (!isJSByteArray(globalData, baseValue))
            ctiPatchCallByReturnAddress(callFrame->codeBlock(), STUB_RETURN_ADDRESS, FunctionPtr(cti_op_get_by_val));
    } else {
        Identifier property(callFrame, subscript.toString(callFrame));
        result = baseValue.get(callFrame, property);
    }
    
    CHECK_FOR_EXCEPTION_AT_END();
    return JSValue::encode(result);
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_sub)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    JSValue src1 = stackFrame.args[0].jsValue();
    JSValue src2 = stackFrame.args[1].jsValue();

    double left;
    double right;
    if (src1.getNumber(left) && src2.getNumber(right))
        return JSValue::encode(jsNumber(left - right));

    CallFrame* callFrame = stackFrame.callFrame;
    JSValue result = jsNumber(src1.toNumber(callFrame) - src2.toNumber(callFrame));
    CHECK_FOR_EXCEPTION_AT_END();
    return JSValue::encode(result);
}

DEFINE_STUB_FUNCTION(void, op_put_by_val)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    CallFrame* callFrame = stackFrame.callFrame;
    JSGlobalData* globalData = stackFrame.globalData;

    JSValue baseValue = stackFrame.args[0].jsValue();
    JSValue subscript = stackFrame.args[1].jsValue();
    JSValue value = stackFrame.args[2].jsValue();

    if (LIKELY(subscript.isUInt32())) {
        uint32_t i = subscript.asUInt32();
        if (isJSArray(globalData, baseValue)) {
            JSArray* jsArray = asArray(baseValue);
            if (jsArray->canSetIndex(i))
                jsArray->setIndex(*globalData, i, value);
            else
                jsArray->JSArray::put(callFrame, i, value);
        } else if (isJSByteArray(globalData, baseValue) && asByteArray(baseValue)->canAccessIndex(i)) {
            JSByteArray* jsByteArray = asByteArray(baseValue);
            ctiPatchCallByReturnAddress(callFrame->codeBlock(), STUB_RETURN_ADDRESS, FunctionPtr(cti_op_put_by_val_byte_array));
            // All fast byte array accesses are safe from exceptions so return immediately to avoid exception checks.
            if (value.isInt32()) {
                jsByteArray->setIndex(i, value.asInt32());
                return;
            } else {
                double dValue = 0;
                if (value.getNumber(dValue)) {
                    jsByteArray->setIndex(i, dValue);
                    return;
                }
            }

            baseValue.put(callFrame, i, value);
        } else
            baseValue.put(callFrame, i, value);
    } else {
        Identifier property(callFrame, subscript.toString(callFrame));
        if (!stackFrame.globalData->exception) { // Don't put to an object if toString threw an exception.
            PutPropertySlot slot(callFrame->codeBlock()->isStrictMode());
            baseValue.put(callFrame, property, value, slot);
        }
    }

    CHECK_FOR_EXCEPTION_AT_END();
}

DEFINE_STUB_FUNCTION(void, op_put_by_val_byte_array)
{
    STUB_INIT_STACK_FRAME(stackFrame);
    
    CallFrame* callFrame = stackFrame.callFrame;
    JSGlobalData* globalData = stackFrame.globalData;
    
    JSValue baseValue = stackFrame.args[0].jsValue();
    JSValue subscript = stackFrame.args[1].jsValue();
    JSValue value = stackFrame.args[2].jsValue();
    
    if (LIKELY(subscript.isUInt32())) {
        uint32_t i = subscript.asUInt32();
        if (isJSByteArray(globalData, baseValue) && asByteArray(baseValue)->canAccessIndex(i)) {
            JSByteArray* jsByteArray = asByteArray(baseValue);
            
            // All fast byte array accesses are safe from exceptions so return immediately to avoid exception checks.
            if (value.isInt32()) {
                jsByteArray->setIndex(i, value.asInt32());
                return;
            } else {
                double dValue = 0;                
                if (value.getNumber(dValue)) {
                    jsByteArray->setIndex(i, dValue);
                    return;
                }
            }
        }

        if (!isJSByteArray(globalData, baseValue))
            ctiPatchCallByReturnAddress(callFrame->codeBlock(), STUB_RETURN_ADDRESS, FunctionPtr(cti_op_put_by_val));
        baseValue.put(callFrame, i, value);
    } else {
        Identifier property(callFrame, subscript.toString(callFrame));
        if (!stackFrame.globalData->exception) { // Don't put to an object if toString threw an exception.
            PutPropertySlot slot(callFrame->codeBlock()->isStrictMode());
            baseValue.put(callFrame, property, value, slot);
        }
    }
    
    CHECK_FOR_EXCEPTION_AT_END();
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_lesseq)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    CallFrame* callFrame = stackFrame.callFrame;
    JSValue result = jsBoolean(jsLessEq(callFrame, stackFrame.args[0].jsValue(), stackFrame.args[1].jsValue()));
    CHECK_FOR_EXCEPTION_AT_END();
    return JSValue::encode(result);
}

DEFINE_STUB_FUNCTION(int, op_load_varargs)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    CallFrame* callFrame = stackFrame.callFrame;
    RegisterFile* registerFile = stackFrame.registerFile;
    int argsOffset = stackFrame.args[0].int32();
    JSValue arguments = callFrame->registers()[argsOffset].jsValue();
    uint32_t argCount = 0;
    if (!arguments) {
        int providedParams = callFrame->registers()[RegisterFile::ArgumentCount].i() - 1;
        argCount = providedParams;
        argCount = min(argCount, static_cast<uint32_t>(Arguments::MaxArguments));
        int32_t sizeDelta = argsOffset + argCount + RegisterFile::CallFrameHeaderSize;
        Register* newEnd = callFrame->registers() + sizeDelta;
        if (!registerFile->grow(newEnd) || ((newEnd - callFrame->registers()) != sizeDelta)) {
            stackFrame.globalData->exception = createStackOverflowError(callFrame);
            VM_THROW_EXCEPTION();
        }
        int32_t expectedParams = asFunction(callFrame->callee())->jsExecutable()->parameterCount();
        int32_t inplaceArgs = min(providedParams, expectedParams);
        
        Register* inplaceArgsDst = callFrame->registers() + argsOffset;

        Register* inplaceArgsEnd = inplaceArgsDst + inplaceArgs;
        Register* inplaceArgsEnd2 = inplaceArgsDst + providedParams;

        Register* inplaceArgsSrc = callFrame->registers() - RegisterFile::CallFrameHeaderSize - expectedParams;
        Register* inplaceArgsSrc2 = inplaceArgsSrc - providedParams - 1 + inplaceArgs;
 
        // First step is to copy the "expected" parameters from their normal location relative to the callframe
        while (inplaceArgsDst < inplaceArgsEnd)
            *inplaceArgsDst++ = *inplaceArgsSrc++;

        // Then we copy any additional arguments that may be further up the stack ('-1' to account for 'this')
        while (inplaceArgsDst < inplaceArgsEnd2)
            *inplaceArgsDst++ = *inplaceArgsSrc2++;

    } else if (!arguments.isUndefinedOrNull()) {
        if (!arguments.isObject()) {
            stackFrame.globalData->exception = createInvalidParamError(callFrame, "Function.prototype.apply", arguments);
            VM_THROW_EXCEPTION();
        }
        if (asObject(arguments)->classInfo() == &Arguments::s_info) {
            Arguments* argsObject = asArguments(arguments);
            argCount = argsObject->numProvidedArguments(callFrame);
            argCount = min(argCount, static_cast<uint32_t>(Arguments::MaxArguments));
            int32_t sizeDelta = argsOffset + argCount + RegisterFile::CallFrameHeaderSize;
            Register* newEnd = callFrame->registers() + sizeDelta;
            if (!registerFile->grow(newEnd) || ((newEnd - callFrame->registers()) != sizeDelta)) {
                stackFrame.globalData->exception = createStackOverflowError(callFrame);
                VM_THROW_EXCEPTION();
            }
            argsObject->copyToRegisters(callFrame, callFrame->registers() + argsOffset, argCount);
        } else if (isJSArray(&callFrame->globalData(), arguments)) {
            JSArray* array = asArray(arguments);
            argCount = array->length();
            argCount = min(argCount, static_cast<uint32_t>(Arguments::MaxArguments));
            int32_t sizeDelta = argsOffset + argCount + RegisterFile::CallFrameHeaderSize;
            Register* newEnd = callFrame->registers() + sizeDelta;
            if (!registerFile->grow(newEnd) || ((newEnd - callFrame->registers()) != sizeDelta)) {
                stackFrame.globalData->exception = createStackOverflowError(callFrame);
                VM_THROW_EXCEPTION();
            }
            array->copyToRegisters(callFrame, callFrame->registers() + argsOffset, argCount);
        } else if (asObject(arguments)->inherits(&JSArray::s_info)) {
            JSObject* argObject = asObject(arguments);
            argCount = argObject->get(callFrame, callFrame->propertyNames().length).toUInt32(callFrame);
            argCount = min(argCount, static_cast<uint32_t>(Arguments::MaxArguments));
            int32_t sizeDelta = argsOffset + argCount + RegisterFile::CallFrameHeaderSize;
            Register* newEnd = callFrame->registers() + sizeDelta;
            if (!registerFile->grow(newEnd) || ((newEnd - callFrame->registers()) != sizeDelta)) {
                stackFrame.globalData->exception = createStackOverflowError(callFrame);
                VM_THROW_EXCEPTION();
            }
            Register* argsBuffer = callFrame->registers() + argsOffset;
            for (unsigned i = 0; i < argCount; ++i) {
                argsBuffer[i] = asObject(arguments)->get(callFrame, i);
                CHECK_FOR_EXCEPTION();
            }
        } else {
            stackFrame.globalData->exception = createInvalidParamError(callFrame, "Function.prototype.apply", arguments);
            VM_THROW_EXCEPTION();
        }
    }

    return argCount + 1;
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_negate)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    JSValue src = stackFrame.args[0].jsValue();

    double v;
    if (src.getNumber(v))
        return JSValue::encode(jsNumber(-v));

    CallFrame* callFrame = stackFrame.callFrame;
    JSValue result = jsNumber(-src.toNumber(callFrame));
    CHECK_FOR_EXCEPTION_AT_END();
    return JSValue::encode(result);
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_resolve_base)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    return JSValue::encode(JSC::resolveBase(stackFrame.callFrame, stackFrame.args[0].identifier(), stackFrame.callFrame->scopeChain(), false));
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_resolve_base_strict_put)
{
    STUB_INIT_STACK_FRAME(stackFrame);
    JSValue base = JSC::resolveBase(stackFrame.callFrame, stackFrame.args[0].identifier(), stackFrame.callFrame->scopeChain(), true);
    if (!base) {
        stackFrame.globalData->exception = createErrorForInvalidGlobalAssignment(stackFrame.callFrame, stackFrame.args[0].identifier().ustring());
        VM_THROW_EXCEPTION();
    }
    return JSValue::encode(base);
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_ensure_property_exists)
{
    STUB_INIT_STACK_FRAME(stackFrame);
    JSValue base = stackFrame.callFrame->r(stackFrame.args[0].int32()).jsValue();
    JSObject* object = asObject(base);
    PropertySlot slot(object);
    ASSERT(stackFrame.callFrame->codeBlock()->isStrictMode());
    if (!object->getPropertySlot(stackFrame.callFrame, stackFrame.args[1].identifier(), slot)) {
        stackFrame.globalData->exception = createErrorForInvalidGlobalAssignment(stackFrame.callFrame, stackFrame.args[1].identifier().ustring());
        VM_THROW_EXCEPTION();
    }

    return JSValue::encode(base);
}
    
DEFINE_STUB_FUNCTION(EncodedJSValue, op_resolve_skip)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    CallFrame* callFrame = stackFrame.callFrame;
    ScopeChainNode* scopeChain = callFrame->scopeChain();

    int skip = stackFrame.args[1].int32();

    ScopeChainIterator iter = scopeChain->begin();
    ScopeChainIterator end = scopeChain->end();
    ASSERT(iter != end);
    CodeBlock* codeBlock = callFrame->codeBlock();
    bool checkTopLevel = codeBlock->codeType() == FunctionCode && codeBlock->needsFullScopeChain();
    ASSERT(skip || !checkTopLevel);
    if (checkTopLevel && skip--) {
        if (callFrame->uncheckedR(codeBlock->activationRegister()).jsValue())
            ++iter;
    }
    while (skip--) {
        ++iter;
        ASSERT(iter != end);
    }
    Identifier& ident = stackFrame.args[0].identifier();
    do {
        JSObject* o = iter->get();
        PropertySlot slot(o);
        if (o->getPropertySlot(callFrame, ident, slot)) {
            JSValue result = slot.getValue(callFrame, ident);
            CHECK_FOR_EXCEPTION_AT_END();
            return JSValue::encode(result);
        }
    } while (++iter != end);

    stackFrame.globalData->exception = createUndefinedVariableError(callFrame, ident);
    VM_THROW_EXCEPTION();
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_resolve_global)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    CallFrame* callFrame = stackFrame.callFrame;
    CodeBlock* codeBlock = callFrame->codeBlock();
    JSGlobalObject* globalObject = codeBlock->globalObject();
    Identifier& ident = stackFrame.args[0].identifier();
    unsigned globalResolveInfoIndex = stackFrame.args[1].int32();
    ASSERT(globalObject->isGlobalObject());

    PropertySlot slot(globalObject);
    if (globalObject->getPropertySlot(callFrame, ident, slot)) {
        JSValue result = slot.getValue(callFrame, ident);
        if (slot.isCacheableValue() && !globalObject->structure()->isUncacheableDictionary() && slot.slotBase() == globalObject) {
            GlobalResolveInfo& globalResolveInfo = codeBlock->globalResolveInfo(globalResolveInfoIndex);
            globalResolveInfo.structure.set(callFrame->globalData(), codeBlock->ownerExecutable(), globalObject->structure());
            globalResolveInfo.offset = slot.cachedOffset();
            return JSValue::encode(result);
        }

        CHECK_FOR_EXCEPTION_AT_END();
        return JSValue::encode(result);
    }

    stackFrame.globalData->exception = createUndefinedVariableError(callFrame, ident);
    VM_THROW_EXCEPTION();
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_div)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    JSValue src1 = stackFrame.args[0].jsValue();
    JSValue src2 = stackFrame.args[1].jsValue();

    double left;
    double right;
    if (src1.getNumber(left) && src2.getNumber(right))
        return JSValue::encode(jsNumber(left / right));

    CallFrame* callFrame = stackFrame.callFrame;
    JSValue result = jsNumber(src1.toNumber(callFrame) / src2.toNumber(callFrame));
    CHECK_FOR_EXCEPTION_AT_END();
    return JSValue::encode(result);
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_pre_dec)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    JSValue v = stackFrame.args[0].jsValue();

    CallFrame* callFrame = stackFrame.callFrame;
    JSValue result = jsNumber(v.toNumber(callFrame) - 1);
    CHECK_FOR_EXCEPTION_AT_END();
    return JSValue::encode(result);
}

DEFINE_STUB_FUNCTION(int, op_jless)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    JSValue src1 = stackFrame.args[0].jsValue();
    JSValue src2 = stackFrame.args[1].jsValue();
    CallFrame* callFrame = stackFrame.callFrame;

    bool result = jsLess(callFrame, src1, src2);
    CHECK_FOR_EXCEPTION_AT_END();
    return result;
}

DEFINE_STUB_FUNCTION(int, op_jlesseq)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    JSValue src1 = stackFrame.args[0].jsValue();
    JSValue src2 = stackFrame.args[1].jsValue();
    CallFrame* callFrame = stackFrame.callFrame;

    bool result = jsLessEq(callFrame, src1, src2);
    CHECK_FOR_EXCEPTION_AT_END();
    return result;
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_not)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    JSValue src = stackFrame.args[0].jsValue();

    CallFrame* callFrame = stackFrame.callFrame;

    JSValue result = jsBoolean(!src.toBoolean(callFrame));
    CHECK_FOR_EXCEPTION_AT_END();
    return JSValue::encode(result);
}

DEFINE_STUB_FUNCTION(int, op_jtrue)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    JSValue src1 = stackFrame.args[0].jsValue();

    CallFrame* callFrame = stackFrame.callFrame;

    bool result = src1.toBoolean(callFrame);
    CHECK_FOR_EXCEPTION_AT_END();
    return result;
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_post_inc)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    JSValue v = stackFrame.args[0].jsValue();

    CallFrame* callFrame = stackFrame.callFrame;

    JSValue number = v.toJSNumber(callFrame);
    CHECK_FOR_EXCEPTION_AT_END();

    callFrame->registers()[stackFrame.args[1].int32()] = jsNumber(number.uncheckedGetNumber() + 1);
    return JSValue::encode(number);
}

DEFINE_STUB_FUNCTION(int, op_eq)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    JSValue src1 = stackFrame.args[0].jsValue();
    JSValue src2 = stackFrame.args[1].jsValue();

#if USE(JSVALUE32_64)
    start:
    if (src2.isUndefined()) {
        return src1.isNull() || 
               (src1.isCell() && src1.asCell()->structure()->typeInfo().masqueradesAsUndefined())
               || src1.isUndefined();
    }
    
    if (src2.isNull()) {
        return src1.isUndefined() || 
               (src1.isCell() && src1.asCell()->structure()->typeInfo().masqueradesAsUndefined())
               || src1.isNull();
    }

    if (src1.isInt32()) {
        if (src2.isDouble())
            return src1.asInt32() == src2.asDouble();
        double d = src2.toNumber(stackFrame.callFrame);
        CHECK_FOR_EXCEPTION();
        return src1.asInt32() == d;
    }

    if (src1.isDouble()) {
        if (src2.isInt32())
            return src1.asDouble() == src2.asInt32();
        double d = src2.toNumber(stackFrame.callFrame);
        CHECK_FOR_EXCEPTION();
        return src1.asDouble() == d;
    }

    if (src1.isTrue()) {
        if (src2.isFalse())
            return false;
        double d = src2.toNumber(stackFrame.callFrame);
        CHECK_FOR_EXCEPTION();
        return d == 1.0;
    }

    if (src1.isFalse()) {
        if (src2.isTrue())
            return false;
        double d = src2.toNumber(stackFrame.callFrame);
        CHECK_FOR_EXCEPTION();
        return d == 0.0;
    }
    
    if (src1.isUndefined())
        return src2.isCell() && src2.asCell()->structure()->typeInfo().masqueradesAsUndefined();
    
    if (src1.isNull())
        return src2.isCell() && src2.asCell()->structure()->typeInfo().masqueradesAsUndefined();

    JSCell* cell1 = src1.asCell();

    if (cell1->isString()) {
        if (src2.isInt32())
            return jsToNumber(static_cast<JSString*>(cell1)->value(stackFrame.callFrame)) == src2.asInt32();
            
        if (src2.isDouble())
            return jsToNumber(static_cast<JSString*>(cell1)->value(stackFrame.callFrame)) == src2.asDouble();

        if (src2.isTrue())
            return jsToNumber(static_cast<JSString*>(cell1)->value(stackFrame.callFrame)) == 1.0;

        if (src2.isFalse())
            return jsToNumber(static_cast<JSString*>(cell1)->value(stackFrame.callFrame)) == 0.0;

        JSCell* cell2 = src2.asCell();
        if (cell2->isString())
            return static_cast<JSString*>(cell1)->value(stackFrame.callFrame) == static_cast<JSString*>(cell2)->value(stackFrame.callFrame);

        src2 = asObject(cell2)->toPrimitive(stackFrame.callFrame);
        CHECK_FOR_EXCEPTION();
        goto start;
    }

    if (src2.isObject())
        return asObject(cell1) == asObject(src2);
    src1 = asObject(cell1)->toPrimitive(stackFrame.callFrame);
    CHECK_FOR_EXCEPTION();
    goto start;
    
#else // USE(JSVALUE32_64)
    CallFrame* callFrame = stackFrame.callFrame;
    
    bool result = JSValue::equalSlowCaseInline(callFrame, src1, src2);
    CHECK_FOR_EXCEPTION_AT_END();
    return result;
#endif // USE(JSVALUE32_64)
}

DEFINE_STUB_FUNCTION(int, op_eq_strings)
{
#if USE(JSVALUE32_64)
    STUB_INIT_STACK_FRAME(stackFrame);

    JSString* string1 = stackFrame.args[0].jsString();
    JSString* string2 = stackFrame.args[1].jsString();

    ASSERT(string1->isString());
    ASSERT(string2->isString());
    return string1->value(stackFrame.callFrame) == string2->value(stackFrame.callFrame);
#else
    UNUSED_PARAM(args);
    ASSERT_NOT_REACHED();
    return 0;
#endif
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_lshift)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    JSValue val = stackFrame.args[0].jsValue();
    JSValue shift = stackFrame.args[1].jsValue();

    CallFrame* callFrame = stackFrame.callFrame;
    JSValue result = jsNumber((val.toInt32(callFrame)) << (shift.toUInt32(callFrame) & 0x1f));
    CHECK_FOR_EXCEPTION_AT_END();
    return JSValue::encode(result);
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_bitand)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    JSValue src1 = stackFrame.args[0].jsValue();
    JSValue src2 = stackFrame.args[1].jsValue();

    ASSERT(!src1.isInt32() || !src2.isInt32());
    CallFrame* callFrame = stackFrame.callFrame;
    JSValue result = jsNumber(src1.toInt32(callFrame) & src2.toInt32(callFrame));
    CHECK_FOR_EXCEPTION_AT_END();
    return JSValue::encode(result);
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_rshift)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    JSValue val = stackFrame.args[0].jsValue();
    JSValue shift = stackFrame.args[1].jsValue();

    CallFrame* callFrame = stackFrame.callFrame;
    JSValue result = jsNumber((val.toInt32(callFrame)) >> (shift.toUInt32(callFrame) & 0x1f));

    CHECK_FOR_EXCEPTION_AT_END();
    return JSValue::encode(result);
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_bitnot)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    JSValue src = stackFrame.args[0].jsValue();

    ASSERT(!src.isInt32());
    CallFrame* callFrame = stackFrame.callFrame;
    JSValue result = jsNumber(~src.toInt32(callFrame));
    CHECK_FOR_EXCEPTION_AT_END();
    return JSValue::encode(result);
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_resolve_with_base)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    CallFrame* callFrame = stackFrame.callFrame;
    ScopeChainNode* scopeChain = callFrame->scopeChain();

    ScopeChainIterator iter = scopeChain->begin();
    ScopeChainIterator end = scopeChain->end();

    // FIXME: add scopeDepthIsZero optimization

    ASSERT(iter != end);

    Identifier& ident = stackFrame.args[0].identifier();
    JSObject* base;
    do {
        base = iter->get();
        PropertySlot slot(base);
        if (base->getPropertySlot(callFrame, ident, slot)) {
            JSValue result = slot.getValue(callFrame, ident);
            CHECK_FOR_EXCEPTION_AT_END();

            callFrame->registers()[stackFrame.args[1].int32()] = JSValue(base);
            return JSValue::encode(result);
        }
        ++iter;
    } while (iter != end);

    stackFrame.globalData->exception = createUndefinedVariableError(callFrame, ident);
    VM_THROW_EXCEPTION_AT_END();
    return JSValue::encode(JSValue());
}

DEFINE_STUB_FUNCTION(JSObject*, op_new_func_exp)
{
    STUB_INIT_STACK_FRAME(stackFrame);
    CallFrame* callFrame = stackFrame.callFrame;

    FunctionExecutable* function = stackFrame.args[0].function();
    JSFunction* func = function->make(callFrame, callFrame->scopeChain());
    ASSERT(callFrame->codeBlock()->codeType() != FunctionCode || !callFrame->codeBlock()->needsFullScopeChain() || callFrame->uncheckedR(callFrame->codeBlock()->activationRegister()).jsValue());

    /* 
        The Identifier in a FunctionExpression can be referenced from inside
        the FunctionExpression's FunctionBody to allow the function to call
        itself recursively. However, unlike in a FunctionDeclaration, the
        Identifier in a FunctionExpression cannot be referenced from and
        does not affect the scope enclosing the FunctionExpression.
     */
    if (!function->name().isNull()) {
        JSStaticScopeObject* functionScopeObject = new (callFrame) JSStaticScopeObject(callFrame, function->name(), func, ReadOnly | DontDelete);
        func->setScope(callFrame->globalData(), func->scope()->push(functionScopeObject));
    }

    return func;
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_mod)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    JSValue dividendValue = stackFrame.args[0].jsValue();
    JSValue divisorValue = stackFrame.args[1].jsValue();

    CallFrame* callFrame = stackFrame.callFrame;
    double d = dividendValue.toNumber(callFrame);
    JSValue result = jsNumber(fmod(d, divisorValue.toNumber(callFrame)));
    CHECK_FOR_EXCEPTION_AT_END();
    return JSValue::encode(result);
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_less)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    CallFrame* callFrame = stackFrame.callFrame;
    JSValue result = jsBoolean(jsLess(callFrame, stackFrame.args[0].jsValue(), stackFrame.args[1].jsValue()));
    CHECK_FOR_EXCEPTION_AT_END();
    return JSValue::encode(result);
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_post_dec)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    JSValue v = stackFrame.args[0].jsValue();

    CallFrame* callFrame = stackFrame.callFrame;

    JSValue number = v.toJSNumber(callFrame);
    CHECK_FOR_EXCEPTION_AT_END();

    callFrame->registers()[stackFrame.args[1].int32()] = jsNumber(number.uncheckedGetNumber() - 1);
    return JSValue::encode(number);
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_urshift)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    JSValue val = stackFrame.args[0].jsValue();
    JSValue shift = stackFrame.args[1].jsValue();

    CallFrame* callFrame = stackFrame.callFrame;
    JSValue result = jsNumber((val.toUInt32(callFrame)) >> (shift.toUInt32(callFrame) & 0x1f));
    CHECK_FOR_EXCEPTION_AT_END();
    return JSValue::encode(result);
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_bitxor)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    JSValue src1 = stackFrame.args[0].jsValue();
    JSValue src2 = stackFrame.args[1].jsValue();

    CallFrame* callFrame = stackFrame.callFrame;

    JSValue result = jsNumber(src1.toInt32(callFrame) ^ src2.toInt32(callFrame));
    CHECK_FOR_EXCEPTION_AT_END();
    return JSValue::encode(result);
}

DEFINE_STUB_FUNCTION(JSObject*, op_new_regexp)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    CallFrame* callFrame = stackFrame.callFrame;

    RegExp* regExp = stackFrame.args[0].regExp();
    if (!regExp->isValid()) {
        stackFrame.globalData->exception = createSyntaxError(callFrame, "Invalid flags supplied to RegExp constructor.");
        VM_THROW_EXCEPTION();
    }

    return new (stackFrame.globalData) RegExpObject(stackFrame.callFrame->lexicalGlobalObject(), stackFrame.callFrame->lexicalGlobalObject()->regExpStructure(), regExp);
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_bitor)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    JSValue src1 = stackFrame.args[0].jsValue();
    JSValue src2 = stackFrame.args[1].jsValue();

    CallFrame* callFrame = stackFrame.callFrame;

    JSValue result = jsNumber(src1.toInt32(callFrame) | src2.toInt32(callFrame));
    CHECK_FOR_EXCEPTION_AT_END();
    return JSValue::encode(result);
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_call_eval)
{
    STUB_INIT_STACK_FRAME(stackFrame);
    ASSERT(stackFrame.callFrame->codeBlock()->codeType() != FunctionCode || !stackFrame.callFrame->codeBlock()->needsFullScopeChain() || stackFrame.callFrame->uncheckedR(stackFrame.callFrame->codeBlock()->activationRegister()).jsValue());

    CallFrame* callFrame = stackFrame.callFrame;
    RegisterFile* registerFile = stackFrame.registerFile;

    Interpreter* interpreter = stackFrame.globalData->interpreter;
    
    JSValue funcVal = stackFrame.args[0].jsValue();
    int registerOffset = stackFrame.args[1].int32();
    int argCount = stackFrame.args[2].int32();

    Register* newCallFrame = callFrame->registers() + registerOffset;
    Register* argv = newCallFrame - RegisterFile::CallFrameHeaderSize - argCount;
    JSValue baseValue = argv[0].jsValue();
    JSGlobalObject* globalObject = callFrame->scopeChain()->globalObject.get();

    if (baseValue == globalObject && funcVal == globalObject->evalFunction()) {
        JSValue result = interpreter->callEval(callFrame, registerFile, argv, argCount, registerOffset);
        CHECK_FOR_EXCEPTION_AT_END();
        return JSValue::encode(result);
    }

    return JSValue::encode(JSValue());
}

DEFINE_STUB_FUNCTION(void*, op_throw)
{
    STUB_INIT_STACK_FRAME(stackFrame);
    ExceptionHandler handler = jitThrow(stackFrame.globalData, stackFrame.callFrame, stackFrame.args[0].jsValue(), STUB_RETURN_ADDRESS);
    STUB_SET_RETURN_ADDRESS(handler.catchRoutine);
    return handler.callFrame;
}

DEFINE_STUB_FUNCTION(JSPropertyNameIterator*, op_get_pnames)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    CallFrame* callFrame = stackFrame.callFrame;
    JSObject* o = stackFrame.args[0].jsObject();
    Structure* structure = o->structure();
    JSPropertyNameIterator* jsPropertyNameIterator = structure->enumerationCache();
    if (!jsPropertyNameIterator || jsPropertyNameIterator->cachedPrototypeChain() != structure->prototypeChain(callFrame))
        jsPropertyNameIterator = JSPropertyNameIterator::create(callFrame, o);
    return jsPropertyNameIterator;
}

DEFINE_STUB_FUNCTION(int, has_property)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    JSObject* base = stackFrame.args[0].jsObject();
    JSString* property = stackFrame.args[1].jsString();
    int result = base->hasProperty(stackFrame.callFrame, Identifier(stackFrame.callFrame, property->value(stackFrame.callFrame)));
    CHECK_FOR_EXCEPTION_AT_END();
    return result;
}

DEFINE_STUB_FUNCTION(JSObject*, op_push_scope)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    JSObject* o = stackFrame.args[0].jsValue().toObject(stackFrame.callFrame);
    CHECK_FOR_EXCEPTION();
    stackFrame.callFrame->setScopeChain(stackFrame.callFrame->scopeChain()->push(o));
    return o;
}

DEFINE_STUB_FUNCTION(void, op_pop_scope)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    stackFrame.callFrame->setScopeChain(stackFrame.callFrame->scopeChain()->pop());
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_typeof)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    return JSValue::encode(jsTypeStringForValue(stackFrame.callFrame, stackFrame.args[0].jsValue()));
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_is_undefined)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    JSValue v = stackFrame.args[0].jsValue();
    return JSValue::encode(jsBoolean(v.isCell() ? v.asCell()->structure()->typeInfo().masqueradesAsUndefined() : v.isUndefined()));
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_is_boolean)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    return JSValue::encode(jsBoolean(stackFrame.args[0].jsValue().isBoolean()));
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_is_number)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    return JSValue::encode(jsBoolean(stackFrame.args[0].jsValue().isNumber()));
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_is_string)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    return JSValue::encode(jsBoolean(isJSString(stackFrame.globalData, stackFrame.args[0].jsValue())));
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_is_object)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    return JSValue::encode(jsBoolean(jsIsObjectType(stackFrame.args[0].jsValue())));
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_is_function)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    return JSValue::encode(jsBoolean(jsIsFunctionType(stackFrame.args[0].jsValue())));
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_stricteq)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    JSValue src1 = stackFrame.args[0].jsValue();
    JSValue src2 = stackFrame.args[1].jsValue();

    bool result = JSValue::strictEqual(stackFrame.callFrame, src1, src2);
    CHECK_FOR_EXCEPTION_AT_END();
    return JSValue::encode(jsBoolean(result));
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_to_primitive)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    return JSValue::encode(stackFrame.args[0].jsValue().toPrimitive(stackFrame.callFrame));
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_strcat)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    JSValue result = jsString(stackFrame.callFrame, &stackFrame.callFrame->registers()[stackFrame.args[0].int32()], stackFrame.args[1].int32());
    CHECK_FOR_EXCEPTION_AT_END();
    return JSValue::encode(result);
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_nstricteq)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    JSValue src1 = stackFrame.args[0].jsValue();
    JSValue src2 = stackFrame.args[1].jsValue();

    bool result = !JSValue::strictEqual(stackFrame.callFrame, src1, src2);
    CHECK_FOR_EXCEPTION_AT_END();
    return JSValue::encode(jsBoolean(result));
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_to_jsnumber)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    JSValue src = stackFrame.args[0].jsValue();
    CallFrame* callFrame = stackFrame.callFrame;

    JSValue result = src.toJSNumber(callFrame);
    CHECK_FOR_EXCEPTION_AT_END();
    return JSValue::encode(result);
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_in)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    CallFrame* callFrame = stackFrame.callFrame;
    JSValue baseVal = stackFrame.args[1].jsValue();

    if (!baseVal.isObject()) {
        stackFrame.globalData->exception = createInvalidParamError(stackFrame.callFrame, "in", baseVal);
        VM_THROW_EXCEPTION();
    }

    JSValue propName = stackFrame.args[0].jsValue();
    JSObject* baseObj = asObject(baseVal);

    uint32_t i;
    if (propName.getUInt32(i))
        return JSValue::encode(jsBoolean(baseObj->hasProperty(callFrame, i)));

    Identifier property(callFrame, propName.toString(callFrame));
    CHECK_FOR_EXCEPTION();
    return JSValue::encode(jsBoolean(baseObj->hasProperty(callFrame, property)));
}

DEFINE_STUB_FUNCTION(JSObject*, op_push_new_scope)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    JSObject* scope = new (stackFrame.globalData) JSStaticScopeObject(stackFrame.callFrame, stackFrame.args[0].identifier(), stackFrame.args[1].jsValue(), DontDelete);

    CallFrame* callFrame = stackFrame.callFrame;
    callFrame->setScopeChain(callFrame->scopeChain()->push(scope));
    return scope;
}

DEFINE_STUB_FUNCTION(void, op_jmp_scopes)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    unsigned count = stackFrame.args[0].int32();
    CallFrame* callFrame = stackFrame.callFrame;

    ScopeChainNode* tmp = callFrame->scopeChain();
    while (count--)
        tmp = tmp->pop();
    callFrame->setScopeChain(tmp);
}

DEFINE_STUB_FUNCTION(void, op_put_by_index)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    CallFrame* callFrame = stackFrame.callFrame;
    unsigned property = stackFrame.args[1].int32();

    stackFrame.args[0].jsValue().put(callFrame, property, stackFrame.args[2].jsValue());
}

DEFINE_STUB_FUNCTION(void*, op_switch_imm)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    JSValue scrutinee = stackFrame.args[0].jsValue();
    unsigned tableIndex = stackFrame.args[1].int32();
    CallFrame* callFrame = stackFrame.callFrame;
    CodeBlock* codeBlock = callFrame->codeBlock();

    if (scrutinee.isInt32())
        return codeBlock->immediateSwitchJumpTable(tableIndex).ctiForValue(scrutinee.asInt32()).executableAddress();
    else {
        double value;
        int32_t intValue;
        if (scrutinee.getNumber(value) && ((intValue = static_cast<int32_t>(value)) == value))
            return codeBlock->immediateSwitchJumpTable(tableIndex).ctiForValue(intValue).executableAddress();
        else
            return codeBlock->immediateSwitchJumpTable(tableIndex).ctiDefault.executableAddress();
    }
}

DEFINE_STUB_FUNCTION(void*, op_switch_char)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    JSValue scrutinee = stackFrame.args[0].jsValue();
    unsigned tableIndex = stackFrame.args[1].int32();
    CallFrame* callFrame = stackFrame.callFrame;
    CodeBlock* codeBlock = callFrame->codeBlock();

    void* result = codeBlock->characterSwitchJumpTable(tableIndex).ctiDefault.executableAddress();

    if (scrutinee.isString()) {
        StringImpl* value = asString(scrutinee)->value(callFrame).impl();
        if (value->length() == 1)
            result = codeBlock->characterSwitchJumpTable(tableIndex).ctiForValue(value->characters()[0]).executableAddress();
    }

    CHECK_FOR_EXCEPTION_AT_END();
    return result;
}

DEFINE_STUB_FUNCTION(void*, op_switch_string)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    JSValue scrutinee = stackFrame.args[0].jsValue();
    unsigned tableIndex = stackFrame.args[1].int32();
    CallFrame* callFrame = stackFrame.callFrame;
    CodeBlock* codeBlock = callFrame->codeBlock();

    void* result = codeBlock->stringSwitchJumpTable(tableIndex).ctiDefault.executableAddress();

    if (scrutinee.isString()) {
        StringImpl* value = asString(scrutinee)->value(callFrame).impl();
        result = codeBlock->stringSwitchJumpTable(tableIndex).ctiForValue(value).executableAddress();
    }

    CHECK_FOR_EXCEPTION_AT_END();
    return result;
}

DEFINE_STUB_FUNCTION(EncodedJSValue, op_del_by_val)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    CallFrame* callFrame = stackFrame.callFrame;

    JSValue baseValue = stackFrame.args[0].jsValue();
    JSObject* baseObj = baseValue.toObject(callFrame); // may throw

    JSValue subscript = stackFrame.args[1].jsValue();
    bool result;
    uint32_t i;
    if (subscript.getUInt32(i))
        result = baseObj->deleteProperty(callFrame, i);
    else {
        CHECK_FOR_EXCEPTION();
        Identifier property(callFrame, subscript.toString(callFrame));
        CHECK_FOR_EXCEPTION();
        result = baseObj->deleteProperty(callFrame, property);
    }

    if (!result && callFrame->codeBlock()->isStrictMode())
        stackFrame.globalData->exception = createTypeError(stackFrame.callFrame, "Unable to delete property.");

    CHECK_FOR_EXCEPTION_AT_END();
    return JSValue::encode(jsBoolean(result));
}

DEFINE_STUB_FUNCTION(void, op_put_getter)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    CallFrame* callFrame = stackFrame.callFrame;

    ASSERT(stackFrame.args[0].jsValue().isObject());
    JSObject* baseObj = asObject(stackFrame.args[0].jsValue());
    ASSERT(stackFrame.args[2].jsValue().isObject());
    baseObj->defineGetter(callFrame, stackFrame.args[1].identifier(), asObject(stackFrame.args[2].jsValue()));
}

DEFINE_STUB_FUNCTION(void, op_put_setter)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    CallFrame* callFrame = stackFrame.callFrame;

    ASSERT(stackFrame.args[0].jsValue().isObject());
    JSObject* baseObj = asObject(stackFrame.args[0].jsValue());
    ASSERT(stackFrame.args[2].jsValue().isObject());
    baseObj->defineSetter(callFrame, stackFrame.args[1].identifier(), asObject(stackFrame.args[2].jsValue()));
}

DEFINE_STUB_FUNCTION(void, op_throw_reference_error)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    CallFrame* callFrame = stackFrame.callFrame;
    UString message = stackFrame.args[0].jsValue().toString(callFrame);
    stackFrame.globalData->exception = createReferenceError(callFrame, message);
    VM_THROW_EXCEPTION_AT_END();
}

DEFINE_STUB_FUNCTION(void, op_debug)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    CallFrame* callFrame = stackFrame.callFrame;

    int debugHookID = stackFrame.args[0].int32();
    int firstLine = stackFrame.args[1].int32();
    int lastLine = stackFrame.args[2].int32();

    stackFrame.globalData->interpreter->debug(callFrame, static_cast<DebugHookID>(debugHookID), firstLine, lastLine);
}

DEFINE_STUB_FUNCTION(void*, vm_throw)
{
    STUB_INIT_STACK_FRAME(stackFrame);
    JSGlobalData* globalData = stackFrame.globalData;
    ExceptionHandler handler = jitThrow(globalData, stackFrame.callFrame, globalData->exception, globalData->exceptionLocation);
    STUB_SET_RETURN_ADDRESS(handler.catchRoutine);
    return handler.callFrame;
}

DEFINE_STUB_FUNCTION(EncodedJSValue, to_object)
{
    STUB_INIT_STACK_FRAME(stackFrame);

    CallFrame* callFrame = stackFrame.callFrame;
    return JSValue::encode(stackFrame.args[0].jsValue().toObject(callFrame));
}

MacroAssemblerCodePtr JITThunks::ctiStub(JSGlobalData* globalData, ThunkGenerator generator)
{
    std::pair<CTIStubMap::iterator, bool> entry = m_ctiStubMap.add(generator, MacroAssemblerCodePtr());
    if (entry.second)
        entry.first->second = generator(globalData, m_executablePool.get());
    return entry.first->second;
}

NativeExecutable* JITThunks::hostFunctionStub(JSGlobalData* globalData, NativeFunction function)
{
    std::pair<HostFunctionStubMap::iterator, bool> entry = m_hostFunctionStubMap->add(function, Weak<NativeExecutable>());
    if (!*entry.first->second)
        entry.first->second.set(*globalData, NativeExecutable::create(*globalData, JIT::compileCTINativeCall(globalData, m_executablePool, function), function, ctiNativeConstruct(), callHostFunctionAsConstructor));
    return entry.first->second.get();
}

NativeExecutable* JITThunks::hostFunctionStub(JSGlobalData* globalData, NativeFunction function, ThunkGenerator generator)
{
    std::pair<HostFunctionStubMap::iterator, bool> entry = m_hostFunctionStubMap->add(function, Weak<NativeExecutable>());
    if (!*entry.first->second) {
        MacroAssemblerCodePtr code = globalData->canUseJIT() ? generator(globalData, m_executablePool.get()) : MacroAssemblerCodePtr();
        entry.first->second.set(*globalData, NativeExecutable::create(*globalData, code, function, ctiNativeConstruct(), callHostFunctionAsConstructor));
    }
    return entry.first->second.get();
}

void JITThunks::clearHostFunctionStubs()
{
    m_hostFunctionStubMap.clear();
}

} // namespace JSC

#endif // ENABLE(JIT)
