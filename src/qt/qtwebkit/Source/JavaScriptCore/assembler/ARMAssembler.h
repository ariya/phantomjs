/*
 * Copyright (C) 2009, 2010 University of Szeged
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY UNIVERSITY OF SZEGED ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL UNIVERSITY OF SZEGED OR
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

#if ENABLE(ASSEMBLER) && CPU(ARM_TRADITIONAL)

#include "AssemblerBufferWithConstantPool.h"
#include "JITCompilationEffort.h"
#include <wtf/Assertions.h>
namespace JSC {

    typedef uint32_t ARMWord;

    namespace ARMRegisters {
        typedef enum {
            r0 = 0,
            r1,
            r2,
            r3, S0 = r3, /* Same as thumb assembler. */
            r4,
            r5,
            r6,
            r7,
            r8,
            r9,
            r10,
            r11,
            r12, S1 = r12,
            r13, sp = r13,
            r14, lr = r14,
            r15, pc = r15
        } RegisterID;

        typedef enum {
            d0,
            d1,
            d2,
            d3,
            d4,
            d5,
            d6,
            d7, SD0 = d7, /* Same as thumb assembler. */
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
            d31
        } FPRegisterID;

    } // namespace ARMRegisters

    class ARMAssembler {
    public:
        typedef ARMRegisters::RegisterID RegisterID;
        typedef ARMRegisters::FPRegisterID FPRegisterID;
        typedef AssemblerBufferWithConstantPool<2048, 4, 4, ARMAssembler> ARMBuffer;
        typedef SegmentedVector<AssemblerLabel, 64> Jumps;

        ARMAssembler()
            : m_indexOfTailOfLastWatchpoint(1)
        {
        }

        // ARM conditional constants
        typedef enum {
            EQ = 0x00000000, // Zero / Equal.
            NE = 0x10000000, // Non-zero / Not equal.
            CS = 0x20000000, // Unsigned higher or same.
            CC = 0x30000000, // Unsigned lower.
            MI = 0x40000000, // Negative.
            PL = 0x50000000, // Positive or zero.
            VS = 0x60000000, // Overflowed.
            VC = 0x70000000, // Not overflowed.
            HI = 0x80000000, // Unsigned higher.
            LS = 0x90000000, // Unsigned lower or same.
            GE = 0xa0000000, // Signed greater than or equal.
            LT = 0xb0000000, // Signed less than.
            GT = 0xc0000000, // Signed greater than.
            LE = 0xd0000000, // Signed less than or equal.
            AL = 0xe0000000  // Unconditional / Always execute.
        } Condition;

        // ARM instruction constants
        enum {
            AND = (0x0 << 21),
            EOR = (0x1 << 21),
            SUB = (0x2 << 21),
            RSB = (0x3 << 21),
            ADD = (0x4 << 21),
            ADC = (0x5 << 21),
            SBC = (0x6 << 21),
            RSC = (0x7 << 21),
            TST = (0x8 << 21),
            TEQ = (0x9 << 21),
            CMP = (0xa << 21),
            CMN = (0xb << 21),
            ORR = (0xc << 21),
            MOV = (0xd << 21),
            BIC = (0xe << 21),
            MVN = (0xf << 21),
            MUL = 0x00000090,
            MULL = 0x00c00090,
            VMOV_F64 = 0x0eb00b40,
            VADD_F64 = 0x0e300b00,
            VDIV_F64 = 0x0e800b00,
            VSUB_F64 = 0x0e300b40,
            VMUL_F64 = 0x0e200b00,
            VCMP_F64 = 0x0eb40b40,
            VSQRT_F64 = 0x0eb10bc0,
            VABS_F64 = 0x0eb00bc0,
            VNEG_F64 = 0x0eb10b40,
            STMDB = 0x09200000,
            LDMIA = 0x08b00000,
            B = 0x0a000000,
            BL = 0x0b000000,
            BX = 0x012fff10,
            VMOV_VFP64 = 0x0c400a10,
            VMOV_ARM64 = 0x0c500a10,
            VMOV_VFP32 = 0x0e000a10,
            VMOV_ARM32 = 0x0e100a10,
            VCVT_F64_S32 = 0x0eb80bc0,
            VCVT_S32_F64 = 0x0ebd0bc0,
            VCVT_U32_F64 = 0x0ebc0bc0,
            VCVT_F32_F64 = 0x0eb70bc0,
            VCVT_F64_F32 = 0x0eb70ac0,
            VMRS_APSR = 0x0ef1fa10,
            CLZ = 0x016f0f10,
            BKPT = 0xe1200070,
            BLX = 0x012fff30,
#if WTF_ARM_ARCH_AT_LEAST(7)
            MOVW = 0x03000000,
            MOVT = 0x03400000,
#endif
            NOP = 0xe1a00000,
        };

        enum {
            Op2Immediate = (1 << 25),
            ImmediateForHalfWordTransfer = (1 << 22),
            Op2InvertedImmediate = (1 << 26),
            SetConditionalCodes = (1 << 20),
            Op2IsRegisterArgument = (1 << 25),
            // Data transfer flags.
            DataTransferUp = (1 << 23),
            DataTransferWriteBack = (1 << 21),
            DataTransferPostUpdate = (1 << 24),
            DataTransferLoad = (1 << 20),
            ByteDataTransfer = (1 << 22),
        };

        enum DataTransferTypeA {
            LoadUint32 = 0x05000000 | DataTransferLoad,
            LoadUint8 = 0x05400000 | DataTransferLoad,
            StoreUint32 = 0x05000000,
            StoreUint8 = 0x05400000,
        };

        enum DataTransferTypeB {
            LoadUint16 = 0x010000b0 | DataTransferLoad,
            LoadInt16 = 0x010000f0 | DataTransferLoad,
            LoadInt8 = 0x010000d0 | DataTransferLoad,
            StoreUint16 = 0x010000b0,
        };

        enum DataTransferTypeFloat {
            LoadFloat = 0x0d000a00 | DataTransferLoad,
            LoadDouble = 0x0d000b00 | DataTransferLoad,
            StoreFloat = 0x0d000a00,
            StoreDouble = 0x0d000b00,
        };

        // Masks of ARM instructions
        enum {
            BranchOffsetMask = 0x00ffffff,
            ConditionalFieldMask = 0xf0000000,
            DataTransferOffsetMask = 0xfff,
        };

        enum {
            MinimumBranchOffsetDistance = -0x00800000,
            MaximumBranchOffsetDistance = 0x007fffff,
        };

        enum {
            padForAlign8  = 0x00,
            padForAlign16 = 0x0000,
            padForAlign32 = 0xe12fff7f // 'bkpt 0xffff' instruction.
        };

        static const ARMWord InvalidImmediate = 0xf0000000;
        static const ARMWord InvalidBranchTarget = 0xffffffff;
        static const int DefaultPrefetchOffset = 2;

        static const ARMWord BlxInstructionMask = 0x012fff30;
        static const ARMWord LdrOrAddInstructionMask = 0x0ff00000;
        static const ARMWord LdrPcImmediateInstructionMask = 0x0f7f0000;

        static const ARMWord AddImmediateInstruction = 0x02800000;
        static const ARMWord BlxInstruction = 0x012fff30;
        static const ARMWord LdrImmediateInstruction = 0x05900000;
        static const ARMWord LdrPcImmediateInstruction = 0x051f0000;

        // Instruction formating

        void emitInstruction(ARMWord op, int rd, int rn, ARMWord op2)
        {
            ASSERT(((op2 & ~Op2Immediate) <= 0xfff) || (((op2 & ~ImmediateForHalfWordTransfer) <= 0xfff)));
            m_buffer.putInt(op | RN(rn) | RD(rd) | op2);
        }

        void emitDoublePrecisionInstruction(ARMWord op, int dd, int dn, int dm)
        {
            ASSERT((dd >= 0 && dd <= 31) && (dn >= 0 && dn <= 31) && (dm >= 0 && dm <= 31));
            m_buffer.putInt(op | ((dd & 0xf) << 12) | ((dd & 0x10) << (22 - 4))
                               | ((dn & 0xf) << 16) | ((dn & 0x10) << (7 - 4))
                               | (dm & 0xf) | ((dm & 0x10) << (5 - 4)));
        }

        void emitSinglePrecisionInstruction(ARMWord op, int sd, int sn, int sm)
        {
            ASSERT((sd >= 0 && sd <= 31) && (sn >= 0 && sn <= 31) && (sm >= 0 && sm <= 31));
            m_buffer.putInt(op | ((sd >> 1) << 12) | ((sd & 0x1) << 22)
                               | ((sn >> 1) << 16) | ((sn & 0x1) << 7)
                               | (sm >> 1) | ((sm & 0x1) << 5));
        }

        void bitAnd(int rd, int rn, ARMWord op2, Condition cc = AL)
        {
            emitInstruction(toARMWord(cc) | AND, rd, rn, op2);
        }

        void bitAnds(int rd, int rn, ARMWord op2, Condition cc = AL)
        {
            emitInstruction(toARMWord(cc) | AND | SetConditionalCodes, rd, rn, op2);
        }

        void eor(int rd, int rn, ARMWord op2, Condition cc = AL)
        {
            emitInstruction(toARMWord(cc) | EOR, rd, rn, op2);
        }

        void eors(int rd, int rn, ARMWord op2, Condition cc = AL)
        {
            emitInstruction(toARMWord(cc) | EOR | SetConditionalCodes, rd, rn, op2);
        }

        void sub(int rd, int rn, ARMWord op2, Condition cc = AL)
        {
            emitInstruction(toARMWord(cc) | SUB, rd, rn, op2);
        }

        void subs(int rd, int rn, ARMWord op2, Condition cc = AL)
        {
            emitInstruction(toARMWord(cc) | SUB | SetConditionalCodes, rd, rn, op2);
        }

        void rsb(int rd, int rn, ARMWord op2, Condition cc = AL)
        {
            emitInstruction(toARMWord(cc) | RSB, rd, rn, op2);
        }

        void rsbs(int rd, int rn, ARMWord op2, Condition cc = AL)
        {
            emitInstruction(toARMWord(cc) | RSB | SetConditionalCodes, rd, rn, op2);
        }

        void add(int rd, int rn, ARMWord op2, Condition cc = AL)
        {
            emitInstruction(toARMWord(cc) | ADD, rd, rn, op2);
        }

        void adds(int rd, int rn, ARMWord op2, Condition cc = AL)
        {
            emitInstruction(toARMWord(cc) | ADD | SetConditionalCodes, rd, rn, op2);
        }

        void adc(int rd, int rn, ARMWord op2, Condition cc = AL)
        {
            emitInstruction(toARMWord(cc) | ADC, rd, rn, op2);
        }

        void adcs(int rd, int rn, ARMWord op2, Condition cc = AL)
        {
            emitInstruction(toARMWord(cc) | ADC | SetConditionalCodes, rd, rn, op2);
        }

        void sbc(int rd, int rn, ARMWord op2, Condition cc = AL)
        {
            emitInstruction(toARMWord(cc) | SBC, rd, rn, op2);
        }

        void sbcs(int rd, int rn, ARMWord op2, Condition cc = AL)
        {
            emitInstruction(toARMWord(cc) | SBC | SetConditionalCodes, rd, rn, op2);
        }

        void rsc(int rd, int rn, ARMWord op2, Condition cc = AL)
        {
            emitInstruction(toARMWord(cc) | RSC, rd, rn, op2);
        }

        void rscs(int rd, int rn, ARMWord op2, Condition cc = AL)
        {
            emitInstruction(toARMWord(cc) | RSC | SetConditionalCodes, rd, rn, op2);
        }

        void tst(int rn, ARMWord op2, Condition cc = AL)
        {
            emitInstruction(toARMWord(cc) | TST | SetConditionalCodes, 0, rn, op2);
        }

        void teq(int rn, ARMWord op2, Condition cc = AL)
        {
            emitInstruction(toARMWord(cc) | TEQ | SetConditionalCodes, 0, rn, op2);
        }

        void cmp(int rn, ARMWord op2, Condition cc = AL)
        {
            emitInstruction(toARMWord(cc) | CMP | SetConditionalCodes, 0, rn, op2);
        }

        void cmn(int rn, ARMWord op2, Condition cc = AL)
        {
            emitInstruction(toARMWord(cc) | CMN | SetConditionalCodes, 0, rn, op2);
        }

        void orr(int rd, int rn, ARMWord op2, Condition cc = AL)
        {
            emitInstruction(toARMWord(cc) | ORR, rd, rn, op2);
        }

        void orrs(int rd, int rn, ARMWord op2, Condition cc = AL)
        {
            emitInstruction(toARMWord(cc) | ORR | SetConditionalCodes, rd, rn, op2);
        }

        void mov(int rd, ARMWord op2, Condition cc = AL)
        {
            emitInstruction(toARMWord(cc) | MOV, rd, ARMRegisters::r0, op2);
        }

#if WTF_ARM_ARCH_AT_LEAST(7)
        void movw(int rd, ARMWord op2, Condition cc = AL)
        {
            ASSERT((op2 | 0xf0fff) == 0xf0fff);
            m_buffer.putInt(toARMWord(cc) | MOVW | RD(rd) | op2);
        }

        void movt(int rd, ARMWord op2, Condition cc = AL)
        {
            ASSERT((op2 | 0xf0fff) == 0xf0fff);
            m_buffer.putInt(toARMWord(cc) | MOVT | RD(rd) | op2);
        }
#endif

        void movs(int rd, ARMWord op2, Condition cc = AL)
        {
            emitInstruction(toARMWord(cc) | MOV | SetConditionalCodes, rd, ARMRegisters::r0, op2);
        }

        void bic(int rd, int rn, ARMWord op2, Condition cc = AL)
        {
            emitInstruction(toARMWord(cc) | BIC, rd, rn, op2);
        }

        void bics(int rd, int rn, ARMWord op2, Condition cc = AL)
        {
            emitInstruction(toARMWord(cc) | BIC | SetConditionalCodes, rd, rn, op2);
        }

        void mvn(int rd, ARMWord op2, Condition cc = AL)
        {
            emitInstruction(toARMWord(cc) | MVN, rd, ARMRegisters::r0, op2);
        }

        void mvns(int rd, ARMWord op2, Condition cc = AL)
        {
            emitInstruction(toARMWord(cc) | MVN | SetConditionalCodes, rd, ARMRegisters::r0, op2);
        }

        void mul(int rd, int rn, int rm, Condition cc = AL)
        {
            m_buffer.putInt(toARMWord(cc) | MUL | RN(rd) | RS(rn) | RM(rm));
        }

        void muls(int rd, int rn, int rm, Condition cc = AL)
        {
            m_buffer.putInt(toARMWord(cc) | MUL | SetConditionalCodes | RN(rd) | RS(rn) | RM(rm));
        }

        void mull(int rdhi, int rdlo, int rn, int rm, Condition cc = AL)
        {
            m_buffer.putInt(toARMWord(cc) | MULL | RN(rdhi) | RD(rdlo) | RS(rn) | RM(rm));
        }

        void vmov_f64(int dd, int dm, Condition cc = AL)
        {
            emitDoublePrecisionInstruction(toARMWord(cc) | VMOV_F64, dd, 0, dm);
        }

        void vadd_f64(int dd, int dn, int dm, Condition cc = AL)
        {
            emitDoublePrecisionInstruction(toARMWord(cc) | VADD_F64, dd, dn, dm);
        }

        void vdiv_f64(int dd, int dn, int dm, Condition cc = AL)
        {
            emitDoublePrecisionInstruction(toARMWord(cc) | VDIV_F64, dd, dn, dm);
        }

        void vsub_f64(int dd, int dn, int dm, Condition cc = AL)
        {
            emitDoublePrecisionInstruction(toARMWord(cc) | VSUB_F64, dd, dn, dm);
        }

        void vmul_f64(int dd, int dn, int dm, Condition cc = AL)
        {
            emitDoublePrecisionInstruction(toARMWord(cc) | VMUL_F64, dd, dn, dm);
        }

        void vcmp_f64(int dd, int dm, Condition cc = AL)
        {
            emitDoublePrecisionInstruction(toARMWord(cc) | VCMP_F64, dd, 0, dm);
        }

        void vsqrt_f64(int dd, int dm, Condition cc = AL)
        {
            emitDoublePrecisionInstruction(toARMWord(cc) | VSQRT_F64, dd, 0, dm);
        }

        void vabs_f64(int dd, int dm, Condition cc = AL)
        {
            emitDoublePrecisionInstruction(toARMWord(cc) | VABS_F64, dd, 0, dm);
        }

        void vneg_f64(int dd, int dm, Condition cc = AL)
        {
            emitDoublePrecisionInstruction(toARMWord(cc) | VNEG_F64, dd, 0, dm);
        }

        void ldrImmediate(int rd, ARMWord imm, Condition cc = AL)
        {
            m_buffer.putIntWithConstantInt(toARMWord(cc) | LoadUint32 | DataTransferUp | RN(ARMRegisters::pc) | RD(rd), imm, true);
        }

        void ldrUniqueImmediate(int rd, ARMWord imm, Condition cc = AL)
        {
            m_buffer.putIntWithConstantInt(toARMWord(cc) | LoadUint32 | DataTransferUp | RN(ARMRegisters::pc) | RD(rd), imm);
        }

        void dtrUp(DataTransferTypeA transferType, int rd, int rb, ARMWord op2, Condition cc = AL)
        {
            emitInstruction(toARMWord(cc) | transferType | DataTransferUp, rd, rb, op2);
        }

        void dtrUpRegister(DataTransferTypeA transferType, int rd, int rb, int rm, Condition cc = AL)
        {
            emitInstruction(toARMWord(cc) | transferType | DataTransferUp | Op2IsRegisterArgument, rd, rb, rm);
        }

        void dtrDown(DataTransferTypeA transferType, int rd, int rb, ARMWord op2, Condition cc = AL)
        {
            emitInstruction(toARMWord(cc) | transferType, rd, rb, op2);
        }

        void dtrDownRegister(DataTransferTypeA transferType, int rd, int rb, int rm, Condition cc = AL)
        {
            emitInstruction(toARMWord(cc) | transferType | Op2IsRegisterArgument, rd, rb, rm);
        }

        void halfDtrUp(DataTransferTypeB transferType, int rd, int rb, ARMWord op2, Condition cc = AL)
        {
            emitInstruction(toARMWord(cc) | transferType | DataTransferUp, rd, rb, op2);
        }

        void halfDtrUpRegister(DataTransferTypeB transferType, int rd, int rn, int rm, Condition cc = AL)
        {
            emitInstruction(toARMWord(cc) | transferType | DataTransferUp, rd, rn, rm);
        }

        void halfDtrDown(DataTransferTypeB transferType, int rd, int rb, ARMWord op2, Condition cc = AL)
        {
            emitInstruction(toARMWord(cc) | transferType, rd, rb, op2);
        }

        void halfDtrDownRegister(DataTransferTypeB transferType, int rd, int rn, int rm, Condition cc = AL)
        {
            emitInstruction(toARMWord(cc) | transferType, rd, rn, rm);
        }

        void doubleDtrUp(DataTransferTypeFloat type, int rd, int rb, ARMWord op2, Condition cc = AL)
        {
            ASSERT(op2 <= 0xff && rd <= 15);
            /* Only d0-d15 and s0, s2, s4 ... s30 are supported. */
            m_buffer.putInt(toARMWord(cc) | DataTransferUp | type | (rd << 12) | RN(rb) | op2);
        }

        void doubleDtrDown(DataTransferTypeFloat type, int rd, int rb, ARMWord op2, Condition cc = AL)
        {
            ASSERT(op2 <= 0xff && rd <= 15);
            /* Only d0-d15 and s0, s2, s4 ... s30 are supported. */
            m_buffer.putInt(toARMWord(cc) | type | (rd << 12) | RN(rb) | op2);
        }

        void push(int reg, Condition cc = AL)
        {
            ASSERT(ARMWord(reg) <= 0xf);
            m_buffer.putInt(toARMWord(cc) | StoreUint32 | DataTransferWriteBack | RN(ARMRegisters::sp) | RD(reg) | 0x4);
        }

        void pop(int reg, Condition cc = AL)
        {
            ASSERT(ARMWord(reg) <= 0xf);
            m_buffer.putInt(toARMWord(cc) | (LoadUint32 ^ DataTransferPostUpdate) | DataTransferUp | RN(ARMRegisters::sp) | RD(reg) | 0x4);
        }

        inline void poke(int reg, Condition cc = AL)
        {
            dtrDown(StoreUint32, ARMRegisters::sp, 0, reg, cc);
        }

        inline void peek(int reg, Condition cc = AL)
        {
            dtrUp(LoadUint32, reg, ARMRegisters::sp, 0, cc);
        }

        void vmov_vfp64(int sm, int rt, int rt2, Condition cc = AL)
        {
            ASSERT(rt != rt2);
            m_buffer.putInt(toARMWord(cc) | VMOV_VFP64 | RN(rt2) | RD(rt) | (sm & 0xf) | ((sm & 0x10) << (5 - 4)));
        }

        void vmov_arm64(int rt, int rt2, int sm, Condition cc = AL)
        {
            ASSERT(rt != rt2);
            m_buffer.putInt(toARMWord(cc) | VMOV_ARM64 | RN(rt2) | RD(rt) | (sm & 0xf) | ((sm & 0x10) << (5 - 4)));
        }

        void vmov_vfp32(int sn, int rt, Condition cc = AL)
        {
            ASSERT(rt <= 15);
            emitSinglePrecisionInstruction(toARMWord(cc) | VMOV_VFP32, rt << 1, sn, 0);
        }

        void vmov_arm32(int rt, int sn, Condition cc = AL)
        {
            ASSERT(rt <= 15);
            emitSinglePrecisionInstruction(toARMWord(cc) | VMOV_ARM32, rt << 1, sn, 0);
        }

        void vcvt_f64_s32(int dd, int sm, Condition cc = AL)
        {
            ASSERT(!(sm & 0x1)); // sm must be divisible by 2
            emitDoublePrecisionInstruction(toARMWord(cc) | VCVT_F64_S32, dd, 0, (sm >> 1));
        }

        void vcvt_s32_f64(int sd, int dm, Condition cc = AL)
        {
            ASSERT(!(sd & 0x1)); // sd must be divisible by 2
            emitDoublePrecisionInstruction(toARMWord(cc) | VCVT_S32_F64, (sd >> 1), 0, dm);
        }

        void vcvt_u32_f64(int sd, int dm, Condition cc = AL)
        {
            ASSERT(!(sd & 0x1)); // sd must be divisible by 2
            emitDoublePrecisionInstruction(toARMWord(cc) | VCVT_U32_F64, (sd >> 1), 0, dm);
        }

        void vcvt_f64_f32(int dd, int sm, Condition cc = AL)
        {
            ASSERT(dd <= 15 && sm <= 15);
            emitDoublePrecisionInstruction(toARMWord(cc) | VCVT_F64_F32, dd, 0, sm);
        }

        void vcvt_f32_f64(int dd, int sm, Condition cc = AL)
        {
            ASSERT(dd <= 15 && sm <= 15);
            emitDoublePrecisionInstruction(toARMWord(cc) | VCVT_F32_F64, dd, 0, sm);
        }

        void vmrs_apsr(Condition cc = AL)
        {
            m_buffer.putInt(toARMWord(cc) | VMRS_APSR);
        }

        void clz(int rd, int rm, Condition cc = AL)
        {
            m_buffer.putInt(toARMWord(cc) | CLZ | RD(rd) | RM(rm));
        }

        void bkpt(ARMWord value)
        {
            m_buffer.putInt(BKPT | ((value & 0xff0) << 4) | (value & 0xf));
        }

        void nop()
        {
            m_buffer.putInt(NOP);
        }

        void bx(int rm, Condition cc = AL)
        {
            emitInstruction(toARMWord(cc) | BX, 0, 0, RM(rm));
        }

        AssemblerLabel blx(int rm, Condition cc = AL)
        {
            emitInstruction(toARMWord(cc) | BLX, 0, 0, RM(rm));
            return m_buffer.label();
        }

        static ARMWord lsl(int reg, ARMWord value)
        {
            ASSERT(reg <= ARMRegisters::pc);
            ASSERT(value <= 0x1f);
            return reg | (value << 7) | 0x00;
        }

        static ARMWord lsr(int reg, ARMWord value)
        {
            ASSERT(reg <= ARMRegisters::pc);
            ASSERT(value <= 0x1f);
            return reg | (value << 7) | 0x20;
        }

        static ARMWord asr(int reg, ARMWord value)
        {
            ASSERT(reg <= ARMRegisters::pc);
            ASSERT(value <= 0x1f);
            return reg | (value << 7) | 0x40;
        }

        static ARMWord lslRegister(int reg, int shiftReg)
        {
            ASSERT(reg <= ARMRegisters::pc);
            ASSERT(shiftReg <= ARMRegisters::pc);
            return reg | (shiftReg << 8) | 0x10;
        }

        static ARMWord lsrRegister(int reg, int shiftReg)
        {
            ASSERT(reg <= ARMRegisters::pc);
            ASSERT(shiftReg <= ARMRegisters::pc);
            return reg | (shiftReg << 8) | 0x30;
        }

        static ARMWord asrRegister(int reg, int shiftReg)
        {
            ASSERT(reg <= ARMRegisters::pc);
            ASSERT(shiftReg <= ARMRegisters::pc);
            return reg | (shiftReg << 8) | 0x50;
        }

        // General helpers

        size_t codeSize() const
        {
            return m_buffer.codeSize();
        }

        void ensureSpace(int insnSpace, int constSpace)
        {
            m_buffer.ensureSpace(insnSpace, constSpace);
        }

        int sizeOfConstantPool()
        {
            return m_buffer.sizeOfConstantPool();
        }

        AssemblerLabel labelIgnoringWatchpoints()
        {
            m_buffer.ensureSpaceForAnyInstruction();
            return m_buffer.label();
        }

        AssemblerLabel labelForWatchpoint()
        {
            m_buffer.ensureSpaceForAnyInstruction(maxJumpReplacementSize() / sizeof(ARMWord));
            AssemblerLabel result = m_buffer.label();
            if (result.m_offset != (m_indexOfTailOfLastWatchpoint - maxJumpReplacementSize()))
                result = label();
            m_indexOfTailOfLastWatchpoint = result.m_offset + maxJumpReplacementSize();
            return label();
        }

        AssemblerLabel label()
        {
            AssemblerLabel result = labelIgnoringWatchpoints();
            while (result.m_offset + 1 < m_indexOfTailOfLastWatchpoint) {
                nop();
                // The available number of instructions are ensured by labelForWatchpoint.
                result = m_buffer.label();
            }
            return result;
        }

        AssemblerLabel align(int alignment)
        {
            while (!m_buffer.isAligned(alignment))
                mov(ARMRegisters::r0, ARMRegisters::r0);

            return label();
        }

        AssemblerLabel loadBranchTarget(int rd, Condition cc = AL, int useConstantPool = 0)
        {
            ensureSpace(sizeof(ARMWord), sizeof(ARMWord));
            m_jumps.append(m_buffer.codeSize() | (useConstantPool & 0x1));
            ldrUniqueImmediate(rd, InvalidBranchTarget, cc);
            return m_buffer.label();
        }

        AssemblerLabel jmp(Condition cc = AL, int useConstantPool = 0)
        {
            return loadBranchTarget(ARMRegisters::pc, cc, useConstantPool);
        }

        PassRefPtr<ExecutableMemoryHandle> executableCopy(VM&, void* ownerUID, JITCompilationEffort);

        unsigned debugOffset() { return m_buffer.debugOffset(); }

        // DFG assembly helpers for moving data between fp and registers.
        void vmov(RegisterID rd1, RegisterID rd2, FPRegisterID rn)
        {
            vmov_arm64(rd1, rd2, rn);
        }

        void vmov(FPRegisterID rd, RegisterID rn1, RegisterID rn2)
        {
            vmov_vfp64(rd, rn1, rn2);
        }

        // Patching helpers

        static ARMWord* getLdrImmAddress(ARMWord* insn)
        {
            // Check for call
            if ((*insn & LdrPcImmediateInstructionMask) != LdrPcImmediateInstruction) {
                // Must be BLX
                ASSERT((*insn & BlxInstructionMask) == BlxInstruction);
                insn--;
            }

            // Must be an ldr ..., [pc +/- imm]
            ASSERT((*insn & LdrPcImmediateInstructionMask) == LdrPcImmediateInstruction);

            ARMWord addr = reinterpret_cast<ARMWord>(insn) + DefaultPrefetchOffset * sizeof(ARMWord);
            if (*insn & DataTransferUp)
                return reinterpret_cast<ARMWord*>(addr + (*insn & DataTransferOffsetMask));
            return reinterpret_cast<ARMWord*>(addr - (*insn & DataTransferOffsetMask));
        }

        static ARMWord* getLdrImmAddressOnPool(ARMWord* insn, uint32_t* constPool)
        {
            // Must be an ldr ..., [pc +/- imm]
            ASSERT((*insn & LdrPcImmediateInstructionMask) == LdrPcImmediateInstruction);

            if (*insn & 0x1)
                return reinterpret_cast<ARMWord*>(constPool + ((*insn & DataTransferOffsetMask) >> 1));
            return getLdrImmAddress(insn);
        }

        static void patchPointerInternal(intptr_t from, void* to)
        {
            ARMWord* insn = reinterpret_cast<ARMWord*>(from);
            ARMWord* addr = getLdrImmAddress(insn);
            *addr = reinterpret_cast<ARMWord>(to);
        }

        static ARMWord patchConstantPoolLoad(ARMWord load, ARMWord value)
        {
            value = (value << 1) + 1;
            ASSERT(!(value & ~DataTransferOffsetMask));
            return (load & ~DataTransferOffsetMask) | value;
        }

        static void patchConstantPoolLoad(void* loadAddr, void* constPoolAddr);

        // Read pointers
        static void* readPointer(void* from)
        {
            ARMWord* instruction = reinterpret_cast<ARMWord*>(from);
            ARMWord* address = getLdrImmAddress(instruction);
            return *reinterpret_cast<void**>(address);
        }

        // Patch pointers

        static void linkPointer(void* code, AssemblerLabel from, void* to)
        {
            patchPointerInternal(reinterpret_cast<intptr_t>(code) + from.m_offset, to);
        }

        static void repatchInt32(void* where, int32_t to)
        {
            patchPointerInternal(reinterpret_cast<intptr_t>(where), reinterpret_cast<void*>(to));
        }

        static void repatchCompact(void* where, int32_t value)
        {
            ARMWord* instruction = reinterpret_cast<ARMWord*>(where);
            ASSERT((*instruction & 0x0f700000) == LoadUint32);
            if (value >= 0)
                *instruction = (*instruction & 0xff7ff000) | DataTransferUp | value;
            else
                *instruction = (*instruction & 0xff7ff000) | -value;
            cacheFlush(instruction, sizeof(ARMWord));
        }

        static void repatchPointer(void* from, void* to)
        {
            patchPointerInternal(reinterpret_cast<intptr_t>(from), to);
        }

        // Linkers
        static intptr_t getAbsoluteJumpAddress(void* base, int offset = 0)
        {
            return reinterpret_cast<intptr_t>(base) + offset - sizeof(ARMWord);
        }

        void linkJump(AssemblerLabel from, AssemblerLabel to)
        {
            ARMWord* insn = reinterpret_cast<ARMWord*>(getAbsoluteJumpAddress(m_buffer.data(), from.m_offset));
            ARMWord* addr = getLdrImmAddressOnPool(insn, m_buffer.poolAddress());
            *addr = toARMWord(to.m_offset);
        }

        static void linkJump(void* code, AssemblerLabel from, void* to)
        {
            patchPointerInternal(getAbsoluteJumpAddress(code, from.m_offset), to);
        }

        static void relinkJump(void* from, void* to)
        {
            patchPointerInternal(getAbsoluteJumpAddress(from), to);
        }

        static void linkCall(void* code, AssemblerLabel from, void* to)
        {
            patchPointerInternal(getAbsoluteJumpAddress(code, from.m_offset), to);
        }

        static void relinkCall(void* from, void* to)
        {
            patchPointerInternal(getAbsoluteJumpAddress(from), to);
        }

        static void* readCallTarget(void* from)
        {
            return reinterpret_cast<void*>(readPointer(reinterpret_cast<void*>(getAbsoluteJumpAddress(from))));
        }

        static void replaceWithJump(void* instructionStart, void* to)
        {
            ARMWord* instruction = reinterpret_cast<ARMWord*>(instructionStart);
            intptr_t difference = reinterpret_cast<intptr_t>(to) - (reinterpret_cast<intptr_t>(instruction) + DefaultPrefetchOffset * sizeof(ARMWord));

            if (!(difference & 1)) {
                difference >>= 2;
                if ((difference <= MaximumBranchOffsetDistance && difference >= MinimumBranchOffsetDistance)) {
                     // Direct branch.
                     instruction[0] = B | AL | (difference & BranchOffsetMask);
                     cacheFlush(instruction, sizeof(ARMWord));
                     return;
                }
            }

            // Load target.
            instruction[0] = LoadUint32 | AL | RN(ARMRegisters::pc) | RD(ARMRegisters::pc) | 4;
            instruction[1] = reinterpret_cast<ARMWord>(to);
            cacheFlush(instruction, sizeof(ARMWord) * 2);
        }

        static ptrdiff_t maxJumpReplacementSize()
        {
            return sizeof(ARMWord) * 2;
        }

        static void replaceWithLoad(void* instructionStart)
        {
            ARMWord* instruction = reinterpret_cast<ARMWord*>(instructionStart);
            cacheFlush(instruction, sizeof(ARMWord));

            ASSERT((*instruction & LdrOrAddInstructionMask) == AddImmediateInstruction || (*instruction & LdrOrAddInstructionMask) == LdrImmediateInstruction);
            if ((*instruction & LdrOrAddInstructionMask) == AddImmediateInstruction) {
                 *instruction = (*instruction & ~LdrOrAddInstructionMask) | LdrImmediateInstruction;
                 cacheFlush(instruction, sizeof(ARMWord));
            }
        }

        static void replaceWithAddressComputation(void* instructionStart)
        {
            ARMWord* instruction = reinterpret_cast<ARMWord*>(instructionStart);
            cacheFlush(instruction, sizeof(ARMWord));

            ASSERT((*instruction & LdrOrAddInstructionMask) == AddImmediateInstruction || (*instruction & LdrOrAddInstructionMask) == LdrImmediateInstruction);
            if ((*instruction & LdrOrAddInstructionMask) == LdrImmediateInstruction) {
                 *instruction = (*instruction & ~LdrOrAddInstructionMask) | AddImmediateInstruction;
                 cacheFlush(instruction, sizeof(ARMWord));
            }
        }

        static void revertBranchPtrWithPatch(void* instructionStart, RegisterID rn, ARMWord imm)
        {
            ARMWord* instruction = reinterpret_cast<ARMWord*>(instructionStart);

            ASSERT((instruction[2] & LdrPcImmediateInstructionMask) == LdrPcImmediateInstruction);
            instruction[0] = toARMWord(AL) | ((instruction[2] & 0x0fff0fff) + sizeof(ARMWord)) | RD(ARMRegisters::S1);
            *getLdrImmAddress(instruction) = imm;
            instruction[1] = toARMWord(AL) | CMP | SetConditionalCodes | RN(rn) | RM(ARMRegisters::S1);
            cacheFlush(instruction, 2 * sizeof(ARMWord));
        }

        // Address operations

        static void* getRelocatedAddress(void* code, AssemblerLabel label)
        {
            return reinterpret_cast<void*>(reinterpret_cast<char*>(code) + label.m_offset);
        }

        // Address differences

        static int getDifferenceBetweenLabels(AssemblerLabel a, AssemblerLabel b)
        {
            return b.m_offset - a.m_offset;
        }

        static unsigned getCallReturnOffset(AssemblerLabel call)
        {
            return call.m_offset;
        }

        // Handle immediates

        static ARMWord getOp2(ARMWord imm);

        // Fast case if imm is known to be between 0 and 0xff
        static ARMWord getOp2Byte(ARMWord imm)
        {
            ASSERT(imm <= 0xff);
            return Op2Immediate | imm;
        }

        static ARMWord getOp2Half(ARMWord imm)
        {
            ASSERT(imm <= 0xff);
            return ImmediateForHalfWordTransfer | (imm & 0x0f) | ((imm & 0xf0) << 4);
        }

#if WTF_ARM_ARCH_AT_LEAST(7)
        static ARMWord getImm16Op2(ARMWord imm)
        {
            if (imm <= 0xffff)
                return (imm & 0xf000) << 4 | (imm & 0xfff);
            return InvalidImmediate;
        }
#endif
        ARMWord getImm(ARMWord imm, int tmpReg, bool invert = false);
        void moveImm(ARMWord imm, int dest);
        ARMWord encodeComplexImm(ARMWord imm, int dest);

        // Memory load/store helpers

        void dataTransfer32(DataTransferTypeA, RegisterID srcDst, RegisterID base, int32_t offset);
        void baseIndexTransfer32(DataTransferTypeA, RegisterID srcDst, RegisterID base, RegisterID index, int scale, int32_t offset);
        void dataTransfer16(DataTransferTypeB, RegisterID srcDst, RegisterID base, int32_t offset);
        void baseIndexTransfer16(DataTransferTypeB, RegisterID srcDst, RegisterID base, RegisterID index, int scale, int32_t offset);
        void dataTransferFloat(DataTransferTypeFloat, FPRegisterID srcDst, RegisterID base, int32_t offset);
        void baseIndexTransferFloat(DataTransferTypeFloat, FPRegisterID srcDst, RegisterID base, RegisterID index, int scale, int32_t offset);

        // Constant pool hnadlers

        static ARMWord placeConstantPoolBarrier(int offset)
        {
            offset = (offset - sizeof(ARMWord)) >> 2;
            ASSERT((offset <= MaximumBranchOffsetDistance && offset >= MinimumBranchOffsetDistance));
            return AL | B | (offset & BranchOffsetMask);
        }

#if OS(LINUX) && COMPILER(GCC)
        static inline void linuxPageFlush(uintptr_t begin, uintptr_t end)
        {
            asm volatile(
                "push    {r7}\n"
                "mov     r0, %0\n"
                "mov     r1, %1\n"
                "mov     r7, #0xf0000\n"
                "add     r7, r7, #0x2\n"
                "mov     r2, #0x0\n"
                "svc     0x0\n"
                "pop     {r7}\n"
                :
                : "r" (begin), "r" (end)
                : "r0", "r1", "r2");
        }
#endif

#if OS(LINUX) && COMPILER(RVCT)
        static __asm void cacheFlush(void* code, size_t);
#else
        static void cacheFlush(void* code, size_t size)
        {
#if OS(LINUX) && COMPILER(GCC)
            size_t page = pageSize();
            uintptr_t current = reinterpret_cast<uintptr_t>(code);
            uintptr_t end = current + size;
            uintptr_t firstPageEnd = (current & ~(page - 1)) + page;

            if (end <= firstPageEnd) {
                linuxPageFlush(current, end);
                return;
            }

            linuxPageFlush(current, firstPageEnd);

            for (current = firstPageEnd; current + page < end; current += page)
                linuxPageFlush(current, current + page);

            linuxPageFlush(current, end);
#elif OS(WINCE)
            CacheRangeFlush(code, size, CACHE_SYNC_ALL);
#elif OS(QNX) && ENABLE(ASSEMBLER_WX_EXCLUSIVE)
            UNUSED_PARAM(code);
            UNUSED_PARAM(size);
#elif OS(QNX)
            msync(code, size, MS_INVALIDATE_ICACHE);
#else
#error "The cacheFlush support is missing on this platform."
#endif
        }
#endif

    private:
        static ARMWord RM(int reg)
        {
            ASSERT(reg <= ARMRegisters::pc);
            return reg;
        }

        static ARMWord RS(int reg)
        {
            ASSERT(reg <= ARMRegisters::pc);
            return reg << 8;
        }

        static ARMWord RD(int reg)
        {
            ASSERT(reg <= ARMRegisters::pc);
            return reg << 12;
        }

        static ARMWord RN(int reg)
        {
            ASSERT(reg <= ARMRegisters::pc);
            return reg << 16;
        }

        static ARMWord getConditionalField(ARMWord i)
        {
            return i & ConditionalFieldMask;
        }

        static ARMWord toARMWord(Condition cc)
        {
            return static_cast<ARMWord>(cc);
        }

        static ARMWord toARMWord(uint32_t u)
        {
            return static_cast<ARMWord>(u);
        }

        int genInt(int reg, ARMWord imm, bool positive);

        ARMBuffer m_buffer;
        Jumps m_jumps;
        uint32_t m_indexOfTailOfLastWatchpoint;
    };

} // namespace JSC

#endif // ENABLE(ASSEMBLER) && CPU(ARM_TRADITIONAL)

#endif // ARMAssembler_h
