/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2009 University of Szeged
 * All rights reserved.
 * Copyright (C) 2010 MIPS Technologies, Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY MIPS TECHNOLOGIES, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL MIPS TECHNOLOGIES, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef MIPSAssembler_h
#define MIPSAssembler_h

#if ENABLE(ASSEMBLER) && CPU(MIPS)

#include "AssemblerBuffer.h"
#include <wtf/Assertions.h>
#include <wtf/SegmentedVector.h>

namespace JSC {

typedef uint32_t MIPSWord;

namespace MIPSRegisters {
typedef enum {
    r0 = 0,
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
    r14,
    r15,
    r16,
    r17,
    r18,
    r19,
    r20,
    r21,
    r22,
    r23,
    r24,
    r25,
    r26,
    r27,
    r28,
    r29,
    r30,
    r31,
    zero = r0,
    at = r1,
    v0 = r2,
    v1 = r3,
    a0 = r4,
    a1 = r5,
    a2 = r6,
    a3 = r7,
    t0 = r8,
    t1 = r9,
    t2 = r10,
    t3 = r11,
    t4 = r12,
    t5 = r13,
    t6 = r14,
    t7 = r15,
    s0 = r16,
    s1 = r17,
    s2 = r18,
    s3 = r19,
    s4 = r20,
    s5 = r21,
    s6 = r22,
    s7 = r23,
    t8 = r24,
    t9 = r25,
    k0 = r26,
    k1 = r27,
    gp = r28,
    sp = r29,
    fp = r30,
    ra = r31
} RegisterID;

typedef enum {
    f0,
    f1,
    f2,
    f3,
    f4,
    f5,
    f6,
    f7,
    f8,
    f9,
    f10,
    f11,
    f12,
    f13,
    f14,
    f15,
    f16,
    f17,
    f18,
    f19,
    f20,
    f21,
    f22,
    f23,
    f24,
    f25,
    f26,
    f27,
    f28,
    f29,
    f30,
    f31
} FPRegisterID;

} // namespace MIPSRegisters

class MIPSAssembler {
public:
    typedef MIPSRegisters::RegisterID RegisterID;
    typedef MIPSRegisters::FPRegisterID FPRegisterID;
    typedef SegmentedVector<AssemblerLabel, 64> Jumps;

    MIPSAssembler()
    {
    }

    // MIPS instruction opcode field position
    enum {
        OP_SH_RD = 11,
        OP_SH_RT = 16,
        OP_SH_RS = 21,
        OP_SH_SHAMT = 6,
        OP_SH_CODE = 16,
        OP_SH_FD = 6,
        OP_SH_FS = 11,
        OP_SH_FT = 16
    };

    void emitInst(MIPSWord op)
    {
        void* oldBase = m_buffer.data();

        m_buffer.putInt(op);

        void* newBase = m_buffer.data();
        if (oldBase != newBase)
            relocateJumps(oldBase, newBase);
    }

    void nop()
    {
        emitInst(0x00000000);
    }

    /* Need to insert one load data delay nop for mips1.  */
    void loadDelayNop()
    {
#if WTF_MIPS_ISA(1)
        nop();
#endif
    }

    /* Need to insert one coprocessor access delay nop for mips1.  */
    void copDelayNop()
    {
#if WTF_MIPS_ISA(1)
        nop();
#endif
    }

    void move(RegisterID rd, RegisterID rs)
    {
        /* addu */
        emitInst(0x00000021 | (rd << OP_SH_RD) | (rs << OP_SH_RS));
    }

    /* Set an immediate value to a register.  This may generate 1 or 2
       instructions.  */
    void li(RegisterID dest, int imm)
    {
        if (imm >= -32768 && imm <= 32767)
            addiu(dest, MIPSRegisters::zero, imm);
        else if (imm >= 0 && imm < 65536)
            ori(dest, MIPSRegisters::zero, imm);
        else {
            lui(dest, imm >> 16);
            if (imm & 0xffff)
                ori(dest, dest, imm);
        }
    }

    void lui(RegisterID rt, int imm)
    {
        emitInst(0x3c000000 | (rt << OP_SH_RT) | (imm & 0xffff));
    }

    void addiu(RegisterID rt, RegisterID rs, int imm)
    {
        emitInst(0x24000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS)
                 | (imm & 0xffff));
    }

    void addu(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        emitInst(0x00000021 | (rd << OP_SH_RD) | (rs << OP_SH_RS)
                 | (rt << OP_SH_RT));
    }

    void subu(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        emitInst(0x00000023 | (rd << OP_SH_RD) | (rs << OP_SH_RS)
                 | (rt << OP_SH_RT));
    }

    void mult(RegisterID rs, RegisterID rt)
    {
        emitInst(0x00000018 | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    void div(RegisterID rs, RegisterID rt)
    {
        emitInst(0x0000001a | (rs << OP_SH_RS) | (rt << OP_SH_RT));
    }

    void mfhi(RegisterID rd)
    {
        emitInst(0x00000010 | (rd << OP_SH_RD));
    }

    void mflo(RegisterID rd)
    {
        emitInst(0x00000012 | (rd << OP_SH_RD));
    }

    void mul(RegisterID rd, RegisterID rs, RegisterID rt)
    {
#if WTF_MIPS_ISA_AT_LEAST(32) 
        emitInst(0x70000002 | (rd << OP_SH_RD) | (rs << OP_SH_RS)
                 | (rt << OP_SH_RT));
#else
        mult(rs, rt);
        mflo(rd);
#endif
    }

    void andInsn(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        emitInst(0x00000024 | (rd << OP_SH_RD) | (rs << OP_SH_RS)
                 | (rt << OP_SH_RT));
    }

    void andi(RegisterID rt, RegisterID rs, int imm)
    {
        emitInst(0x30000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS)
                 | (imm & 0xffff));
    }

    void nor(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        emitInst(0x00000027 | (rd << OP_SH_RD) | (rs << OP_SH_RS)
                 | (rt << OP_SH_RT));
    }

    void orInsn(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        emitInst(0x00000025 | (rd << OP_SH_RD) | (rs << OP_SH_RS)
                 | (rt << OP_SH_RT));
    }

    void ori(RegisterID rt, RegisterID rs, int imm)
    {
        emitInst(0x34000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS)
                 | (imm & 0xffff));
    }

    void xorInsn(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        emitInst(0x00000026 | (rd << OP_SH_RD) | (rs << OP_SH_RS)
                 | (rt << OP_SH_RT));
    }

    void xori(RegisterID rt, RegisterID rs, int imm)
    {
        emitInst(0x38000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS)
                 | (imm & 0xffff));
    }

    void slt(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        emitInst(0x0000002a | (rd << OP_SH_RD) | (rs << OP_SH_RS)
                 | (rt << OP_SH_RT));
    }

    void sltu(RegisterID rd, RegisterID rs, RegisterID rt)
    {
        emitInst(0x0000002b | (rd << OP_SH_RD) | (rs << OP_SH_RS)
                 | (rt << OP_SH_RT));
    }

    void sltiu(RegisterID rt, RegisterID rs, int imm)
    {
        emitInst(0x2c000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS)
                 | (imm & 0xffff));
    }

    void sll(RegisterID rd, RegisterID rt, int shamt)
    {
        emitInst(0x00000000 | (rd << OP_SH_RD) | (rt << OP_SH_RT)
                 | ((shamt & 0x1f) << OP_SH_SHAMT));
    }

    void sllv(RegisterID rd, RegisterID rt, int rs)
    {
        emitInst(0x00000004 | (rd << OP_SH_RD) | (rt << OP_SH_RT)
                 | (rs << OP_SH_RS));
    }

    void sra(RegisterID rd, RegisterID rt, int shamt)
    {
        emitInst(0x00000003 | (rd << OP_SH_RD) | (rt << OP_SH_RT)
                 | ((shamt & 0x1f) << OP_SH_SHAMT));
    }

    void srav(RegisterID rd, RegisterID rt, RegisterID rs)
    {
        emitInst(0x00000007 | (rd << OP_SH_RD) | (rt << OP_SH_RT)
                 | (rs << OP_SH_RS));
    }

    void srl(RegisterID rd, RegisterID rt, int shamt)
    {
        emitInst(0x00000002 | (rd << OP_SH_RD) | (rt << OP_SH_RT)
                 | ((shamt & 0x1f) << OP_SH_SHAMT));
    }

    void srlv(RegisterID rd, RegisterID rt, RegisterID rs)
    {
        emitInst(0x00000006 | (rd << OP_SH_RD) | (rt << OP_SH_RT)
                 | (rs << OP_SH_RS));
    }

    void lbu(RegisterID rt, RegisterID rs, int offset)
    {
        emitInst(0x90000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS)
                 | (offset & 0xffff));
        loadDelayNop();
    }

    void lw(RegisterID rt, RegisterID rs, int offset)
    {
        emitInst(0x8c000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS)
                 | (offset & 0xffff));
        loadDelayNop();
    }

    void lwl(RegisterID rt, RegisterID rs, int offset)
    {
        emitInst(0x88000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS)
                 | (offset & 0xffff));
        loadDelayNop();
    }

    void lwr(RegisterID rt, RegisterID rs, int offset)
    {
        emitInst(0x98000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS)
                 | (offset & 0xffff));
        loadDelayNop();
    }

    void lhu(RegisterID rt, RegisterID rs, int offset)
    {
        emitInst(0x94000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS)
                 | (offset & 0xffff));
        loadDelayNop();
    }

    void sw(RegisterID rt, RegisterID rs, int offset)
    {
        emitInst(0xac000000 | (rt << OP_SH_RT) | (rs << OP_SH_RS)
                 | (offset & 0xffff));
    }

    void jr(RegisterID rs)
    {
        emitInst(0x00000008 | (rs << OP_SH_RS));
    }

    void jalr(RegisterID rs)
    {
        emitInst(0x0000f809 | (rs << OP_SH_RS));
    }

    void jal()
    {
        emitInst(0x0c000000);
    }

    void bkpt()
    {
        int value = 512; /* BRK_BUG */
        emitInst(0x0000000d | ((value & 0x3ff) << OP_SH_CODE));
    }

    void bgez(RegisterID rs, int imm)
    {
        emitInst(0x04010000 | (rs << OP_SH_RS) | (imm & 0xffff));
    }

    void bltz(RegisterID rs, int imm)
    {
        emitInst(0x04000000 | (rs << OP_SH_RS) | (imm & 0xffff));
    }

    void beq(RegisterID rs, RegisterID rt, int imm)
    {
        emitInst(0x10000000 | (rs << OP_SH_RS) | (rt << OP_SH_RT) | (imm & 0xffff));
    }

    void bne(RegisterID rs, RegisterID rt, int imm)
    {
        emitInst(0x14000000 | (rs << OP_SH_RS) | (rt << OP_SH_RT) | (imm & 0xffff));
    }

    void bc1t()
    {
        emitInst(0x45010000);
    }

    void bc1f()
    {
        emitInst(0x45000000);
    }

    void appendJump()
    {
        m_jumps.append(m_buffer.label());
    }

    void addd(FPRegisterID fd, FPRegisterID fs, FPRegisterID ft)
    {
        emitInst(0x46200000 | (fd << OP_SH_FD) | (fs << OP_SH_FS)
                 | (ft << OP_SH_FT));
    }

    void subd(FPRegisterID fd, FPRegisterID fs, FPRegisterID ft)
    {
        emitInst(0x46200001 | (fd << OP_SH_FD) | (fs << OP_SH_FS)
                 | (ft << OP_SH_FT));
    }

    void muld(FPRegisterID fd, FPRegisterID fs, FPRegisterID ft)
    {
        emitInst(0x46200002 | (fd << OP_SH_FD) | (fs << OP_SH_FS)
                 | (ft << OP_SH_FT));
    }

    void divd(FPRegisterID fd, FPRegisterID fs, FPRegisterID ft)
    {
        emitInst(0x46200003 | (fd << OP_SH_FD) | (fs << OP_SH_FS)
                 | (ft << OP_SH_FT));
    }

    void lwc1(FPRegisterID ft, RegisterID rs, int offset)
    {
        emitInst(0xc4000000 | (ft << OP_SH_FT) | (rs << OP_SH_RS)
                 | (offset & 0xffff));
        copDelayNop();
    }

    void ldc1(FPRegisterID ft, RegisterID rs, int offset)
    {
        emitInst(0xd4000000 | (ft << OP_SH_FT) | (rs << OP_SH_RS)
                 | (offset & 0xffff));
    }

    void swc1(FPRegisterID ft, RegisterID rs, int offset)
    {
        emitInst(0xe4000000 | (ft << OP_SH_FT) | (rs << OP_SH_RS)
                 | (offset & 0xffff));
    }

    void sdc1(FPRegisterID ft, RegisterID rs, int offset)
    {
        emitInst(0xf4000000 | (ft << OP_SH_FT) | (rs << OP_SH_RS)
                 | (offset & 0xffff));
    }

    void mtc1(RegisterID rt, FPRegisterID fs)
    {
        emitInst(0x44800000 | (fs << OP_SH_FS) | (rt << OP_SH_RT));
        copDelayNop();
    }

    void mthc1(RegisterID rt, FPRegisterID fs)
    {
        emitInst(0x44e00000 | (fs << OP_SH_FS) | (rt << OP_SH_RT));
        copDelayNop();
    }

    void mfc1(RegisterID rt, FPRegisterID fs)
    {
        emitInst(0x44000000 | (fs << OP_SH_FS) | (rt << OP_SH_RT));
        copDelayNop();
    }

    void sqrtd(FPRegisterID fd, FPRegisterID fs)
    {
        emitInst(0x46200004 | (fd << OP_SH_FD) | (fs << OP_SH_FS));
    }

    void truncwd(FPRegisterID fd, FPRegisterID fs)
    {
        emitInst(0x4620000d | (fd << OP_SH_FD) | (fs << OP_SH_FS));
    }

    void cvtdw(FPRegisterID fd, FPRegisterID fs)
    {
        emitInst(0x46800021 | (fd << OP_SH_FD) | (fs << OP_SH_FS));
    }

    void cvtwd(FPRegisterID fd, FPRegisterID fs)
    {
        emitInst(0x46200024 | (fd << OP_SH_FD) | (fs << OP_SH_FS));
    }

    void ceqd(FPRegisterID fs, FPRegisterID ft)
    {
        emitInst(0x46200032 | (fs << OP_SH_FS) | (ft << OP_SH_FT));
        copDelayNop();
    }

    void cngtd(FPRegisterID fs, FPRegisterID ft)
    {
        emitInst(0x4620003f | (fs << OP_SH_FS) | (ft << OP_SH_FT));
        copDelayNop();
    }

    void cnged(FPRegisterID fs, FPRegisterID ft)
    {
        emitInst(0x4620003d | (fs << OP_SH_FS) | (ft << OP_SH_FT));
        copDelayNop();
    }

    void cltd(FPRegisterID fs, FPRegisterID ft)
    {
        emitInst(0x4620003c | (fs << OP_SH_FS) | (ft << OP_SH_FT));
        copDelayNop();
    }

    void cled(FPRegisterID fs, FPRegisterID ft)
    {
        emitInst(0x4620003e | (fs << OP_SH_FS) | (ft << OP_SH_FT));
        copDelayNop();
    }

    void cueqd(FPRegisterID fs, FPRegisterID ft)
    {
        emitInst(0x46200033 | (fs << OP_SH_FS) | (ft << OP_SH_FT));
        copDelayNop();
    }

    void coled(FPRegisterID fs, FPRegisterID ft)
    {
        emitInst(0x46200036 | (fs << OP_SH_FS) | (ft << OP_SH_FT));
        copDelayNop();
    }

    void coltd(FPRegisterID fs, FPRegisterID ft)
    {
        emitInst(0x46200034 | (fs << OP_SH_FS) | (ft << OP_SH_FT));
        copDelayNop();
    }

    void culed(FPRegisterID fs, FPRegisterID ft)
    {
        emitInst(0x46200037 | (fs << OP_SH_FS) | (ft << OP_SH_FT));
        copDelayNop();
    }

    void cultd(FPRegisterID fs, FPRegisterID ft)
    {
        emitInst(0x46200035 | (fs << OP_SH_FS) | (ft << OP_SH_FT));
        copDelayNop();
    }

    // General helpers

    AssemblerLabel label()
    {
        return m_buffer.label();
    }

    AssemblerLabel align(int alignment)
    {
        while (!m_buffer.isAligned(alignment))
            bkpt();

        return label();
    }

    static void* getRelocatedAddress(void* code, AssemblerLabel label)
    {
        return reinterpret_cast<void*>(reinterpret_cast<char*>(code) + label.m_offset);
    }

    static int getDifferenceBetweenLabels(AssemblerLabel a, AssemblerLabel b)
    {
        return b.m_offset - a.m_offset;
    }

    // Assembler admin methods:

    size_t codeSize() const
    {
        return m_buffer.codeSize();
    }

    void* executableCopy(ExecutablePool* allocator)
    {
        void *result = m_buffer.executableCopy(allocator);
        if (!result)
            return 0;

        relocateJumps(m_buffer.data(), result);
        return result;
    }

#ifndef NDEBUG
    unsigned debugOffset() { return m_buffer.debugOffset(); }
#endif

    static unsigned getCallReturnOffset(AssemblerLabel call)
    {
        // The return address is after a call and a delay slot instruction
        return call.m_offset;
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
        ASSERT(to.isSet());
        ASSERT(from.isSet());
        MIPSWord* insn = reinterpret_cast<MIPSWord*>(reinterpret_cast<intptr_t>(m_buffer.data()) + from.m_offset);
        MIPSWord* toPos = reinterpret_cast<MIPSWord*>(reinterpret_cast<intptr_t>(m_buffer.data()) + to.m_offset);

        ASSERT(!(*(insn - 1)) && !(*(insn - 2)) && !(*(insn - 3)) && !(*(insn - 5)));
        insn = insn - 6;
        linkWithOffset(insn, toPos);
    }

    static void linkJump(void* code, AssemblerLabel from, void* to)
    {
        ASSERT(from.isSet());
        MIPSWord* insn = reinterpret_cast<MIPSWord*>(reinterpret_cast<intptr_t>(code) + from.m_offset);

        ASSERT(!(*(insn - 1)) && !(*(insn - 2)) && !(*(insn - 3)) && !(*(insn - 5)));
        insn = insn - 6;
        linkWithOffset(insn, to);
    }

    static void linkCall(void* code, AssemblerLabel from, void* to)
    {
        MIPSWord* insn = reinterpret_cast<MIPSWord*>(reinterpret_cast<intptr_t>(code) + from.m_offset);
        linkCallInternal(insn, to);
    }

    static void linkPointer(void* code, AssemblerLabel from, void* to)
    {
        MIPSWord* insn = reinterpret_cast<MIPSWord*>(reinterpret_cast<intptr_t>(code) + from.m_offset);
        ASSERT((*insn & 0xffe00000) == 0x3c000000); // lui
        *insn = (*insn & 0xffff0000) | ((reinterpret_cast<intptr_t>(to) >> 16) & 0xffff);
        insn++;
        ASSERT((*insn & 0xfc000000) == 0x34000000); // ori
        *insn = (*insn & 0xffff0000) | (reinterpret_cast<intptr_t>(to) & 0xffff);
    }

    static void relinkJump(void* from, void* to)
    {
        MIPSWord* insn = reinterpret_cast<MIPSWord*>(from);

        ASSERT(!(*(insn - 1)) && !(*(insn - 5)));
        insn = insn - 6;
        int flushSize = linkWithOffset(insn, to);

        ExecutableAllocator::cacheFlush(insn, flushSize);
    }

    static void relinkCall(void* from, void* to)
    {
        void* start;
        int size = linkCallInternal(from, to);
        if (size == sizeof(MIPSWord))
            start = reinterpret_cast<void*>(reinterpret_cast<intptr_t>(from) - 2 * sizeof(MIPSWord));
        else
            start = reinterpret_cast<void*>(reinterpret_cast<intptr_t>(from) - 4 * sizeof(MIPSWord));

        ExecutableAllocator::cacheFlush(start, size);
    }

    static void repatchInt32(void* from, int32_t to)
    {
        MIPSWord* insn = reinterpret_cast<MIPSWord*>(from);
        ASSERT((*insn & 0xffe00000) == 0x3c000000); // lui
        *insn = (*insn & 0xffff0000) | ((to >> 16) & 0xffff);
        insn++;
        ASSERT((*insn & 0xfc000000) == 0x34000000); // ori
        *insn = (*insn & 0xffff0000) | (to & 0xffff);
        insn--;
        ExecutableAllocator::cacheFlush(insn, 2 * sizeof(MIPSWord));
    }

    static void repatchPointer(void* from, void* to)
    {
        repatchInt32(from, reinterpret_cast<int32_t>(to));
    }

private:
    /* Update each jump in the buffer of newBase.  */
    void relocateJumps(void* oldBase, void* newBase)
    {
        // Check each jump
        for (Jumps::Iterator iter = m_jumps.begin(); iter != m_jumps.end(); ++iter) {
            int pos = iter->m_offset;
            MIPSWord* insn = reinterpret_cast<MIPSWord*>(reinterpret_cast<intptr_t>(newBase) + pos);
            insn = insn + 2;
            // Need to make sure we have 5 valid instructions after pos
            if ((unsigned int)pos >= m_buffer.codeSize() - 5 * sizeof(MIPSWord))
                continue;

            if ((*insn & 0xfc000000) == 0x08000000) { // j
                int offset = *insn & 0x03ffffff;
                int oldInsnAddress = (int)insn - (int)newBase + (int)oldBase;
                int topFourBits = (oldInsnAddress + 4) >> 28;
                int oldTargetAddress = (topFourBits << 28) | (offset << 2);
                int newTargetAddress = oldTargetAddress - (int)oldBase + (int)newBase;
                int newInsnAddress = (int)insn;
                if (((newInsnAddress + 4) >> 28) == (newTargetAddress >> 28))
                    *insn = 0x08000000 | ((newTargetAddress >> 2) & 0x3ffffff);
                else {
                    /* lui */
                    *insn = 0x3c000000 | (MIPSRegisters::t9 << OP_SH_RT) | ((newTargetAddress >> 16) & 0xffff);
                    /* ori */
                    *(insn + 1) = 0x34000000 | (MIPSRegisters::t9 << OP_SH_RT) | (MIPSRegisters::t9 << OP_SH_RS) | (newTargetAddress & 0xffff);
                    /* jr */
                    *(insn + 2) = 0x00000008 | (MIPSRegisters::t9 << OP_SH_RS);
                }
            } else if ((*insn & 0xffe00000) == 0x3c000000) { // lui
                int high = (*insn & 0xffff) << 16;
                int low = *(insn + 1) & 0xffff;
                int oldTargetAddress = high | low;
                int newTargetAddress = oldTargetAddress - (int)oldBase + (int)newBase;
                /* lui */
                *insn = 0x3c000000 | (MIPSRegisters::t9 << OP_SH_RT) | ((newTargetAddress >> 16) & 0xffff);
                /* ori */
                *(insn + 1) = 0x34000000 | (MIPSRegisters::t9 << OP_SH_RT) | (MIPSRegisters::t9 << OP_SH_RS) | (newTargetAddress & 0xffff);
            }
        }
    }

    static int linkWithOffset(MIPSWord* insn, void* to)
    {
        ASSERT((*insn & 0xfc000000) == 0x10000000 // beq
               || (*insn & 0xfc000000) == 0x14000000 // bne
               || (*insn & 0xffff0000) == 0x45010000 // bc1t
               || (*insn & 0xffff0000) == 0x45000000); // bc1f
        intptr_t diff = (reinterpret_cast<intptr_t>(to)
                         - reinterpret_cast<intptr_t>(insn) - 4) >> 2;

        if (diff < -32768 || diff > 32767 || *(insn + 2) != 0x10000003) {
            /*
                Convert the sequence:
                  beq $2, $3, target
                  nop
                  b 1f
                  nop
                  nop
                  nop
                1:

                to the new sequence if possible:
                  bne $2, $3, 1f
                  nop
                  j    target
                  nop
                  nop
                  nop
                1:

                OR to the new sequence:
                  bne $2, $3, 1f
                  nop
                  lui $25, target >> 16
                  ori $25, $25, target & 0xffff
                  jr $25
                  nop
                1:

                Note: beq/bne/bc1t/bc1f are converted to bne/beq/bc1f/bc1t.
            */

            if (*(insn + 2) == 0x10000003) {
                if ((*insn & 0xfc000000) == 0x10000000) // beq
                    *insn = (*insn & 0x03ff0000) | 0x14000005; // bne
                else if ((*insn & 0xfc000000) == 0x14000000) // bne
                    *insn = (*insn & 0x03ff0000) | 0x10000005; // beq
                else if ((*insn & 0xffff0000) == 0x45010000) // bc1t
                    *insn = 0x45000005; // bc1f
                else if ((*insn & 0xffff0000) == 0x45000000) // bc1f
                    *insn = 0x45010005; // bc1t
                else
                    ASSERT(0);
            }

            insn = insn + 2;
            if ((reinterpret_cast<intptr_t>(insn) + 4) >> 28
                == reinterpret_cast<intptr_t>(to) >> 28) {
                *insn = 0x08000000 | ((reinterpret_cast<intptr_t>(to) >> 2) & 0x3ffffff);
                *(insn + 1) = 0;
                return 4 * sizeof(MIPSWord);
            }

            intptr_t newTargetAddress = reinterpret_cast<intptr_t>(to);
            /* lui */
            *insn = 0x3c000000 | (MIPSRegisters::t9 << OP_SH_RT) | ((newTargetAddress >> 16) & 0xffff);
            /* ori */
            *(insn + 1) = 0x34000000 | (MIPSRegisters::t9 << OP_SH_RT) | (MIPSRegisters::t9 << OP_SH_RS) | (newTargetAddress & 0xffff);
            /* jr */
            *(insn + 2) = 0x00000008 | (MIPSRegisters::t9 << OP_SH_RS);
            return 5 * sizeof(MIPSWord);
        }

        *insn = (*insn & 0xffff0000) | (diff & 0xffff);
        return sizeof(MIPSWord);
    }

    static int linkCallInternal(void* from, void* to)
    {
        MIPSWord* insn = reinterpret_cast<MIPSWord*>(from);
        insn = insn - 4;

        if ((*(insn + 2) & 0xfc000000) == 0x0c000000) { // jal
            if ((reinterpret_cast<intptr_t>(from) - 4) >> 28
                == reinterpret_cast<intptr_t>(to) >> 28) {
                *(insn + 2) = 0x0c000000 | ((reinterpret_cast<intptr_t>(to) >> 2) & 0x3ffffff);
                return sizeof(MIPSWord);
            }

            /* lui $25, (to >> 16) & 0xffff */
            *insn = 0x3c000000 | (MIPSRegisters::t9 << OP_SH_RT) | ((reinterpret_cast<intptr_t>(to) >> 16) & 0xffff);
            /* ori $25, $25, to & 0xffff */
            *(insn + 1) = 0x34000000 | (MIPSRegisters::t9 << OP_SH_RT) | (MIPSRegisters::t9 << OP_SH_RS) | (reinterpret_cast<intptr_t>(to) & 0xffff);
            /* jalr $25 */
            *(insn + 2) = 0x0000f809 | (MIPSRegisters::t9 << OP_SH_RS);
            return 3 * sizeof(MIPSWord);
        }

        ASSERT((*insn & 0xffe00000) == 0x3c000000); // lui
        ASSERT((*(insn + 1) & 0xfc000000) == 0x34000000); // ori

        /* lui */
        *insn = (*insn & 0xffff0000) | ((reinterpret_cast<intptr_t>(to) >> 16) & 0xffff);
        /* ori */
        *(insn + 1) = (*(insn + 1) & 0xffff0000) | (reinterpret_cast<intptr_t>(to) & 0xffff);
        return 2 * sizeof(MIPSWord);
    }

    AssemblerBuffer m_buffer;
    Jumps m_jumps;
};

} // namespace JSC

#endif // ENABLE(ASSEMBLER) && CPU(MIPS)

#endif // MIPSAssembler_h
