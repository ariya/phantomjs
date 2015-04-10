/*
 * Copyright (C) 2013 Cisco Systems, Inc. All rights reserved.
 * Copyright (C) 2009-2011 STMicroelectronics. All rights reserved.
 * Copyright (C) 2008 Apple Inc. All rights reserved.
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

#ifndef SH4Assembler_h
#define SH4Assembler_h

#if ENABLE(ASSEMBLER) && CPU(SH4)

#include "AssemblerBuffer.h"
#include "AssemblerBufferWithConstantPool.h"
#include "JITCompilationEffort.h"
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <wtf/Assertions.h>
#include <wtf/DataLog.h>
#include <wtf/Vector.h>

#ifndef NDEBUG
#define SH4_ASSEMBLER_TRACING
#endif

namespace JSC {
typedef uint16_t SH4Word;

enum {
    INVALID_OPCODE = 0xffff,
    ADD_OPCODE = 0x300c,
    ADDIMM_OPCODE = 0x7000,
    ADDC_OPCODE = 0x300e,
    ADDV_OPCODE = 0x300f,
    AND_OPCODE = 0x2009,
    ANDIMM_OPCODE = 0xc900,
    DIV0_OPCODE = 0x2007,
    DIV1_OPCODE = 0x3004,
    BF_OPCODE = 0x8b00,
    BFS_OPCODE = 0x8f00,
    BRA_OPCODE = 0xa000,
    BRAF_OPCODE = 0x0023,
    NOP_OPCODE = 0x0009,
    BSR_OPCODE = 0xb000,
    RTS_OPCODE = 0x000b,
    BT_OPCODE = 0x8900,
    BTS_OPCODE = 0x8d00,
    BSRF_OPCODE = 0x0003,
    BRK_OPCODE = 0x003b,
    FTRC_OPCODE = 0xf03d,
    CMPEQ_OPCODE = 0x3000,
    CMPEQIMM_OPCODE = 0x8800,
    CMPGE_OPCODE = 0x3003,
    CMPGT_OPCODE = 0x3007,
    CMPHI_OPCODE = 0x3006,
    CMPHS_OPCODE = 0x3002,
    CMPPL_OPCODE = 0x4015,
    CMPPZ_OPCODE = 0x4011,
    CMPSTR_OPCODE = 0x200c,
    DT_OPCODE = 0x4010,
    FCMPEQ_OPCODE = 0xf004,
    FCMPGT_OPCODE = 0xf005,
    FMOV_OPCODE = 0xf00c,
    FADD_OPCODE = 0xf000,
    FMUL_OPCODE = 0xf002,
    FSUB_OPCODE = 0xf001,
    FDIV_OPCODE = 0xf003,
    FNEG_OPCODE = 0xf04d,
    JMP_OPCODE = 0x402b,
    JSR_OPCODE = 0x400b,
    LDSPR_OPCODE = 0x402a,
    LDSLPR_OPCODE = 0x4026,
    MOV_OPCODE = 0x6003,
    MOVIMM_OPCODE = 0xe000,
    MOVB_WRITE_RN_OPCODE = 0x2000,
    MOVB_WRITE_RNDEC_OPCODE = 0x2004,
    MOVB_WRITE_R0RN_OPCODE = 0x0004,
    MOVB_WRITE_OFFGBR_OPCODE = 0xc000,
    MOVB_WRITE_OFFRN_OPCODE = 0x8000,
    MOVB_READ_RM_OPCODE = 0x6000,
    MOVB_READ_RMINC_OPCODE = 0x6004,
    MOVB_READ_R0RM_OPCODE = 0x000c,
    MOVB_READ_OFFGBR_OPCODE = 0xc400,
    MOVB_READ_OFFRM_OPCODE = 0x8400,
    MOVL_WRITE_RN_OPCODE = 0x2002,
    MOVL_WRITE_RNDEC_OPCODE = 0x2006,
    MOVL_WRITE_R0RN_OPCODE = 0x0006,
    MOVL_WRITE_OFFGBR_OPCODE = 0xc200,
    MOVL_WRITE_OFFRN_OPCODE = 0x1000,
    MOVL_READ_RM_OPCODE = 0x6002,
    MOVL_READ_RMINC_OPCODE = 0x6006,
    MOVL_READ_R0RM_OPCODE = 0x000e,
    MOVL_READ_OFFGBR_OPCODE = 0xc600,
    MOVL_READ_OFFPC_OPCODE = 0xd000,
    MOVL_READ_OFFRM_OPCODE = 0x5000,
    MOVW_WRITE_RN_OPCODE = 0x2001,
    MOVW_WRITE_R0RN_OPCODE = 0x0005,
    MOVW_READ_RM_OPCODE = 0x6001,
    MOVW_READ_RMINC_OPCODE = 0x6005,
    MOVW_READ_R0RM_OPCODE = 0x000d,
    MOVW_READ_OFFRM_OPCODE = 0x8500,
    MOVW_READ_OFFPC_OPCODE = 0x9000,
    MOVA_READ_OFFPC_OPCODE = 0xc700,
    MOVT_OPCODE = 0x0029,
    MULL_OPCODE = 0x0007,
    DMULL_L_OPCODE = 0x3005,
    STSMACL_OPCODE = 0x001a,
    STSMACH_OPCODE = 0x000a,
    DMULSL_OPCODE = 0x300d,
    NEG_OPCODE = 0x600b,
    NEGC_OPCODE = 0x600a,
    NOT_OPCODE = 0x6007,
    OR_OPCODE = 0x200b,
    ORIMM_OPCODE = 0xcb00,
    ORBIMM_OPCODE = 0xcf00,
    SETS_OPCODE = 0x0058,
    SETT_OPCODE = 0x0018,
    SHAD_OPCODE = 0x400c,
    SHAL_OPCODE = 0x4020,
    SHAR_OPCODE = 0x4021,
    SHLD_OPCODE = 0x400d,
    SHLL_OPCODE = 0x4000,
    SHLL2_OPCODE = 0x4008,
    SHLL8_OPCODE = 0x4018,
    SHLL16_OPCODE = 0x4028,
    SHLR_OPCODE = 0x4001,
    SHLR2_OPCODE = 0x4009,
    SHLR8_OPCODE = 0x4019,
    SHLR16_OPCODE = 0x4029,
    STSPR_OPCODE = 0x002a,
    STSLPR_OPCODE = 0x4022,
    FLOAT_OPCODE = 0xf02d,
    SUB_OPCODE = 0x3008,
    SUBC_OPCODE = 0x300a,
    SUBV_OPCODE = 0x300b,
    TST_OPCODE = 0x2008,
    TSTIMM_OPCODE = 0xc800,
    TSTB_OPCODE = 0xcc00,
    EXTUB_OPCODE = 0x600c,
    EXTUW_OPCODE = 0x600d,
    XOR_OPCODE = 0x200a,
    XORIMM_OPCODE = 0xca00,
    XORB_OPCODE = 0xce00,
    FMOVS_READ_RM_INC_OPCODE = 0xf009,
    FMOVS_READ_RM_OPCODE = 0xf008,
    FMOVS_READ_R0RM_OPCODE = 0xf006,
    FMOVS_WRITE_RN_OPCODE = 0xf00a,
    FMOVS_WRITE_RN_DEC_OPCODE = 0xf00b,
    FMOVS_WRITE_R0RN_OPCODE = 0xf007,
    FCNVDS_DRM_FPUL_OPCODE = 0xf0bd,
    FCNVSD_FPUL_DRN_OPCODE = 0xf0ad,
    LDS_RM_FPUL_OPCODE = 0x405a,
    FLDS_FRM_FPUL_OPCODE = 0xf01d,
    STS_FPUL_RN_OPCODE = 0x005a,
    FSTS_FPUL_FRN_OPCODE = 0xF00d,
    LDSFPSCR_OPCODE = 0x406a,
    STSFPSCR_OPCODE = 0x006a,
    LDSRMFPUL_OPCODE = 0x405a,
    FSTSFPULFRN_OPCODE = 0xf00d,
    FABS_OPCODE = 0xf05d,
    FSQRT_OPCODE = 0xf06d,
    FSCHG_OPCODE = 0xf3fd,
    CLRT_OPCODE = 8,
};

namespace SH4Registers {
typedef enum {
    r0,
    r1,
    r2,
    r3,
    r4,
    r5,
    r6,
    r7,
    r8,
    r9,
    r10,
    r11,
    r12,
    r13,
    r14, fp = r14,
    r15, sp = r15,
    pc,
    pr,
} RegisterID;

typedef enum {
    fr0, dr0 = fr0,
    fr1,
    fr2, dr2 = fr2,
    fr3,
    fr4, dr4 = fr4,
    fr5,
    fr6, dr6 = fr6,
    fr7,
    fr8, dr8 = fr8,
    fr9,
    fr10, dr10 = fr10,
    fr11,
    fr12, dr12 = fr12,
    fr13,
    fr14, dr14 = fr14,
    fr15,
} FPRegisterID;
}

inline uint16_t getOpcodeGroup1(uint16_t opc, int rm, int rn)
{
    return (opc | ((rm & 0xf) << 8) | ((rn & 0xf) << 4));
}

inline uint16_t getOpcodeGroup2(uint16_t opc, int rm)
{
    return (opc | ((rm & 0xf) << 8));
}

inline uint16_t getOpcodeGroup3(uint16_t opc, int rm, int rn)
{
    return (opc | ((rm & 0xf) << 8) | (rn & 0xff));
}

inline uint16_t getOpcodeGroup4(uint16_t opc, int rm, int rn, int offset)
{
    return (opc | ((rm & 0xf) << 8) | ((rn & 0xf) << 4) | (offset & 0xf));
}

inline uint16_t getOpcodeGroup5(uint16_t opc, int rm)
{
    return (opc | (rm & 0xff));
}

inline uint16_t getOpcodeGroup6(uint16_t opc, int rm)
{
    return (opc | (rm & 0xfff));
}

inline uint16_t getOpcodeGroup7(uint16_t opc, int rm)
{
    return (opc | ((rm & 0x7) << 9));
}

inline uint16_t getOpcodeGroup8(uint16_t opc, int rm, int rn)
{
    return (opc | ((rm & 0x7) << 9) | ((rn & 0x7) << 5));
}

inline uint16_t getOpcodeGroup9(uint16_t opc, int rm, int rn)
{
    return (opc | ((rm & 0xf) << 8) | ((rn & 0x7) << 5));
}

inline uint16_t getOpcodeGroup10(uint16_t opc, int rm, int rn)
{
    return (opc | ((rm & 0x7) << 9) | ((rn & 0xf) << 4));
}

inline uint16_t getOpcodeGroup11(uint16_t opc, int rm, int rn)
{
    return (opc | ((rm & 0xf) << 4) | (rn & 0xf));
}

inline uint16_t getRn(uint16_t x)
{
    return ((x & 0xf00) >> 8);
}

inline uint16_t getRm(uint16_t x)
{
    return ((x & 0xf0) >> 4);
}

inline uint16_t getDisp(uint16_t x)
{
    return (x & 0xf);
}

inline uint16_t getImm8(uint16_t x)
{
    return (x & 0xff);
}

inline uint16_t getImm12(uint16_t x)
{
    return (x & 0xfff);
}

inline uint16_t getDRn(uint16_t x)
{
    return ((x & 0xe00) >> 9);
}

inline uint16_t getDRm(uint16_t x)
{
    return ((x & 0xe0) >> 5);
}

class SH4Assembler {
public:
    typedef SH4Registers::RegisterID RegisterID;
    typedef SH4Registers::FPRegisterID FPRegisterID;
    typedef AssemblerBufferWithConstantPool<512, 4, 2, SH4Assembler> SH4Buffer;
    static const RegisterID scratchReg1 = SH4Registers::r3;
    static const RegisterID scratchReg2 = SH4Registers::r11;
    static const uint32_t maxInstructionSize = 16;

    enum {
        padForAlign8 = 0x00,
        padForAlign16 = 0x0009,
        padForAlign32 = 0x00090009,
    };

    enum JumpType {
        JumpFar,
        JumpNear
    };

    SH4Assembler()
        : m_claimscratchReg(0x0)
        , m_indexOfLastWatchpoint(INT_MIN)
        , m_indexOfTailOfLastWatchpoint(INT_MIN)
    {
    }

    // SH4 condition codes
    typedef enum {
        EQ = 0x0, // Equal
        NE = 0x1, // Not Equal
        HS = 0x2, // Unsigned Greater Than equal
        HI = 0x3, // Unsigned Greater Than
        LS = 0x4, // Unsigned Lower or Same
        LI = 0x5, // Unsigned Lower
        GE = 0x6, // Greater or Equal
        LT = 0x7, // Less Than
        GT = 0x8, // Greater Than
        LE = 0x9, // Less or Equal
        OF = 0xa, // OverFlow
        SI = 0xb, // Signed
        NS = 0xc, // Not Signed
        EQU= 0xd, // Equal or unordered(NaN)
        NEU= 0xe,
        GTU= 0xf,
        GEU= 0x10,
        LTU= 0x11,
        LEU= 0x12,
    } Condition;

    // Opaque label types
public:
    bool isImmediate(int constant)
    {
        return ((constant <= 127) && (constant >= -128));
    }

    RegisterID claimScratch()
    {
        ASSERT((m_claimscratchReg != 0x3));

        if (!(m_claimscratchReg & 0x1)) {
            m_claimscratchReg = (m_claimscratchReg | 0x1);
            return scratchReg1;
        }

        m_claimscratchReg = (m_claimscratchReg | 0x2);
        return scratchReg2;
    }

    void releaseScratch(RegisterID scratchR)
    {
        if (scratchR == scratchReg1)
            m_claimscratchReg = (m_claimscratchReg & 0x2);
        else
            m_claimscratchReg = (m_claimscratchReg & 0x1);
    }

    // Stack operations

    void pushReg(RegisterID reg)
    {
        if (reg == SH4Registers::pr) {
            oneShortOp(getOpcodeGroup2(STSLPR_OPCODE, SH4Registers::sp));
            return;
        }

        oneShortOp(getOpcodeGroup1(MOVL_WRITE_RNDEC_OPCODE, SH4Registers::sp, reg));
    }

    void popReg(RegisterID reg)
    {
        if (reg == SH4Registers::pr) {
            oneShortOp(getOpcodeGroup2(LDSLPR_OPCODE, SH4Registers::sp));
            return;
        }

        oneShortOp(getOpcodeGroup1(MOVL_READ_RMINC_OPCODE, reg, SH4Registers::sp));
    }

    void movt(RegisterID dst)
    {
        uint16_t opc = getOpcodeGroup2(MOVT_OPCODE, dst);
        oneShortOp(opc);
    }

    // Arithmetic operations

    void addlRegReg(RegisterID src, RegisterID dst)
    {
        uint16_t opc = getOpcodeGroup1(ADD_OPCODE, dst, src);
        oneShortOp(opc);
    }

    void addclRegReg(RegisterID src, RegisterID dst)
    {
        uint16_t opc = getOpcodeGroup1(ADDC_OPCODE, dst, src);
        oneShortOp(opc);
    }

    void addvlRegReg(RegisterID src, RegisterID dst)
    {
        uint16_t opc = getOpcodeGroup1(ADDV_OPCODE, dst, src);
        oneShortOp(opc);
    }

    void addlImm8r(int imm8, RegisterID dst)
    {
        ASSERT((imm8 <= 127) && (imm8 >= -128));

        uint16_t opc = getOpcodeGroup3(ADDIMM_OPCODE, dst, imm8);
        oneShortOp(opc);
    }

    void andlRegReg(RegisterID src, RegisterID dst)
    {
        uint16_t opc = getOpcodeGroup1(AND_OPCODE, dst, src);
        oneShortOp(opc);
    }

    void andlImm8r(int imm8, RegisterID dst)
    {
        ASSERT((imm8 <= 255) && (imm8 >= 0));
        ASSERT(dst == SH4Registers::r0);

        uint16_t opc = getOpcodeGroup5(ANDIMM_OPCODE, imm8);
        oneShortOp(opc);
    }

    void div1lRegReg(RegisterID src, RegisterID dst)
    {
        uint16_t opc = getOpcodeGroup1(DIV1_OPCODE, dst, src);
        oneShortOp(opc);
    }

    void div0lRegReg(RegisterID src, RegisterID dst)
    {
        uint16_t opc = getOpcodeGroup1(DIV0_OPCODE, dst, src);
        oneShortOp(opc);
    }

    void notlReg(RegisterID src, RegisterID dst)
    {
        uint16_t opc = getOpcodeGroup1(NOT_OPCODE, dst, src);
        oneShortOp(opc);
    }

    void orlRegReg(RegisterID src, RegisterID dst)
    {
        uint16_t opc = getOpcodeGroup1(OR_OPCODE, dst, src);
        oneShortOp(opc);
    }

    void orlImm8r(int imm8, RegisterID dst)
    {
        ASSERT((imm8 <= 255) && (imm8 >= 0));
        ASSERT(dst == SH4Registers::r0);

        uint16_t opc = getOpcodeGroup5(ORIMM_OPCODE, imm8);
        oneShortOp(opc);
    }

    void sublRegReg(RegisterID src, RegisterID dst)
    {
        uint16_t opc = getOpcodeGroup1(SUB_OPCODE, dst, src);
        oneShortOp(opc);
    }

    void subvlRegReg(RegisterID src, RegisterID dst)
    {
        uint16_t opc = getOpcodeGroup1(SUBV_OPCODE, dst, src);
        oneShortOp(opc);
    }

    void xorlRegReg(RegisterID src, RegisterID dst)
    {
        uint16_t opc = getOpcodeGroup1(XOR_OPCODE, dst, src);
        oneShortOp(opc);
    }

    void xorlImm8r(int imm8, RegisterID dst)
    {
        ASSERT((imm8 <= 255) && (imm8 >= 0));
        ASSERT(dst == SH4Registers::r0);

        uint16_t opc = getOpcodeGroup5(XORIMM_OPCODE, imm8);
        oneShortOp(opc);
    }

    void shllImm8r(int imm, RegisterID dst)
    {
        switch (imm) {
        case 1:
            oneShortOp(getOpcodeGroup2(SHLL_OPCODE, dst));
            break;
        case 2:
            oneShortOp(getOpcodeGroup2(SHLL2_OPCODE, dst));
            break;
        case 8:
            oneShortOp(getOpcodeGroup2(SHLL8_OPCODE, dst));
            break;
        case 16:
            oneShortOp(getOpcodeGroup2(SHLL16_OPCODE, dst));
            break;
        default:
            RELEASE_ASSERT_NOT_REACHED();
        }
    }

    void neg(RegisterID dst, RegisterID src)
    {
        uint16_t opc = getOpcodeGroup1(NEG_OPCODE, dst, src);
        oneShortOp(opc);
    }

    void shldRegReg(RegisterID dst, RegisterID rShift)
    {
        oneShortOp(getOpcodeGroup1(SHLD_OPCODE, dst, rShift));
    }

    void shadRegReg(RegisterID dst, RegisterID rShift)
    {
        oneShortOp(getOpcodeGroup1(SHAD_OPCODE, dst, rShift));
    }

    void shlrImm8r(int imm, RegisterID dst)
    {
        switch (imm) {
        case 1:
            oneShortOp(getOpcodeGroup2(SHLR_OPCODE, dst));
            break;
        case 2:
            oneShortOp(getOpcodeGroup2(SHLR2_OPCODE, dst));
            break;
        case 8:
            oneShortOp(getOpcodeGroup2(SHLR8_OPCODE, dst));
            break;
        case 16:
            oneShortOp(getOpcodeGroup2(SHLR16_OPCODE, dst));
            break;
        default:
            RELEASE_ASSERT_NOT_REACHED();
        }
    }

    void shalImm8r(int imm, RegisterID dst)
    {
        switch (imm) {
        case 1:
            oneShortOp(getOpcodeGroup2(SHAL_OPCODE, dst));
            break;
        default:
            RELEASE_ASSERT_NOT_REACHED();
        }
    }

    void sharImm8r(int imm, RegisterID dst)
    {
        switch (imm) {
        case 1:
            oneShortOp(getOpcodeGroup2(SHAR_OPCODE, dst));
            break;
        default:
            RELEASE_ASSERT_NOT_REACHED();
        }
    }

    void imullRegReg(RegisterID src, RegisterID dst)
    {
        uint16_t opc = getOpcodeGroup1(MULL_OPCODE, dst, src);
        oneShortOp(opc);
    }

    void dmullRegReg(RegisterID src, RegisterID dst)
    {
        uint16_t opc = getOpcodeGroup1(DMULL_L_OPCODE, dst, src);
        oneShortOp(opc);
    }

    void dmulslRegReg(RegisterID src, RegisterID dst)
    {
        uint16_t opc = getOpcodeGroup1(DMULSL_OPCODE, dst, src);
        oneShortOp(opc);
    }

    void stsmacl(RegisterID reg)
    {
        uint16_t opc = getOpcodeGroup2(STSMACL_OPCODE, reg);
        oneShortOp(opc);
    }

    void stsmach(RegisterID reg)
    {
        uint16_t opc = getOpcodeGroup2(STSMACH_OPCODE, reg);
        oneShortOp(opc);
    }

    // Comparisons

    void cmplRegReg(RegisterID left, RegisterID right, Condition cond)
    {
        switch (cond) {
        case NE:
            oneShortOp(getOpcodeGroup1(CMPEQ_OPCODE, right, left));
            break;
        case GT:
            oneShortOp(getOpcodeGroup1(CMPGT_OPCODE, right, left));
            break;
        case EQ:
            oneShortOp(getOpcodeGroup1(CMPEQ_OPCODE, right, left));
            break;
        case GE:
            oneShortOp(getOpcodeGroup1(CMPGE_OPCODE, right, left));
            break;
        case HS:
            oneShortOp(getOpcodeGroup1(CMPHS_OPCODE, right, left));
            break;
        case HI:
            oneShortOp(getOpcodeGroup1(CMPHI_OPCODE, right, left));
            break;
        case LI:
            oneShortOp(getOpcodeGroup1(CMPHI_OPCODE, left, right));
            break;
        case LS:
            oneShortOp(getOpcodeGroup1(CMPHS_OPCODE, left, right));
            break;
        case LE:
            oneShortOp(getOpcodeGroup1(CMPGE_OPCODE, left, right));
            break;
        case LT:
            oneShortOp(getOpcodeGroup1(CMPGT_OPCODE, left, right));
            break;
        default:
            RELEASE_ASSERT_NOT_REACHED();
        }
    }

    void cmppl(RegisterID reg)
    {
        uint16_t opc = getOpcodeGroup2(CMPPL_OPCODE, reg);
        oneShortOp(opc);
    }

    void cmppz(RegisterID reg)
    {
        uint16_t opc = getOpcodeGroup2(CMPPZ_OPCODE, reg);
        oneShortOp(opc);
    }

    void cmpEqImmR0(int imm, RegisterID dst)
    {
        uint16_t opc = getOpcodeGroup5(CMPEQIMM_OPCODE, imm);
        oneShortOp(opc);
    }

    void testlRegReg(RegisterID src, RegisterID dst)
    {
        uint16_t opc = getOpcodeGroup1(TST_OPCODE, dst, src);
        oneShortOp(opc);
    }

    void testlImm8r(int imm, RegisterID dst)
    {
        ASSERT((dst == SH4Registers::r0) && (imm <= 255) && (imm >= 0));

        uint16_t opc = getOpcodeGroup5(TSTIMM_OPCODE, imm);
        oneShortOp(opc);
    }

    void nop()
    {
        oneShortOp(NOP_OPCODE, false);
    }

    void sett()
    {
        oneShortOp(SETT_OPCODE);
    }

    void clrt()
    {
        oneShortOp(CLRT_OPCODE);
    }

    void fschg()
    {
        oneShortOp(FSCHG_OPCODE);
    }

    void bkpt()
    {
        oneShortOp(BRK_OPCODE, false);
    }

    void branch(uint16_t opc, int label)
    {
        switch (opc) {
        case BT_OPCODE:
            ASSERT((label <= 127) && (label >= -128));
            oneShortOp(getOpcodeGroup5(BT_OPCODE, label));
            break;
        case BRA_OPCODE:
            ASSERT((label <= 2047) && (label >= -2048));
            oneShortOp(getOpcodeGroup6(BRA_OPCODE, label));
            break;
        case BF_OPCODE:
            ASSERT((label <= 127) && (label >= -128));
            oneShortOp(getOpcodeGroup5(BF_OPCODE, label));
            break;
        default:
            RELEASE_ASSERT_NOT_REACHED();
        }
    }

    void branch(uint16_t opc, RegisterID reg)
    {
        switch (opc) {
        case BRAF_OPCODE:
            oneShortOp(getOpcodeGroup2(BRAF_OPCODE, reg));
            break;
        case JMP_OPCODE:
            oneShortOp(getOpcodeGroup2(JMP_OPCODE, reg));
            break;
        case JSR_OPCODE:
            oneShortOp(getOpcodeGroup2(JSR_OPCODE, reg));
            break;
        case BSRF_OPCODE:
            oneShortOp(getOpcodeGroup2(BSRF_OPCODE, reg));
            break;
        default:
            RELEASE_ASSERT_NOT_REACHED();
        }
    }

    void ldspr(RegisterID reg)
    {
        uint16_t opc = getOpcodeGroup2(LDSPR_OPCODE, reg);
        oneShortOp(opc);
    }

    void stspr(RegisterID reg)
    {
        uint16_t opc = getOpcodeGroup2(STSPR_OPCODE, reg);
        oneShortOp(opc);
    }

    void extub(RegisterID src, RegisterID dst)
    {
        uint16_t opc = getOpcodeGroup1(EXTUB_OPCODE, dst, src);
        oneShortOp(opc);
    }
    
    void extuw(RegisterID src, RegisterID dst)
    {
        uint16_t opc = getOpcodeGroup1(EXTUW_OPCODE, dst, src);
        oneShortOp(opc);
    }

    // float operations

    void ldsrmfpul(RegisterID src)
    {
        uint16_t opc = getOpcodeGroup2(LDS_RM_FPUL_OPCODE, src);
        oneShortOp(opc);
    }

    void fneg(FPRegisterID dst)
    {
        uint16_t opc = getOpcodeGroup2(FNEG_OPCODE, dst);
        oneShortOp(opc, true, false);
    }

    void fsqrt(FPRegisterID dst)
    {
        uint16_t opc = getOpcodeGroup2(FSQRT_OPCODE, dst);
        oneShortOp(opc, true, false);
    }

    void stsfpulReg(RegisterID src)
    {
        uint16_t opc = getOpcodeGroup2(STS_FPUL_RN_OPCODE, src);
        oneShortOp(opc);
    }

    void floatfpulfrn(FPRegisterID src)
    {
        uint16_t opc = getOpcodeGroup2(FLOAT_OPCODE, src);
        oneShortOp(opc, true, false);
    }

    void fmull(FPRegisterID src, FPRegisterID dst)
    {
        uint16_t opc = getOpcodeGroup1(FMUL_OPCODE, dst, src);
        oneShortOp(opc, true, false);
    }

    void fmovsRegReg(FPRegisterID src, FPRegisterID dst)
    {
        uint16_t opc = getOpcodeGroup1(FMOV_OPCODE, dst, src);
        oneShortOp(opc, true, false);
    }

    void fmovsReadrm(RegisterID src, FPRegisterID dst)
    {
        uint16_t opc = getOpcodeGroup1(FMOVS_READ_RM_OPCODE, dst, src);
        oneShortOp(opc, true, false);
    }

    void fmovsWriterm(FPRegisterID src, RegisterID dst)
    {
        uint16_t opc = getOpcodeGroup1(FMOVS_WRITE_RN_OPCODE, dst, src);
        oneShortOp(opc, true, false);
    }

    void fmovsWriter0r(FPRegisterID src, RegisterID dst)
    {
        uint16_t opc = getOpcodeGroup1(FMOVS_WRITE_R0RN_OPCODE, dst, src);
        oneShortOp(opc, true, false);
    }

    void fmovsReadr0r(RegisterID src, FPRegisterID dst)
    {
        uint16_t opc = getOpcodeGroup1(FMOVS_READ_R0RM_OPCODE, dst, src);
        oneShortOp(opc, true, false);
    }

    void fmovsReadrminc(RegisterID src, FPRegisterID dst)
    {
        uint16_t opc = getOpcodeGroup1(FMOVS_READ_RM_INC_OPCODE, dst, src);
        oneShortOp(opc, true, false);
    }

    void fmovsWriterndec(FPRegisterID src, RegisterID dst)
    {
        uint16_t opc = getOpcodeGroup1(FMOVS_WRITE_RN_DEC_OPCODE, dst, src);
        oneShortOp(opc, true, false);
    }

    void ftrcRegfpul(FPRegisterID src)
    {
        uint16_t opc = getOpcodeGroup2(FTRC_OPCODE, src);
        oneShortOp(opc, true, false);
    }

    void fldsfpul(FPRegisterID src)
    {
        uint16_t opc = getOpcodeGroup2(FLDS_FRM_FPUL_OPCODE, src);
        oneShortOp(opc);
    }

    void fstsfpul(FPRegisterID src)
    {
        uint16_t opc = getOpcodeGroup2(FSTS_FPUL_FRN_OPCODE, src);
        oneShortOp(opc);
    }

    void ldsfpscr(RegisterID reg)
    {
        uint16_t opc = getOpcodeGroup2(LDSFPSCR_OPCODE, reg);
        oneShortOp(opc);
    }

    void stsfpscr(RegisterID reg)
    {
        uint16_t opc = getOpcodeGroup2(STSFPSCR_OPCODE, reg);
        oneShortOp(opc);
    }

    // double operations

    void dcnvds(FPRegisterID src)
    {
        uint16_t opc = getOpcodeGroup7(FCNVDS_DRM_FPUL_OPCODE, src >> 1);
        oneShortOp(opc);
    }

    void dcnvsd(FPRegisterID dst)
    {
        uint16_t opc = getOpcodeGroup7(FCNVSD_FPUL_DRN_OPCODE, dst >> 1);
        oneShortOp(opc);
    }

    void dcmppeq(FPRegisterID src, FPRegisterID dst)
    {
        uint16_t opc = getOpcodeGroup8(FCMPEQ_OPCODE, dst >> 1, src >> 1);
        oneShortOp(opc);
    }

    void dcmppgt(FPRegisterID src, FPRegisterID dst)
    {
        uint16_t opc = getOpcodeGroup8(FCMPGT_OPCODE, dst >> 1, src >> 1);
        oneShortOp(opc);
    }

    void dmulRegReg(FPRegisterID src, FPRegisterID dst)
    {
        uint16_t opc = getOpcodeGroup8(FMUL_OPCODE, dst >> 1, src >> 1);
        oneShortOp(opc);
    }

    void dsubRegReg(FPRegisterID src, FPRegisterID dst)
    {
        uint16_t opc = getOpcodeGroup8(FSUB_OPCODE, dst >> 1, src >> 1);
        oneShortOp(opc);
    }

    void daddRegReg(FPRegisterID src, FPRegisterID dst)
    {
        uint16_t opc = getOpcodeGroup8(FADD_OPCODE, dst >> 1, src >> 1);
        oneShortOp(opc);
    }

    void dmovRegReg(FPRegisterID src, FPRegisterID dst)
    {
        uint16_t opc = getOpcodeGroup8(FMOV_OPCODE, dst >> 1, src >> 1);
        oneShortOp(opc);
    }

    void ddivRegReg(FPRegisterID src, FPRegisterID dst)
    {
        uint16_t opc = getOpcodeGroup8(FDIV_OPCODE, dst >> 1, src >> 1);
        oneShortOp(opc);
    }

    void dabs(FPRegisterID dst)
    {
        uint16_t opc = getOpcodeGroup7(FABS_OPCODE, dst >> 1);
        oneShortOp(opc);
    }

    void dsqrt(FPRegisterID dst)
    {
        uint16_t opc = getOpcodeGroup7(FSQRT_OPCODE, dst >> 1);
        oneShortOp(opc);
    }

    void dneg(FPRegisterID dst)
    {
        uint16_t opc = getOpcodeGroup7(FNEG_OPCODE, dst >> 1);
        oneShortOp(opc);
    }

    void fmovReadrm(RegisterID src, FPRegisterID dst)
    {
        uint16_t opc = getOpcodeGroup10(FMOVS_READ_RM_OPCODE, dst >> 1, src);
        oneShortOp(opc);
    }

    void fmovWriterm(FPRegisterID src, RegisterID dst)
    {
        uint16_t opc = getOpcodeGroup9(FMOVS_WRITE_RN_OPCODE, dst, src >> 1);
        oneShortOp(opc);
    }

    void fmovWriter0r(FPRegisterID src, RegisterID dst)
    {
        uint16_t opc = getOpcodeGroup9(FMOVS_WRITE_R0RN_OPCODE, dst, src >> 1);
        oneShortOp(opc);
    }

    void fmovReadr0r(RegisterID src, FPRegisterID dst)
    {
        uint16_t opc = getOpcodeGroup10(FMOVS_READ_R0RM_OPCODE, dst >> 1, src);
        oneShortOp(opc);
    }

    void fmovReadrminc(RegisterID src, FPRegisterID dst)
    {
        uint16_t opc = getOpcodeGroup10(FMOVS_READ_RM_INC_OPCODE, dst >> 1, src);
        oneShortOp(opc);
    }

    void fmovWriterndec(FPRegisterID src, RegisterID dst)
    {
        uint16_t opc = getOpcodeGroup9(FMOVS_WRITE_RN_DEC_OPCODE, dst, src >> 1);
        oneShortOp(opc);
    }

    void floatfpulDreg(FPRegisterID src)
    {
        uint16_t opc = getOpcodeGroup7(FLOAT_OPCODE, src >> 1);
        oneShortOp(opc);
    }

    void ftrcdrmfpul(FPRegisterID src)
    {
        uint16_t opc = getOpcodeGroup7(FTRC_OPCODE, src >> 1);
        oneShortOp(opc);
    }

    // Various move ops

    void movImm8(int imm8, RegisterID dst)
    {
        ASSERT((imm8 <= 127) && (imm8 >= -128));

        uint16_t opc = getOpcodeGroup3(MOVIMM_OPCODE, dst, imm8);
        oneShortOp(opc);
    }

    void movlRegReg(RegisterID src, RegisterID dst)
    {
        uint16_t opc = getOpcodeGroup1(MOV_OPCODE, dst, src);
        oneShortOp(opc);
    }

    void movwRegMem(RegisterID src, RegisterID dst)
    {
        uint16_t opc = getOpcodeGroup1(MOVW_WRITE_RN_OPCODE, dst, src);
        oneShortOp(opc);
    }

    void movwMemReg(RegisterID src, RegisterID dst)
    {
        uint16_t opc = getOpcodeGroup1(MOVW_READ_RM_OPCODE, dst, src);
        oneShortOp(opc);
    }

    void movwMemRegIn(RegisterID base, RegisterID dst)
    {
        uint16_t opc = getOpcodeGroup1(MOVW_READ_RMINC_OPCODE, dst, base);
        oneShortOp(opc);
    }

    void movwPCReg(int offset, RegisterID base, RegisterID dst)
    {
        ASSERT(base == SH4Registers::pc);
        ASSERT((offset <= 255) && (offset >= 0));

        uint16_t opc = getOpcodeGroup3(MOVW_READ_OFFPC_OPCODE, dst, offset);
        oneShortOp(opc);
    }

    void movwMemReg(int offset, RegisterID base, RegisterID dst)
    {
        ASSERT(dst == SH4Registers::r0);

        uint16_t opc = getOpcodeGroup11(MOVW_READ_OFFRM_OPCODE, base, offset);
        oneShortOp(opc);
    }

    void movwR0mr(RegisterID src, RegisterID dst)
    {
        uint16_t opc = getOpcodeGroup1(MOVW_READ_R0RM_OPCODE, dst, src);
        oneShortOp(opc);
    }

    void movwRegMemr0(RegisterID src, RegisterID dst)
    {
        uint16_t opc = getOpcodeGroup1(MOVW_WRITE_R0RN_OPCODE, dst, src);
        oneShortOp(opc);
    }

    void movlRegMem(RegisterID src, int offset, RegisterID base)
    {
        ASSERT((offset <= 15) && (offset >= 0));

        if (!offset) {
            oneShortOp(getOpcodeGroup1(MOVL_WRITE_RN_OPCODE, base, src));
            return;
        }

        oneShortOp(getOpcodeGroup4(MOVL_WRITE_OFFRN_OPCODE, base, src, offset));
    }

    void movlRegMem(RegisterID src, RegisterID base)
    {
        uint16_t opc = getOpcodeGroup1(MOVL_WRITE_RN_OPCODE, base, src);
        oneShortOp(opc);
    }

    void movlMemReg(int offset, RegisterID base, RegisterID dst)
    {
        if (base == SH4Registers::pc) {
            ASSERT((offset <= 255) && (offset >= 0));
            oneShortOp(getOpcodeGroup3(MOVL_READ_OFFPC_OPCODE, dst, offset));
            return;
        }

        ASSERT((offset <= 15) && (offset >= 0));
        if (!offset) {
            oneShortOp(getOpcodeGroup1(MOVL_READ_RM_OPCODE, dst, base));
            return;
        }

        oneShortOp(getOpcodeGroup4(MOVL_READ_OFFRM_OPCODE, dst, base, offset));
    }

    void movlMemRegCompact(int offset, RegisterID base, RegisterID dst)
    {
        oneShortOp(getOpcodeGroup4(MOVL_READ_OFFRM_OPCODE, dst, base, offset));
    }

    void movbRegMem(RegisterID src, RegisterID base)
    {
        uint16_t opc = getOpcodeGroup1(MOVB_WRITE_RN_OPCODE, base, src);
        oneShortOp(opc);
    }

    void movbMemReg(int offset, RegisterID base, RegisterID dst)
    {
        ASSERT(dst == SH4Registers::r0);

        uint16_t opc = getOpcodeGroup11(MOVB_READ_OFFRM_OPCODE, base, offset);
        oneShortOp(opc);
    }

    void movbR0mr(RegisterID src, RegisterID dst)
    {
        uint16_t opc = getOpcodeGroup1(MOVB_READ_R0RM_OPCODE, dst, src);
        oneShortOp(opc);
    }

    void movbMemReg(RegisterID src, RegisterID dst)
    {
        uint16_t opc = getOpcodeGroup1(MOVB_READ_RM_OPCODE, dst, src);
        oneShortOp(opc);
    }

    void movbMemRegIn(RegisterID base, RegisterID dst)
    {
        uint16_t opc = getOpcodeGroup1(MOVB_READ_RMINC_OPCODE, dst, base);
        oneShortOp(opc);
    }

    void movbRegMemr0(RegisterID src, RegisterID dst)
    {
        uint16_t opc = getOpcodeGroup1(MOVB_WRITE_R0RN_OPCODE, dst, src);
        oneShortOp(opc);
    }

    void movlMemReg(RegisterID base, RegisterID dst)
    {
        uint16_t opc = getOpcodeGroup1(MOVL_READ_RM_OPCODE, dst, base);
        oneShortOp(opc);
    }

    void movlMemRegIn(RegisterID base, RegisterID dst)
    {
        uint16_t opc = getOpcodeGroup1(MOVL_READ_RMINC_OPCODE, dst, base);
        oneShortOp(opc);
    }

    void movlR0mr(RegisterID src, RegisterID dst)
    {
        uint16_t opc = getOpcodeGroup1(MOVL_READ_R0RM_OPCODE, dst, src);
        oneShortOp(opc);
    }

    void movlRegMemr0(RegisterID src, RegisterID dst)
    {
        uint16_t opc = getOpcodeGroup1(MOVL_WRITE_R0RN_OPCODE, dst, src);
        oneShortOp(opc);
    }

    void loadConstant(uint32_t constant, RegisterID dst)
    {
        if (((int)constant <= 0x7f) && ((int)constant >= -0x80)) {
            movImm8(constant, dst);
            return;
        }

        uint16_t opc = getOpcodeGroup3(MOVIMM_OPCODE, dst, 0);

        m_buffer.ensureSpace(maxInstructionSize, sizeof(uint32_t));
        printInstr(getOpcodeGroup3(MOVIMM_OPCODE, dst, constant), m_buffer.codeSize());
        m_buffer.putShortWithConstantInt(opc, constant, true);
    }

    void loadConstantUnReusable(uint32_t constant, RegisterID dst, bool ensureSpace = false)
    {
        uint16_t opc = getOpcodeGroup3(MOVIMM_OPCODE, dst, 0);

        if (ensureSpace)
            m_buffer.ensureSpace(maxInstructionSize, sizeof(uint32_t));

        printInstr(getOpcodeGroup3(MOVIMM_OPCODE, dst, constant), m_buffer.codeSize());
        m_buffer.putShortWithConstantInt(opc, constant);
    }

    // Flow control

    AssemblerLabel call()
    {
        RegisterID scr = claimScratch();
        m_buffer.ensureSpace(maxInstructionSize + 4, sizeof(uint32_t));
        loadConstantUnReusable(0x0, scr);
        branch(JSR_OPCODE, scr);
        nop();
        releaseScratch(scr);
        return m_buffer.label();
    }

    AssemblerLabel call(RegisterID dst)
    {
        m_buffer.ensureSpace(maxInstructionSize + 2);
        branch(JSR_OPCODE, dst);
        nop();
        return m_buffer.label();
    }

    AssemblerLabel jmp()
    {
        RegisterID scr = claimScratch();
        m_buffer.ensureSpace(maxInstructionSize + 4, sizeof(uint32_t));
        loadConstantUnReusable(0x0, scr);
        branch(BRAF_OPCODE, scr);
        nop();
        releaseScratch(scr);
        return m_buffer.label();
    }

    AssemblerLabel extraInstrForBranch(RegisterID dst)
    {
        loadConstantUnReusable(0x0, dst);
        branch(BRAF_OPCODE, dst);
        nop();
        return m_buffer.label();
    }

    AssemblerLabel jmp(RegisterID dst)
    {
        jmpReg(dst);
        return m_buffer.label();
    }

    void jmpReg(RegisterID dst)
    {
        m_buffer.ensureSpace(maxInstructionSize + 2);
        branch(JMP_OPCODE, dst);
        nop();
    }

    AssemblerLabel jne()
    {
        branch(BF_OPCODE, 0);
        return m_buffer.label();
    }

    AssemblerLabel je()
    {
        branch(BT_OPCODE, 0);
        return m_buffer.label();
    }

    AssemblerLabel bra()
    {
        branch(BRA_OPCODE, 0);
        return m_buffer.label();
    }

    void ret()
    {
        m_buffer.ensureSpace(maxInstructionSize + 2);
        oneShortOp(RTS_OPCODE, false);
    }

    AssemblerLabel labelIgnoringWatchpoints()
    {
        m_buffer.ensureSpaceForAnyInstruction();
        return m_buffer.label();
    }

    AssemblerLabel labelForWatchpoint()
    {
        m_buffer.ensureSpaceForAnyInstruction();
        AssemblerLabel result = m_buffer.label();
        if (static_cast<int>(result.m_offset) != m_indexOfLastWatchpoint)
            result = label();
        m_indexOfLastWatchpoint = result.m_offset;
        m_indexOfTailOfLastWatchpoint = result.m_offset + maxJumpReplacementSize();
        return result;
    }

    AssemblerLabel label()
    {
        AssemblerLabel result = labelIgnoringWatchpoints();
        while (UNLIKELY(static_cast<int>(result.m_offset) < m_indexOfTailOfLastWatchpoint)) {
            nop();
            result = labelIgnoringWatchpoints();
        }
        return result;
    }

    int sizeOfConstantPool()
    {
        return m_buffer.sizeOfConstantPool();
    }

    AssemblerLabel align(int alignment)
    {
        m_buffer.ensureSpace(maxInstructionSize + 2);
        while (!m_buffer.isAligned(alignment)) {
            nop();
            m_buffer.ensureSpace(maxInstructionSize + 2);
        }
        return label();
    }

    static void changePCrelativeAddress(int offset, uint16_t* instructionPtr, uint32_t newAddress)
    {
        ASSERT((instructionPtr[0] & 0xf000) == MOVL_READ_OFFPC_OPCODE);
        uint32_t address = (offset << 2) + ((reinterpret_cast<uint32_t>(instructionPtr) + 4) &(~0x3));
        *reinterpret_cast<uint32_t*>(address) = newAddress;
    }

    static uint32_t readPCrelativeAddress(int offset, uint16_t* instructionPtr)
    {
        ASSERT((instructionPtr[0] & 0xf000) == MOVL_READ_OFFPC_OPCODE);
        uint32_t address = (offset << 2) + ((reinterpret_cast<uint32_t>(instructionPtr) + 4) &(~0x3));
        return *reinterpret_cast<uint32_t*>(address);
    }

    static uint16_t* getInstructionPtr(void* code, int offset)
    {
        return reinterpret_cast<uint16_t*> (reinterpret_cast<uint32_t>(code) + offset);
    }

    static void linkJump(void* code, AssemblerLabel from, void* to)
    {
        ASSERT(from.isSet());

        uint16_t* instructionPtr = getInstructionPtr(code, from.m_offset) - 3;
        int offsetBits = (reinterpret_cast<uint32_t>(to) - reinterpret_cast<uint32_t>(code)) - from.m_offset;

        /* MOV #imm, reg => LDR reg
           braf @reg        braf @reg
           nop              nop
        */
        ASSERT((instructionPtr[0] & 0xf000) == MOVL_READ_OFFPC_OPCODE);
        ASSERT((instructionPtr[1] & 0xf0ff) == BRAF_OPCODE);
        changePCrelativeAddress((*instructionPtr & 0xff), instructionPtr, offsetBits);
        printInstr(*instructionPtr, from.m_offset + 2);
    }

    static void linkCall(void* code, AssemblerLabel from, void* to)
    {
        uint16_t* instructionPtr = getInstructionPtr(code, from.m_offset);
        instructionPtr -= 3;
        ASSERT((instructionPtr[0] & 0xf000) == MOVL_READ_OFFPC_OPCODE);
        changePCrelativeAddress((*instructionPtr & 0xff), instructionPtr, reinterpret_cast<uint32_t>(to));
    }

    static void linkPointer(void* code, AssemblerLabel where, void* value)
    {
        uint16_t* instructionPtr = getInstructionPtr(code, where.m_offset);
        ASSERT((instructionPtr[0] & 0xf000) == MOVL_READ_OFFPC_OPCODE);
        changePCrelativeAddress((*instructionPtr & 0xff), instructionPtr, reinterpret_cast<uint32_t>(value));
    }

    static unsigned getCallReturnOffset(AssemblerLabel call)
    {
        ASSERT(call.isSet());
        return call.m_offset;
    }

    static uint32_t* getLdrImmAddressOnPool(SH4Word* insn, uint32_t* constPool)
    {
        return (constPool + (*insn & 0xff));
    }

    static SH4Word patchConstantPoolLoad(SH4Word load, int value)
    {
        return ((load & ~0xff) | value);
    }

    static SH4Buffer::TwoShorts placeConstantPoolBarrier(int offset)
    {
        ASSERT(((offset >> 1) <= 2047) && ((offset >> 1) >= -2048));

        SH4Buffer::TwoShorts m_barrier;
        m_barrier.high = (BRA_OPCODE | (offset >> 1));
        m_barrier.low = NOP_OPCODE;
        printInstr(((BRA_OPCODE | (offset >> 1))), 0);
        printInstr(NOP_OPCODE, 0);
        return m_barrier;
    }

    static void patchConstantPoolLoad(void* loadAddr, void* constPoolAddr)
    {
        SH4Word* instructionPtr = reinterpret_cast<SH4Word*>(loadAddr);
        SH4Word instruction = *instructionPtr;
        SH4Word index = instruction & 0xff;

        if ((instruction & 0xf000) != MOVIMM_OPCODE)
            return;

        ASSERT((((reinterpret_cast<uint32_t>(constPoolAddr) - reinterpret_cast<uint32_t>(loadAddr)) + index * 4)) < 1024);

        int offset = reinterpret_cast<uint32_t>(constPoolAddr) + (index * 4) - ((reinterpret_cast<uint32_t>(instructionPtr) & ~0x03) + 4);
        instruction &= 0x0f00;
        instruction |= 0xd000;
        offset &= 0x03ff;
        instruction |= (offset >> 2);
        *instructionPtr = instruction;
        printInstr(instruction, reinterpret_cast<uint32_t>(loadAddr));
    }

    static void repatchPointer(void* where, void* value)
    {
        patchPointer(where, value);
    }

    static void* readPointer(void* code)
    {
        return reinterpret_cast<void*>(readInt32(code));
    }

    static void repatchInt32(void* where, int32_t value)
    {
        uint16_t* instructionPtr = reinterpret_cast<uint16_t*>(where);
        ASSERT((instructionPtr[0] & 0xf000) == MOVL_READ_OFFPC_OPCODE);
        changePCrelativeAddress((*instructionPtr & 0xff), instructionPtr, value);
    }

    static void repatchCompact(void* where, int32_t value)
    {
        uint16_t* instructionPtr = reinterpret_cast<uint16_t*>(where);
        ASSERT(value >= 0);
        ASSERT(value <= 60);

        // Handle the uncommon case where a flushConstantPool occurred in movlMemRegCompact.
        if ((instructionPtr[0] & 0xf000) == BRA_OPCODE)
            instructionPtr += (instructionPtr[0] & 0x0fff) + 2;

        ASSERT((instructionPtr[0] & 0xf000) == MOVL_READ_OFFRM_OPCODE);
        instructionPtr[0] = (instructionPtr[0] & 0xfff0) | (value >> 2);
        cacheFlush(instructionPtr, sizeof(uint16_t));
    }

    static void relinkCall(void* from, void* to)
    {
        uint16_t* instructionPtr = reinterpret_cast<uint16_t*>(from);
        instructionPtr -= 3;
        ASSERT((instructionPtr[0] & 0xf000) == MOVL_READ_OFFPC_OPCODE);
        changePCrelativeAddress((*instructionPtr & 0xff), instructionPtr, reinterpret_cast<uint32_t>(to));
    }

    static void relinkJump(void* from, void* to)
    {
        uint16_t* instructionPtr = reinterpret_cast<uint16_t*> (from);
        instructionPtr -= 3;
        ASSERT((instructionPtr[0] & 0xf000) == MOVL_READ_OFFPC_OPCODE);
        ASSERT((instructionPtr[1] & 0xf0ff) == BRAF_OPCODE);
        changePCrelativeAddress((*instructionPtr & 0xff), instructionPtr, reinterpret_cast<uint32_t>(to) - reinterpret_cast<uint32_t>(from));
    }

    // Linking & patching

    static ptrdiff_t maxJumpReplacementSize()
    {
        return sizeof(SH4Word) * 6;
    }

    static void replaceWithJump(void *instructionStart, void *to)
    {
        SH4Word* instruction = reinterpret_cast<SH4Word*>(instructionStart);
        intptr_t difference = reinterpret_cast<intptr_t>(to) - (reinterpret_cast<intptr_t>(instruction) + 3 * sizeof(SH4Word));

        if ((instruction[0] & 0xf000) == MOVL_READ_OFFPC_OPCODE) {
            // We have an entry in constant pool and we potentially replace a branchPtrWithPatch, so let's backup what would be the
            // condition (CMP/xx and Bx opcodes) for later use in revertJumpReplacementToBranchPtrWithPatch before putting the jump.
            instruction[4] = instruction[1];
            instruction[5] = instruction[2];
            instruction[1] = (BRAF_OPCODE | (instruction[0] & 0x0f00));
            instruction[2] = NOP_OPCODE;
            cacheFlush(&instruction[1], 2 * sizeof(SH4Word));
        } else {
            instruction[0] = getOpcodeGroup3(MOVL_READ_OFFPC_OPCODE, SH4Registers::r13, 1);
            instruction[1] = getOpcodeGroup2(BRAF_OPCODE, SH4Registers::r13);
            instruction[2] = NOP_OPCODE;
            cacheFlush(instruction, 3 * sizeof(SH4Word));
        }

        changePCrelativeAddress(instruction[0] & 0x00ff, instruction, difference);
    }

    static void revertJumpReplacementToBranchPtrWithPatch(void* instructionStart, RegisterID rd, int imm)
    {
        SH4Word *insn = reinterpret_cast<SH4Word*>(instructionStart);
        ASSERT((insn[0] & 0xf000) == MOVL_READ_OFFPC_OPCODE);
        ASSERT((insn[0] & 0x00ff) != 1);

        insn[0] = getOpcodeGroup3(MOVL_READ_OFFPC_OPCODE, SH4Registers::r13, insn[0] & 0x00ff);
        if ((insn[1] & 0xf0ff) == BRAF_OPCODE) {
            insn[1] = (insn[4] & 0xf00f) | (rd << 8) | (SH4Registers::r13 << 4); // Restore CMP/xx opcode.
            insn[2] = insn[5];
            ASSERT(((insn[2] & 0xff00) == BT_OPCODE) || ((insn[2] & 0xff00) == BF_OPCODE));
            ASSERT((insn[3] & 0xf000) == MOVL_READ_OFFPC_OPCODE);
            insn[4] = (BRAF_OPCODE | (insn[3] & 0x0f00));
            insn[5] = NOP_OPCODE;
            cacheFlush(insn, 6 * sizeof(SH4Word));
        } else {
            // The branchPtrWithPatch has already been restored, so we just patch the immediate value and ASSERT all is as expected.
            ASSERT((insn[1] & 0xf000) == 0x3000);
            insn[1] = (insn[1] & 0xf00f) | (rd << 8) | (SH4Registers::r13 << 4);
            cacheFlush(insn, 2 * sizeof(SH4Word));
            ASSERT(((insn[2] & 0xff00) == BT_OPCODE) || ((insn[2] & 0xff00) == BF_OPCODE));
            ASSERT((insn[3] & 0xf000) == MOVL_READ_OFFPC_OPCODE);
            ASSERT(insn[5] == NOP_OPCODE);
        }

        changePCrelativeAddress(insn[0] & 0x00ff, insn, imm);
    }

    void linkJump(AssemblerLabel from, AssemblerLabel to, JumpType type = JumpFar)
    {
        ASSERT(to.isSet());
        ASSERT(from.isSet());

        uint16_t* instructionPtr = getInstructionPtr(data(), from.m_offset) - 1;
        int offsetBits = (to.m_offset - from.m_offset);

        if (type == JumpNear) {
            uint16_t instruction = instructionPtr[0];
            int offset = (offsetBits - 2);
            ASSERT((((instruction == BT_OPCODE) || (instruction == BF_OPCODE)) && (offset >= -256) && (offset <= 254))
                || ((instruction == BRA_OPCODE) && (offset >= -4096) && (offset <= 4094)));
            *instructionPtr++ = instruction | (offset >> 1);
            printInstr(*instructionPtr, from.m_offset + 2);
            return;
        }

        /* MOV # imm, reg => LDR reg
           braf @reg         braf @reg
           nop               nop
        */
        instructionPtr -= 2;
        ASSERT((instructionPtr[1] & 0xf0ff) == BRAF_OPCODE);

        if ((instructionPtr[0] & 0xf000) == MOVIMM_OPCODE) {
            uint32_t* addr = getLdrImmAddressOnPool(instructionPtr, m_buffer.poolAddress());
            *addr = offsetBits;
            printInstr(*instructionPtr, from.m_offset + 2);
            return;
        }

        ASSERT((instructionPtr[0] & 0xf000) == MOVL_READ_OFFPC_OPCODE);
        changePCrelativeAddress((*instructionPtr & 0xff), instructionPtr, offsetBits);
        printInstr(*instructionPtr, from.m_offset + 2);
    }

    static void* getRelocatedAddress(void* code, AssemblerLabel label)
    {
        return reinterpret_cast<void*>(reinterpret_cast<char*>(code) + label.m_offset);
    }

    static int getDifferenceBetweenLabels(AssemblerLabel a, AssemblerLabel b)
    {
        return b.m_offset - a.m_offset;
    }

    static void patchPointer(void* code, AssemblerLabel where, void* value)
    {
        patchPointer(reinterpret_cast<uint32_t*>(code) + where.m_offset, value);
    }

    static void patchPointer(void* code, void* value)
    {
        patchInt32(code, reinterpret_cast<uint32_t>(value));
    }

    static void patchInt32(void* code, uint32_t value)
    {
        changePCrelativeAddress((*(reinterpret_cast<uint16_t*>(code)) & 0xff), reinterpret_cast<uint16_t*>(code), value);
    }

    static uint32_t readInt32(void* code)
    {
        return readPCrelativeAddress((*(reinterpret_cast<uint16_t*>(code)) & 0xff), reinterpret_cast<uint16_t*>(code));
    }

    static void* readCallTarget(void* from)
    {
        uint16_t* instructionPtr = static_cast<uint16_t*>(from);
        instructionPtr -= 3;
        return reinterpret_cast<void*>(readPCrelativeAddress((*instructionPtr & 0xff), instructionPtr));
    }

    PassRefPtr<ExecutableMemoryHandle> executableCopy(VM& vm, void* ownerUID, JITCompilationEffort effort)
    {
        return m_buffer.executableCopy(vm, ownerUID, effort);
    }

    static void cacheFlush(void* code, size_t size)
    {
#if OS(LINUX)
        // Flush each page separately, otherwise the whole flush will fail if an uncommited page is in the area.
        unsigned currentPage = reinterpret_cast<unsigned>(code) & ~(pageSize() - 1);
        unsigned lastPage = (reinterpret_cast<unsigned>(code) + size - 1) & ~(pageSize() - 1);
        do {
#if defined CACHEFLUSH_D_L2
            syscall(__NR_cacheflush, currentPage, pageSize(), CACHEFLUSH_D_WB | CACHEFLUSH_I | CACHEFLUSH_D_L2);
#else
            syscall(__NR_cacheflush, currentPage, pageSize(), CACHEFLUSH_D_WB | CACHEFLUSH_I);
#endif
            currentPage += pageSize();
        } while (lastPage >= currentPage);
#else
#error "The cacheFlush support is missing on this platform."
#endif
    }

    void prefix(uint16_t pre)
    {
        m_buffer.putByte(pre);
    }

    void oneShortOp(uint16_t opcode, bool checksize = true, bool isDouble = true)
    {
        printInstr(opcode, m_buffer.codeSize(), isDouble);
        if (checksize)
            m_buffer.ensureSpace(maxInstructionSize);
        m_buffer.putShortUnchecked(opcode);
    }

    void ensureSpace(int space)
    {
        m_buffer.ensureSpace(space);
    }

    void ensureSpace(int insnSpace, int constSpace)
    {
        m_buffer.ensureSpace(insnSpace, constSpace);
    }

    // Administrative methods

    void* data() const { return m_buffer.data(); }
    size_t codeSize() const { return m_buffer.codeSize(); }

    unsigned debugOffset() { return m_buffer.debugOffset(); }

#ifdef SH4_ASSEMBLER_TRACING
    static void printInstr(uint16_t opc, unsigned size, bool isdoubleInst = true)
    {
        if (!getenv("JavaScriptCoreDumpJIT"))
            return;

        const char *format = 0;
        printfStdoutInstr("offset: 0x%8.8x\t", size);
        switch (opc) {
        case BRK_OPCODE:
            format = "    BRK\n";
            break;
        case NOP_OPCODE:
            format = "    NOP\n";
            break;
        case RTS_OPCODE:
            format ="    *RTS\n";
            break;
        case SETS_OPCODE:
            format = "    SETS\n";
            break;
        case SETT_OPCODE:
            format = "    SETT\n";
            break;
        case CLRT_OPCODE:
            format = "    CLRT\n";
            break;
        case FSCHG_OPCODE:
            format = "    FSCHG\n";
            break;
        }
        if (format) {
            printfStdoutInstr(format);
            return;
        }
        switch (opc & 0xf0ff) {
        case BRAF_OPCODE:
            format = "    *BRAF R%d\n";
            break;
        case DT_OPCODE:
            format = "    DT R%d\n";
            break;
        case CMPPL_OPCODE:
            format = "    CMP/PL R%d\n";
            break;
        case CMPPZ_OPCODE:
            format = "    CMP/PZ R%d\n";
            break;
        case JMP_OPCODE:
            format = "    *JMP @R%d\n";
            break;
        case JSR_OPCODE:
            format = "    *JSR @R%d\n";
            break;
        case LDSPR_OPCODE:
            format = "    LDS R%d, PR\n";
            break;
        case LDSLPR_OPCODE:
            format = "    LDS.L @R%d+, PR\n";
            break;
        case MOVT_OPCODE:
            format = "    MOVT R%d\n";
            break;
        case SHAL_OPCODE:
            format = "    SHAL R%d\n";
            break;
        case SHAR_OPCODE:
            format = "    SHAR R%d\n";
            break;
        case SHLL_OPCODE:
            format = "    SHLL R%d\n";
            break;
        case SHLL2_OPCODE:
            format = "    SHLL2 R%d\n";
            break;
        case SHLL8_OPCODE:
            format = "    SHLL8 R%d\n";
            break;
        case SHLL16_OPCODE:
            format = "    SHLL16 R%d\n";
            break;
        case SHLR_OPCODE:
            format = "    SHLR R%d\n";
            break;
        case SHLR2_OPCODE:
            format = "    SHLR2 R%d\n";
            break;
        case SHLR8_OPCODE:
            format = "    SHLR8 R%d\n";
            break;
        case SHLR16_OPCODE:
            format = "    SHLR16 R%d\n";
            break;
        case STSPR_OPCODE:
            format = "    STS PR, R%d\n";
            break;
        case STSLPR_OPCODE:
            format = "    STS.L PR, @-R%d\n";
            break;
        case LDS_RM_FPUL_OPCODE:
            format = "    LDS R%d, FPUL\n";
            break;
        case STS_FPUL_RN_OPCODE:
            format = "    STS FPUL, R%d \n";
            break;
        case FLDS_FRM_FPUL_OPCODE:
            format = "    FLDS FR%d, FPUL\n";
            break;
        case FSTS_FPUL_FRN_OPCODE:
            format = "    FSTS FPUL, R%d \n";
            break;
        case LDSFPSCR_OPCODE:
            format = "    LDS R%d, FPSCR \n";
            break;
        case STSFPSCR_OPCODE:
            format = "    STS FPSCR, R%d \n";
            break;
        case STSMACL_OPCODE:
            format = "    STS MACL, R%d \n";
            break;
        case STSMACH_OPCODE:
            format = "    STS MACH, R%d \n";
            break;
        case BSRF_OPCODE:
            format = "    *BSRF R%d";
            break;
        case FTRC_OPCODE:
            format = "    FTRC FR%d, FPUL\n";
            break;
        }
        if (format) {
            printfStdoutInstr(format, getRn(opc));
            return;
        }
        switch (opc & 0xf0ff) {
        case FNEG_OPCODE:
            format = "    FNEG DR%d\n";
            break;
        case FLOAT_OPCODE:
            format = "    FLOAT DR%d\n";
            break;
        case FTRC_OPCODE:
            format = "    FTRC FR%d, FPUL\n";
            break;
        case FABS_OPCODE:
            format = "    FABS FR%d\n";
            break;
        case FSQRT_OPCODE:
            format = "    FSQRT FR%d\n";
            break;
        case FCNVDS_DRM_FPUL_OPCODE:
            format = "    FCNVDS FR%d, FPUL\n";
            break;
        case FCNVSD_FPUL_DRN_OPCODE:
            format = "    FCNVSD FPUL, FR%d\n";
            break;
        }
        if (format) {
            if (isdoubleInst)
                printfStdoutInstr(format, getDRn(opc) << 1);
            else
                printfStdoutInstr(format, getRn(opc));
            return;
        }
        switch (opc & 0xf00f) {
        case ADD_OPCODE:
            format = "    ADD R%d, R%d\n";
            break;
        case ADDC_OPCODE:
            format = "    ADDC R%d, R%d\n";
            break;
        case ADDV_OPCODE:
            format = "    ADDV R%d, R%d\n";
            break;
        case AND_OPCODE:
            format = "    AND R%d, R%d\n";
            break;
        case DIV1_OPCODE:
            format = "    DIV1 R%d, R%d\n";
            break;
        case CMPEQ_OPCODE:
            format = "    CMP/EQ R%d, R%d\n";
            break;
        case CMPGE_OPCODE:
            format = "    CMP/GE R%d, R%d\n";
            break;
        case CMPGT_OPCODE:
            format = "    CMP/GT R%d, R%d\n";
            break;
        case CMPHI_OPCODE:
            format = "    CMP/HI R%d, R%d\n";
            break;
        case CMPHS_OPCODE:
            format = "    CMP/HS R%d, R%d\n";
            break;
        case MOV_OPCODE:
            format = "    MOV R%d, R%d\n";
            break;
        case MOVB_WRITE_RN_OPCODE:
            format = "    MOV.B R%d, @R%d\n";
            break;
        case MOVB_WRITE_RNDEC_OPCODE:
            format = "    MOV.B R%d, @-R%d\n";
            break;
        case MOVB_WRITE_R0RN_OPCODE:
            format = "    MOV.B R%d, @(R0, R%d)\n";
            break;
        case MOVB_READ_RM_OPCODE:
            format = "    MOV.B @R%d, R%d\n";
            break;
        case MOVB_READ_RMINC_OPCODE:
            format = "    MOV.B @R%d+, R%d\n";
            break;
        case MOVB_READ_R0RM_OPCODE:
            format = "    MOV.B @(R0, R%d), R%d\n";
            break;
        case MOVL_WRITE_RN_OPCODE:
            format = "    MOV.L R%d, @R%d\n";
            break;
        case MOVL_WRITE_RNDEC_OPCODE:
            format = "    MOV.L R%d, @-R%d\n";
            break;
        case MOVL_WRITE_R0RN_OPCODE:
            format = "    MOV.L R%d, @(R0, R%d)\n";
            break;
        case MOVL_READ_RM_OPCODE:
            format = "    MOV.L @R%d, R%d\n";
            break;
        case MOVL_READ_RMINC_OPCODE:
            format = "    MOV.L @R%d+, R%d\n";
            break;
        case MOVL_READ_R0RM_OPCODE:
            format = "    MOV.L @(R0, R%d), R%d\n";
            break;
        case MULL_OPCODE:
            format = "    MUL.L R%d, R%d\n";
            break;
        case DMULL_L_OPCODE:
            format = "    DMULU.L R%d, R%d\n";
            break;
        case DMULSL_OPCODE:
            format = "    DMULS.L R%d, R%d\n";
            break;
        case NEG_OPCODE:
            format = "    NEG R%d, R%d\n";
            break;
        case NEGC_OPCODE:
            format = "    NEGC R%d, R%d\n";
            break;
        case NOT_OPCODE:
            format = "    NOT R%d, R%d\n";
            break;
        case OR_OPCODE:
            format = "    OR R%d, R%d\n";
            break;
        case SHAD_OPCODE:
            format = "    SHAD R%d, R%d\n";
            break;
        case SHLD_OPCODE:
            format = "    SHLD R%d, R%d\n";
            break;
        case SUB_OPCODE:
            format = "    SUB R%d, R%d\n";
            break;
        case SUBC_OPCODE:
            format = "    SUBC R%d, R%d\n";
            break;
        case SUBV_OPCODE:
            format = "    SUBV R%d, R%d\n";
            break;
        case TST_OPCODE:
            format = "    TST R%d, R%d\n";
            break;
        case XOR_OPCODE:
            format = "    XOR R%d, R%d\n";break;
        case MOVW_WRITE_RN_OPCODE:
            format = "    MOV.W R%d, @R%d\n";
            break;
        case MOVW_READ_RM_OPCODE:
            format = "    MOV.W @R%d, R%d\n";
            break;
        case MOVW_READ_RMINC_OPCODE:
            format = "    MOV.W @R%d+, R%d\n";
            break;
        case MOVW_READ_R0RM_OPCODE:
            format = "    MOV.W @(R0, R%d), R%d\n";
            break;
        case MOVW_WRITE_R0RN_OPCODE:
            format = "    MOV.W R%d, @(R0, R%d)\n";
            break;
        case EXTUB_OPCODE:
            format = "    EXTU.B R%d, R%d\n";
            break;
        case EXTUW_OPCODE:
            format = "    EXTU.W R%d, R%d\n";
            break;
        }
        if (format) {
            printfStdoutInstr(format, getRm(opc), getRn(opc));
            return;
        }
        switch (opc & 0xf00f) {
        case FSUB_OPCODE:
            format = "    FSUB FR%d, FR%d\n";
            break;
        case FADD_OPCODE:
            format = "    FADD FR%d, FR%d\n";
            break;
        case FDIV_OPCODE:
            format = "    FDIV FR%d, FR%d\n";
            break;
        case FMUL_OPCODE:
            format = "    DMULL FR%d, FR%d\n";
            break;
        case FMOV_OPCODE:
            format = "    FMOV FR%d, FR%d\n";
            break;
        case FCMPEQ_OPCODE:
            format = "    FCMP/EQ FR%d, FR%d\n";
            break;
        case FCMPGT_OPCODE:
            format = "    FCMP/GT FR%d, FR%d\n";
            break;
        }
        if (format) {
            if (isdoubleInst)
                printfStdoutInstr(format, getDRm(opc) << 1, getDRn(opc) << 1);
            else
                printfStdoutInstr(format, getRm(opc), getRn(opc));
            return;
        }
        switch (opc & 0xf00f) {
        case FMOVS_WRITE_RN_DEC_OPCODE:
            format = "    %s FR%d, @-R%d\n";
            break;
        case FMOVS_WRITE_RN_OPCODE:
            format = "    %s FR%d, @R%d\n";
            break;
        case FMOVS_WRITE_R0RN_OPCODE:
            format = "    %s FR%d, @(R0, R%d)\n";
            break;
        }
        if (format) {
            if (isdoubleInst)
                printfStdoutInstr(format, "FMOV", getDRm(opc) << 1, getDRn(opc));
            else
                printfStdoutInstr(format, "FMOV.S", getRm(opc), getRn(opc));
            return;
        }
        switch (opc & 0xf00f) {
        case FMOVS_READ_RM_OPCODE:
            format = "    %s @R%d, FR%d\n";
            break;
        case FMOVS_READ_RM_INC_OPCODE:
            format = "    %s @R%d+, FR%d\n";
            break;
        case FMOVS_READ_R0RM_OPCODE:
            format = "    %s @(R0, R%d), FR%d\n";
            break;
        }
        if (format) {
            if (isdoubleInst)
                printfStdoutInstr(format, "FMOV", getDRm(opc), getDRn(opc) << 1);
            else
                printfStdoutInstr(format, "FMOV.S", getRm(opc), getRn(opc));
            return;
        }
        switch (opc & 0xff00) {
        case BF_OPCODE:
            format = "    BF %d\n";
            break;
        case BFS_OPCODE:
            format = "    *BF/S %d\n";
            break;
        case ANDIMM_OPCODE:
            format = "    AND #%d, R0\n";
            break;
        case BT_OPCODE:
            format = "    BT %d\n";
            break;
        case BTS_OPCODE:
            format = "    *BT/S %d\n";
            break;
        case CMPEQIMM_OPCODE:
            format = "    CMP/EQ #%d, R0\n";
            break;
        case MOVB_WRITE_OFFGBR_OPCODE:
            format = "    MOV.B R0, @(%d, GBR)\n";
            break;
        case MOVB_READ_OFFGBR_OPCODE:
            format = "    MOV.B @(%d, GBR), R0\n";
            break;
        case MOVL_WRITE_OFFGBR_OPCODE:
            format = "    MOV.L R0, @(%d, GBR)\n";
            break;
        case MOVL_READ_OFFGBR_OPCODE:
            format = "    MOV.L @(%d, GBR), R0\n";
            break;
        case MOVA_READ_OFFPC_OPCODE:
            format = "    MOVA @(%d, PC), R0\n";
            break;
        case ORIMM_OPCODE:
            format = "    OR #%d, R0\n";
            break;
        case ORBIMM_OPCODE:
            format = "    OR.B #%d, @(R0, GBR)\n";
            break;
        case TSTIMM_OPCODE:
            format = "    TST #%d, R0\n";
            break;
        case TSTB_OPCODE:
            format = "    TST.B %d, @(R0, GBR)\n";
            break;
        case XORIMM_OPCODE:
            format = "    XOR #%d, R0\n";
            break;
        case XORB_OPCODE:
            format = "    XOR.B %d, @(R0, GBR)\n";
            break;
        }
        if (format) {
            printfStdoutInstr(format, getImm8(opc));
            return;
        }
        switch (opc & 0xff00) {
        case MOVB_WRITE_OFFRN_OPCODE:
            format = "    MOV.B R0, @(%d, R%d)\n";
            break;
        case MOVB_READ_OFFRM_OPCODE:
            format = "    MOV.B @(%d, R%d), R0\n";
            break;
        }
        if (format) {
            printfStdoutInstr(format, getDisp(opc), getRm(opc));
            return;
        }
        switch (opc & 0xf000) {
        case BRA_OPCODE:
            format = "    *BRA %d\n";
            break;
        case BSR_OPCODE:
            format = "    *BSR %d\n";
            break;
        }
        if (format) {
            printfStdoutInstr(format, getImm12(opc));
            return;
        }
        switch (opc & 0xf000) {
        case MOVL_READ_OFFPC_OPCODE:
            format = "    MOV.L @(%d, PC), R%d\n";
            break;
        case ADDIMM_OPCODE:
            format = "    ADD #%d, R%d\n";
            break;
        case MOVIMM_OPCODE:
            format = "    MOV #%d, R%d\n";
            break;
        case MOVW_READ_OFFPC_OPCODE:
            format = "    MOV.W @(%d, PC), R%d\n";
            break;
        }
        if (format) {
            printfStdoutInstr(format, getImm8(opc), getRn(opc));
            return;
        }
        switch (opc & 0xf000) {
        case MOVL_WRITE_OFFRN_OPCODE:
            format = "    MOV.L R%d, @(%d, R%d)\n";
            printfStdoutInstr(format, getRm(opc), getDisp(opc), getRn(opc));
            break;
        case MOVL_READ_OFFRM_OPCODE:
            format = "    MOV.L @(%d, R%d), R%d\n";
            printfStdoutInstr(format, getDisp(opc), getRm(opc), getRn(opc));
            break;
        }
    }

    static void printfStdoutInstr(const char* format, ...)
    {
        if (getenv("JavaScriptCoreDumpJIT")) {
            va_list args;
            va_start(args, format);
            vprintfStdoutInstr(format, args);
            va_end(args);
        }
    }

    static void vprintfStdoutInstr(const char* format, va_list args)
    {
        if (getenv("JavaScriptCoreDumpJIT"))
            WTF::dataLogFV(format, args);
    }

    static void printBlockInstr(uint16_t* first, unsigned offset, int nbInstr)
    {
        printfStdoutInstr(">> repatch instructions after link\n");
        for (int i = 0; i <= nbInstr; i++)
            printInstr(*(first + i), offset + i);
        printfStdoutInstr(">> end repatch\n");
    }
#else
    static void printInstr(uint16_t opc, unsigned size, bool isdoubleInst = true) { };
    static void printBlockInstr(uint16_t* first, unsigned offset, int nbInstr) { };
#endif

    static void replaceWithLoad(void* instructionStart)
    {
        SH4Word* insPtr = reinterpret_cast<SH4Word*>(instructionStart);

        insPtr += 2; // skip MOV and ADD opcodes

        if (((*insPtr) & 0xf00f) != MOVL_READ_RM_OPCODE) {
            *insPtr = MOVL_READ_RM_OPCODE | (*insPtr & 0x0ff0);
            cacheFlush(insPtr, sizeof(SH4Word));
        }
    }

    static void replaceWithAddressComputation(void* instructionStart)
    {
        SH4Word* insPtr = reinterpret_cast<SH4Word*>(instructionStart);

        insPtr += 2; // skip MOV and ADD opcodes

        if (((*insPtr) & 0xf00f) != MOV_OPCODE) {
            *insPtr = MOV_OPCODE | (*insPtr & 0x0ff0);
            cacheFlush(insPtr, sizeof(SH4Word));
        }
    }

private:
    SH4Buffer m_buffer;
    int m_claimscratchReg;
    int m_indexOfLastWatchpoint;
    int m_indexOfTailOfLastWatchpoint;
};

} // namespace JSC

#endif // ENABLE(ASSEMBLER) && CPU(SH4)

#endif // SH4Assembler_h
