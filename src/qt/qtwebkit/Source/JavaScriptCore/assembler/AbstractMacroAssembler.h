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

#ifndef AbstractMacroAssembler_h
#define AbstractMacroAssembler_h

#include "AssemblerBuffer.h"
#include "CodeLocation.h"
#include "MacroAssemblerCodeRef.h"
#include <wtf/CryptographicallyRandomNumber.h>
#include <wtf/Noncopyable.h>

#if ENABLE(ASSEMBLER)


#if PLATFORM(QT)
#define ENABLE_JIT_CONSTANT_BLINDING 0
#endif

#ifndef ENABLE_JIT_CONSTANT_BLINDING
#define ENABLE_JIT_CONSTANT_BLINDING 1
#endif

namespace JSC {

inline bool isARMv7s()
{
#if CPU(APPLE_ARMV7S)
    return true;
#else
    return false;
#endif
}

inline bool isX86()
{
#if CPU(X86_64) || CPU(X86)
    return true;
#else
    return false;
#endif
}

class JumpReplacementWatchpoint;
class LinkBuffer;
class RepatchBuffer;
class Watchpoint;
namespace DFG {
struct OSRExit;
}

template <class AssemblerType>
class AbstractMacroAssembler {
public:
    friend class JITWriteBarrierBase;
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
        TrustedImmPtr() { }
        
        explicit TrustedImmPtr(const void* value)
            : m_value(value)
        {
        }
        
        // This is only here so that TrustedImmPtr(0) does not confuse the C++
        // overload handling rules.
        explicit TrustedImmPtr(int value)
            : m_value(0)
        {
            ASSERT_UNUSED(value, !value);
        }

        explicit TrustedImmPtr(size_t value)
            : m_value(reinterpret_cast<void*>(value))
        {
        }

        intptr_t asIntptr()
        {
            return reinterpret_cast<intptr_t>(m_value);
        }

        const void* m_value;
    };

    struct ImmPtr : 
#if ENABLE(JIT_CONSTANT_BLINDING)
        private TrustedImmPtr 
#else
        public TrustedImmPtr
#endif
    {
        explicit ImmPtr(const void* value)
            : TrustedImmPtr(value)
        {
        }

        TrustedImmPtr asTrustedImmPtr() { return *this; }
    };

    // TrustedImm32:
    //
    // A 32bit immediate operand to an instruction - this is wrapped in a
    // class requiring explicit construction in order to prevent RegisterIDs
    // (which are implemented as an enum) from accidentally being passed as
    // immediate values.
    struct TrustedImm32 {
        TrustedImm32() { }
        
        explicit TrustedImm32(int32_t value)
            : m_value(value)
        {
        }

#if !CPU(X86_64)
        explicit TrustedImm32(TrustedImmPtr ptr)
            : m_value(ptr.asIntptr())
        {
        }
#endif

        int32_t m_value;
    };


    struct Imm32 : 
#if ENABLE(JIT_CONSTANT_BLINDING)
        private TrustedImm32 
#else
        public TrustedImm32
#endif
    {
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
        const TrustedImm32& asTrustedImm32() const { return *this; }

    };
    
    // TrustedImm64:
    //
    // A 64bit immediate operand to an instruction - this is wrapped in a
    // class requiring explicit construction in order to prevent RegisterIDs
    // (which are implemented as an enum) from accidentally being passed as
    // immediate values.
    struct TrustedImm64 {
        TrustedImm64() { }
        
        explicit TrustedImm64(int64_t value)
            : m_value(value)
        {
        }

#if CPU(X86_64)
        explicit TrustedImm64(TrustedImmPtr ptr)
            : m_value(ptr.asIntptr())
        {
        }
#endif

        int64_t m_value;
    };

    struct Imm64 : 
#if ENABLE(JIT_CONSTANT_BLINDING)
        private TrustedImm64 
#else
        public TrustedImm64
#endif
    {
        explicit Imm64(int64_t value)
            : TrustedImm64(value)
        {
        }
#if CPU(X86_64)
        explicit Imm64(TrustedImmPtr ptr)
            : TrustedImm64(ptr)
        {
        }
#endif
        const TrustedImm64& asTrustedImm64() const { return *this; }
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
        friend struct DFG::OSRExit;
        friend class Jump;
        friend class JumpReplacementWatchpoint;
        friend class MacroAssemblerCodeRef;
        friend class LinkBuffer;
        friend class Watchpoint;

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
    
    // ConvertibleLoadLabel:
    //
    // A ConvertibleLoadLabel records a loadPtr instruction that can be patched to an addPtr
    // so that:
    //
    // loadPtr(Address(a, i), b)
    //
    // becomes:
    //
    // addPtr(TrustedImmPtr(i), a, b)
    class ConvertibleLoadLabel {
        template<class TemplateAssemblerType>
        friend class AbstractMacroAssembler;
        friend class LinkBuffer;
        
    public:
        ConvertibleLoadLabel()
        {
        }
        
        ConvertibleLoadLabel(AbstractMacroAssembler<AssemblerType>* masm)
            : m_label(masm->m_assembler.labelIgnoringWatchpoints())
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

        AssemblerLabel label() const { return m_label; }

    private:
        AssemblerLabel m_label;
    };

    // DataLabelCompact:
    //
    // A DataLabelCompact is used to refer to a location in the code containing a
    // compact immediate to be patched after the code has been generated.
    class DataLabelCompact {
        template<class TemplateAssemblerType>
        friend class AbstractMacroAssembler;
        friend class LinkBuffer;
    public:
        DataLabelCompact()
        {
        }
        
        DataLabelCompact(AbstractMacroAssembler<AssemblerType>* masm)
            : m_label(masm->m_assembler.label())
        {
        }
    
        DataLabelCompact(AssemblerLabel label)
            : m_label(label)
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
            : m_label(jmp)
            , m_flags(flags)
        {
        }

        bool isFlagSet(Flags flag)
        {
            return m_flags & flag;
        }

        static Call fromTailJump(Jump jump)
        {
            return Call(jump.m_label, Linkable);
        }

        AssemblerLabel m_label;
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
        friend struct DFG::OSRExit;
        friend class LinkBuffer;
    public:
        Jump()
        {
        }
        
#if CPU(ARM_THUMB2)
        // Fixme: this information should be stored in the instruction stream, not in the Jump object.
        Jump(AssemblerLabel jmp, ARMv7Assembler::JumpType type = ARMv7Assembler::JumpNoCondition, ARMv7Assembler::Condition condition = ARMv7Assembler::ConditionInvalid)
            : m_label(jmp)
            , m_type(type)
            , m_condition(condition)
        {
        }
#elif CPU(SH4)
        Jump(AssemblerLabel jmp, SH4Assembler::JumpType type = SH4Assembler::JumpFar)
            : m_label(jmp)
            , m_type(type)
        {
        }
#else
        Jump(AssemblerLabel jmp)    
            : m_label(jmp)
        {
        }
#endif
        
        Label label() const
        {
            Label result;
            result.m_label = m_label;
            return result;
        }

        void link(AbstractMacroAssembler<AssemblerType>* masm) const
        {
#if ENABLE(DFG_REGISTER_ALLOCATION_VALIDATION)
            masm->checkRegisterAllocationAgainstBranchRange(m_label.m_offset, masm->debugOffset());
#endif

#if CPU(ARM_THUMB2)
            masm->m_assembler.linkJump(m_label, masm->m_assembler.label(), m_type, m_condition);
#elif CPU(SH4)
            masm->m_assembler.linkJump(m_label, masm->m_assembler.label(), m_type);
#else
            masm->m_assembler.linkJump(m_label, masm->m_assembler.label());
#endif
        }
        
        void linkTo(Label label, AbstractMacroAssembler<AssemblerType>* masm) const
        {
#if ENABLE(DFG_REGISTER_ALLOCATION_VALIDATION)
            masm->checkRegisterAllocationAgainstBranchRange(label.m_label.m_offset, m_label.m_offset);
#endif

#if CPU(ARM_THUMB2)
            masm->m_assembler.linkJump(m_label, label.m_label, m_type, m_condition);
#else
            masm->m_assembler.linkJump(m_label, label.m_label);
#endif
        }

        bool isSet() const { return m_label.isSet(); }

    private:
        AssemblerLabel m_label;
#if CPU(ARM_THUMB2)
        ARMv7Assembler::JumpType m_type;
        ARMv7Assembler::Condition m_condition;
#endif
#if CPU(SH4)
        SH4Assembler::JumpType m_type;
#endif
    };

    struct PatchableJump {
        PatchableJump()
        {
        }

        explicit PatchableJump(Jump jump)
            : m_jump(jump)
        {
        }

        operator Jump&() { return m_jump; }

        Jump m_jump;
    };

    // JumpList:
    //
    // A JumpList is a set of Jump objects.
    // All jumps in the set will be linked to the same destination.
    class JumpList {
        friend class LinkBuffer;

    public:
        typedef Vector<Jump, 2> JumpVector;
        
        JumpList() { }
        
        JumpList(Jump jump)
        {
            append(jump);
        }

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
        
        void append(const JumpList& other)
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
        
        const JumpVector& jumps() const { return m_jumps; }

    private:
        JumpVector m_jumps;
    };


    // Section 3: Misc admin methods
#if ENABLE(DFG_JIT)
    Label labelIgnoringWatchpoints()
    {
        Label result;
        result.m_label = m_assembler.labelIgnoringWatchpoints();
        return result;
    }
#else
    Label labelIgnoringWatchpoints()
    {
        return label();
    }
#endif
    
    Label label()
    {
        return Label(this);
    }
    
    void padBeforePatch()
    {
        // Rely on the fact that asking for a label already does the padding.
        (void)label();
    }
    
    Label watchpointLabel()
    {
        Label result;
        result.m_label = m_assembler.labelForWatchpoint();
        return result;
    }
    
    Label align()
    {
        m_assembler.align(16);
        return Label(this);
    }

#if ENABLE(DFG_REGISTER_ALLOCATION_VALIDATION)
    class RegisterAllocationOffset {
    public:
        RegisterAllocationOffset(unsigned offset)
            : m_offset(offset)
        {
        }

        void check(unsigned low, unsigned high)
        {
            RELEASE_ASSERT_WITH_MESSAGE(!(low <= m_offset && m_offset <= high), "Unsafe branch over register allocation at instruction offset %u in jump offset range %u..%u", m_offset, low, high);
        }

    private:
        unsigned m_offset;
    };

    void addRegisterAllocationAtOffset(unsigned offset)
    {
        m_registerAllocationForOffsets.append(RegisterAllocationOffset(offset));
    }

    void clearRegisterAllocationOffsets()
    {
        m_registerAllocationForOffsets.clear();
    }

    void checkRegisterAllocationAgainstBranchRange(unsigned offset1, unsigned offset2)
    {
        if (offset1 > offset2)
            std::swap(offset1, offset2);

        size_t size = m_registerAllocationForOffsets.size();
        for (size_t i = 0; i < size; ++i)
            m_registerAllocationForOffsets[i].check(offset1, offset2);
    }
#endif

    template<typename T, typename U>
    static ptrdiff_t differenceBetween(T from, U to)
    {
        return AssemblerType::getDifferenceBetweenLabels(from.m_label, to.m_label);
    }

    static ptrdiff_t differenceBetweenCodePtr(const MacroAssemblerCodePtr& a, const MacroAssemblerCodePtr& b)
    {
        return reinterpret_cast<ptrdiff_t>(b.executableAddress()) - reinterpret_cast<ptrdiff_t>(a.executableAddress());
    }

    unsigned debugOffset() { return m_assembler.debugOffset(); }

    ALWAYS_INLINE static void cacheFlush(void* code, size_t size)
    {
        AssemblerType::cacheFlush(code, size);
    }
protected:
    AbstractMacroAssembler()
        : m_randomSource(cryptographicallyRandomNumber())
    {
    }

    AssemblerType m_assembler;
    
    uint32_t random()
    {
        return m_randomSource.getUint32();
    }

    WeakRandom m_randomSource;

#if ENABLE(DFG_REGISTER_ALLOCATION_VALIDATION)
    Vector<RegisterAllocationOffset, 10> m_registerAllocationForOffsets;
#endif

#if ENABLE(JIT_CONSTANT_BLINDING)
    static bool scratchRegisterForBlinding() { return false; }
    static bool shouldBlindForSpecificArch(uint32_t) { return true; }
    static bool shouldBlindForSpecificArch(uint64_t) { return true; }
#endif

    friend class LinkBuffer;
    friend class RepatchBuffer;

    static void linkJump(void* code, Jump jump, CodeLocationLabel target)
    {
        AssemblerType::linkJump(code, jump.m_label, target.dataLocation());
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
        return AssemblerType::getCallReturnOffset(call.m_label);
    }

    static void repatchJump(CodeLocationJump jump, CodeLocationLabel destination)
    {
        AssemblerType::relinkJump(jump.dataLocation(), destination.dataLocation());
    }

    static void repatchNearCall(CodeLocationNearCall nearCall, CodeLocationLabel destination)
    {
        AssemblerType::relinkCall(nearCall.dataLocation(), destination.executableAddress());
    }

    static void repatchCompact(CodeLocationDataLabelCompact dataLabelCompact, int32_t value)
    {
        AssemblerType::repatchCompact(dataLabelCompact.dataLocation(), value);
    }
    
    static void repatchInt32(CodeLocationDataLabel32 dataLabel32, int32_t value)
    {
        AssemblerType::repatchInt32(dataLabel32.dataLocation(), value);
    }

    static void repatchPointer(CodeLocationDataLabelPtr dataLabelPtr, void* value)
    {
        AssemblerType::repatchPointer(dataLabelPtr.dataLocation(), value);
    }
    
    static void* readPointer(CodeLocationDataLabelPtr dataLabelPtr)
    {
        return AssemblerType::readPointer(dataLabelPtr.dataLocation());
    }
    
    static void replaceWithLoad(CodeLocationConvertibleLoad label)
    {
        AssemblerType::replaceWithLoad(label.dataLocation());
    }
    
    static void replaceWithAddressComputation(CodeLocationConvertibleLoad label)
    {
        AssemblerType::replaceWithAddressComputation(label.dataLocation());
    }
};

} // namespace JSC

#endif // ENABLE(ASSEMBLER)

#endif // AbstractMacroAssembler_h
