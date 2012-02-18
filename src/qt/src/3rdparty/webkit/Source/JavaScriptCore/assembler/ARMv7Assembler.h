/*
 * Copyright (C) 2009, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 University of Szeged
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

#ifndef ARMAssembler_h
#define ARMAssembler_h

#if ENABLE(ASSEMBLER) && CPU(ARM_THUMB2)

#include "AssemblerBuffer.h"
#include <wtf/Assertions.h>
#include <wtf/Vector.h>
#include <stdint.h>

namespace JSC {

namespace ARMRegisters {
    typedef enum {
        r0,
        r1,
        r2,
        r3,
        r4,
        r5,
        r6,
        r7, wr = r7,   // thumb work register
        r8,
        r9, sb = r9,   // static base
        r10, sl = r10, // stack limit
        r11, fp = r11, // frame pointer
        r12, ip = r12,
        r13, sp = r13,
        r14, lr = r14,
        r15, pc = r15,
    } RegisterID;

    typedef enum {
        s0,
        s1,
        s2,
        s3,
        s4,
        s5,
        s6,
        s7,
        s8,
        s9,
        s10,
        s11,
        s12,
        s13,
        s14,
        s15,
        s16,
        s17,
        s18,
        s19,
        s20,
        s21,
        s22,
        s23,
        s24,
        s25,
        s26,
        s27,
        s28,
        s29,
        s30,
        s31,
    } FPSingleRegisterID;

    typedef enum {
        d0,
        d1,
        d2,
        d3,
        d4,
        d5,
        d6,
        d7,
        d8,
        d9,
        d10,
        d11,
        d12,
        d13,
        d14,
        d15,
        d16,
        d17,
        d18,
        d19,
        d20,
        d21,
        d22,
        d23,
        d24,
        d25,
        d26,
        d27,
        d28,
        d29,
        d30,
        d31,
    } FPDoubleRegisterID;

    typedef enum {
        q0,
        q1,
        q2,
        q3,
        q4,
        q5,
        q6,
        q7,
        q8,
        q9,
        q10,
        q11,
        q12,
        q13,
        q14,
        q15,
        q16,
        q17,
        q18,
        q19,
        q20,
        q21,
        q22,
        q23,
        q24,
        q25,
        q26,
        q27,
        q28,
        q29,
        q30,
        q31,
    } FPQuadRegisterID;

    inline FPSingleRegisterID asSingle(FPDoubleRegisterID reg)
    {
        ASSERT(reg < d16);
        return (FPSingleRegisterID)(reg << 1);
    }

    inline FPDoubleRegisterID asDouble(FPSingleRegisterID reg)
    {
        ASSERT(!(reg & 1));
        return (FPDoubleRegisterID)(reg >> 1);
    }
}

class ARMv7Assembler;
class ARMThumbImmediate {
    friend class ARMv7Assembler;

    typedef uint8_t ThumbImmediateType;
    static const ThumbImmediateType TypeInvalid = 0;
    static const ThumbImmediateType TypeEncoded = 1;
    static const ThumbImmediateType TypeUInt16 = 2;

    typedef union {
        int16_t asInt;
        struct {
            unsigned imm8 : 8;
            unsigned imm3 : 3;
            unsigned i    : 1;
            unsigned imm4 : 4;
        };
        // If this is an encoded immediate, then it may describe a shift, or a pattern.
        struct {
            unsigned shiftValue7 : 7;
            unsigned shiftAmount : 5;
        };
        struct {
            unsigned immediate   : 8;
            unsigned pattern     : 4;
        };
    } ThumbImmediateValue;

    // byte0 contains least significant bit; not using an array to make client code endian agnostic.
    typedef union {
        int32_t asInt;
        struct {
            uint8_t byte0;
            uint8_t byte1;
            uint8_t byte2;
            uint8_t byte3;
        };
    } PatternBytes;

    ALWAYS_INLINE static void countLeadingZerosPartial(uint32_t& value, int32_t& zeros, const int N)
    {
        if (value & ~((1 << N) - 1)) /* check for any of the top N bits (of 2N bits) are set */
            value >>= N;             /* if any were set, lose the bottom N */
        else                         /* if none of the top N bits are set, */
            zeros += N;              /* then we have identified N leading zeros */
    }

    static int32_t countLeadingZeros(uint32_t value)
    {
        if (!value)
            return 32;

        int32_t zeros = 0;
        countLeadingZerosPartial(value, zeros, 16);
        countLeadingZerosPartial(value, zeros, 8);
        countLeadingZerosPartial(value, zeros, 4);
        countLeadingZerosPartial(value, zeros, 2);
        countLeadingZerosPartial(value, zeros, 1);
        return zeros;
    }

    ARMThumbImmediate()
        : m_type(TypeInvalid)
    {
        m_value.asInt = 0;
    }
        
    ARMThumbImmediate(ThumbImmediateType type, ThumbImmediateValue value)
        : m_type(type)
        , m_value(value)
    {
    }

    ARMThumbImmediate(ThumbImmediateType type, uint16_t value)
        : m_type(TypeUInt16)
    {
        // Make sure this constructor is only reached with type TypeUInt16;
        // this extra parameter makes the code a little clearer by making it
        // explicit at call sites which type is being constructed
        ASSERT_UNUSED(type, type == TypeUInt16);

        m_value.asInt = value;
    }

public:
    static ARMThumbImmediate makeEncodedImm(uint32_t value)
    {
        ThumbImmediateValue encoding;
        encoding.asInt = 0;

        // okay, these are easy.
        if (value < 256) {
            encoding.immediate = value;
            encoding.pattern = 0;
            return ARMThumbImmediate(TypeEncoded, encoding);
        }

        int32_t leadingZeros = countLeadingZeros(value);
        // if there were 24 or more leading zeros, then we'd have hit the (value < 256) case.
        ASSERT(leadingZeros < 24);

        // Given a number with bit fields Z:B:C, where count(Z)+count(B)+count(C) == 32,
        // Z are the bits known zero, B is the 8-bit immediate, C are the bits to check for
        // zero.  count(B) == 8, so the count of bits to be checked is 24 - count(Z).
        int32_t rightShiftAmount = 24 - leadingZeros;
        if (value == ((value >> rightShiftAmount) << rightShiftAmount)) {
            // Shift the value down to the low byte position.  The assign to 
            // shiftValue7 drops the implicit top bit.
            encoding.shiftValue7 = value >> rightShiftAmount;
            // The endoded shift amount is the magnitude of a right rotate.
            encoding.shiftAmount = 8 + leadingZeros;
            return ARMThumbImmediate(TypeEncoded, encoding);
        }
        
        PatternBytes bytes;
        bytes.asInt = value;

        if ((bytes.byte0 == bytes.byte1) && (bytes.byte0 == bytes.byte2) && (bytes.byte0 == bytes.byte3)) {
            encoding.immediate = bytes.byte0;
            encoding.pattern = 3;
            return ARMThumbImmediate(TypeEncoded, encoding);
        }

        if ((bytes.byte0 == bytes.byte2) && !(bytes.byte1 | bytes.byte3)) {
            encoding.immediate = bytes.byte0;
            encoding.pattern = 1;
            return ARMThumbImmediate(TypeEncoded, encoding);
        }

        if ((bytes.byte1 == bytes.byte3) && !(bytes.byte0 | bytes.byte2)) {
            encoding.immediate = bytes.byte1;
            encoding.pattern = 2;
            return ARMThumbImmediate(TypeEncoded, encoding);
        }

        return ARMThumbImmediate();
    }

    static ARMThumbImmediate makeUInt12(int32_t value)
    {
        return (!(value & 0xfffff000))
            ? ARMThumbImmediate(TypeUInt16, (uint16_t)value)
            : ARMThumbImmediate();
    }

    static ARMThumbImmediate makeUInt12OrEncodedImm(int32_t value)
    {
        // If this is not a 12-bit unsigned it, try making an encoded immediate.
        return (!(value & 0xfffff000))
            ? ARMThumbImmediate(TypeUInt16, (uint16_t)value)
            : makeEncodedImm(value);
    }

    // The 'make' methods, above, return a !isValid() value if the argument
    // cannot be represented as the requested type.  This methods  is called
    // 'get' since the argument can always be represented.
    static ARMThumbImmediate makeUInt16(uint16_t value)
    {
        return ARMThumbImmediate(TypeUInt16, value);
    }
    
    bool isValid()
    {
        return m_type != TypeInvalid;
    }

    // These methods rely on the format of encoded byte values.
    bool isUInt3() { return !(m_value.asInt & 0xfff8); }
    bool isUInt4() { return !(m_value.asInt & 0xfff0); }
    bool isUInt5() { return !(m_value.asInt & 0xffe0); }
    bool isUInt6() { return !(m_value.asInt & 0xffc0); }
    bool isUInt7() { return !(m_value.asInt & 0xff80); }
    bool isUInt8() { return !(m_value.asInt & 0xff00); }
    bool isUInt9() { return (m_type == TypeUInt16) && !(m_value.asInt & 0xfe00); }
    bool isUInt10() { return (m_type == TypeUInt16) && !(m_value.asInt & 0xfc00); }
    bool isUInt12() { return (m_type == TypeUInt16) && !(m_value.asInt & 0xf000); }
    bool isUInt16() { return m_type == TypeUInt16; }
    uint8_t getUInt3() { ASSERT(isUInt3()); return m_value.asInt; }
    uint8_t getUInt4() { ASSERT(isUInt4()); return m_value.asInt; }
    uint8_t getUInt5() { ASSERT(isUInt5()); return m_value.asInt; }
    uint8_t getUInt6() { ASSERT(isUInt6()); return m_value.asInt; }
    uint8_t getUInt7() { ASSERT(isUInt7()); return m_value.asInt; }
    uint8_t getUInt8() { ASSERT(isUInt8()); return m_value.asInt; }
    uint8_t getUInt9() { ASSERT(isUInt9()); return m_value.asInt; }
    uint8_t getUInt10() { ASSERT(isUInt10()); return m_value.asInt; }
    uint16_t getUInt12() { ASSERT(isUInt12()); return m_value.asInt; }
    uint16_t getUInt16() { ASSERT(isUInt16()); return m_value.asInt; }

    bool isEncodedImm() { return m_type == TypeEncoded; }

private:
    ThumbImmediateType m_type;
    ThumbImmediateValue m_value;
};

typedef enum {
    SRType_LSL,
    SRType_LSR,
    SRType_ASR,
    SRType_ROR,

    SRType_RRX = SRType_ROR
} ARMShiftType;

class ShiftTypeAndAmount {
    friend class ARMv7Assembler;

public:
    ShiftTypeAndAmount()
    {
        m_u.type = (ARMShiftType)0;
        m_u.amount = 0;
    }
    
    ShiftTypeAndAmount(ARMShiftType type, unsigned amount)
    {
        m_u.type = type;
        m_u.amount = amount & 31;
    }
    
    unsigned lo4() { return m_u.lo4; }
    unsigned hi4() { return m_u.hi4; }
    
private:
    union {
        struct {
            unsigned lo4 : 4;
            unsigned hi4 : 4;
        };
        struct {
            unsigned type   : 2;
            unsigned amount : 6;
        };
    } m_u;
};

class ARMv7Assembler {
public:
    ~ARMv7Assembler()
    {
        ASSERT(m_jumpsToLink.isEmpty());
    }

    typedef ARMRegisters::RegisterID RegisterID;
    typedef ARMRegisters::FPSingleRegisterID FPSingleRegisterID;
    typedef ARMRegisters::FPDoubleRegisterID FPDoubleRegisterID;
    typedef ARMRegisters::FPQuadRegisterID FPQuadRegisterID;

    // (HS, LO, HI, LS) -> (AE, B, A, BE)
    // (VS, VC) -> (O, NO)
    typedef enum {
        ConditionEQ,
        ConditionNE,
        ConditionHS, ConditionCS = ConditionHS,
        ConditionLO, ConditionCC = ConditionLO,
        ConditionMI,
        ConditionPL,
        ConditionVS,
        ConditionVC,
        ConditionHI,
        ConditionLS,
        ConditionGE,
        ConditionLT,
        ConditionGT,
        ConditionLE,
        ConditionAL,
        ConditionInvalid
    } Condition;

    enum JumpType { JumpFixed, JumpNoCondition, JumpCondition, JumpNoConditionFixedSize, JumpConditionFixedSize, JumpTypeCount };
    enum JumpLinkType { LinkInvalid, LinkJumpT1, LinkJumpT2, LinkJumpT3,
        LinkJumpT4, LinkConditionalJumpT4, LinkBX, LinkConditionalBX, JumpLinkTypeCount };
    static const int JumpSizes[JumpLinkTypeCount];
    static const int JumpPaddingSizes[JumpTypeCount];
    class LinkRecord {
    public:
        LinkRecord(intptr_t from, intptr_t to, JumpType type, Condition condition)
            : m_from(from)
            , m_to(to)
            , m_type(type)
            , m_linkType(LinkInvalid)
            , m_condition(condition)
        {
        }
        intptr_t from() const { return m_from; }
        void setFrom(intptr_t from) { m_from = from; }
        intptr_t to() const { return m_to; }
        JumpType type() const { return m_type; }
        JumpLinkType linkType() const { return m_linkType; }
        void setLinkType(JumpLinkType linkType) { ASSERT(m_linkType == LinkInvalid); m_linkType = linkType; }
        Condition condition() const { return m_condition; }
    private:
        intptr_t m_from : 31;
        intptr_t m_to : 31;
        JumpType m_type : 3;
        JumpLinkType m_linkType : 4;
        Condition m_condition : 16;
    };

private:

    // ARMv7, Appx-A.6.3
    bool BadReg(RegisterID reg)
    {
        return (reg == ARMRegisters::sp) || (reg == ARMRegisters::pc);
    }

    uint32_t singleRegisterMask(FPSingleRegisterID rdNum, int highBitsShift, int lowBitShift)
    {
        uint32_t rdMask = (rdNum >> 1) << highBitsShift;
        if (rdNum & 1)
            rdMask |= 1 << lowBitShift;
        return rdMask;
    }

    uint32_t doubleRegisterMask(FPDoubleRegisterID rdNum, int highBitShift, int lowBitsShift)
    {
        uint32_t rdMask = (rdNum & 0xf) << lowBitsShift;
        if (rdNum & 16)
            rdMask |= 1 << highBitShift;
        return rdMask;
    }

    typedef enum {
        OP_ADD_reg_T1       = 0x1800,
        OP_SUB_reg_T1       = 0x1A00,
        OP_ADD_imm_T1       = 0x1C00,
        OP_SUB_imm_T1       = 0x1E00,
        OP_MOV_imm_T1       = 0x2000,
        OP_CMP_imm_T1       = 0x2800,
        OP_ADD_imm_T2       = 0x3000,
        OP_SUB_imm_T2       = 0x3800,
        OP_AND_reg_T1       = 0x4000,
        OP_EOR_reg_T1       = 0x4040,
        OP_TST_reg_T1       = 0x4200,
        OP_RSB_imm_T1       = 0x4240,
        OP_CMP_reg_T1       = 0x4280,
        OP_ORR_reg_T1       = 0x4300,
        OP_MVN_reg_T1       = 0x43C0,
        OP_ADD_reg_T2       = 0x4400,
        OP_MOV_reg_T1       = 0x4600,
        OP_BLX              = 0x4700,
        OP_BX               = 0x4700,
        OP_STR_reg_T1       = 0x5000,
        OP_LDR_reg_T1       = 0x5800,
        OP_LDRH_reg_T1      = 0x5A00,
        OP_LDRB_reg_T1      = 0x5C00,
        OP_STR_imm_T1       = 0x6000,
        OP_LDR_imm_T1       = 0x6800,
        OP_LDRB_imm_T1      = 0x7800,
        OP_LDRH_imm_T1      = 0x8800,
        OP_STR_imm_T2       = 0x9000,
        OP_LDR_imm_T2       = 0x9800,
        OP_ADD_SP_imm_T1    = 0xA800,
        OP_ADD_SP_imm_T2    = 0xB000,
        OP_SUB_SP_imm_T1    = 0xB080,
        OP_BKPT             = 0xBE00,
        OP_IT               = 0xBF00,
        OP_NOP_T1           = 0xBF00,
    } OpcodeID;

    typedef enum {
        OP_B_T1         = 0xD000,
        OP_B_T2         = 0xE000,
        OP_AND_reg_T2   = 0xEA00,
        OP_TST_reg_T2   = 0xEA10,
        OP_ORR_reg_T2   = 0xEA40,
        OP_ORR_S_reg_T2 = 0xEA50,
        OP_ASR_imm_T1   = 0xEA4F,
        OP_LSL_imm_T1   = 0xEA4F,
        OP_LSR_imm_T1   = 0xEA4F,
        OP_ROR_imm_T1   = 0xEA4F,
        OP_MVN_reg_T2   = 0xEA6F,
        OP_EOR_reg_T2   = 0xEA80,
        OP_ADD_reg_T3   = 0xEB00,
        OP_ADD_S_reg_T3 = 0xEB10,
        OP_SUB_reg_T2   = 0xEBA0,
        OP_SUB_S_reg_T2 = 0xEBB0,
        OP_CMP_reg_T2   = 0xEBB0,
        OP_VSTR         = 0xED00,
        OP_VLDR         = 0xED10,
        OP_VMOV_StoC    = 0xEE00,
        OP_VMOV_CtoS    = 0xEE10,
        OP_VMUL_T2      = 0xEE20,
        OP_VADD_T2      = 0xEE30,
        OP_VSUB_T2      = 0xEE30,
        OP_VDIV         = 0xEE80,
        OP_VCMP         = 0xEEB0,
        OP_VCVT_FPIVFP  = 0xEEB0,
        OP_VMOV_IMM_T2  = 0xEEB0,
        OP_VMRS         = 0xEEB0,
        OP_B_T3a        = 0xF000,
        OP_B_T4a        = 0xF000,
        OP_AND_imm_T1   = 0xF000,
        OP_TST_imm      = 0xF010,
        OP_ORR_imm_T1   = 0xF040,
        OP_MOV_imm_T2   = 0xF040,
        OP_MVN_imm      = 0xF060,
        OP_EOR_imm_T1   = 0xF080,
        OP_ADD_imm_T3   = 0xF100,
        OP_ADD_S_imm_T3 = 0xF110,
        OP_CMN_imm      = 0xF110,
        OP_SUB_imm_T3   = 0xF1A0,
        OP_SUB_S_imm_T3 = 0xF1B0,
        OP_CMP_imm_T2   = 0xF1B0,
        OP_RSB_imm_T2   = 0xF1C0,
        OP_ADD_imm_T4   = 0xF200,
        OP_MOV_imm_T3   = 0xF240,
        OP_SUB_imm_T4   = 0xF2A0,
        OP_MOVT         = 0xF2C0,
        OP_NOP_T2a      = 0xF3AF,
        OP_LDRB_imm_T3  = 0xF810,
        OP_LDRB_reg_T2  = 0xF810,
        OP_LDRH_reg_T2  = 0xF830,
        OP_LDRH_imm_T3  = 0xF830,
        OP_STR_imm_T4   = 0xF840,
        OP_STR_reg_T2   = 0xF840,
        OP_LDR_imm_T4   = 0xF850,
        OP_LDR_reg_T2   = 0xF850,
        OP_LDRB_imm_T2  = 0xF890,
        OP_LDRH_imm_T2  = 0xF8B0,
        OP_STR_imm_T3   = 0xF8C0,
        OP_LDR_imm_T3   = 0xF8D0,
        OP_LSL_reg_T2   = 0xFA00,
        OP_LSR_reg_T2   = 0xFA20,
        OP_ASR_reg_T2   = 0xFA40,
        OP_ROR_reg_T2   = 0xFA60,
        OP_CLZ          = 0xFAB0,
        OP_SMULL_T1     = 0xFB80,
    } OpcodeID1;

    typedef enum {
        OP_VADD_T2b     = 0x0A00,
        OP_VDIVb        = 0x0A00,
        OP_VLDRb        = 0x0A00,
        OP_VMOV_IMM_T2b = 0x0A00,
        OP_VMUL_T2b     = 0x0A00,
        OP_VSTRb        = 0x0A00,
        OP_VMOV_CtoSb   = 0x0A10,
        OP_VMOV_StoCb   = 0x0A10,
        OP_VMRSb        = 0x0A10,
        OP_VCMPb        = 0x0A40,
        OP_VCVT_FPIVFPb = 0x0A40,
        OP_VSUB_T2b     = 0x0A40,
        OP_NOP_T2b      = 0x8000,
        OP_B_T3b        = 0x8000,
        OP_B_T4b        = 0x9000,
    } OpcodeID2;

    struct FourFours {
        FourFours(unsigned f3, unsigned f2, unsigned f1, unsigned f0)
        {
            m_u.f0 = f0;
            m_u.f1 = f1;
            m_u.f2 = f2;
            m_u.f3 = f3;
        }

        union {
            unsigned value;
            struct {
                unsigned f0 : 4;
                unsigned f1 : 4;
                unsigned f2 : 4;
                unsigned f3 : 4;
            };
        } m_u;
    };

    class ARMInstructionFormatter;

    // false means else!
    bool ifThenElseConditionBit(Condition condition, bool isIf)
    {
        return isIf ? (condition & 1) : !(condition & 1);
    }
    uint8_t ifThenElse(Condition condition, bool inst2if, bool inst3if, bool inst4if)
    {
        int mask = (ifThenElseConditionBit(condition, inst2if) << 3)
            | (ifThenElseConditionBit(condition, inst3if) << 2)
            | (ifThenElseConditionBit(condition, inst4if) << 1)
            | 1;
        ASSERT((condition != ConditionAL) || !(mask & (mask - 1)));
        return (condition << 4) | mask;
    }
    uint8_t ifThenElse(Condition condition, bool inst2if, bool inst3if)
    {
        int mask = (ifThenElseConditionBit(condition, inst2if) << 3)
            | (ifThenElseConditionBit(condition, inst3if) << 2)
            | 2;
        ASSERT((condition != ConditionAL) || !(mask & (mask - 1)));
        return (condition << 4) | mask;
    }
    uint8_t ifThenElse(Condition condition, bool inst2if)
    {
        int mask = (ifThenElseConditionBit(condition, inst2if) << 3)
            | 4;
        ASSERT((condition != ConditionAL) || !(mask & (mask - 1)));
        return (condition << 4) | mask;
    }

    uint8_t ifThenElse(Condition condition)
    {
        int mask = 8;
        return (condition << 4) | mask;
    }

public:
    
    void add(RegisterID rd, RegisterID rn, ARMThumbImmediate imm)
    {
        // Rd can only be SP if Rn is also SP.
        ASSERT((rd != ARMRegisters::sp) || (rn == ARMRegisters::sp));
        ASSERT(rd != ARMRegisters::pc);
        ASSERT(rn != ARMRegisters::pc);
        ASSERT(imm.isValid());

        if (rn == ARMRegisters::sp) {
            if (!(rd & 8) && imm.isUInt10()) {
                m_formatter.oneWordOp5Reg3Imm8(OP_ADD_SP_imm_T1, rd, imm.getUInt10() >> 2);
                return;
            } else if ((rd == ARMRegisters::sp) && imm.isUInt9()) {
                m_formatter.oneWordOp9Imm7(OP_ADD_SP_imm_T2, imm.getUInt9() >> 2);
                return;
            }
        } else if (!((rd | rn) & 8)) {
            if (imm.isUInt3()) {
                m_formatter.oneWordOp7Reg3Reg3Reg3(OP_ADD_imm_T1, (RegisterID)imm.getUInt3(), rn, rd);
                return;
            } else if ((rd == rn) && imm.isUInt8()) {
                m_formatter.oneWordOp5Reg3Imm8(OP_ADD_imm_T2, rd, imm.getUInt8());
                return;
            }
        }

        if (imm.isEncodedImm())
            m_formatter.twoWordOp5i6Imm4Reg4EncodedImm(OP_ADD_imm_T3, rn, rd, imm);
        else {
            ASSERT(imm.isUInt12());
            m_formatter.twoWordOp5i6Imm4Reg4EncodedImm(OP_ADD_imm_T4, rn, rd, imm);
        }
    }

    void add(RegisterID rd, RegisterID rn, RegisterID rm, ShiftTypeAndAmount shift)
    {
        ASSERT((rd != ARMRegisters::sp) || (rn == ARMRegisters::sp));
        ASSERT(rd != ARMRegisters::pc);
        ASSERT(rn != ARMRegisters::pc);
        ASSERT(!BadReg(rm));
        m_formatter.twoWordOp12Reg4FourFours(OP_ADD_reg_T3, rn, FourFours(shift.hi4(), rd, shift.lo4(), rm));
    }

    // NOTE: In an IT block, add doesn't modify the flags register.
    void add(RegisterID rd, RegisterID rn, RegisterID rm)
    {
        if (rd == rn)
            m_formatter.oneWordOp8RegReg143(OP_ADD_reg_T2, rm, rd);
        else if (rd == rm)
            m_formatter.oneWordOp8RegReg143(OP_ADD_reg_T2, rn, rd);
        else if (!((rd | rn | rm) & 8))
            m_formatter.oneWordOp7Reg3Reg3Reg3(OP_ADD_reg_T1, rm, rn, rd);
        else
            add(rd, rn, rm, ShiftTypeAndAmount());
    }

    // Not allowed in an IT (if then) block.
    void add_S(RegisterID rd, RegisterID rn, ARMThumbImmediate imm)
    {
        // Rd can only be SP if Rn is also SP.
        ASSERT((rd != ARMRegisters::sp) || (rn == ARMRegisters::sp));
        ASSERT(rd != ARMRegisters::pc);
        ASSERT(rn != ARMRegisters::pc);
        ASSERT(imm.isEncodedImm());

        if (!((rd | rn) & 8)) {
            if (imm.isUInt3()) {
                m_formatter.oneWordOp7Reg3Reg3Reg3(OP_ADD_imm_T1, (RegisterID)imm.getUInt3(), rn, rd);
                return;
            } else if ((rd == rn) && imm.isUInt8()) {
                m_formatter.oneWordOp5Reg3Imm8(OP_ADD_imm_T2, rd, imm.getUInt8());
                return;
            }
        }

        m_formatter.twoWordOp5i6Imm4Reg4EncodedImm(OP_ADD_S_imm_T3, rn, rd, imm);
    }

    // Not allowed in an IT (if then) block?
    void add_S(RegisterID rd, RegisterID rn, RegisterID rm, ShiftTypeAndAmount shift)
    {
        ASSERT((rd != ARMRegisters::sp) || (rn == ARMRegisters::sp));
        ASSERT(rd != ARMRegisters::pc);
        ASSERT(rn != ARMRegisters::pc);
        ASSERT(!BadReg(rm));
        m_formatter.twoWordOp12Reg4FourFours(OP_ADD_S_reg_T3, rn, FourFours(shift.hi4(), rd, shift.lo4(), rm));
    }

    // Not allowed in an IT (if then) block.
    void add_S(RegisterID rd, RegisterID rn, RegisterID rm)
    {
        if (!((rd | rn | rm) & 8))
            m_formatter.oneWordOp7Reg3Reg3Reg3(OP_ADD_reg_T1, rm, rn, rd);
        else
            add_S(rd, rn, rm, ShiftTypeAndAmount());
    }

    void ARM_and(RegisterID rd, RegisterID rn, ARMThumbImmediate imm)
    {
        ASSERT(!BadReg(rd));
        ASSERT(!BadReg(rn));
        ASSERT(imm.isEncodedImm());
        m_formatter.twoWordOp5i6Imm4Reg4EncodedImm(OP_AND_imm_T1, rn, rd, imm);
    }

    void ARM_and(RegisterID rd, RegisterID rn, RegisterID rm, ShiftTypeAndAmount shift)
    {
        ASSERT(!BadReg(rd));
        ASSERT(!BadReg(rn));
        ASSERT(!BadReg(rm));
        m_formatter.twoWordOp12Reg4FourFours(OP_AND_reg_T2, rn, FourFours(shift.hi4(), rd, shift.lo4(), rm));
    }

    void ARM_and(RegisterID rd, RegisterID rn, RegisterID rm)
    {
        if ((rd == rn) && !((rd | rm) & 8))
            m_formatter.oneWordOp10Reg3Reg3(OP_AND_reg_T1, rm, rd);
        else if ((rd == rm) && !((rd | rn) & 8))
            m_formatter.oneWordOp10Reg3Reg3(OP_AND_reg_T1, rn, rd);
        else
            ARM_and(rd, rn, rm, ShiftTypeAndAmount());
    }

    void asr(RegisterID rd, RegisterID rm, int32_t shiftAmount)
    {
        ASSERT(!BadReg(rd));
        ASSERT(!BadReg(rm));
        ShiftTypeAndAmount shift(SRType_ASR, shiftAmount);
        m_formatter.twoWordOp16FourFours(OP_ASR_imm_T1, FourFours(shift.hi4(), rd, shift.lo4(), rm));
    }

    void asr(RegisterID rd, RegisterID rn, RegisterID rm)
    {
        ASSERT(!BadReg(rd));
        ASSERT(!BadReg(rn));
        ASSERT(!BadReg(rm));
        m_formatter.twoWordOp12Reg4FourFours(OP_ASR_reg_T2, rn, FourFours(0xf, rd, 0, rm));
    }
    
    // Only allowed in IT (if then) block if last instruction.
    AssemblerLabel b()
    {
        m_formatter.twoWordOp16Op16(OP_B_T4a, OP_B_T4b);
        return m_formatter.label();
    }
    
    // Only allowed in IT (if then) block if last instruction.
    AssemblerLabel blx(RegisterID rm)
    {
        ASSERT(rm != ARMRegisters::pc);
        m_formatter.oneWordOp8RegReg143(OP_BLX, rm, (RegisterID)8);
        return m_formatter.label();
    }

    // Only allowed in IT (if then) block if last instruction.
    AssemblerLabel bx(RegisterID rm)
    {
        m_formatter.oneWordOp8RegReg143(OP_BX, rm, (RegisterID)0);
        return m_formatter.label();
    }

    void bkpt(uint8_t imm=0)
    {
        m_formatter.oneWordOp8Imm8(OP_BKPT, imm);
    }

    void clz(RegisterID rd, RegisterID rm)
    {
        ASSERT(!BadReg(rd));
        ASSERT(!BadReg(rm));
        m_formatter.twoWordOp12Reg4FourFours(OP_CLZ, rm, FourFours(0xf, rd, 8, rm));
    }

    void cmn(RegisterID rn, ARMThumbImmediate imm)
    {
        ASSERT(rn != ARMRegisters::pc);
        ASSERT(imm.isEncodedImm());

        m_formatter.twoWordOp5i6Imm4Reg4EncodedImm(OP_CMN_imm, rn, (RegisterID)0xf, imm);
    }

    void cmp(RegisterID rn, ARMThumbImmediate imm)
    {
        ASSERT(rn != ARMRegisters::pc);
        ASSERT(imm.isEncodedImm());

        if (!(rn & 8) && imm.isUInt8())
            m_formatter.oneWordOp5Reg3Imm8(OP_CMP_imm_T1, rn, imm.getUInt8());
        else
            m_formatter.twoWordOp5i6Imm4Reg4EncodedImm(OP_CMP_imm_T2, rn, (RegisterID)0xf, imm);
    }

    void cmp(RegisterID rn, RegisterID rm, ShiftTypeAndAmount shift)
    {
        ASSERT(rn != ARMRegisters::pc);
        ASSERT(!BadReg(rm));
        m_formatter.twoWordOp12Reg4FourFours(OP_CMP_reg_T2, rn, FourFours(shift.hi4(), 0xf, shift.lo4(), rm));
    }

    void cmp(RegisterID rn, RegisterID rm)
    {
        if ((rn | rm) & 8)
            cmp(rn, rm, ShiftTypeAndAmount());
        else
            m_formatter.oneWordOp10Reg3Reg3(OP_CMP_reg_T1, rm, rn);
    }

    // xor is not spelled with an 'e'. :-(
    void eor(RegisterID rd, RegisterID rn, ARMThumbImmediate imm)
    {
        ASSERT(!BadReg(rd));
        ASSERT(!BadReg(rn));
        ASSERT(imm.isEncodedImm());
        m_formatter.twoWordOp5i6Imm4Reg4EncodedImm(OP_EOR_imm_T1, rn, rd, imm);
    }

    // xor is not spelled with an 'e'. :-(
    void eor(RegisterID rd, RegisterID rn, RegisterID rm, ShiftTypeAndAmount shift)
    {
        ASSERT(!BadReg(rd));
        ASSERT(!BadReg(rn));
        ASSERT(!BadReg(rm));
        m_formatter.twoWordOp12Reg4FourFours(OP_EOR_reg_T2, rn, FourFours(shift.hi4(), rd, shift.lo4(), rm));
    }

    // xor is not spelled with an 'e'. :-(
    void eor(RegisterID rd, RegisterID rn, RegisterID rm)
    {
        if ((rd == rn) && !((rd | rm) & 8))
            m_formatter.oneWordOp10Reg3Reg3(OP_EOR_reg_T1, rm, rd);
        else if ((rd == rm) && !((rd | rn) & 8))
            m_formatter.oneWordOp10Reg3Reg3(OP_EOR_reg_T1, rn, rd);
        else
            eor(rd, rn, rm, ShiftTypeAndAmount());
    }

    void it(Condition cond)
    {
        m_formatter.oneWordOp8Imm8(OP_IT, ifThenElse(cond));
    }

    void it(Condition cond, bool inst2if)
    {
        m_formatter.oneWordOp8Imm8(OP_IT, ifThenElse(cond, inst2if));
    }

    void it(Condition cond, bool inst2if, bool inst3if)
    {
        m_formatter.oneWordOp8Imm8(OP_IT, ifThenElse(cond, inst2if, inst3if));
    }

    void it(Condition cond, bool inst2if, bool inst3if, bool inst4if)
    {
        m_formatter.oneWordOp8Imm8(OP_IT, ifThenElse(cond, inst2if, inst3if, inst4if));
    }

    // rt == ARMRegisters::pc only allowed if last instruction in IT (if then) block.
    void ldr(RegisterID rt, RegisterID rn, ARMThumbImmediate imm)
    {
        ASSERT(rn != ARMRegisters::pc); // LDR (literal)
        ASSERT(imm.isUInt12());

        if (!((rt | rn) & 8) && imm.isUInt7())
            m_formatter.oneWordOp5Imm5Reg3Reg3(OP_LDR_imm_T1, imm.getUInt7() >> 2, rn, rt);
        else if ((rn == ARMRegisters::sp) && !(rt & 8) && imm.isUInt10())
            m_formatter.oneWordOp5Reg3Imm8(OP_LDR_imm_T2, rt, imm.getUInt10() >> 2);
        else
            m_formatter.twoWordOp12Reg4Reg4Imm12(OP_LDR_imm_T3, rn, rt, imm.getUInt12());
    }

    // If index is set, this is a regular offset or a pre-indexed load;
    // if index is not set then is is a post-index load.
    //
    // If wback is set rn is updated - this is a pre or post index load,
    // if wback is not set this is a regular offset memory access.
    //
    // (-255 <= offset <= 255)
    // _reg = REG[rn]
    // _tmp = _reg + offset
    // MEM[index ? _tmp : _reg] = REG[rt]
    // if (wback) REG[rn] = _tmp
    void ldr(RegisterID rt, RegisterID rn, int offset, bool index, bool wback)
    {
        ASSERT(rt != ARMRegisters::pc);
        ASSERT(rn != ARMRegisters::pc);
        ASSERT(index || wback);
        ASSERT(!wback | (rt != rn));
    
        bool add = true;
        if (offset < 0) {
            add = false;
            offset = -offset;
        }
        ASSERT((offset & ~0xff) == 0);
        
        offset |= (wback << 8);
        offset |= (add   << 9);
        offset |= (index << 10);
        offset |= (1 << 11);
        
        m_formatter.twoWordOp12Reg4Reg4Imm12(OP_LDR_imm_T4, rn, rt, offset);
    }

    // rt == ARMRegisters::pc only allowed if last instruction in IT (if then) block.
    void ldr(RegisterID rt, RegisterID rn, RegisterID rm, unsigned shift=0)
    {
        ASSERT(rn != ARMRegisters::pc); // LDR (literal)
        ASSERT(!BadReg(rm));
        ASSERT(shift <= 3);

        if (!shift && !((rt | rn | rm) & 8))
            m_formatter.oneWordOp7Reg3Reg3Reg3(OP_LDR_reg_T1, rm, rn, rt);
        else
            m_formatter.twoWordOp12Reg4FourFours(OP_LDR_reg_T2, rn, FourFours(rt, 0, shift, rm));
    }

    // rt == ARMRegisters::pc only allowed if last instruction in IT (if then) block.
    void ldrh(RegisterID rt, RegisterID rn, ARMThumbImmediate imm)
    {
        ASSERT(rn != ARMRegisters::pc); // LDR (literal)
        ASSERT(imm.isUInt12());

        if (!((rt | rn) & 8) && imm.isUInt6())
            m_formatter.oneWordOp5Imm5Reg3Reg3(OP_LDRH_imm_T1, imm.getUInt6() >> 2, rn, rt);
        else
            m_formatter.twoWordOp12Reg4Reg4Imm12(OP_LDRH_imm_T2, rn, rt, imm.getUInt12());
    }

    // If index is set, this is a regular offset or a pre-indexed load;
    // if index is not set then is is a post-index load.
    //
    // If wback is set rn is updated - this is a pre or post index load,
    // if wback is not set this is a regular offset memory access.
    //
    // (-255 <= offset <= 255)
    // _reg = REG[rn]
    // _tmp = _reg + offset
    // MEM[index ? _tmp : _reg] = REG[rt]
    // if (wback) REG[rn] = _tmp
    void ldrh(RegisterID rt, RegisterID rn, int offset, bool index, bool wback)
    {
        ASSERT(rt != ARMRegisters::pc);
        ASSERT(rn != ARMRegisters::pc);
        ASSERT(index || wback);
        ASSERT(!wback | (rt != rn));
    
        bool add = true;
        if (offset < 0) {
            add = false;
            offset = -offset;
        }
        ASSERT((offset & ~0xff) == 0);
        
        offset |= (wback << 8);
        offset |= (add   << 9);
        offset |= (index << 10);
        offset |= (1 << 11);
        
        m_formatter.twoWordOp12Reg4Reg4Imm12(OP_LDRH_imm_T3, rn, rt, offset);
    }

    void ldrh(RegisterID rt, RegisterID rn, RegisterID rm, unsigned shift=0)
    {
        ASSERT(!BadReg(rt));   // Memory hint
        ASSERT(rn != ARMRegisters::pc); // LDRH (literal)
        ASSERT(!BadReg(rm));
        ASSERT(shift <= 3);

        if (!shift && !((rt | rn | rm) & 8))
            m_formatter.oneWordOp7Reg3Reg3Reg3(OP_LDRH_reg_T1, rm, rn, rt);
        else
            m_formatter.twoWordOp12Reg4FourFours(OP_LDRH_reg_T2, rn, FourFours(rt, 0, shift, rm));
    }

    void ldrb(RegisterID rt, RegisterID rn, ARMThumbImmediate imm)
    {
        ASSERT(rn != ARMRegisters::pc); // LDR (literal)
        ASSERT(imm.isUInt12());

        if (!((rt | rn) & 8) && imm.isUInt5())
            m_formatter.oneWordOp5Imm5Reg3Reg3(OP_LDRB_imm_T1, imm.getUInt5(), rn, rt);
        else
            m_formatter.twoWordOp12Reg4Reg4Imm12(OP_LDRB_imm_T2, rn, rt, imm.getUInt12());
    }

    void ldrb(RegisterID rt, RegisterID rn, int offset, bool index, bool wback)
    {
        ASSERT(rt != ARMRegisters::pc);
        ASSERT(rn != ARMRegisters::pc);
        ASSERT(index || wback);
        ASSERT(!wback | (rt != rn));

        bool add = true;
        if (offset < 0) {
            add = false;
            offset = -offset;
        }

        ASSERT(!(offset & ~0xff));

        offset |= (wback << 8);
        offset |= (add   << 9);
        offset |= (index << 10);
        offset |= (1 << 11);

        m_formatter.twoWordOp12Reg4Reg4Imm12(OP_LDRB_imm_T3, rn, rt, offset);
    }

    void ldrb(RegisterID rt, RegisterID rn, RegisterID rm, unsigned shift = 0)
    {
        ASSERT(rn != ARMRegisters::pc); // LDR (literal)
        ASSERT(!BadReg(rm));
        ASSERT(shift <= 3);

        if (!shift && !((rt | rn | rm) & 8))
            m_formatter.oneWordOp7Reg3Reg3Reg3(OP_LDRB_reg_T1, rm, rn, rt);
        else
            m_formatter.twoWordOp12Reg4FourFours(OP_LDRB_reg_T2, rn, FourFours(rt, 0, shift, rm));
    }

    void lsl(RegisterID rd, RegisterID rm, int32_t shiftAmount)
    {
        ASSERT(!BadReg(rd));
        ASSERT(!BadReg(rm));
        ShiftTypeAndAmount shift(SRType_LSL, shiftAmount);
        m_formatter.twoWordOp16FourFours(OP_LSL_imm_T1, FourFours(shift.hi4(), rd, shift.lo4(), rm));
    }

    void lsl(RegisterID rd, RegisterID rn, RegisterID rm)
    {
        ASSERT(!BadReg(rd));
        ASSERT(!BadReg(rn));
        ASSERT(!BadReg(rm));
        m_formatter.twoWordOp12Reg4FourFours(OP_LSL_reg_T2, rn, FourFours(0xf, rd, 0, rm));
    }

    void lsr(RegisterID rd, RegisterID rm, int32_t shiftAmount)
    {
        ASSERT(!BadReg(rd));
        ASSERT(!BadReg(rm));
        ShiftTypeAndAmount shift(SRType_LSR, shiftAmount);
        m_formatter.twoWordOp16FourFours(OP_LSR_imm_T1, FourFours(shift.hi4(), rd, shift.lo4(), rm));
    }

    void lsr(RegisterID rd, RegisterID rn, RegisterID rm)
    {
        ASSERT(!BadReg(rd));
        ASSERT(!BadReg(rn));
        ASSERT(!BadReg(rm));
        m_formatter.twoWordOp12Reg4FourFours(OP_LSR_reg_T2, rn, FourFours(0xf, rd, 0, rm));
    }

    void movT3(RegisterID rd, ARMThumbImmediate imm)
    {
        ASSERT(imm.isValid());
        ASSERT(!imm.isEncodedImm());
        ASSERT(!BadReg(rd));
        
        m_formatter.twoWordOp5i6Imm4Reg4EncodedImm(OP_MOV_imm_T3, imm.m_value.imm4, rd, imm);
    }

     void mov(RegisterID rd, ARMThumbImmediate imm)
    {
        ASSERT(imm.isValid());
        ASSERT(!BadReg(rd));
        
        if ((rd < 8) && imm.isUInt8())
            m_formatter.oneWordOp5Reg3Imm8(OP_MOV_imm_T1, rd, imm.getUInt8());
        else if (imm.isEncodedImm())
            m_formatter.twoWordOp5i6Imm4Reg4EncodedImm(OP_MOV_imm_T2, 0xf, rd, imm);
        else
            movT3(rd, imm);
    }

   void mov(RegisterID rd, RegisterID rm)
    {
        m_formatter.oneWordOp8RegReg143(OP_MOV_reg_T1, rm, rd);
    }

    void movt(RegisterID rd, ARMThumbImmediate imm)
    {
        ASSERT(imm.isUInt16());
        ASSERT(!BadReg(rd));
        m_formatter.twoWordOp5i6Imm4Reg4EncodedImm(OP_MOVT, imm.m_value.imm4, rd, imm);
    }

    void mvn(RegisterID rd, ARMThumbImmediate imm)
    {
        ASSERT(imm.isEncodedImm());
        ASSERT(!BadReg(rd));
        
        m_formatter.twoWordOp5i6Imm4Reg4EncodedImm(OP_MVN_imm, 0xf, rd, imm);
    }

    void mvn(RegisterID rd, RegisterID rm, ShiftTypeAndAmount shift)
    {
        ASSERT(!BadReg(rd));
        ASSERT(!BadReg(rm));
        m_formatter.twoWordOp16FourFours(OP_MVN_reg_T2, FourFours(shift.hi4(), rd, shift.lo4(), rm));
    }

    void mvn(RegisterID rd, RegisterID rm)
    {
        if (!((rd | rm) & 8))
            m_formatter.oneWordOp10Reg3Reg3(OP_MVN_reg_T1, rm, rd);
        else
            mvn(rd, rm, ShiftTypeAndAmount());
    }

    void neg(RegisterID rd, RegisterID rm)
    {
        ARMThumbImmediate zero = ARMThumbImmediate::makeUInt12(0);
        sub(rd, zero, rm);
    }

    void orr(RegisterID rd, RegisterID rn, ARMThumbImmediate imm)
    {
        ASSERT(!BadReg(rd));
        ASSERT(!BadReg(rn));
        ASSERT(imm.isEncodedImm());
        m_formatter.twoWordOp5i6Imm4Reg4EncodedImm(OP_ORR_imm_T1, rn, rd, imm);
    }

    void orr(RegisterID rd, RegisterID rn, RegisterID rm, ShiftTypeAndAmount shift)
    {
        ASSERT(!BadReg(rd));
        ASSERT(!BadReg(rn));
        ASSERT(!BadReg(rm));
        m_formatter.twoWordOp12Reg4FourFours(OP_ORR_reg_T2, rn, FourFours(shift.hi4(), rd, shift.lo4(), rm));
    }

    void orr(RegisterID rd, RegisterID rn, RegisterID rm)
    {
        if ((rd == rn) && !((rd | rm) & 8))
            m_formatter.oneWordOp10Reg3Reg3(OP_ORR_reg_T1, rm, rd);
        else if ((rd == rm) && !((rd | rn) & 8))
            m_formatter.oneWordOp10Reg3Reg3(OP_ORR_reg_T1, rn, rd);
        else
            orr(rd, rn, rm, ShiftTypeAndAmount());
    }

    void orr_S(RegisterID rd, RegisterID rn, RegisterID rm, ShiftTypeAndAmount shift)
    {
        ASSERT(!BadReg(rd));
        ASSERT(!BadReg(rn));
        ASSERT(!BadReg(rm));
        m_formatter.twoWordOp12Reg4FourFours(OP_ORR_S_reg_T2, rn, FourFours(shift.hi4(), rd, shift.lo4(), rm));
    }

    void orr_S(RegisterID rd, RegisterID rn, RegisterID rm)
    {
        if ((rd == rn) && !((rd | rm) & 8))
            m_formatter.oneWordOp10Reg3Reg3(OP_ORR_reg_T1, rm, rd);
        else if ((rd == rm) && !((rd | rn) & 8))
            m_formatter.oneWordOp10Reg3Reg3(OP_ORR_reg_T1, rn, rd);
        else
            orr_S(rd, rn, rm, ShiftTypeAndAmount());
    }

    void ror(RegisterID rd, RegisterID rm, int32_t shiftAmount)
    {
        ASSERT(!BadReg(rd));
        ASSERT(!BadReg(rm));
        ShiftTypeAndAmount shift(SRType_ROR, shiftAmount);
        m_formatter.twoWordOp16FourFours(OP_ROR_imm_T1, FourFours(shift.hi4(), rd, shift.lo4(), rm));
    }

    void ror(RegisterID rd, RegisterID rn, RegisterID rm)
    {
        ASSERT(!BadReg(rd));
        ASSERT(!BadReg(rn));
        ASSERT(!BadReg(rm));
        m_formatter.twoWordOp12Reg4FourFours(OP_ROR_reg_T2, rn, FourFours(0xf, rd, 0, rm));
    }

    void smull(RegisterID rdLo, RegisterID rdHi, RegisterID rn, RegisterID rm)
    {
        ASSERT(!BadReg(rdLo));
        ASSERT(!BadReg(rdHi));
        ASSERT(!BadReg(rn));
        ASSERT(!BadReg(rm));
        ASSERT(rdLo != rdHi);
        m_formatter.twoWordOp12Reg4FourFours(OP_SMULL_T1, rn, FourFours(rdLo, rdHi, 0, rm));
    }

    // rt == ARMRegisters::pc only allowed if last instruction in IT (if then) block.
    void str(RegisterID rt, RegisterID rn, ARMThumbImmediate imm)
    {
        ASSERT(rt != ARMRegisters::pc);
        ASSERT(rn != ARMRegisters::pc);
        ASSERT(imm.isUInt12());

        if (!((rt | rn) & 8) && imm.isUInt7())
            m_formatter.oneWordOp5Imm5Reg3Reg3(OP_STR_imm_T1, imm.getUInt7() >> 2, rn, rt);
        else if ((rn == ARMRegisters::sp) && !(rt & 8) && imm.isUInt10())
            m_formatter.oneWordOp5Reg3Imm8(OP_STR_imm_T2, rt, imm.getUInt10() >> 2);
        else
            m_formatter.twoWordOp12Reg4Reg4Imm12(OP_STR_imm_T3, rn, rt, imm.getUInt12());
    }

    // If index is set, this is a regular offset or a pre-indexed store;
    // if index is not set then is is a post-index store.
    //
    // If wback is set rn is updated - this is a pre or post index store,
    // if wback is not set this is a regular offset memory access.
    //
    // (-255 <= offset <= 255)
    // _reg = REG[rn]
    // _tmp = _reg + offset
    // MEM[index ? _tmp : _reg] = REG[rt]
    // if (wback) REG[rn] = _tmp
    void str(RegisterID rt, RegisterID rn, int offset, bool index, bool wback)
    {
        ASSERT(rt != ARMRegisters::pc);
        ASSERT(rn != ARMRegisters::pc);
        ASSERT(index || wback);
        ASSERT(!wback | (rt != rn));
    
        bool add = true;
        if (offset < 0) {
            add = false;
            offset = -offset;
        }
        ASSERT((offset & ~0xff) == 0);
        
        offset |= (wback << 8);
        offset |= (add   << 9);
        offset |= (index << 10);
        offset |= (1 << 11);
        
        m_formatter.twoWordOp12Reg4Reg4Imm12(OP_STR_imm_T4, rn, rt, offset);
    }

    // rt == ARMRegisters::pc only allowed if last instruction in IT (if then) block.
    void str(RegisterID rt, RegisterID rn, RegisterID rm, unsigned shift=0)
    {
        ASSERT(rn != ARMRegisters::pc);
        ASSERT(!BadReg(rm));
        ASSERT(shift <= 3);

        if (!shift && !((rt | rn | rm) & 8))
            m_formatter.oneWordOp7Reg3Reg3Reg3(OP_STR_reg_T1, rm, rn, rt);
        else
            m_formatter.twoWordOp12Reg4FourFours(OP_STR_reg_T2, rn, FourFours(rt, 0, shift, rm));
    }

    void sub(RegisterID rd, RegisterID rn, ARMThumbImmediate imm)
    {
        // Rd can only be SP if Rn is also SP.
        ASSERT((rd != ARMRegisters::sp) || (rn == ARMRegisters::sp));
        ASSERT(rd != ARMRegisters::pc);
        ASSERT(rn != ARMRegisters::pc);
        ASSERT(imm.isValid());

        if ((rn == ARMRegisters::sp) && (rd == ARMRegisters::sp) && imm.isUInt9()) {
            m_formatter.oneWordOp9Imm7(OP_SUB_SP_imm_T1, imm.getUInt9() >> 2);
            return;
        } else if (!((rd | rn) & 8)) {
            if (imm.isUInt3()) {
                m_formatter.oneWordOp7Reg3Reg3Reg3(OP_SUB_imm_T1, (RegisterID)imm.getUInt3(), rn, rd);
                return;
            } else if ((rd == rn) && imm.isUInt8()) {
                m_formatter.oneWordOp5Reg3Imm8(OP_SUB_imm_T2, rd, imm.getUInt8());
                return;
            }
        }

        if (imm.isEncodedImm())
            m_formatter.twoWordOp5i6Imm4Reg4EncodedImm(OP_SUB_imm_T3, rn, rd, imm);
        else {
            ASSERT(imm.isUInt12());
            m_formatter.twoWordOp5i6Imm4Reg4EncodedImm(OP_SUB_imm_T4, rn, rd, imm);
        }
    }

    void sub(RegisterID rd, ARMThumbImmediate imm, RegisterID rn)
    {
        ASSERT(rd != ARMRegisters::pc);
        ASSERT(rn != ARMRegisters::pc);
        ASSERT(imm.isValid());
        ASSERT(imm.isUInt12());

        if (!((rd | rn) & 8) && !imm.getUInt12())
            m_formatter.oneWordOp10Reg3Reg3(OP_RSB_imm_T1, rn, rd);
        else
            m_formatter.twoWordOp5i6Imm4Reg4EncodedImm(OP_RSB_imm_T2, rn, rd, imm);
    }

    void sub(RegisterID rd, RegisterID rn, RegisterID rm, ShiftTypeAndAmount shift)
    {
        ASSERT((rd != ARMRegisters::sp) || (rn == ARMRegisters::sp));
        ASSERT(rd != ARMRegisters::pc);
        ASSERT(rn != ARMRegisters::pc);
        ASSERT(!BadReg(rm));
        m_formatter.twoWordOp12Reg4FourFours(OP_SUB_reg_T2, rn, FourFours(shift.hi4(), rd, shift.lo4(), rm));
    }

    // NOTE: In an IT block, add doesn't modify the flags register.
    void sub(RegisterID rd, RegisterID rn, RegisterID rm)
    {
        if (!((rd | rn | rm) & 8))
            m_formatter.oneWordOp7Reg3Reg3Reg3(OP_SUB_reg_T1, rm, rn, rd);
        else
            sub(rd, rn, rm, ShiftTypeAndAmount());
    }

    // Not allowed in an IT (if then) block.
    void sub_S(RegisterID rd, RegisterID rn, ARMThumbImmediate imm)
    {
        // Rd can only be SP if Rn is also SP.
        ASSERT((rd != ARMRegisters::sp) || (rn == ARMRegisters::sp));
        ASSERT(rd != ARMRegisters::pc);
        ASSERT(rn != ARMRegisters::pc);
        ASSERT(imm.isValid());

        if ((rn == ARMRegisters::sp) && (rd == ARMRegisters::sp) && imm.isUInt9()) {
            m_formatter.oneWordOp9Imm7(OP_SUB_SP_imm_T1, imm.getUInt9() >> 2);
            return;
        } else if (!((rd | rn) & 8)) {
            if (imm.isUInt3()) {
                m_formatter.oneWordOp7Reg3Reg3Reg3(OP_SUB_imm_T1, (RegisterID)imm.getUInt3(), rn, rd);
                return;
            } else if ((rd == rn) && imm.isUInt8()) {
                m_formatter.oneWordOp5Reg3Imm8(OP_SUB_imm_T2, rd, imm.getUInt8());
                return;
            }
        }

        m_formatter.twoWordOp5i6Imm4Reg4EncodedImm(OP_SUB_S_imm_T3, rn, rd, imm);
    }

    // Not allowed in an IT (if then) block?
    void sub_S(RegisterID rd, RegisterID rn, RegisterID rm, ShiftTypeAndAmount shift)
    {
        ASSERT((rd != ARMRegisters::sp) || (rn == ARMRegisters::sp));
        ASSERT(rd != ARMRegisters::pc);
        ASSERT(rn != ARMRegisters::pc);
        ASSERT(!BadReg(rm));
        m_formatter.twoWordOp12Reg4FourFours(OP_SUB_S_reg_T2, rn, FourFours(shift.hi4(), rd, shift.lo4(), rm));
    }

    // Not allowed in an IT (if then) block.
    void sub_S(RegisterID rd, RegisterID rn, RegisterID rm)
    {
        if (!((rd | rn | rm) & 8))
            m_formatter.oneWordOp7Reg3Reg3Reg3(OP_SUB_reg_T1, rm, rn, rd);
        else
            sub_S(rd, rn, rm, ShiftTypeAndAmount());
    }

    void tst(RegisterID rn, ARMThumbImmediate imm)
    {
        ASSERT(!BadReg(rn));
        ASSERT(imm.isEncodedImm());

        m_formatter.twoWordOp5i6Imm4Reg4EncodedImm(OP_TST_imm, rn, (RegisterID)0xf, imm);
    }

    void tst(RegisterID rn, RegisterID rm, ShiftTypeAndAmount shift)
    {
        ASSERT(!BadReg(rn));
        ASSERT(!BadReg(rm));
        m_formatter.twoWordOp12Reg4FourFours(OP_TST_reg_T2, rn, FourFours(shift.hi4(), 0xf, shift.lo4(), rm));
    }

    void tst(RegisterID rn, RegisterID rm)
    {
        if ((rn | rm) & 8)
            tst(rn, rm, ShiftTypeAndAmount());
        else
            m_formatter.oneWordOp10Reg3Reg3(OP_TST_reg_T1, rm, rn);
    }

    void vadd_F64(FPDoubleRegisterID rd, FPDoubleRegisterID rn, FPDoubleRegisterID rm)
    {
        m_formatter.vfpOp(OP_VADD_T2, OP_VADD_T2b, true, rn, rd, rm);
    }

    void vcmp_F64(FPDoubleRegisterID rd, FPDoubleRegisterID rm)
    {
        m_formatter.vfpOp(OP_VCMP, OP_VCMPb, true, VFPOperand(4), rd, rm);
    }

    void vcmpz_F64(FPDoubleRegisterID rd)
    {
        m_formatter.vfpOp(OP_VCMP, OP_VCMPb, true, VFPOperand(5), rd, VFPOperand(0));
    }

    void vcvt_F64_S32(FPDoubleRegisterID rd, FPSingleRegisterID rm)
    {
        // boolean values are 64bit (toInt, unsigned, roundZero)
        m_formatter.vfpOp(OP_VCVT_FPIVFP, OP_VCVT_FPIVFPb, true, vcvtOp(false, false, false), rd, rm);
    }

    void vcvtr_S32_F64(FPSingleRegisterID rd, FPDoubleRegisterID rm)
    {
        // boolean values are 64bit (toInt, unsigned, roundZero)
        m_formatter.vfpOp(OP_VCVT_FPIVFP, OP_VCVT_FPIVFPb, true, vcvtOp(true, false, true), rd, rm);
    }

    void vdiv_F64(FPDoubleRegisterID rd, FPDoubleRegisterID rn, FPDoubleRegisterID rm)
    {
        m_formatter.vfpOp(OP_VDIV, OP_VDIVb, true, rn, rd, rm);
    }

    void vldr(FPDoubleRegisterID rd, RegisterID rn, int32_t imm)
    {
        m_formatter.vfpMemOp(OP_VLDR, OP_VLDRb, true, rn, rd, imm);
    }

    void vmov(RegisterID rd, FPSingleRegisterID rn)
    {
        ASSERT(!BadReg(rd));
        m_formatter.vfpOp(OP_VMOV_CtoS, OP_VMOV_CtoSb, false, rn, rd, VFPOperand(0));
    }

    void vmov(FPSingleRegisterID rd, RegisterID rn)
    {
        ASSERT(!BadReg(rn));
        m_formatter.vfpOp(OP_VMOV_StoC, OP_VMOV_StoCb, false, rd, rn, VFPOperand(0));
    }

    void vmrs(RegisterID reg = ARMRegisters::pc)
    {
        ASSERT(reg != ARMRegisters::sp);
        m_formatter.vfpOp(OP_VMRS, OP_VMRSb, false, VFPOperand(1), VFPOperand(0x10 | reg), VFPOperand(0));
    }

    void vmul_F64(FPDoubleRegisterID rd, FPDoubleRegisterID rn, FPDoubleRegisterID rm)
    {
        m_formatter.vfpOp(OP_VMUL_T2, OP_VMUL_T2b, true, rn, rd, rm);
    }

    void vstr(FPDoubleRegisterID rd, RegisterID rn, int32_t imm)
    {
        m_formatter.vfpMemOp(OP_VSTR, OP_VSTRb, true, rn, rd, imm);
    }

    void vsub_F64(FPDoubleRegisterID rd, FPDoubleRegisterID rn, FPDoubleRegisterID rm)
    {
        m_formatter.vfpOp(OP_VSUB_T2, OP_VSUB_T2b, true, rn, rd, rm);
    }

    AssemblerLabel label()
    {
        return m_formatter.label();
    }
    
    AssemblerLabel align(int alignment)
    {
        while (!m_formatter.isAligned(alignment))
            bkpt();

        return label();
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

    int executableOffsetFor(int location)
    {
        if (!location)
            return 0;
        return static_cast<int32_t*>(m_formatter.data())[location / sizeof(int32_t) - 1];
    }
    
    int jumpSizeDelta(JumpType jumpType, JumpLinkType jumpLinkType) { return JumpPaddingSizes[jumpType] - JumpSizes[jumpLinkType]; }
    
    // Assembler admin methods:

    static bool linkRecordSourceComparator(const LinkRecord& a, const LinkRecord& b)
    {
        return a.from() < b.from();
    }

    bool canCompact(JumpType jumpType)
    {
        // The following cannot be compacted:
        //   JumpFixed: represents custom jump sequence
        //   JumpNoConditionFixedSize: represents unconditional jump that must remain a fixed size
        //   JumpConditionFixedSize: represents conditional jump that must remain a fixed size
        return (jumpType == JumpNoCondition) || (jumpType == JumpCondition);
    }
    
    JumpLinkType computeJumpType(JumpType jumpType, const uint8_t* from, const uint8_t* to)
    {
        if (jumpType == JumpFixed)
            return LinkInvalid;
        
        // for patchable jump we must leave space for the longest code sequence
        if (jumpType == JumpNoConditionFixedSize)
            return LinkBX;
        if (jumpType == JumpConditionFixedSize)
            return LinkConditionalBX;
        
        const int paddingSize = JumpPaddingSizes[jumpType];
        bool mayTriggerErrata = false;
        
        if (jumpType == JumpCondition) {
            // 2-byte conditional T1
            const uint16_t* jumpT1Location = reinterpret_cast<const uint16_t*>(from - (paddingSize - JumpSizes[LinkJumpT1]));
            if (canBeJumpT1(jumpT1Location, to))
                return LinkJumpT1;
            // 4-byte conditional T3
            const uint16_t* jumpT3Location = reinterpret_cast<const uint16_t*>(from - (paddingSize - JumpSizes[LinkJumpT3]));
            if (canBeJumpT3(jumpT3Location, to, mayTriggerErrata)) {
                if (!mayTriggerErrata)
                    return LinkJumpT3;
            }
            // 4-byte conditional T4 with IT
            const uint16_t* conditionalJumpT4Location = 
            reinterpret_cast<const uint16_t*>(from - (paddingSize - JumpSizes[LinkConditionalJumpT4]));
            if (canBeJumpT4(conditionalJumpT4Location, to, mayTriggerErrata)) {
                if (!mayTriggerErrata)
                    return LinkConditionalJumpT4;
            }
        } else {
            // 2-byte unconditional T2
            const uint16_t* jumpT2Location = reinterpret_cast<const uint16_t*>(from - (paddingSize - JumpSizes[LinkJumpT2]));
            if (canBeJumpT2(jumpT2Location, to))
                return LinkJumpT2;
            // 4-byte unconditional T4
            const uint16_t* jumpT4Location = reinterpret_cast<const uint16_t*>(from - (paddingSize - JumpSizes[LinkJumpT4]));
            if (canBeJumpT4(jumpT4Location, to, mayTriggerErrata)) {
                if (!mayTriggerErrata)
                    return LinkJumpT4;
            }
            // use long jump sequence
            return LinkBX;
        }
        
        ASSERT(jumpType == JumpCondition);
        return LinkConditionalBX;
    }
    
    JumpLinkType computeJumpType(LinkRecord& record, const uint8_t* from, const uint8_t* to)
    {
        JumpLinkType linkType = computeJumpType(record.type(), from, to);
        record.setLinkType(linkType);
        return linkType;
    }
    
    void recordLinkOffsets(int32_t regionStart, int32_t regionEnd, int32_t offset)
    {
        int32_t ptr = regionStart / sizeof(int32_t);
        const int32_t end = regionEnd / sizeof(int32_t);
        int32_t* offsets = static_cast<int32_t*>(m_formatter.data());
        while (ptr < end)
            offsets[ptr++] = offset;
    }
    
    Vector<LinkRecord>& jumpsToLink()
    {
        std::sort(m_jumpsToLink.begin(), m_jumpsToLink.end(), linkRecordSourceComparator);
        return m_jumpsToLink;
    }

    void link(LinkRecord& record, uint8_t* from, uint8_t* to)
    {
        switch (record.linkType()) {
        case LinkJumpT1:
            linkJumpT1(record.condition(), reinterpret_cast<uint16_t*>(from), to);
            break;
        case LinkJumpT2:
            linkJumpT2(reinterpret_cast<uint16_t*>(from), to);
            break;
        case LinkJumpT3:
            linkJumpT3(record.condition(), reinterpret_cast<uint16_t*>(from), to);
            break;
        case LinkJumpT4:
            linkJumpT4(reinterpret_cast<uint16_t*>(from), to);
            break;
        case LinkConditionalJumpT4:
            linkConditionalJumpT4(record.condition(), reinterpret_cast<uint16_t*>(from), to);
            break;
        case LinkConditionalBX:
            linkConditionalBX(record.condition(), reinterpret_cast<uint16_t*>(from), to);
            break;
        case LinkBX:
            linkBX(reinterpret_cast<uint16_t*>(from), to);
            break;
        default:
            ASSERT_NOT_REACHED();
            break;
        }
    }

    void* unlinkedCode() { return m_formatter.data(); }
    size_t codeSize() const { return m_formatter.codeSize(); }

    static unsigned getCallReturnOffset(AssemblerLabel call)
    {
        ASSERT(call.isSet());
        return call.m_offset;
    }

    // Linking & patching:
    //
    // 'link' and 'patch' methods are for use on unprotected code - such as the code
    // within the AssemblerBuffer, and code being patched by the patch buffer.  Once
    // code has been finalized it is (platform support permitting) within a non-
    // writable region of memory; to modify the code in an execute-only execuable
    // pool the 'repatch' and 'relink' methods should be used.

    void linkJump(AssemblerLabel from, AssemblerLabel to, JumpType type, Condition condition)
    {
        ASSERT(to.isSet());
        ASSERT(from.isSet());
        m_jumpsToLink.append(LinkRecord(from.m_offset, to.m_offset, type, condition));
    }

    static void linkJump(void* code, AssemblerLabel from, void* to)
    {
        ASSERT(from.isSet());
        
        uint16_t* location = reinterpret_cast<uint16_t*>(reinterpret_cast<intptr_t>(code) + from.m_offset);
        linkJumpAbsolute(location, to);
    }

    static void linkCall(void* code, AssemblerLabel from, void* to)
    {
        ASSERT(!(reinterpret_cast<intptr_t>(code) & 1));
        ASSERT(from.isSet());
        ASSERT(reinterpret_cast<intptr_t>(to) & 1);

        setPointer(reinterpret_cast<uint16_t*>(reinterpret_cast<intptr_t>(code) + from.m_offset) - 1, to);
    }

    static void linkPointer(void* code, AssemblerLabel where, void* value)
    {
        setPointer(reinterpret_cast<char*>(code) + where.m_offset, value);
    }

    static void relinkJump(void* from, void* to)
    {
        ASSERT(!(reinterpret_cast<intptr_t>(from) & 1));
        ASSERT(!(reinterpret_cast<intptr_t>(to) & 1));

        linkJumpAbsolute(reinterpret_cast<uint16_t*>(from), to);

        ExecutableAllocator::cacheFlush(reinterpret_cast<uint16_t*>(from) - 5, 5 * sizeof(uint16_t));
    }
    
    static void relinkCall(void* from, void* to)
    {
        ASSERT(!(reinterpret_cast<intptr_t>(from) & 1));
        ASSERT(reinterpret_cast<intptr_t>(to) & 1);

        setPointer(reinterpret_cast<uint16_t*>(from) - 1, to);
    }

    static void repatchInt32(void* where, int32_t value)
    {
        ASSERT(!(reinterpret_cast<intptr_t>(where) & 1));
        
        setInt32(where, value);
    }

    static void repatchPointer(void* where, void* value)
    {
        ASSERT(!(reinterpret_cast<intptr_t>(where) & 1));
        
        setPointer(where, value);
    }

private:
    // VFP operations commonly take one or more 5-bit operands, typically representing a
    // floating point register number.  This will commonly be encoded in the instruction
    // in two parts, with one single bit field, and one 4-bit field.  In the case of
    // double precision operands the high bit of the register number will be encoded
    // separately, and for single precision operands the high bit of the register number
    // will be encoded individually.
    // VFPOperand encapsulates a 5-bit VFP operand, with bits 0..3 containing the 4-bit
    // field to be encoded together in the instruction (the low 4-bits of a double
    // register number, or the high 4-bits of a single register number), and bit 4
    // contains the bit value to be encoded individually.
    struct VFPOperand {
        explicit VFPOperand(uint32_t value)
            : m_value(value)
        {
            ASSERT(!(m_value & ~0x1f));
        }

        VFPOperand(FPDoubleRegisterID reg)
            : m_value(reg)
        {
        }

        VFPOperand(RegisterID reg)
            : m_value(reg)
        {
        }

        VFPOperand(FPSingleRegisterID reg)
            : m_value(((reg & 1) << 4) | (reg >> 1)) // rotate the lowest bit of 'reg' to the top.
        {
        }

        uint32_t bits1()
        {
            return m_value >> 4;
        }

        uint32_t bits4()
        {
            return m_value & 0xf;
        }

        uint32_t m_value;
    };

    VFPOperand vcvtOp(bool toInteger, bool isUnsigned, bool isRoundZero)
    {
        // Cannot specify rounding when converting to float.
        ASSERT(toInteger || !isRoundZero);

        uint32_t op = 0x8;
        if (toInteger) {
            // opc2 indicates both toInteger & isUnsigned.
            op |= isUnsigned ? 0x4 : 0x5;
            // 'op' field in instruction is isRoundZero
            if (isRoundZero)
                op |= 0x10;
        } else {
            // 'op' field in instruction is isUnsigned
            if (!isUnsigned)
                op |= 0x10;
        }
        return VFPOperand(op);
    }

    static void setInt32(void* code, uint32_t value)
    {
        uint16_t* location = reinterpret_cast<uint16_t*>(code);
        ASSERT(isMOV_imm_T3(location - 4) && isMOVT(location - 2));

        ARMThumbImmediate lo16 = ARMThumbImmediate::makeUInt16(static_cast<uint16_t>(value));
        ARMThumbImmediate hi16 = ARMThumbImmediate::makeUInt16(static_cast<uint16_t>(value >> 16));
        location[-4] = twoWordOp5i6Imm4Reg4EncodedImmFirst(OP_MOV_imm_T3, lo16);
        location[-3] = twoWordOp5i6Imm4Reg4EncodedImmSecond((location[-3] >> 8) & 0xf, lo16);
        location[-2] = twoWordOp5i6Imm4Reg4EncodedImmFirst(OP_MOVT, hi16);
        location[-1] = twoWordOp5i6Imm4Reg4EncodedImmSecond((location[-1] >> 8) & 0xf, hi16);

        ExecutableAllocator::cacheFlush(location - 4, 4 * sizeof(uint16_t));
    }

    static void setPointer(void* code, void* value)
    {
        setInt32(code, reinterpret_cast<uint32_t>(value));
    }

    static bool isB(void* address)
    {
        uint16_t* instruction = static_cast<uint16_t*>(address);
        return ((instruction[0] & 0xf800) == OP_B_T4a) && ((instruction[1] & 0xd000) == OP_B_T4b);
    }

    static bool isBX(void* address)
    {
        uint16_t* instruction = static_cast<uint16_t*>(address);
        return (instruction[0] & 0xff87) == OP_BX;
    }

    static bool isMOV_imm_T3(void* address)
    {
        uint16_t* instruction = static_cast<uint16_t*>(address);
        return ((instruction[0] & 0xFBF0) == OP_MOV_imm_T3) && ((instruction[1] & 0x8000) == 0);
    }

    static bool isMOVT(void* address)
    {
        uint16_t* instruction = static_cast<uint16_t*>(address);
        return ((instruction[0] & 0xFBF0) == OP_MOVT) && ((instruction[1] & 0x8000) == 0);
    }

    static bool isNOP_T1(void* address)
    {
        uint16_t* instruction = static_cast<uint16_t*>(address);
        return instruction[0] == OP_NOP_T1;
    }

    static bool isNOP_T2(void* address)
    {
        uint16_t* instruction = static_cast<uint16_t*>(address);
        return (instruction[0] == OP_NOP_T2a) && (instruction[1] == OP_NOP_T2b);
    }

    static bool canBeJumpT1(const uint16_t* instruction, const void* target)
    {
        ASSERT(!(reinterpret_cast<intptr_t>(instruction) & 1));
        ASSERT(!(reinterpret_cast<intptr_t>(target) & 1));
        
        intptr_t relative = reinterpret_cast<intptr_t>(target) - (reinterpret_cast<intptr_t>(instruction));
        // It does not appear to be documented in the ARM ARM (big surprise), but
        // for OP_B_T1 the branch displacement encoded in the instruction is 2 
        // less than the actual displacement.
        relative -= 2;
        return ((relative << 23) >> 23) == relative;
    }
    
    static bool canBeJumpT2(const uint16_t* instruction, const void* target)
    {
        ASSERT(!(reinterpret_cast<intptr_t>(instruction) & 1));
        ASSERT(!(reinterpret_cast<intptr_t>(target) & 1));
        
        intptr_t relative = reinterpret_cast<intptr_t>(target) - (reinterpret_cast<intptr_t>(instruction));
        // It does not appear to be documented in the ARM ARM (big surprise), but
        // for OP_B_T2 the branch displacement encoded in the instruction is 2 
        // less than the actual displacement.
        relative -= 2;
        return ((relative << 20) >> 20) == relative;
    }
    
    static bool canBeJumpT3(const uint16_t* instruction, const void* target, bool& mayTriggerErrata)
    {
        ASSERT(!(reinterpret_cast<intptr_t>(instruction) & 1));
        ASSERT(!(reinterpret_cast<intptr_t>(target) & 1));
        
        intptr_t relative = reinterpret_cast<intptr_t>(target) - (reinterpret_cast<intptr_t>(instruction));
        // From Cortex-A8 errata:
        // If the 32-bit Thumb-2 branch instruction spans two 4KiB regions and
        // the target of the branch falls within the first region it is
        // possible for the processor to incorrectly determine the branch
        // instruction, and it is also possible in some cases for the processor
        // to enter a deadlock state.
        // The instruction is spanning two pages if it ends at an address ending 0x002
        bool spansTwo4K = ((reinterpret_cast<intptr_t>(instruction) & 0xfff) == 0x002);
        mayTriggerErrata = spansTwo4K;
        // The target is in the first page if the jump branch back by [3..0x1002] bytes
        bool targetInFirstPage = (relative >= -0x1002) && (relative < -2);
        bool wouldTriggerA8Errata = spansTwo4K && targetInFirstPage;
        return ((relative << 11) >> 11) == relative && !wouldTriggerA8Errata;
    }
    
    static bool canBeJumpT4(const uint16_t* instruction, const void* target, bool& mayTriggerErrata)
    {
        ASSERT(!(reinterpret_cast<intptr_t>(instruction) & 1));
        ASSERT(!(reinterpret_cast<intptr_t>(target) & 1));
        
        intptr_t relative = reinterpret_cast<intptr_t>(target) - (reinterpret_cast<intptr_t>(instruction));
        // From Cortex-A8 errata:
        // If the 32-bit Thumb-2 branch instruction spans two 4KiB regions and
        // the target of the branch falls within the first region it is
        // possible for the processor to incorrectly determine the branch
        // instruction, and it is also possible in some cases for the processor
        // to enter a deadlock state.
        // The instruction is spanning two pages if it ends at an address ending 0x002
        bool spansTwo4K = ((reinterpret_cast<intptr_t>(instruction) & 0xfff) == 0x002);
        mayTriggerErrata = spansTwo4K;
        // The target is in the first page if the jump branch back by [3..0x1002] bytes
        bool targetInFirstPage = (relative >= -0x1002) && (relative < -2);
        bool wouldTriggerA8Errata = spansTwo4K && targetInFirstPage;
        return ((relative << 7) >> 7) == relative && !wouldTriggerA8Errata;
    }
    
    void linkJumpT1(Condition cond, uint16_t* instruction, void* target)
    {
        // FIMXE: this should be up in the MacroAssembler layer. :-(        
        ASSERT(!(reinterpret_cast<intptr_t>(instruction) & 1));
        ASSERT(!(reinterpret_cast<intptr_t>(target) & 1));
        ASSERT(canBeJumpT1(instruction, target));
        
        intptr_t relative = reinterpret_cast<intptr_t>(target) - (reinterpret_cast<intptr_t>(instruction));
        // It does not appear to be documented in the ARM ARM (big surprise), but
        // for OP_B_T1 the branch displacement encoded in the instruction is 2 
        // less than the actual displacement.
        relative -= 2;
        
        // All branch offsets should be an even distance.
        ASSERT(!(relative & 1));
        instruction[-1] = OP_B_T1 | ((cond & 0xf) << 8) | ((relative & 0x1fe) >> 1);
    }
    
    static void linkJumpT2(uint16_t* instruction, void* target)
    {
        // FIMXE: this should be up in the MacroAssembler layer. :-(        
        ASSERT(!(reinterpret_cast<intptr_t>(instruction) & 1));
        ASSERT(!(reinterpret_cast<intptr_t>(target) & 1));
        ASSERT(canBeJumpT2(instruction, target));
        
        intptr_t relative = reinterpret_cast<intptr_t>(target) - (reinterpret_cast<intptr_t>(instruction));
        // It does not appear to be documented in the ARM ARM (big surprise), but
        // for OP_B_T2 the branch displacement encoded in the instruction is 2 
        // less than the actual displacement.
        relative -= 2;
        
        // All branch offsets should be an even distance.
        ASSERT(!(relative & 1));
        instruction[-1] = OP_B_T2 | ((relative & 0xffe) >> 1);
    }
    
    void linkJumpT3(Condition cond, uint16_t* instruction, void* target)
    {
        // FIMXE: this should be up in the MacroAssembler layer. :-(
        ASSERT(!(reinterpret_cast<intptr_t>(instruction) & 1));
        ASSERT(!(reinterpret_cast<intptr_t>(target) & 1));
        bool scratch;
        UNUSED_PARAM(scratch);
        ASSERT(canBeJumpT3(instruction, target, scratch));
        
        intptr_t relative = reinterpret_cast<intptr_t>(target) - (reinterpret_cast<intptr_t>(instruction));
        
        // All branch offsets should be an even distance.
        ASSERT(!(relative & 1));
        instruction[-2] = OP_B_T3a | ((relative & 0x100000) >> 10) | ((cond & 0xf) << 6) | ((relative & 0x3f000) >> 12);
        instruction[-1] = OP_B_T3b | ((relative & 0x80000) >> 8) | ((relative & 0x40000) >> 5) | ((relative & 0xffe) >> 1);
    }
    
    static void linkJumpT4(uint16_t* instruction, void* target)
    {
        // FIMXE: this should be up in the MacroAssembler layer. :-(        
        ASSERT(!(reinterpret_cast<intptr_t>(instruction) & 1));
        ASSERT(!(reinterpret_cast<intptr_t>(target) & 1));
        bool scratch;
        UNUSED_PARAM(scratch);
        ASSERT(canBeJumpT4(instruction, target, scratch));
        
        intptr_t relative = reinterpret_cast<intptr_t>(target) - (reinterpret_cast<intptr_t>(instruction));
        // ARM encoding for the top two bits below the sign bit is 'peculiar'.
        if (relative >= 0)
            relative ^= 0xC00000;
        
        // All branch offsets should be an even distance.
        ASSERT(!(relative & 1));
        instruction[-2] = OP_B_T4a | ((relative & 0x1000000) >> 14) | ((relative & 0x3ff000) >> 12);
        instruction[-1] = OP_B_T4b | ((relative & 0x800000) >> 10) | ((relative & 0x400000) >> 11) | ((relative & 0xffe) >> 1);
    }
    
    void linkConditionalJumpT4(Condition cond, uint16_t* instruction, void* target)
    {
        // FIMXE: this should be up in the MacroAssembler layer. :-(        
        ASSERT(!(reinterpret_cast<intptr_t>(instruction) & 1));
        ASSERT(!(reinterpret_cast<intptr_t>(target) & 1));
        
        instruction[-3] = ifThenElse(cond) | OP_IT;
        linkJumpT4(instruction, target);
    }
    
    static void linkBX(uint16_t* instruction, void* target)
    {
        // FIMXE: this should be up in the MacroAssembler layer. :-(
        ASSERT(!(reinterpret_cast<intptr_t>(instruction) & 1));
        ASSERT(!(reinterpret_cast<intptr_t>(target) & 1));
        
        const uint16_t JUMP_TEMPORARY_REGISTER = ARMRegisters::ip;
        ARMThumbImmediate lo16 = ARMThumbImmediate::makeUInt16(static_cast<uint16_t>(reinterpret_cast<uint32_t>(target) + 1));
        ARMThumbImmediate hi16 = ARMThumbImmediate::makeUInt16(static_cast<uint16_t>(reinterpret_cast<uint32_t>(target) >> 16));
        instruction[-5] = twoWordOp5i6Imm4Reg4EncodedImmFirst(OP_MOV_imm_T3, lo16);
        instruction[-4] = twoWordOp5i6Imm4Reg4EncodedImmSecond(JUMP_TEMPORARY_REGISTER, lo16);
        instruction[-3] = twoWordOp5i6Imm4Reg4EncodedImmFirst(OP_MOVT, hi16);
        instruction[-2] = twoWordOp5i6Imm4Reg4EncodedImmSecond(JUMP_TEMPORARY_REGISTER, hi16);
        instruction[-1] = OP_BX | (JUMP_TEMPORARY_REGISTER << 3);
    }
    
    void linkConditionalBX(Condition cond, uint16_t* instruction, void* target)
    {
        // FIMXE: this should be up in the MacroAssembler layer. :-(        
        ASSERT(!(reinterpret_cast<intptr_t>(instruction) & 1));
        ASSERT(!(reinterpret_cast<intptr_t>(target) & 1));
        
        linkBX(instruction, target);
        instruction[-6] = ifThenElse(cond, true, true) | OP_IT;
    }
    
    static void linkJumpAbsolute(uint16_t* instruction, void* target)
    {
        // FIMXE: this should be up in the MacroAssembler layer. :-(
        ASSERT(!(reinterpret_cast<intptr_t>(instruction) & 1));
        ASSERT(!(reinterpret_cast<intptr_t>(target) & 1));
        
        ASSERT((isMOV_imm_T3(instruction - 5) && isMOVT(instruction - 3) && isBX(instruction - 1))
               || (isNOP_T1(instruction - 5) && isNOP_T2(instruction - 4) && isB(instruction - 2)));
        
        bool scratch;
        if (canBeJumpT4(instruction, target, scratch)) {
            // There may be a better way to fix this, but right now put the NOPs first, since in the
            // case of an conditional branch this will be coming after an ITTT predicating *three*
            // instructions!  Looking backwards to modify the ITTT to an IT is not easy, due to
            // variable wdith encoding - the previous instruction might *look* like an ITTT but
            // actually be the second half of a 2-word op.
            instruction[-5] = OP_NOP_T1;
            instruction[-4] = OP_NOP_T2a;
            instruction[-3] = OP_NOP_T2b;
            linkJumpT4(instruction, target);
        } else {
            const uint16_t JUMP_TEMPORARY_REGISTER = ARMRegisters::ip;
            ARMThumbImmediate lo16 = ARMThumbImmediate::makeUInt16(static_cast<uint16_t>(reinterpret_cast<uint32_t>(target) + 1));
            ARMThumbImmediate hi16 = ARMThumbImmediate::makeUInt16(static_cast<uint16_t>(reinterpret_cast<uint32_t>(target) >> 16));
            instruction[-5] = twoWordOp5i6Imm4Reg4EncodedImmFirst(OP_MOV_imm_T3, lo16);
            instruction[-4] = twoWordOp5i6Imm4Reg4EncodedImmSecond(JUMP_TEMPORARY_REGISTER, lo16);
            instruction[-3] = twoWordOp5i6Imm4Reg4EncodedImmFirst(OP_MOVT, hi16);
            instruction[-2] = twoWordOp5i6Imm4Reg4EncodedImmSecond(JUMP_TEMPORARY_REGISTER, hi16);
            instruction[-1] = OP_BX | (JUMP_TEMPORARY_REGISTER << 3);
        }
    }
    
    static uint16_t twoWordOp5i6Imm4Reg4EncodedImmFirst(uint16_t op, ARMThumbImmediate imm)
    {
        return op | (imm.m_value.i << 10) | imm.m_value.imm4;
    }

    static uint16_t twoWordOp5i6Imm4Reg4EncodedImmSecond(uint16_t rd, ARMThumbImmediate imm)
    {
        return (imm.m_value.imm3 << 12) | (rd << 8) | imm.m_value.imm8;
    }

    class ARMInstructionFormatter {
    public:
        void oneWordOp5Reg3Imm8(OpcodeID op, RegisterID rd, uint8_t imm)
        {
            m_buffer.putShort(op | (rd << 8) | imm);
        }
        
        void oneWordOp5Imm5Reg3Reg3(OpcodeID op, uint8_t imm, RegisterID reg1, RegisterID reg2)
        {
            m_buffer.putShort(op | (imm << 6) | (reg1 << 3) | reg2);
        }

        void oneWordOp7Reg3Reg3Reg3(OpcodeID op, RegisterID reg1, RegisterID reg2, RegisterID reg3)
        {
            m_buffer.putShort(op | (reg1 << 6) | (reg2 << 3) | reg3);
        }

        void oneWordOp8Imm8(OpcodeID op, uint8_t imm)
        {
            m_buffer.putShort(op | imm);
        }

        void oneWordOp8RegReg143(OpcodeID op, RegisterID reg1, RegisterID reg2)
        {
            m_buffer.putShort(op | ((reg2 & 8) << 4) | (reg1 << 3) | (reg2 & 7));
        }
        void oneWordOp9Imm7(OpcodeID op, uint8_t imm)
        {
            m_buffer.putShort(op | imm);
        }

        void oneWordOp10Reg3Reg3(OpcodeID op, RegisterID reg1, RegisterID reg2)
        {
            m_buffer.putShort(op | (reg1 << 3) | reg2);
        }

        void twoWordOp12Reg4FourFours(OpcodeID1 op, RegisterID reg, FourFours ff)
        {
            m_buffer.putShort(op | reg);
            m_buffer.putShort(ff.m_u.value);
        }
        
        void twoWordOp16FourFours(OpcodeID1 op, FourFours ff)
        {
            m_buffer.putShort(op);
            m_buffer.putShort(ff.m_u.value);
        }
        
        void twoWordOp16Op16(OpcodeID1 op1, OpcodeID2 op2)
        {
            m_buffer.putShort(op1);
            m_buffer.putShort(op2);
        }

        void twoWordOp5i6Imm4Reg4EncodedImm(OpcodeID1 op, int imm4, RegisterID rd, ARMThumbImmediate imm)
        {
            ARMThumbImmediate newImm = imm;
            newImm.m_value.imm4 = imm4;

            m_buffer.putShort(ARMv7Assembler::twoWordOp5i6Imm4Reg4EncodedImmFirst(op, newImm));
            m_buffer.putShort(ARMv7Assembler::twoWordOp5i6Imm4Reg4EncodedImmSecond(rd, newImm));
        }

        void twoWordOp12Reg4Reg4Imm12(OpcodeID1 op, RegisterID reg1, RegisterID reg2, uint16_t imm)
        {
            m_buffer.putShort(op | reg1);
            m_buffer.putShort((reg2 << 12) | imm);
        }

        // Formats up instructions of the pattern:
        //    111111111B11aaaa:bbbb222SA2C2cccc
        // Where 1s in the pattern come from op1, 2s in the pattern come from op2, S is the provided size bit.
        // Operands provide 5 bit values of the form Aaaaa, Bbbbb, Ccccc.
        void vfpOp(OpcodeID1 op1, OpcodeID2 op2, bool size, VFPOperand a, VFPOperand b, VFPOperand c)
        {
            ASSERT(!(op1 & 0x004f));
            ASSERT(!(op2 & 0xf1af));
            m_buffer.putShort(op1 | b.bits1() << 6 | a.bits4());
            m_buffer.putShort(op2 | b.bits4() << 12 | size << 8 | a.bits1() << 7 | c.bits1() << 5 | c.bits4());
        }

        // Arm vfp addresses can be offset by a 9-bit ones-comp immediate, left shifted by 2.
        // (i.e. +/-(0..255) 32-bit words)
        void vfpMemOp(OpcodeID1 op1, OpcodeID2 op2, bool size, RegisterID rn, VFPOperand rd, int32_t imm)
        {
            bool up = true;
            if (imm < 0) {
                imm = -imm;
                up = false;
            }
            
            uint32_t offset = imm;
            ASSERT(!(offset & ~0x3fc));
            offset >>= 2;

            m_buffer.putShort(op1 | (up << 7) | rd.bits1() << 6 | rn);
            m_buffer.putShort(op2 | rd.bits4() << 12 | size << 8 | offset);
        }

        // Administrative methods:

        size_t codeSize() const { return m_buffer.codeSize(); }
        AssemblerLabel label() const { return m_buffer.label(); }
        bool isAligned(int alignment) const { return m_buffer.isAligned(alignment); }
        void* data() const { return m_buffer.data(); }

#ifndef NDEBUG
        unsigned debugOffset() { return m_buffer.debugOffset(); }
#endif

    private:
        AssemblerBuffer m_buffer;
    } m_formatter;

    Vector<LinkRecord> m_jumpsToLink;
    Vector<int32_t> m_offsets;
};

} // namespace JSC

#endif // ENABLE(ASSEMBLER) && CPU(ARM_THUMB2)

#endif // ARMAssembler_h
