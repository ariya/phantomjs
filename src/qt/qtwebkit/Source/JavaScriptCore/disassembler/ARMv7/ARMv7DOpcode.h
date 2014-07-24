/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#ifndef ARMv7DOpcode_h
#define ARMv7DOpcode_h

#if USE(ARMV7_DISASSEMBLER)

#include <stdint.h>
#include <wtf/Assertions.h>

namespace JSC { namespace ARMv7Disassembler {

class ARMv7DOpcode {
public:
    static void init();

    ARMv7DOpcode()
        : m_opcode(0)
        , m_bufferOffset(0)
    {
        init();

        for (unsigned i = 0; i < 4; i++)
            m_ifThenConditions[i] = CondNone;

        endITBlock();

        m_formatBuffer[0] = '\0';
    }

    const char* disassemble(uint16_t*& currentPC);

protected:
    const unsigned RegSP = 0xd;
    const unsigned RegLR = 0xe;
    const unsigned RegPC = 0xf;

    void fetchOpcode(uint16_t*&);
    bool is32BitInstruction() { return (m_opcode & 0xfffff800) > 0xe000; }
    bool isFPInstruction() { return (m_opcode & 0xfc000e00) == 0xec000a00; }

    static const char* const s_conditionNames[16];
    static const char* const s_shiftNames[4];
    static const char* const s_optionName[8];
    static const char* const s_specialRegisterNames[3];

    static const char* conditionName(unsigned condition) { return s_conditionNames[condition & 0xf]; }
    static const char* shiftName(unsigned shiftValue) { return s_shiftNames[shiftValue & 0x3]; }

    bool inITBlock() { return m_ITConditionIndex < m_ITBlocksize; }
    bool startingITBlock() { return m_ITConditionIndex == m_ITBlocksize + 1; }

    void startITBlock(unsigned, unsigned);
    void saveITConditionAt(unsigned, unsigned);
    void endITBlock()
    {
        m_currentITCondition = CondNone;
        m_ITConditionIndex = 0;
        m_ITBlocksize = 0;
    }

    void bufferPrintf(const char* format, ...) WTF_ATTRIBUTE_PRINTF(2, 3);
    void appendInstructionName(const char*, bool addS = false);

    void appendInstructionNameNoITBlock(const char* instructionName)
    {
        bufferPrintf("   %-7.7s", instructionName);
    }

    void appendRegisterName(unsigned);
    void appendRegisterList(unsigned);
    void appendFPRegisterName(char, unsigned);

    void appendSeparator()
    {
        bufferPrintf(", ");
    }

    void appendCharacter(const char c)
    {
        bufferPrintf("%c", c);
    }

    void appendString(const char* string)
    {
        bufferPrintf("%s", string);
    }

    void appendShiftType(unsigned shiftValue)
    {
        bufferPrintf("%s ", shiftName(shiftValue));
    }

    void appendSignedImmediate(int immediate)
    {
        bufferPrintf("#%d", immediate);
    }

    void appendUnsignedImmediate(unsigned immediate)
    {
        bufferPrintf("#%u", immediate);
    }

    void appendPCRelativeOffset(int32_t immediate)
    {
        bufferPrintf("0x%x", reinterpret_cast<uint32_t>(m_currentPC + immediate));
    }

    void appendShiftAmount(unsigned amount)
    {
        bufferPrintf("lsl #%u", 16 * amount);
    }

    static const int bufferSize = 81;
    static const unsigned char CondNone = 0xe;
    static const unsigned MaxITBlockSize = 4;

    char m_formatBuffer[bufferSize];
    unsigned char m_ifThenConditions[MaxITBlockSize];
    uint16_t* m_currentPC;
    uint32_t m_opcode;
    int m_bufferOffset;
    int m_currentITCondition;
    unsigned m_ITConditionIndex;
    unsigned m_ITBlocksize;

private:
    static bool s_initialized;
};

#define DEFINE_STATIC_FORMAT16(klass, thisObj) \
    static const char* format(ARMv7D16BitOpcode* thisObj) { return reinterpret_cast< klass *>(thisObj)->format(); }

class ARMv7D16BitOpcode : public ARMv7DOpcode {
private:
    class OpcodeGroup {
    public:
        OpcodeGroup(uint16_t opcodeMask, uint16_t opcodePattern, const char* (*format)(ARMv7D16BitOpcode*))
            : m_opcodeMask(opcodeMask)
            , m_opcodePattern(opcodePattern)
            , m_format(format)
            , m_next(0)
        {
        }

        void setNext(OpcodeGroup* next)
        {
            m_next = next;
        }

        OpcodeGroup* next()
        {
            return m_next;
        }

        bool matches(uint16_t opcode)
        {
            return (opcode & m_opcodeMask) == m_opcodePattern;
        }

        const char* format(ARMv7D16BitOpcode* thisObj)
        {
            return m_format(thisObj);
        }

    public:
        static const unsigned opcodeTableSize = 32;
        static const unsigned opcodeTableMask = opcodeTableSize-1;

        // private:
        uint16_t m_opcodeMask;
        uint16_t m_opcodePattern;
        const char* (*m_format)(ARMv7D16BitOpcode*);
        OpcodeGroup* m_next;
    };

public:
    static void init();

    const char* defaultFormat();
    const char* doDisassemble();

protected:
    unsigned rm() { return (m_opcode >> 3) & 0x7; }
    unsigned rd() { return m_opcode & 0x7; }
    unsigned opcodeGroupNumber(unsigned opcode) { return (opcode >> 11) & OpcodeGroup::opcodeTableMask; }

private:
    static OpcodeGroup* opcodeTable[OpcodeGroup::opcodeTableSize];
};

class ARMv7DOpcodeAddRegisterT2 : public ARMv7D16BitOpcode {
public:
    static const uint16_t s_mask = 0xff00;
    static const uint16_t s_pattern = 0x4400;

    DEFINE_STATIC_FORMAT16(ARMv7DOpcodeAddRegisterT2, thisObj);

protected:
    const char* format();

    unsigned rdn() { return ((m_opcode >> 4) & 0x8) | (m_opcode & 0x7); }
    unsigned rm() { return ((m_opcode >> 3) & 0xf); }
};

class ARMv7DOpcodeAddSPPlusImmediate : public ARMv7D16BitOpcode {
public:
    static const uint16_t s_mask = 0xf800;
    static const uint16_t s_pattern = 0xc800;

    DEFINE_STATIC_FORMAT16(ARMv7DOpcodeAddSPPlusImmediate, thisObj);

protected:
    const char* format();

    unsigned rd() { return (m_opcode >> 8) & 0x7; }
    unsigned immediate8() { return m_opcode & 0x0ff; }
};

class ARMv7DOpcodeAddSubtract : public ARMv7D16BitOpcode {
protected:
    static const char* const s_opNames[2];
};

class ARMv7DOpcodeAddSubtractT1 : public ARMv7DOpcodeAddSubtract {
public:
    static const uint16_t s_mask = 0xfc00;
    static const uint16_t s_pattern = 0x1800;

    DEFINE_STATIC_FORMAT16(ARMv7DOpcodeAddSubtractT1, thisObj);

protected:
    const char* format();

    const char* opName() { return s_opNames[op()]; }

    unsigned op() { return (m_opcode >> 9) & 0x1; }
    unsigned rm() { return (m_opcode >> 6) & 0x7; }
    unsigned rn() { return (m_opcode >> 3) & 0x7; }
};

class ARMv7DOpcodeAddSubtractImmediate3 : public ARMv7DOpcodeAddSubtract {
public:
    static const uint16_t s_mask = 0xfc00;
    static const uint16_t s_pattern = 0x1c00;

    DEFINE_STATIC_FORMAT16(ARMv7DOpcodeAddSubtractImmediate3, thisObj);

protected:
    const char* format();

    const char* opName() { return s_opNames[op()]; }

    unsigned op() { return (m_opcode >> 9) & 0x1; }
    unsigned immediate3() { return (m_opcode >> 6) & 0x3; }
    unsigned rn() { return (m_opcode >> 3) & 0x7; }
};

class ARMv7DOpcodeAddSubtractImmediate8 : public ARMv7DOpcodeAddSubtract {
public:
    static const uint16_t s_mask = 0xf000;
    static const uint16_t s_pattern = 0x3000;

    DEFINE_STATIC_FORMAT16(ARMv7DOpcodeAddSubtractImmediate8, thisObj);

protected:
    const char* format();

    const char* opName() { return s_opNames[op()]; }

    unsigned op() { return (m_opcode >> 11) & 0x1; }
    unsigned rdn() { return (m_opcode >> 8) & 0x7; }
    unsigned immediate8() { return m_opcode & 0xff; }
};

class ARMv7DOpcodeBranchConditionalT1 : public ARMv7D16BitOpcode {
public:
    static const uint16_t s_mask = 0xf000;
    static const uint16_t s_pattern = 0xd000;

    DEFINE_STATIC_FORMAT16(ARMv7DOpcodeBranchConditionalT1, thisObj);

protected:
    const char* format();

    unsigned condition() { return (m_opcode >> 8) & 0xf; }
    int offset() { return static_cast<int>(m_opcode & 0xff); }
};

class ARMv7DOpcodeBranchExchangeT1 : public ARMv7D16BitOpcode {
public:
    static const uint16_t s_mask = 0xff00;
    static const uint16_t s_pattern = 0x4700;

    DEFINE_STATIC_FORMAT16(ARMv7DOpcodeBranchExchangeT1, thisObj);

protected:
    const char* format();

    const char* opName() { return (m_opcode & 0x80) ? "blx" : "bx"; }
    unsigned rm() { return ((m_opcode >> 3) & 0xf); }
};

class ARMv7DOpcodeBranchT2 : public ARMv7D16BitOpcode {
public:
    static const uint16_t s_mask = 0xf800;
    static const uint16_t s_pattern = 0xe000;

    DEFINE_STATIC_FORMAT16(ARMv7DOpcodeBranchT2, thisObj);

protected:
    const char* format();

    int immediate11() { return static_cast<int>(m_opcode & 0x7ff); }
};

class ARMv7DOpcodeCompareImmediateT1 : public ARMv7D16BitOpcode {
public:
    static const uint16_t s_mask = 0xf800;
    static const uint16_t s_pattern = 0x2800;

    DEFINE_STATIC_FORMAT16(ARMv7DOpcodeCompareImmediateT1, thisObj);

protected:
    const char* format();

    unsigned rn() { return (m_opcode >> 8) & 0x3; }
    unsigned immediate8() { return m_opcode & 0xff; }
};

class ARMv7DOpcodeCompareRegisterT1 : public ARMv7D16BitOpcode {
public:
    static const uint16_t s_mask = 0xffc0;
    static const uint16_t s_pattern = 0x4280;

    DEFINE_STATIC_FORMAT16(ARMv7DOpcodeCompareRegisterT1, thisObj);

protected:
    const char* format();

    unsigned rn() { return m_opcode & 0x7; }
};

class ARMv7DOpcodeCompareRegisterT2 : public ARMv7D16BitOpcode {
public:
    static const uint16_t s_mask = 0xff00;
    static const uint16_t s_pattern = 0x4500;

    DEFINE_STATIC_FORMAT16(ARMv7DOpcodeCompareRegisterT2, thisObj);

protected:
    const char* format();

    unsigned rn() { return ((m_opcode >> 4) & 0x8) | (m_opcode & 0x7); }
    unsigned rm() { return ((m_opcode >> 3) & 0xf); }
};

class ARMv7DOpcodeDataProcessingRegisterT1 : public ARMv7D16BitOpcode {
private:
    static const char* const s_opNames[16];

public:
    static const uint16_t s_mask = 0xfc00;
    static const uint16_t s_pattern = 0x4000;

    DEFINE_STATIC_FORMAT16(ARMv7DOpcodeDataProcessingRegisterT1, thisObj);

protected:
    const char* format();

    const char* opName() { return s_opNames[op()]; }

    unsigned op() { return (m_opcode >> 6) & 0xf; }

    unsigned rm() { return (m_opcode >> 3) & 0x7; }
    unsigned rdn() { return m_opcode & 0x7; }
};

class ARMv7DOpcodeGeneratePCRelativeAddress : public ARMv7D16BitOpcode {
public:
    static const uint16_t s_mask = 0xf800;
    static const uint16_t s_pattern = 0xa000;

    DEFINE_STATIC_FORMAT16(ARMv7DOpcodeGeneratePCRelativeAddress, thisObj);

protected:
    const char* format();

    unsigned rd() { return (m_opcode >> 8) & 0x7; }
    unsigned immediate8() { return m_opcode & 0x0ff; }
};

class ARMv7DOpcodeLoadFromLiteralPool : public ARMv7D16BitOpcode {
public:
    static const uint16_t s_mask = 0xf800;
    static const uint16_t s_pattern = 0x4800;

    DEFINE_STATIC_FORMAT16(ARMv7DOpcodeLoadFromLiteralPool, thisObj);

protected:
    const char* format();

    unsigned rt() { return (m_opcode >> 8) & 0x7; }
    unsigned immediate8() { return m_opcode & 0x0ff; }
};

class ARMv7DOpcodeLoadStoreRegisterImmediate : public ARMv7D16BitOpcode {
private:
    static const char* const s_opNames[6];

public:
    const char* format();

protected:
    const char* opName() { return s_opNames[op()]; }

    unsigned op() { return ((m_opcode >> 11) & 0x1f) - 0xc; }
    unsigned immediate5() { return (m_opcode >> 6) & 0x01f; }
    unsigned rn() { return (m_opcode >> 3) & 0x7; }
    unsigned rt() { return m_opcode & 0x7; }
    unsigned scale() { return 2 - (op() >> 1); }
};

class ARMv7DOpcodeLoadStoreRegisterImmediateWordAndByte : public ARMv7DOpcodeLoadStoreRegisterImmediate {
public:
    static const uint16_t s_mask = 0xe000;
    static const uint16_t s_pattern = 0x6000;

    DEFINE_STATIC_FORMAT16(ARMv7DOpcodeLoadStoreRegisterImmediate, thisObj);
};

class ARMv7DOpcodeLoadStoreRegisterImmediateHalfWord : public ARMv7DOpcodeLoadStoreRegisterImmediate {
public:
    static const uint16_t s_mask = 0xf800;
    static const uint16_t s_pattern = 0x8000;

    DEFINE_STATIC_FORMAT16(ARMv7DOpcodeLoadStoreRegisterImmediate, thisObj);
};

class ARMv7DOpcodeLoadStoreRegisterOffsetT1 : public ARMv7D16BitOpcode {
private:
    static const char* const s_opNames[8];

public:
    static const uint16_t s_mask = 0xf000;
    static const uint16_t s_pattern = 0x5000;

    DEFINE_STATIC_FORMAT16(ARMv7DOpcodeLoadStoreRegisterOffsetT1, thisObj);

protected:
    const char* format();

    const char* opName() { return s_opNames[opB()]; }

    unsigned opB() { return (m_opcode >> 9) & 0x7; }
    unsigned rm() { return (m_opcode >> 6) & 0x7; }
    unsigned rn() { return (m_opcode >> 3) & 0x7; }
    unsigned rt() { return m_opcode & 0x7; }
};

class ARMv7DOpcodeLoadStoreRegisterSPRelative : public ARMv7D16BitOpcode {
private:
    static const char* const s_opNames[8];

public:
    static const uint16_t s_mask = 0xf000;
    static const uint16_t s_pattern = 0x9000;

    DEFINE_STATIC_FORMAT16(ARMv7DOpcodeLoadStoreRegisterSPRelative, thisObj);

protected:
    const char* format();

    const char* opName() { return op() ? "ldr" : "str"; }

    unsigned op() { return (m_opcode >> 11) & 0x1; }
    unsigned rt() { return (m_opcode >> 8) & 0x7; }
    unsigned immediate8() { return m_opcode & 0xff; }
};

class ARMv7DOpcodeLogicalImmediateT1 : public ARMv7D16BitOpcode {
public:
    static const uint16_t s_mask = 0xe000;
    static const uint16_t s_pattern = 0x0000;

    DEFINE_STATIC_FORMAT16(ARMv7DOpcodeLogicalImmediateT1, thisObj);

protected:
    const char* format();

    const char* opName() { return shiftName(op()); }

    unsigned op() { return (m_opcode >> 12) & 0x3; }
    unsigned immediate5() { return (m_opcode >> 6) & 0x1f; }
};

class ARMv7DOpcodeMiscAddSubSP : public ARMv7D16BitOpcode {
public:
    static const uint16_t s_mask = 0xff00;
    static const uint16_t s_pattern = 0xb000;

    DEFINE_STATIC_FORMAT16(ARMv7DOpcodeMiscAddSubSP, thisObj);

protected:
    const char* format();

    const char* opName() { return op() ? "sub" : "add"; }
    unsigned op() { return (m_opcode >> 7) & 0x1; }
    unsigned immediate7() { return m_opcode & 0x7f; }
};

class ARMv7DOpcodeMiscByteHalfwordOps : public ARMv7D16BitOpcode {
private:
    static const char* const s_opNames[8];

public:
    static const uint16_t s_mask = 0xf700;
    static const uint16_t s_pattern = 0xb200;

    DEFINE_STATIC_FORMAT16(ARMv7DOpcodeMiscByteHalfwordOps, thisObj);

protected:
    const char* format();

    const char* opName() { return s_opNames[op()]; }
    unsigned op() { return ((m_opcode >> 9) & 0x4) || ((m_opcode >> 6) & 0x3); }
};

class ARMv7DOpcodeMiscBreakpointT1 : public ARMv7D16BitOpcode {
public:
    static const uint16_t s_mask = 0xff00;
    static const uint16_t s_pattern = 0xbe00;

    DEFINE_STATIC_FORMAT16(ARMv7DOpcodeMiscBreakpointT1, thisObj);

protected:
    const char* format();

    unsigned immediate8() { return m_opcode & 0xff; }
};

class ARMv7DOpcodeMiscCompareAndBranch : public ARMv7D16BitOpcode {
public:
    static const uint16_t s_mask = 0xf500;
    static const uint16_t s_pattern = 0xb100;

    DEFINE_STATIC_FORMAT16(ARMv7DOpcodeMiscCompareAndBranch, thisObj);

protected:
    const char* format();

    const char* opName() { return op() ? "cbnz" : "cbz"; }
    unsigned op() { return (m_opcode >> 11) & 0x1; }
    int32_t immediate6() { return ((m_opcode >> 4) & 0x20) | ((m_opcode >> 3) & 0x1f); }
    unsigned rn() { return m_opcode & 0x7; }
};

class ARMv7DOpcodeMiscHint16 : public ARMv7D16BitOpcode {
private:
    static const char* const s_opNames[16];

public:
    static const uint16_t s_mask = 0xff0f;
    static const uint16_t s_pattern = 0xbf00;

    DEFINE_STATIC_FORMAT16(ARMv7DOpcodeMiscHint16, thisObj);

protected:
    const char* format();

    const char* opName() { return s_opNames[opA()]; }
    unsigned opA() { return (m_opcode >> 4) & 0xf; }
};

class ARMv7DOpcodeMiscIfThenT1 : public ARMv7D16BitOpcode {
public:
    static const uint16_t s_mask = 0xff00;
    static const uint16_t s_pattern = 0xbf00;

    DEFINE_STATIC_FORMAT16(ARMv7DOpcodeMiscIfThenT1, thisObj);

protected:
    const char* format();

    unsigned firstCondition() { return (m_opcode >> 4) & 0xf; }
    unsigned mask() { return m_opcode & 0xf; }
};

class ARMv7DOpcodeMiscPushPop : public ARMv7D16BitOpcode {
public:
    static const uint16_t s_mask = 0xf600;
    static const uint16_t s_pattern = 0xb400;

    DEFINE_STATIC_FORMAT16(ARMv7DOpcodeMiscPushPop, thisObj);

protected:
    const char* format();

    const char* opName() { return op() ? "pop" : "push"; }
    unsigned op() { return (m_opcode >> 11) & 0x1; }
    unsigned registerMask() { return ((m_opcode << 6) & 0x4000) | (m_opcode & 0x7f); }
};

class ARMv7DOpcodeMoveImmediateT1 : public ARMv7D16BitOpcode {
public:
    static const uint16_t s_mask = 0xf800;
    static const uint16_t s_pattern = 0x2000;

    DEFINE_STATIC_FORMAT16(ARMv7DOpcodeMoveImmediateT1, thisObj);

protected:
    const char* format();

    unsigned rd() { return (m_opcode >> 8) & 0x3; }
    unsigned immediate8() { return m_opcode & 0xff; }
};

class ARMv7DOpcodeMoveRegisterT1 : public ARMv7D16BitOpcode {
public:
    static const uint16_t s_mask = 0xff00;
    static const uint16_t s_pattern = 0x4600;

    DEFINE_STATIC_FORMAT16(ARMv7DOpcodeMoveRegisterT1, thisObj);

protected:
    const char* format();

    unsigned rd() { return ((m_opcode >> 4) & 0x8) | (m_opcode & 0x7); }
    unsigned rm() { return ((m_opcode >> 3) & 0xf); }
};

// 32 Bit instructions

#define DEFINE_STATIC_FORMAT32(klass, thisObj) \
    static const char* format(ARMv7D32BitOpcode* thisObj) { return reinterpret_cast< klass *>(thisObj)->format(); }

class ARMv7D32BitOpcode : public ARMv7DOpcode {
private:
    class OpcodeGroup {
    public:
        OpcodeGroup(uint32_t opcodeMask, uint32_t opcodePattern, const char* (*format)(ARMv7D32BitOpcode*))
            : m_opcodeMask(opcodeMask)
            , m_opcodePattern(opcodePattern)
            , m_format(format)
            , m_next(0)
        {
        }

        void setNext(OpcodeGroup* next)
        {
            m_next = next;
        }

        OpcodeGroup* next()
        {
            return m_next;
        }

        bool matches(uint32_t opcode)
        {
            return (opcode & m_opcodeMask) == m_opcodePattern;
        }

        const char* format(ARMv7D32BitOpcode* thisObj)
        {
            return m_format(thisObj);
        }

    public:
        static const unsigned opcodeTableSize = 16;
        static const unsigned opcodeTableMask = opcodeTableSize-1;

    private:
        uint32_t m_opcodeMask;
        uint32_t m_opcodePattern;
        const char* (*m_format)(ARMv7D32BitOpcode*);
        OpcodeGroup* m_next;
    };

public:
    static void init();

    const char* defaultFormat();
    const char* doDisassemble();

protected:
    unsigned rd() { return (m_opcode >> 8) & 0xf; }
    unsigned rm() { return m_opcode & 0xf; }
    unsigned rn() { return (m_opcode >> 16) & 0xf; }
    unsigned rt() { return (m_opcode >> 12) & 0xf; }

    unsigned opcodeGroupNumber(unsigned opcode) { return (opcode >> 25) & OpcodeGroup::opcodeTableMask; }

private:
    static OpcodeGroup* opcodeTable[OpcodeGroup::opcodeTableSize];
};

class ARMv7DOpcodeBranchRelative : public ARMv7D32BitOpcode {
protected:
    unsigned sBit() { return (m_opcode >> 26) & 0x1; }
    unsigned j1() { return (m_opcode >> 13) & 0x1; }
    unsigned j2() { return (m_opcode >> 11) & 0x1; }
    unsigned immediate11() { return m_opcode & 0x7ff; }
};

class ARMv7DOpcodeConditionalBranchT3 : public ARMv7DOpcodeBranchRelative {
public:
    static const uint32_t s_mask = 0xf800d000;
    static const uint32_t s_pattern = 0xf0008000;

    DEFINE_STATIC_FORMAT32(ARMv7DOpcodeConditionalBranchT3, thisObj);

protected:
    const char* format();

    int32_t offset() { return ((static_cast<int32_t>(sBit() << 31)) >> 12) | static_cast<int32_t>((j1() << 18) | (j2() << 17) | (immediate6() << 11) | immediate11()); }
    unsigned condition() { return (m_opcode >> 22) & 0xf; }
    unsigned immediate6() { return (m_opcode >> 16) & 0x3f; }
};

class ARMv7DOpcodeBranchOrBranchLink : public ARMv7DOpcodeBranchRelative {
public:
    static const uint32_t s_mask = 0xf8009000;
    static const uint32_t s_pattern = 0xf0009000;

    DEFINE_STATIC_FORMAT32(ARMv7DOpcodeBranchOrBranchLink, thisObj);

protected:
    const char* format();

    int32_t offset() { return ((static_cast<int32_t>(sBit() << 31)) >> 8) | static_cast<int32_t>((~(j1() ^ sBit()) << 22) | (~(j2() ^ sBit()) << 21) | (immediate10() << 11) | immediate11()); }
    unsigned immediate10() { return (m_opcode >> 16) & 0x3ff; }
    bool isBL() { return !!((m_opcode >> 14) & 0x1); }
};

class ARMv7DOpcodeDataProcessingLogicalAndRithmetic : public ARMv7D32BitOpcode {
protected:
    static const char* const s_opNames[16];
};

class ARMv7DOpcodeDataProcessingModifiedImmediate : public ARMv7DOpcodeDataProcessingLogicalAndRithmetic {
private:
    void appendImmShift(unsigned, unsigned);

public:
    static const uint32_t s_mask = 0xfa008000;
    static const uint32_t s_pattern = 0xf0000000;

    DEFINE_STATIC_FORMAT32(ARMv7DOpcodeDataProcessingModifiedImmediate, thisObj);

protected:
    const char* format();
    void appendModifiedImmediate(unsigned);

    const char* opName() { return s_opNames[op()]; }

    unsigned op() { return (m_opcode >> 21) & 0xf; }
    unsigned sBit() { return (m_opcode >> 20) & 0x1; }
    unsigned immediate12() { return ((m_opcode >> 15) & 0x0800) | ((m_opcode >> 4) & 0x0700) | (m_opcode & 0x00ff); }
};

class ARMv7DOpcodeDataProcessingShiftedReg : public ARMv7DOpcodeDataProcessingLogicalAndRithmetic {
private:
    void appendImmShift(unsigned, unsigned);

public:
    static const uint32_t s_mask = 0xfe000000;
    static const uint32_t s_pattern = 0xea000000;

    DEFINE_STATIC_FORMAT32(ARMv7DOpcodeDataProcessingShiftedReg, thisObj);

protected:
    const char* format();

    const char* opName() { return s_opNames[op()]; }

    unsigned sBit() { return (m_opcode >> 20) & 0x1; }
    unsigned op() { return (m_opcode >> 21) & 0xf; }
    unsigned immediate5() { return ((m_opcode >> 10) & 0x1c) | ((m_opcode >> 6) & 0x3); }
    unsigned type() { return (m_opcode >> 4) & 0x3; }
    unsigned tbBit() { return (m_opcode >> 5) & 0x1; }
    unsigned tBit() { return (m_opcode >> 4) & 0x1; }
};

class ARMv7DOpcodeDataProcessingReg : public ARMv7D32BitOpcode {
protected:
    unsigned op1() { return (m_opcode >> 20) & 0xf; }
    unsigned op2() { return (m_opcode >> 4) & 0xf; }
};

class ARMv7DOpcodeDataProcessingRegShift : public ARMv7DOpcodeDataProcessingReg {
public:
    static const uint32_t s_mask = 0xffe0f0f0;
    static const uint32_t s_pattern = 0xfa00f000;

    DEFINE_STATIC_FORMAT32(ARMv7DOpcodeDataProcessingRegShift, thisObj);

protected:
    const char* format();

    const char* opName() { return shiftName((op1() >> 1) & 0x3); }
};

class ARMv7DOpcodeDataProcessingRegExtend : public ARMv7DOpcodeDataProcessingReg {
private:
    static const char* const s_opExtendNames[8];
    static const char* const s_opExtendAndAddNames[8];

public:
    static const uint32_t s_mask = 0xff80f0c0;
    static const uint32_t s_pattern = 0xfa00f080;

    DEFINE_STATIC_FORMAT32(ARMv7DOpcodeDataProcessingRegExtend, thisObj);

protected:
    const char* format();

    const char* opExtendName() { return s_opExtendNames[op1()]; }
    const char* opExtendAndAddName() { return s_opExtendAndAddNames[op1()]; }
    unsigned rotate() { return (m_opcode >> 4) & 0x3; }
};

class ARMv7DOpcodeDataProcessingRegParallel : public ARMv7DOpcodeDataProcessingReg {
private:
    static const char* const s_opNames[16];

public:
    static const uint32_t s_mask = 0xff80f0e0;
    static const uint32_t s_pattern = 0xfa00f000;

    DEFINE_STATIC_FORMAT32(ARMv7DOpcodeDataProcessingRegParallel, thisObj);

protected:
    const char* format();

    const char* opName() { return s_opNames[((op1() & 0x7) << 1) | (op2() & 0x1)]; }
};

class ARMv7DOpcodeDataProcessingRegMisc : public ARMv7DOpcodeDataProcessingReg {
private:
    static const char* const s_opNames[16];

public:
    static const uint32_t s_mask = 0xffc0f0c0;
    static const uint32_t s_pattern = 0xfa80f080;

    DEFINE_STATIC_FORMAT32(ARMv7DOpcodeDataProcessingRegMisc, thisObj);

protected:
    const char* format();

    const char* opName() { return s_opNames[((op1() & 0x3) << 2) | (op2() & 0x3)]; }
};

class ARMv7DOpcodeHint32 : public ARMv7D32BitOpcode {
private:
    static const char* const s_opNames[8];

public:
    static const uint32_t s_mask = 0xfff0d000;
    static const uint32_t s_pattern = 0xf3a08000;

    DEFINE_STATIC_FORMAT32(ARMv7DOpcodeHint32, thisObj);

protected:
    const char* format();

    const char* opName() { return s_opNames[op()]; }

    bool isDebugHint() { return (m_opcode & 0xf0) == 0xf0; }
    unsigned debugOption() { return m_opcode & 0xf; }
    unsigned op() { return m_opcode & 0x7; }
};

class ARMv7DOpcodeFPTransfer : public ARMv7D32BitOpcode {
public:
    static const uint32_t s_mask = 0xffc00e7f;
    static const uint32_t s_pattern = 0xee000a10;

    DEFINE_STATIC_FORMAT32(ARMv7DOpcodeFPTransfer, thisObj);

protected:
    const char* format();

    void appendFPRegister();

    unsigned opH() { return (m_opcode >> 21) & 0x1; }
    unsigned opL() { return (m_opcode >> 20) & 0x1; }
    unsigned rt() { return (m_opcode >> 12) & 0xf; }
    unsigned opC() { return (m_opcode >> 8) & 0x1; }
    unsigned opB() { return (m_opcode >> 5) & 0x3; }
    unsigned vd() { return ((m_opcode >> 3) & 0x10) | ((m_opcode >> 16) & 0xf); }
    unsigned vn() { return ((m_opcode >> 7) & 0x1) | ((m_opcode >> 15) & 0x1e); }
};

class ARMv7DOpcodeDataLoad : public ARMv7D32BitOpcode {
protected:
    static const char* const s_opNames[8];

protected:
    const char* opName() { return s_opNames[op()]; }

    unsigned op() { return ((m_opcode >> 22) & 0x4) | ((m_opcode >> 21) & 0x3); }
};

class ARMv7DOpcodeLoadRegister : public ARMv7DOpcodeDataLoad {
public:
    static const uint32_t s_mask = 0xfe900800;
    static const uint32_t s_pattern = 0xf8100000;

    DEFINE_STATIC_FORMAT32(ARMv7DOpcodeLoadRegister, thisObj);

protected:
    const char* format();

    unsigned immediate2() { return (m_opcode >> 4) & 0x3; }
};

class ARMv7DOpcodeLoadSignedImmediate : public ARMv7DOpcodeDataLoad {
public:
    static const uint32_t s_mask = 0xfe900800;
    static const uint32_t s_pattern = 0xf8100800;

    DEFINE_STATIC_FORMAT32(ARMv7DOpcodeLoadSignedImmediate, thisObj);

protected:
    const char* format();

    unsigned pBit() { return (m_opcode >> 10) & 0x1; }
    unsigned uBit() { return (m_opcode >> 9) & 0x1; }
    unsigned wBit() { return (m_opcode >> 8) & 0x1; }
    unsigned immediate8() { return m_opcode & 0xff; }
};

class ARMv7DOpcodeLoadUnsignedImmediate : public ARMv7DOpcodeDataLoad {
public:
    static const uint32_t s_mask = 0xfe900000;
    static const uint32_t s_pattern = 0xf8900000;

    DEFINE_STATIC_FORMAT32(ARMv7DOpcodeLoadUnsignedImmediate, thisObj);

protected:
    const char* format();

    unsigned immediate12() { return m_opcode & 0xfff; }
};

class ARMv7DOpcodeLongMultipleDivide : public ARMv7D32BitOpcode {
protected:
    static const char* const s_opNames[8];
    static const char* const s_smlalOpNames[4];
    static const char* const s_smlaldOpNames[2];
    static const char* const s_smlsldOpNames[2];

public:
    static const uint32_t s_mask = 0xff800000;
    static const uint32_t s_pattern = 0xfb800000;

    DEFINE_STATIC_FORMAT32(ARMv7DOpcodeLongMultipleDivide, thisObj);

protected:
    const char* format();

    const char* opName() { return s_opNames[op1()]; }
    const char* smlalOpName() { return s_smlalOpNames[(nBit() << 1) | mBit()]; }
    const char* smlaldOpName() { return s_smlaldOpNames[mBit()]; }
    const char* smlsldOpName() { return s_smlsldOpNames[mBit()]; }

    unsigned rdLo() { return rt(); }
    unsigned rdHi() { return rd(); }
    unsigned op1() { return (m_opcode >> 20) & 0x7; }
    unsigned op2() { return (m_opcode >> 4) & 0xf; }
    unsigned nBit() { return (m_opcode >> 5) & 0x1; }
    unsigned mBit() { return (m_opcode >> 4) & 0x1; }
};

class ARMv7DOpcodeDataPushPopSingle : public ARMv7D32BitOpcode {
public:
    static const uint32_t s_mask = 0xffef0fff;
    static const uint32_t s_pattern = 0xf84d0d04;

    DEFINE_STATIC_FORMAT32(ARMv7DOpcodeDataPushPopSingle, thisObj);

protected:
    const char* format();

    const char* opName() { return op() ? "pop" : "push"; }
    unsigned op() { return (m_opcode >> 20) & 0x1; }
};

class ARMv7DOpcodeDataStoreSingle : public ARMv7D32BitOpcode {
protected:
    static const char* const s_opNames[4];

protected:
    const char* opName() { return s_opNames[op()]; }

    unsigned op() { return (m_opcode >> 21) & 0x3; }
};

class ARMv7DOpcodeStoreSingleImmediate12 : public ARMv7DOpcodeDataStoreSingle {
public:
    static const uint32_t s_mask = 0xfff00000;
    static const uint32_t s_pattern = 0xf8c00000;

    DEFINE_STATIC_FORMAT32(ARMv7DOpcodeStoreSingleImmediate12, thisObj);

    const char* format();

protected:
    unsigned immediate12() { return m_opcode & 0xfff; }
};

class ARMv7DOpcodeStoreSingleImmediate8 : public ARMv7DOpcodeDataStoreSingle {
public:
    static const uint32_t s_mask = 0xfff00800;
    static const uint32_t s_pattern = 0xf8400800;

    DEFINE_STATIC_FORMAT32(ARMv7DOpcodeStoreSingleImmediate8, thisObj);

    const char* format();

protected:
    unsigned pBit() { return (m_opcode >> 10) & 0x1; }
    unsigned uBit() { return (m_opcode >> 9) & 0x1; }
    unsigned wBit() { return (m_opcode >> 8) & 0x1; }
    unsigned immediate8() { return m_opcode & 0xff; }
};

class ARMv7DOpcodeStoreSingleRegister : public ARMv7DOpcodeDataStoreSingle {
public:
    static const uint32_t s_mask = 0xfff00fc0;
    static const uint32_t s_pattern = 0xf8400000;

    DEFINE_STATIC_FORMAT32(ARMv7DOpcodeStoreSingleRegister, thisObj);

protected:
    const char* format();

    unsigned immediate2() { return (m_opcode >> 4) & 0x3; }
};

class ARMv7DOpcodeUnmodifiedImmediate : public ARMv7D32BitOpcode {
protected:
    static const char* const s_opNames[16];

public:
    static const uint32_t s_mask = 0xfa008000;
    static const uint32_t s_pattern = 0xf2000000;

    DEFINE_STATIC_FORMAT32(ARMv7DOpcodeUnmodifiedImmediate, thisObj);

protected:
    const char* format();

    const char* opName() { return s_opNames[op() >> 1]; }

    unsigned op() { return (m_opcode >> 20) & 0x1f; }
    unsigned shBit() { return (m_opcode >> 21) & 0x1; }
    unsigned bitNumOrSatImmediate() { return m_opcode & 0x1f; }
    unsigned immediate5() { return ((m_opcode >> 9) & 0x1c) | ((m_opcode >> 6) & 0x3); }
    unsigned immediate12() { return ((m_opcode >> 15) & 0x0800) | ((m_opcode >> 4) & 0x0700) | (m_opcode & 0x00ff); }
    unsigned immediate16() { return ((m_opcode >> 4) & 0xf000) | ((m_opcode >> 15) & 0x0800) | ((m_opcode >> 4) & 0x0700) | (m_opcode & 0x00ff); }
};

class ARMv7DOpcodeVMOVDoublePrecision : public ARMv7D32BitOpcode {
public:
    static const uint32_t s_mask = 0xffe00fd0;
    static const uint32_t s_pattern = 0xec400b10;

    DEFINE_STATIC_FORMAT32(ARMv7DOpcodeVMOVDoublePrecision, thisObj);

protected:
    const char* format();

    unsigned op() { return (m_opcode >> 20) & 0x1; }
    unsigned rt2() { return (m_opcode >> 16) & 0xf; }
    unsigned rt() { return (m_opcode >> 16) & 0xf; }
    unsigned vm() { return (m_opcode & 0xf) | ((m_opcode >> 1) & 0x10); }
};

class ARMv7DOpcodeVMOVSinglePrecision : public ARMv7D32BitOpcode {
public:
    static const uint32_t s_mask = 0xffe00fd0;
    static const uint32_t s_pattern = 0xec400a10;

    DEFINE_STATIC_FORMAT32(ARMv7DOpcodeVMOVSinglePrecision, thisObj);

protected:
    const char* format();

    unsigned op() { return (m_opcode >> 20) & 0x1; }
    unsigned rt2() { return (m_opcode >> 16) & 0xf; }
    unsigned rt() { return (m_opcode >> 16) & 0xf; }
    unsigned vm() { return ((m_opcode << 1) & 0x1e) | ((m_opcode >> 5) & 0x1); }
};

class ARMv7DOpcodeVMSR : public ARMv7D32BitOpcode {
public:
    static const uint32_t s_mask = 0xffef0fff;
    static const uint32_t s_pattern = 0xeee10a10;

    DEFINE_STATIC_FORMAT32(ARMv7DOpcodeVMSR, thisObj);

protected:
    const char* format();

    unsigned opL() { return (m_opcode >> 20) & 0x1; }
    unsigned rt() { return (m_opcode >> 12) & 0xf; }
};


} } // namespace JSC::ARMv7Disassembler

using JSC::ARMv7Disassembler::ARMv7DOpcode;

#endif // #if USE(ARMV7_DISASSEMBLER)

#endif // ARMv7DOpcode_h
