/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
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

#ifndef MacroAssemblerMIPS_h
#define MacroAssemblerMIPS_h

#if ENABLE(ASSEMBLER) && CPU(MIPS)

#include "MIPSAssembler.h"
#include "AbstractMacroAssembler.h"

namespace JSC {

class MacroAssemblerMIPS : public AbstractMacroAssembler<MIPSAssembler> {
public:
    typedef MIPSRegisters::FPRegisterID FPRegisterID;

    MacroAssemblerMIPS()
        : m_fixedWidth(false)
    {
    }

    static const Scale ScalePtr = TimesFour;

    // For storing immediate number
    static const RegisterID immTempRegister = MIPSRegisters::t0;
    // For storing data loaded from the memory
    static const RegisterID dataTempRegister = MIPSRegisters::t1;
    // For storing address base
    static const RegisterID addrTempRegister = MIPSRegisters::t2;
    // For storing compare result
    static const RegisterID cmpTempRegister = MIPSRegisters::t3;

    // FP temp register
    static const FPRegisterID fpTempRegister = MIPSRegisters::f16;

    enum RelationalCondition {
        Equal,
        NotEqual,
        Above,
        AboveOrEqual,
        Below,
        BelowOrEqual,
        GreaterThan,
        GreaterThanOrEqual,
        LessThan,
        LessThanOrEqual
    };

    enum ResultCondition {
        Overflow,
        Signed,
        Zero,
        NonZero
    };

    enum DoubleCondition {
        DoubleEqual,
        DoubleNotEqual,
        DoubleGreaterThan,
        DoubleGreaterThanOrEqual,
        DoubleLessThan,
        DoubleLessThanOrEqual,
        DoubleEqualOrUnordered,
        DoubleNotEqualOrUnordered,
        DoubleGreaterThanOrUnordered,
        DoubleGreaterThanOrEqualOrUnordered,
        DoubleLessThanOrUnordered,
        DoubleLessThanOrEqualOrUnordered
    };

    static const RegisterID stackPointerRegister = MIPSRegisters::sp;
    static const RegisterID returnAddressRegister = MIPSRegisters::ra;

    // Integer arithmetic operations:
    //
    // Operations are typically two operand - operation(source, srcDst)
    // For many operations the source may be an TrustedImm32, the srcDst operand
    // may often be a memory location (explictly described using an Address
    // object).

    void add32(RegisterID src, RegisterID dest)
    {
        m_assembler.addu(dest, dest, src);
    }

    void add32(TrustedImm32 imm, RegisterID dest)
    {
        add32(imm, dest, dest);
    }

    void add32(TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        if (!imm.m_isPointer && imm.m_value >= -32768 && imm.m_value <= 32767
            && !m_fixedWidth) {
            /*
              addiu     dest, src, imm
            */
            m_assembler.addiu(dest, src, imm.m_value);
        } else {
            /*
              li        immTemp, imm
              addu      dest, src, immTemp
            */
            move(imm, immTempRegister);
            m_assembler.addu(dest, src, immTempRegister);
        }
    }

    void add32(TrustedImm32 imm, Address address)
    {
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth) {
            /*
              lw        dataTemp, offset(base)
              li        immTemp, imm
              addu      dataTemp, dataTemp, immTemp
              sw        dataTemp, offset(base)
            */
            m_assembler.lw(dataTempRegister, address.base, address.offset);
            if (!imm.m_isPointer
                && imm.m_value >= -32768 && imm.m_value <= 32767
                && !m_fixedWidth)
                m_assembler.addiu(dataTempRegister, dataTempRegister,
                                  imm.m_value);
            else {
                move(imm, immTempRegister);
                m_assembler.addu(dataTempRegister, dataTempRegister,
                                 immTempRegister);
            }
            m_assembler.sw(dataTempRegister, address.base, address.offset);
        } else {
            /*
              lui       addrTemp, (offset + 0x8000) >> 16
              addu      addrTemp, addrTemp, base
              lw        dataTemp, (offset & 0xffff)(addrTemp)
              li        immtemp, imm
              addu      dataTemp, dataTemp, immTemp
              sw        dataTemp, (offset & 0xffff)(addrTemp)
            */
            m_assembler.lui(addrTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lw(dataTempRegister, addrTempRegister, address.offset);

            if (imm.m_value >= -32768 && imm.m_value <= 32767 && !m_fixedWidth)
                m_assembler.addiu(dataTempRegister, dataTempRegister,
                                  imm.m_value);
            else {
                move(imm, immTempRegister);
                m_assembler.addu(dataTempRegister, dataTempRegister,
                                 immTempRegister);
            }
            m_assembler.sw(dataTempRegister, addrTempRegister, address.offset);
        }
    }

    void add32(Address src, RegisterID dest)
    {
        load32(src, dataTempRegister);
        add32(dataTempRegister, dest);
    }

    void add32(RegisterID src, Address dest)
    {
        if (dest.offset >= -32768 && dest.offset <= 32767 && !m_fixedWidth) {
            /*
              lw        dataTemp, offset(base)
              addu      dataTemp, dataTemp, src
              sw        dataTemp, offset(base)
            */
            m_assembler.lw(dataTempRegister, dest.base, dest.offset);
            m_assembler.addu(dataTempRegister, dataTempRegister, src);
            m_assembler.sw(dataTempRegister, dest.base, dest.offset);
        } else {
            /*
              lui       addrTemp, (offset + 0x8000) >> 16
              addu      addrTemp, addrTemp, base
              lw        dataTemp, (offset & 0xffff)(addrTemp)
              addu      dataTemp, dataTemp, src
              sw        dataTemp, (offset & 0xffff)(addrTemp)
            */
            m_assembler.lui(addrTempRegister, (dest.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister, dest.base);
            m_assembler.lw(dataTempRegister, addrTempRegister, dest.offset);
            m_assembler.addu(dataTempRegister, dataTempRegister, src);
            m_assembler.sw(dataTempRegister, addrTempRegister, dest.offset);
        }
    }

    void add32(TrustedImm32 imm, AbsoluteAddress address)
    {
        /*
           li   addrTemp, address
           li   immTemp, imm
           lw   dataTemp, 0(addrTemp)
           addu dataTemp, dataTemp, immTemp
           sw   dataTemp, 0(addrTemp)
        */
        move(TrustedImmPtr(address.m_ptr), addrTempRegister);
        m_assembler.lw(dataTempRegister, addrTempRegister, 0);
        if (!imm.m_isPointer && imm.m_value >= -32768 && imm.m_value <= 32767
            && !m_fixedWidth)
            m_assembler.addiu(dataTempRegister, dataTempRegister, imm.m_value);
        else {
            move(imm, immTempRegister);
            m_assembler.addu(dataTempRegister, dataTempRegister, immTempRegister);
        }
        m_assembler.sw(dataTempRegister, addrTempRegister, 0);
    }

    void and32(RegisterID src, RegisterID dest)
    {
        m_assembler.andInsn(dest, dest, src);
    }

    void and32(TrustedImm32 imm, RegisterID dest)
    {
        if (!imm.m_isPointer && !imm.m_value && !m_fixedWidth)
            move(MIPSRegisters::zero, dest);
        else if (!imm.m_isPointer && imm.m_value > 0 && imm.m_value < 65535
                 && !m_fixedWidth)
            m_assembler.andi(dest, dest, imm.m_value);
        else {
            /*
              li        immTemp, imm
              and       dest, dest, immTemp
            */
            move(imm, immTempRegister);
            m_assembler.andInsn(dest, dest, immTempRegister);
        }
    }

    void lshift32(TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.sll(dest, dest, imm.m_value);
    }

    void lshift32(RegisterID shiftAmount, RegisterID dest)
    {
        m_assembler.sllv(dest, dest, shiftAmount);
    }

    void mul32(RegisterID src, RegisterID dest)
    {
        m_assembler.mul(dest, dest, src);
    }

    void mul32(TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        if (!imm.m_isPointer && !imm.m_value && !m_fixedWidth)
            move(MIPSRegisters::zero, dest);
        else if (!imm.m_isPointer && imm.m_value == 1 && !m_fixedWidth)
            move(src, dest);
        else {
            /*
                li      dataTemp, imm
                mul     dest, src, dataTemp
            */
            move(imm, dataTempRegister);
            m_assembler.mul(dest, src, dataTempRegister);
        }
    }

    void neg32(RegisterID srcDest)
    {
        m_assembler.subu(srcDest, MIPSRegisters::zero, srcDest);
    }

    void not32(RegisterID srcDest)
    {
        m_assembler.nor(srcDest, srcDest, MIPSRegisters::zero);
    }

    void or32(RegisterID src, RegisterID dest)
    {
        m_assembler.orInsn(dest, dest, src);
    }

    void or32(TrustedImm32 imm, RegisterID dest)
    {
        if (!imm.m_isPointer && !imm.m_value && !m_fixedWidth)
            return;

        if (!imm.m_isPointer && imm.m_value > 0 && imm.m_value < 65535
            && !m_fixedWidth) {
            m_assembler.ori(dest, dest, imm.m_value);
            return;
        }

        /*
            li      dataTemp, imm
            or      dest, dest, dataTemp
        */
        move(imm, dataTempRegister);
        m_assembler.orInsn(dest, dest, dataTempRegister);
    }

    void rshift32(RegisterID shiftAmount, RegisterID dest)
    {
        m_assembler.srav(dest, dest, shiftAmount);
    }

    void rshift32(TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.sra(dest, dest, imm.m_value);
    }

    void urshift32(RegisterID shiftAmount, RegisterID dest)
    {
        m_assembler.srlv(dest, dest, shiftAmount);
    }

    void urshift32(TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.srl(dest, dest, imm.m_value);
    }

    void sub32(RegisterID src, RegisterID dest)
    {
        m_assembler.subu(dest, dest, src);
    }

    void sub32(TrustedImm32 imm, RegisterID dest)
    {
        if (!imm.m_isPointer && imm.m_value >= -32767 && imm.m_value <= 32768
            && !m_fixedWidth) {
            /*
              addiu     dest, src, imm
            */
            m_assembler.addiu(dest, dest, -imm.m_value);
        } else {
            /*
              li        immTemp, imm
              subu      dest, src, immTemp
            */
            move(imm, immTempRegister);
            m_assembler.subu(dest, dest, immTempRegister);
        }
    }

    void sub32(TrustedImm32 imm, Address address)
    {
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth) {
            /*
              lw        dataTemp, offset(base)
              li        immTemp, imm
              subu      dataTemp, dataTemp, immTemp
              sw        dataTemp, offset(base)
            */
            m_assembler.lw(dataTempRegister, address.base, address.offset);
            if (!imm.m_isPointer
                && imm.m_value >= -32767 && imm.m_value <= 32768
                && !m_fixedWidth)
                m_assembler.addiu(dataTempRegister, dataTempRegister,
                                  -imm.m_value);
            else {
                move(imm, immTempRegister);
                m_assembler.subu(dataTempRegister, dataTempRegister,
                                 immTempRegister);
            }
            m_assembler.sw(dataTempRegister, address.base, address.offset);
        } else {
            /*
              lui       addrTemp, (offset + 0x8000) >> 16
              addu      addrTemp, addrTemp, base
              lw        dataTemp, (offset & 0xffff)(addrTemp)
              li        immtemp, imm
              subu      dataTemp, dataTemp, immTemp
              sw        dataTemp, (offset & 0xffff)(addrTemp)
            */
            m_assembler.lui(addrTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lw(dataTempRegister, addrTempRegister, address.offset);

            if (!imm.m_isPointer
                && imm.m_value >= -32767 && imm.m_value <= 32768
                && !m_fixedWidth)
                m_assembler.addiu(dataTempRegister, dataTempRegister,
                                  -imm.m_value);
            else {
                move(imm, immTempRegister);
                m_assembler.subu(dataTempRegister, dataTempRegister,
                                 immTempRegister);
            }
            m_assembler.sw(dataTempRegister, addrTempRegister, address.offset);
        }
    }

    void sub32(Address src, RegisterID dest)
    {
        load32(src, dataTempRegister);
        sub32(dataTempRegister, dest);
    }

    void sub32(TrustedImm32 imm, AbsoluteAddress address)
    {
        /*
           li   addrTemp, address
           li   immTemp, imm
           lw   dataTemp, 0(addrTemp)
           subu dataTemp, dataTemp, immTemp
           sw   dataTemp, 0(addrTemp)
        */
        move(TrustedImmPtr(address.m_ptr), addrTempRegister);
        m_assembler.lw(dataTempRegister, addrTempRegister, 0);

        if (!imm.m_isPointer && imm.m_value >= -32767 && imm.m_value <= 32768
            && !m_fixedWidth) {
            m_assembler.addiu(dataTempRegister, dataTempRegister,
                              -imm.m_value);
        } else {
            move(imm, immTempRegister);
            m_assembler.subu(dataTempRegister, dataTempRegister, immTempRegister);
        }
        m_assembler.sw(dataTempRegister, addrTempRegister, 0);
    }

    void xor32(RegisterID src, RegisterID dest)
    {
        m_assembler.xorInsn(dest, dest, src);
    }

    void xor32(TrustedImm32 imm, RegisterID dest)
    {
        /*
            li  immTemp, imm
            xor dest, dest, immTemp
        */
        move(imm, immTempRegister);
        m_assembler.xorInsn(dest, dest, immTempRegister);
    }

    void sqrtDouble(FPRegisterID src, FPRegisterID dst)
    {
        m_assembler.sqrtd(dst, src);
    }

    // Memory access operations:
    //
    // Loads are of the form load(address, destination) and stores of the form
    // store(source, address).  The source for a store may be an TrustedImm32.  Address
    // operand objects to loads and store will be implicitly constructed if a
    // register is passed.

    /* Need to use zero-extened load byte for load8.  */
    void load8(ImplicitAddress address, RegisterID dest)
    {
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth)
            m_assembler.lbu(dest, address.base, address.offset);
        else {
            /*
                lui     addrTemp, (offset + 0x8000) >> 16
                addu    addrTemp, addrTemp, base
                lbu     dest, (offset & 0xffff)(addrTemp)
              */
            m_assembler.lui(addrTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lbu(dest, addrTempRegister, address.offset);
        }
    }

    void load32(ImplicitAddress address, RegisterID dest)
    {
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth)
            m_assembler.lw(dest, address.base, address.offset);
        else {
            /*
                lui     addrTemp, (offset + 0x8000) >> 16
                addu    addrTemp, addrTemp, base
                lw      dest, (offset & 0xffff)(addrTemp)
              */
            m_assembler.lui(addrTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lw(dest, addrTempRegister, address.offset);
        }
    }

    void load32(BaseIndex address, RegisterID dest)
    {
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth) {
            /*
                sll     addrTemp, address.index, address.scale
                addu    addrTemp, addrTemp, address.base
                lw      dest, address.offset(addrTemp)
            */
            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lw(dest, addrTempRegister, address.offset);
        } else {
            /*
                sll     addrTemp, address.index, address.scale
                addu    addrTemp, addrTemp, address.base
                lui     immTemp, (address.offset + 0x8000) >> 16
                addu    addrTemp, addrTemp, immTemp
                lw      dest, (address.offset & 0xffff)(at)
            */
            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lui(immTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister,
                             immTempRegister);
            m_assembler.lw(dest, addrTempRegister, address.offset);
        }
    }

    void load32WithUnalignedHalfWords(BaseIndex address, RegisterID dest)
    {
        if (address.offset >= -32768 && address.offset <= 32764
            && !m_fixedWidth) {
            /*
                sll     addrTemp, address.index, address.scale
                addu    addrTemp, addrTemp, address.base
                (Big-Endian)
                lwl     dest, address.offset(addrTemp)
                lwr     dest, address.offset+3(addrTemp)
                (Little-Endian)
                lwl     dest, address.offset+3(addrTemp)
                lwr     dest, address.offset(addrTemp)
            */
            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
#if CPU(BIG_ENDIAN)
            m_assembler.lwl(dest, addrTempRegister, address.offset);
            m_assembler.lwr(dest, addrTempRegister, address.offset + 3);
#else
            m_assembler.lwl(dest, addrTempRegister, address.offset + 3);
            m_assembler.lwr(dest, addrTempRegister, address.offset);

#endif
        } else {
            /*
                sll     addrTemp, address.index, address.scale
                addu    addrTemp, addrTemp, address.base
                lui     immTemp, address.offset >> 16
                ori     immTemp, immTemp, address.offset & 0xffff
                addu    addrTemp, addrTemp, immTemp
                (Big-Endian)
                lw      dest, 0(at)
                lw      dest, 3(at)
                (Little-Endian)
                lw      dest, 3(at)
                lw      dest, 0(at)
            */
            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lui(immTempRegister, address.offset >> 16);
            m_assembler.ori(immTempRegister, immTempRegister, address.offset);
            m_assembler.addu(addrTempRegister, addrTempRegister,
                             immTempRegister);
#if CPU(BIG_ENDIAN)
            m_assembler.lwl(dest, addrTempRegister, 0);
            m_assembler.lwr(dest, addrTempRegister, 3);
#else
            m_assembler.lwl(dest, addrTempRegister, 3);
            m_assembler.lwr(dest, addrTempRegister, 0);
#endif
        }
    }

    void load32(const void* address, RegisterID dest)
    {
        /*
            li  addrTemp, address
            lw  dest, 0(addrTemp)
        */
        move(TrustedImmPtr(address), addrTempRegister);
        m_assembler.lw(dest, addrTempRegister, 0);
    }

    DataLabel32 load32WithAddressOffsetPatch(Address address, RegisterID dest)
    {
        m_fixedWidth = true;
        /*
            lui addrTemp, address.offset >> 16
            ori addrTemp, addrTemp, address.offset & 0xffff
            addu        addrTemp, addrTemp, address.base
            lw  dest, 0(addrTemp)
        */
        DataLabel32 dataLabel(this);
        move(TrustedImm32(address.offset), addrTempRegister);
        m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
        m_assembler.lw(dest, addrTempRegister, 0);
        m_fixedWidth = false;
        return dataLabel;
    }

    /* Need to use zero-extened load half-word for load16.  */
    void load16(ImplicitAddress address, RegisterID dest)
    {
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth)
            m_assembler.lhu(dest, address.base, address.offset);
        else {
            /*
                lui     addrTemp, (offset + 0x8000) >> 16
                addu    addrTemp, addrTemp, base
                lhu     dest, (offset & 0xffff)(addrTemp)
              */
            m_assembler.lui(addrTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lhu(dest, addrTempRegister, address.offset);
        }
    }

    /* Need to use zero-extened load half-word for load16.  */
    void load16(BaseIndex address, RegisterID dest)
    {
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth) {
            /*
                sll     addrTemp, address.index, address.scale
                addu    addrTemp, addrTemp, address.base
                lhu     dest, address.offset(addrTemp)
            */
            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lhu(dest, addrTempRegister, address.offset);
        } else {
            /*
                sll     addrTemp, address.index, address.scale
                addu    addrTemp, addrTemp, address.base
                lui     immTemp, (address.offset + 0x8000) >> 16
                addu    addrTemp, addrTemp, immTemp
                lhu     dest, (address.offset & 0xffff)(addrTemp)
            */
            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lui(immTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister,
                             immTempRegister);
            m_assembler.lhu(dest, addrTempRegister, address.offset);
        }
    }

    DataLabel32 store32WithAddressOffsetPatch(RegisterID src, Address address)
    {
        m_fixedWidth = true;
        /*
            lui addrTemp, address.offset >> 16
            ori addrTemp, addrTemp, address.offset & 0xffff
            addu        addrTemp, addrTemp, address.base
            sw  src, 0(addrTemp)
        */
        DataLabel32 dataLabel(this);
        move(TrustedImm32(address.offset), addrTempRegister);
        m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
        m_assembler.sw(src, addrTempRegister, 0);
        m_fixedWidth = false;
        return dataLabel;
    }

    void store32(RegisterID src, ImplicitAddress address)
    {
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth)
            m_assembler.sw(src, address.base, address.offset);
        else {
            /*
                lui     addrTemp, (offset + 0x8000) >> 16
                addu    addrTemp, addrTemp, base
                sw      src, (offset & 0xffff)(addrTemp)
              */
            m_assembler.lui(addrTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.sw(src, addrTempRegister, address.offset);
        }
    }

    void store32(RegisterID src, BaseIndex address)
    {
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth) {
            /*
                sll     addrTemp, address.index, address.scale
                addu    addrTemp, addrTemp, address.base
                sw      src, address.offset(addrTemp)
            */
            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.sw(src, addrTempRegister, address.offset);
        } else {
            /*
                sll     addrTemp, address.index, address.scale
                addu    addrTemp, addrTemp, address.base
                lui     immTemp, (address.offset + 0x8000) >> 16
                addu    addrTemp, addrTemp, immTemp
                sw      src, (address.offset & 0xffff)(at)
            */
            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lui(immTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister,
                             immTempRegister);
            m_assembler.sw(src, addrTempRegister, address.offset);
        }
    }

    void store32(TrustedImm32 imm, ImplicitAddress address)
    {
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth) {
            if (!imm.m_isPointer && !imm.m_value)
                m_assembler.sw(MIPSRegisters::zero, address.base,
                               address.offset);
            else {
                move(imm, immTempRegister);
                m_assembler.sw(immTempRegister, address.base, address.offset);
            }
        } else {
            /*
                lui     addrTemp, (offset + 0x8000) >> 16
                addu    addrTemp, addrTemp, base
                sw      immTemp, (offset & 0xffff)(addrTemp)
              */
            m_assembler.lui(addrTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            if (!imm.m_isPointer && !imm.m_value && !m_fixedWidth)
                m_assembler.sw(MIPSRegisters::zero, addrTempRegister,
                               address.offset);
            else {
                move(imm, immTempRegister);
                m_assembler.sw(immTempRegister, addrTempRegister,
                               address.offset);
            }
        }
    }

    void store32(RegisterID src, const void* address)
    {
        /*
            li  addrTemp, address
            sw  src, 0(addrTemp)
        */
        move(TrustedImmPtr(address), addrTempRegister);
        m_assembler.sw(src, addrTempRegister, 0);
    }

    void store32(TrustedImm32 imm, const void* address)
    {
        /*
            li  immTemp, imm
            li  addrTemp, address
            sw  src, 0(addrTemp)
        */
        if (!imm.m_isPointer && !imm.m_value && !m_fixedWidth) {
            move(TrustedImmPtr(address), addrTempRegister);
            m_assembler.sw(MIPSRegisters::zero, addrTempRegister, 0);
        } else {
            move(imm, immTempRegister);
            move(TrustedImmPtr(address), addrTempRegister);
            m_assembler.sw(immTempRegister, addrTempRegister, 0);
        }
    }

    // Floating-point operations:

    bool supportsFloatingPoint() const
    {
#if WTF_MIPS_DOUBLE_FLOAT
        return true;
#else
        return false;
#endif
    }

    bool supportsFloatingPointTruncate() const
    {
#if WTF_MIPS_DOUBLE_FLOAT && WTF_MIPS_ISA_AT_LEAST(2)
        return true;
#else
        return false;
#endif
    }

    bool supportsFloatingPointSqrt() const
    {
#if WTF_MIPS_DOUBLE_FLOAT && WTF_MIPS_ISA_AT_LEAST(2)
        return true;
#else
        return false;
#endif
    }

    // Stack manipulation operations:
    //
    // The ABI is assumed to provide a stack abstraction to memory,
    // containing machine word sized units of data.  Push and pop
    // operations add and remove a single register sized unit of data
    // to or from the stack.  Peek and poke operations read or write
    // values on the stack, without moving the current stack position.

    void pop(RegisterID dest)
    {
        m_assembler.lw(dest, MIPSRegisters::sp, 0);
        m_assembler.addiu(MIPSRegisters::sp, MIPSRegisters::sp, 4);
    }

    void push(RegisterID src)
    {
        m_assembler.addiu(MIPSRegisters::sp, MIPSRegisters::sp, -4);
        m_assembler.sw(src, MIPSRegisters::sp, 0);
    }

    void push(Address address)
    {
        load32(address, dataTempRegister);
        push(dataTempRegister);
    }

    void push(TrustedImm32 imm)
    {
        move(imm, immTempRegister);
        push(immTempRegister);
    }

    // Register move operations:
    //
    // Move values in registers.

    void move(TrustedImm32 imm, RegisterID dest)
    {
        if (!imm.m_isPointer && !imm.m_value && !m_fixedWidth)
            move(MIPSRegisters::zero, dest);
        else if (imm.m_isPointer || m_fixedWidth) {
            m_assembler.lui(dest, imm.m_value >> 16);
            m_assembler.ori(dest, dest, imm.m_value);
        } else
            m_assembler.li(dest, imm.m_value);
    }

    void move(RegisterID src, RegisterID dest)
    {
        if (src != dest || m_fixedWidth)
            m_assembler.move(dest, src);
    }

    void move(TrustedImmPtr imm, RegisterID dest)
    {
        move(TrustedImm32(imm), dest);
    }

    void swap(RegisterID reg1, RegisterID reg2)
    {
        move(reg1, immTempRegister);
        move(reg2, reg1);
        move(immTempRegister, reg2);
    }

    void signExtend32ToPtr(RegisterID src, RegisterID dest)
    {
        if (src != dest || m_fixedWidth)
            move(src, dest);
    }

    void zeroExtend32ToPtr(RegisterID src, RegisterID dest)
    {
        if (src != dest || m_fixedWidth)
            move(src, dest);
    }

    // Forwards / external control flow operations:
    //
    // This set of jump and conditional branch operations return a Jump
    // object which may linked at a later point, allow forwards jump,
    // or jumps that will require external linkage (after the code has been
    // relocated).
    //
    // For branches, signed <, >, <= and >= are denoted as l, g, le, and ge
    // respecitvely, for unsigned comparisons the names b, a, be, and ae are
    // used (representing the names 'below' and 'above').
    //
    // Operands to the comparision are provided in the expected order, e.g.
    // jle32(reg1, TrustedImm32(5)) will branch if the value held in reg1, when
    // treated as a signed 32bit value, is less than or equal to 5.
    //
    // jz and jnz test whether the first operand is equal to zero, and take
    // an optional second operand of a mask under which to perform the test.

    Jump branch8(RelationalCondition cond, Address left, TrustedImm32 right)
    {
        // Make sure the immediate value is unsigned 8 bits.
        ASSERT(!(right.m_value & 0xFFFFFF00));
        load8(left, dataTempRegister);
        move(right, immTempRegister);
        return branch32(cond, dataTempRegister, immTempRegister);
    }

    Jump branch32(RelationalCondition cond, RegisterID left, RegisterID right)
    {
        if (cond == Equal)
            return branchEqual(left, right);
        if (cond == NotEqual)
            return branchNotEqual(left, right);
        if (cond == Above) {
            m_assembler.sltu(cmpTempRegister, right, left);
            return branchNotEqual(cmpTempRegister, MIPSRegisters::zero);
        }
        if (cond == AboveOrEqual) {
            m_assembler.sltu(cmpTempRegister, left, right);
            return branchEqual(cmpTempRegister, MIPSRegisters::zero);
        }
        if (cond == Below) {
            m_assembler.sltu(cmpTempRegister, left, right);
            return branchNotEqual(cmpTempRegister, MIPSRegisters::zero);
        }
        if (cond == BelowOrEqual) {
            m_assembler.sltu(cmpTempRegister, right, left);
            return branchEqual(cmpTempRegister, MIPSRegisters::zero);
        }
        if (cond == GreaterThan) {
            m_assembler.slt(cmpTempRegister, right, left);
            return branchNotEqual(cmpTempRegister, MIPSRegisters::zero);
        }
        if (cond == GreaterThanOrEqual) {
            m_assembler.slt(cmpTempRegister, left, right);
            return branchEqual(cmpTempRegister, MIPSRegisters::zero);
        }
        if (cond == LessThan) {
            m_assembler.slt(cmpTempRegister, left, right);
            return branchNotEqual(cmpTempRegister, MIPSRegisters::zero);
        }
        if (cond == LessThanOrEqual) {
            m_assembler.slt(cmpTempRegister, right, left);
            return branchEqual(cmpTempRegister, MIPSRegisters::zero);
        }
        ASSERT(0);

        return Jump();
    }

    Jump branch32(RelationalCondition cond, RegisterID left, TrustedImm32 right)
    {
        move(right, immTempRegister);
        return branch32(cond, left, immTempRegister);
    }

    Jump branch32(RelationalCondition cond, RegisterID left, Address right)
    {
        load32(right, dataTempRegister);
        return branch32(cond, left, dataTempRegister);
    }

    Jump branch32(RelationalCondition cond, Address left, RegisterID right)
    {
        load32(left, dataTempRegister);
        return branch32(cond, dataTempRegister, right);
    }

    Jump branch32(RelationalCondition cond, Address left, TrustedImm32 right)
    {
        load32(left, dataTempRegister);
        move(right, immTempRegister);
        return branch32(cond, dataTempRegister, immTempRegister);
    }

    Jump branch32(RelationalCondition cond, BaseIndex left, TrustedImm32 right)
    {
        load32(left, dataTempRegister);
        // Be careful that the previous load32() uses immTempRegister.
        // So, we need to put move() after load32().
        move(right, immTempRegister);
        return branch32(cond, dataTempRegister, immTempRegister);
    }

    Jump branch32WithUnalignedHalfWords(RelationalCondition cond, BaseIndex left, TrustedImm32 right)
    {
        load32WithUnalignedHalfWords(left, dataTempRegister);
        // Be careful that the previous load32WithUnalignedHalfWords()
        // uses immTempRegister.
        // So, we need to put move() after load32WithUnalignedHalfWords().
        move(right, immTempRegister);
        return branch32(cond, dataTempRegister, immTempRegister);
    }

    Jump branch32(RelationalCondition cond, AbsoluteAddress left, RegisterID right)
    {
        load32(left.m_ptr, dataTempRegister);
        return branch32(cond, dataTempRegister, right);
    }

    Jump branch32(RelationalCondition cond, AbsoluteAddress left, TrustedImm32 right)
    {
        load32(left.m_ptr, dataTempRegister);
        move(right, immTempRegister);
        return branch32(cond, dataTempRegister, immTempRegister);
    }

    Jump branch16(RelationalCondition cond, BaseIndex left, RegisterID right)
    {
        load16(left, dataTempRegister);
        return branch32(cond, dataTempRegister, right);
    }

    Jump branch16(RelationalCondition cond, BaseIndex left, TrustedImm32 right)
    {
        ASSERT(!(right.m_value & 0xFFFF0000));
        load16(left, dataTempRegister);
        // Be careful that the previous load16() uses immTempRegister.
        // So, we need to put move() after load16().
        move(right, immTempRegister);
        return branch32(cond, dataTempRegister, immTempRegister);
    }

    Jump branchTest32(ResultCondition cond, RegisterID reg, RegisterID mask)
    {
        ASSERT((cond == Zero) || (cond == NonZero));
        m_assembler.andInsn(cmpTempRegister, reg, mask);
        if (cond == Zero)
            return branchEqual(cmpTempRegister, MIPSRegisters::zero);
        return branchNotEqual(cmpTempRegister, MIPSRegisters::zero);
    }

    Jump branchTest32(ResultCondition cond, RegisterID reg, TrustedImm32 mask = TrustedImm32(-1))
    {
        ASSERT((cond == Zero) || (cond == NonZero));
        if (mask.m_value == -1 && !m_fixedWidth) {
            if (cond == Zero)
                return branchEqual(reg, MIPSRegisters::zero);
            return branchNotEqual(reg, MIPSRegisters::zero);
        }
        move(mask, immTempRegister);
        return branchTest32(cond, reg, immTempRegister);
    }

    Jump branchTest32(ResultCondition cond, Address address, TrustedImm32 mask = TrustedImm32(-1))
    {
        load32(address, dataTempRegister);
        return branchTest32(cond, dataTempRegister, mask);
    }

    Jump branchTest32(ResultCondition cond, BaseIndex address, TrustedImm32 mask = TrustedImm32(-1))
    {
        load32(address, dataTempRegister);
        return branchTest32(cond, dataTempRegister, mask);
    }

    Jump branchTest8(ResultCondition cond, Address address, TrustedImm32 mask = TrustedImm32(-1))
    {
        load8(address, dataTempRegister);
        return branchTest32(cond, dataTempRegister, mask);
    }

    Jump jump()
    {
        return branchEqual(MIPSRegisters::zero, MIPSRegisters::zero);
    }

    void jump(RegisterID target)
    {
        m_assembler.jr(target);
        m_assembler.nop();
    }

    void jump(Address address)
    {
        m_fixedWidth = true;
        load32(address, MIPSRegisters::t9);
        m_assembler.jr(MIPSRegisters::t9);
        m_assembler.nop();
        m_fixedWidth = false;
    }

    // Arithmetic control flow operations:
    //
    // This set of conditional branch operations branch based
    // on the result of an arithmetic operation.  The operation
    // is performed as normal, storing the result.
    //
    // * jz operations branch if the result is zero.
    // * jo operations branch if the (signed) arithmetic
    //   operation caused an overflow to occur.

    Jump branchAdd32(ResultCondition cond, RegisterID src, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == Zero) || (cond == NonZero));
        if (cond == Overflow) {
            /*
                move    dest, dataTemp
                xor     cmpTemp, dataTemp, src
                bltz    cmpTemp, No_overflow    # diff sign bit -> no overflow
                addu    dest, dataTemp, src
                xor     cmpTemp, dest, dataTemp
                bgez    cmpTemp, No_overflow    # same sign big -> no overflow
                nop
                b       Overflow
                nop
                nop
                nop
                nop
                nop
            No_overflow:
            */
            move(dest, dataTempRegister);
            m_assembler.xorInsn(cmpTempRegister, dataTempRegister, src);
            m_assembler.bltz(cmpTempRegister, 10);
            m_assembler.addu(dest, dataTempRegister, src);
            m_assembler.xorInsn(cmpTempRegister, dest, dataTempRegister);
            m_assembler.bgez(cmpTempRegister, 7);
            m_assembler.nop();
            return jump();
        }
        if (cond == Signed) {
            add32(src, dest);
            // Check if dest is negative.
            m_assembler.slt(cmpTempRegister, dest, MIPSRegisters::zero);
            return branchNotEqual(cmpTempRegister, MIPSRegisters::zero);
        }
        if (cond == Zero) {
            add32(src, dest);
            return branchEqual(dest, MIPSRegisters::zero);
        }
        if (cond == NonZero) {
            add32(src, dest);
            return branchNotEqual(dest, MIPSRegisters::zero);
        }
        ASSERT(0);
        return Jump();
    }

    Jump branchAdd32(ResultCondition cond, TrustedImm32 imm, RegisterID dest)
    {
        move(imm, immTempRegister);
        return branchAdd32(cond, immTempRegister, dest);
    }

    Jump branchMul32(ResultCondition cond, RegisterID src, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == Zero) || (cond == NonZero));
        if (cond == Overflow) {
            /*
                mult    src, dest
                mfhi    dataTemp
                mflo    dest
                sra     addrTemp, dest, 31
                beq     dataTemp, addrTemp, No_overflow # all sign bits (bit 63 to bit 31) are the same -> no overflow
                nop
                b       Overflow
                nop
                nop
                nop
                nop
                nop
            No_overflow:
            */
            m_assembler.mult(src, dest);
            m_assembler.mfhi(dataTempRegister);
            m_assembler.mflo(dest);
            m_assembler.sra(addrTempRegister, dest, 31);
            m_assembler.beq(dataTempRegister, addrTempRegister, 7);
            m_assembler.nop();
            return jump();
        }
        if (cond == Signed) {
            mul32(src, dest);
            // Check if dest is negative.
            m_assembler.slt(cmpTempRegister, dest, MIPSRegisters::zero);
            return branchNotEqual(cmpTempRegister, MIPSRegisters::zero);
        }
        if (cond == Zero) {
            mul32(src, dest);
            return branchEqual(dest, MIPSRegisters::zero);
        }
        if (cond == NonZero) {
            mul32(src, dest);
            return branchNotEqual(dest, MIPSRegisters::zero);
        }
        ASSERT(0);
        return Jump();
    }

    Jump branchMul32(ResultCondition cond, TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        move(imm, immTempRegister);
        move(src, dest);
        return branchMul32(cond, immTempRegister, dest);
    }

    Jump branchSub32(ResultCondition cond, RegisterID src, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == Zero) || (cond == NonZero));
        if (cond == Overflow) {
            /*
                move    dest, dataTemp
                xor     cmpTemp, dataTemp, src
                bgez    cmpTemp, No_overflow    # same sign bit -> no overflow
                subu    dest, dataTemp, src
                xor     cmpTemp, dest, dataTemp
                bgez    cmpTemp, No_overflow    # same sign bit -> no overflow
                nop
                b       Overflow
                nop
                nop
                nop
                nop
                nop
            No_overflow:
            */
            move(dest, dataTempRegister);
            m_assembler.xorInsn(cmpTempRegister, dataTempRegister, src);
            m_assembler.bgez(cmpTempRegister, 10);
            m_assembler.subu(dest, dataTempRegister, src);
            m_assembler.xorInsn(cmpTempRegister, dest, dataTempRegister);
            m_assembler.bgez(cmpTempRegister, 7);
            m_assembler.nop();
            return jump();
        }
        if (cond == Signed) {
            sub32(src, dest);
            // Check if dest is negative.
            m_assembler.slt(cmpTempRegister, dest, MIPSRegisters::zero);
            return branchNotEqual(cmpTempRegister, MIPSRegisters::zero);
        }
        if (cond == Zero) {
            sub32(src, dest);
            return branchEqual(dest, MIPSRegisters::zero);
        }
        if (cond == NonZero) {
            sub32(src, dest);
            return branchNotEqual(dest, MIPSRegisters::zero);
        }
        ASSERT(0);
        return Jump();
    }

    Jump branchSub32(ResultCondition cond, TrustedImm32 imm, RegisterID dest)
    {
        move(imm, immTempRegister);
        return branchSub32(cond, immTempRegister, dest);
    }

    Jump branchOr32(ResultCondition cond, RegisterID src, RegisterID dest)
    {
        ASSERT((cond == Signed) || (cond == Zero) || (cond == NonZero));
        if (cond == Signed) {
            or32(src, dest);
            // Check if dest is negative.
            m_assembler.slt(cmpTempRegister, dest, MIPSRegisters::zero);
            return branchNotEqual(cmpTempRegister, MIPSRegisters::zero);
        }
        if (cond == Zero) {
            or32(src, dest);
            return branchEqual(dest, MIPSRegisters::zero);
        }
        if (cond == NonZero) {
            or32(src, dest);
            return branchNotEqual(dest, MIPSRegisters::zero);
        }
        ASSERT(0);
        return Jump();
    }

    // Miscellaneous operations:

    void breakpoint()
    {
        m_assembler.bkpt();
    }

    Call nearCall()
    {
        /* We need two words for relaxation.  */
        m_assembler.nop();
        m_assembler.nop();
        m_assembler.jal();
        m_assembler.nop();
        return Call(m_assembler.label(), Call::LinkableNear);
    }

    Call call()
    {
        m_assembler.lui(MIPSRegisters::t9, 0);
        m_assembler.ori(MIPSRegisters::t9, MIPSRegisters::t9, 0);
        m_assembler.jalr(MIPSRegisters::t9);
        m_assembler.nop();
        return Call(m_assembler.label(), Call::Linkable);
    }

    Call call(RegisterID target)
    {
        m_assembler.jalr(target);
        m_assembler.nop();
        return Call(m_assembler.label(), Call::None);
    }

    Call call(Address address)
    {
        m_fixedWidth = true;
        load32(address, MIPSRegisters::t9);
        m_assembler.jalr(MIPSRegisters::t9);
        m_assembler.nop();
        m_fixedWidth = false;
        return Call(m_assembler.label(), Call::None);
    }

    void ret()
    {
        m_assembler.jr(MIPSRegisters::ra);
        m_assembler.nop();
    }

    void compare32(RelationalCondition cond, RegisterID left, RegisterID right, RegisterID dest)
    {
        if (cond == Equal) {
            m_assembler.xorInsn(dest, left, right);
            m_assembler.sltiu(dest, dest, 1);
        } else if (cond == NotEqual) {
            m_assembler.xorInsn(dest, left, right);
            m_assembler.sltu(dest, MIPSRegisters::zero, dest);
        } else if (cond == Above)
            m_assembler.sltu(dest, right, left);
        else if (cond == AboveOrEqual) {
            m_assembler.sltu(dest, left, right);
            m_assembler.xori(dest, dest, 1);
        } else if (cond == Below)
            m_assembler.sltu(dest, left, right);
        else if (cond == BelowOrEqual) {
            m_assembler.sltu(dest, right, left);
            m_assembler.xori(dest, dest, 1);
        } else if (cond == GreaterThan)
            m_assembler.slt(dest, right, left);
        else if (cond == GreaterThanOrEqual) {
            m_assembler.slt(dest, left, right);
            m_assembler.xori(dest, dest, 1);
        } else if (cond == LessThan)
            m_assembler.slt(dest, left, right);
        else if (cond == LessThanOrEqual) {
            m_assembler.slt(dest, right, left);
            m_assembler.xori(dest, dest, 1);
        }
    }

    void compare32(RelationalCondition cond, RegisterID left, TrustedImm32 right, RegisterID dest)
    {
        move(right, immTempRegister);
        compare32(cond, left, immTempRegister, dest);
    }

    void test8(ResultCondition cond, Address address, TrustedImm32 mask, RegisterID dest)
    {
        ASSERT((cond == Zero) || (cond == NonZero));
        load8(address, dataTempRegister);
        if (mask.m_value == -1 && !m_fixedWidth) {
            if (cond == Zero)
                m_assembler.sltiu(dest, dataTempRegister, 1);
            else
                m_assembler.sltu(dest, MIPSRegisters::zero, dataTempRegister);
        } else {
            move(mask, immTempRegister);
            m_assembler.andInsn(cmpTempRegister, dataTempRegister,
                                immTempRegister);
            if (cond == Zero)
                m_assembler.sltiu(dest, cmpTempRegister, 1);
            else
                m_assembler.sltu(dest, MIPSRegisters::zero, cmpTempRegister);
        }
    }

    void test32(ResultCondition cond, Address address, TrustedImm32 mask, RegisterID dest)
    {
        ASSERT((cond == Zero) || (cond == NonZero));
        load32(address, dataTempRegister);
        if (mask.m_value == -1 && !m_fixedWidth) {
            if (cond == Zero)
                m_assembler.sltiu(dest, dataTempRegister, 1);
            else
                m_assembler.sltu(dest, MIPSRegisters::zero, dataTempRegister);
        } else {
            move(mask, immTempRegister);
            m_assembler.andInsn(cmpTempRegister, dataTempRegister,
                                immTempRegister);
            if (cond == Zero)
                m_assembler.sltiu(dest, cmpTempRegister, 1);
            else
                m_assembler.sltu(dest, MIPSRegisters::zero, cmpTempRegister);
        }
    }

    DataLabel32 moveWithPatch(TrustedImm32 imm, RegisterID dest)
    {
        m_fixedWidth = true;
        DataLabel32 label(this);
        move(imm, dest);
        m_fixedWidth = false;
        return label;
    }

    DataLabelPtr moveWithPatch(TrustedImmPtr initialValue, RegisterID dest)
    {
        m_fixedWidth = true;
        DataLabelPtr label(this);
        move(initialValue, dest);
        m_fixedWidth = false;
        return label;
    }

    Jump branchPtrWithPatch(RelationalCondition cond, RegisterID left, DataLabelPtr& dataLabel, TrustedImmPtr initialRightValue = TrustedImmPtr(0))
    {
        m_fixedWidth = true;
        dataLabel = moveWithPatch(initialRightValue, immTempRegister);
        Jump temp = branch32(cond, left, immTempRegister);
        m_fixedWidth = false;
        return temp;
    }

    Jump branchPtrWithPatch(RelationalCondition cond, Address left, DataLabelPtr& dataLabel, TrustedImmPtr initialRightValue = TrustedImmPtr(0))
    {
        m_fixedWidth = true;
        load32(left, dataTempRegister);
        dataLabel = moveWithPatch(initialRightValue, immTempRegister);
        Jump temp = branch32(cond, dataTempRegister, immTempRegister);
        m_fixedWidth = false;
        return temp;
    }

    DataLabelPtr storePtrWithPatch(TrustedImmPtr initialValue, ImplicitAddress address)
    {
        m_fixedWidth = true;
        DataLabelPtr dataLabel = moveWithPatch(initialValue, dataTempRegister);
        store32(dataTempRegister, address);
        m_fixedWidth = false;
        return dataLabel;
    }

    DataLabelPtr storePtrWithPatch(ImplicitAddress address)
    {
        return storePtrWithPatch(TrustedImmPtr(0), address);
    }

    Call tailRecursiveCall()
    {
        // Like a normal call, but don't update the returned address register
        m_fixedWidth = true;
        move(TrustedImm32(0), MIPSRegisters::t9);
        m_assembler.jr(MIPSRegisters::t9);
        m_assembler.nop();
        m_fixedWidth = false;
        return Call(m_assembler.label(), Call::Linkable);
    }

    Call makeTailRecursiveCall(Jump oldJump)
    {
        oldJump.link(this);
        return tailRecursiveCall();
    }

    void loadDouble(ImplicitAddress address, FPRegisterID dest)
    {
#if WTF_MIPS_ISA(1)
        /*
            li          addrTemp, address.offset
            addu        addrTemp, addrTemp, base
            lwc1        dest, 0(addrTemp)
            lwc1        dest+1, 4(addrTemp)
         */
        move(TrustedImm32(address.offset), addrTempRegister);
        m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
        m_assembler.lwc1(dest, addrTempRegister, 0);
        m_assembler.lwc1(FPRegisterID(dest + 1), addrTempRegister, 4);
#else
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth) {
            m_assembler.ldc1(dest, address.base, address.offset);
        } else {
            /*
                lui     addrTemp, (offset + 0x8000) >> 16
                addu    addrTemp, addrTemp, base
                ldc1    dest, (offset & 0xffff)(addrTemp)
              */
            m_assembler.lui(addrTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.ldc1(dest, addrTempRegister, address.offset);
        }
#endif
    }

    void loadDouble(const void* address, FPRegisterID dest)
    {
#if WTF_MIPS_ISA(1)
        /*
            li          addrTemp, address
            lwc1        dest, 0(addrTemp)
            lwc1        dest+1, 4(addrTemp)
         */
        move(TrustedImmPtr(address), addrTempRegister);
        m_assembler.lwc1(dest, addrTempRegister, 0);
        m_assembler.lwc1(FPRegisterID(dest + 1), addrTempRegister, 4);
#else
        /*
            li          addrTemp, address
            ldc1        dest, 0(addrTemp)
        */
        move(TrustedImmPtr(address), addrTempRegister);
        m_assembler.ldc1(dest, addrTempRegister, 0);
#endif
    }


    void storeDouble(FPRegisterID src, ImplicitAddress address)
    {
#if WTF_MIPS_ISA(1)
        /*
            li          addrTemp, address.offset
            addu        addrTemp, addrTemp, base
            swc1        dest, 0(addrTemp)
            swc1        dest+1, 4(addrTemp)
         */
        move(TrustedImm32(address.offset), addrTempRegister);
        m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
        m_assembler.swc1(src, addrTempRegister, 0);
        m_assembler.swc1(FPRegisterID(src + 1), addrTempRegister, 4);
#else
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth)
            m_assembler.sdc1(src, address.base, address.offset);
        else {
            /*
                lui     addrTemp, (offset + 0x8000) >> 16
                addu    addrTemp, addrTemp, base
                sdc1    src, (offset & 0xffff)(addrTemp)
              */
            m_assembler.lui(addrTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.sdc1(src, addrTempRegister, address.offset);
        }
#endif
    }

    void addDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.addd(dest, dest, src);
    }

    void addDouble(Address src, FPRegisterID dest)
    {
        loadDouble(src, fpTempRegister);
        m_assembler.addd(dest, dest, fpTempRegister);
    }

    void subDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.subd(dest, dest, src);
    }

    void subDouble(Address src, FPRegisterID dest)
    {
        loadDouble(src, fpTempRegister);
        m_assembler.subd(dest, dest, fpTempRegister);
    }

    void mulDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.muld(dest, dest, src);
    }

    void mulDouble(Address src, FPRegisterID dest)
    {
        loadDouble(src, fpTempRegister);
        m_assembler.muld(dest, dest, fpTempRegister);
    }

    void divDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.divd(dest, dest, src);
    }

    void convertInt32ToDouble(RegisterID src, FPRegisterID dest)
    {
        m_assembler.mtc1(src, fpTempRegister);
        m_assembler.cvtdw(dest, fpTempRegister);
    }

    void convertInt32ToDouble(Address src, FPRegisterID dest)
    {
        load32(src, dataTempRegister);
        m_assembler.mtc1(dataTempRegister, fpTempRegister);
        m_assembler.cvtdw(dest, fpTempRegister);
    }

    void convertInt32ToDouble(AbsoluteAddress src, FPRegisterID dest)
    {
        load32(src.m_ptr, dataTempRegister);
        m_assembler.mtc1(dataTempRegister, fpTempRegister);
        m_assembler.cvtdw(dest, fpTempRegister);
    }

    void insertRelaxationWords()
    {
        /* We need four words for relaxation. */
        m_assembler.beq(MIPSRegisters::zero, MIPSRegisters::zero, 3); // Jump over nops;
        m_assembler.nop();
        m_assembler.nop();
        m_assembler.nop();
    }

    Jump branchTrue()
    {
        m_assembler.appendJump();
        m_assembler.bc1t();
        m_assembler.nop();
        insertRelaxationWords();
        return Jump(m_assembler.label());
    }

    Jump branchFalse()
    {
        m_assembler.appendJump();
        m_assembler.bc1f();
        m_assembler.nop();
        insertRelaxationWords();
        return Jump(m_assembler.label());
    }

    Jump branchEqual(RegisterID rs, RegisterID rt)
    {
        m_assembler.appendJump();
        m_assembler.beq(rs, rt, 0);
        m_assembler.nop();
        insertRelaxationWords();
        return Jump(m_assembler.label());
    }

    Jump branchNotEqual(RegisterID rs, RegisterID rt)
    {
        m_assembler.appendJump();
        m_assembler.bne(rs, rt, 0);
        m_assembler.nop();
        insertRelaxationWords();
        return Jump(m_assembler.label());
    }

    Jump branchDouble(DoubleCondition cond, FPRegisterID left, FPRegisterID right)
    {
        if (cond == DoubleEqual) {
            m_assembler.ceqd(left, right);
            return branchTrue();
        }
        if (cond == DoubleNotEqual) {
            m_assembler.cueqd(left, right);
            return branchFalse(); // false
        }
        if (cond == DoubleGreaterThan) {
            m_assembler.cngtd(left, right);
            return branchFalse(); // false
        }
        if (cond == DoubleGreaterThanOrEqual) {
            m_assembler.cnged(left, right);
            return branchFalse(); // false
        }
        if (cond == DoubleLessThan) {
            m_assembler.cltd(left, right);
            return branchTrue();
        }
        if (cond == DoubleLessThanOrEqual) {
            m_assembler.cled(left, right);
            return branchTrue();
        }
        if (cond == DoubleEqualOrUnordered) {
            m_assembler.cueqd(left, right);
            return branchTrue();
        }
        if (cond == DoubleNotEqualOrUnordered) {
            m_assembler.ceqd(left, right);
            return branchFalse(); // false
        }
        if (cond == DoubleGreaterThanOrUnordered) {
            m_assembler.coled(left, right);
            return branchFalse(); // false
        }
        if (cond == DoubleGreaterThanOrEqualOrUnordered) {
            m_assembler.coltd(left, right);
            return branchFalse(); // false
        }
        if (cond == DoubleLessThanOrUnordered) {
            m_assembler.cultd(left, right);
            return branchTrue();
        }
        if (cond == DoubleLessThanOrEqualOrUnordered) {
            m_assembler.culed(left, right);
            return branchTrue();
        }
        ASSERT(0);

        return Jump();
    }

    // Truncates 'src' to an integer, and places the resulting 'dest'.
    // If the result is not representable as a 32 bit value, branch.
    // May also branch for some values that are representable in 32 bits
    // (specifically, in this case, INT_MAX 0x7fffffff).
    Jump branchTruncateDoubleToInt32(FPRegisterID src, RegisterID dest)
    {
        m_assembler.truncwd(fpTempRegister, src);
        m_assembler.mfc1(dest, fpTempRegister);
        return branch32(Equal, dest, TrustedImm32(0x7fffffff));
    }

    // Convert 'src' to an integer, and places the resulting 'dest'.
    // If the result is not representable as a 32 bit value, branch.
    // May also branch for some values that are representable in 32 bits
    // (specifically, in this case, 0).
    void branchConvertDoubleToInt32(FPRegisterID src, RegisterID dest, JumpList& failureCases, FPRegisterID fpTemp)
    {
        m_assembler.cvtwd(fpTempRegister, src);
        m_assembler.mfc1(dest, fpTempRegister);

        // If the result is zero, it might have been -0.0, and the double comparison won't catch this!
        failureCases.append(branch32(Equal, dest, MIPSRegisters::zero));

        // Convert the integer result back to float & compare to the original value - if not equal or unordered (NaN) then jump.
        convertInt32ToDouble(dest, fpTemp);
        failureCases.append(branchDouble(DoubleNotEqualOrUnordered, fpTemp, src));
    }

    Jump branchDoubleNonZero(FPRegisterID reg, FPRegisterID scratch)
    {
#if WTF_MIPS_ISA_REV(2) && WTF_MIPS_FP64
        m_assembler.mtc1(MIPSRegisters::zero, scratch);
        m_assembler.mthc1(MIPSRegisters::zero, scratch);
#else
        m_assembler.mtc1(MIPSRegisters::zero, scratch);
        m_assembler.mtc1(MIPSRegisters::zero, FPRegisterID(scratch + 1));
#endif
        return branchDouble(DoubleNotEqual, reg, scratch);
    }

    Jump branchDoubleZeroOrNaN(FPRegisterID reg, FPRegisterID scratch)
    {
#if WTF_MIPS_ISA_REV(2) && WTF_MIPS_FP64
        m_assembler.mtc1(MIPSRegisters::zero, scratch);
        m_assembler.mthc1(MIPSRegisters::zero, scratch);
#else
        m_assembler.mtc1(MIPSRegisters::zero, scratch);
        m_assembler.mtc1(MIPSRegisters::zero, FPRegisterID(scratch + 1));
#endif
        return branchDouble(DoubleEqualOrUnordered, reg, scratch);
    }


private:
    // If m_fixedWidth is true, we will generate a fixed number of instructions.
    // Otherwise, we can emit any number of instructions.
    bool m_fixedWidth;

    friend class LinkBuffer;
    friend class RepatchBuffer;

    static void linkCall(void* code, Call call, FunctionPtr function)
    {
        MIPSAssembler::linkCall(code, call.m_jmp, function.value());
    }

    static void repatchCall(CodeLocationCall call, CodeLocationLabel destination)
    {
        MIPSAssembler::relinkCall(call.dataLocation(), destination.executableAddress());
    }

    static void repatchCall(CodeLocationCall call, FunctionPtr destination)
    {
        MIPSAssembler::relinkCall(call.dataLocation(), destination.executableAddress());
    }

};

}

#endif // ENABLE(ASSEMBLER) && CPU(MIPS)

#endif // MacroAssemblerMIPS_h
