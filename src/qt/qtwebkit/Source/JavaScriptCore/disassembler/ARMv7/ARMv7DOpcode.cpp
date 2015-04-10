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

#include "config.h"

#if USE(ARMV7_DISASSEMBLER)

#include "ARMv7DOpcode.h"

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>

namespace JSC { namespace ARMv7Disassembler {

ARMv7D16BitOpcode::OpcodeGroup* ARMv7D16BitOpcode::opcodeTable[32];
ARMv7D32BitOpcode::OpcodeGroup* ARMv7D32BitOpcode::opcodeTable[16];

const char* const ARMv7DOpcode::s_conditionNames[16] = {
    "eq", "ne", "hs", "lo", "mi", "pl", "vs", "vc",
    "hi", "ls", "ge", "lt", "gt", "le", "al", "al"
};

const char* const ARMv7DOpcode::s_optionName[8] = {
    "uxtb", "uxth", "uxtw", "uxtx", "sxtb", "sxth", "sxtw", "sxtx"
};

const char* const ARMv7DOpcode::s_shiftNames[4] = {
    "lsl", "lsr", "asl", "ror"
};

const char* const ARMv7DOpcode::s_specialRegisterNames[3] = { "sp", "lr", "pc" };

template <typename OpcodeType, typename InstructionType>
struct OpcodeGroupInitializer {
    unsigned m_opcodeGroupNumber;
    InstructionType m_mask;
    InstructionType m_pattern;
    const char* (*m_format)(OpcodeType*);
};

#define OPCODE_GROUP_ENTRY(groupIndex, groupClass) \
{ groupIndex, groupClass::s_mask, groupClass::s_pattern, groupClass::format }

typedef OpcodeGroupInitializer<ARMv7D16BitOpcode, uint16_t> Opcode16GroupInitializer;
typedef OpcodeGroupInitializer<ARMv7D32BitOpcode, uint32_t> Opcode32GroupInitializer;

static Opcode16GroupInitializer opcode16BitGroupList[] = {
    OPCODE_GROUP_ENTRY(0x0, ARMv7DOpcodeLogicalImmediateT1),
    OPCODE_GROUP_ENTRY(0x1, ARMv7DOpcodeLogicalImmediateT1),
    OPCODE_GROUP_ENTRY(0x2, ARMv7DOpcodeLogicalImmediateT1),
    OPCODE_GROUP_ENTRY(0x3, ARMv7DOpcodeAddSubtractT1),
    OPCODE_GROUP_ENTRY(0x3, ARMv7DOpcodeAddSubtractImmediate3),
    OPCODE_GROUP_ENTRY(0x4, ARMv7DOpcodeMoveImmediateT1),
    OPCODE_GROUP_ENTRY(0x5, ARMv7DOpcodeCompareImmediateT1),
    OPCODE_GROUP_ENTRY(0x6, ARMv7DOpcodeAddSubtractImmediate8),
    OPCODE_GROUP_ENTRY(0x7, ARMv7DOpcodeAddSubtractImmediate8),
    OPCODE_GROUP_ENTRY(0x8, ARMv7DOpcodeDataProcessingRegisterT1),
    OPCODE_GROUP_ENTRY(0x8, ARMv7DOpcodeAddRegisterT2),
    OPCODE_GROUP_ENTRY(0x8, ARMv7DOpcodeCompareRegisterT2),
    OPCODE_GROUP_ENTRY(0x8, ARMv7DOpcodeCompareRegisterT1),
    OPCODE_GROUP_ENTRY(0x8, ARMv7DOpcodeMoveRegisterT1),
    OPCODE_GROUP_ENTRY(0x8, ARMv7DOpcodeBranchExchangeT1),
    OPCODE_GROUP_ENTRY(0x9, ARMv7DOpcodeLoadFromLiteralPool),
    OPCODE_GROUP_ENTRY(0xa, ARMv7DOpcodeLoadStoreRegisterOffsetT1),
    OPCODE_GROUP_ENTRY(0xb, ARMv7DOpcodeLoadStoreRegisterOffsetT1),
    OPCODE_GROUP_ENTRY(0xc, ARMv7DOpcodeLoadStoreRegisterImmediateWordAndByte),
    OPCODE_GROUP_ENTRY(0xd, ARMv7DOpcodeLoadStoreRegisterImmediateWordAndByte),
    OPCODE_GROUP_ENTRY(0xe, ARMv7DOpcodeLoadStoreRegisterImmediateWordAndByte),
    OPCODE_GROUP_ENTRY(0xf, ARMv7DOpcodeLoadStoreRegisterImmediateWordAndByte),
    OPCODE_GROUP_ENTRY(0x10, ARMv7DOpcodeLoadStoreRegisterImmediateHalfWord),
    OPCODE_GROUP_ENTRY(0x11, ARMv7DOpcodeLoadStoreRegisterImmediateHalfWord),
    OPCODE_GROUP_ENTRY(0x12, ARMv7DOpcodeLoadStoreRegisterSPRelative),
    OPCODE_GROUP_ENTRY(0x13, ARMv7DOpcodeLoadStoreRegisterSPRelative),
    OPCODE_GROUP_ENTRY(0x14, ARMv7DOpcodeGeneratePCRelativeAddress),
    OPCODE_GROUP_ENTRY(0x15, ARMv7DOpcodeAddSPPlusImmediate),
    OPCODE_GROUP_ENTRY(0x16, ARMv7DOpcodeMiscCompareAndBranch),
    OPCODE_GROUP_ENTRY(0x16, ARMv7DOpcodeMiscByteHalfwordOps),
    OPCODE_GROUP_ENTRY(0x16, ARMv7DOpcodeMiscPushPop),
    OPCODE_GROUP_ENTRY(0x16, ARMv7DOpcodeMiscAddSubSP),
    OPCODE_GROUP_ENTRY(0x17, ARMv7DOpcodeMiscHint16), // Needs to be before IfThenT1
    OPCODE_GROUP_ENTRY(0x17, ARMv7DOpcodeMiscIfThenT1),
    OPCODE_GROUP_ENTRY(0x17, ARMv7DOpcodeMiscByteHalfwordOps),
    OPCODE_GROUP_ENTRY(0x17, ARMv7DOpcodeMiscCompareAndBranch),
    OPCODE_GROUP_ENTRY(0x17, ARMv7DOpcodeMiscPushPop),
    OPCODE_GROUP_ENTRY(0x17, ARMv7DOpcodeMiscBreakpointT1),
    OPCODE_GROUP_ENTRY(0x1a, ARMv7DOpcodeBranchConditionalT1),
    OPCODE_GROUP_ENTRY(0x1b, ARMv7DOpcodeBranchConditionalT1),
    OPCODE_GROUP_ENTRY(0x1c, ARMv7DOpcodeBranchT2)
};

static Opcode32GroupInitializer opcode32BitGroupList[] = {
    OPCODE_GROUP_ENTRY(0x5, ARMv7DOpcodeDataProcessingShiftedReg),
    OPCODE_GROUP_ENTRY(0x6, ARMv7DOpcodeVMOVSinglePrecision),
    OPCODE_GROUP_ENTRY(0x6, ARMv7DOpcodeVMOVDoublePrecision),
    OPCODE_GROUP_ENTRY(0x7, ARMv7DOpcodeFPTransfer),
    OPCODE_GROUP_ENTRY(0x7, ARMv7DOpcodeVMSR),
    OPCODE_GROUP_ENTRY(0x8, ARMv7DOpcodeDataProcessingModifiedImmediate),
    OPCODE_GROUP_ENTRY(0x8, ARMv7DOpcodeConditionalBranchT3),
    OPCODE_GROUP_ENTRY(0x8, ARMv7DOpcodeBranchOrBranchLink),
    OPCODE_GROUP_ENTRY(0x9, ARMv7DOpcodeUnmodifiedImmediate),
    OPCODE_GROUP_ENTRY(0x9, ARMv7DOpcodeHint32),
    OPCODE_GROUP_ENTRY(0x9, ARMv7DOpcodeConditionalBranchT3),
    OPCODE_GROUP_ENTRY(0x9, ARMv7DOpcodeBranchOrBranchLink),
    OPCODE_GROUP_ENTRY(0xa, ARMv7DOpcodeDataProcessingModifiedImmediate),
    OPCODE_GROUP_ENTRY(0xa, ARMv7DOpcodeConditionalBranchT3),
    OPCODE_GROUP_ENTRY(0xa, ARMv7DOpcodeBranchOrBranchLink),
    OPCODE_GROUP_ENTRY(0xb, ARMv7DOpcodeUnmodifiedImmediate),
    OPCODE_GROUP_ENTRY(0xb, ARMv7DOpcodeConditionalBranchT3),
    OPCODE_GROUP_ENTRY(0xb, ARMv7DOpcodeBranchOrBranchLink),
    OPCODE_GROUP_ENTRY(0xc, ARMv7DOpcodeLoadRegister),
    OPCODE_GROUP_ENTRY(0xc, ARMv7DOpcodeDataPushPopSingle), // Should be before StoreSingle*
    OPCODE_GROUP_ENTRY(0xc, ARMv7DOpcodeStoreSingleRegister),
    OPCODE_GROUP_ENTRY(0xc, ARMv7DOpcodeStoreSingleImmediate12),
    OPCODE_GROUP_ENTRY(0xc, ARMv7DOpcodeStoreSingleImmediate8),
    OPCODE_GROUP_ENTRY(0xc, ARMv7DOpcodeLoadSignedImmediate),
    OPCODE_GROUP_ENTRY(0xc, ARMv7DOpcodeLoadUnsignedImmediate),
    OPCODE_GROUP_ENTRY(0xd, ARMv7DOpcodeLongMultipleDivide),
    OPCODE_GROUP_ENTRY(0xd, ARMv7DOpcodeDataProcessingRegShift),
    OPCODE_GROUP_ENTRY(0xd, ARMv7DOpcodeDataProcessingRegExtend),
    OPCODE_GROUP_ENTRY(0xd, ARMv7DOpcodeDataProcessingRegParallel),
    OPCODE_GROUP_ENTRY(0xd, ARMv7DOpcodeDataProcessingRegMisc),
};

bool ARMv7DOpcode::s_initialized = false;

void ARMv7DOpcode::init()
{
    if (s_initialized)
        return;

    ARMv7D16BitOpcode::init();
    ARMv7D32BitOpcode::init();

    s_initialized = true;
}

void ARMv7DOpcode::startITBlock(unsigned blocksize, unsigned firstCondition)
{
    ASSERT(blocksize > 0 && blocksize <= MaxITBlockSize);
    m_ITBlocksize = blocksize;
    m_ITConditionIndex = m_ITBlocksize + 1;
    m_currentITCondition = 0;
    m_ifThenConditions[0] = firstCondition;
}

void ARMv7DOpcode::saveITConditionAt(unsigned blockPosition, unsigned condition)
{
    if (blockPosition < m_ITBlocksize)
        m_ifThenConditions[blockPosition] = static_cast<unsigned char>(condition);
}

void ARMv7DOpcode::fetchOpcode(uint16_t*& newPC)
{
    m_bufferOffset = 0;
    m_formatBuffer[0] = '\0';
    m_currentPC = newPC;

    m_opcode = *newPC++;

    if (is32BitInstruction()) {
        m_opcode <<= 16;
        m_opcode |= *newPC++;
    }

    if (m_ITConditionIndex < m_ITBlocksize)
        m_currentITCondition = m_ifThenConditions[m_ITConditionIndex];
    else
        m_currentITCondition = CondNone;
}

const char* ARMv7DOpcode::disassemble(uint16_t*& currentPC)
{
    const char* result;
    fetchOpcode(currentPC);

    if (is32BitInstruction())
        result = reinterpret_cast<ARMv7D32BitOpcode*>(this)->doDisassemble();
    else
        result = reinterpret_cast<ARMv7D16BitOpcode*>(this)->doDisassemble();

    if (startingITBlock())
        m_ITConditionIndex = 0;
    else if (inITBlock() && (++m_ITConditionIndex >= m_ITBlocksize))
        endITBlock();

    return result;
}

void ARMv7DOpcode::bufferPrintf(const char* format, ...)
{
    if (m_bufferOffset >= bufferSize)
        return;

    va_list argList;
    va_start(argList, format);

    m_bufferOffset += vsnprintf(m_formatBuffer + m_bufferOffset, bufferSize - m_bufferOffset, format, argList);

    va_end(argList);
}

void ARMv7DOpcode::appendInstructionName(const char* instructionName, bool addS)
{
    if (!inITBlock()  && !addS) {
        appendInstructionNameNoITBlock(instructionName);

        return;
    }

    const char sevenSpaces[8] = "       ";

    unsigned length = strlen(instructionName);

    bufferPrintf("   %s", instructionName);
    if (inITBlock()) {
        const char* condition = conditionName(m_currentITCondition);
        length += strlen(condition);
        appendString(condition);
    } else if (addS) {
        length++;
        appendCharacter('s');
    }

    if (length >= 7)
        length = 6;

    appendString(sevenSpaces + length);
}

void ARMv7DOpcode::appendRegisterName(unsigned registerNumber)
{
    registerNumber &= 0xf;

    if (registerNumber > 12) {
        appendString(s_specialRegisterNames[registerNumber - 13]);
        return;
    }

    bufferPrintf("r%u", registerNumber);
}

void ARMv7DOpcode::appendRegisterList(unsigned registers)
{
    unsigned numberPrinted = 0;

    appendCharacter('{');

    for (unsigned i = 0; i < 16; i++) {
        if (registers & i) {
            if (numberPrinted++)
                appendSeparator();
            appendRegisterName(i);
        }
    }

    appendCharacter('}');
}

void ARMv7DOpcode::appendFPRegisterName(char registerPrefix, unsigned registerNumber)
{
    bufferPrintf("%c%u", registerPrefix, registerNumber);
}

// 16 Bit Instructions

void ARMv7D16BitOpcode::init()
{
    OpcodeGroup* lastGroups[OpcodeGroup::opcodeTableSize];

    for (unsigned i = 0; i < OpcodeGroup::opcodeTableSize; i++) {
        opcodeTable[i] = 0;
        lastGroups[i] = 0;
    }

    for (unsigned i = 0; i < sizeof(opcode16BitGroupList) / sizeof(Opcode16GroupInitializer); i++) {
        OpcodeGroup* newOpcodeGroup = new OpcodeGroup(opcode16BitGroupList[i].m_mask, opcode16BitGroupList[i].m_pattern, opcode16BitGroupList[i].m_format);
        uint16_t opcodeGroupNumber = opcode16BitGroupList[i].m_opcodeGroupNumber;

        if (!opcodeTable[opcodeGroupNumber])
            opcodeTable[opcodeGroupNumber] = newOpcodeGroup;
        else
            lastGroups[opcodeGroupNumber]->setNext(newOpcodeGroup);
        lastGroups[opcodeGroupNumber] = newOpcodeGroup;
    }
}

const char* ARMv7D16BitOpcode::doDisassemble()
{
    OpcodeGroup* opGroup = opcodeTable[opcodeGroupNumber(m_opcode)];

    while (opGroup) {
        if (opGroup->matches(static_cast<uint16_t>(m_opcode)))
            return opGroup->format(this);
        opGroup = opGroup->next();
    }

    return defaultFormat();
}

const char* ARMv7D16BitOpcode::defaultFormat()
{
    bufferPrintf("   .word  %04x", m_opcode);
    return m_formatBuffer;
}

const char* ARMv7DOpcodeAddRegisterT2::format()
{
    appendInstructionName("add");
    appendRegisterName(rdn());
    appendSeparator();
    appendRegisterName(rm());

    return m_formatBuffer;
}

const char* ARMv7DOpcodeAddSPPlusImmediate::format()
{
    appendInstructionName("add");
    appendRegisterName(rd());
    appendSeparator();
    appendRegisterName(RegSP);
    appendSeparator();
    appendUnsignedImmediate(immediate8());

    return m_formatBuffer;
}

const char* const ARMv7DOpcodeAddSubtract::s_opNames[2] = { "add", "sub" };

const char* ARMv7DOpcodeAddSubtractT1::format()
{
    appendInstructionName(opName(), !inITBlock());
    appendRegisterName(rd());
    appendSeparator();
    appendRegisterName(rn());
    appendSeparator();
    appendRegisterName(rm());

    return m_formatBuffer;
}

const char* ARMv7DOpcodeAddSubtractImmediate3::format()
{
    appendInstructionName(opName(), !inITBlock());
    appendRegisterName(rd());
    appendSeparator();
    appendRegisterName(rn());
    appendSeparator();
    appendUnsignedImmediate(immediate3());

    return m_formatBuffer;
}

const char* ARMv7DOpcodeAddSubtractImmediate8::format()
{
    appendInstructionName(opName(), !inITBlock());
    appendRegisterName(rdn());
    appendSeparator();
    appendUnsignedImmediate(immediate8());

    return m_formatBuffer;
}

const char* ARMv7DOpcodeBranchConditionalT1::format()
{
    if (condition() == 0xe)
        return defaultFormat();

    if (condition() == 0xf) {
        appendInstructionName("svc");
        appendUnsignedImmediate(offset());

        return m_formatBuffer;
    }

    bufferPrintf("   b%-6.6s", conditionName(condition()));
    appendPCRelativeOffset(static_cast<int32_t>(offset()) + 2);

    return m_formatBuffer;
}

const char* ARMv7DOpcodeBranchExchangeT1::format()
{
    appendInstructionName(opName());
    appendRegisterName(rm());

    return m_formatBuffer;
}

const char* ARMv7DOpcodeBranchT2::format()
{
    appendInstructionName("b");
    appendPCRelativeOffset(static_cast<int32_t>(immediate11()) + 2);

    return m_formatBuffer;
}

const char* ARMv7DOpcodeCompareImmediateT1::format()
{
    appendInstructionName("cmp");
    appendRegisterName(rn());
    appendSeparator();
    appendUnsignedImmediate(immediate8());

    return m_formatBuffer;
}

const char* ARMv7DOpcodeCompareRegisterT1::format()
{
    appendInstructionName("cmp");
    appendRegisterName(rn());
    appendSeparator();
    appendRegisterName(rm());

    return m_formatBuffer;
}

const char* ARMv7DOpcodeCompareRegisterT2::format()
{
    appendInstructionName("compare");
    appendRegisterName(rn());
    appendSeparator();
    appendRegisterName(rm());

    return m_formatBuffer;
}

const char* const ARMv7DOpcodeDataProcessingRegisterT1::s_opNames[16] = {
    "and", "eor", "lsl", "lsr", "asr", "adc", "sbc", "ror", "tst", "rsb", "cmp", "cmn", "orr", "mul", "bic", "mvn"
};

const char* ARMv7DOpcodeDataProcessingRegisterT1::format()
{
    appendInstructionName(opName(), inITBlock() && (!(op() == 0x8) || (op() == 0xa) || (op() == 0xb)));
    appendRegisterName(rdn());
    appendSeparator();
    appendRegisterName(rm());
    if (op() == 0x9) // rsb T1
        appendString(", #0");
    else if (op() == 0xd) { // mul T1
        appendSeparator();
        appendRegisterName(rdn());
    }

    return m_formatBuffer;
}

const char* ARMv7DOpcodeGeneratePCRelativeAddress::format()
{
    appendInstructionName("adr");
    appendRegisterName(rd());
    appendSeparator();
    appendPCRelativeOffset(static_cast<int32_t>(immediate8()));

    return m_formatBuffer;
}

const char* ARMv7DOpcodeLoadFromLiteralPool::format()
{
    appendInstructionName("ldr");
    appendRegisterName(rt());
    appendSeparator();
    appendPCRelativeOffset(static_cast<int32_t>(immediate8()));

    return m_formatBuffer;
}

const char* const ARMv7DOpcodeLoadStoreRegisterImmediate::s_opNames[6] = {
    "str", "ldr", "strb",  "ldrb", "strh", "ldrh"
};

const char* ARMv7DOpcodeLoadStoreRegisterImmediate::format()
{
    const char* instructionName = opName();

    if (!instructionName)
        return defaultFormat();

    appendInstructionName(opName());
    appendRegisterName(rt());
    appendSeparator();
    appendCharacter('[');
    appendRegisterName(rn());
    if (immediate5()) {
        appendSeparator();
        appendUnsignedImmediate(immediate5() << scale());
    }
    appendCharacter(']');

    return m_formatBuffer;
}

const char* const ARMv7DOpcodeLoadStoreRegisterOffsetT1::s_opNames[8] = {
    "str", "strh", "strb", "ldrsb", "ldr", "ldrh", "ldrb", "ldrsh"
};

const char* ARMv7DOpcodeLoadStoreRegisterOffsetT1::format()
{
    appendInstructionName(opName());
    appendRegisterName(rt());
    appendSeparator();
    appendCharacter('[');
    appendRegisterName(rn());
    appendSeparator();
    appendRegisterName(rm());
    appendCharacter(']');

    return m_formatBuffer;
}

const char* ARMv7DOpcodeLoadStoreRegisterSPRelative::format()
{
    appendInstructionName(opName());
    appendRegisterName(rt());
    appendSeparator();
    appendCharacter('[');
    appendRegisterName(RegSP);
    if (immediate8()) {
        appendSeparator();
        appendUnsignedImmediate(immediate8() << 2);
    }
    appendCharacter(']');

    return m_formatBuffer;
}

const char* ARMv7DOpcodeLogicalImmediateT1::format()
{
    if (!op() && !immediate5()) {
        // mov T2
        appendInstructionName("movs");
        appendRegisterName(rd());
        appendSeparator();
        appendRegisterName(rm());

        return m_formatBuffer;
    }

    appendInstructionName(opName(), !inITBlock());
    appendRegisterName(rd());
    appendSeparator();
    appendRegisterName(rm());
    appendSeparator();
    appendUnsignedImmediate((op() && !immediate5()) ? 32 : immediate5());

    return m_formatBuffer;
}

const char* ARMv7DOpcodeMiscAddSubSP::format()
{
    appendInstructionName(opName());
    appendRegisterName(RegSP);
    appendSeparator();
    appendRegisterName(RegSP);
    appendSeparator();
    appendUnsignedImmediate(immediate7());

    return m_formatBuffer;
}

const char* ARMv7DOpcodeMiscBreakpointT1::format()
{
    appendInstructionNameNoITBlock("bkpt");
    appendUnsignedImmediate(immediate8());

    return m_formatBuffer;
}

const char* const ARMv7DOpcodeMiscByteHalfwordOps::s_opNames[8] = {
    "sxth", "sxb", "uxth", "uxtb", "rev", "rev16", "revsh"
};

const char* ARMv7DOpcodeMiscByteHalfwordOps::format()
{
    const char* instructionName = opName();

    if (!instructionName)
        return defaultFormat();

    appendInstructionName(instructionName);
    appendRegisterName(rd());
    appendSeparator();
    appendRegisterName(rm());

    return m_formatBuffer;
}

const char* ARMv7DOpcodeMiscCompareAndBranch::format()
{
    appendInstructionName(opName());
    appendPCRelativeOffset(immediate6() + 2);

    return m_formatBuffer;
}

const char* const ARMv7DOpcodeMiscHint16::s_opNames[16] = {
    "nop", "yield", "wfe", "wfi", "sev"
};

const char* ARMv7DOpcodeMiscHint16::format()
{
    if (opA() > 4)
        return defaultFormat();

    appendInstructionName(opName());

    return m_formatBuffer;
}

const char* ARMv7DOpcodeMiscIfThenT1::format()
{
    char opName[6];
    opName[0] = 'i';
    opName[1] = 't';

    unsigned condition = firstCondition();
    unsigned maskBits = mask();
    unsigned blockLength = 0;

    for (unsigned i = 0; i < 4; ++i) {
        if (maskBits & (1 << i)) {
            blockLength = 4 - i;
            break;
        }
    }

    startITBlock(blockLength, condition);

    for (unsigned i = 1; i < blockLength; ++i) {
        unsigned currMaskBit = (maskBits >> (4-i)) & 0x1;
        opName[i + 1] = (currMaskBit ^ (condition & 1)) ? 'e' : 't';
        saveITConditionAt(i, (condition & ~1) | currMaskBit);
    }
    opName[blockLength + 1] = '\0';

    appendInstructionNameNoITBlock(opName);
    appendString(conditionName(condition));

    return m_formatBuffer;
}

const char* ARMv7DOpcodeMiscPushPop::format()
{
    appendInstructionName(opName());
    appendRegisterList(registerMask());

    return m_formatBuffer;
}

const char* ARMv7DOpcodeMoveImmediateT1::format()
{
    appendInstructionName("mov", !inITBlock());
    appendRegisterName(rd());
    appendSeparator();
    appendUnsignedImmediate(immediate8());

    return m_formatBuffer;
}

const char* ARMv7DOpcodeMoveRegisterT1::format()
{
    appendInstructionName("mov");
    appendRegisterName(rd());
    appendSeparator();
    appendRegisterName(rm());

    return m_formatBuffer;
}

// 32 bit Intructions

void ARMv7D32BitOpcode::init()
{
    OpcodeGroup* lastGroups[OpcodeGroup::opcodeTableSize];

    for (unsigned i = 0; i < OpcodeGroup::opcodeTableSize; i++) {
        opcodeTable[i] = 0;
        lastGroups[i] = 0;
    }

    for (unsigned i = 0; i < sizeof(opcode32BitGroupList) / sizeof(Opcode32GroupInitializer); i++) {
        OpcodeGroup* newOpcodeGroup = new OpcodeGroup(opcode32BitGroupList[i].m_mask, opcode32BitGroupList[i].m_pattern, opcode32BitGroupList[i].m_format);
        uint16_t opcodeGroupNumber = opcode32BitGroupList[i].m_opcodeGroupNumber;

        if (!opcodeTable[opcodeGroupNumber])
            opcodeTable[opcodeGroupNumber] = newOpcodeGroup;
        else
            lastGroups[opcodeGroupNumber]->setNext(newOpcodeGroup);
        lastGroups[opcodeGroupNumber] = newOpcodeGroup;
    }
}

const char* ARMv7D32BitOpcode::doDisassemble()
{
    OpcodeGroup* opGroup = opcodeTable[opcodeGroupNumber(m_opcode)];

    while (opGroup) {
        if (opGroup->matches(m_opcode))
            return opGroup->format(this);
        opGroup = opGroup->next();
    }

    return defaultFormat();
}

const char* ARMv7D32BitOpcode::defaultFormat()
{
    bufferPrintf("   .long  %08x", m_opcode);
    return m_formatBuffer;
}

const char* ARMv7DOpcodeConditionalBranchT3::format()
{
    if (condition() < 0xe)
        bufferPrintf("   b%-6.6s", conditionName(condition()));
    else
        appendInstructionName("b");
    appendPCRelativeOffset(offset() + 2);

    return m_formatBuffer;
}

const char* ARMv7DOpcodeBranchOrBranchLink::format()
{
    appendInstructionName(isBL() ? "bl" : "b");
    appendPCRelativeOffset(offset() + 2);

    return m_formatBuffer;
}

const char* const ARMv7DOpcodeDataProcessingLogicalAndRithmetic::s_opNames[16] = {
    "and", "bic", "orr", "orn", "eor", 0, "pkh", 0, "add", 0, "adc", "sbc", 0, "sub", "rsb", 0
};

void ARMv7DOpcodeDataProcessingModifiedImmediate::appendModifiedImmediate(unsigned immediate12)
{
    if (!(immediate12 & 0xc00)) {
        unsigned immediate = 0;
        unsigned lower8Bits = immediate12 & 0xff;

        switch ((immediate12 >> 8) & 3) {
        case 0:
            immediate = lower8Bits;
            break;
        case 1:
            immediate = (lower8Bits << 16) | lower8Bits;
            break;
        case 2:
            immediate = (lower8Bits << 24) | (lower8Bits << 8);
            break;
        case 3:
            immediate = (lower8Bits << 24) | (lower8Bits << 16) | (lower8Bits << 8) | lower8Bits;
            break;
        }
        appendUnsignedImmediate(immediate);
        return;
    }

    unsigned immediate8 = 0x80 | (immediate12 & 0x7f);
    unsigned shiftAmount = 32 - ((immediate12 >> 7) & 0x1f);

    appendUnsignedImmediate(immediate8 << shiftAmount);
}

const char* ARMv7DOpcodeDataProcessingModifiedImmediate::format()
{
    if ((op() == 0x5) || (op() == 0x6) || (op() == 0x7) || (op() == 0x9) || (op() == 0xc) || (op() == 0xf))
        return defaultFormat();

    const char* instructionName = opName();

    if (rn() == 15) {
        if (op() == 2) {
            // MOV T2
            instructionName = sBit() ? "movs" : "mov";
            appendInstructionName(instructionName);
            appendRegisterName(rd());
            appendSeparator();
            appendModifiedImmediate(immediate12());

            return m_formatBuffer;
        }

        if (op() == 3) {
            // MVN T1
            instructionName = sBit() ? "mvns" : "mvn";
            appendInstructionName(instructionName);
            appendRegisterName(rd());
            appendSeparator();
            appendModifiedImmediate(immediate12());

            return m_formatBuffer;
        }
    }

    if (rd() == 15) {
        if (sBit()) {
            bool testOrCmpInstruction = false;

            switch (op()) {
            case 0x0:
                instructionName = "tst";
                testOrCmpInstruction = true;
                break;
            case 0x4:
                instructionName = "teq";
                testOrCmpInstruction = true;
                break;
            case 0x8:
                instructionName = "cmn";
                testOrCmpInstruction = true;
                break;
            case 0xd:
                instructionName = "cmp";
                testOrCmpInstruction = true;
                break;
            }

            if (testOrCmpInstruction) {
                appendInstructionName(instructionName);
                appendRegisterName(rn());
                appendSeparator();
                appendModifiedImmediate(immediate12());

                return m_formatBuffer;
            }
        }
    }

    appendInstructionName(instructionName);
    appendRegisterName(rd());
    appendSeparator();
    appendRegisterName(rn());
    appendSeparator();
    appendModifiedImmediate(immediate12());

    return m_formatBuffer;
}

void ARMv7DOpcodeDataProcessingShiftedReg::appendImmShift(unsigned type, unsigned immediate)
{
    if (type || immediate) {
        appendSeparator();

        if (!immediate) {
            switch (type) {
            case 1:
            case 2:
                immediate = 32;
                break;
            case 3:
                appendString("rrx");
                return;
            }
        }

        appendShiftType(type);
        appendUnsignedImmediate(immediate);
    }
}

const char* ARMv7DOpcodeDataProcessingShiftedReg::format()
{
    if ((op() == 0x5) || (op() == 0x7) || (op() == 0x9) || (op() == 0xc) || (op() == 0xf))
        return defaultFormat();

    if (op() == 6) {
        // pkhbt or pkhtb
        if (sBit() || tBit())
            return defaultFormat();

        if (tbBit())
            appendInstructionName("pkhtb");
        else
            appendInstructionName("pkhbt");
        appendRegisterName(rd());
        appendSeparator();
        appendRegisterName(rn());
        appendSeparator();
        appendRegisterName(rm());
        appendImmShift(tbBit() << 1, immediate5());

        return m_formatBuffer;
    }

    const char* instructionName = opName();

    if (rn() == 15) {
        if (op() == 2) {
            if (!type() && !immediate5()) {
                // MOV T3
                instructionName = sBit() ? "movs" : "mov";
                appendInstructionName(instructionName);
                appendRegisterName(rd());
                appendSeparator();
                appendRegisterName(rm());

                return m_formatBuffer;
            }

            if (type() == 3 && !immediate5()) {
                // RRX T1
                instructionName = sBit() ? "rrx" : "rrx";
                appendInstructionName(instructionName);
                appendRegisterName(rd());
                appendSeparator();
                appendRegisterName(rm());

                return m_formatBuffer;
            }

            // Logical
            if (sBit())
                bufferPrintf("%ss ", shiftName(type()));
            else
                appendInstructionName(shiftName(type()));
            appendRegisterName(rd());
            appendSeparator();
            appendRegisterName(rm());
            appendSeparator();
            appendUnsignedImmediate(immediate5());

            return m_formatBuffer;
        }

        if (op() == 3) {
            // MVN T2
            instructionName = sBit() ? "mvns" : "mvn";
            appendInstructionName(instructionName);
            appendRegisterName(rd());
            appendSeparator();
            appendRegisterName(rm());
            appendImmShift(type(), immediate5());

            return m_formatBuffer;
        }
    }

    if (rd() == 15) {
        if (sBit()) {
            bool testOrCmpInstruction = false;

            switch (op()) {
            case 0x0:
                instructionName = "tst";
                testOrCmpInstruction = true;
                break;
            case 0x4:
                instructionName = "teq";
                testOrCmpInstruction = true;
                break;
            case 0x8:
                instructionName = "cmn";
                testOrCmpInstruction = true;
                break;
            case 0xd:
                instructionName = "cmp";
                testOrCmpInstruction = true;
                break;
            }

            if (testOrCmpInstruction) {
                appendInstructionName(instructionName);
                appendRegisterName(rn());
                appendSeparator();
                appendRegisterName(rm());
                appendImmShift(type(), immediate5());

                return m_formatBuffer;
            }
        }
    }

    appendInstructionName(instructionName);
    appendRegisterName(rd());
    appendSeparator();
    appendRegisterName(rn());
    appendSeparator();
    appendRegisterName(rm());
    appendImmShift(type(), immediate5());

    return m_formatBuffer;
}

const char* ARMv7DOpcodeFPTransfer::format()
{
    appendInstructionName("vmov");

    if (opL()) {
        appendFPRegister();
        appendSeparator();
    }

    appendRegisterName(rt());

    if (!opL()) {
        appendSeparator();
        appendFPRegister();
    }

    return m_formatBuffer;
}

void ARMv7DOpcodeFPTransfer::appendFPRegister()
{
    if (opC()) {
        appendFPRegisterName('d', vd());
        bufferPrintf("[%u]", opH());
    } else
        appendFPRegisterName('s', vn());
}

const char* ARMv7DOpcodeDataProcessingRegShift::format()
{
    appendInstructionName(opName());
    appendRegisterName(rd());
    appendSeparator();
    appendRegisterName(rn());
    appendSeparator();
    appendRegisterName(rm());

    return m_formatBuffer;
}

const char* const ARMv7DOpcodeDataProcessingRegExtend::s_opExtendNames[8] = {
    "sxth", "uxth", "sxtb16", "uxtb16", "sxtb", "uxtb"
};

const char* const ARMv7DOpcodeDataProcessingRegExtend::s_opExtendAndAddNames[8] = {
    "sxtah", "uxtah", "sxtab16", "uxtab16", "sxtab", "uxtab"
};

const char* ARMv7DOpcodeDataProcessingRegExtend::format()
{
    const char* instructionName;

    if (rn() == 0xf)
        instructionName = opExtendName();
    else
        instructionName = opExtendAndAddName();

    if (!instructionName)
        return defaultFormat();

    appendInstructionName(instructionName);
    appendRegisterName(rd());
    appendSeparator();
    appendRegisterName(rn());
    appendSeparator();
    appendRegisterName(rm());

    if (rotate()) {
        appendSeparator();
        appendString("ror ");
        appendUnsignedImmediate(rotate() * 8);
    }

    return m_formatBuffer;
}

const char* const ARMv7DOpcodeDataProcessingRegParallel::s_opNames[16] = {
    "sadd8", "sadd16", "sasx", 0, "ssub8", "ssub16", "ssax", 0,
    "qadd8", "qadd16", "qasx", 0, "qsub8", "qsub16", "qsax", 0
};

const char* ARMv7DOpcodeDataProcessingRegParallel::format()
{
    const char* instructionName;

    instructionName = opName();

    if (!instructionName)
        return defaultFormat();

    appendInstructionName(instructionName);
    appendRegisterName(rd());
    appendSeparator();
    appendRegisterName(rn());
    appendSeparator();
    appendRegisterName(rm());

    return m_formatBuffer;
}

const char* const ARMv7DOpcodeDataProcessingRegMisc::s_opNames[16] = {
    "qadd", "qdadd", "qsub", "qdsub", "rev", "rev16", "rbit", "revsh",
    "sel", 0, 0, 0, "clz"
};

const char* ARMv7DOpcodeDataProcessingRegMisc::format()
{
    const char* instructionName;

    instructionName = opName();

    if (!instructionName)
        return defaultFormat();

    if ((op1() & 0x1) && (rn() != rm()))
        return defaultFormat();

    appendInstructionName(instructionName);
    appendRegisterName(rd());
    appendSeparator();

    if (op1() == 0x2) { // sel
        appendRegisterName(rn());
        appendSeparator();
        appendRegisterName(rm());

        return m_formatBuffer;
    }

    appendRegisterName(rm());

    if (!(op1() & 0x1)) {
        appendSeparator();
        appendRegisterName(rn());
    }

    return m_formatBuffer;
}

const char* const ARMv7DOpcodeHint32::s_opNames[8] = {
    "nop", "yield", "wfe", "wfi", "sev"
};

const char* ARMv7DOpcodeHint32::format()
{
    if (isDebugHint()) {
        appendInstructionName("debug");
        appendUnsignedImmediate(debugOption());

        return m_formatBuffer;
    }

    if (op() > 0x4)
        return defaultFormat();

    appendInstructionName(opName());

    return m_formatBuffer;
}

const char* const ARMv7DOpcodeDataLoad::s_opNames[8] = {
    "ldrb", "ldrh", "ldr", 0, "ldrsb", "ldrsh"
};

const char* ARMv7DOpcodeLoadRegister::format()
{
    appendInstructionName(opName());
    appendRegisterName(rt());
    appendSeparator();
    appendCharacter('[');
    appendRegisterName(rn());
    appendSeparator();
    appendRegisterName(rm());
    if (immediate2()) {
        appendSeparator();
        appendUnsignedImmediate(immediate2());
    }
    appendCharacter(']');

    return m_formatBuffer;
}

const char* ARMv7DOpcodeLoadSignedImmediate::format()
{
    appendInstructionName(opName());
    appendRegisterName(rt());
    appendSeparator();
    appendCharacter('[');
    appendRegisterName(rn());
    if (pBit()) {
        if (wBit() || immediate8()) {
            appendSeparator();
            if (uBit())
                appendUnsignedImmediate(immediate8());
            else
                appendSignedImmediate(0 - static_cast<int>(immediate8()));
        }
        appendCharacter(']');
        if (wBit())
            appendCharacter('!');
    } else {
        appendCharacter(']');
        appendSeparator();
        if (uBit())
            appendUnsignedImmediate(immediate8());
        else
            appendSignedImmediate(0 - static_cast<int>(immediate8()));
    }

    return m_formatBuffer;
}

const char* ARMv7DOpcodeLoadUnsignedImmediate::format()
{
    appendInstructionName(opName());
    appendRegisterName(rt());
    appendSeparator();
    appendCharacter('[');
    appendRegisterName(rn());
    if (immediate12()) {
        appendSeparator();
        appendUnsignedImmediate(immediate12());
    }
    appendCharacter(']');

    return m_formatBuffer;
}

const char* const ARMv7DOpcodeLongMultipleDivide::s_opNames[8] = {
    "smull", "sdiv", "umull", "udiv", "smlal", "smlsld", "umlal", 0
};

const char* const ARMv7DOpcodeLongMultipleDivide::s_smlalOpNames[4] = {
    "smlalbb", "smlalbt", "smlaltb", "smlaltt"
};

const char* const ARMv7DOpcodeLongMultipleDivide::s_smlaldOpNames[2] = {
    "smlald", "smlaldx"
};

const char* const ARMv7DOpcodeLongMultipleDivide::s_smlsldOpNames[2] = {
    "smlsld", "smlsldx"
};

const char* ARMv7DOpcodeLongMultipleDivide::format()
{
    const char* instructionName = opName();

    switch (op1()) {
    case 0x0:
    case 0x2:
        if (op2())
            return defaultFormat();
        break;
    case 0x1:
    case 0x3:
        if (op2() != 0xf)
            return defaultFormat();
        break;
    case 0x4:
        if ((op2() & 0xc) == 0x8)
            instructionName = smlalOpName();
        else if ((op2() & 0xe) == 0xc)
            instructionName = smlaldOpName();
        else if (op2())
            return defaultFormat();
        break;
    case 0x5:
        if ((op2() & 0xe) == 0xc)
            instructionName = smlaldOpName();
        else
            return defaultFormat();
        break;
    case 0x6:
        if (op2() == 0x5)
            instructionName = "umaal";
        else if (op2())
            return defaultFormat();
        break;
    case 0x7:
        return defaultFormat();
        break;
    }

    appendInstructionName(instructionName);
    if ((op1() & 0x5) == 0x1) { // sdiv and udiv
        if (rt() != 0xf)
            return defaultFormat();
    } else {
        appendRegisterName(rdLo());
        appendSeparator();
    }
    appendRegisterName(rdHi());
    appendSeparator();
    appendRegisterName(rn());
    appendSeparator();
    appendRegisterName(rm());

    return m_formatBuffer;
}

const char* const ARMv7DOpcodeUnmodifiedImmediate::s_opNames[16] = {
    "addw", 0, "movw", 0, 0, "subw", "movt", 0,
    "ssat", "ssat16", "sbfx", "bfi", "usat" , "usat16", "ubfx", 0
};

const char* ARMv7DOpcodeUnmodifiedImmediate::format()
{
    const char* instructionName = opName();

    switch (op() >> 1) {
    case 0x0:
    case 0x5:
        if (rn() == 0xf)
            instructionName = "adr";
        break;
    case 0x9:
        if (immediate5())
            instructionName = "ssat";
        break;
    case 0xb:
        if (rn() == 0xf)
            instructionName = "bfc";
        break;
    case 0xd:
        if (immediate5())
            instructionName = "usat";
        break;
    }

    if (!instructionName)
        return defaultFormat();

    appendInstructionName(instructionName);
    appendRegisterName(rd());
    appendSeparator();

    if ((op() & 0x17) == 0x4) { // movw or movt
        appendUnsignedImmediate(immediate16());

        return m_formatBuffer;
    }

    if (!op() || (op() == 0xa)) { // addw, subw and adr
        if (rn() == 0xf) {
            int32_t offset;

            if ((op() == 0xa) && (rn() == 0xf))
                offset = 0 - static_cast<int32_t>(immediate12());
            else
                offset = static_cast<int32_t>(immediate12());

            appendPCRelativeOffset(offset);

            return m_formatBuffer;
        }

        appendRegisterName(rn());
        appendSeparator();
        appendUnsignedImmediate(immediate12());

        return m_formatBuffer;
    }

    if (((op() & 0x15) == 0x10) || (((op() & 0x17) == 0x12) && immediate5())) { // ssat, usat, ssat16 & usat16
        appendSeparator();
        appendUnsignedImmediate(bitNumOrSatImmediate() + 1);
        appendSeparator();
        appendRegisterName(rn());
        if (shBit() || immediate5()) {
            appendSeparator();
            appendShiftType(shBit() << 1);
            appendUnsignedImmediate(immediate5());
        }

        return m_formatBuffer;
    }

    if (op() == 0x16) { // bfi or bfc
        int width = static_cast<int>(bitNumOrSatImmediate()) - static_cast<int>(immediate5()) + 1;

        if (width < 0)
            return defaultFormat();

        if (rn() != 0xf) {
            appendSeparator();
            appendRegisterName(rn());
        }
        appendSeparator();
        appendUnsignedImmediate(immediate5());
        appendSeparator();
        appendSignedImmediate(width);

        return m_formatBuffer;
    }

    // Must be sbfx or ubfx
    appendSeparator();
    appendRegisterName(rn());
    appendSeparator();
    appendUnsignedImmediate(immediate5());
    appendSeparator();
    appendUnsignedImmediate(bitNumOrSatImmediate() + 1);

    return m_formatBuffer;
}

const char* const ARMv7DOpcodeDataStoreSingle::s_opNames[4] = {
    "strb", "strh", "str", 0
};

const char* ARMv7DOpcodeDataPushPopSingle::format()
{
    appendInstructionName(opName());
    appendRegisterName(rt());

    return m_formatBuffer;
}

const char* ARMv7DOpcodeStoreSingleImmediate12::format()
{
    appendInstructionName(opName());
    appendRegisterName(rt());
    appendSeparator();
    appendCharacter('[');
    appendRegisterName(rn());
    if (immediate12()) {
        appendSeparator();
        appendUnsignedImmediate(immediate12());
    }
    appendCharacter(']');

    return m_formatBuffer;
}

const char* ARMv7DOpcodeStoreSingleImmediate8::format()
{
    if (pBit() && uBit() && !wBit()) // Really undecoded strt
        return defaultFormat();

    if ((rn() == 0xf) || (!pBit() && !wBit()))
        return defaultFormat();

    appendInstructionName(opName());
    appendRegisterName(rt());
    appendSeparator();
    appendCharacter('[');
    appendRegisterName(rn());

    if (!pBit()) {
        appendCharacter(']');
        appendSeparator();
        appendSignedImmediate(uBit() ? static_cast<int32_t>(immediate8()) : (0 - static_cast<int32_t>(immediate8())));

        return m_formatBuffer;
    }

    if (immediate8()) {
        appendSeparator();
        appendSignedImmediate(uBit() ? static_cast<int32_t>(immediate8()) : (0 - static_cast<int32_t>(immediate8())));
    }
    appendCharacter(']');

    if (wBit())
        appendCharacter('!');

    return m_formatBuffer;
}

const char* ARMv7DOpcodeStoreSingleRegister::format()
{
    appendInstructionName(opName());
    appendRegisterName(rt());
    appendSeparator();
    appendCharacter('[');
    appendRegisterName(rn());
    appendSeparator();
    appendRegisterName(rm());
    if (immediate2()) {
        appendSeparator();
        appendString("lsl ");
        appendUnsignedImmediate(immediate2());
    }
    appendCharacter(']');

    return m_formatBuffer;
}

const char* ARMv7DOpcodeVMOVDoublePrecision::format()
{
    appendInstructionName("vmov");
    if (op()) {
        appendRegisterName(rt());
        appendSeparator();
        appendRegisterName(rt2());
        appendSeparator();
    }

    appendFPRegisterName('d', vm());

    if (!op()) {
        appendSeparator();
        appendRegisterName(rt());
        appendSeparator();
        appendRegisterName(rt2());
    }

    return m_formatBuffer;
}

const char* ARMv7DOpcodeVMOVSinglePrecision::format()
{
    appendInstructionName("vmov");
    if (op()) {
        appendRegisterName(rt());
        appendSeparator();
        appendRegisterName(rt2());
        appendSeparator();
    }

    appendFPRegisterName('s', vm());
    appendSeparator();
    appendFPRegisterName('s', (vm() + 1) % 32);

    if (!op()) {
        appendSeparator();
        appendRegisterName(rt());
        appendSeparator();
        appendRegisterName(rt2());
    }

    return m_formatBuffer;
}

const char* ARMv7DOpcodeVMSR::format()
{
    appendInstructionName("vmrs");
    if (opL()) {
        if (rt() == 0xf)
            appendString("apsr_nzcv");
        else
            appendRegisterName(rt());
        appendSeparator();
    }

    appendString("fpscr");

    if (!opL()) {
        appendSeparator();
        appendRegisterName(rt());
    }

    return m_formatBuffer;
}

} } // namespace JSC::ARMv7Disassembler

#endif // #if USE(ARMV7_DISASSEMBLER)
