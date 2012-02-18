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
    typedef ARMv7Assembler::LinkRecord LinkRecord;
    typedef ARMv7Assembler::JumpType JumpType;
    typedef ARMv7Assembler::JumpLinkType JumpLinkType;

    MacroAssemblerARMv7()
        : m_inUninterruptedSequence(false)
    {
    }
    
    void beginUninterruptedSequence() { m_inUninterruptedSequence = true; }
    void endUninterruptedSequence() { m_inUninterruptedSequence = false; }
    Vector<LinkRecord>& jumpsToLink() { return m_assembler.jumpsToLink(); }
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

    void and32(RegisterID src, RegisterID dest)
    {
        m_assembler.ARM_and(dest, dest, src);
    }

    void and32(TrustedImm32 imm, RegisterID dest)
    {
        ARMThumbImmediate armImm = ARMThumbImmediate::makeEncodedImm(imm.m_value);
        if (armImm.isValid())
            m_assembler.ARM_and(dest, dest, armImm);
        else {
            move(imm, dataTempRegister);
            m_assembler.ARM_and(dest, dest, dataTempRegister);
        }
    }

    void countLeadingZeros32(RegisterID src, RegisterID dest)
    {
        m_assembler.clz(dest, src);
    }

    void lshift32(RegisterID shift_amount, RegisterID dest)
    {
        // Clamp the shift to the range 0..31
        ARMThumbImmediate armImm = ARMThumbImmediate::makeEncodedImm(0x1f);
        ASSERT(armImm.isValid());
        m_assembler.ARM_and(dataTempRegister, shift_amount, armImm);

        m_assembler.lsl(dest, dest, dataTempRegister);
    }

    void lshift32(TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.lsl(dest, dest, imm.m_value & 0x1f);
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

    void not32(RegisterID srcDest)
    {
        m_assembler.mvn(srcDest, srcDest);
    }

    void or32(RegisterID src, RegisterID dest)
    {
        m_assembler.orr(dest, dest, src);
    }

    void or32(TrustedImm32 imm, RegisterID dest)
    {
        ARMThumbImmediate armImm = ARMThumbImmediate::makeEncodedImm(imm.m_value);
        if (armImm.isValid())
            m_assembler.orr(dest, dest, armImm);
        else {
            move(imm, dataTempRegister);
            m_assembler.orr(dest, dest, dataTempRegister);
        }
    }

    void rshift32(RegisterID shift_amount, RegisterID dest)
    {
        // Clamp the shift to the range 0..31
        ARMThumbImmediate armImm = ARMThumbImmediate::makeEncodedImm(0x1f);
        ASSERT(armImm.isValid());
        m_assembler.ARM_and(dataTempRegister, shift_amount, armImm);

        m_assembler.asr(dest, dest, dataTempRegister);
    }

    void rshift32(TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.asr(dest, dest, imm.m_value & 0x1f);
    }
    
    void urshift32(RegisterID shift_amount, RegisterID dest)
    {
        // Clamp the shift to the range 0..31
        ARMThumbImmediate armImm = ARMThumbImmediate::makeEncodedImm(0x1f);
        ASSERT(armImm.isValid());
        m_assembler.ARM_and(dataTempRegister, shift_amount, armImm);
        
        m_assembler.lsr(dest, dest, dataTempRegister);
    }
    
    void urshift32(TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.lsr(dest, dest, imm.m_value & 0x1f);
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

    void xor32(RegisterID src, RegisterID dest)
    {
        m_assembler.eor(dest, dest, src);
    }

    void xor32(TrustedImm32 imm, RegisterID dest)
    {
        ARMThumbImmediate armImm = ARMThumbImmediate::makeEncodedImm(imm.m_value);
        if (armImm.isValid())
            m_assembler.eor(dest, dest, armImm);
        else {
            move(imm, dataTempRegister);
            m_assembler.eor(dest, dest, dataTempRegister);
        }
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

    void load32(const void* address, RegisterID dest)
    {
        move(TrustedImmPtr(address), addressTempRegister);
        m_assembler.ldr(dest, addressTempRegister, ARMThumbImmediate::makeUInt16(0));
    }

    void load8(ImplicitAddress address, RegisterID dest)
    {
        load8(setupArmAddress(address), dest);
    }

    DataLabel32 load32WithAddressOffsetPatch(Address address, RegisterID dest)
    {
        DataLabel32 label = moveWithPatch(TrustedImm32(address.offset), dataTempRegister);
        load32(ArmAddress(address.base, dataTempRegister), dest);
        return label;
    }

    void load16(BaseIndex address, RegisterID dest)
    {
        m_assembler.ldrh(dest, makeBaseIndexBase(address), address.index, address.scale);
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


    // Floating-point operations:

    bool supportsFloatingPoint() const { return true; }
    // On x86(_64) the MacroAssembler provides an interface to truncate a double to an integer.
    // If a value is not representable as an integer, and possibly for some values that are,
    // (on x86 INT_MIN, since this is indistinguishable from results for out-of-range/NaN input)
    // a branch will  be taken.  It is not clear whether this interface will be well suited to
    // other platforms.  On ARMv7 the hardware truncation operation produces multiple possible
    // failure values (saturates to INT_MIN & INT_MAX, NaN reulsts in a value of 0).  This is a
    // temporary solution while we work out what this interface should be.  Either we need to
    // decide to make this interface work on all platforms, rework the interface to make it more
    // generic, or decide that the MacroAssembler cannot practically be used to abstracted these
    // operations, and make clients go directly to the m_assembler to plant truncation instructions.
    // In short, FIXME:.
    bool supportsFloatingPointTruncate() const { return false; }

    bool supportsFloatingPointSqrt() const
    {
        return false;
    }

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

    void addDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.vadd_F64(dest, dest, src);
    }

    void addDouble(Address src, FPRegisterID dest)
    {
        loadDouble(src, fpTempRegister);
        addDouble(fpTempRegister, dest);
    }

    void divDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.vdiv_F64(dest, dest, src);
    }

    void subDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.vsub_F64(dest, dest, src);
    }

    void subDouble(Address src, FPRegisterID dest)
    {
        loadDouble(src, fpTempRegister);
        subDouble(fpTempRegister, dest);
    }

    void mulDouble(FPRegisterID src, FPRegisterID dest)
    {
        m_assembler.vmul_F64(dest, dest, src);
    }

    void mulDouble(Address src, FPRegisterID dest)
    {
        loadDouble(src, fpTempRegister);
        mulDouble(fpTempRegister, dest);
    }

    void sqrtDouble(FPRegisterID, FPRegisterID)
    {
        ASSERT_NOT_REACHED();
    }

    void convertInt32ToDouble(RegisterID src, FPRegisterID dest)
    {
        m_assembler.vmov(fpTempRegisterAsSingle(), src);
        m_assembler.vcvt_F64_S32(dest, fpTempRegisterAsSingle());
    }

    void convertInt32ToDouble(Address address, FPRegisterID dest)
    {
        // Fixme: load directly into the fpr!
        load32(address, dataTempRegister);
        m_assembler.vmov(fpTempRegisterAsSingle(), dataTempRegister);
        m_assembler.vcvt_F64_S32(dest, fpTempRegisterAsSingle());
    }

    void convertInt32ToDouble(AbsoluteAddress address, FPRegisterID dest)
    {
        // Fixme: load directly into the fpr!
        load32(address.m_ptr, dataTempRegister);
        m_assembler.vmov(fpTempRegisterAsSingle(), dataTempRegister);
        m_assembler.vcvt_F64_S32(dest, fpTempRegisterAsSingle());
    }

    Jump branchDouble(DoubleCondition cond, FPRegisterID left, FPRegisterID right)
    {
        m_assembler.vcmp_F64(left, right);
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

    Jump branchTruncateDoubleToInt32(FPRegisterID, RegisterID)
    {
        ASSERT_NOT_REACHED();
        return jump();
    }

    // Convert 'src' to an integer, and places the resulting 'dest'.
    // If the result is not representable as a 32 bit value, branch.
    // May also branch for some values that are representable in 32 bits
    // (specifically, in this case, 0).
    void branchConvertDoubleToInt32(FPRegisterID src, RegisterID dest, JumpList& failureCases, FPRegisterID)
    {
        m_assembler.vcvtr_S32_F64(fpTempRegisterAsSingle(), src);
        m_assembler.vmov(dest, fpTempRegisterAsSingle());

        // Convert the integer result back to float & compare to the original value - if not equal or unordered (NaN) then jump.
        m_assembler.vcvt_F64_S32(fpTempRegister, fpTempRegisterAsSingle());
        failureCases.append(branchDouble(DoubleNotEqualOrUnordered, src, fpTempRegister));

        // If the result is zero, it might have been -0.0, and the double comparison won't catch this!
        failureCases.append(branchTest32(Zero, dest));
    }

    Jump branchDoubleNonZero(FPRegisterID reg, FPRegisterID)
    {
        m_assembler.vcmpz_F64(reg);
        m_assembler.vmrs();
        Jump unordered = makeBranch(ARMv7Assembler::ConditionVS);
        Jump result = makeBranch(ARMv7Assembler::ConditionNE);
        unordered.link(this);
        return result;
    }

    Jump branchDoubleZeroOrNaN(FPRegisterID reg, FPRegisterID)
    {
        m_assembler.vcmpz_F64(reg);
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

        if (imm.m_isPointer)
            moveFixedWidthEncoding(imm, dest);
        else {
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
    }

    void move(RegisterID src, RegisterID dest)
    {
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
        if (src != dest)
            move(src, dest);
    }

    void zeroExtend32ToPtr(RegisterID src, RegisterID dest)
    {
        if (src != dest)
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

    Jump branch16(RelationalCondition cond, BaseIndex left, RegisterID right)
    {
        load16(left, dataTempRegister);
        m_assembler.lsl(addressTempRegister, right, 16);
        m_assembler.lsl(dataTempRegister, dataTempRegister, 16);
        return branch32(cond, dataTempRegister, addressTempRegister);
    }

    Jump branch16(RelationalCondition cond, BaseIndex left, TrustedImm32 right)
    {
        // use addressTempRegister incase the branch32 we call uses dataTempRegister. :-/
        load16(left, addressTempRegister);
        m_assembler.lsl(addressTempRegister, addressTempRegister, 16);
        return branch32(cond, addressTempRegister, TrustedImm32(right.m_value << 16));
    }

    Jump branch8(RelationalCondition cond, RegisterID left, TrustedImm32 right)
    {
        compare32(left, right);
        return Jump(makeBranch(cond));
    }

    Jump branch8(RelationalCondition cond, Address left, TrustedImm32 right)
    {
        // use addressTempRegister incase the branch8 we call uses dataTempRegister. :-/
        load8(left, addressTempRegister);
        return branch8(cond, addressTempRegister, right);
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

    Jump branchTest8(ResultCondition cond, RegisterID reg, TrustedImm32 mask = TrustedImm32(-1))
    {
        test32(reg, mask);
        return Jump(makeBranch(cond));
    }

    Jump branchTest8(ResultCondition cond, Address address, TrustedImm32 mask = TrustedImm32(-1))
    {
        // use addressTempRegister incase the branchTest8 we call uses dataTempRegister. :-/
        load8(address, addressTempRegister);
        return branchTest8(cond, addressTempRegister, mask);
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
        m_assembler.add_S(dest, dest, src);
        return Jump(makeBranch(cond));
    }

    Jump branchAdd32(ResultCondition cond, TrustedImm32 imm, RegisterID dest)
    {
        ARMThumbImmediate armImm = ARMThumbImmediate::makeEncodedImm(imm.m_value);
        if (armImm.isValid())
            m_assembler.add_S(dest, dest, armImm);
        else {
            move(imm, dataTempRegister);
            m_assembler.add_S(dest, dest, dataTempRegister);
        }
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

    Jump branchOr32(ResultCondition cond, RegisterID src, RegisterID dest)
    {
        m_assembler.orr_S(dest, dest, src);
        return Jump(makeBranch(cond));
    }

    Jump branchSub32(ResultCondition cond, RegisterID src, RegisterID dest)
    {
        m_assembler.sub_S(dest, dest, src);
        return Jump(makeBranch(cond));
    }

    Jump branchSub32(ResultCondition cond, TrustedImm32 imm, RegisterID dest)
    {
        ARMThumbImmediate armImm = ARMThumbImmediate::makeEncodedImm(imm.m_value);
        if (armImm.isValid())
            m_assembler.sub_S(dest, dest, armImm);
        else {
            move(imm, dataTempRegister);
            m_assembler.sub_S(dest, dest, dataTempRegister);
        }
        return Jump(makeBranch(cond));
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

    void breakpoint()
    {
        m_assembler.bkpt(0);
    }

    Call nearCall()
    {
        moveFixedWidthEncoding(TrustedImm32(0), dataTempRegister);
        return Call(m_assembler.blx(dataTempRegister), Call::LinkableNear);
    }

    Call call()
    {
        moveFixedWidthEncoding(TrustedImm32(0), dataTempRegister);
        return Call(m_assembler.blx(dataTempRegister), Call::Linkable);
    }

    Call call(RegisterID target)
    {
        return Call(m_assembler.blx(target), Call::None);
    }

    Call call(Address address)
    {
        load32(address, dataTempRegister);
        return Call(m_assembler.blx(dataTempRegister), Call::None);
    }

    void ret()
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

    DataLabel32 moveWithPatch(TrustedImm32 imm, RegisterID dst)
    {
        moveFixedWidthEncoding(imm, dst);
        return DataLabel32(this);
    }

    DataLabelPtr moveWithPatch(TrustedImmPtr imm, RegisterID dst)
    {
        moveFixedWidthEncoding(TrustedImm32(imm), dst);
        return DataLabelPtr(this);
    }

    Jump branchPtrWithPatch(RelationalCondition cond, RegisterID left, DataLabelPtr& dataLabel, TrustedImmPtr initialRightValue = TrustedImmPtr(0))
    {
        dataLabel = moveWithPatch(initialRightValue, dataTempRegister);
        return branch32(cond, left, dataTempRegister);
    }

    Jump branchPtrWithPatch(RelationalCondition cond, Address left, DataLabelPtr& dataLabel, TrustedImmPtr initialRightValue = TrustedImmPtr(0))
    {
        load32(left, addressTempRegister);
        dataLabel = moveWithPatch(initialRightValue, dataTempRegister);
        return branch32(cond, addressTempRegister, dataTempRegister);
    }

    DataLabelPtr storePtrWithPatch(TrustedImmPtr initialValue, ImplicitAddress address)
    {
        DataLabelPtr label = moveWithPatch(initialValue, dataTempRegister);
        store32(dataTempRegister, address);
        return label;
    }
    DataLabelPtr storePtrWithPatch(ImplicitAddress address) { return storePtrWithPatch(TrustedImmPtr(0), address); }


    Call tailRecursiveCall()
    {
        // Like a normal call, but don't link.
        moveFixedWidthEncoding(TrustedImm32(0), dataTempRegister);
        return Call(m_assembler.bx(dataTempRegister), Call::Linkable);
    }

    Call makeTailRecursiveCall(Jump oldJump)
    {
        oldJump.link(this);
        return tailRecursiveCall();
    }

    
    int executableOffsetFor(int location)
    {
        return m_assembler.executableOffsetFor(location);
    }

protected:
    bool inUninterruptedSequence()
    {
        return m_inUninterruptedSequence;
    }

    Jump jump()
    {
        moveFixedWidthEncoding(TrustedImm32(0), dataTempRegister);
        return Jump(m_assembler.bx(dataTempRegister), inUninterruptedSequence() ? ARMv7Assembler::JumpNoConditionFixedSize : ARMv7Assembler::JumpNoCondition);
    }

    Jump makeBranch(ARMv7Assembler::Condition cond)
    {
        m_assembler.it(cond, true, true);
        moveFixedWidthEncoding(TrustedImm32(0), dataTempRegister);
        return Jump(m_assembler.bx(dataTempRegister), inUninterruptedSequence() ? ARMv7Assembler::JumpConditionFixedSize : ARMv7Assembler::JumpCondition, cond);
    }
    Jump makeBranch(RelationalCondition cond) { return makeBranch(armV7Condition(cond)); }
    Jump makeBranch(ResultCondition cond) { return makeBranch(armV7Condition(cond)); }
    Jump makeBranch(DoubleCondition cond) { return makeBranch(armV7Condition(cond)); }

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
        ARMv7Assembler::linkCall(code, call.m_jmp, function.value());
    }

    static void repatchCall(CodeLocationCall call, CodeLocationLabel destination)
    {
        ARMv7Assembler::relinkCall(call.dataLocation(), destination.executableAddress());
    }

    static void repatchCall(CodeLocationCall call, FunctionPtr destination)
    {
        ARMv7Assembler::relinkCall(call.dataLocation(), destination.executableAddress());
    }
    
    bool m_inUninterruptedSequence;
};

} // namespace JSC

#endif // ENABLE(ASSEMBLER)

#endif // MacroAssemblerARMv7_h
