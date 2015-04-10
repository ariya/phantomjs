/*
 * Copyright (C) 2008, 2012 Apple Inc. All rights reserved.
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

#ifndef X86Assembler_h
#define X86Assembler_h

#if ENABLE(ASSEMBLER) && (CPU(X86) || CPU(X86_64))

#include "AssemblerBuffer.h"
#include "JITCompilationEffort.h"
#include <stdint.h>
#include <wtf/Assertions.h>
#include <wtf/Vector.h>

namespace JSC {

inline bool CAN_SIGN_EXTEND_8_32(int32_t value) { return value == (int32_t)(signed char)value; }

namespace X86Registers {
    typedef enum {
        eax,
        ecx,
        edx,
        ebx,
        esp,
        ebp,
        esi,
        edi,

#if CPU(X86_64)
        r8,
        r9,
        r10,
        r11,
        r12,
        r13,
        r14,
        r15,
#endif
    } RegisterID;

    typedef enum {
        xmm0,
        xmm1,
        xmm2,
        xmm3,
        xmm4,
        xmm5,
        xmm6,
        xmm7,
    } XMMRegisterID;
}

class X86Assembler {
public:
    typedef X86Registers::RegisterID RegisterID;
    typedef X86Registers::XMMRegisterID XMMRegisterID;
    typedef XMMRegisterID FPRegisterID;

    typedef enum {
        ConditionO,
        ConditionNO,
        ConditionB,
        ConditionAE,
        ConditionE,
        ConditionNE,
        ConditionBE,
        ConditionA,
        ConditionS,
        ConditionNS,
        ConditionP,
        ConditionNP,
        ConditionL,
        ConditionGE,
        ConditionLE,
        ConditionG,

        ConditionC  = ConditionB,
        ConditionNC = ConditionAE,
    } Condition;

private:
    typedef enum {
        OP_ADD_EvGv                     = 0x01,
        OP_ADD_GvEv                     = 0x03,
        OP_OR_EvGv                      = 0x09,
        OP_OR_GvEv                      = 0x0B,
        OP_2BYTE_ESCAPE                 = 0x0F,
        OP_AND_EvGv                     = 0x21,
        OP_AND_GvEv                     = 0x23,
        OP_SUB_EvGv                     = 0x29,
        OP_SUB_GvEv                     = 0x2B,
        PRE_PREDICT_BRANCH_NOT_TAKEN    = 0x2E,
        OP_XOR_EvGv                     = 0x31,
        OP_XOR_GvEv                     = 0x33,
        OP_CMP_EvGv                     = 0x39,
        OP_CMP_GvEv                     = 0x3B,
#if CPU(X86_64)
        PRE_REX                         = 0x40,
#endif
        OP_PUSH_EAX                     = 0x50,
        OP_POP_EAX                      = 0x58,
#if CPU(X86_64)
        OP_MOVSXD_GvEv                  = 0x63,
#endif
        PRE_OPERAND_SIZE                = 0x66,
        PRE_SSE_66                      = 0x66,
        OP_PUSH_Iz                      = 0x68,
        OP_IMUL_GvEvIz                  = 0x69,
        OP_GROUP1_EbIb                  = 0x80,
        OP_GROUP1_EvIz                  = 0x81,
        OP_GROUP1_EvIb                  = 0x83,
        OP_TEST_EbGb                    = 0x84,
        OP_TEST_EvGv                    = 0x85,
        OP_XCHG_EvGv                    = 0x87,
        OP_MOV_EbGb                     = 0x88,
        OP_MOV_EvGv                     = 0x89,
        OP_MOV_GvEv                     = 0x8B,
        OP_LEA                          = 0x8D,
        OP_GROUP1A_Ev                   = 0x8F,
        OP_NOP                          = 0x90,
        OP_CDQ                          = 0x99,
        OP_MOV_EAXOv                    = 0xA1,
        OP_MOV_OvEAX                    = 0xA3,
        OP_MOV_EAXIv                    = 0xB8,
        OP_GROUP2_EvIb                  = 0xC1,
        OP_RET                          = 0xC3,
        OP_GROUP11_EvIb                 = 0xC6,
        OP_GROUP11_EvIz                 = 0xC7,
        OP_INT3                         = 0xCC,
        OP_GROUP2_Ev1                   = 0xD1,
        OP_GROUP2_EvCL                  = 0xD3,
        OP_ESCAPE_DD                    = 0xDD,
        OP_CALL_rel32                   = 0xE8,
        OP_JMP_rel32                    = 0xE9,
        PRE_SSE_F2                      = 0xF2,
        PRE_SSE_F3                      = 0xF3,
        OP_HLT                          = 0xF4,
        OP_GROUP3_EbIb                  = 0xF6,
        OP_GROUP3_Ev                    = 0xF7,
        OP_GROUP3_EvIz                  = 0xF7, // OP_GROUP3_Ev has an immediate, when instruction is a test. 
        OP_GROUP5_Ev                    = 0xFF,
    } OneByteOpcodeID;

    typedef enum {
        OP2_MOVSD_VsdWsd    = 0x10,
        OP2_MOVSD_WsdVsd    = 0x11,
        OP2_MOVSS_VsdWsd    = 0x10,
        OP2_MOVSS_WsdVsd    = 0x11,
        OP2_CVTSI2SD_VsdEd  = 0x2A,
        OP2_CVTTSD2SI_GdWsd = 0x2C,
        OP2_UCOMISD_VsdWsd  = 0x2E,
        OP2_ADDSD_VsdWsd    = 0x58,
        OP2_MULSD_VsdWsd    = 0x59,
        OP2_CVTSD2SS_VsdWsd = 0x5A,
        OP2_CVTSS2SD_VsdWsd = 0x5A,
        OP2_SUBSD_VsdWsd    = 0x5C,
        OP2_DIVSD_VsdWsd    = 0x5E,
        OP2_SQRTSD_VsdWsd   = 0x51,
        OP2_ANDNPD_VpdWpd   = 0x55,
        OP2_XORPD_VpdWpd    = 0x57,
        OP2_MOVD_VdEd       = 0x6E,
        OP2_MOVD_EdVd       = 0x7E,
        OP2_JCC_rel32       = 0x80,
        OP_SETCC            = 0x90,
        OP2_IMUL_GvEv       = 0xAF,
        OP2_MOVZX_GvEb      = 0xB6,
        OP2_MOVSX_GvEb      = 0xBE,
        OP2_MOVZX_GvEw      = 0xB7,
        OP2_MOVSX_GvEw      = 0xBF,
        OP2_PEXTRW_GdUdIb   = 0xC5,
        OP2_PSLLQ_UdqIb     = 0x73,
        OP2_PSRLQ_UdqIb     = 0x73,
        OP2_POR_VdqWdq      = 0XEB,
    } TwoByteOpcodeID;

    TwoByteOpcodeID jccRel32(Condition cond)
    {
        return (TwoByteOpcodeID)(OP2_JCC_rel32 + cond);
    }

    TwoByteOpcodeID setccOpcode(Condition cond)
    {
        return (TwoByteOpcodeID)(OP_SETCC + cond);
    }

    typedef enum {
        GROUP1_OP_ADD = 0,
        GROUP1_OP_OR  = 1,
        GROUP1_OP_ADC = 2,
        GROUP1_OP_AND = 4,
        GROUP1_OP_SUB = 5,
        GROUP1_OP_XOR = 6,
        GROUP1_OP_CMP = 7,

        GROUP1A_OP_POP = 0,
        
        GROUP2_OP_ROL = 0,
        GROUP2_OP_ROR = 1,
        GROUP2_OP_RCL = 2,
        GROUP2_OP_RCR = 3,
        
        GROUP2_OP_SHL = 4,
        GROUP2_OP_SHR = 5,
        GROUP2_OP_SAR = 7,

        GROUP3_OP_TEST = 0,
        GROUP3_OP_NOT  = 2,
        GROUP3_OP_NEG  = 3,
        GROUP3_OP_IDIV = 7,

        GROUP5_OP_CALLN = 2,
        GROUP5_OP_JMPN  = 4,
        GROUP5_OP_PUSH  = 6,

        GROUP11_MOV = 0,

        GROUP14_OP_PSLLQ = 6,
        GROUP14_OP_PSRLQ = 2,

        ESCAPE_DD_FSTP_doubleReal = 3,
    } GroupOpcodeID;
    
    class X86InstructionFormatter;
public:

    X86Assembler()
        : m_indexOfLastWatchpoint(INT_MIN)
        , m_indexOfTailOfLastWatchpoint(INT_MIN)
    {
    }

    // Stack operations:

    void push_r(RegisterID reg)
    {
        m_formatter.oneByteOp(OP_PUSH_EAX, reg);
    }

    void pop_r(RegisterID reg)
    {
        m_formatter.oneByteOp(OP_POP_EAX, reg);
    }

    void push_i32(int imm)
    {
        m_formatter.oneByteOp(OP_PUSH_Iz);
        m_formatter.immediate32(imm);
    }

    void push_m(int offset, RegisterID base)
    {
        m_formatter.oneByteOp(OP_GROUP5_Ev, GROUP5_OP_PUSH, base, offset);
    }

    void pop_m(int offset, RegisterID base)
    {
        m_formatter.oneByteOp(OP_GROUP1A_Ev, GROUP1A_OP_POP, base, offset);
    }

    // Arithmetic operations:

#if !CPU(X86_64)
    void adcl_im(int imm, const void* addr)
    {
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp(OP_GROUP1_EvIb, GROUP1_OP_ADC, addr);
            m_formatter.immediate8(imm);
        } else {
            m_formatter.oneByteOp(OP_GROUP1_EvIz, GROUP1_OP_ADC, addr);
            m_formatter.immediate32(imm);
        }
    }
#endif

    void addl_rr(RegisterID src, RegisterID dst)
    {
        m_formatter.oneByteOp(OP_ADD_EvGv, src, dst);
    }

    void addl_mr(int offset, RegisterID base, RegisterID dst)
    {
        m_formatter.oneByteOp(OP_ADD_GvEv, dst, base, offset);
    }
    
#if !CPU(X86_64)
    void addl_mr(const void* addr, RegisterID dst)
    {
        m_formatter.oneByteOp(OP_ADD_GvEv, dst, addr);
    }
#endif

    void addl_rm(RegisterID src, int offset, RegisterID base)
    {
        m_formatter.oneByteOp(OP_ADD_EvGv, src, base, offset);
    }

    void addl_ir(int imm, RegisterID dst)
    {
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp(OP_GROUP1_EvIb, GROUP1_OP_ADD, dst);
            m_formatter.immediate8(imm);
        } else {
            m_formatter.oneByteOp(OP_GROUP1_EvIz, GROUP1_OP_ADD, dst);
            m_formatter.immediate32(imm);
        }
    }

    void addl_im(int imm, int offset, RegisterID base)
    {
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp(OP_GROUP1_EvIb, GROUP1_OP_ADD, base, offset);
            m_formatter.immediate8(imm);
        } else {
            m_formatter.oneByteOp(OP_GROUP1_EvIz, GROUP1_OP_ADD, base, offset);
            m_formatter.immediate32(imm);
        }
    }

#if CPU(X86_64)
    void addq_rr(RegisterID src, RegisterID dst)
    {
        m_formatter.oneByteOp64(OP_ADD_EvGv, src, dst);
    }

    void addq_mr(int offset, RegisterID base, RegisterID dst)
    {
        m_formatter.oneByteOp64(OP_ADD_GvEv, dst, base, offset);
    }

    void addq_ir(int imm, RegisterID dst)
    {
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp64(OP_GROUP1_EvIb, GROUP1_OP_ADD, dst);
            m_formatter.immediate8(imm);
        } else {
            m_formatter.oneByteOp64(OP_GROUP1_EvIz, GROUP1_OP_ADD, dst);
            m_formatter.immediate32(imm);
        }
    }

    void addq_im(int imm, int offset, RegisterID base)
    {
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp64(OP_GROUP1_EvIb, GROUP1_OP_ADD, base, offset);
            m_formatter.immediate8(imm);
        } else {
            m_formatter.oneByteOp64(OP_GROUP1_EvIz, GROUP1_OP_ADD, base, offset);
            m_formatter.immediate32(imm);
        }
    }
#else
    void addl_im(int imm, const void* addr)
    {
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp(OP_GROUP1_EvIb, GROUP1_OP_ADD, addr);
            m_formatter.immediate8(imm);
        } else {
            m_formatter.oneByteOp(OP_GROUP1_EvIz, GROUP1_OP_ADD, addr);
            m_formatter.immediate32(imm);
        }
    }
#endif

    void andl_rr(RegisterID src, RegisterID dst)
    {
        m_formatter.oneByteOp(OP_AND_EvGv, src, dst);
    }

    void andl_mr(int offset, RegisterID base, RegisterID dst)
    {
        m_formatter.oneByteOp(OP_AND_GvEv, dst, base, offset);
    }

    void andl_rm(RegisterID src, int offset, RegisterID base)
    {
        m_formatter.oneByteOp(OP_AND_EvGv, src, base, offset);
    }

    void andl_ir(int imm, RegisterID dst)
    {
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp(OP_GROUP1_EvIb, GROUP1_OP_AND, dst);
            m_formatter.immediate8(imm);
        } else {
            m_formatter.oneByteOp(OP_GROUP1_EvIz, GROUP1_OP_AND, dst);
            m_formatter.immediate32(imm);
        }
    }

    void andl_im(int imm, int offset, RegisterID base)
    {
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp(OP_GROUP1_EvIb, GROUP1_OP_AND, base, offset);
            m_formatter.immediate8(imm);
        } else {
            m_formatter.oneByteOp(OP_GROUP1_EvIz, GROUP1_OP_AND, base, offset);
            m_formatter.immediate32(imm);
        }
    }

#if CPU(X86_64)
    void andq_rr(RegisterID src, RegisterID dst)
    {
        m_formatter.oneByteOp64(OP_AND_EvGv, src, dst);
    }

    void andq_ir(int imm, RegisterID dst)
    {
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp64(OP_GROUP1_EvIb, GROUP1_OP_AND, dst);
            m_formatter.immediate8(imm);
        } else {
            m_formatter.oneByteOp64(OP_GROUP1_EvIz, GROUP1_OP_AND, dst);
            m_formatter.immediate32(imm);
        }
    }
#else
    void andl_im(int imm, const void* addr)
    {
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp(OP_GROUP1_EvIb, GROUP1_OP_AND, addr);
            m_formatter.immediate8(imm);
        } else {
            m_formatter.oneByteOp(OP_GROUP1_EvIz, GROUP1_OP_AND, addr);
            m_formatter.immediate32(imm);
        }
    }
#endif

    void negl_r(RegisterID dst)
    {
        m_formatter.oneByteOp(OP_GROUP3_Ev, GROUP3_OP_NEG, dst);
    }

#if CPU(X86_64)
    void negq_r(RegisterID dst)
    {
        m_formatter.oneByteOp64(OP_GROUP3_Ev, GROUP3_OP_NEG, dst);
    }
#endif

    void negl_m(int offset, RegisterID base)
    {
        m_formatter.oneByteOp(OP_GROUP3_Ev, GROUP3_OP_NEG, base, offset);
    }

    void notl_r(RegisterID dst)
    {
        m_formatter.oneByteOp(OP_GROUP3_Ev, GROUP3_OP_NOT, dst);
    }

    void notl_m(int offset, RegisterID base)
    {
        m_formatter.oneByteOp(OP_GROUP3_Ev, GROUP3_OP_NOT, base, offset);
    }

    void orl_rr(RegisterID src, RegisterID dst)
    {
        m_formatter.oneByteOp(OP_OR_EvGv, src, dst);
    }

    void orl_mr(int offset, RegisterID base, RegisterID dst)
    {
        m_formatter.oneByteOp(OP_OR_GvEv, dst, base, offset);
    }

    void orl_rm(RegisterID src, int offset, RegisterID base)
    {
        m_formatter.oneByteOp(OP_OR_EvGv, src, base, offset);
    }

    void orl_ir(int imm, RegisterID dst)
    {
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp(OP_GROUP1_EvIb, GROUP1_OP_OR, dst);
            m_formatter.immediate8(imm);
        } else {
            m_formatter.oneByteOp(OP_GROUP1_EvIz, GROUP1_OP_OR, dst);
            m_formatter.immediate32(imm);
        }
    }

    void orl_im(int imm, int offset, RegisterID base)
    {
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp(OP_GROUP1_EvIb, GROUP1_OP_OR, base, offset);
            m_formatter.immediate8(imm);
        } else {
            m_formatter.oneByteOp(OP_GROUP1_EvIz, GROUP1_OP_OR, base, offset);
            m_formatter.immediate32(imm);
        }
    }

#if CPU(X86_64)
    void orq_rr(RegisterID src, RegisterID dst)
    {
        m_formatter.oneByteOp64(OP_OR_EvGv, src, dst);
    }

    void orq_ir(int imm, RegisterID dst)
    {
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp64(OP_GROUP1_EvIb, GROUP1_OP_OR, dst);
            m_formatter.immediate8(imm);
        } else {
            m_formatter.oneByteOp64(OP_GROUP1_EvIz, GROUP1_OP_OR, dst);
            m_formatter.immediate32(imm);
        }
    }
#else
    void orl_im(int imm, const void* addr)
    {
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp(OP_GROUP1_EvIb, GROUP1_OP_OR, addr);
            m_formatter.immediate8(imm);
        } else {
            m_formatter.oneByteOp(OP_GROUP1_EvIz, GROUP1_OP_OR, addr);
            m_formatter.immediate32(imm);
        }
    }

    void orl_rm(RegisterID src, const void* addr)
    {
        m_formatter.oneByteOp(OP_OR_EvGv, src, addr);
    }
#endif

    void subl_rr(RegisterID src, RegisterID dst)
    {
        m_formatter.oneByteOp(OP_SUB_EvGv, src, dst);
    }

    void subl_mr(int offset, RegisterID base, RegisterID dst)
    {
        m_formatter.oneByteOp(OP_SUB_GvEv, dst, base, offset);
    }

    void subl_rm(RegisterID src, int offset, RegisterID base)
    {
        m_formatter.oneByteOp(OP_SUB_EvGv, src, base, offset);
    }

    void subl_ir(int imm, RegisterID dst)
    {
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp(OP_GROUP1_EvIb, GROUP1_OP_SUB, dst);
            m_formatter.immediate8(imm);
        } else {
            m_formatter.oneByteOp(OP_GROUP1_EvIz, GROUP1_OP_SUB, dst);
            m_formatter.immediate32(imm);
        }
    }
    
    void subl_im(int imm, int offset, RegisterID base)
    {
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp(OP_GROUP1_EvIb, GROUP1_OP_SUB, base, offset);
            m_formatter.immediate8(imm);
        } else {
            m_formatter.oneByteOp(OP_GROUP1_EvIz, GROUP1_OP_SUB, base, offset);
            m_formatter.immediate32(imm);
        }
    }

#if CPU(X86_64)
    void subq_rr(RegisterID src, RegisterID dst)
    {
        m_formatter.oneByteOp64(OP_SUB_EvGv, src, dst);
    }

    void subq_ir(int imm, RegisterID dst)
    {
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp64(OP_GROUP1_EvIb, GROUP1_OP_SUB, dst);
            m_formatter.immediate8(imm);
        } else {
            m_formatter.oneByteOp64(OP_GROUP1_EvIz, GROUP1_OP_SUB, dst);
            m_formatter.immediate32(imm);
        }
    }
#else
    void subl_im(int imm, const void* addr)
    {
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp(OP_GROUP1_EvIb, GROUP1_OP_SUB, addr);
            m_formatter.immediate8(imm);
        } else {
            m_formatter.oneByteOp(OP_GROUP1_EvIz, GROUP1_OP_SUB, addr);
            m_formatter.immediate32(imm);
        }
    }
#endif

    void xorl_rr(RegisterID src, RegisterID dst)
    {
        m_formatter.oneByteOp(OP_XOR_EvGv, src, dst);
    }

    void xorl_mr(int offset, RegisterID base, RegisterID dst)
    {
        m_formatter.oneByteOp(OP_XOR_GvEv, dst, base, offset);
    }

    void xorl_rm(RegisterID src, int offset, RegisterID base)
    {
        m_formatter.oneByteOp(OP_XOR_EvGv, src, base, offset);
    }

    void xorl_im(int imm, int offset, RegisterID base)
    {
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp(OP_GROUP1_EvIb, GROUP1_OP_XOR, base, offset);
            m_formatter.immediate8(imm);
        } else {
            m_formatter.oneByteOp(OP_GROUP1_EvIz, GROUP1_OP_XOR, base, offset);
            m_formatter.immediate32(imm);
        }
    }

    void xorl_ir(int imm, RegisterID dst)
    {
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp(OP_GROUP1_EvIb, GROUP1_OP_XOR, dst);
            m_formatter.immediate8(imm);
        } else {
            m_formatter.oneByteOp(OP_GROUP1_EvIz, GROUP1_OP_XOR, dst);
            m_formatter.immediate32(imm);
        }
    }

#if CPU(X86_64)
    void xorq_rr(RegisterID src, RegisterID dst)
    {
        m_formatter.oneByteOp64(OP_XOR_EvGv, src, dst);
    }

    void xorq_ir(int imm, RegisterID dst)
    {
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp64(OP_GROUP1_EvIb, GROUP1_OP_XOR, dst);
            m_formatter.immediate8(imm);
        } else {
            m_formatter.oneByteOp64(OP_GROUP1_EvIz, GROUP1_OP_XOR, dst);
            m_formatter.immediate32(imm);
        }
    }
    
    void xorq_rm(RegisterID src, int offset, RegisterID base)
    {
        m_formatter.oneByteOp64(OP_XOR_EvGv, src, base, offset);
    }
    
    void rorq_i8r(int imm, RegisterID dst)
    {
        if (imm == 1)
            m_formatter.oneByteOp64(OP_GROUP2_Ev1, GROUP2_OP_ROR, dst);
        else {
            m_formatter.oneByteOp64(OP_GROUP2_EvIb, GROUP2_OP_ROR, dst);
            m_formatter.immediate8(imm);
        }
    }

#endif

    void sarl_i8r(int imm, RegisterID dst)
    {
        if (imm == 1)
            m_formatter.oneByteOp(OP_GROUP2_Ev1, GROUP2_OP_SAR, dst);
        else {
            m_formatter.oneByteOp(OP_GROUP2_EvIb, GROUP2_OP_SAR, dst);
            m_formatter.immediate8(imm);
        }
    }

    void sarl_CLr(RegisterID dst)
    {
        m_formatter.oneByteOp(OP_GROUP2_EvCL, GROUP2_OP_SAR, dst);
    }
    
    void shrl_i8r(int imm, RegisterID dst)
    {
        if (imm == 1)
            m_formatter.oneByteOp(OP_GROUP2_Ev1, GROUP2_OP_SHR, dst);
        else {
            m_formatter.oneByteOp(OP_GROUP2_EvIb, GROUP2_OP_SHR, dst);
            m_formatter.immediate8(imm);
        }
    }
    
    void shrl_CLr(RegisterID dst)
    {
        m_formatter.oneByteOp(OP_GROUP2_EvCL, GROUP2_OP_SHR, dst);
    }

    void shll_i8r(int imm, RegisterID dst)
    {
        if (imm == 1)
            m_formatter.oneByteOp(OP_GROUP2_Ev1, GROUP2_OP_SHL, dst);
        else {
            m_formatter.oneByteOp(OP_GROUP2_EvIb, GROUP2_OP_SHL, dst);
            m_formatter.immediate8(imm);
        }
    }

    void shll_CLr(RegisterID dst)
    {
        m_formatter.oneByteOp(OP_GROUP2_EvCL, GROUP2_OP_SHL, dst);
    }

#if CPU(X86_64)
    void sarq_CLr(RegisterID dst)
    {
        m_formatter.oneByteOp64(OP_GROUP2_EvCL, GROUP2_OP_SAR, dst);
    }

    void sarq_i8r(int imm, RegisterID dst)
    {
        if (imm == 1)
            m_formatter.oneByteOp64(OP_GROUP2_Ev1, GROUP2_OP_SAR, dst);
        else {
            m_formatter.oneByteOp64(OP_GROUP2_EvIb, GROUP2_OP_SAR, dst);
            m_formatter.immediate8(imm);
        }
    }
#endif

    void imull_rr(RegisterID src, RegisterID dst)
    {
        m_formatter.twoByteOp(OP2_IMUL_GvEv, dst, src);
    }

    void imull_mr(int offset, RegisterID base, RegisterID dst)
    {
        m_formatter.twoByteOp(OP2_IMUL_GvEv, dst, base, offset);
    }

    void imull_i32r(RegisterID src, int32_t value, RegisterID dst)
    {
        m_formatter.oneByteOp(OP_IMUL_GvEvIz, dst, src);
        m_formatter.immediate32(value);
    }

    void idivl_r(RegisterID dst)
    {
        m_formatter.oneByteOp(OP_GROUP3_Ev, GROUP3_OP_IDIV, dst);
    }

    // Comparisons:

    void cmpl_rr(RegisterID src, RegisterID dst)
    {
        m_formatter.oneByteOp(OP_CMP_EvGv, src, dst);
    }

    void cmpl_rm(RegisterID src, int offset, RegisterID base)
    {
        m_formatter.oneByteOp(OP_CMP_EvGv, src, base, offset);
    }

    void cmpl_mr(int offset, RegisterID base, RegisterID src)
    {
        m_formatter.oneByteOp(OP_CMP_GvEv, src, base, offset);
    }

    void cmpl_ir(int imm, RegisterID dst)
    {
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp(OP_GROUP1_EvIb, GROUP1_OP_CMP, dst);
            m_formatter.immediate8(imm);
        } else {
            m_formatter.oneByteOp(OP_GROUP1_EvIz, GROUP1_OP_CMP, dst);
            m_formatter.immediate32(imm);
        }
    }

    void cmpl_ir_force32(int imm, RegisterID dst)
    {
        m_formatter.oneByteOp(OP_GROUP1_EvIz, GROUP1_OP_CMP, dst);
        m_formatter.immediate32(imm);
    }
    
    void cmpl_im(int imm, int offset, RegisterID base)
    {
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp(OP_GROUP1_EvIb, GROUP1_OP_CMP, base, offset);
            m_formatter.immediate8(imm);
        } else {
            m_formatter.oneByteOp(OP_GROUP1_EvIz, GROUP1_OP_CMP, base, offset);
            m_formatter.immediate32(imm);
        }
    }
    
    void cmpb_im(int imm, int offset, RegisterID base)
    {
        m_formatter.oneByteOp(OP_GROUP1_EbIb, GROUP1_OP_CMP, base, offset);
        m_formatter.immediate8(imm);
    }
    
    void cmpb_im(int imm, int offset, RegisterID base, RegisterID index, int scale)
    {
        m_formatter.oneByteOp(OP_GROUP1_EbIb, GROUP1_OP_CMP, base, index, scale, offset);
        m_formatter.immediate8(imm);
    }
    
#if CPU(X86)
    void cmpb_im(int imm, const void* addr)
    {
        m_formatter.oneByteOp(OP_GROUP1_EbIb, GROUP1_OP_CMP, addr);
        m_formatter.immediate8(imm);
    }
#endif

    void cmpl_im(int imm, int offset, RegisterID base, RegisterID index, int scale)
    {
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp(OP_GROUP1_EvIb, GROUP1_OP_CMP, base, index, scale, offset);
            m_formatter.immediate8(imm);
        } else {
            m_formatter.oneByteOp(OP_GROUP1_EvIz, GROUP1_OP_CMP, base, index, scale, offset);
            m_formatter.immediate32(imm);
        }
    }

    void cmpl_im_force32(int imm, int offset, RegisterID base)
    {
        m_formatter.oneByteOp(OP_GROUP1_EvIz, GROUP1_OP_CMP, base, offset);
        m_formatter.immediate32(imm);
    }

#if CPU(X86_64)
    void cmpq_rr(RegisterID src, RegisterID dst)
    {
        m_formatter.oneByteOp64(OP_CMP_EvGv, src, dst);
    }

    void cmpq_rm(RegisterID src, int offset, RegisterID base)
    {
        m_formatter.oneByteOp64(OP_CMP_EvGv, src, base, offset);
    }

    void cmpq_mr(int offset, RegisterID base, RegisterID src)
    {
        m_formatter.oneByteOp64(OP_CMP_GvEv, src, base, offset);
    }

    void cmpq_ir(int imm, RegisterID dst)
    {
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp64(OP_GROUP1_EvIb, GROUP1_OP_CMP, dst);
            m_formatter.immediate8(imm);
        } else {
            m_formatter.oneByteOp64(OP_GROUP1_EvIz, GROUP1_OP_CMP, dst);
            m_formatter.immediate32(imm);
        }
    }

    void cmpq_im(int imm, int offset, RegisterID base)
    {
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp64(OP_GROUP1_EvIb, GROUP1_OP_CMP, base, offset);
            m_formatter.immediate8(imm);
        } else {
            m_formatter.oneByteOp64(OP_GROUP1_EvIz, GROUP1_OP_CMP, base, offset);
            m_formatter.immediate32(imm);
        }
    }

    void cmpq_im(int imm, int offset, RegisterID base, RegisterID index, int scale)
    {
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp64(OP_GROUP1_EvIb, GROUP1_OP_CMP, base, index, scale, offset);
            m_formatter.immediate8(imm);
        } else {
            m_formatter.oneByteOp64(OP_GROUP1_EvIz, GROUP1_OP_CMP, base, index, scale, offset);
            m_formatter.immediate32(imm);
        }
    }
#else
    void cmpl_rm(RegisterID reg, const void* addr)
    {
        m_formatter.oneByteOp(OP_CMP_EvGv, reg, addr);
    }

    void cmpl_im(int imm, const void* addr)
    {
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.oneByteOp(OP_GROUP1_EvIb, GROUP1_OP_CMP, addr);
            m_formatter.immediate8(imm);
        } else {
            m_formatter.oneByteOp(OP_GROUP1_EvIz, GROUP1_OP_CMP, addr);
            m_formatter.immediate32(imm);
        }
    }
#endif

    void cmpw_ir(int imm, RegisterID dst)
    {
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.prefix(PRE_OPERAND_SIZE);
            m_formatter.oneByteOp(OP_GROUP1_EvIb, GROUP1_OP_CMP, dst);
            m_formatter.immediate8(imm);
        } else {
            m_formatter.prefix(PRE_OPERAND_SIZE);
            m_formatter.oneByteOp(OP_GROUP1_EvIz, GROUP1_OP_CMP, dst);
            m_formatter.immediate16(imm);
        }
    }

    void cmpw_rm(RegisterID src, int offset, RegisterID base, RegisterID index, int scale)
    {
        m_formatter.prefix(PRE_OPERAND_SIZE);
        m_formatter.oneByteOp(OP_CMP_EvGv, src, base, index, scale, offset);
    }

    void cmpw_im(int imm, int offset, RegisterID base, RegisterID index, int scale)
    {
        if (CAN_SIGN_EXTEND_8_32(imm)) {
            m_formatter.prefix(PRE_OPERAND_SIZE);
            m_formatter.oneByteOp(OP_GROUP1_EvIb, GROUP1_OP_CMP, base, index, scale, offset);
            m_formatter.immediate8(imm);
        } else {
            m_formatter.prefix(PRE_OPERAND_SIZE);
            m_formatter.oneByteOp(OP_GROUP1_EvIz, GROUP1_OP_CMP, base, index, scale, offset);
            m_formatter.immediate16(imm);
        }
    }

    void testl_rr(RegisterID src, RegisterID dst)
    {
        m_formatter.oneByteOp(OP_TEST_EvGv, src, dst);
    }
    
    void testl_i32r(int imm, RegisterID dst)
    {
        m_formatter.oneByteOp(OP_GROUP3_EvIz, GROUP3_OP_TEST, dst);
        m_formatter.immediate32(imm);
    }

    void testl_i32m(int imm, int offset, RegisterID base)
    {
        m_formatter.oneByteOp(OP_GROUP3_EvIz, GROUP3_OP_TEST, base, offset);
        m_formatter.immediate32(imm);
    }

    void testb_rr(RegisterID src, RegisterID dst)
    {
        m_formatter.oneByteOp8(OP_TEST_EbGb, src, dst);
    }

    void testb_im(int imm, int offset, RegisterID base)
    {
        m_formatter.oneByteOp(OP_GROUP3_EbIb, GROUP3_OP_TEST, base, offset);
        m_formatter.immediate8(imm);
    }
    
    void testb_im(int imm, int offset, RegisterID base, RegisterID index, int scale)
    {
        m_formatter.oneByteOp(OP_GROUP3_EbIb, GROUP3_OP_TEST, base, index, scale, offset);
        m_formatter.immediate8(imm);
    }

#if CPU(X86)
    void testb_im(int imm, const void* addr)
    {
        m_formatter.oneByteOp(OP_GROUP3_EbIb, GROUP3_OP_TEST, addr);
        m_formatter.immediate8(imm);
    }
#endif

    void testl_i32m(int imm, int offset, RegisterID base, RegisterID index, int scale)
    {
        m_formatter.oneByteOp(OP_GROUP3_EvIz, GROUP3_OP_TEST, base, index, scale, offset);
        m_formatter.immediate32(imm);
    }

#if CPU(X86_64)
    void testq_rr(RegisterID src, RegisterID dst)
    {
        m_formatter.oneByteOp64(OP_TEST_EvGv, src, dst);
    }

    void testq_rm(RegisterID src, int offset, RegisterID base)
    {
        m_formatter.oneByteOp64(OP_TEST_EvGv, src, base, offset);
    }

    void testq_i32r(int imm, RegisterID dst)
    {
        m_formatter.oneByteOp64(OP_GROUP3_EvIz, GROUP3_OP_TEST, dst);
        m_formatter.immediate32(imm);
    }

    void testq_i32m(int imm, int offset, RegisterID base)
    {
        m_formatter.oneByteOp64(OP_GROUP3_EvIz, GROUP3_OP_TEST, base, offset);
        m_formatter.immediate32(imm);
    }

    void testq_i32m(int imm, int offset, RegisterID base, RegisterID index, int scale)
    {
        m_formatter.oneByteOp64(OP_GROUP3_EvIz, GROUP3_OP_TEST, base, index, scale, offset);
        m_formatter.immediate32(imm);
    }
#endif 

    void testw_rr(RegisterID src, RegisterID dst)
    {
        m_formatter.prefix(PRE_OPERAND_SIZE);
        m_formatter.oneByteOp(OP_TEST_EvGv, src, dst);
    }
    
    void testb_i8r(int imm, RegisterID dst)
    {
        m_formatter.oneByteOp8(OP_GROUP3_EbIb, GROUP3_OP_TEST, dst);
        m_formatter.immediate8(imm);
    }

    void setCC_r(Condition cond, RegisterID dst)
    {
        m_formatter.twoByteOp8(setccOpcode(cond), (GroupOpcodeID)0, dst);
    }

    void sete_r(RegisterID dst)
    {
        m_formatter.twoByteOp8(setccOpcode(ConditionE), (GroupOpcodeID)0, dst);
    }

    void setz_r(RegisterID dst)
    {
        sete_r(dst);
    }

    void setne_r(RegisterID dst)
    {
        m_formatter.twoByteOp8(setccOpcode(ConditionNE), (GroupOpcodeID)0, dst);
    }

    void setnz_r(RegisterID dst)
    {
        setne_r(dst);
    }

    // Various move ops:

    void cdq()
    {
        m_formatter.oneByteOp(OP_CDQ);
    }

    void fstpl(int offset, RegisterID base)
    {
        m_formatter.oneByteOp(OP_ESCAPE_DD, ESCAPE_DD_FSTP_doubleReal, base, offset);
    }

    void xchgl_rr(RegisterID src, RegisterID dst)
    {
        m_formatter.oneByteOp(OP_XCHG_EvGv, src, dst);
    }

#if CPU(X86_64)
    void xchgq_rr(RegisterID src, RegisterID dst)
    {
        m_formatter.oneByteOp64(OP_XCHG_EvGv, src, dst);
    }
#endif

    void movl_rr(RegisterID src, RegisterID dst)
    {
        m_formatter.oneByteOp(OP_MOV_EvGv, src, dst);
    }
    
    void movl_rm(RegisterID src, int offset, RegisterID base)
    {
        m_formatter.oneByteOp(OP_MOV_EvGv, src, base, offset);
    }

    void movl_rm_disp32(RegisterID src, int offset, RegisterID base)
    {
        m_formatter.oneByteOp_disp32(OP_MOV_EvGv, src, base, offset);
    }

    void movl_rm(RegisterID src, int offset, RegisterID base, RegisterID index, int scale)
    {
        m_formatter.oneByteOp(OP_MOV_EvGv, src, base, index, scale, offset);
    }
    
    void movl_mEAX(const void* addr)
    {
        m_formatter.oneByteOp(OP_MOV_EAXOv);
#if CPU(X86_64)
        m_formatter.immediate64(reinterpret_cast<int64_t>(addr));
#else
        m_formatter.immediate32(reinterpret_cast<int>(addr));
#endif
    }

    void movl_mr(int offset, RegisterID base, RegisterID dst)
    {
        m_formatter.oneByteOp(OP_MOV_GvEv, dst, base, offset);
    }

    void movl_mr_disp32(int offset, RegisterID base, RegisterID dst)
    {
        m_formatter.oneByteOp_disp32(OP_MOV_GvEv, dst, base, offset);
    }
    
    void movl_mr_disp8(int offset, RegisterID base, RegisterID dst)
    {
        m_formatter.oneByteOp_disp8(OP_MOV_GvEv, dst, base, offset);
    }

    void movl_mr(int offset, RegisterID base, RegisterID index, int scale, RegisterID dst)
    {
        m_formatter.oneByteOp(OP_MOV_GvEv, dst, base, index, scale, offset);
    }

    void movl_i32r(int imm, RegisterID dst)
    {
        m_formatter.oneByteOp(OP_MOV_EAXIv, dst);
        m_formatter.immediate32(imm);
    }

    void movl_i32m(int imm, int offset, RegisterID base)
    {
        m_formatter.oneByteOp(OP_GROUP11_EvIz, GROUP11_MOV, base, offset);
        m_formatter.immediate32(imm);
    }
    
    void movl_i32m(int imm, int offset, RegisterID base, RegisterID index, int scale)
    {
        m_formatter.oneByteOp(OP_GROUP11_EvIz, GROUP11_MOV, base, index, scale, offset);
        m_formatter.immediate32(imm);
    }

#if !CPU(X86_64)
    void movb_i8m(int imm, const void* addr)
    {
        ASSERT(-128 <= imm && imm < 128);
        m_formatter.oneByteOp(OP_GROUP11_EvIb, GROUP11_MOV, addr);
        m_formatter.immediate8(imm);
    }
#endif

    void movb_i8m(int imm, int offset, RegisterID base)
    {
        ASSERT(-128 <= imm && imm < 128);
        m_formatter.oneByteOp(OP_GROUP11_EvIb, GROUP11_MOV, base, offset);
        m_formatter.immediate8(imm);
    }

    void movb_i8m(int imm, int offset, RegisterID base, RegisterID index, int scale)
    {
        ASSERT(-128 <= imm && imm < 128);
        m_formatter.oneByteOp(OP_GROUP11_EvIb, GROUP11_MOV, base, index, scale, offset);
        m_formatter.immediate8(imm);
    }
    
    void movb_rm(RegisterID src, int offset, RegisterID base, RegisterID index, int scale)
    {
        m_formatter.oneByteOp8(OP_MOV_EbGb, src, base, index, scale, offset);
    }
    
    void movw_rm(RegisterID src, int offset, RegisterID base, RegisterID index, int scale)
    {
        m_formatter.prefix(PRE_OPERAND_SIZE);
        m_formatter.oneByteOp8(OP_MOV_EvGv, src, base, index, scale, offset);
    }

    void movl_EAXm(const void* addr)
    {
        m_formatter.oneByteOp(OP_MOV_OvEAX);
#if CPU(X86_64)
        m_formatter.immediate64(reinterpret_cast<int64_t>(addr));
#else
        m_formatter.immediate32(reinterpret_cast<int>(addr));
#endif
    }

#if CPU(X86_64)
    void movq_rr(RegisterID src, RegisterID dst)
    {
        m_formatter.oneByteOp64(OP_MOV_EvGv, src, dst);
    }

    void movq_rm(RegisterID src, int offset, RegisterID base)
    {
        m_formatter.oneByteOp64(OP_MOV_EvGv, src, base, offset);
    }

    void movq_rm_disp32(RegisterID src, int offset, RegisterID base)
    {
        m_formatter.oneByteOp64_disp32(OP_MOV_EvGv, src, base, offset);
    }

    void movq_rm(RegisterID src, int offset, RegisterID base, RegisterID index, int scale)
    {
        m_formatter.oneByteOp64(OP_MOV_EvGv, src, base, index, scale, offset);
    }

    void movq_mEAX(const void* addr)
    {
        m_formatter.oneByteOp64(OP_MOV_EAXOv);
        m_formatter.immediate64(reinterpret_cast<int64_t>(addr));
    }

    void movq_EAXm(const void* addr)
    {
        m_formatter.oneByteOp64(OP_MOV_OvEAX);
        m_formatter.immediate64(reinterpret_cast<int64_t>(addr));
    }

    void movq_mr(int offset, RegisterID base, RegisterID dst)
    {
        m_formatter.oneByteOp64(OP_MOV_GvEv, dst, base, offset);
    }

    void movq_mr_disp32(int offset, RegisterID base, RegisterID dst)
    {
        m_formatter.oneByteOp64_disp32(OP_MOV_GvEv, dst, base, offset);
    }

    void movq_mr_disp8(int offset, RegisterID base, RegisterID dst)
    {
        m_formatter.oneByteOp64_disp8(OP_MOV_GvEv, dst, base, offset);
    }

    void movq_mr(int offset, RegisterID base, RegisterID index, int scale, RegisterID dst)
    {
        m_formatter.oneByteOp64(OP_MOV_GvEv, dst, base, index, scale, offset);
    }

    void movq_i32m(int imm, int offset, RegisterID base)
    {
        m_formatter.oneByteOp64(OP_GROUP11_EvIz, GROUP11_MOV, base, offset);
        m_formatter.immediate32(imm);
    }

    void movq_i64r(int64_t imm, RegisterID dst)
    {
        m_formatter.oneByteOp64(OP_MOV_EAXIv, dst);
        m_formatter.immediate64(imm);
    }
    
    void movsxd_rr(RegisterID src, RegisterID dst)
    {
        m_formatter.oneByteOp64(OP_MOVSXD_GvEv, dst, src);
    }
    
    
#else
    void movl_rm(RegisterID src, const void* addr)
    {
        if (src == X86Registers::eax)
            movl_EAXm(addr);
        else 
            m_formatter.oneByteOp(OP_MOV_EvGv, src, addr);
    }
    
    void movl_mr(const void* addr, RegisterID dst)
    {
        if (dst == X86Registers::eax)
            movl_mEAX(addr);
        else
            m_formatter.oneByteOp(OP_MOV_GvEv, dst, addr);
    }

    void movl_i32m(int imm, const void* addr)
    {
        m_formatter.oneByteOp(OP_GROUP11_EvIz, GROUP11_MOV, addr);
        m_formatter.immediate32(imm);
    }
#endif

    void movzwl_mr(int offset, RegisterID base, RegisterID dst)
    {
        m_formatter.twoByteOp(OP2_MOVZX_GvEw, dst, base, offset);
    }

    void movzwl_mr(int offset, RegisterID base, RegisterID index, int scale, RegisterID dst)
    {
        m_formatter.twoByteOp(OP2_MOVZX_GvEw, dst, base, index, scale, offset);
    }

    void movswl_mr(int offset, RegisterID base, RegisterID dst)
    {
        m_formatter.twoByteOp(OP2_MOVSX_GvEw, dst, base, offset);
    }

    void movswl_mr(int offset, RegisterID base, RegisterID index, int scale, RegisterID dst)
    {
        m_formatter.twoByteOp(OP2_MOVSX_GvEw, dst, base, index, scale, offset);
    }

    void movzbl_mr(int offset, RegisterID base, RegisterID dst)
    {
        m_formatter.twoByteOp(OP2_MOVZX_GvEb, dst, base, offset);
    }
    
    void movzbl_mr(int offset, RegisterID base, RegisterID index, int scale, RegisterID dst)
    {
        m_formatter.twoByteOp(OP2_MOVZX_GvEb, dst, base, index, scale, offset);
    }

    void movsbl_mr(int offset, RegisterID base, RegisterID dst)
    {
        m_formatter.twoByteOp(OP2_MOVSX_GvEb, dst, base, offset);
    }
    
    void movsbl_mr(int offset, RegisterID base, RegisterID index, int scale, RegisterID dst)
    {
        m_formatter.twoByteOp(OP2_MOVSX_GvEb, dst, base, index, scale, offset);
    }

    void movzbl_rr(RegisterID src, RegisterID dst)
    {
        // In 64-bit, this may cause an unnecessary REX to be planted (if the dst register
        // is in the range ESP-EDI, and the src would not have required a REX).  Unneeded
        // REX prefixes are defined to be silently ignored by the processor.
        m_formatter.twoByteOp8(OP2_MOVZX_GvEb, dst, src);
    }

    void leal_mr(int offset, RegisterID base, RegisterID dst)
    {
        m_formatter.oneByteOp(OP_LEA, dst, base, offset);
    }
#if CPU(X86_64)
    void leaq_mr(int offset, RegisterID base, RegisterID dst)
    {
        m_formatter.oneByteOp64(OP_LEA, dst, base, offset);
    }
#endif

    // Flow control:

    AssemblerLabel call()
    {
        m_formatter.oneByteOp(OP_CALL_rel32);
        return m_formatter.immediateRel32();
    }
    
    AssemblerLabel call(RegisterID dst)
    {
        m_formatter.oneByteOp(OP_GROUP5_Ev, GROUP5_OP_CALLN, dst);
        return m_formatter.label();
    }
    
    void call_m(int offset, RegisterID base)
    {
        m_formatter.oneByteOp(OP_GROUP5_Ev, GROUP5_OP_CALLN, base, offset);
    }

    AssemblerLabel jmp()
    {
        m_formatter.oneByteOp(OP_JMP_rel32);
        return m_formatter.immediateRel32();
    }
    
    // Return a AssemblerLabel so we have a label to the jump, so we can use this
    // To make a tail recursive call on x86-64.  The MacroAssembler
    // really shouldn't wrap this as a Jump, since it can't be linked. :-/
    AssemblerLabel jmp_r(RegisterID dst)
    {
        m_formatter.oneByteOp(OP_GROUP5_Ev, GROUP5_OP_JMPN, dst);
        return m_formatter.label();
    }
    
    void jmp_m(int offset, RegisterID base)
    {
        m_formatter.oneByteOp(OP_GROUP5_Ev, GROUP5_OP_JMPN, base, offset);
    }
    
#if !CPU(X86_64)
    void jmp_m(const void* address)
    {
        m_formatter.oneByteOp(OP_GROUP5_Ev, GROUP5_OP_JMPN, address);
    }
#endif

    AssemblerLabel jne()
    {
        m_formatter.twoByteOp(jccRel32(ConditionNE));
        return m_formatter.immediateRel32();
    }
    
    AssemblerLabel jnz()
    {
        return jne();
    }

    AssemblerLabel je()
    {
        m_formatter.twoByteOp(jccRel32(ConditionE));
        return m_formatter.immediateRel32();
    }
    
    AssemblerLabel jz()
    {
        return je();
    }

    AssemblerLabel jl()
    {
        m_formatter.twoByteOp(jccRel32(ConditionL));
        return m_formatter.immediateRel32();
    }
    
    AssemblerLabel jb()
    {
        m_formatter.twoByteOp(jccRel32(ConditionB));
        return m_formatter.immediateRel32();
    }
    
    AssemblerLabel jle()
    {
        m_formatter.twoByteOp(jccRel32(ConditionLE));
        return m_formatter.immediateRel32();
    }
    
    AssemblerLabel jbe()
    {
        m_formatter.twoByteOp(jccRel32(ConditionBE));
        return m_formatter.immediateRel32();
    }
    
    AssemblerLabel jge()
    {
        m_formatter.twoByteOp(jccRel32(ConditionGE));
        return m_formatter.immediateRel32();
    }

    AssemblerLabel jg()
    {
        m_formatter.twoByteOp(jccRel32(ConditionG));
        return m_formatter.immediateRel32();
    }

    AssemblerLabel ja()
    {
        m_formatter.twoByteOp(jccRel32(ConditionA));
        return m_formatter.immediateRel32();
    }
    
    AssemblerLabel jae()
    {
        m_formatter.twoByteOp(jccRel32(ConditionAE));
        return m_formatter.immediateRel32();
    }
    
    AssemblerLabel jo()
    {
        m_formatter.twoByteOp(jccRel32(ConditionO));
        return m_formatter.immediateRel32();
    }

    AssemblerLabel jnp()
    {
        m_formatter.twoByteOp(jccRel32(ConditionNP));
        return m_formatter.immediateRel32();
    }

    AssemblerLabel jp()
    {
        m_formatter.twoByteOp(jccRel32(ConditionP));
        return m_formatter.immediateRel32();
    }
    
    AssemblerLabel js()
    {
        m_formatter.twoByteOp(jccRel32(ConditionS));
        return m_formatter.immediateRel32();
    }

    AssemblerLabel jCC(Condition cond)
    {
        m_formatter.twoByteOp(jccRel32(cond));
        return m_formatter.immediateRel32();
    }

    // SSE operations:

    void addsd_rr(XMMRegisterID src, XMMRegisterID dst)
    {
        m_formatter.prefix(PRE_SSE_F2);
        m_formatter.twoByteOp(OP2_ADDSD_VsdWsd, (RegisterID)dst, (RegisterID)src);
    }

    void addsd_mr(int offset, RegisterID base, XMMRegisterID dst)
    {
        m_formatter.prefix(PRE_SSE_F2);
        m_formatter.twoByteOp(OP2_ADDSD_VsdWsd, (RegisterID)dst, base, offset);
    }

#if !CPU(X86_64)
    void addsd_mr(const void* address, XMMRegisterID dst)
    {
        m_formatter.prefix(PRE_SSE_F2);
        m_formatter.twoByteOp(OP2_ADDSD_VsdWsd, (RegisterID)dst, address);
    }
#endif

    void cvtsi2sd_rr(RegisterID src, XMMRegisterID dst)
    {
        m_formatter.prefix(PRE_SSE_F2);
        m_formatter.twoByteOp(OP2_CVTSI2SD_VsdEd, (RegisterID)dst, src);
    }

    void cvtsi2sd_mr(int offset, RegisterID base, XMMRegisterID dst)
    {
        m_formatter.prefix(PRE_SSE_F2);
        m_formatter.twoByteOp(OP2_CVTSI2SD_VsdEd, (RegisterID)dst, base, offset);
    }

#if !CPU(X86_64)
    void cvtsi2sd_mr(const void* address, XMMRegisterID dst)
    {
        m_formatter.prefix(PRE_SSE_F2);
        m_formatter.twoByteOp(OP2_CVTSI2SD_VsdEd, (RegisterID)dst, address);
    }
#endif

    void cvttsd2si_rr(XMMRegisterID src, RegisterID dst)
    {
        m_formatter.prefix(PRE_SSE_F2);
        m_formatter.twoByteOp(OP2_CVTTSD2SI_GdWsd, dst, (RegisterID)src);
    }

    void cvtsd2ss_rr(XMMRegisterID src, XMMRegisterID dst)
    {
        m_formatter.prefix(PRE_SSE_F2);
        m_formatter.twoByteOp(OP2_CVTSD2SS_VsdWsd, dst, (RegisterID)src);
    }

    void cvtss2sd_rr(XMMRegisterID src, XMMRegisterID dst)
    {
        m_formatter.prefix(PRE_SSE_F3);
        m_formatter.twoByteOp(OP2_CVTSS2SD_VsdWsd, dst, (RegisterID)src);
    }
    
#if CPU(X86_64)
    void cvttsd2siq_rr(XMMRegisterID src, RegisterID dst)
    {
        m_formatter.prefix(PRE_SSE_F2);
        m_formatter.twoByteOp64(OP2_CVTTSD2SI_GdWsd, dst, (RegisterID)src);
    }
#endif

    void movd_rr(XMMRegisterID src, RegisterID dst)
    {
        m_formatter.prefix(PRE_SSE_66);
        m_formatter.twoByteOp(OP2_MOVD_EdVd, (RegisterID)src, dst);
    }

    void movd_rr(RegisterID src, XMMRegisterID dst)
    {
        m_formatter.prefix(PRE_SSE_66);
        m_formatter.twoByteOp(OP2_MOVD_VdEd, (RegisterID)dst, src);
    }

#if CPU(X86_64)
    void movq_rr(XMMRegisterID src, RegisterID dst)
    {
        m_formatter.prefix(PRE_SSE_66);
        m_formatter.twoByteOp64(OP2_MOVD_EdVd, (RegisterID)src, dst);
    }

    void movq_rr(RegisterID src, XMMRegisterID dst)
    {
        m_formatter.prefix(PRE_SSE_66);
        m_formatter.twoByteOp64(OP2_MOVD_VdEd, (RegisterID)dst, src);
    }
#endif

    void movsd_rr(XMMRegisterID src, XMMRegisterID dst)
    {
        m_formatter.prefix(PRE_SSE_F2);
        m_formatter.twoByteOp(OP2_MOVSD_VsdWsd, (RegisterID)dst, (RegisterID)src);
    }

    void movsd_rm(XMMRegisterID src, int offset, RegisterID base)
    {
        m_formatter.prefix(PRE_SSE_F2);
        m_formatter.twoByteOp(OP2_MOVSD_WsdVsd, (RegisterID)src, base, offset);
    }
    
    void movsd_rm(XMMRegisterID src, int offset, RegisterID base, RegisterID index, int scale)
    {
        m_formatter.prefix(PRE_SSE_F2);
        m_formatter.twoByteOp(OP2_MOVSD_WsdVsd, (RegisterID)src, base, index, scale, offset);
    }
    
    void movss_rm(XMMRegisterID src, int offset, RegisterID base, RegisterID index, int scale)
    {
        m_formatter.prefix(PRE_SSE_F3);
        m_formatter.twoByteOp(OP2_MOVSD_WsdVsd, (RegisterID)src, base, index, scale, offset);
    }
    
    void movsd_mr(int offset, RegisterID base, XMMRegisterID dst)
    {
        m_formatter.prefix(PRE_SSE_F2);
        m_formatter.twoByteOp(OP2_MOVSD_VsdWsd, (RegisterID)dst, base, offset);
    }

    void movsd_mr(int offset, RegisterID base, RegisterID index, int scale, XMMRegisterID dst)
    {
        m_formatter.prefix(PRE_SSE_F2);
        m_formatter.twoByteOp(OP2_MOVSD_VsdWsd, dst, base, index, scale, offset);
    }
    
    void movss_mr(int offset, RegisterID base, RegisterID index, int scale, XMMRegisterID dst)
    {
        m_formatter.prefix(PRE_SSE_F3);
        m_formatter.twoByteOp(OP2_MOVSD_VsdWsd, dst, base, index, scale, offset);
    }

#if !CPU(X86_64)
    void movsd_mr(const void* address, XMMRegisterID dst)
    {
        m_formatter.prefix(PRE_SSE_F2);
        m_formatter.twoByteOp(OP2_MOVSD_VsdWsd, (RegisterID)dst, address);
    }
    void movsd_rm(XMMRegisterID src, const void* address)
    {
        m_formatter.prefix(PRE_SSE_F2);
        m_formatter.twoByteOp(OP2_MOVSD_WsdVsd, (RegisterID)src, address);
    }
#endif

    void mulsd_rr(XMMRegisterID src, XMMRegisterID dst)
    {
        m_formatter.prefix(PRE_SSE_F2);
        m_formatter.twoByteOp(OP2_MULSD_VsdWsd, (RegisterID)dst, (RegisterID)src);
    }

    void mulsd_mr(int offset, RegisterID base, XMMRegisterID dst)
    {
        m_formatter.prefix(PRE_SSE_F2);
        m_formatter.twoByteOp(OP2_MULSD_VsdWsd, (RegisterID)dst, base, offset);
    }

    void pextrw_irr(int whichWord, XMMRegisterID src, RegisterID dst)
    {
        m_formatter.prefix(PRE_SSE_66);
        m_formatter.twoByteOp(OP2_PEXTRW_GdUdIb, (RegisterID)dst, (RegisterID)src);
        m_formatter.immediate8(whichWord);
    }

    void psllq_i8r(int imm, XMMRegisterID dst)
    {
        m_formatter.prefix(PRE_SSE_66);
        m_formatter.twoByteOp8(OP2_PSLLQ_UdqIb, GROUP14_OP_PSLLQ, (RegisterID)dst);
        m_formatter.immediate8(imm);
    }

    void psrlq_i8r(int imm, XMMRegisterID dst)
    {
        m_formatter.prefix(PRE_SSE_66);
        m_formatter.twoByteOp8(OP2_PSRLQ_UdqIb, GROUP14_OP_PSRLQ, (RegisterID)dst);
        m_formatter.immediate8(imm);
    }

    void por_rr(XMMRegisterID src, XMMRegisterID dst)
    {
        m_formatter.prefix(PRE_SSE_66);
        m_formatter.twoByteOp(OP2_POR_VdqWdq, (RegisterID)dst, (RegisterID)src);
    }

    void subsd_rr(XMMRegisterID src, XMMRegisterID dst)
    {
        m_formatter.prefix(PRE_SSE_F2);
        m_formatter.twoByteOp(OP2_SUBSD_VsdWsd, (RegisterID)dst, (RegisterID)src);
    }

    void subsd_mr(int offset, RegisterID base, XMMRegisterID dst)
    {
        m_formatter.prefix(PRE_SSE_F2);
        m_formatter.twoByteOp(OP2_SUBSD_VsdWsd, (RegisterID)dst, base, offset);
    }

    void ucomisd_rr(XMMRegisterID src, XMMRegisterID dst)
    {
        m_formatter.prefix(PRE_SSE_66);
        m_formatter.twoByteOp(OP2_UCOMISD_VsdWsd, (RegisterID)dst, (RegisterID)src);
    }

    void ucomisd_mr(int offset, RegisterID base, XMMRegisterID dst)
    {
        m_formatter.prefix(PRE_SSE_66);
        m_formatter.twoByteOp(OP2_UCOMISD_VsdWsd, (RegisterID)dst, base, offset);
    }

    void divsd_rr(XMMRegisterID src, XMMRegisterID dst)
    {
        m_formatter.prefix(PRE_SSE_F2);
        m_formatter.twoByteOp(OP2_DIVSD_VsdWsd, (RegisterID)dst, (RegisterID)src);
    }

    void divsd_mr(int offset, RegisterID base, XMMRegisterID dst)
    {
        m_formatter.prefix(PRE_SSE_F2);
        m_formatter.twoByteOp(OP2_DIVSD_VsdWsd, (RegisterID)dst, base, offset);
    }

    void xorpd_rr(XMMRegisterID src, XMMRegisterID dst)
    {
        m_formatter.prefix(PRE_SSE_66);
        m_formatter.twoByteOp(OP2_XORPD_VpdWpd, (RegisterID)dst, (RegisterID)src);
    }

    void andnpd_rr(XMMRegisterID src, XMMRegisterID dst)
    {
        m_formatter.prefix(PRE_SSE_66);
        m_formatter.twoByteOp(OP2_ANDNPD_VpdWpd, (RegisterID)dst, (RegisterID)src);
    }

    void sqrtsd_rr(XMMRegisterID src, XMMRegisterID dst)
    {
        m_formatter.prefix(PRE_SSE_F2);
        m_formatter.twoByteOp(OP2_SQRTSD_VsdWsd, (RegisterID)dst, (RegisterID)src);
    }

    // Misc instructions:

    void int3()
    {
        m_formatter.oneByteOp(OP_INT3);
    }
    
    void ret()
    {
        m_formatter.oneByteOp(OP_RET);
    }

    void predictNotTaken()
    {
        m_formatter.prefix(PRE_PREDICT_BRANCH_NOT_TAKEN);
    }

    // Assembler admin methods:

    size_t codeSize() const
    {
        return m_formatter.codeSize();
    }
    
    AssemblerLabel labelForWatchpoint()
    {
        AssemblerLabel result = m_formatter.label();
        if (static_cast<int>(result.m_offset) != m_indexOfLastWatchpoint)
            result = label();
        m_indexOfLastWatchpoint = result.m_offset;
        m_indexOfTailOfLastWatchpoint = result.m_offset + maxJumpReplacementSize();
        return result;
    }
    
    AssemblerLabel labelIgnoringWatchpoints()
    {
        return m_formatter.label();
    }

    AssemblerLabel label()
    {
        AssemblerLabel result = m_formatter.label();
        while (UNLIKELY(static_cast<int>(result.m_offset) < m_indexOfTailOfLastWatchpoint)) {
            nop();
            result = m_formatter.label();
        }
        return result;
    }

    AssemblerLabel align(int alignment)
    {
        while (!m_formatter.isAligned(alignment))
            m_formatter.oneByteOp(OP_HLT);

        return label();
    }

    // Linking & patching:
    //
    // 'link' and 'patch' methods are for use on unprotected code - such as the code
    // within the AssemblerBuffer, and code being patched by the patch buffer.  Once
    // code has been finalized it is (platform support permitting) within a non-
    // writable region of memory; to modify the code in an execute-only execuable
    // pool the 'repatch' and 'relink' methods should be used.

    void linkJump(AssemblerLabel from, AssemblerLabel to)
    {
        ASSERT(from.isSet());
        ASSERT(to.isSet());

        char* code = reinterpret_cast<char*>(m_formatter.data());
        ASSERT(!reinterpret_cast<int32_t*>(code + from.m_offset)[-1]);
        setRel32(code + from.m_offset, code + to.m_offset);
    }
    
    static void linkJump(void* code, AssemblerLabel from, void* to)
    {
        ASSERT(from.isSet());

        setRel32(reinterpret_cast<char*>(code) + from.m_offset, to);
    }

    static void linkCall(void* code, AssemblerLabel from, void* to)
    {
        ASSERT(from.isSet());

        setRel32(reinterpret_cast<char*>(code) + from.m_offset, to);
    }

    static void linkPointer(void* code, AssemblerLabel where, void* value)
    {
        ASSERT(where.isSet());

        setPointer(reinterpret_cast<char*>(code) + where.m_offset, value);
    }

    static void relinkJump(void* from, void* to)
    {
        setRel32(from, to);
    }
    
    static void relinkCall(void* from, void* to)
    {
        setRel32(from, to);
    }
    
    static void repatchCompact(void* where, int32_t value)
    {
        ASSERT(value >= std::numeric_limits<int8_t>::min());
        ASSERT(value <= std::numeric_limits<int8_t>::max());
        setInt8(where, value);
    }

    static void repatchInt32(void* where, int32_t value)
    {
        setInt32(where, value);
    }

    static void repatchPointer(void* where, void* value)
    {
        setPointer(where, value);
    }
    
    static void* readPointer(void* where)
    {
        return reinterpret_cast<void**>(where)[-1];
    }

    static void replaceWithJump(void* instructionStart, void* to)
    {
        uint8_t* ptr = reinterpret_cast<uint8_t*>(instructionStart);
        uint8_t* dstPtr = reinterpret_cast<uint8_t*>(to);
        intptr_t distance = (intptr_t)(dstPtr - (ptr + 5));
        ptr[0] = static_cast<uint8_t>(OP_JMP_rel32);
        *reinterpret_cast<int32_t*>(ptr + 1) = static_cast<int32_t>(distance);
    }
    
    static ptrdiff_t maxJumpReplacementSize()
    {
        return 5;
    }
    
#if CPU(X86_64)
    static void revertJumpTo_movq_i64r(void* instructionStart, int64_t imm, RegisterID dst)
    {
        const int rexBytes = 1;
        const int opcodeBytes = 1;
        ASSERT(rexBytes + opcodeBytes <= maxJumpReplacementSize());
        uint8_t* ptr = reinterpret_cast<uint8_t*>(instructionStart);
        ptr[0] = PRE_REX | (1 << 3) | (dst >> 3);
        ptr[1] = OP_MOV_EAXIv | (dst & 7);
        
        union {
            uint64_t asWord;
            uint8_t asBytes[8];
        } u;
        u.asWord = imm;
        for (unsigned i = rexBytes + opcodeBytes; i < static_cast<unsigned>(maxJumpReplacementSize()); ++i)
            ptr[i] = u.asBytes[i - rexBytes - opcodeBytes];
    }
#endif
    
    static void revertJumpTo_cmpl_ir_force32(void* instructionStart, int32_t imm, RegisterID dst)
    {
        const int opcodeBytes = 1;
        const int modRMBytes = 1;
        ASSERT(opcodeBytes + modRMBytes <= maxJumpReplacementSize());
        uint8_t* ptr = reinterpret_cast<uint8_t*>(instructionStart);
        ptr[0] = OP_GROUP1_EvIz;
        ptr[1] = (X86InstructionFormatter::ModRmRegister << 6) | (GROUP1_OP_CMP << 3) | dst;
        union {
            uint32_t asWord;
            uint8_t asBytes[4];
        } u;
        u.asWord = imm;
        for (unsigned i = opcodeBytes + modRMBytes; i < static_cast<unsigned>(maxJumpReplacementSize()); ++i)
            ptr[i] = u.asBytes[i - opcodeBytes - modRMBytes];
    }
    
    static void revertJumpTo_cmpl_im_force32(void* instructionStart, int32_t imm, int offset, RegisterID dst)
    {
        ASSERT_UNUSED(offset, !offset);
        const int opcodeBytes = 1;
        const int modRMBytes = 1;
        ASSERT(opcodeBytes + modRMBytes <= maxJumpReplacementSize());
        uint8_t* ptr = reinterpret_cast<uint8_t*>(instructionStart);
        ptr[0] = OP_GROUP1_EvIz;
        ptr[1] = (X86InstructionFormatter::ModRmMemoryNoDisp << 6) | (GROUP1_OP_CMP << 3) | dst;
        union {
            uint32_t asWord;
            uint8_t asBytes[4];
        } u;
        u.asWord = imm;
        for (unsigned i = opcodeBytes + modRMBytes; i < static_cast<unsigned>(maxJumpReplacementSize()); ++i)
            ptr[i] = u.asBytes[i - opcodeBytes - modRMBytes];
    }
    
    static void replaceWithLoad(void* instructionStart)
    {
        uint8_t* ptr = reinterpret_cast<uint8_t*>(instructionStart);
#if CPU(X86_64)
        if ((*ptr & ~15) == PRE_REX)
            ptr++;
#endif
        switch (*ptr) {
        case OP_MOV_GvEv:
            break;
        case OP_LEA:
            *ptr = OP_MOV_GvEv;
            break;
        default:
            RELEASE_ASSERT_NOT_REACHED();
        }
    }
    
    static void replaceWithAddressComputation(void* instructionStart)
    {
        uint8_t* ptr = reinterpret_cast<uint8_t*>(instructionStart);
#if CPU(X86_64)
        if ((*ptr & ~15) == PRE_REX)
            ptr++;
#endif
        switch (*ptr) {
        case OP_MOV_GvEv:
            *ptr = OP_LEA;
            break;
        case OP_LEA:
            break;
        default:
            RELEASE_ASSERT_NOT_REACHED();
        }
    }
    
    static unsigned getCallReturnOffset(AssemblerLabel call)
    {
        ASSERT(call.isSet());
        return call.m_offset;
    }

    static void* getRelocatedAddress(void* code, AssemblerLabel label)
    {
        ASSERT(label.isSet());
        return reinterpret_cast<void*>(reinterpret_cast<ptrdiff_t>(code) + label.m_offset);
    }
    
    static int getDifferenceBetweenLabels(AssemblerLabel a, AssemblerLabel b)
    {
        return b.m_offset - a.m_offset;
    }
    
    PassRefPtr<ExecutableMemoryHandle> executableCopy(VM& vm, void* ownerUID, JITCompilationEffort effort)
    {
        return m_formatter.executableCopy(vm, ownerUID, effort);
    }

    unsigned debugOffset() { return m_formatter.debugOffset(); }

    void nop()
    {
        m_formatter.oneByteOp(OP_NOP);
    }

    // This is a no-op on x86
    ALWAYS_INLINE static void cacheFlush(void*, size_t) { }

private:

    static void setPointer(void* where, void* value)
    {
        reinterpret_cast<void**>(where)[-1] = value;
    }

    static void setInt32(void* where, int32_t value)
    {
        reinterpret_cast<int32_t*>(where)[-1] = value;
    }
    
    static void setInt8(void* where, int8_t value)
    {
        reinterpret_cast<int8_t*>(where)[-1] = value;
    }

    static void setRel32(void* from, void* to)
    {
        intptr_t offset = reinterpret_cast<intptr_t>(to) - reinterpret_cast<intptr_t>(from);
        ASSERT(offset == static_cast<int32_t>(offset));

        setInt32(from, offset);
    }

    class X86InstructionFormatter {

        static const int maxInstructionSize = 16;

    public:

        enum ModRmMode {
            ModRmMemoryNoDisp,
            ModRmMemoryDisp8,
            ModRmMemoryDisp32,
            ModRmRegister,
        };

        // Legacy prefix bytes:
        //
        // These are emmitted prior to the instruction.

        void prefix(OneByteOpcodeID pre)
        {
            m_buffer.putByte(pre);
        }

        // Word-sized operands / no operand instruction formatters.
        //
        // In addition to the opcode, the following operand permutations are supported:
        //   * None - instruction takes no operands.
        //   * One register - the low three bits of the RegisterID are added into the opcode.
        //   * Two registers - encode a register form ModRm (for all ModRm formats, the reg field is passed first, and a GroupOpcodeID may be passed in its place).
        //   * Three argument ModRM - a register, and a register and an offset describing a memory operand.
        //   * Five argument ModRM - a register, and a base register, an index, scale, and offset describing a memory operand.
        //
        // For 32-bit x86 targets, the address operand may also be provided as a void*.
        // On 64-bit targets REX prefixes will be planted as necessary, where high numbered registers are used.
        //
        // The twoByteOp methods plant two-byte Intel instructions sequences (first opcode byte 0x0F).

        void oneByteOp(OneByteOpcodeID opcode)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            m_buffer.putByteUnchecked(opcode);
        }

        void oneByteOp(OneByteOpcodeID opcode, RegisterID reg)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexIfNeeded(0, 0, reg);
            m_buffer.putByteUnchecked(opcode + (reg & 7));
        }

        void oneByteOp(OneByteOpcodeID opcode, int reg, RegisterID rm)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexIfNeeded(reg, 0, rm);
            m_buffer.putByteUnchecked(opcode);
            registerModRM(reg, rm);
        }

        void oneByteOp(OneByteOpcodeID opcode, int reg, RegisterID base, int offset)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexIfNeeded(reg, 0, base);
            m_buffer.putByteUnchecked(opcode);
            memoryModRM(reg, base, offset);
        }

        void oneByteOp_disp32(OneByteOpcodeID opcode, int reg, RegisterID base, int offset)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexIfNeeded(reg, 0, base);
            m_buffer.putByteUnchecked(opcode);
            memoryModRM_disp32(reg, base, offset);
        }
        
        void oneByteOp_disp8(OneByteOpcodeID opcode, int reg, RegisterID base, int offset)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexIfNeeded(reg, 0, base);
            m_buffer.putByteUnchecked(opcode);
            memoryModRM_disp8(reg, base, offset);
        }

        void oneByteOp(OneByteOpcodeID opcode, int reg, RegisterID base, RegisterID index, int scale, int offset)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexIfNeeded(reg, index, base);
            m_buffer.putByteUnchecked(opcode);
            memoryModRM(reg, base, index, scale, offset);
        }

#if !CPU(X86_64)
        void oneByteOp(OneByteOpcodeID opcode, int reg, const void* address)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            m_buffer.putByteUnchecked(opcode);
            memoryModRM(reg, address);
        }
#endif

        void twoByteOp(TwoByteOpcodeID opcode)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            m_buffer.putByteUnchecked(OP_2BYTE_ESCAPE);
            m_buffer.putByteUnchecked(opcode);
        }

        void twoByteOp(TwoByteOpcodeID opcode, int reg, RegisterID rm)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexIfNeeded(reg, 0, rm);
            m_buffer.putByteUnchecked(OP_2BYTE_ESCAPE);
            m_buffer.putByteUnchecked(opcode);
            registerModRM(reg, rm);
        }

        void twoByteOp(TwoByteOpcodeID opcode, int reg, RegisterID base, int offset)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexIfNeeded(reg, 0, base);
            m_buffer.putByteUnchecked(OP_2BYTE_ESCAPE);
            m_buffer.putByteUnchecked(opcode);
            memoryModRM(reg, base, offset);
        }

        void twoByteOp(TwoByteOpcodeID opcode, int reg, RegisterID base, RegisterID index, int scale, int offset)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexIfNeeded(reg, index, base);
            m_buffer.putByteUnchecked(OP_2BYTE_ESCAPE);
            m_buffer.putByteUnchecked(opcode);
            memoryModRM(reg, base, index, scale, offset);
        }

#if !CPU(X86_64)
        void twoByteOp(TwoByteOpcodeID opcode, int reg, const void* address)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            m_buffer.putByteUnchecked(OP_2BYTE_ESCAPE);
            m_buffer.putByteUnchecked(opcode);
            memoryModRM(reg, address);
        }
#endif

#if CPU(X86_64)
        // Quad-word-sized operands:
        //
        // Used to format 64-bit operantions, planting a REX.w prefix.
        // When planting d64 or f64 instructions, not requiring a REX.w prefix,
        // the normal (non-'64'-postfixed) formatters should be used.

        void oneByteOp64(OneByteOpcodeID opcode)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexW(0, 0, 0);
            m_buffer.putByteUnchecked(opcode);
        }

        void oneByteOp64(OneByteOpcodeID opcode, RegisterID reg)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexW(0, 0, reg);
            m_buffer.putByteUnchecked(opcode + (reg & 7));
        }

        void oneByteOp64(OneByteOpcodeID opcode, int reg, RegisterID rm)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexW(reg, 0, rm);
            m_buffer.putByteUnchecked(opcode);
            registerModRM(reg, rm);
        }

        void oneByteOp64(OneByteOpcodeID opcode, int reg, RegisterID base, int offset)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexW(reg, 0, base);
            m_buffer.putByteUnchecked(opcode);
            memoryModRM(reg, base, offset);
        }

        void oneByteOp64_disp32(OneByteOpcodeID opcode, int reg, RegisterID base, int offset)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexW(reg, 0, base);
            m_buffer.putByteUnchecked(opcode);
            memoryModRM_disp32(reg, base, offset);
        }
        
        void oneByteOp64_disp8(OneByteOpcodeID opcode, int reg, RegisterID base, int offset)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexW(reg, 0, base);
            m_buffer.putByteUnchecked(opcode);
            memoryModRM_disp8(reg, base, offset);
        }

        void oneByteOp64(OneByteOpcodeID opcode, int reg, RegisterID base, RegisterID index, int scale, int offset)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexW(reg, index, base);
            m_buffer.putByteUnchecked(opcode);
            memoryModRM(reg, base, index, scale, offset);
        }

        void twoByteOp64(TwoByteOpcodeID opcode, int reg, RegisterID rm)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexW(reg, 0, rm);
            m_buffer.putByteUnchecked(OP_2BYTE_ESCAPE);
            m_buffer.putByteUnchecked(opcode);
            registerModRM(reg, rm);
        }
#endif

        // Byte-operands:
        //
        // These methods format byte operations.  Byte operations differ from the normal
        // formatters in the circumstances under which they will decide to emit REX prefixes.
        // These should be used where any register operand signifies a byte register.
        //
        // The disctinction is due to the handling of register numbers in the range 4..7 on
        // x86-64.  These register numbers may either represent the second byte of the first
        // four registers (ah..bh) or the first byte of the second four registers (spl..dil).
        //
        // Since ah..bh cannot be used in all permutations of operands (specifically cannot
        // be accessed where a REX prefix is present), these are likely best treated as
        // deprecated.  In order to ensure the correct registers spl..dil are selected a
        // REX prefix will be emitted for any byte register operand in the range 4..15.
        //
        // These formatters may be used in instructions where a mix of operand sizes, in which
        // case an unnecessary REX will be emitted, for example:
        //     movzbl %al, %edi
        // In this case a REX will be planted since edi is 7 (and were this a byte operand
        // a REX would be required to specify dil instead of bh).  Unneeded REX prefixes will
        // be silently ignored by the processor.
        //
        // Address operands should still be checked using regRequiresRex(), while byteRegRequiresRex()
        // is provided to check byte register operands.

        void oneByteOp8(OneByteOpcodeID opcode, GroupOpcodeID groupOp, RegisterID rm)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexIf(byteRegRequiresRex(rm), 0, 0, rm);
            m_buffer.putByteUnchecked(opcode);
            registerModRM(groupOp, rm);
        }

        void oneByteOp8(OneByteOpcodeID opcode, int reg, RegisterID rm)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexIf(byteRegRequiresRex(reg) || byteRegRequiresRex(rm), reg, 0, rm);
            m_buffer.putByteUnchecked(opcode);
            registerModRM(reg, rm);
        }

        void oneByteOp8(OneByteOpcodeID opcode, int reg, RegisterID base, RegisterID index, int scale, int offset)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexIf(byteRegRequiresRex(reg) || regRequiresRex(index) || regRequiresRex(base), reg, index, base);
            m_buffer.putByteUnchecked(opcode);
            memoryModRM(reg, base, index, scale, offset);
        }

        void twoByteOp8(TwoByteOpcodeID opcode, RegisterID reg, RegisterID rm)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexIf(byteRegRequiresRex(reg)|byteRegRequiresRex(rm), reg, 0, rm);
            m_buffer.putByteUnchecked(OP_2BYTE_ESCAPE);
            m_buffer.putByteUnchecked(opcode);
            registerModRM(reg, rm);
        }

        void twoByteOp8(TwoByteOpcodeID opcode, GroupOpcodeID groupOp, RegisterID rm)
        {
            m_buffer.ensureSpace(maxInstructionSize);
            emitRexIf(byteRegRequiresRex(rm), 0, 0, rm);
            m_buffer.putByteUnchecked(OP_2BYTE_ESCAPE);
            m_buffer.putByteUnchecked(opcode);
            registerModRM(groupOp, rm);
        }

        // Immediates:
        //
        // An immedaite should be appended where appropriate after an op has been emitted.
        // The writes are unchecked since the opcode formatters above will have ensured space.

        void immediate8(int imm)
        {
            m_buffer.putByteUnchecked(imm);
        }

        void immediate16(int imm)
        {
            m_buffer.putShortUnchecked(imm);
        }

        void immediate32(int imm)
        {
            m_buffer.putIntUnchecked(imm);
        }

        void immediate64(int64_t imm)
        {
            m_buffer.putInt64Unchecked(imm);
        }

        AssemblerLabel immediateRel32()
        {
            m_buffer.putIntUnchecked(0);
            return label();
        }

        // Administrative methods:

        size_t codeSize() const { return m_buffer.codeSize(); }
        AssemblerLabel label() const { return m_buffer.label(); }
        bool isAligned(int alignment) const { return m_buffer.isAligned(alignment); }
        void* data() const { return m_buffer.data(); }

        PassRefPtr<ExecutableMemoryHandle> executableCopy(VM& vm, void* ownerUID, JITCompilationEffort effort)
        {
            return m_buffer.executableCopy(vm, ownerUID, effort);
        }

        unsigned debugOffset() { return m_buffer.debugOffset(); }

    private:

        // Internals; ModRm and REX formatters.

        static const RegisterID noBase = X86Registers::ebp;
        static const RegisterID hasSib = X86Registers::esp;
        static const RegisterID noIndex = X86Registers::esp;
#if CPU(X86_64)
        static const RegisterID noBase2 = X86Registers::r13;
        static const RegisterID hasSib2 = X86Registers::r12;

        // Registers r8 & above require a REX prefixe.
        inline bool regRequiresRex(int reg)
        {
            return (reg >= X86Registers::r8);
        }

        // Byte operand register spl & above require a REX prefix (to prevent the 'H' registers be accessed).
        inline bool byteRegRequiresRex(int reg)
        {
            return (reg >= X86Registers::esp);
        }

        // Format a REX prefix byte.
        inline void emitRex(bool w, int r, int x, int b)
        {
            ASSERT(r >= 0);
            ASSERT(x >= 0);
            ASSERT(b >= 0);
            m_buffer.putByteUnchecked(PRE_REX | ((int)w << 3) | ((r>>3)<<2) | ((x>>3)<<1) | (b>>3));
        }

        // Used to plant a REX byte with REX.w set (for 64-bit operations).
        inline void emitRexW(int r, int x, int b)
        {
            emitRex(true, r, x, b);
        }

        // Used for operations with byte operands - use byteRegRequiresRex() to check register operands,
        // regRequiresRex() to check other registers (i.e. address base & index).
        inline void emitRexIf(bool condition, int r, int x, int b)
        {
            if (condition) emitRex(false, r, x, b);
        }

        // Used for word sized operations, will plant a REX prefix if necessary (if any register is r8 or above).
        inline void emitRexIfNeeded(int r, int x, int b)
        {
            emitRexIf(regRequiresRex(r) || regRequiresRex(x) || regRequiresRex(b), r, x, b);
        }
#else
        // No REX prefix bytes on 32-bit x86.
        inline bool regRequiresRex(int) { return false; }
        inline bool byteRegRequiresRex(int) { return false; }
        inline void emitRexIf(bool, int, int, int) {}
        inline void emitRexIfNeeded(int, int, int) {}
#endif

        void putModRm(ModRmMode mode, int reg, RegisterID rm)
        {
            m_buffer.putByteUnchecked((mode << 6) | ((reg & 7) << 3) | (rm & 7));
        }

        void putModRmSib(ModRmMode mode, int reg, RegisterID base, RegisterID index, int scale)
        {
            ASSERT(mode != ModRmRegister);

            putModRm(mode, reg, hasSib);
            m_buffer.putByteUnchecked((scale << 6) | ((index & 7) << 3) | (base & 7));
        }

        void registerModRM(int reg, RegisterID rm)
        {
            putModRm(ModRmRegister, reg, rm);
        }

        void memoryModRM(int reg, RegisterID base, int offset)
        {
            // A base of esp or r12 would be interpreted as a sib, so force a sib with no index & put the base in there.
#if CPU(X86_64)
            if ((base == hasSib) || (base == hasSib2)) {
#else
            if (base == hasSib) {
#endif
                if (!offset) // No need to check if the base is noBase, since we know it is hasSib!
                    putModRmSib(ModRmMemoryNoDisp, reg, base, noIndex, 0);
                else if (CAN_SIGN_EXTEND_8_32(offset)) {
                    putModRmSib(ModRmMemoryDisp8, reg, base, noIndex, 0);
                    m_buffer.putByteUnchecked(offset);
                } else {
                    putModRmSib(ModRmMemoryDisp32, reg, base, noIndex, 0);
                    m_buffer.putIntUnchecked(offset);
                }
            } else {
#if CPU(X86_64)
                if (!offset && (base != noBase) && (base != noBase2))
#else
                if (!offset && (base != noBase))
#endif
                    putModRm(ModRmMemoryNoDisp, reg, base);
                else if (CAN_SIGN_EXTEND_8_32(offset)) {
                    putModRm(ModRmMemoryDisp8, reg, base);
                    m_buffer.putByteUnchecked(offset);
                } else {
                    putModRm(ModRmMemoryDisp32, reg, base);
                    m_buffer.putIntUnchecked(offset);
                }
            }
        }

        void memoryModRM_disp8(int reg, RegisterID base, int offset)
        {
            // A base of esp or r12 would be interpreted as a sib, so force a sib with no index & put the base in there.
            ASSERT(CAN_SIGN_EXTEND_8_32(offset));
#if CPU(X86_64)
            if ((base == hasSib) || (base == hasSib2)) {
#else
            if (base == hasSib) {
#endif
                putModRmSib(ModRmMemoryDisp8, reg, base, noIndex, 0);
                m_buffer.putByteUnchecked(offset);
            } else {
                putModRm(ModRmMemoryDisp8, reg, base);
                m_buffer.putByteUnchecked(offset);
            }
        }

        void memoryModRM_disp32(int reg, RegisterID base, int offset)
        {
            // A base of esp or r12 would be interpreted as a sib, so force a sib with no index & put the base in there.
#if CPU(X86_64)
            if ((base == hasSib) || (base == hasSib2)) {
#else
            if (base == hasSib) {
#endif
                putModRmSib(ModRmMemoryDisp32, reg, base, noIndex, 0);
                m_buffer.putIntUnchecked(offset);
            } else {
                putModRm(ModRmMemoryDisp32, reg, base);
                m_buffer.putIntUnchecked(offset);
            }
        }
    
        void memoryModRM(int reg, RegisterID base, RegisterID index, int scale, int offset)
        {
            ASSERT(index != noIndex);

#if CPU(X86_64)
            if (!offset && (base != noBase) && (base != noBase2))
#else
            if (!offset && (base != noBase))
#endif
                putModRmSib(ModRmMemoryNoDisp, reg, base, index, scale);
            else if (CAN_SIGN_EXTEND_8_32(offset)) {
                putModRmSib(ModRmMemoryDisp8, reg, base, index, scale);
                m_buffer.putByteUnchecked(offset);
            } else {
                putModRmSib(ModRmMemoryDisp32, reg, base, index, scale);
                m_buffer.putIntUnchecked(offset);
            }
        }

#if !CPU(X86_64)
        void memoryModRM(int reg, const void* address)
        {
            // noBase + ModRmMemoryNoDisp means noBase + ModRmMemoryDisp32!
            putModRm(ModRmMemoryNoDisp, reg, noBase);
            m_buffer.putIntUnchecked(reinterpret_cast<int32_t>(address));
        }
#endif

        AssemblerBuffer m_buffer;
    } m_formatter;
    int m_indexOfLastWatchpoint;
    int m_indexOfTailOfLastWatchpoint;
};

} // namespace JSC

#endif // ENABLE(ASSEMBLER) && CPU(X86)

#endif // X86Assembler_h
