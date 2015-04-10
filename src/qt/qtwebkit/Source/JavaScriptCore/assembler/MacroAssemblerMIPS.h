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

#include "AbstractMacroAssembler.h"
#include "MIPSAssembler.h"

namespace JSC {

class MacroAssemblerMIPS : public AbstractMacroAssembler<MIPSAssembler> {
public:
    typedef MIPSRegisters::FPRegisterID FPRegisterID;

    MacroAssemblerMIPS()
        : m_fixedWidth(false)
    {
    }

    static bool isCompactPtrAlignedAddressOffset(ptrdiff_t value)
    {
        return value >= -2147483647 - 1 && value <= 2147483647;
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

    static const int MaximumCompactPtrAlignedAddressOffset = 0x7FFFFFFF;

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
        PositiveOrZero,
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

    void add32(RegisterID op1, RegisterID op2, RegisterID dest)
    {
        m_assembler.addu(dest, op1, op2);
    }

    void add32(TrustedImm32 imm, RegisterID dest)
    {
        add32(imm, dest, dest);
    }

    void add32(TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        if (imm.m_value >= -32768 && imm.m_value <= 32767
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

    void add32(RegisterID src, TrustedImm32 imm, RegisterID dest)
    {
        add32(imm, src, dest);
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
            if (imm.m_value >= -32768 && imm.m_value <= 32767
                && !m_fixedWidth)
                m_assembler.addiu(dataTempRegister, dataTempRegister, imm.m_value);
            else {
                move(imm, immTempRegister);
                m_assembler.addu(dataTempRegister, dataTempRegister, immTempRegister);
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
                m_assembler.addiu(dataTempRegister, dataTempRegister, imm.m_value);
            else {
                move(imm, immTempRegister);
                m_assembler.addu(dataTempRegister, dataTempRegister, immTempRegister);
            }
            m_assembler.sw(dataTempRegister, addrTempRegister, address.offset);
        }
    }

    void add32(Address src, RegisterID dest)
    {
        load32(src, dataTempRegister);
        add32(dataTempRegister, dest);
    }

    void add32(AbsoluteAddress src, RegisterID dest)
    {
        load32(src.m_ptr, dataTempRegister);
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
           lw   cmpTemp, 0(addrTemp)
           addu dataTemp, cmpTemp, immTemp
           sw   dataTemp, 0(addrTemp)
        */
        move(TrustedImmPtr(address.m_ptr), addrTempRegister);
        m_assembler.lw(cmpTempRegister, addrTempRegister, 0);
        if (imm.m_value >= -32768 && imm.m_value <= 32767 && !m_fixedWidth)
            m_assembler.addiu(dataTempRegister, cmpTempRegister, imm.m_value);
        else {
            move(imm, immTempRegister);
            m_assembler.addu(dataTempRegister, cmpTempRegister, immTempRegister);
        }
        m_assembler.sw(dataTempRegister, addrTempRegister, 0);
    }

    void add64(TrustedImm32 imm, AbsoluteAddress address)
    {
        /*
            add32(imm, address)
            sltu  immTemp, dataTemp, cmpTemp    # set carry-in bit
            lw    dataTemp, 4(addrTemp)
            addiu dataTemp, imm.m_value >> 31 ? -1 : 0
            addu  dataTemp, dataTemp, immTemp
            sw    dataTemp, 4(addrTemp)
        */
        add32(imm, address);
        m_assembler.sltu(immTempRegister, dataTempRegister, cmpTempRegister);
        m_assembler.lw(dataTempRegister, addrTempRegister, 4);
        if (imm.m_value >> 31)
            m_assembler.addiu(dataTempRegister, dataTempRegister, -1);
        m_assembler.addu(dataTempRegister, dataTempRegister, immTempRegister);
        m_assembler.sw(dataTempRegister, addrTempRegister, 4);
    }

    void and32(Address src, RegisterID dest)
    {
        load32(src, dataTempRegister);
        and32(dataTempRegister, dest);
    }

    void and32(RegisterID src, RegisterID dest)
    {
        m_assembler.andInsn(dest, dest, src);
    }

    void and32(RegisterID op1, RegisterID op2, RegisterID dest)
    {
        m_assembler.andInsn(dest, op1, op2);
    }

    void and32(TrustedImm32 imm, RegisterID dest)
    {
        if (!imm.m_value && !m_fixedWidth)
            move(MIPSRegisters::zero, dest);
        else if (imm.m_value > 0 && imm.m_value < 65535 && !m_fixedWidth)
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

    void and32(TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        if (!imm.m_value && !m_fixedWidth)
            move(MIPSRegisters::zero, dest);
        else if (imm.m_value > 0 && imm.m_value < 65535 && !m_fixedWidth)
            m_assembler.andi(dest, src, imm.m_value);
        else {
            move(imm, immTempRegister);
            m_assembler.andInsn(dest, src, immTempRegister);
        }
    }

    void lshift32(RegisterID shiftAmount, RegisterID dest)
    {
        m_assembler.sllv(dest, dest, shiftAmount);
    }

    void lshift32(RegisterID src, RegisterID shiftAmount, RegisterID dest)
    {
        m_assembler.sllv(dest, src, shiftAmount);
    }

    void lshift32(TrustedImm32 imm, RegisterID dest)
    {
        move(imm, immTempRegister);
        m_assembler.sllv(dest, dest, immTempRegister);
    }

    void lshift32(RegisterID src, TrustedImm32 imm, RegisterID dest)
    {
        move(imm, immTempRegister);
        m_assembler.sllv(dest, src, immTempRegister);
    }

    void mul32(RegisterID src, RegisterID dest)
    {
        m_assembler.mul(dest, dest, src);
    }

    void mul32(RegisterID op1, RegisterID op2, RegisterID dest)
    {
        m_assembler.mul(dest, op1, op2);
    }

    void mul32(TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        if (!imm.m_value && !m_fixedWidth)
            move(MIPSRegisters::zero, dest);
        else if (imm.m_value == 1 && !m_fixedWidth)
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

    void or32(RegisterID src, RegisterID dest)
    {
        m_assembler.orInsn(dest, dest, src);
    }

    void or32(RegisterID op1, RegisterID op2, RegisterID dest)
    {
        m_assembler.orInsn(dest, op1, op2);
    }

    void or32(TrustedImm32 imm, RegisterID dest)
    {
        if (!imm.m_value && !m_fixedWidth)
            return;

        if (imm.m_value > 0 && imm.m_value < 65535
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

    void or32(TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        if (!imm.m_value && !m_fixedWidth) {
            move(src, dest);
            return;
        }

        if (imm.m_value > 0 && imm.m_value < 65535 && !m_fixedWidth) {
            m_assembler.ori(dest, src, imm.m_value);
            return;
        }

        /*
            li      dataTemp, imm
            or      dest, src, dataTemp
        */
        move(imm, dataTempRegister);
        m_assembler.orInsn(dest, src, dataTempRegister);
    }

    void or32(RegisterID src, AbsoluteAddress dest)
    {
        load32(dest.m_ptr, dataTempRegister);
        m_assembler.orInsn(dataTempRegister, dataTempRegister, src);
        store32(dataTempRegister, dest.m_ptr);
    }

    void rshift32(RegisterID shiftAmount, RegisterID dest)
    {
        m_assembler.srav(dest, dest, shiftAmount);
    }

    void rshift32(RegisterID src, RegisterID shiftAmount, RegisterID dest)
    {
        m_assembler.srav(dest, src, shiftAmount);
    }

    void rshift32(TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.sra(dest, dest, imm.m_value);
    }

    void rshift32(RegisterID src, TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.sra(dest, src, imm.m_value);
    }

    void urshift32(RegisterID shiftAmount, RegisterID dest)
    {
        m_assembler.srlv(dest, dest, shiftAmount);
    }

    void urshift32(RegisterID src, RegisterID shiftAmount, RegisterID dest)
    {
        m_assembler.srlv(dest, src, shiftAmount);
    }

    void urshift32(TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.srl(dest, dest, imm.m_value);
    }

    void urshift32(RegisterID src, TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.srl(dest, src, imm.m_value);
    }

    void sub32(RegisterID src, RegisterID dest)
    {
        m_assembler.subu(dest, dest, src);
    }

    void sub32(RegisterID op1, RegisterID op2, RegisterID dest)
    {
        m_assembler.subu(dest, op1, op2);
    }

    void sub32(TrustedImm32 imm, RegisterID dest)
    {
        if (imm.m_value >= -32767 && imm.m_value <= 32768
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

    void sub32(RegisterID src, TrustedImm32 imm, RegisterID dest)
    {
        if (imm.m_value >= -32767 && imm.m_value <= 32768
            && !m_fixedWidth) {
            /*
              addiu     dest, src, imm
            */
            m_assembler.addiu(dest, src, -imm.m_value);
        } else {
            /*
              li        immTemp, imm
              subu      dest, src, immTemp
            */
            move(imm, immTempRegister);
            m_assembler.subu(dest, src, immTempRegister);
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
            if (imm.m_value >= -32767 && imm.m_value <= 32768 && !m_fixedWidth)
                m_assembler.addiu(dataTempRegister, dataTempRegister, -imm.m_value);
            else {
                move(imm, immTempRegister);
                m_assembler.subu(dataTempRegister, dataTempRegister, immTempRegister);
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

            if (imm.m_value >= -32767 && imm.m_value <= 32768
                && !m_fixedWidth)
                m_assembler.addiu(dataTempRegister, dataTempRegister, -imm.m_value);
            else {
                move(imm, immTempRegister);
                m_assembler.subu(dataTempRegister, dataTempRegister, immTempRegister);
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

        if (imm.m_value >= -32767 && imm.m_value <= 32768 && !m_fixedWidth)
            m_assembler.addiu(dataTempRegister, dataTempRegister, -imm.m_value);
        else {
            move(imm, immTempRegister);
            m_assembler.subu(dataTempRegister, dataTempRegister, immTempRegister);
        }
        m_assembler.sw(dataTempRegister, addrTempRegister, 0);
    }

    void xor32(RegisterID src, RegisterID dest)
    {
        m_assembler.xorInsn(dest, dest, src);
    }

    void xor32(RegisterID op1, RegisterID op2, RegisterID dest)
    {
        m_assembler.xorInsn(dest, op1, op2);
    }

    void xor32(TrustedImm32 imm, RegisterID dest)
    {
        if (imm.m_value == -1) {
            m_assembler.nor(dest, dest, MIPSRegisters::zero);
            return;
        }

        /*
            li  immTemp, imm
            xor dest, dest, immTemp
        */
        move(imm, immTempRegister);
        m_assembler.xorInsn(dest, dest, immTempRegister);
    }

    void xor32(TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        if (imm.m_value == -1) {
            m_assembler.nor(dest, src, MIPSRegisters::zero);
            return;
        }

        /*
            li  immTemp, imm
            xor dest, dest, immTemp
        */
        move(imm, immTempRegister);
        m_assembler.xorInsn(dest, src, immTempRegister);
    }

    void sqrtDouble(FPRegisterID src, FPRegisterID dst)
    {
        m_assembler.sqrtd(dst, src);
    }
    
    void absDouble(FPRegisterID src, FPRegisterID dst)
    {
        m_assembler.absd(dst, src);
    }

    ConvertibleLoadLabel convertibleLoadPtr(Address address, RegisterID dest)
    {
        ConvertibleLoadLabel result(this);
        /*
            lui     addrTemp, (offset + 0x8000) >> 16
            addu    addrTemp, addrTemp, base
            lw      dest, (offset & 0xffff)(addrTemp)
        */
        m_assembler.lui(addrTempRegister, (address.offset + 0x8000) >> 16);
        m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
        m_assembler.lw(dest, addrTempRegister, address.offset);
        return result;
    }

    // Memory access operations:
    //
    // Loads are of the form load(address, destination) and stores of the form
    // store(source, address). The source for a store may be an TrustedImm32. Address
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

    void load8(BaseIndex address, RegisterID dest)
    {
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth) {
            /*
             sll     addrTemp, address.index, address.scale
             addu    addrTemp, addrTemp, address.base
             lbu     dest, address.offset(addrTemp)
             */
            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lbu(dest, addrTempRegister, address.offset);
        } else {
            /*
             sll     addrTemp, address.index, address.scale
             addu    addrTemp, addrTemp, address.base
             lui     immTemp, (address.offset + 0x8000) >> 16
             addu    addrTemp, addrTemp, immTemp
             lbu     dest, (address.offset & 0xffff)(at)
             */
            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lui(immTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister, immTempRegister);
            m_assembler.lbu(dest, addrTempRegister, address.offset);
        }
    }

    void load8Signed(BaseIndex address, RegisterID dest)
    {
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth) {
            /*
                sll     addrTemp, address.index, address.scale
                addu    addrTemp, addrTemp, address.base
                lb      dest, address.offset(addrTemp)
            */
            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lb(dest, addrTempRegister, address.offset);
        } else {
            /*
                sll     addrTemp, address.index, address.scale
                addu    addrTemp, addrTemp, address.base
                lui     immTemp, (address.offset + 0x8000) >> 16
                addu    addrTemp, addrTemp, immTemp
                lb     dest, (address.offset & 0xffff)(at)
            */
            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lui(immTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister, immTempRegister);
            m_assembler.lb(dest, addrTempRegister, address.offset);
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
            m_assembler.addu(addrTempRegister, addrTempRegister, immTempRegister);
            m_assembler.lw(dest, addrTempRegister, address.offset);
        }
    }

    void load16Unaligned(BaseIndex address, RegisterID dest)
    {
        load16(address, dest);
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
            m_assembler.addu(addrTempRegister, addrTempRegister, immTempRegister);
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
    
    DataLabelCompact load32WithCompactAddressOffsetPatch(Address address, RegisterID dest)
    {
        DataLabelCompact dataLabel(this);
        load32WithAddressOffsetPatch(address, dest);
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
            m_assembler.addu(addrTempRegister, addrTempRegister, immTempRegister);
            m_assembler.lhu(dest, addrTempRegister, address.offset);
        }
    }

    void load16Signed(BaseIndex address, RegisterID dest)
    {
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth) {
            /*
                sll     addrTemp, address.index, address.scale
                addu    addrTemp, addrTemp, address.base
                lh     dest, address.offset(addrTemp)
            */
            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lh(dest, addrTempRegister, address.offset);
        } else {
            /*
                sll     addrTemp, address.index, address.scale
                addu    addrTemp, addrTemp, address.base
                lui     immTemp, (address.offset + 0x8000) >> 16
                addu    addrTemp, addrTemp, immTemp
                lh     dest, (address.offset & 0xffff)(addrTemp)
            */
            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lui(immTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister, immTempRegister);
            m_assembler.lh(dest, addrTempRegister, address.offset);
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

    void store8(RegisterID src, BaseIndex address)
    {
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth) {
            /*
                sll     addrTemp, address.index, address.scale
                addu    addrTemp, addrTemp, address.base
                sb      src, address.offset(addrTemp)
            */
            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.sb(src, addrTempRegister, address.offset);
        } else {
            /*
                sll     addrTemp, address.index, address.scale
                addu    addrTemp, addrTemp, address.base
                lui     immTemp, (address.offset + 0x8000) >> 16
                addu    addrTemp, addrTemp, immTemp
                sb      src, (address.offset & 0xffff)(at)
            */
            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lui(immTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister, immTempRegister);
            m_assembler.sb(src, addrTempRegister, address.offset);
        }
    }

    void store8(TrustedImm32 imm, void* address)
    {
        /*
            li  immTemp, imm
            li  addrTemp, address
            sb  src, 0(addrTemp)
        */
        if (!imm.m_value && !m_fixedWidth) {
            move(TrustedImmPtr(address), addrTempRegister);
            m_assembler.sb(MIPSRegisters::zero, addrTempRegister, 0);
        } else {
            move(imm, immTempRegister);
            move(TrustedImmPtr(address), addrTempRegister);
            m_assembler.sb(immTempRegister, addrTempRegister, 0);
        }
    }

    void store16(RegisterID src, BaseIndex address)
    {
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth) {
            /*
                sll     addrTemp, address.index, address.scale
                addu    addrTemp, addrTemp, address.base
                sh      src, address.offset(addrTemp)
            */
            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.sh(src, addrTempRegister, address.offset);
        } else {
            /*
                sll     addrTemp, address.index, address.scale
                addu    addrTemp, addrTemp, address.base
                lui     immTemp, (address.offset + 0x8000) >> 16
                addu    addrTemp, addrTemp, immTemp
                sh      src, (address.offset & 0xffff)(at)
            */
            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lui(immTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister, immTempRegister);
            m_assembler.sh(src, addrTempRegister, address.offset);
        }
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
            m_assembler.addu(addrTempRegister, addrTempRegister, immTempRegister);
            m_assembler.sw(src, addrTempRegister, address.offset);
        }
    }

    void store32(TrustedImm32 imm, ImplicitAddress address)
    {
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth) {
            if (!imm.m_value)
                m_assembler.sw(MIPSRegisters::zero, address.base, address.offset);
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
            if (!imm.m_value && !m_fixedWidth)
                m_assembler.sw(MIPSRegisters::zero, addrTempRegister, address.offset);
            else {
                move(imm, immTempRegister);
                m_assembler.sw(immTempRegister, addrTempRegister, address.offset);
            }
        }
    }

    void store32(TrustedImm32 imm, BaseIndex address)
    {
        if (address.offset >= -32768 && address.offset <= 32767 && !m_fixedWidth) {
            /*
                sll     addrTemp, address.index, address.scale
                addu    addrTemp, addrTemp, address.base
                sw      src, address.offset(addrTemp)
            */
            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            if (!imm.m_value)
                m_assembler.sw(MIPSRegisters::zero, addrTempRegister, address.offset);
            else {
                move(imm, immTempRegister);
                m_assembler.sw(immTempRegister, addrTempRegister, address.offset);
            }
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
            m_assembler.addu(addrTempRegister, addrTempRegister, immTempRegister);
            if (!imm.m_value && !m_fixedWidth)
                m_assembler.sw(MIPSRegisters::zero, addrTempRegister, address.offset);
            else {
                move(imm, immTempRegister);
                m_assembler.sw(immTempRegister, addrTempRegister, address.offset);
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
        if (!imm.m_value && !m_fixedWidth) {
            move(TrustedImmPtr(address), addrTempRegister);
            m_assembler.sw(MIPSRegisters::zero, addrTempRegister, 0);
        } else {
            move(imm, immTempRegister);
            move(TrustedImmPtr(address), addrTempRegister);
            m_assembler.sw(immTempRegister, addrTempRegister, 0);
        }
    }

    // Floating-point operations:

    static bool supportsFloatingPoint()
    {
#if WTF_MIPS_DOUBLE_FLOAT
        return true;
#else
        return false;
#endif
    }

    static bool supportsFloatingPointTruncate()
    {
#if WTF_MIPS_DOUBLE_FLOAT && WTF_MIPS_ISA_AT_LEAST(2)
        return true;
#else
        return false;
#endif
    }

    static bool supportsFloatingPointSqrt()
    {
#if WTF_MIPS_DOUBLE_FLOAT && WTF_MIPS_ISA_AT_LEAST(2)
        return true;
#else
        return false;
#endif
    }

    static bool supportsFloatingPointAbs()
    {
#if WTF_MIPS_DOUBLE_FLOAT
        return true;
#else
        return false;
#endif
    }

    // Stack manipulation operations:
    //
    // The ABI is assumed to provide a stack abstraction to memory,
    // containing machine word sized units of data. Push and pop
    // operations add and remove a single register sized unit of data
    // to or from the stack. Peek and poke operations read or write
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
        if (!imm.m_value && !m_fixedWidth)
            move(MIPSRegisters::zero, dest);
        else if (m_fixedWidth) {
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

    void compare8(RelationalCondition cond, Address left, TrustedImm32 right, RegisterID dest)
    {
        // Make sure the immediate value is unsigned 8 bits.
        ASSERT(!(right.m_value & 0xFFFFFF00));
        load8(left, dataTempRegister);
        move(right, immTempRegister);
        compare32(cond, dataTempRegister, immTempRegister, dest);
    }

    Jump branch8(RelationalCondition cond, BaseIndex left, TrustedImm32 right)
    {
        ASSERT(!(right.m_value & 0xFFFFFF00));
        load8(left, dataTempRegister);
        // Be careful that the previous load8() uses immTempRegister.
        // So, we need to put move() after load8().
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

    Jump branchTest8(ResultCondition cond, AbsoluteAddress address, TrustedImm32 mask = TrustedImm32(-1))
    {
        move(TrustedImmPtr(address.m_ptr), dataTempRegister);
        load8(Address(dataTempRegister), dataTempRegister);
        return branchTest32(cond, dataTempRegister, mask);
    }

    Jump jump()
    {
        return branchEqual(MIPSRegisters::zero, MIPSRegisters::zero);
    }

    void jump(RegisterID target)
    {
        move(target, MIPSRegisters::t9);
        m_assembler.jr(MIPSRegisters::t9);
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

    void jump(AbsoluteAddress address)
    {
        m_fixedWidth = true;
        load32(address.m_ptr, MIPSRegisters::t9);
        m_assembler.jr(MIPSRegisters::t9);
        m_assembler.nop();
        m_fixedWidth = false;
    }

    void moveDoubleToInts(FPRegisterID src, RegisterID dest1, RegisterID dest2)
    {
        m_assembler.vmov(dest1, dest2, src);
    }

    void moveIntsToDouble(RegisterID src1, RegisterID src2, FPRegisterID dest, FPRegisterID scratch)
    {
        UNUSED_PARAM(scratch);
        m_assembler.vmov(dest, src1, src2);
    }

    // Arithmetic control flow operations:
    //
    // This set of conditional branch operations branch based
    // on the result of an arithmetic operation. The operation
    // is performed as normal, storing the result.
    //
    // * jz operations branch if the result is zero.
    // * jo operations branch if the (signed) arithmetic
    //   operation caused an overflow to occur.

    Jump branchAdd32(ResultCondition cond, RegisterID src, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == PositiveOrZero) || (cond == Zero) || (cond == NonZero));
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
        if (cond == PositiveOrZero) {
            add32(src, dest);
            // Check if dest is not negative.
            m_assembler.slt(cmpTempRegister, dest, MIPSRegisters::zero);
            return branchEqual(cmpTempRegister, MIPSRegisters::zero);
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

    Jump branchAdd32(ResultCondition cond, RegisterID op1, RegisterID op2, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == PositiveOrZero) || (cond == Zero) || (cond == NonZero));
        if (cond == Overflow) {
            /*
                move    dataTemp, op1
                xor     cmpTemp, dataTemp, op2
                bltz    cmpTemp, No_overflow    # diff sign bit -> no overflow
                addu    dest, dataTemp, op2
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
            move(op1, dataTempRegister);
            m_assembler.xorInsn(cmpTempRegister, dataTempRegister, op2);
            m_assembler.bltz(cmpTempRegister, 10);
            m_assembler.addu(dest, dataTempRegister, op2);
            m_assembler.xorInsn(cmpTempRegister, dest, dataTempRegister);
            m_assembler.bgez(cmpTempRegister, 7);
            m_assembler.nop();
            return jump();
        }
        if (cond == Signed) {
            add32(op1, op2, dest);
            // Check if dest is negative.
            m_assembler.slt(cmpTempRegister, dest, MIPSRegisters::zero);
            return branchNotEqual(cmpTempRegister, MIPSRegisters::zero);
        }
        if (cond == PositiveOrZero) {
            add32(op1, op2, dest);
            // Check if dest is not negative.
            m_assembler.slt(cmpTempRegister, dest, MIPSRegisters::zero);
            return branchEqual(cmpTempRegister, MIPSRegisters::zero);
        }
        if (cond == Zero) {
            add32(op1, op2, dest);
            return branchEqual(dest, MIPSRegisters::zero);
        }
        if (cond == NonZero) {
            add32(op1, op2, dest);
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

    Jump branchAdd32(ResultCondition cond, RegisterID src, TrustedImm32 imm, RegisterID dest)
    {
        move(imm, immTempRegister);
        move(src, dest);
        return branchAdd32(cond, immTempRegister, dest);
    }

    Jump branchAdd32(ResultCondition cond, TrustedImm32 imm, AbsoluteAddress dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == PositiveOrZero) || (cond == Zero) || (cond == NonZero));
        if (cond == Overflow) {
            /*
                move    dataTemp, dest
                xori    cmpTemp, dataTemp, imm
                bltz    cmpTemp, No_overflow    # diff sign bit -> no overflow
                addiu   dataTemp, dataTemp, imm
                move    dest, dataTemp
                xori    cmpTemp, dataTemp, imm
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
            if (imm.m_value >= -32768 && imm.m_value  <= 32767 && !m_fixedWidth) {
                load32(dest.m_ptr, dataTempRegister);
                m_assembler.xori(cmpTempRegister, dataTempRegister, imm.m_value);
                m_assembler.bltz(cmpTempRegister, 10);
                m_assembler.addiu(dataTempRegister, dataTempRegister, imm.m_value);
                store32(dataTempRegister, dest.m_ptr);
                m_assembler.xori(cmpTempRegister, dataTempRegister, imm.m_value);
                m_assembler.bgez(cmpTempRegister, 7);
                m_assembler.nop();
            } else {
                load32(dest.m_ptr, dataTempRegister);
                move(imm, immTempRegister);
                m_assembler.xorInsn(cmpTempRegister, dataTempRegister, immTempRegister);
                m_assembler.bltz(cmpTempRegister, 10);
                m_assembler.addiu(dataTempRegister, dataTempRegister, immTempRegister);
                store32(dataTempRegister, dest.m_ptr);
                m_assembler.xori(cmpTempRegister, dataTempRegister, immTempRegister);
                m_assembler.bgez(cmpTempRegister, 7);
                m_assembler.nop();
            }
            return jump();
        }
        move(imm, immTempRegister);
        load32(dest.m_ptr, dataTempRegister);
        add32(immTempRegister, dataTempRegister);
        store32(dataTempRegister, dest.m_ptr);
        if (cond == Signed) {
            // Check if dest is negative.
            m_assembler.slt(cmpTempRegister, dataTempRegister, MIPSRegisters::zero);
            return branchNotEqual(cmpTempRegister, MIPSRegisters::zero);
        }
        if (cond == PositiveOrZero) {
            // Check if dest is not negative.
            m_assembler.slt(cmpTempRegister, dataTempRegister, MIPSRegisters::zero);
            return branchEqual(cmpTempRegister, MIPSRegisters::zero);
        }
        if (cond == Zero)
            return branchEqual(dataTempRegister, MIPSRegisters::zero);
        if (cond == NonZero)
            return branchNotEqual(dataTempRegister, MIPSRegisters::zero);
        ASSERT(0);
        return Jump();
    }

    Jump branchMul32(ResultCondition cond, RegisterID src1, RegisterID src2, RegisterID dest)
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
            m_assembler.mult(src1, src2);
            m_assembler.mfhi(dataTempRegister);
            m_assembler.mflo(dest);
            m_assembler.sra(addrTempRegister, dest, 31);
            m_assembler.beq(dataTempRegister, addrTempRegister, 7);
            m_assembler.nop();
            return jump();
        }
        if (cond == Signed) {
            mul32(src1, src2, dest);
            // Check if dest is negative.
            m_assembler.slt(cmpTempRegister, dest, MIPSRegisters::zero);
            return branchNotEqual(cmpTempRegister, MIPSRegisters::zero);
        }
        if (cond == Zero) {
            mul32(src1, src2, dest);
            return branchEqual(dest, MIPSRegisters::zero);
        }
        if (cond == NonZero) {
            mul32(src1, src2, dest);
            return branchNotEqual(dest, MIPSRegisters::zero);
        }
        ASSERT(0);
        return Jump();
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
        return branchMul32(cond, immTempRegister, src, dest);
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

    Jump branchSub32(ResultCondition cond, RegisterID src, TrustedImm32 imm, RegisterID dest)
    {
        move(imm, immTempRegister);
        return branchSub32(cond, src, immTempRegister, dest);
    }

    Jump branchSub32(ResultCondition cond, RegisterID op1, RegisterID op2, RegisterID dest)
    {
        ASSERT((cond == Overflow) || (cond == Signed) || (cond == Zero) || (cond == NonZero));
        if (cond == Overflow) {
            /*
                move    dataTemp, op1
                xor     cmpTemp, dataTemp, op2
                bgez    cmpTemp, No_overflow    # same sign bit -> no overflow
                subu    dest, dataTemp, op2
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
            move(op1, dataTempRegister);
            m_assembler.xorInsn(cmpTempRegister, dataTempRegister, op2);
            m_assembler.bgez(cmpTempRegister, 10);
            m_assembler.subu(dest, dataTempRegister, op2);
            m_assembler.xorInsn(cmpTempRegister, dest, dataTempRegister);
            m_assembler.bgez(cmpTempRegister, 7);
            m_assembler.nop();
            return jump();
        }
        if (cond == Signed) {
            sub32(op1, op2, dest);
            // Check if dest is negative.
            m_assembler.slt(cmpTempRegister, dest, MIPSRegisters::zero);
            return branchNotEqual(cmpTempRegister, MIPSRegisters::zero);
        }
        if (cond == Zero) {
            sub32(op1, op2, dest);
            return branchEqual(dest, MIPSRegisters::zero);
        }
        if (cond == NonZero) {
            sub32(op1, op2, dest);
            return branchNotEqual(dest, MIPSRegisters::zero);
        }
        ASSERT(0);
        return Jump();
    }

    Jump branchNeg32(ResultCondition cond, RegisterID srcDest)
    {
        m_assembler.li(dataTempRegister, -1);
        return branchMul32(cond, dataTempRegister, srcDest);
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
        /* We need two words for relaxation. */
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
        move(target, MIPSRegisters::t9);
        m_assembler.jalr(MIPSRegisters::t9);
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
            m_assembler.andInsn(cmpTempRegister, dataTempRegister, immTempRegister);
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
            m_assembler.andInsn(cmpTempRegister, dataTempRegister, immTempRegister);
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

    void loadFloat(BaseIndex address, FPRegisterID dest)
    {
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth) {
            /*
                sll     addrTemp, address.index, address.scale
                addu    addrTemp, addrTemp, address.base
                lwc1    dest, address.offset(addrTemp)
            */
            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lwc1(dest, addrTempRegister, address.offset);
        } else {
            /*
                sll     addrTemp, address.index, address.scale
                addu    addrTemp, addrTemp, address.base
                lui     immTemp, (address.offset + 0x8000) >> 16
                addu    addrTemp, addrTemp, immTemp
                lwc1    dest, (address.offset & 0xffff)(at)
            */
            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lui(immTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister, immTempRegister);
            m_assembler.lwc1(dest, addrTempRegister, address.offset);
        }
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

    void loadDouble(BaseIndex address, FPRegisterID dest)
    {
#if WTF_MIPS_ISA(1)
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth) {
            /*
                sll     addrTemp, address.index, address.scale
                addu    addrTemp, addrTemp, address.base
                lwc1    dest, address.offset(addrTemp)
                lwc1    dest+1, (address.offset+4)(addrTemp)
            */
            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lwc1(dest, addrTempRegister, address.offset);
            m_assembler.lwc1(FPRegisterID(dest + 1), addrTempRegister, address.offset + 4);
        } else {
            /*
                sll     addrTemp, address.index, address.scale
                addu    addrTemp, addrTemp, address.base
                lui     immTemp, (address.offset + 0x8000) >> 16
                addu    addrTemp, addrTemp, immTemp
                lwc1    dest, (address.offset & 0xffff)(at)
                lwc1    dest+1, (address.offset & 0xffff + 4)(at)
            */
            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lui(immTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister, immTempRegister);
            m_assembler.lwc1(dest, addrTempRegister, address.offset);
            m_assembler.lwc1(FPRegisterID(dest + 1), addrTempRegister, address.offset + 4);
        }
#else
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth) {
            /*
                sll     addrTemp, address.index, address.scale
                addu    addrTemp, addrTemp, address.base
                ldc1    dest, address.offset(addrTemp)
            */
            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.ldc1(dest, addrTempRegister, address.offset);
        } else {
            /*
                sll     addrTemp, address.index, address.scale
                addu    addrTemp, addrTemp, address.base
                lui     immTemp, (address.offset + 0x8000) >> 16
                addu    addrTemp, addrTemp, immTemp
                ldc1    dest, (address.offset & 0xffff)(at)
            */
            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lui(immTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister, immTempRegister);
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

    void storeFloat(FPRegisterID src, BaseIndex address)
    {
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth) {
            /*
                sll     addrTemp, address.index, address.scale
                addu    addrTemp, addrTemp, address.base
                swc1    src, address.offset(addrTemp)
            */
            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.swc1(src, addrTempRegister, address.offset);
        } else {
            /*
                sll     addrTemp, address.index, address.scale
                addu    addrTemp, addrTemp, address.base
                lui     immTemp, (address.offset + 0x8000) >> 16
                addu    addrTemp, addrTemp, immTemp
                swc1    src, (address.offset & 0xffff)(at)
            */
            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lui(immTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister, immTempRegister);
            m_assembler.swc1(src, addrTempRegister, address.offset);
        }
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

    void storeDouble(FPRegisterID src, BaseIndex address)
    {
#if WTF_MIPS_ISA(1)
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth) {
            /*
                sll     addrTemp, address.index, address.scale
                addu    addrTemp, addrTemp, address.base
                swc1    src, address.offset(addrTemp)
                swc1    src+1, (address.offset + 4)(addrTemp)
            */
            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.swc1(src, addrTempRegister, address.offset);
            m_assembler.swc1(FPRegisterID(src + 1), addrTempRegister, address.offset + 4);
        } else {
            /*
                sll     addrTemp, address.index, address.scale
                addu    addrTemp, addrTemp, address.base
                lui     immTemp, (address.offset + 0x8000) >> 16
                addu    addrTemp, addrTemp, immTemp
                swc1    src, (address.offset & 0xffff)(at)
                swc1    src+1, (address.offset & 0xffff + 4)(at)
            */
            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lui(immTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister, immTempRegister);
            m_assembler.swc1(src, addrTempRegister, address.offset);
            m_assembler.swc1(FPRegisterID(src + 1), addrTempRegister, address.offset + 4);
        }
#else
        if (address.offset >= -32768 && address.offset <= 32767
            && !m_fixedWidth) {
            /*
                sll     addrTemp, address.index, address.scale
                addu    addrTemp, addrTemp, address.base
                sdc1    src, address.offset(addrTemp)
            */
            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.sdc1(src, addrTempRegister, address.offset);
        } else {
            /*
                sll     addrTemp, address.index, address.scale
                addu    addrTemp, addrTemp, address.base
                lui     immTemp, (address.offset + 0x8000) >> 16
                addu    addrTemp, addrTemp, immTemp
                sdc1    src, (address.offset & 0xffff)(at)
            */
            m_assembler.sll(addrTempRegister, address.index, address.scale);
            m_assembler.addu(addrTempRegister, addrTempRegister, address.base);
            m_assembler.lui(immTempRegister, (address.offset + 0x8000) >> 16);
            m_assembler.addu(addrTempRegister, addrTempRegister, immTempRegister);
            m_assembler.sdc1(src, addrTempRegister, address.offset);
        }
#endif
    }

    void storeDouble(FPRegisterID src, const void* address)
    {
#if WTF_MIPS_ISA(1)
        move(TrustedImmPtr(address), addrTempRegister);
        m_assembler.swc1(src, addrTempRegister, 0);
        m_assembler.swc1(FPRegisterID(src + 1), addrTempRegister, 4);
#else
        move(TrustedImmPtr(address), addrTempRegister);
        m_assembler.sdc1(src, addrTempRegister, 0);
#endif
    }

    void moveDouble(FPRegisterID src, FPRegisterID dest)
    {
        if (src != dest || m_fixedWidth)
            m_assembler.movd(dest, src);
    }

    void swapDouble(FPRegisterID fr1, FPRegisterID fr2)
    {
        moveDouble(fr1, fpTempRegister);
        moveDouble(fr2, fr1);
        moveDouble(fpTempRegister, fr2);
    }

    void addDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.addd(dest, dest, src);
    }

    void addDouble(FPRegisterID op1, FPRegisterID op2, FPRegisterID dest)
    {
        m_assembler.addd(dest, op1, op2);
    }

    void addDouble(Address src, FPRegisterID dest)
    {
        loadDouble(src, fpTempRegister);
        m_assembler.addd(dest, dest, fpTempRegister);
    }

    void addDouble(AbsoluteAddress address, FPRegisterID dest)
    {
        loadDouble(address.m_ptr, fpTempRegister);
        m_assembler.addd(dest, dest, fpTempRegister);
    }

    void subDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.subd(dest, dest, src);
    }

    void subDouble(FPRegisterID op1, FPRegisterID op2, FPRegisterID dest)
    {
        m_assembler.subd(dest, op1, op2);
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

    void mulDouble(FPRegisterID op1, FPRegisterID op2, FPRegisterID dest)
    {
        m_assembler.muld(dest, op1, op2);
    }

    void divDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.divd(dest, dest, src);
    }

    void divDouble(FPRegisterID op1, FPRegisterID op2, FPRegisterID dest)
    {
        m_assembler.divd(dest, op1, op2);
    }

    void divDouble(Address src, FPRegisterID dest)
    {
        loadDouble(src, fpTempRegister);
        m_assembler.divd(dest, dest, fpTempRegister);
    }

    void negateDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.negd(dest, src);
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

    void convertFloatToDouble(FPRegisterID src, FPRegisterID dst)
    {
        m_assembler.cvtds(dst, src);
    }

    void convertDoubleToFloat(FPRegisterID src, FPRegisterID dst)
    {
        m_assembler.cvtsd(dst, src);
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
        m_assembler.nop();
        m_assembler.nop();
        m_assembler.appendJump();
        m_assembler.beq(rs, rt, 0);
        m_assembler.nop();
        insertRelaxationWords();
        return Jump(m_assembler.label());
    }

    Jump branchNotEqual(RegisterID rs, RegisterID rt)
    {
        m_assembler.nop();
        m_assembler.nop();
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
    enum BranchTruncateType { BranchIfTruncateFailed, BranchIfTruncateSuccessful };
    Jump branchTruncateDoubleToInt32(FPRegisterID src, RegisterID dest, BranchTruncateType branchType = BranchIfTruncateFailed)
    {
        m_assembler.truncwd(fpTempRegister, src);
        m_assembler.mfc1(dest, fpTempRegister);
        return branch32(branchType == BranchIfTruncateFailed ? Equal : NotEqual, dest, TrustedImm32(0x7fffffff));
    }

    Jump branchTruncateDoubleToUint32(FPRegisterID src, RegisterID dest, BranchTruncateType branchType = BranchIfTruncateFailed)
    {
        m_assembler.truncwd(fpTempRegister, src);
        m_assembler.mfc1(dest, fpTempRegister);
        return branch32(branchType == BranchIfTruncateFailed ? Equal : NotEqual, dest, TrustedImm32(0x7fffffff));
    }

    // Result is undefined if the value is outside of the integer range.
    void truncateDoubleToInt32(FPRegisterID src, RegisterID dest)
    {
        m_assembler.truncwd(fpTempRegister, src);
        m_assembler.mfc1(dest, fpTempRegister);
    }

    // Result is undefined if src > 2^31
    void truncateDoubleToUint32(FPRegisterID src, RegisterID dest)
    {
        m_assembler.truncwd(fpTempRegister, src);
        m_assembler.mfc1(dest, fpTempRegister);
    }

    // Convert 'src' to an integer, and places the resulting 'dest'.
    // If the result is not representable as a 32 bit value, branch.
    // May also branch for some values that are representable in 32 bits
    // (specifically, in this case, 0).
    void branchConvertDoubleToInt32(FPRegisterID src, RegisterID dest, JumpList& failureCases, FPRegisterID fpTemp, bool negZeroCheck = true)
    {
        m_assembler.cvtwd(fpTempRegister, src);
        m_assembler.mfc1(dest, fpTempRegister);

        // If the result is zero, it might have been -0.0, and the double comparison won't catch this!
        if (negZeroCheck)
            failureCases.append(branch32(Equal, dest, MIPSRegisters::zero));

        // Convert the integer result back to float & compare to the original value - if not equal or unordered (NaN) then jump.
        convertInt32ToDouble(dest, fpTemp);
        failureCases.append(branchDouble(DoubleNotEqualOrUnordered, fpTemp, src));
    }

    Jump branchDoubleNonZero(FPRegisterID reg, FPRegisterID scratch)
    {
        m_assembler.vmov(scratch, MIPSRegisters::zero, MIPSRegisters::zero);
        return branchDouble(DoubleNotEqual, reg, scratch);
    }

    Jump branchDoubleZeroOrNaN(FPRegisterID reg, FPRegisterID scratch)
    {
        m_assembler.vmov(scratch, MIPSRegisters::zero, MIPSRegisters::zero);
        return branchDouble(DoubleEqualOrUnordered, reg, scratch);
    }

    // Invert a relational condition, e.g. == becomes !=, < becomes >=, etc.
    static RelationalCondition invert(RelationalCondition cond)
    {
        RelationalCondition r;
        if (cond == Equal)
            r = NotEqual;
        else if (cond == NotEqual)
            r = Equal;
        else if (cond == Above)
            r = BelowOrEqual;
        else if (cond == AboveOrEqual)
            r = Below;
        else if (cond == Below)
            r = AboveOrEqual;
        else if (cond == BelowOrEqual)
            r = Above;
        else if (cond == GreaterThan)
            r = LessThanOrEqual;
        else if (cond == GreaterThanOrEqual)
            r = LessThan;
        else if (cond == LessThan)
            r = GreaterThanOrEqual;
        else if (cond == LessThanOrEqual)
            r = GreaterThan;
        return r;
    }

    void nop()
    {
        m_assembler.nop();
    }

    static FunctionPtr readCallTarget(CodeLocationCall call)
    {
        return FunctionPtr(reinterpret_cast<void(*)()>(MIPSAssembler::readCallTarget(call.dataLocation())));
    }

    static void replaceWithJump(CodeLocationLabel instructionStart, CodeLocationLabel destination)
    {
        MIPSAssembler::replaceWithJump(instructionStart.dataLocation(), destination.dataLocation());
    }
    
    static ptrdiff_t maxJumpReplacementSize()
    {
        MIPSAssembler::maxJumpReplacementSize();
        return 0;
    }

    static bool canJumpReplacePatchableBranchPtrWithPatch() { return false; }

    static CodeLocationLabel startOfBranchPtrWithPatchOnRegister(CodeLocationDataLabelPtr label)
    {
        return label.labelAtOffset(0);
    }

    static void revertJumpReplacementToBranchPtrWithPatch(CodeLocationLabel instructionStart, RegisterID, void* initialValue)
    {
        MIPSAssembler::revertJumpToMove(instructionStart.dataLocation(), immTempRegister, reinterpret_cast<int>(initialValue) & 0xffff);
    }

    static CodeLocationLabel startOfPatchableBranchPtrWithPatchOnAddress(CodeLocationDataLabelPtr)
    {
        UNREACHABLE_FOR_PLATFORM();
        return CodeLocationLabel();
    }

    static void revertJumpReplacementToPatchableBranchPtrWithPatch(CodeLocationLabel instructionStart, Address, void* initialValue)
    {
        UNREACHABLE_FOR_PLATFORM();
    }


private:
    // If m_fixedWidth is true, we will generate a fixed number of instructions.
    // Otherwise, we can emit any number of instructions.
    bool m_fixedWidth;

    friend class LinkBuffer;
    friend class RepatchBuffer;

    static void linkCall(void* code, Call call, FunctionPtr function)
    {
        MIPSAssembler::linkCall(code, call.m_label, function.value());
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
