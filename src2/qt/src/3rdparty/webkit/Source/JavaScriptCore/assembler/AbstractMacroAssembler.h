/*
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

#ifndef AbstractMacroAssembler_h
#define AbstractMacroAssembler_h

#include "CodeLocation.h"
#include "MacroAssemblerCodeRef.h"
#include <wtf/Noncopyable.h>
#include <wtf/UnusedParam.h>

#if ENABLE(ASSEMBLER)

namespace JSC {

class LinkBuffer;
class RepatchBuffer;

template <class AssemblerType>
class AbstractMacroAssembler {
public:
    typedef AssemblerType AssemblerType_T;

    typedef MacroAssemblerCodePtr CodePtr;
    typedef MacroAssemblerCodeRef CodeRef;

    class Jump;

    typedef typename AssemblerType::RegisterID RegisterID;

    // Section 1: MacroAssembler operand types
    //
    // The following types are used as operands to MacroAssembler operations,
    // describing immediate  and memory operands to the instructions to be planted.


    enum Scale {
        TimesOne,
        TimesTwo,
        TimesFour,
        TimesEight,
    };

    // Address:
    //
    // Describes a simple base-offset address.
    struct Address {
        explicit Address(RegisterID base, int32_t offset = 0)
            : base(base)
            , offset(offset)
        {
        }

        RegisterID base;
        int32_t offset;
    };

    struct ExtendedAddress {
        explicit ExtendedAddress(RegisterID base, intptr_t offset = 0)
            : base(base)
            , offset(offset)
        {
        }
        
        RegisterID base;
        intptr_t offset;
    };

    // ImplicitAddress:
    //
    // This class is used for explicit 'load' and 'store' operations
    // (as opposed to situations in which a memory operand is provided
    // to a generic operation, such as an integer arithmetic instruction).
    //
    // In the case of a load (or store) operation we want to permit
    // addresses to be implicitly constructed, e.g. the two calls:
    //
    //     load32(Address(addrReg), destReg);
    //     load32(addrReg, destReg);
    //
    // Are equivalent, and the explicit wrapping of the Address in the former
    // is unnecessary.
    struct ImplicitAddress {
        ImplicitAddress(RegisterID base)
            : base(base)
            , offset(0)
        {
        }

        ImplicitAddress(Address address)
            : base(address.base)
            , offset(address.offset)
        {
        }

        RegisterID base;
        int32_t offset;
    };

    // BaseIndex:
    //
    // Describes a complex addressing mode.
    struct BaseIndex {
        BaseIndex(RegisterID base, RegisterID index, Scale scale, int32_t offset = 0)
            : base(base)
            , index(index)
            , scale(scale)
            , offset(offset)
        {
        }

        RegisterID base;
        RegisterID index;
        Scale scale;
        int32_t offset;
    };

    // AbsoluteAddress:
    //
    // Describes an memory operand given by a pointer.  For regular load & store
    // operations an unwrapped void* will be used, rather than using this.
    struct AbsoluteAddress {
        explicit AbsoluteAddress(const void* ptr)
            : m_ptr(ptr)
        {
        }

        const void* m_ptr;
    };

    // TrustedImmPtr:
    //
    // A pointer sized immediate operand to an instruction - this is wrapped
    // in a class requiring explicit construction in order to differentiate
    // from pointers used as absolute addresses to memory operations
    struct TrustedImmPtr {
        explicit TrustedImmPtr(const void* value)
            : m_value(value)
        {
        }

        intptr_t asIntptr()
        {
            return reinterpret_cast<intptr_t>(m_value);
        }

        const void* m_value;
    };

    struct ImmPtr : public TrustedImmPtr {
        explicit ImmPtr(const void* value)
            : TrustedImmPtr(value)
        {
        }
    };

    // TrustedImm32:
    //
    // A 32bit immediate operand to an instruction - this is wrapped in a
    // class requiring explicit construction in order to prevent RegisterIDs
    // (which are implemented as an enum) from accidentally being passed as
    // immediate values.
    struct TrustedImm32 {
        explicit TrustedImm32(int32_t value)
            : m_value(value)
#if CPU(ARM) || CPU(MIPS)
            , m_isPointer(false)
#endif
        {
        }

#if !CPU(X86_64)
        explicit TrustedImm32(TrustedImmPtr ptr)
            : m_value(ptr.asIntptr())
#if CPU(ARM) || CPU(MIPS)
            , m_isPointer(true)
#endif
        {
        }
#endif

        int32_t m_value;
#if CPU(ARM) || CPU(MIPS)
        // We rely on being able to regenerate code to recover exception handling
        // information.  Since ARMv7 supports 16-bit immediates there is a danger
        // that if pointer values change the layout of the generated code will change.
        // To avoid this problem, always generate pointers (and thus Imm32s constructed
        // from ImmPtrs) with a code sequence that is able  to represent  any pointer
        // value - don't use a more compact form in these cases.
        // Same for MIPS.
        bool m_isPointer;
#endif
    };


    struct Imm32 : public TrustedImm32 {
        explicit Imm32(int32_t value)
            : TrustedImm32(value)
        {
        }
#if !CPU(X86_64)
        explicit Imm32(TrustedImmPtr ptr)
            : TrustedImm32(ptr)
        {
        }
#endif
    };
    
    // Section 2: MacroAssembler code buffer handles
    //
    // The following types are used to reference items in the code buffer
    // during JIT code generation.  For example, the type Jump is used to
    // track the location of a jump instruction so that it may later be
    // linked to a label marking its destination.


    // Label:
    //
    // A Label records a point in the generated instruction stream, typically such that
    // it may be used as a destination for a jump.
    class Label {
        template<class TemplateAssemblerType>
        friend class AbstractMacroAssembler;
        friend class Jump;
        friend class MacroAssemblerCodeRef;
        friend class LinkBuffer;

    public:
        Label()
        {
        }

        Label(AbstractMacroAssembler<AssemblerType>* masm)
            : m_label(masm->m_assembler.label())
        {
        }
        
        bool isSet() const { return m_label.isSet(); }
    private:
        AssemblerLabel m_label;
    };

    // DataLabelPtr:
    //
    // A DataLabelPtr is used to refer to a location in the code containing a pointer to be
    // patched after the code has been generated.
    class DataLabelPtr {
        template<class TemplateAssemblerType>
        friend class AbstractMacroAssembler;
        friend class LinkBuffer;
    public:
        DataLabelPtr()
        {
        }

        DataLabelPtr(AbstractMacroAssembler<AssemblerType>* masm)
            : m_label(masm->m_assembler.label())
        {
        }
        
        bool isSet() const { return m_label.isSet(); }
        
    private:
        AssemblerLabel m_label;
    };

    // DataLabel32:
    //
    // A DataLabelPtr is used to refer to a location in the code containing a pointer to be
    // patched after the code has been generated.
    class DataLabel32 {
        template<class TemplateAssemblerType>
        friend class AbstractMacroAssembler;
        friend class LinkBuffer;
    public:
        DataLabel32()
        {
        }

        DataLabel32(AbstractMacroAssembler<AssemblerType>* masm)
            : m_label(masm->m_assembler.label())
        {
        }

    private:
        AssemblerLabel m_label;
    };

    // Call:
    //
    // A Call object is a reference to a call instruction that has been planted
    // into the code buffer - it is typically used to link the call, setting the
    // relative offset such that when executed it will call to the desired
    // destination.
    class Call {
        template<class TemplateAssemblerType>
        friend class AbstractMacroAssembler;

    public:
        enum Flags {
            None = 0x0,
            Linkable = 0x1,
            Near = 0x2,
            LinkableNear = 0x3,
        };

        Call()
            : m_flags(None)
        {
        }
        
        Call(AssemblerLabel jmp, Flags flags)
            : m_jmp(jmp)
            , m_flags(flags)
        {
        }

        bool isFlagSet(Flags flag)
        {
            return m_flags & flag;
        }

        static Call fromTailJump(Jump jump)
        {
            return Call(jump.m_jmp, Linkable);
        }

        AssemblerLabel m_jmp;
    private:
        Flags m_flags;
    };

    // Jump:
    //
    // A jump object is a reference to a jump instruction that has been planted
    // into the code buffer - it is typically used to link the jump, setting the
    // relative offset such that when executed it will jump to the desired
    // destination.
    class Jump {
        template<class TemplateAssemblerType>
        friend class AbstractMacroAssembler;
        friend class Call;
        friend class LinkBuffer;
    public:
        Jump()
        {
        }
        
#if CPU(ARM_THUMB2)
        // Fixme: this information should be stored in the instruction stream, not in the Jump object.
        Jump(AssemblerLabel jmp, ARMv7Assembler::JumpType type, ARMv7Assembler::Condition condition = ARMv7Assembler::ConditionInvalid)
            : m_jmp(jmp)
            , m_type(type)
            , m_condition(condition)
        {
        }
#else
        Jump(AssemblerLabel jmp)    
            : m_jmp(jmp)
        {
        }
#endif

        void link(AbstractMacroAssembler<AssemblerType>* masm) const
        {
#if CPU(ARM_THUMB2)
            masm->m_assembler.linkJump(m_jmp, masm->m_assembler.label(), m_type, m_condition);
#else
            masm->m_assembler.linkJump(m_jmp, masm->m_assembler.label());
#endif
        }
        
        void linkTo(Label label, AbstractMacroAssembler<AssemblerType>* masm) const
        {
#if CPU(ARM_THUMB2)
            masm->m_assembler.linkJump(m_jmp, label.m_label, m_type, m_condition);
#else
            masm->m_assembler.linkJump(m_jmp, label.m_label);
#endif
        }

        bool isSet() const { return m_jmp.isSet(); }

    private:
        AssemblerLabel m_jmp;
#if CPU(ARM_THUMB2)
        ARMv7Assembler::JumpType m_type;
        ARMv7Assembler::Condition m_condition;
#endif
    };

    // JumpList:
    //
    // A JumpList is a set of Jump objects.
    // All jumps in the set will be linked to the same destination.
    class JumpList {
        friend class LinkBuffer;

    public:
        typedef Vector<Jump, 16> JumpVector;

        void link(AbstractMacroAssembler<AssemblerType>* masm)
        {
            size_t size = m_jumps.size();
            for (size_t i = 0; i < size; ++i)
                m_jumps[i].link(masm);
            m_jumps.clear();
        }
        
        void linkTo(Label label, AbstractMacroAssembler<AssemblerType>* masm)
        {
            size_t size = m_jumps.size();
            for (size_t i = 0; i < size; ++i)
                m_jumps[i].linkTo(label, masm);
            m_jumps.clear();
        }
        
        void append(Jump jump)
        {
            m_jumps.append(jump);
        }
        
        void append(JumpList& other)
        {
            m_jumps.append(other.m_jumps.begin(), other.m_jumps.size());
        }

        bool empty()
        {
            return !m_jumps.size();
        }
        
        void clear()
        {
            m_jumps.clear();
        }
        
        const JumpVector& jumps() { return m_jumps; }

    private:
        JumpVector m_jumps;
    };


    // Section 3: Misc admin methods
    Label label()
    {
        return Label(this);
    }
    
    Label align()
    {
        m_assembler.align(16);
        return Label(this);
    }

    ptrdiff_t differenceBetween(Label from, Jump to)
    {
        return AssemblerType::getDifferenceBetweenLabels(from.m_label, to.m_jmp);
    }

    ptrdiff_t differenceBetween(Label from, Call to)
    {
        return AssemblerType::getDifferenceBetweenLabels(from.m_label, to.m_jmp);
    }

    ptrdiff_t differenceBetween(Label from, Label to)
    {
        return AssemblerType::getDifferenceBetweenLabels(from.m_label, to.m_label);
    }

    ptrdiff_t differenceBetween(Label from, DataLabelPtr to)
    {
        return AssemblerType::getDifferenceBetweenLabels(from.m_label, to.m_label);
    }

    ptrdiff_t differenceBetween(Label from, DataLabel32 to)
    {
        return AssemblerType::getDifferenceBetweenLabels(from.m_label, to.m_label);
    }

    ptrdiff_t differenceBetween(DataLabelPtr from, Jump to)
    {
        return AssemblerType::getDifferenceBetweenLabels(from.m_label, to.m_jmp);
    }

    ptrdiff_t differenceBetween(DataLabelPtr from, DataLabelPtr to)
    {
        return AssemblerType::getDifferenceBetweenLabels(from.m_label, to.m_label);
    }

    ptrdiff_t differenceBetween(DataLabelPtr from, Call to)
    {
        return AssemblerType::getDifferenceBetweenLabels(from.m_label, to.m_jmp);
    }

    // Temporary interface; likely to be removed, since may be hard to port to all architectures.
#if CPU(X86) || CPU(X86_64)
    void rewindToLabel(Label rewindTo) { m_assembler.rewindToLabel(rewindTo.m_label); }
#endif

    void beginUninterruptedSequence() { }
    void endUninterruptedSequence() { }

#ifndef NDEBUG
    unsigned debugOffset() { return m_assembler.debugOffset(); }
#endif

protected:
    AssemblerType m_assembler;

    friend class LinkBuffer;
    friend class RepatchBuffer;

    static void linkJump(void* code, Jump jump, CodeLocationLabel target)
    {
        AssemblerType::linkJump(code, jump.m_jmp, target.dataLocation());
    }

    static void linkPointer(void* code, AssemblerLabel label, void* value)
    {
        AssemblerType::linkPointer(code, label, value);
    }

    static void* getLinkerAddress(void* code, AssemblerLabel label)
    {
        return AssemblerType::getRelocatedAddress(code, label);
    }

    static unsigned getLinkerCallReturnOffset(Call call)
    {
        return AssemblerType::getCallReturnOffset(call.m_jmp);
    }

    static void repatchJump(CodeLocationJump jump, CodeLocationLabel destination)
    {
        AssemblerType::relinkJump(jump.dataLocation(), destination.dataLocation());
    }

    static void repatchNearCall(CodeLocationNearCall nearCall, CodeLocationLabel destination)
    {
        AssemblerType::relinkCall(nearCall.dataLocation(), destination.executableAddress());
    }

    static void repatchInt32(CodeLocationDataLabel32 dataLabel32, int32_t value)
    {
        AssemblerType::repatchInt32(dataLabel32.dataLocation(), value);
    }

    static void repatchPointer(CodeLocationDataLabelPtr dataLabelPtr, void* value)
    {
        AssemblerType::repatchPointer(dataLabelPtr.dataLocation(), value);
    }
};

} // namespace JSC

#endif // ENABLE(ASSEMBLER)

#endif // AbstractMacroAssembler_h
