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

#ifndef MacroAssemblerARMv7_h
#define MacroAssemblerARMv7_h

#if ENABLE(ASSEMBLER)

#include "ARMv7Assembler.h"
#include "AbstractMacroAssembler.h"

namespace JSC {

class MacroAssemblerARMv7 : public AbstractMacroAssembler<ARMv7Assembler> {
    // FIXME: switch dataTempRegister & addressTempRegister, or possibly use r7?
    //        - dTR is likely used more than aTR, and we'll get better instruction
    //        encoding if it's in the low 8 registers.
    static const RegisterID dataTempRegister = ARMRegisters::ip;
    static const RegisterID addressTempRegister = ARMRegisters::r3;

    static const ARMRegisters::FPDoubleRegisterID fpTempRegister = ARMRegisters::d7;
    inline ARMRegisters::FPSingleRegisterID fpTempRegisterAsSingle() { return ARMRegisters::asSingle(fpTempRegister); }

public:
    MacroAssemblerARMv7()
        : m_makeJumpPatchable(false)
    {
    }

    typedef ARMv7Assembler::LinkRecord LinkRecord;
    typedef ARMv7Assembler::JumpType JumpType;
    typedef ARMv7Assembler::JumpLinkType JumpLinkType;

    static bool isCompactPtrAlignedAddressOffset(ptrdiff_t value)
    {
        return value >= -255 && value <= 255;
    }

    Vector<LinkRecord, 0, UnsafeVectorOverflow>& jumpsToLink() { return m_assembler.jumpsToLink(); }
    void* unlinkedCode() { return m_assembler.unlinkedCode(); }
    bool canCompact(JumpType jumpType) { return m_assembler.canCompact(jumpType); }
    JumpLinkType computeJumpType(JumpType jumpType, const uint8_t* from, const uint8_t* to) { return m_assembler.computeJumpType(jumpType, from, to); }
    JumpLinkType computeJumpType(LinkRecord& record, const uint8_t* from, const uint8_t* to) { return m_assembler.computeJumpType(record, from, to); }
    void recordLinkOffsets(int32_t regionStart, int32_t regionEnd, int32_t offset) {return m_assembler.recordLinkOffsets(regionStart, regionEnd, offset); }
    int jumpSizeDelta(JumpType jumpType, JumpLinkType jumpLinkType) { return m_assembler.jumpSizeDelta(jumpType, jumpLinkType); }
    void link(LinkRecord& record, uint8_t* from, uint8_t* to) { return m_assembler.link(record, from, to); }

    struct ArmAddress {
        enum AddressType {
            HasOffset,
            HasIndex,
        } type;
        RegisterID base;
        union {
            int32_t offset;
            struct {
                RegisterID index;
                Scale scale;
            };
        } u;
        
        explicit ArmAddress(RegisterID base, int32_t offset = 0)
            : type(HasOffset)
            , base(base)
        {
            u.offset = offset;
        }
        
        explicit ArmAddress(RegisterID base, RegisterID index, Scale scale = TimesOne)
            : type(HasIndex)
            , base(base)
        {
            u.index = index;
            u.scale = scale;
        }
    };
    
public:
    typedef ARMRegisters::FPDoubleRegisterID FPRegisterID;

    static const Scale ScalePtr = TimesFour;

    enum RelationalCondition {
        Equal = ARMv7Assembler::ConditionEQ,
        NotEqual = ARMv7Assembler::ConditionNE,
        Above = ARMv7Assembler::ConditionHI,
        AboveOrEqual = ARMv7Assembler::ConditionHS,
        Below = ARMv7Assembler::ConditionLO,
        BelowOrEqual = ARMv7Assembler::ConditionLS,
        GreaterThan = ARMv7Assembler::ConditionGT,
        GreaterThanOrEqual = ARMv7Assembler::ConditionGE,
        LessThan = ARMv7Assembler::ConditionLT,
        LessThanOrEqual = ARMv7Assembler::ConditionLE
    };

    enum ResultCondition {
        Overflow = ARMv7Assembler::ConditionVS,
        Signed = ARMv7Assembler::ConditionMI,
        PositiveOrZero = ARMv7Assembler::ConditionPL,
        Zero = ARMv7Assembler::ConditionEQ,
        NonZero = ARMv7Assembler::ConditionNE
    };

    enum DoubleCondition {
        // These conditions will only evaluate to true if the comparison is ordered - i.e. neither operand is NaN.
        DoubleEqual = ARMv7Assembler::ConditionEQ,
        DoubleNotEqual = ARMv7Assembler::ConditionVC, // Not the right flag! check for this & handle differently.
        DoubleGreaterThan = ARMv7Assembler::ConditionGT,
        DoubleGreaterThanOrEqual = ARMv7Assembler::ConditionGE,
        DoubleLessThan = ARMv7Assembler::ConditionLO,
        DoubleLessThanOrEqual = ARMv7Assembler::ConditionLS,
        // If either operand is NaN, these conditions always evaluate to true.
        DoubleEqualOrUnordered = ARMv7Assembler::ConditionVS, // Not the right flag! check for this & handle differently.
        DoubleNotEqualOrUnordered = ARMv7Assembler::ConditionNE,
        DoubleGreaterThanOrUnordered = ARMv7Assembler::ConditionHI,
        DoubleGreaterThanOrEqualOrUnordered = ARMv7Assembler::ConditionHS,
        DoubleLessThanOrUnordered = ARMv7Assembler::ConditionLT,
        DoubleLessThanOrEqualOrUnordered = ARMv7Assembler::ConditionLE,
    };

    static const RegisterID stackPointerRegister = ARMRegisters::sp;
    static const RegisterID linkRegister = ARMRegisters::lr;

    // Integer arithmetic operations:
    //
    // Operations are typically two operand - operation(source, srcDst)
    // For many operations the source may be an TrustedImm32, the srcDst operand
    // may often be a memory location (explictly described using an Address
    // object).

    void add32(RegisterID src, RegisterID dest)
    {
        m_assembler.add(dest, dest, src);
    }

    void add32(TrustedImm32 imm, RegisterID dest)
    {
        add32(imm, dest, dest);
    }
    
    void add32(AbsoluteAddress src, RegisterID dest)
    {
        load32(src.m_ptr, dataTempRegister);
        add32(dataTempRegister, dest);
    }

    void add32(TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        ARMThumbImmediate armImm = ARMThumbImmediate::makeUInt12OrEncodedImm(imm.m_value);
        if (armImm.isValid())
            m_assembler.add(dest, src, armImm);
        else {
            move(imm, dataTempRegister);
            m_assembler.add(dest, src, dataTempRegister);
        }
    }

    void add32(TrustedImm32 imm, Address address)
    {
        load32(address, dataTempRegister);

        ARMThumbImmediate armImm = ARMThumbImmediate::makeUInt12OrEncodedImm(imm.m_value);
        if (armImm.isValid())
            m_assembler.add(dataTempRegister, dataTempRegister, armImm);
        else {
            // Hrrrm, since dataTempRegister holds the data loaded,
            // use addressTempRegister to hold the immediate.
            move(imm, addressTempRegister);
            m_assembler.add(dataTempRegister, dataTempRegister, addressTempRegister);
        }

        store32(dataTempRegister, address);
    }

    void add32(Address src, RegisterID dest)
    {
        load32(src, dataTempRegister);
        add32(dataTempRegister, dest);
    }

    void add32(TrustedImm32 imm, AbsoluteAddress address)
    {
        load32(address.m_ptr, dataTempRegister);

        ARMThumbImmediate armImm = ARMThumbImmediate::makeUInt12OrEncodedImm(imm.m_value);
        if (armImm.isValid())
            m_assembler.add(dataTempRegister, dataTempRegister, armImm);
        else {
            // Hrrrm, since dataTempRegister holds the data loaded,
            // use addressTempRegister to hold the immediate.
            move(imm, addressTempRegister);
            m_assembler.add(dataTempRegister, dataTempRegister, addressTempRegister);
        }

        store32(dataTempRegister, address.m_ptr);
    }

    void add64(TrustedImm32 imm, AbsoluteAddress address)
    {
        move(TrustedImmPtr(address.m_ptr), addressTempRegister);

        m_assembler.ldr(dataTempRegister, addressTempRegister, ARMThumbImmediate::makeUInt12(0));
        ARMThumbImmediate armImm = ARMThumbImmediate::makeEncodedImm(imm.m_value);
        if (armImm.isValid())
            m_assembler.add_S(dataTempRegister, dataTempRegister, armImm);
        else {
            move(imm, addressTempRegister);
            m_assembler.add_S(dataTempRegister, dataTempRegister, addressTempRegister);
            move(TrustedImmPtr(address.m_ptr), addressTempRegister);
        }
        m_assembler.str(dataTempRegister, addressTempRegister, ARMThumbImmediate::makeUInt12(0));

        m_assembler.ldr(dataTempRegister, addressTempRegister, ARMThumbImmediate::makeUInt12(4));
        m_assembler.adc(dataTempRegister, dataTempRegister, ARMThumbImmediate::makeEncodedImm(imm.m_value >> 31));
        m_assembler.str(dataTempRegister, addressTempRegister, ARMThumbImmediate::makeUInt12(4));
    }

    void and32(RegisterID op1, RegisterID op2, RegisterID dest)
    {
        m_assembler.ARM_and(dest, op1, op2);
    }

    void and32(TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        ARMThumbImmediate armImm = ARMThumbImmediate::makeEncodedImm(imm.m_value);
        if (armImm.isValid())
            m_assembler.ARM_and(dest, src, armImm);
        else {
            move(imm, dataTempRegister);
            m_assembler.ARM_and(dest, src, dataTempRegister);
        }
    }

    void and32(RegisterID src, RegisterID dest)
    {
        and32(dest, src, dest);
    }

    void and32(TrustedImm32 imm, RegisterID dest)
    {
        and32(imm, dest, dest);
    }

    void and32(Address src, RegisterID dest)
    {
        load32(src, dataTempRegister);
        and32(dataTempRegister, dest);
    }

    void countLeadingZeros32(RegisterID src, RegisterID dest)
    {
        m_assembler.clz(dest, src);
    }

    void lshift32(RegisterID src, RegisterID shiftAmount, RegisterID dest)
    {
        // Clamp the shift to the range 0..31
        ARMThumbImmediate armImm = ARMThumbImmediate::makeEncodedImm(0x1f);
        ASSERT(armImm.isValid());
        m_assembler.ARM_and(dataTempRegister, shiftAmount, armImm);

        m_assembler.lsl(dest, src, dataTempRegister);
    }

    void lshift32(RegisterID src, TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.lsl(dest, src, imm.m_value & 0x1f);
    }

    void lshift32(RegisterID shiftAmount, RegisterID dest)
    {
        lshift32(dest, shiftAmount, dest);
    }

    void lshift32(TrustedImm32 imm, RegisterID dest)
    {
        lshift32(dest, imm, dest);
    }

    void mul32(RegisterID src, RegisterID dest)
    {
        m_assembler.smull(dest, dataTempRegister, dest, src);
    }

    void mul32(TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        move(imm, dataTempRegister);
        m_assembler.smull(dest, dataTempRegister, src, dataTempRegister);
    }

    void neg32(RegisterID srcDest)
    {
        m_assembler.neg(srcDest, srcDest);
    }

    void or32(RegisterID src, RegisterID dest)
    {
        m_assembler.orr(dest, dest, src);
    }
    
    void or32(RegisterID src, AbsoluteAddress dest)
    {
        move(TrustedImmPtr(dest.m_ptr), addressTempRegister);
        load32(addressTempRegister, dataTempRegister);
        or32(src, dataTempRegister);
        store32(dataTempRegister, addressTempRegister);
    }

    void or32(TrustedImm32 imm, RegisterID dest)
    {
        or32(imm, dest, dest);
    }

    void or32(RegisterID op1, RegisterID op2, RegisterID dest)
    {
        m_assembler.orr(dest, op1, op2);
    }

    void or32(TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        ARMThumbImmediate armImm = ARMThumbImmediate::makeEncodedImm(imm.m_value);
        if (armImm.isValid())
            m_assembler.orr(dest, src, armImm);
        else {
            move(imm, dataTempRegister);
            m_assembler.orr(dest, src, dataTempRegister);
        }
    }

    void rshift32(RegisterID src, RegisterID shiftAmount, RegisterID dest)
    {
        // Clamp the shift to the range 0..31
        ARMThumbImmediate armImm = ARMThumbImmediate::makeEncodedImm(0x1f);
        ASSERT(armImm.isValid());
        m_assembler.ARM_and(dataTempRegister, shiftAmount, armImm);

        m_assembler.asr(dest, src, dataTempRegister);
    }

    void rshift32(RegisterID src, TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.asr(dest, src, imm.m_value & 0x1f);
    }

    void rshift32(RegisterID shiftAmount, RegisterID dest)
    {
        rshift32(dest, shiftAmount, dest);
    }
    
    void rshift32(TrustedImm32 imm, RegisterID dest)
    {
        rshift32(dest, imm, dest);
    }

    void urshift32(RegisterID src, RegisterID shiftAmount, RegisterID dest)
    {
        // Clamp the shift to the range 0..31
        ARMThumbImmediate armImm = ARMThumbImmediate::makeEncodedImm(0x1f);
        ASSERT(armImm.isValid());
        m_assembler.ARM_and(dataTempRegister, shiftAmount, armImm);
        
        m_assembler.lsr(dest, src, dataTempRegister);
    }
    
    void urshift32(RegisterID src, TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.lsr(dest, src, imm.m_value & 0x1f);
    }

    void urshift32(RegisterID shiftAmount, RegisterID dest)
    {
        urshift32(dest, shiftAmount, dest);
    }
    
    void urshift32(TrustedImm32 imm, RegisterID dest)
    {
        urshift32(dest, imm, dest);
    }

    void sub32(RegisterID src, RegisterID dest)
    {
        m_assembler.sub(dest, dest, src);
    }

    void sub32(TrustedImm32 imm, RegisterID dest)
    {
        ARMThumbImmediate armImm = ARMThumbImmediate::makeUInt12OrEncodedImm(imm.m_value);
        if (armImm.isValid())
            m_assembler.sub(dest, dest, armImm);
        else {
            move(imm, dataTempRegister);
            m_assembler.sub(dest, dest, dataTempRegister);
        }
    }

    void sub32(TrustedImm32 imm, Address address)
    {
        load32(address, dataTempRegister);

        ARMThumbImmediate armImm = ARMThumbImmediate::makeUInt12OrEncodedImm(imm.m_value);
        if (armImm.isValid())
            m_assembler.sub(dataTempRegister, dataTempRegister, armImm);
        else {
            // Hrrrm, since dataTempRegister holds the data loaded,
            // use addressTempRegister to hold the immediate.
            move(imm, addressTempRegister);
            m_assembler.sub(dataTempRegister, dataTempRegister, addressTempRegister);
        }

        store32(dataTempRegister, address);
    }

    void sub32(Address src, RegisterID dest)
    {
        load32(src, dataTempRegister);
        sub32(dataTempRegister, dest);
    }

    void sub32(TrustedImm32 imm, AbsoluteAddress address)
    {
        load32(address.m_ptr, dataTempRegister);

        ARMThumbImmediate armImm = ARMThumbImmediate::makeUInt12OrEncodedImm(imm.m_value);
        if (armImm.isValid())
            m_assembler.sub(dataTempRegister, dataTempRegister, armImm);
        else {
            // Hrrrm, since dataTempRegister holds the data loaded,
            // use addressTempRegister to hold the immediate.
            move(imm, addressTempRegister);
            m_assembler.sub(dataTempRegister, dataTempRegister, addressTempRegister);
        }

        store32(dataTempRegister, address.m_ptr);
    }

    void xor32(RegisterID op1, RegisterID op2, RegisterID dest)
    {
        m_assembler.eor(dest, op1, op2);
    }

    void xor32(TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        if (imm.m_value == -1) {
            m_assembler.mvn(dest, src);
            return;
        }

        ARMThumbImmediate armImm = ARMThumbImmediate::makeEncodedImm(imm.m_value);
        if (armImm.isValid())
            m_assembler.eor(dest, src, armImm);
        else {
            move(imm, dataTempRegister);
            m_assembler.eor(dest, src, dataTempRegister);
        }
    }

    void xor32(RegisterID src, RegisterID dest)
    {
        xor32(dest, src, dest);
    }

    void xor32(TrustedImm32 imm, RegisterID dest)
    {
        if (imm.m_value == -1)
            m_assembler.mvn(dest, dest);
        else
            xor32(imm, dest, dest);
    }
    

    // Memory access operations:
    //
    // Loads are of the form load(address, destination) and stores of the form
    // store(source, address).  The source for a store may be an TrustedImm32.  Address
    // operand objects to loads and store will be implicitly constructed if a
    // register is passed.

private:
    void load32(ArmAddress address, RegisterID dest)
    {
        if (address.type == ArmAddress::HasIndex)
            m_assembler.ldr(dest, address.base, address.u.index, address.u.scale);
        else if (address.u.offset >= 0) {
            ARMThumbImmediate armImm = ARMThumbImmediate::makeUInt12(address.u.offset);
            ASSERT(armImm.isValid());
            m_assembler.ldr(dest, address.base, armImm);
        } else {
            ASSERT(address.u.offset >= -255);
            m_assembler.ldr(dest, address.base, address.u.offset, true, false);
        }
    }

    void load16(ArmAddress address, RegisterID dest)
    {
        if (address.type == ArmAddress::HasIndex)
            m_assembler.ldrh(dest, address.base, address.u.index, address.u.scale);
        else if (address.u.offset >= 0) {
            ARMThumbImmediate armImm = ARMThumbImmediate::makeUInt12(address.u.offset);
            ASSERT(armImm.isValid());
            m_assembler.ldrh(dest, address.base, armImm);
        } else {
            ASSERT(address.u.offset >= -255);
            m_assembler.ldrh(dest, address.base, address.u.offset, true, false);
        }
    }
    
    void load16Signed(ArmAddress address, RegisterID dest)
    {
        ASSERT(address.type == ArmAddress::HasIndex);
        m_assembler.ldrsh(dest, address.base, address.u.index, address.u.scale);
    }

    void load8(ArmAddress address, RegisterID dest)
    {
        if (address.type == ArmAddress::HasIndex)
            m_assembler.ldrb(dest, address.base, address.u.index, address.u.scale);
        else if (address.u.offset >= 0) {
            ARMThumbImmediate armImm = ARMThumbImmediate::makeUInt12(address.u.offset);
            ASSERT(armImm.isValid());
            m_assembler.ldrb(dest, address.base, armImm);
        } else {
            ASSERT(address.u.offset >= -255);
            m_assembler.ldrb(dest, address.base, address.u.offset, true, false);
        }
    }
    
    void load8Signed(ArmAddress address, RegisterID dest)
    {
        ASSERT(address.type == ArmAddress::HasIndex);
        m_assembler.ldrsb(dest, address.base, address.u.index, address.u.scale);
    }

protected:
    void store32(RegisterID src, ArmAddress address)
    {
        if (address.type == ArmAddress::HasIndex)
            m_assembler.str(src, address.base, address.u.index, address.u.scale);
        else if (address.u.offset >= 0) {
            ARMThumbImmediate armImm = ARMThumbImmediate::makeUInt12(address.u.offset);
            ASSERT(armImm.isValid());
            m_assembler.str(src, address.base, armImm);
        } else {
            ASSERT(address.u.offset >= -255);
            m_assembler.str(src, address.base, address.u.offset, true, false);
        }
    }

private:
    void store8(RegisterID src, ArmAddress address)
    {
        if (address.type == ArmAddress::HasIndex)
            m_assembler.strb(src, address.base, address.u.index, address.u.scale);
        else if (address.u.offset >= 0) {
            ARMThumbImmediate armImm = ARMThumbImmediate::makeUInt12(address.u.offset);
            ASSERT(armImm.isValid());
            m_assembler.strb(src, address.base, armImm);
        } else {
            ASSERT(address.u.offset >= -255);
            m_assembler.strb(src, address.base, address.u.offset, true, false);
        }
    }
    
    void store16(RegisterID src, ArmAddress address)
    {
        if (address.type == ArmAddress::HasIndex)
            m_assembler.strh(src, address.base, address.u.index, address.u.scale);
        else if (address.u.offset >= 0) {
            ARMThumbImmediate armImm = ARMThumbImmediate::makeUInt12(address.u.offset);
            ASSERT(armImm.isValid());
            m_assembler.strh(src, address.base, armImm);
        } else {
            ASSERT(address.u.offset >= -255);
            m_assembler.strh(src, address.base, address.u.offset, true, false);
        }
    }

public:
    void load32(ImplicitAddress address, RegisterID dest)
    {
        load32(setupArmAddress(address), dest);
    }

    void load32(BaseIndex address, RegisterID dest)
    {
        load32(setupArmAddress(address), dest);
    }

    void load32WithUnalignedHalfWords(BaseIndex address, RegisterID dest)
    {
        load32(setupArmAddress(address), dest);
    }

    void load16Unaligned(BaseIndex address, RegisterID dest)
    {
        load16(setupArmAddress(address), dest);
    }

    void load32(const void* address, RegisterID dest)
    {
        move(TrustedImmPtr(address), addressTempRegister);
        m_assembler.ldr(dest, addressTempRegister, ARMThumbImmediate::makeUInt16(0));
    }
    
    ConvertibleLoadLabel convertibleLoadPtr(Address address, RegisterID dest)
    {
        ConvertibleLoadLabel result(this);
        ASSERT(address.offset >= 0 && address.offset <= 255);
        m_assembler.ldrWide8BitImmediate(dest, address.base, address.offset);
        return result;
    }

    void load8(ImplicitAddress address, RegisterID dest)
    {
        load8(setupArmAddress(address), dest);
    }

    void load8Signed(ImplicitAddress, RegisterID)
    {
        UNREACHABLE_FOR_PLATFORM();
    }

    void load8(BaseIndex address, RegisterID dest)
    {
        load8(setupArmAddress(address), dest);
    }
    
    void load8Signed(BaseIndex address, RegisterID dest)
    {
        load8Signed(setupArmAddress(address), dest);
    }

    DataLabel32 load32WithAddressOffsetPatch(Address address, RegisterID dest)
    {
        DataLabel32 label = moveWithPatch(TrustedImm32(address.offset), dataTempRegister);
        load32(ArmAddress(address.base, dataTempRegister), dest);
        return label;
    }
    
    DataLabelCompact load32WithCompactAddressOffsetPatch(Address address, RegisterID dest)
    {
        padBeforePatch();

        RegisterID base = address.base;
        
        DataLabelCompact label(this);
        ASSERT(isCompactPtrAlignedAddressOffset(address.offset));

        m_assembler.ldr(dest, base, address.offset, true, false);
        return label;
    }

    void load16(BaseIndex address, RegisterID dest)
    {
        m_assembler.ldrh(dest, makeBaseIndexBase(address), address.index, address.scale);
    }
    
    void load16Signed(BaseIndex address, RegisterID dest)
    {
        load16Signed(setupArmAddress(address), dest);
    }
    
    void load16(ImplicitAddress address, RegisterID dest)
    {
        ARMThumbImmediate armImm = ARMThumbImmediate::makeUInt12(address.offset);
        if (armImm.isValid())
            m_assembler.ldrh(dest, address.base, armImm);
        else {
            move(TrustedImm32(address.offset), dataTempRegister);
            m_assembler.ldrh(dest, address.base, dataTempRegister);
        }
    }
    
    void load16Signed(ImplicitAddress, RegisterID)
    {
        UNREACHABLE_FOR_PLATFORM();
    }

    DataLabel32 store32WithAddressOffsetPatch(RegisterID src, Address address)
    {
        DataLabel32 label = moveWithPatch(TrustedImm32(address.offset), dataTempRegister);
        store32(src, ArmAddress(address.base, dataTempRegister));
        return label;
    }

    void store32(RegisterID src, ImplicitAddress address)
    {
        store32(src, setupArmAddress(address));
    }

    void store32(RegisterID src, BaseIndex address)
    {
        store32(src, setupArmAddress(address));
    }

    void store32(TrustedImm32 imm, ImplicitAddress address)
    {
        move(imm, dataTempRegister);
        store32(dataTempRegister, setupArmAddress(address));
    }

    void store32(TrustedImm32 imm, BaseIndex address)
    {
        move(imm, dataTempRegister);
        store32(dataTempRegister, setupArmAddress(address));
    }

    void store32(RegisterID src, const void* address)
    {
        move(TrustedImmPtr(address), addressTempRegister);
        m_assembler.str(src, addressTempRegister, ARMThumbImmediate::makeUInt16(0));
    }

    void store32(TrustedImm32 imm, const void* address)
    {
        move(imm, dataTempRegister);
        store32(dataTempRegister, address);
    }

    void store8(RegisterID src, BaseIndex address)
    {
        store8(src, setupArmAddress(address));
    }
    
    void store8(RegisterID src, void* address)
    {
        move(TrustedImmPtr(address), addressTempRegister);
        store8(src, ArmAddress(addressTempRegister, 0));
    }
    
    void store8(TrustedImm32 imm, void* address)
    {
        move(imm, dataTempRegister);
        store8(dataTempRegister, address);
    }
    
    void store16(RegisterID src, BaseIndex address)
    {
        store16(src, setupArmAddress(address));
    }

    // Possibly clobbers src, but not on this architecture.
    void moveDoubleToInts(FPRegisterID src, RegisterID dest1, RegisterID dest2)
    {
        m_assembler.vmov(dest1, dest2, src);
    }
    
    void moveIntsToDouble(RegisterID src1, RegisterID src2, FPRegisterID dest, FPRegisterID scratch)
    {
        UNUSED_PARAM(scratch);
        m_assembler.vmov(dest, src1, src2);
    }

#if ENABLE(JIT_CONSTANT_BLINDING)
    static bool shouldBlindForSpecificArch(uint32_t value)
    {
        ARMThumbImmediate immediate = ARMThumbImmediate::makeEncodedImm(value);

        // Couldn't be encoded as an immediate, so assume it's untrusted.
        if (!immediate.isValid())
            return true;
        
        // If we can encode the immediate, we have less than 16 attacker
        // controlled bits.
        if (immediate.isEncodedImm())
            return false;

        // Don't let any more than 12 bits of an instruction word
        // be controlled by an attacker.
        return !immediate.isUInt12();
    }
#endif

    // Floating-point operations:

    static bool supportsFloatingPoint() { return true; }
    static bool supportsFloatingPointTruncate() { return true; }
    static bool supportsFloatingPointSqrt() { return true; }
    static bool supportsFloatingPointAbs() { return true; }

    void loadDouble(ImplicitAddress address, FPRegisterID dest)
    {
        RegisterID base = address.base;
        int32_t offset = address.offset;

        // Arm vfp addresses can be offset by a 9-bit ones-comp immediate, left shifted by 2.
        if ((offset & 3) || (offset > (255 * 4)) || (offset < -(255 * 4))) {
            add32(TrustedImm32(offset), base, addressTempRegister);
            base = addressTempRegister;
            offset = 0;
        }
        
        m_assembler.vldr(dest, base, offset);
    }

    void loadFloat(ImplicitAddress address, FPRegisterID dest)
    {
        RegisterID base = address.base;
        int32_t offset = address.offset;

        // Arm vfp addresses can be offset by a 9-bit ones-comp immediate, left shifted by 2.
        if ((offset & 3) || (offset > (255 * 4)) || (offset < -(255 * 4))) {
            add32(TrustedImm32(offset), base, addressTempRegister);
            base = addressTempRegister;
            offset = 0;
        }
        
        m_assembler.flds(ARMRegisters::asSingle(dest), base, offset);
    }

    void loadDouble(BaseIndex address, FPRegisterID dest)
    {
        move(address.index, addressTempRegister);
        lshift32(TrustedImm32(address.scale), addressTempRegister);
        add32(address.base, addressTempRegister);
        loadDouble(Address(addressTempRegister, address.offset), dest);
    }
    
    void loadFloat(BaseIndex address, FPRegisterID dest)
    {
        move(address.index, addressTempRegister);
        lshift32(TrustedImm32(address.scale), addressTempRegister);
        add32(address.base, addressTempRegister);
        loadFloat(Address(addressTempRegister, address.offset), dest);
    }

    void moveDouble(FPRegisterID src, FPRegisterID dest)
    {
        if (src != dest)
            m_assembler.vmov(dest, src);
    }

    void loadDouble(const void* address, FPRegisterID dest)
    {
        move(TrustedImmPtr(address), addressTempRegister);
        m_assembler.vldr(dest, addressTempRegister, 0);
    }

    void storeDouble(FPRegisterID src, ImplicitAddress address)
    {
        RegisterID base = address.base;
        int32_t offset = address.offset;

        // Arm vfp addresses can be offset by a 9-bit ones-comp immediate, left shifted by 2.
        if ((offset & 3) || (offset > (255 * 4)) || (offset < -(255 * 4))) {
            add32(TrustedImm32(offset), base, addressTempRegister);
            base = addressTempRegister;
            offset = 0;
        }
        
        m_assembler.vstr(src, base, offset);
    }

    void storeFloat(FPRegisterID src, ImplicitAddress address)
    {
        RegisterID base = address.base;
        int32_t offset = address.offset;

        // Arm vfp addresses can be offset by a 9-bit ones-comp immediate, left shifted by 2.
        if ((offset & 3) || (offset > (255 * 4)) || (offset < -(255 * 4))) {
            add32(TrustedImm32(offset), base, addressTempRegister);
            base = addressTempRegister;
            offset = 0;
        }
        
        m_assembler.fsts(ARMRegisters::asSingle(src), base, offset);
    }

    void storeDouble(FPRegisterID src, const void* address)
    {
        move(TrustedImmPtr(address), addressTempRegister);
        storeDouble(src, addressTempRegister);
    }

    void storeDouble(FPRegisterID src, BaseIndex address)
    {
        move(address.index, addressTempRegister);
        lshift32(TrustedImm32(address.scale), addressTempRegister);
        add32(address.base, addressTempRegister);
        storeDouble(src, Address(addressTempRegister, address.offset));
    }
    
    void storeFloat(FPRegisterID src, BaseIndex address)
    {
        move(address.index, addressTempRegister);
        lshift32(TrustedImm32(address.scale), addressTempRegister);
        add32(address.base, addressTempRegister);
        storeFloat(src, Address(addressTempRegister, address.offset));
    }
    
    void addDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.vadd(dest, dest, src);
    }

    void addDouble(Address src, FPRegisterID dest)
    {
        loadDouble(src, fpTempRegister);
        addDouble(fpTempRegister, dest);
    }

    void addDouble(FPRegisterID op1, FPRegisterID op2, FPRegisterID dest)
    {
        m_assembler.vadd(dest, op1, op2);
    }

    void addDouble(AbsoluteAddress address, FPRegisterID dest)
    {
        loadDouble(address.m_ptr, fpTempRegister);
        m_assembler.vadd(dest, dest, fpTempRegister);
    }

    void divDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.vdiv(dest, dest, src);
    }

    void divDouble(FPRegisterID op1, FPRegisterID op2, FPRegisterID dest)
    {
        m_assembler.vdiv(dest, op1, op2);
    }

    void subDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.vsub(dest, dest, src);
    }

    void subDouble(Address src, FPRegisterID dest)
    {
        loadDouble(src, fpTempRegister);
        subDouble(fpTempRegister, dest);
    }

    void subDouble(FPRegisterID op1, FPRegisterID op2, FPRegisterID dest)
    {
        m_assembler.vsub(dest, op1, op2);
    }

    void mulDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.vmul(dest, dest, src);
    }

    void mulDouble(Address src, FPRegisterID dest)
    {
        loadDouble(src, fpTempRegister);
        mulDouble(fpTempRegister, dest);
    }

    void mulDouble(FPRegisterID op1, FPRegisterID op2, FPRegisterID dest)
    {
        m_assembler.vmul(dest, op1, op2);
    }

    void sqrtDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.vsqrt(dest, src);
    }
    
    void absDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.vabs(dest, src);
    }

    void negateDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.vneg(dest, src);
    }

    void convertInt32ToDouble(RegisterID src, FPRegisterID dest)
    {
        m_assembler.vmov(fpTempRegister, src, src);
        m_assembler.vcvt_signedToFloatingPoint(dest, fpTempRegisterAsSingle());
    }

    void convertInt32ToDouble(Address address, FPRegisterID dest)
    {
        // Fixme: load directly into the fpr!
        load32(address, dataTempRegister);
        m_assembler.vmov(fpTempRegister, dataTempRegister, dataTempRegister);
        m_assembler.vcvt_signedToFloatingPoint(dest, fpTempRegisterAsSingle());
    }

    void convertInt32ToDouble(AbsoluteAddress address, FPRegisterID dest)
    {
        // Fixme: load directly into the fpr!
        load32(address.m_ptr, dataTempRegister);
        m_assembler.vmov(fpTempRegister, dataTempRegister, dataTempRegister);
        m_assembler.vcvt_signedToFloatingPoint(dest, fpTempRegisterAsSingle());
    }
    
    void convertFloatToDouble(FPRegisterID src, FPRegisterID dst)
    {
        m_assembler.vcvtds(dst, ARMRegisters::asSingle(src));
    }
    
    void convertDoubleToFloat(FPRegisterID src, FPRegisterID dst)
    {
        m_assembler.vcvtsd(ARMRegisters::asSingle(dst), src);
    }

    Jump branchDouble(DoubleCondition cond, FPRegisterID left, FPRegisterID right)
    {
        m_assembler.vcmp(left, right);
        m_assembler.vmrs();

        if (cond == DoubleNotEqual) {
            // ConditionNE jumps if NotEqual *or* unordered - force the unordered cases not to jump.
            Jump unordered = makeBranch(ARMv7Assembler::ConditionVS);
            Jump result = makeBranch(ARMv7Assembler::ConditionNE);
            unordered.link(this);
            return result;
        }
        if (cond == DoubleEqualOrUnordered) {
            Jump unordered = makeBranch(ARMv7Assembler::ConditionVS);
            Jump notEqual = makeBranch(ARMv7Assembler::ConditionNE);
            unordered.link(this);
            // We get here if either unordered or equal.
            Jump result = jump();
            notEqual.link(this);
            return result;
        }
        return makeBranch(cond);
    }

    enum BranchTruncateType { BranchIfTruncateFailed, BranchIfTruncateSuccessful };
    Jump branchTruncateDoubleToInt32(FPRegisterID src, RegisterID dest, BranchTruncateType branchType = BranchIfTruncateFailed)
    {
        // Convert into dest.
        m_assembler.vcvt_floatingPointToSigned(fpTempRegisterAsSingle(), src);
        m_assembler.vmov(dest, fpTempRegisterAsSingle());

        // Calculate 2x dest.  If the value potentially underflowed, it will have
        // clamped to 0x80000000, so 2x dest is zero in this case. In the case of
        // overflow the result will be equal to -2.
        Jump underflow = branchAdd32(Zero, dest, dest, dataTempRegister);
        Jump noOverflow = branch32(NotEqual, dataTempRegister, TrustedImm32(-2));

        // For BranchIfTruncateSuccessful, we branch if 'noOverflow' jumps.
        underflow.link(this);
        if (branchType == BranchIfTruncateSuccessful)
            return noOverflow;

        // We'll reach the current point in the code on failure, so plant a
        // jump here & link the success case.
        Jump failure = jump();
        noOverflow.link(this);
        return failure;
    }

    Jump branchTruncateDoubleToUint32(FPRegisterID src, RegisterID dest, BranchTruncateType branchType = BranchIfTruncateFailed)
    {
        m_assembler.vcvt_floatingPointToSigned(fpTempRegisterAsSingle(), src);
        m_assembler.vmov(dest, fpTempRegisterAsSingle());
        
        Jump overflow = branch32(Equal, dest, TrustedImm32(0x7fffffff));
        Jump success = branch32(GreaterThanOrEqual, dest, TrustedImm32(0));
        overflow.link(this);

        if (branchType == BranchIfTruncateSuccessful)
            return success;
        
        Jump failure = jump();
        success.link(this);
        return failure;
    }

    // Result is undefined if the value is outside of the integer range.
    void truncateDoubleToInt32(FPRegisterID src, RegisterID dest)
    {
        m_assembler.vcvt_floatingPointToSigned(fpTempRegisterAsSingle(), src);
        m_assembler.vmov(dest, fpTempRegisterAsSingle());
    }

    void truncateDoubleToUint32(FPRegisterID src, RegisterID dest)
    {
        m_assembler.vcvt_floatingPointToUnsigned(fpTempRegisterAsSingle(), src);
        m_assembler.vmov(dest, fpTempRegisterAsSingle());
    }
    
    // Convert 'src' to an integer, and places the resulting 'dest'.
    // If the result is not representable as a 32 bit value, branch.
    // May also branch for some values that are representable in 32 bits
    // (specifically, in this case, 0).
    void branchConvertDoubleToInt32(FPRegisterID src, RegisterID dest, JumpList& failureCases, FPRegisterID, bool negZeroCheck = true)
    {
        m_assembler.vcvt_floatingPointToSigned(fpTempRegisterAsSingle(), src);
        m_assembler.vmov(dest, fpTempRegisterAsSingle());

        // Convert the integer result back to float & compare to the original value - if not equal or unordered (NaN) then jump.
        m_assembler.vcvt_signedToFloatingPoint(fpTempRegister, fpTempRegisterAsSingle());
        failureCases.append(branchDouble(DoubleNotEqualOrUnordered, src, fpTempRegister));

        // If the result is zero, it might have been -0.0, and the double comparison won't catch this!
        if (negZeroCheck)
            failureCases.append(branchTest32(Zero, dest));
    }

    Jump branchDoubleNonZero(FPRegisterID reg, FPRegisterID)
    {
        m_assembler.vcmpz(reg);
        m_assembler.vmrs();
        Jump unordered = makeBranch(ARMv7Assembler::ConditionVS);
        Jump result = makeBranch(ARMv7Assembler::ConditionNE);
        unordered.link(this);
        return result;
    }

    Jump branchDoubleZeroOrNaN(FPRegisterID reg, FPRegisterID)
    {
        m_assembler.vcmpz(reg);
        m_assembler.vmrs();
        Jump unordered = makeBranch(ARMv7Assembler::ConditionVS);
        Jump notEqual = makeBranch(ARMv7Assembler::ConditionNE);
        unordered.link(this);
        // We get here if either unordered or equal.
        Jump result = jump();
        notEqual.link(this);
        return result;
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
        // store postindexed with writeback
        m_assembler.ldr(dest, ARMRegisters::sp, sizeof(void*), false, true);
    }

    void push(RegisterID src)
    {
        // store preindexed with writeback
        m_assembler.str(src, ARMRegisters::sp, -sizeof(void*), true, true);
    }

    void push(Address address)
    {
        load32(address, dataTempRegister);
        push(dataTempRegister);
    }

    void push(TrustedImm32 imm)
    {
        move(imm, dataTempRegister);
        push(dataTempRegister);
    }

    // Register move operations:
    //
    // Move values in registers.

    void move(TrustedImm32 imm, RegisterID dest)
    {
        uint32_t value = imm.m_value;

        ARMThumbImmediate armImm = ARMThumbImmediate::makeEncodedImm(value);

        if (armImm.isValid())
            m_assembler.mov(dest, armImm);
        else if ((armImm = ARMThumbImmediate::makeEncodedImm(~value)).isValid())
            m_assembler.mvn(dest, armImm);
        else {
            m_assembler.mov(dest, ARMThumbImmediate::makeUInt16(value));
            if (value & 0xffff0000)
                m_assembler.movt(dest, ARMThumbImmediate::makeUInt16(value >> 16));
        }
    }

    void move(RegisterID src, RegisterID dest)
    {
        if (src != dest)
            m_assembler.mov(dest, src);
    }

    void move(TrustedImmPtr imm, RegisterID dest)
    {
        move(TrustedImm32(imm), dest);
    }

    void swap(RegisterID reg1, RegisterID reg2)
    {
        move(reg1, dataTempRegister);
        move(reg2, reg1);
        move(dataTempRegister, reg2);
    }

    void signExtend32ToPtr(RegisterID src, RegisterID dest)
    {
        move(src, dest);
    }

    void zeroExtend32ToPtr(RegisterID src, RegisterID dest)
    {
        move(src, dest);
    }

    // Invert a relational condition, e.g. == becomes !=, < becomes >=, etc.
    static RelationalCondition invert(RelationalCondition cond)
    {
        return static_cast<RelationalCondition>(cond ^ 1);
    }

    void nop()
    {
        m_assembler.nop();
    }
    
    static void replaceWithJump(CodeLocationLabel instructionStart, CodeLocationLabel destination)
    {
        ARMv7Assembler::replaceWithJump(instructionStart.dataLocation(), destination.dataLocation());
    }
    
    static ptrdiff_t maxJumpReplacementSize()
    {
        return ARMv7Assembler::maxJumpReplacementSize();
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
private:

    // Should we be using TEQ for equal/not-equal?
    void compare32(RegisterID left, TrustedImm32 right)
    {
        int32_t imm = right.m_value;
        if (!imm)
            m_assembler.tst(left, left);
        else {
            ARMThumbImmediate armImm = ARMThumbImmediate::makeEncodedImm(imm);
            if (armImm.isValid())
                m_assembler.cmp(left, armImm);
            else if ((armImm = ARMThumbImmediate::makeEncodedImm(-imm)).isValid())
                m_assembler.cmn(left, armImm);
            else {
                move(TrustedImm32(imm), dataTempRegister);
                m_assembler.cmp(left, dataTempRegister);
            }
        }
    }

    void test32(RegisterID reg, TrustedImm32 mask)
    {
        int32_t imm = mask.m_value;

        if (imm == -1)
            m_assembler.tst(reg, reg);
        else {
            ARMThumbImmediate armImm = ARMThumbImmediate::makeEncodedImm(imm);
            if (armImm.isValid())
                m_assembler.tst(reg, armImm);
            else {
                move(mask, dataTempRegister);
                m_assembler.tst(reg, dataTempRegister);
            }
        }
    }

public:
    Jump branch32(RelationalCondition cond, RegisterID left, RegisterID right)
    {
        m_assembler.cmp(left, right);
        return Jump(makeBranch(cond));
    }

    Jump branch32(RelationalCondition cond, RegisterID left, TrustedImm32 right)
    {
        compare32(left, right);
        return Jump(makeBranch(cond));
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
        // use addressTempRegister incase the branch32 we call uses dataTempRegister. :-/
        load32(left, addressTempRegister);
        return branch32(cond, addressTempRegister, right);
    }

    Jump branch32(RelationalCondition cond, BaseIndex left, TrustedImm32 right)
    {
        // use addressTempRegister incase the branch32 we call uses dataTempRegister. :-/
        load32(left, addressTempRegister);
        return branch32(cond, addressTempRegister, right);
    }

    Jump branch32WithUnalignedHalfWords(RelationalCondition cond, BaseIndex left, TrustedImm32 right)
    {
        // use addressTempRegister incase the branch32 we call uses dataTempRegister. :-/
        load32WithUnalignedHalfWords(left, addressTempRegister);
        return branch32(cond, addressTempRegister, right);
    }

    Jump branch32(RelationalCondition cond, AbsoluteAddress left, RegisterID right)
    {
        load32(left.m_ptr, dataTempRegister);
        return branch32(cond, dataTempRegister, right);
    }

    Jump branch32(RelationalCondition cond, AbsoluteAddress left, TrustedImm32 right)
    {
        // use addressTempRegister incase the branch32 we call uses dataTempRegister. :-/
        load32(left.m_ptr, addressTempRegister);
        return branch32(cond, addressTempRegister, right);
    }

    Jump branch8(RelationalCondition cond, RegisterID left, TrustedImm32 right)
    {
        compare32(left, right);
        return Jump(makeBranch(cond));
    }

    Jump branch8(RelationalCondition cond, Address left, TrustedImm32 right)
    {
        ASSERT(!(0xffffff00 & right.m_value));
        // use addressTempRegister incase the branch8 we call uses dataTempRegister. :-/
        load8(left, addressTempRegister);
        return branch8(cond, addressTempRegister, right);
    }

    Jump branch8(RelationalCondition cond, BaseIndex left, TrustedImm32 right)
    {
        ASSERT(!(0xffffff00 & right.m_value));
        // use addressTempRegister incase the branch32 we call uses dataTempRegister. :-/
        load8(left, addressTempRegister);
        return branch32(cond, addressTempRegister, right);
    }
    
    Jump branchTest32(ResultCondition cond, RegisterID reg, RegisterID mask)
    {
        m_assembler.tst(reg, mask);
        return Jump(makeBranch(cond));
    }

    Jump branchTest32(ResultCondition cond, RegisterID reg, TrustedImm32 mask = TrustedImm32(-1))
    {
        test32(reg, mask);
        return Jump(makeBranch(cond));
    }

    Jump branchTest32(ResultCondition cond, Address address, TrustedImm32 mask = TrustedImm32(-1))
    {
        // use addressTempRegister incase the branchTest32 we call uses dataTempRegister. :-/
        load32(address, addressTempRegister);
        return branchTest32(cond, addressTempRegister, mask);
    }

    Jump branchTest32(ResultCondition cond, BaseIndex address, TrustedImm32 mask = TrustedImm32(-1))
    {
        // use addressTempRegister incase the branchTest32 we call uses dataTempRegister. :-/
        load32(address, addressTempRegister);
        return branchTest32(cond, addressTempRegister, mask);
    }

    Jump branchTest8(ResultCondition cond, Address address, TrustedImm32 mask = TrustedImm32(-1))
    {
        // use addressTempRegister incase the branchTest8 we call uses dataTempRegister. :-/
        load8(address, addressTempRegister);
        return branchTest32(cond, addressTempRegister, mask);
    }

    Jump branchTest8(ResultCondition cond, AbsoluteAddress address, TrustedImm32 mask = TrustedImm32(-1))
    {
        // use addressTempRegister incase the branchTest8 we call uses dataTempRegister. :-/
        move(TrustedImmPtr(address.m_ptr), addressTempRegister);
        load8(Address(addressTempRegister), addressTempRegister);
        return branchTest32(cond, addressTempRegister, mask);
    }

    void jump(RegisterID target)
    {
        m_assembler.bx(target);
    }

    // Address is a memory location containing the address to jump to
    void jump(Address address)
    {
        load32(address, dataTempRegister);
        m_assembler.bx(dataTempRegister);
    }
    
    void jump(AbsoluteAddress address)
    {
        move(TrustedImmPtr(address.m_ptr), dataTempRegister);
        load32(Address(dataTempRegister), dataTempRegister);
        m_assembler.bx(dataTempRegister);
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
    
    Jump branchAdd32(ResultCondition cond, RegisterID op1, RegisterID op2, RegisterID dest)
    {
        m_assembler.add_S(dest, op1, op2);
        return Jump(makeBranch(cond));
    }

    Jump branchAdd32(ResultCondition cond, RegisterID op1, TrustedImm32 imm, RegisterID dest)
    {
        ARMThumbImmediate armImm = ARMThumbImmediate::makeEncodedImm(imm.m_value);
        if (armImm.isValid())
            m_assembler.add_S(dest, op1, armImm);
        else {
            move(imm, dataTempRegister);
            m_assembler.add_S(dest, op1, dataTempRegister);
        }
        return Jump(makeBranch(cond));
    }

    Jump branchAdd32(ResultCondition cond, RegisterID src, RegisterID dest)
    {
        return branchAdd32(cond, dest, src, dest);
    }

    Jump branchAdd32(ResultCondition cond, TrustedImm32 imm, RegisterID dest)
    {
        return branchAdd32(cond, dest, imm, dest);
    }

    Jump branchAdd32(ResultCondition cond, TrustedImm32 imm, AbsoluteAddress dest)
    {
        // Move the high bits of the address into addressTempRegister,
        // and load the value into dataTempRegister.
        move(TrustedImmPtr(dest.m_ptr), addressTempRegister);
        m_assembler.ldr(dataTempRegister, addressTempRegister, ARMThumbImmediate::makeUInt16(0));

        // Do the add.
        ARMThumbImmediate armImm = ARMThumbImmediate::makeEncodedImm(imm.m_value);
        if (armImm.isValid())
            m_assembler.add_S(dataTempRegister, dataTempRegister, armImm);
        else {
            // If the operand does not fit into an immediate then load it temporarily
            // into addressTempRegister; since we're overwriting addressTempRegister
            // we'll need to reload it with the high bits of the address afterwards.
            move(imm, addressTempRegister);
            m_assembler.add_S(dataTempRegister, dataTempRegister, addressTempRegister);
            move(TrustedImmPtr(dest.m_ptr), addressTempRegister);
        }

        // Store the result.
        m_assembler.str(dataTempRegister, addressTempRegister, ARMThumbImmediate::makeUInt16(0));

        return Jump(makeBranch(cond));
    }

    Jump branchMul32(ResultCondition cond, RegisterID src1, RegisterID src2, RegisterID dest)
    {
        m_assembler.smull(dest, dataTempRegister, src1, src2);

        if (cond == Overflow) {
            m_assembler.asr(addressTempRegister, dest, 31);
            return branch32(NotEqual, addressTempRegister, dataTempRegister);
        }

        return branchTest32(cond, dest);
    }

    Jump branchMul32(ResultCondition cond, RegisterID src, RegisterID dest)
    {
        return branchMul32(cond, src, dest, dest);
    }

    Jump branchMul32(ResultCondition cond, TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        move(imm, dataTempRegister);
        return branchMul32(cond, dataTempRegister, src, dest);
    }

    Jump branchNeg32(ResultCondition cond, RegisterID srcDest)
    {
        ARMThumbImmediate zero = ARMThumbImmediate::makeUInt12(0);
        m_assembler.sub_S(srcDest, zero, srcDest);
        return Jump(makeBranch(cond));
    }

    Jump branchOr32(ResultCondition cond, RegisterID src, RegisterID dest)
    {
        m_assembler.orr_S(dest, dest, src);
        return Jump(makeBranch(cond));
    }

    Jump branchSub32(ResultCondition cond, RegisterID op1, RegisterID op2, RegisterID dest)
    {
        m_assembler.sub_S(dest, op1, op2);
        return Jump(makeBranch(cond));
    }

    Jump branchSub32(ResultCondition cond, RegisterID op1, TrustedImm32 imm, RegisterID dest)
    {
        ARMThumbImmediate armImm = ARMThumbImmediate::makeEncodedImm(imm.m_value);
        if (armImm.isValid())
            m_assembler.sub_S(dest, op1, armImm);
        else {
            move(imm, dataTempRegister);
            m_assembler.sub_S(dest, op1, dataTempRegister);
        }
        return Jump(makeBranch(cond));
    }
    
    Jump branchSub32(ResultCondition cond, RegisterID src, RegisterID dest)
    {
        return branchSub32(cond, dest, src, dest);
    }

    Jump branchSub32(ResultCondition cond, TrustedImm32 imm, RegisterID dest)
    {
        return branchSub32(cond, dest, imm, dest);
    }
    
    void relativeTableJump(RegisterID index, int scale)
    {
        ASSERT(scale >= 0 && scale <= 31);

        // dataTempRegister will point after the jump if index register contains zero
        move(ARMRegisters::pc, dataTempRegister);
        m_assembler.add(dataTempRegister, dataTempRegister, ARMThumbImmediate::makeEncodedImm(9));

        ShiftTypeAndAmount shift(SRType_LSL, scale);
        m_assembler.add(dataTempRegister, dataTempRegister, index, shift);
        jump(dataTempRegister);
    }

    // Miscellaneous operations:

    void breakpoint(uint8_t imm = 0)
    {
        m_assembler.bkpt(imm);
    }

    ALWAYS_INLINE Call nearCall()
    {
        moveFixedWidthEncoding(TrustedImm32(0), dataTempRegister);
        return Call(m_assembler.blx(dataTempRegister), Call::LinkableNear);
    }

    ALWAYS_INLINE Call call()
    {
        moveFixedWidthEncoding(TrustedImm32(0), dataTempRegister);
        return Call(m_assembler.blx(dataTempRegister), Call::Linkable);
    }

    ALWAYS_INLINE Call call(RegisterID target)
    {
        return Call(m_assembler.blx(target), Call::None);
    }

    ALWAYS_INLINE Call call(Address address)
    {
        load32(address, dataTempRegister);
        return Call(m_assembler.blx(dataTempRegister), Call::None);
    }

    ALWAYS_INLINE void ret()
    {
        m_assembler.bx(linkRegister);
    }

    void compare32(RelationalCondition cond, RegisterID left, RegisterID right, RegisterID dest)
    {
        m_assembler.cmp(left, right);
        m_assembler.it(armV7Condition(cond), false);
        m_assembler.mov(dest, ARMThumbImmediate::makeUInt16(1));
        m_assembler.mov(dest, ARMThumbImmediate::makeUInt16(0));
    }

    void compare32(RelationalCondition cond, Address left, RegisterID right, RegisterID dest)
    {
        load32(left, dataTempRegister);
        compare32(cond, dataTempRegister, right, dest);
    }

    void compare8(RelationalCondition cond, Address left, TrustedImm32 right, RegisterID dest)
    {
        load8(left, addressTempRegister);
        compare32(cond, addressTempRegister, right, dest);
    }

    void compare32(RelationalCondition cond, RegisterID left, TrustedImm32 right, RegisterID dest)
    {
        compare32(left, right);
        m_assembler.it(armV7Condition(cond), false);
        m_assembler.mov(dest, ARMThumbImmediate::makeUInt16(1));
        m_assembler.mov(dest, ARMThumbImmediate::makeUInt16(0));
    }

    // FIXME:
    // The mask should be optional... paerhaps the argument order should be
    // dest-src, operations always have a dest? ... possibly not true, considering
    // asm ops like test, or pseudo ops like pop().
    void test32(ResultCondition cond, Address address, TrustedImm32 mask, RegisterID dest)
    {
        load32(address, dataTempRegister);
        test32(dataTempRegister, mask);
        m_assembler.it(armV7Condition(cond), false);
        m_assembler.mov(dest, ARMThumbImmediate::makeUInt16(1));
        m_assembler.mov(dest, ARMThumbImmediate::makeUInt16(0));
    }

    void test8(ResultCondition cond, Address address, TrustedImm32 mask, RegisterID dest)
    {
        load8(address, dataTempRegister);
        test32(dataTempRegister, mask);
        m_assembler.it(armV7Condition(cond), false);
        m_assembler.mov(dest, ARMThumbImmediate::makeUInt16(1));
        m_assembler.mov(dest, ARMThumbImmediate::makeUInt16(0));
    }

    ALWAYS_INLINE DataLabel32 moveWithPatch(TrustedImm32 imm, RegisterID dst)
    {
        padBeforePatch();
        moveFixedWidthEncoding(imm, dst);
        return DataLabel32(this);
    }

    ALWAYS_INLINE DataLabelPtr moveWithPatch(TrustedImmPtr imm, RegisterID dst)
    {
        padBeforePatch();
        moveFixedWidthEncoding(TrustedImm32(imm), dst);
        return DataLabelPtr(this);
    }

    ALWAYS_INLINE Jump branchPtrWithPatch(RelationalCondition cond, RegisterID left, DataLabelPtr& dataLabel, TrustedImmPtr initialRightValue = TrustedImmPtr(0))
    {
        dataLabel = moveWithPatch(initialRightValue, dataTempRegister);
        return branch32(cond, left, dataTempRegister);
    }

    ALWAYS_INLINE Jump branchPtrWithPatch(RelationalCondition cond, Address left, DataLabelPtr& dataLabel, TrustedImmPtr initialRightValue = TrustedImmPtr(0))
    {
        load32(left, addressTempRegister);
        dataLabel = moveWithPatch(initialRightValue, dataTempRegister);
        return branch32(cond, addressTempRegister, dataTempRegister);
    }
    
    PatchableJump patchableBranchPtr(RelationalCondition cond, Address left, TrustedImmPtr right = TrustedImmPtr(0))
    {
        m_makeJumpPatchable = true;
        Jump result = branch32(cond, left, TrustedImm32(right));
        m_makeJumpPatchable = false;
        return PatchableJump(result);
    }
    
    PatchableJump patchableBranchTest32(ResultCondition cond, RegisterID reg, TrustedImm32 mask = TrustedImm32(-1))
    {
        m_makeJumpPatchable = true;
        Jump result = branchTest32(cond, reg, mask);
        m_makeJumpPatchable = false;
        return PatchableJump(result);
    }

    PatchableJump patchableBranch32(RelationalCondition cond, RegisterID reg, TrustedImm32 imm)
    {
        m_makeJumpPatchable = true;
        Jump result = branch32(cond, reg, imm);
        m_makeJumpPatchable = false;
        return PatchableJump(result);
    }

    PatchableJump patchableBranchPtrWithPatch(RelationalCondition cond, Address left, DataLabelPtr& dataLabel, TrustedImmPtr initialRightValue = TrustedImmPtr(0))
    {
        m_makeJumpPatchable = true;
        Jump result = branchPtrWithPatch(cond, left, dataLabel, initialRightValue);
        m_makeJumpPatchable = false;
        return PatchableJump(result);
    }

    PatchableJump patchableJump()
    {
        padBeforePatch();
        m_makeJumpPatchable = true;
        Jump result = jump();
        m_makeJumpPatchable = false;
        return PatchableJump(result);
    }

    ALWAYS_INLINE DataLabelPtr storePtrWithPatch(TrustedImmPtr initialValue, ImplicitAddress address)
    {
        DataLabelPtr label = moveWithPatch(initialValue, dataTempRegister);
        store32(dataTempRegister, address);
        return label;
    }
    ALWAYS_INLINE DataLabelPtr storePtrWithPatch(ImplicitAddress address) { return storePtrWithPatch(TrustedImmPtr(0), address); }


    ALWAYS_INLINE Call tailRecursiveCall()
    {
        // Like a normal call, but don't link.
        moveFixedWidthEncoding(TrustedImm32(0), dataTempRegister);
        return Call(m_assembler.bx(dataTempRegister), Call::Linkable);
    }

    ALWAYS_INLINE Call makeTailRecursiveCall(Jump oldJump)
    {
        oldJump.link(this);
        return tailRecursiveCall();
    }

    
    int executableOffsetFor(int location)
    {
        return m_assembler.executableOffsetFor(location);
    }

    static FunctionPtr readCallTarget(CodeLocationCall call)
    {
        return FunctionPtr(reinterpret_cast<void(*)()>(ARMv7Assembler::readCallTarget(call.dataLocation())));
    }
    
    static bool canJumpReplacePatchableBranchPtrWithPatch() { return false; }
    
    static CodeLocationLabel startOfBranchPtrWithPatchOnRegister(CodeLocationDataLabelPtr label)
    {
        const unsigned twoWordOpSize = 4;
        return label.labelAtOffset(-twoWordOpSize * 2);
    }
    
    static void revertJumpReplacementToBranchPtrWithPatch(CodeLocationLabel instructionStart, RegisterID rd, void* initialValue)
    {
#if OS(LINUX) || OS(QNX)
        ARMv7Assembler::revertJumpTo_movT3movtcmpT2(instructionStart.dataLocation(), rd, dataTempRegister, reinterpret_cast<uintptr_t>(initialValue));
#else
        UNUSED_PARAM(rd);
        ARMv7Assembler::revertJumpTo_movT3(instructionStart.dataLocation(), dataTempRegister, ARMThumbImmediate::makeUInt16(reinterpret_cast<uintptr_t>(initialValue) & 0xffff));
#endif
    }
    
    static CodeLocationLabel startOfPatchableBranchPtrWithPatchOnAddress(CodeLocationDataLabelPtr)
    {
        UNREACHABLE_FOR_PLATFORM();
        return CodeLocationLabel();
    }
    
    static void revertJumpReplacementToPatchableBranchPtrWithPatch(CodeLocationLabel, Address, void*)
    {
        UNREACHABLE_FOR_PLATFORM();
    }

protected:
    ALWAYS_INLINE Jump jump()
    {
        m_assembler.label(); // Force nop-padding if we're in the middle of a watchpoint.
        moveFixedWidthEncoding(TrustedImm32(0), dataTempRegister);
        return Jump(m_assembler.bx(dataTempRegister), m_makeJumpPatchable ? ARMv7Assembler::JumpNoConditionFixedSize : ARMv7Assembler::JumpNoCondition);
    }

    ALWAYS_INLINE Jump makeBranch(ARMv7Assembler::Condition cond)
    {
        m_assembler.label(); // Force nop-padding if we're in the middle of a watchpoint.
        m_assembler.it(cond, true, true);
        moveFixedWidthEncoding(TrustedImm32(0), dataTempRegister);
        return Jump(m_assembler.bx(dataTempRegister), m_makeJumpPatchable ? ARMv7Assembler::JumpConditionFixedSize : ARMv7Assembler::JumpCondition, cond);
    }
    ALWAYS_INLINE Jump makeBranch(RelationalCondition cond) { return makeBranch(armV7Condition(cond)); }
    ALWAYS_INLINE Jump makeBranch(ResultCondition cond) { return makeBranch(armV7Condition(cond)); }
    ALWAYS_INLINE Jump makeBranch(DoubleCondition cond) { return makeBranch(armV7Condition(cond)); }

    ArmAddress setupArmAddress(BaseIndex address)
    {
        if (address.offset) {
            ARMThumbImmediate imm = ARMThumbImmediate::makeUInt12OrEncodedImm(address.offset);
            if (imm.isValid())
                m_assembler.add(addressTempRegister, address.base, imm);
            else {
                move(TrustedImm32(address.offset), addressTempRegister);
                m_assembler.add(addressTempRegister, addressTempRegister, address.base);
            }

            return ArmAddress(addressTempRegister, address.index, address.scale);
        } else
            return ArmAddress(address.base, address.index, address.scale);
    }

    ArmAddress setupArmAddress(Address address)
    {
        if ((address.offset >= -0xff) && (address.offset <= 0xfff))
            return ArmAddress(address.base, address.offset);

        move(TrustedImm32(address.offset), addressTempRegister);
        return ArmAddress(address.base, addressTempRegister);
    }

    ArmAddress setupArmAddress(ImplicitAddress address)
    {
        if ((address.offset >= -0xff) && (address.offset <= 0xfff))
            return ArmAddress(address.base, address.offset);

        move(TrustedImm32(address.offset), addressTempRegister);
        return ArmAddress(address.base, addressTempRegister);
    }

    RegisterID makeBaseIndexBase(BaseIndex address)
    {
        if (!address.offset)
            return address.base;

        ARMThumbImmediate imm = ARMThumbImmediate::makeUInt12OrEncodedImm(address.offset);
        if (imm.isValid())
            m_assembler.add(addressTempRegister, address.base, imm);
        else {
            move(TrustedImm32(address.offset), addressTempRegister);
            m_assembler.add(addressTempRegister, addressTempRegister, address.base);
        }

        return addressTempRegister;
    }

    void moveFixedWidthEncoding(TrustedImm32 imm, RegisterID dst)
    {
        uint32_t value = imm.m_value;
        m_assembler.movT3(dst, ARMThumbImmediate::makeUInt16(value & 0xffff));
        m_assembler.movt(dst, ARMThumbImmediate::makeUInt16(value >> 16));
    }

    ARMv7Assembler::Condition armV7Condition(RelationalCondition cond)
    {
        return static_cast<ARMv7Assembler::Condition>(cond);
    }

    ARMv7Assembler::Condition armV7Condition(ResultCondition cond)
    {
        return static_cast<ARMv7Assembler::Condition>(cond);
    }

    ARMv7Assembler::Condition armV7Condition(DoubleCondition cond)
    {
        return static_cast<ARMv7Assembler::Condition>(cond);
    }
    
private:
    friend class LinkBuffer;
    friend class RepatchBuffer;

    static void linkCall(void* code, Call call, FunctionPtr function)
    {
        ARMv7Assembler::linkCall(code, call.m_label, function.value());
    }

    static void repatchCall(CodeLocationCall call, CodeLocationLabel destination)
    {
        ARMv7Assembler::relinkCall(call.dataLocation(), destination.executableAddress());
    }

    static void repatchCall(CodeLocationCall call, FunctionPtr destination)
    {
        ARMv7Assembler::relinkCall(call.dataLocation(), destination.executableAddress());
    }

    bool m_makeJumpPatchable;
};

} // namespace JSC

#endif // ENABLE(ASSEMBLER)

#endif // MacroAssemblerARMv7_h
