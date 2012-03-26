/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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
#include "YarrJIT.h"

#include "ASCIICType.h"
#include "LinkBuffer.h"
#include "Yarr.h"

#if ENABLE(YARR_JIT)

using namespace WTF;

namespace JSC { namespace Yarr {

class YarrGenerator : private MacroAssembler {
    friend void jitCompile(JSGlobalData*, YarrCodeBlock& jitObject, const UString& pattern, unsigned& numSubpatterns, const char*& error, bool ignoreCase, bool multiline);

#if CPU(ARM)
    static const RegisterID input = ARMRegisters::r0;
    static const RegisterID index = ARMRegisters::r1;
    static const RegisterID length = ARMRegisters::r2;
    static const RegisterID output = ARMRegisters::r4;

    static const RegisterID regT0 = ARMRegisters::r5;
    static const RegisterID regT1 = ARMRegisters::r6;

    static const RegisterID returnRegister = ARMRegisters::r0;
#elif CPU(MIPS)
    static const RegisterID input = MIPSRegisters::a0;
    static const RegisterID index = MIPSRegisters::a1;
    static const RegisterID length = MIPSRegisters::a2;
    static const RegisterID output = MIPSRegisters::a3;

    static const RegisterID regT0 = MIPSRegisters::t4;
    static const RegisterID regT1 = MIPSRegisters::t5;

    static const RegisterID returnRegister = MIPSRegisters::v0;
#elif CPU(SH4)
    static const RegisterID input = SH4Registers::r4;
    static const RegisterID index = SH4Registers::r5;
    static const RegisterID length = SH4Registers::r6;
    static const RegisterID output = SH4Registers::r7;

    static const RegisterID regT0 = SH4Registers::r0;
    static const RegisterID regT1 = SH4Registers::r1;

    static const RegisterID returnRegister = SH4Registers::r0;
#elif CPU(X86)
    static const RegisterID input = X86Registers::eax;
    static const RegisterID index = X86Registers::edx;
    static const RegisterID length = X86Registers::ecx;
    static const RegisterID output = X86Registers::edi;

    static const RegisterID regT0 = X86Registers::ebx;
    static const RegisterID regT1 = X86Registers::esi;

    static const RegisterID returnRegister = X86Registers::eax;
#elif CPU(X86_64)
    static const RegisterID input = X86Registers::edi;
    static const RegisterID index = X86Registers::esi;
    static const RegisterID length = X86Registers::edx;
    static const RegisterID output = X86Registers::ecx;

    static const RegisterID regT0 = X86Registers::eax;
    static const RegisterID regT1 = X86Registers::ebx;

    static const RegisterID returnRegister = X86Registers::eax;
#endif

    void optimizeAlternative(PatternAlternative* alternative)
    {
        if (!alternative->m_terms.size())
            return;

        for (unsigned i = 0; i < alternative->m_terms.size() - 1; ++i) {
            PatternTerm& term = alternative->m_terms[i];
            PatternTerm& nextTerm = alternative->m_terms[i + 1];

            if ((term.type == PatternTerm::TypeCharacterClass)
                && (term.quantityType == QuantifierFixedCount)
                && (nextTerm.type == PatternTerm::TypePatternCharacter)
                && (nextTerm.quantityType == QuantifierFixedCount)) {
                PatternTerm termCopy = term;
                alternative->m_terms[i] = nextTerm;
                alternative->m_terms[i + 1] = termCopy;
            }
        }
    }

    void matchCharacterClassRange(RegisterID character, JumpList& failures, JumpList& matchDest, const CharacterRange* ranges, unsigned count, unsigned* matchIndex, const UChar* matches, unsigned matchCount)
    {
        do {
            // pick which range we're going to generate
            int which = count >> 1;
            char lo = ranges[which].begin;
            char hi = ranges[which].end;

            // check if there are any ranges or matches below lo.  If not, just jl to failure -
            // if there is anything else to check, check that first, if it falls through jmp to failure.
            if ((*matchIndex < matchCount) && (matches[*matchIndex] < lo)) {
                Jump loOrAbove = branch32(GreaterThanOrEqual, character, Imm32((unsigned short)lo));

                // generate code for all ranges before this one
                if (which)
                    matchCharacterClassRange(character, failures, matchDest, ranges, which, matchIndex, matches, matchCount);

                while ((*matchIndex < matchCount) && (matches[*matchIndex] < lo)) {
                    matchDest.append(branch32(Equal, character, Imm32((unsigned short)matches[*matchIndex])));
                    ++*matchIndex;
                }
                failures.append(jump());

                loOrAbove.link(this);
            } else if (which) {
                Jump loOrAbove = branch32(GreaterThanOrEqual, character, Imm32((unsigned short)lo));

                matchCharacterClassRange(character, failures, matchDest, ranges, which, matchIndex, matches, matchCount);
                failures.append(jump());

                loOrAbove.link(this);
            } else
                failures.append(branch32(LessThan, character, Imm32((unsigned short)lo)));

            while ((*matchIndex < matchCount) && (matches[*matchIndex] <= hi))
                ++*matchIndex;

            matchDest.append(branch32(LessThanOrEqual, character, Imm32((unsigned short)hi)));
            // fall through to here, the value is above hi.

            // shuffle along & loop around if there are any more matches to handle.
            unsigned next = which + 1;
            ranges += next;
            count -= next;
        } while (count);
    }

    void matchCharacterClass(RegisterID character, JumpList& matchDest, const CharacterClass* charClass)
    {
        if (charClass->m_table) {
            ExtendedAddress tableEntry(character, reinterpret_cast<intptr_t>(charClass->m_table->m_table));
            matchDest.append(branchTest8(charClass->m_table->m_inverted ? Zero : NonZero, tableEntry));
            return;
        }
        Jump unicodeFail;
        if (charClass->m_matchesUnicode.size() || charClass->m_rangesUnicode.size()) {
            Jump isAscii = branch32(LessThanOrEqual, character, TrustedImm32(0x7f));

            if (charClass->m_matchesUnicode.size()) {
                for (unsigned i = 0; i < charClass->m_matchesUnicode.size(); ++i) {
                    UChar ch = charClass->m_matchesUnicode[i];
                    matchDest.append(branch32(Equal, character, Imm32(ch)));
                }
            }

            if (charClass->m_rangesUnicode.size()) {
                for (unsigned i = 0; i < charClass->m_rangesUnicode.size(); ++i) {
                    UChar lo = charClass->m_rangesUnicode[i].begin;
                    UChar hi = charClass->m_rangesUnicode[i].end;

                    Jump below = branch32(LessThan, character, Imm32(lo));
                    matchDest.append(branch32(LessThanOrEqual, character, Imm32(hi)));
                    below.link(this);
                }
            }

            unicodeFail = jump();
            isAscii.link(this);
        }

        if (charClass->m_ranges.size()) {
            unsigned matchIndex = 0;
            JumpList failures;
            matchCharacterClassRange(character, failures, matchDest, charClass->m_ranges.begin(), charClass->m_ranges.size(), &matchIndex, charClass->m_matches.begin(), charClass->m_matches.size());
            while (matchIndex < charClass->m_matches.size())
                matchDest.append(branch32(Equal, character, Imm32((unsigned short)charClass->m_matches[matchIndex++])));

            failures.link(this);
        } else if (charClass->m_matches.size()) {
            // optimization: gather 'a','A' etc back together, can mask & test once.
            Vector<char> matchesAZaz;

            for (unsigned i = 0; i < charClass->m_matches.size(); ++i) {
                char ch = charClass->m_matches[i];
                if (m_pattern.m_ignoreCase) {
                    if (isASCIILower(ch)) {
                        matchesAZaz.append(ch);
                        continue;
                    }
                    if (isASCIIUpper(ch))
                        continue;
                }
                matchDest.append(branch32(Equal, character, Imm32((unsigned short)ch)));
            }

            if (unsigned countAZaz = matchesAZaz.size()) {
                or32(TrustedImm32(32), character);
                for (unsigned i = 0; i < countAZaz; ++i)
                    matchDest.append(branch32(Equal, character, TrustedImm32(matchesAZaz[i])));
            }
        }

        if (charClass->m_matchesUnicode.size() || charClass->m_rangesUnicode.size())
            unicodeFail.link(this);
    }

    // Jumps if input not available; will have (incorrectly) incremented already!
    Jump jumpIfNoAvailableInput(unsigned countToCheck = 0)
    {
        if (countToCheck)
            add32(Imm32(countToCheck), index);
        return branch32(Above, index, length);
    }

    Jump jumpIfAvailableInput(unsigned countToCheck)
    {
        add32(Imm32(countToCheck), index);
        return branch32(BelowOrEqual, index, length);
    }

    Jump checkInput()
    {
        return branch32(BelowOrEqual, index, length);
    }

    Jump atEndOfInput()
    {
        return branch32(Equal, index, length);
    }

    Jump notAtEndOfInput()
    {
        return branch32(NotEqual, index, length);
    }

    Jump jumpIfCharEquals(UChar ch, int inputPosition)
    {
        return branch16(Equal, BaseIndex(input, index, TimesTwo, inputPosition * sizeof(UChar)), Imm32(ch));
    }

    Jump jumpIfCharNotEquals(UChar ch, int inputPosition)
    {
        return branch16(NotEqual, BaseIndex(input, index, TimesTwo, inputPosition * sizeof(UChar)), Imm32(ch));
    }

    void readCharacter(int inputPosition, RegisterID reg)
    {
        load16(BaseIndex(input, index, TimesTwo, inputPosition * sizeof(UChar)), reg);
    }

    void storeToFrame(RegisterID reg, unsigned frameLocation)
    {
        poke(reg, frameLocation);
    }

    void storeToFrame(TrustedImm32 imm, unsigned frameLocation)
    {
        poke(imm, frameLocation);
    }

    DataLabelPtr storeToFrameWithPatch(unsigned frameLocation)
    {
        return storePtrWithPatch(TrustedImmPtr(0), Address(stackPointerRegister, frameLocation * sizeof(void*)));
    }

    void loadFromFrame(unsigned frameLocation, RegisterID reg)
    {
        peek(reg, frameLocation);
    }

    void loadFromFrameAndJump(unsigned frameLocation)
    {
        jump(Address(stackPointerRegister, frameLocation * sizeof(void*)));
    }

    enum YarrOpCode {
        // These nodes wrap body alternatives - those in the main disjunction,
        // rather than subpatterns or assertions. These are chained together in
        // a doubly linked list, with a 'begin' node for the first alternative,
        // a 'next' node for each subsequent alternative, and an 'end' node at
        // the end. In the case of repeating alternatives, the 'end' node also
        // has a reference back to 'begin'.
        OpBodyAlternativeBegin,
        OpBodyAlternativeNext,
        OpBodyAlternativeEnd,
        // Similar to the body alternatives, but used for subpatterns with two
        // or more alternatives.
        OpNestedAlternativeBegin,
        OpNestedAlternativeNext,
        OpNestedAlternativeEnd,
        // Used for alternatives in subpatterns where there is only a single
        // alternative (backtrackingis easier in these cases), or for alternatives
        // which never need to be backtracked (those in parenthetical assertions,
        // terminal subpatterns).
        OpSimpleNestedAlternativeBegin,
        OpSimpleNestedAlternativeNext,
        OpSimpleNestedAlternativeEnd,
        // Used to wrap 'Once' subpattern matches (quantityCount == 1).
        OpParenthesesSubpatternOnceBegin,
        OpParenthesesSubpatternOnceEnd,
        // Used to wrap 'Terminal' subpattern matches (at the end of the regexp).
        OpParenthesesSubpatternTerminalBegin,
        OpParenthesesSubpatternTerminalEnd,
        // Used to wrap parenthetical assertions.
        OpParentheticalAssertionBegin,
        OpParentheticalAssertionEnd,
        // Wraps all simple terms (pattern characters, character classes).
        OpTerm,
        // Where an expression contains only 'once through' body alternatives
        // and no repeating ones, this op is used to return match failure.
        OpMatchFailed
    };

    // This structure is used to hold the compiled opcode information,
    // including reference back to the original PatternTerm/PatternAlternatives,
    // and JIT compilation data structures.
    struct YarrOp {
        explicit YarrOp(PatternTerm* term)
            : m_op(OpTerm)
            , m_term(term)
            , m_isDeadCode(false)
        {
        }

        explicit YarrOp(YarrOpCode op)
            : m_op(op)
            , m_isDeadCode(false)
        {
        }

        // The operation, as a YarrOpCode, and also a reference to the PatternTerm.
        YarrOpCode m_op;
        PatternTerm* m_term;

        // For alternatives, this holds the PatternAlternative and doubly linked
        // references to this alternative's siblings. In the case of the
        // OpBodyAlternativeEnd node at the end of a section of repeating nodes,
        // m_nextOp will reference the OpBodyAlternativeBegin node of the first
        // repeating alternative.
        PatternAlternative* m_alternative;
        size_t m_previousOp;
        size_t m_nextOp;

        // Used to record a set of Jumps out of the generated code, typically
        // used for jumps out to backtracking code, and a single reentry back
        // into the code for a node (likely where a backtrack will trigger
        // rematching).
        Label m_reentry;
        JumpList m_jumps;

        // This flag is used to null out the second pattern character, when
        // two are fused to match a pair together.
        bool m_isDeadCode;

        // Currently used in the case of some of the more complex management of
        // 'm_checked', to cache the offset used in this alternative, to avoid
        // recalculating it.
        int m_checkAdjust;

        // Used by OpNestedAlternativeNext/End to hold the pointer to the
        // value that will be pushed into the pattern's frame to return to,
        // upon backtracking back into the disjunction.
        DataLabelPtr m_returnAddress;
    };

    // BacktrackingState
    // This class encapsulates information about the state of code generation
    // whilst generating the code for backtracking, when a term fails to match.
    // Upon entry to code generation of the backtracking code for a given node,
    // the Backtracking state will hold references to all control flow sources
    // that are outputs in need of further backtracking from the prior node
    // generated (which is the subsequent operation in the regular expression,
    // and in the m_ops Vector, since we generated backtracking backwards).
    // These references to control flow take the form of:
    //  - A jump list of jumps, to be linked to code that will backtrack them
    //    further.
    //  - A set of DataLabelPtr values, to be populated with values to be
    //    treated effectively as return addresses backtracking into complex
    //    subpatterns.
    //  - A flag indicating that the current sequence of generated code up to
    //    this point requires backtracking.
    class BacktrackingState {
    public:
        BacktrackingState()
            : m_pendingFallthrough(false)
        {
        }

        // Add a jump or jumps, a return address, or set the flag indicating
        // that the current 'fallthrough' control flow requires backtracking.
        void append(const Jump& jump)
        {
            m_laterFailures.append(jump);
        }
        void append(JumpList& jumpList)
        {
            m_laterFailures.append(jumpList);
        }
        void append(const DataLabelPtr& returnAddress)
        {
            m_pendingReturns.append(returnAddress);
        }
        void fallthrough()
        {
            ASSERT(!m_pendingFallthrough);
            m_pendingFallthrough = true;
        }

        // These methods clear the backtracking state, either linking to the
        // current location, a provided label, or copying the backtracking out
        // to a JumpList. All actions may require code generation to take place,
        // and as such are passed a pointer to the assembler.
        void link(MacroAssembler* assembler)
        {
            if (m_pendingReturns.size()) {
                Label here(assembler);
                for (unsigned i = 0; i < m_pendingReturns.size(); ++i)
                    m_backtrackRecords.append(ReturnAddressRecord(m_pendingReturns[i], here));
                m_pendingReturns.clear();
            }
            m_laterFailures.link(assembler);
            m_laterFailures.clear();
            m_pendingFallthrough = false;
        }
        void linkTo(Label label, MacroAssembler* assembler)
        {
            if (m_pendingReturns.size()) {
                for (unsigned i = 0; i < m_pendingReturns.size(); ++i)
                    m_backtrackRecords.append(ReturnAddressRecord(m_pendingReturns[i], label));
                m_pendingReturns.clear();
            }
            if (m_pendingFallthrough)
                assembler->jump(label);
            m_laterFailures.linkTo(label, assembler);
            m_laterFailures.clear();
            m_pendingFallthrough = false;
        }
        void takeBacktracksToJumpList(JumpList& jumpList, MacroAssembler* assembler)
        {
            if (m_pendingReturns.size()) {
                Label here(assembler);
                for (unsigned i = 0; i < m_pendingReturns.size(); ++i)
                    m_backtrackRecords.append(ReturnAddressRecord(m_pendingReturns[i], here));
                m_pendingReturns.clear();
                m_pendingFallthrough = true;
            }
            if (m_pendingFallthrough)
                jumpList.append(assembler->jump());
            jumpList.append(m_laterFailures);
            m_laterFailures.clear();
            m_pendingFallthrough = false;
        }

        bool isEmpty()
        {
            return m_laterFailures.empty() && m_pendingReturns.isEmpty() && !m_pendingFallthrough;
        }

        // Called at the end of code generation to link all return addresses.
        void linkDataLabels(LinkBuffer& linkBuffer)
        {
            ASSERT(isEmpty());
            for (unsigned i = 0; i < m_backtrackRecords.size(); ++i)
                linkBuffer.patch(m_backtrackRecords[i].m_dataLabel, linkBuffer.locationOf(m_backtrackRecords[i].m_backtrackLocation));
        }

    private:
        struct ReturnAddressRecord {
            ReturnAddressRecord(DataLabelPtr dataLabel, Label backtrackLocation)
                : m_dataLabel(dataLabel)
                , m_backtrackLocation(backtrackLocation)
            {
            }

            DataLabelPtr m_dataLabel;
            Label m_backtrackLocation;
        };

        JumpList m_laterFailures;
        bool m_pendingFallthrough;
        Vector<DataLabelPtr, 4> m_pendingReturns;
        Vector<ReturnAddressRecord, 4> m_backtrackRecords;
    };

    // Generation methods:
    // ===================

    // This method provides a default implementation of backtracking common
    // to many terms; terms commonly jump out of the forwards  matching path
    // on any failed conditions, and add these jumps to the m_jumps list. If
    // no special handling is required we can often just backtrack to m_jumps.
    void backtrackTermDefault(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];
        m_backtrackingState.append(op.m_jumps);
    }

    void generateAssertionBOL(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];
        PatternTerm* term = op.m_term;

        if (m_pattern.m_multiline) {
            const RegisterID character = regT0;

            JumpList matchDest;
            if (!term->inputPosition)
                matchDest.append(branch32(Equal, index, Imm32(m_checked)));

            readCharacter((term->inputPosition - m_checked) - 1, character);
            matchCharacterClass(character, matchDest, m_pattern.newlineCharacterClass());
            op.m_jumps.append(jump());

            matchDest.link(this);
        } else {
            // Erk, really should poison out these alternatives early. :-/
            if (term->inputPosition)
                op.m_jumps.append(jump());
            else
                op.m_jumps.append(branch32(NotEqual, index, Imm32(m_checked)));
        }
    }
    void backtrackAssertionBOL(size_t opIndex)
    {
        backtrackTermDefault(opIndex);
    }

    void generateAssertionEOL(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];
        PatternTerm* term = op.m_term;

        if (m_pattern.m_multiline) {
            const RegisterID character = regT0;

            JumpList matchDest;
            if (term->inputPosition == m_checked)
                matchDest.append(atEndOfInput());

            readCharacter((term->inputPosition - m_checked), character);
            matchCharacterClass(character, matchDest, m_pattern.newlineCharacterClass());
            op.m_jumps.append(jump());

            matchDest.link(this);
        } else {
            if (term->inputPosition == m_checked)
                op.m_jumps.append(notAtEndOfInput());
            // Erk, really should poison out these alternatives early. :-/
            else
                op.m_jumps.append(jump());
        }
    }
    void backtrackAssertionEOL(size_t opIndex)
    {
        backtrackTermDefault(opIndex);
    }

    // Also falls though on nextIsNotWordChar.
    void matchAssertionWordchar(size_t opIndex, JumpList& nextIsWordChar, JumpList& nextIsNotWordChar)
    {
        YarrOp& op = m_ops[opIndex];
        PatternTerm* term = op.m_term;

        const RegisterID character = regT0;

        if (term->inputPosition == m_checked)
            nextIsNotWordChar.append(atEndOfInput());

        readCharacter((term->inputPosition - m_checked), character);
        matchCharacterClass(character, nextIsWordChar, m_pattern.wordcharCharacterClass());
    }

    void generateAssertionWordBoundary(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];
        PatternTerm* term = op.m_term;

        const RegisterID character = regT0;

        Jump atBegin;
        JumpList matchDest;
        if (!term->inputPosition)
            atBegin = branch32(Equal, index, Imm32(m_checked));
        readCharacter((term->inputPosition - m_checked) - 1, character);
        matchCharacterClass(character, matchDest, m_pattern.wordcharCharacterClass());
        if (!term->inputPosition)
            atBegin.link(this);

        // We fall through to here if the last character was not a wordchar.
        JumpList nonWordCharThenWordChar;
        JumpList nonWordCharThenNonWordChar;
        if (term->invert()) {
            matchAssertionWordchar(opIndex, nonWordCharThenNonWordChar, nonWordCharThenWordChar);
            nonWordCharThenWordChar.append(jump());
        } else {
            matchAssertionWordchar(opIndex, nonWordCharThenWordChar, nonWordCharThenNonWordChar);
            nonWordCharThenNonWordChar.append(jump());
        }
        op.m_jumps.append(nonWordCharThenNonWordChar);

        // We jump here if the last character was a wordchar.
        matchDest.link(this);
        JumpList wordCharThenWordChar;
        JumpList wordCharThenNonWordChar;
        if (term->invert()) {
            matchAssertionWordchar(opIndex, wordCharThenNonWordChar, wordCharThenWordChar);
            wordCharThenWordChar.append(jump());
        } else {
            matchAssertionWordchar(opIndex, wordCharThenWordChar, wordCharThenNonWordChar);
            // This can fall-though!
        }

        op.m_jumps.append(wordCharThenWordChar);

        nonWordCharThenWordChar.link(this);
        wordCharThenNonWordChar.link(this);
    }
    void backtrackAssertionWordBoundary(size_t opIndex)
    {
        backtrackTermDefault(opIndex);
    }

    void generatePatternCharacterOnce(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];

        // m_ops always ends with a OpBodyAlternativeEnd or OpMatchFailed
        // node, so there must always be at least one more node.
        ASSERT(opIndex + 1 < m_ops.size());
        YarrOp& nextOp = m_ops[opIndex + 1];

        if (op.m_isDeadCode)
            return;

        PatternTerm* term = op.m_term;
        UChar ch = term->patternCharacter;

        const RegisterID character = regT0;

        if (nextOp.m_op == OpTerm) {
            PatternTerm* nextTerm = nextOp.m_term;
            if (nextTerm->type == PatternTerm::TypePatternCharacter
                && nextTerm->quantityType == QuantifierFixedCount
                && nextTerm->quantityCount == 1
                && nextTerm->inputPosition == (term->inputPosition + 1)) {

                UChar ch2 = nextTerm->patternCharacter;

                int mask = 0;
                int chPair = ch | (ch2 << 16);

                if (m_pattern.m_ignoreCase) {
                    if (isASCIIAlpha(ch))
                        mask |= 32;
                    if (isASCIIAlpha(ch2))
                        mask |= 32 << 16;
                }

                BaseIndex address(input, index, TimesTwo, (term->inputPosition - m_checked) * sizeof(UChar));
                if (mask) {
                    load32WithUnalignedHalfWords(address, character);
                    or32(Imm32(mask), character);
                    op.m_jumps.append(branch32(NotEqual, character, Imm32(chPair | mask)));
                } else
                    op.m_jumps.append(branch32WithUnalignedHalfWords(NotEqual, address, Imm32(chPair)));

                nextOp.m_isDeadCode = true;
                return;
            }
        }

        if (m_pattern.m_ignoreCase && isASCIIAlpha(ch)) {
            readCharacter(term->inputPosition - m_checked, character);
            or32(TrustedImm32(32), character);
            op.m_jumps.append(branch32(NotEqual, character, Imm32(Unicode::toLower(ch))));
        } else {
            ASSERT(!m_pattern.m_ignoreCase || (Unicode::toLower(ch) == Unicode::toUpper(ch)));
            op.m_jumps.append(jumpIfCharNotEquals(ch, term->inputPosition - m_checked));
        }
    }
    void backtrackPatternCharacterOnce(size_t opIndex)
    {
        backtrackTermDefault(opIndex);
    }

    void generatePatternCharacterFixed(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];
        PatternTerm* term = op.m_term;
        UChar ch = term->patternCharacter;

        const RegisterID character = regT0;
        const RegisterID countRegister = regT1;

        move(index, countRegister);
        sub32(Imm32(term->quantityCount), countRegister);

        Label loop(this);
        BaseIndex address(input, countRegister, TimesTwo, (term->inputPosition - m_checked + term->quantityCount) * sizeof(UChar));

        if (m_pattern.m_ignoreCase && isASCIIAlpha(ch)) {
            load16(address, character);
            or32(TrustedImm32(32), character);
            op.m_jumps.append(branch32(NotEqual, character, Imm32(Unicode::toLower(ch))));
        } else {
            ASSERT(!m_pattern.m_ignoreCase || (Unicode::toLower(ch) == Unicode::toUpper(ch)));
            op.m_jumps.append(branch16(NotEqual, address, Imm32(ch)));
        }
        add32(TrustedImm32(1), countRegister);
        branch32(NotEqual, countRegister, index).linkTo(loop, this);
    }
    void backtrackPatternCharacterFixed(size_t opIndex)
    {
        backtrackTermDefault(opIndex);
    }

    void generatePatternCharacterGreedy(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];
        PatternTerm* term = op.m_term;
        UChar ch = term->patternCharacter;

        const RegisterID character = regT0;
        const RegisterID countRegister = regT1;

        move(TrustedImm32(0), countRegister);

        JumpList failures;
        Label loop(this);
        failures.append(atEndOfInput());
        if (m_pattern.m_ignoreCase && isASCIIAlpha(ch)) {
            readCharacter(term->inputPosition - m_checked, character);
            or32(TrustedImm32(32), character);
            failures.append(branch32(NotEqual, character, Imm32(Unicode::toLower(ch))));
        } else {
            ASSERT(!m_pattern.m_ignoreCase || (Unicode::toLower(ch) == Unicode::toUpper(ch)));
            failures.append(jumpIfCharNotEquals(ch, term->inputPosition - m_checked));
        }

        add32(TrustedImm32(1), countRegister);
        add32(TrustedImm32(1), index);
        if (term->quantityCount == quantifyInfinite)
            jump(loop);
        else
            branch32(NotEqual, countRegister, Imm32(term->quantityCount)).linkTo(loop, this);

        failures.link(this);
        op.m_reentry = label();

        storeToFrame(countRegister, term->frameLocation);

    }
    void backtrackPatternCharacterGreedy(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];
        PatternTerm* term = op.m_term;

        const RegisterID countRegister = regT1;

        m_backtrackingState.link(this);

        loadFromFrame(term->frameLocation, countRegister);
        m_backtrackingState.append(branchTest32(Zero, countRegister));
        sub32(TrustedImm32(1), countRegister);
        sub32(TrustedImm32(1), index);
        jump(op.m_reentry);
    }

    void generatePatternCharacterNonGreedy(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];
        PatternTerm* term = op.m_term;

        const RegisterID countRegister = regT1;

        move(TrustedImm32(0), countRegister);
        op.m_reentry = label();
        storeToFrame(countRegister, term->frameLocation);
    }
    void backtrackPatternCharacterNonGreedy(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];
        PatternTerm* term = op.m_term;
        UChar ch = term->patternCharacter;

        const RegisterID character = regT0;
        const RegisterID countRegister = regT1;

        JumpList nonGreedyFailures;

        m_backtrackingState.link(this);

        loadFromFrame(term->frameLocation, countRegister);

        nonGreedyFailures.append(atEndOfInput());
        if (term->quantityCount != quantifyInfinite)
            nonGreedyFailures.append(branch32(Equal, countRegister, Imm32(term->quantityCount)));
        if (m_pattern.m_ignoreCase && isASCIIAlpha(ch)) {
            readCharacter(term->inputPosition - m_checked, character);
            or32(TrustedImm32(32), character);
            nonGreedyFailures.append(branch32(NotEqual, character, Imm32(Unicode::toLower(ch))));
        } else {
            ASSERT(!m_pattern.m_ignoreCase || (Unicode::toLower(ch) == Unicode::toUpper(ch)));
            nonGreedyFailures.append(jumpIfCharNotEquals(ch, term->inputPosition - m_checked));
        }

        add32(TrustedImm32(1), countRegister);
        add32(TrustedImm32(1), index);

        jump(op.m_reentry);

        nonGreedyFailures.link(this);
        sub32(countRegister, index);
        m_backtrackingState.fallthrough();
    }

    void generateCharacterClassOnce(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];
        PatternTerm* term = op.m_term;

        const RegisterID character = regT0;

        JumpList matchDest;
        readCharacter((term->inputPosition - m_checked), character);
        matchCharacterClass(character, matchDest, term->characterClass);

        if (term->invert())
            op.m_jumps.append(matchDest);
        else {
            op.m_jumps.append(jump());
            matchDest.link(this);
        }
    }
    void backtrackCharacterClassOnce(size_t opIndex)
    {
        backtrackTermDefault(opIndex);
    }

    void generateCharacterClassFixed(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];
        PatternTerm* term = op.m_term;

        const RegisterID character = regT0;
        const RegisterID countRegister = regT1;

        move(index, countRegister);
        sub32(Imm32(term->quantityCount), countRegister);

        Label loop(this);
        JumpList matchDest;
        load16(BaseIndex(input, countRegister, TimesTwo, (term->inputPosition - m_checked + term->quantityCount) * sizeof(UChar)), character);
        matchCharacterClass(character, matchDest, term->characterClass);

        if (term->invert())
            op.m_jumps.append(matchDest);
        else {
            op.m_jumps.append(jump());
            matchDest.link(this);
        }

        add32(TrustedImm32(1), countRegister);
        branch32(NotEqual, countRegister, index).linkTo(loop, this);
    }
    void backtrackCharacterClassFixed(size_t opIndex)
    {
        backtrackTermDefault(opIndex);
    }

    void generateCharacterClassGreedy(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];
        PatternTerm* term = op.m_term;

        const RegisterID character = regT0;
        const RegisterID countRegister = regT1;

        move(TrustedImm32(0), countRegister);

        JumpList failures;
        Label loop(this);
        failures.append(atEndOfInput());

        if (term->invert()) {
            readCharacter(term->inputPosition - m_checked, character);
            matchCharacterClass(character, failures, term->characterClass);
        } else {
            JumpList matchDest;
            readCharacter(term->inputPosition - m_checked, character);
            matchCharacterClass(character, matchDest, term->characterClass);
            failures.append(jump());
            matchDest.link(this);
        }

        add32(TrustedImm32(1), countRegister);
        add32(TrustedImm32(1), index);
        if (term->quantityCount != quantifyInfinite) {
            branch32(NotEqual, countRegister, Imm32(term->quantityCount)).linkTo(loop, this);
            failures.append(jump());
        } else
            jump(loop);

        failures.link(this);
        op.m_reentry = label();

        storeToFrame(countRegister, term->frameLocation);
    }
    void backtrackCharacterClassGreedy(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];
        PatternTerm* term = op.m_term;

        const RegisterID countRegister = regT1;

        m_backtrackingState.link(this);

        loadFromFrame(term->frameLocation, countRegister);
        m_backtrackingState.append(branchTest32(Zero, countRegister));
        sub32(TrustedImm32(1), countRegister);
        sub32(TrustedImm32(1), index);
        jump(op.m_reentry);
    }

    void generateCharacterClassNonGreedy(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];
        PatternTerm* term = op.m_term;

        const RegisterID countRegister = regT1;

        move(TrustedImm32(0), countRegister);
        op.m_reentry = label();
        storeToFrame(countRegister, term->frameLocation);
    }
    void backtrackCharacterClassNonGreedy(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];
        PatternTerm* term = op.m_term;

        const RegisterID character = regT0;
        const RegisterID countRegister = regT1;

        JumpList nonGreedyFailures;

        m_backtrackingState.link(this);

        Label backtrackBegin(this);
        loadFromFrame(term->frameLocation, countRegister);

        nonGreedyFailures.append(atEndOfInput());
        nonGreedyFailures.append(branch32(Equal, countRegister, Imm32(term->quantityCount)));

        JumpList matchDest;
        readCharacter(term->inputPosition - m_checked, character);
        matchCharacterClass(character, matchDest, term->characterClass);

        if (term->invert())
            nonGreedyFailures.append(matchDest);
        else {
            nonGreedyFailures.append(jump());
            matchDest.link(this);
        }

        add32(TrustedImm32(1), countRegister);
        add32(TrustedImm32(1), index);

        jump(op.m_reentry);

        nonGreedyFailures.link(this);
        sub32(countRegister, index);
        m_backtrackingState.fallthrough();
    }

    // Code generation/backtracking for simple terms
    // (pattern characters, character classes, and assertions).
    // These methods farm out work to the set of functions above.
    void generateTerm(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];
        PatternTerm* term = op.m_term;

        switch (term->type) {
        case PatternTerm::TypePatternCharacter:
            switch (term->quantityType) {
            case QuantifierFixedCount:
                if (term->quantityCount == 1)
                    generatePatternCharacterOnce(opIndex);
                else
                    generatePatternCharacterFixed(opIndex);
                break;
            case QuantifierGreedy:
                generatePatternCharacterGreedy(opIndex);
                break;
            case QuantifierNonGreedy:
                generatePatternCharacterNonGreedy(opIndex);
                break;
            }
            break;

        case PatternTerm::TypeCharacterClass:
            switch (term->quantityType) {
            case QuantifierFixedCount:
                if (term->quantityCount == 1)
                    generateCharacterClassOnce(opIndex);
                else
                    generateCharacterClassFixed(opIndex);
                break;
            case QuantifierGreedy:
                generateCharacterClassGreedy(opIndex);
                break;
            case QuantifierNonGreedy:
                generateCharacterClassNonGreedy(opIndex);
                break;
            }
            break;

        case PatternTerm::TypeAssertionBOL:
            generateAssertionBOL(opIndex);
            break;

        case PatternTerm::TypeAssertionEOL:
            generateAssertionEOL(opIndex);
            break;

        case PatternTerm::TypeAssertionWordBoundary:
            generateAssertionWordBoundary(opIndex);
            break;

        case PatternTerm::TypeForwardReference:
            break;

        case PatternTerm::TypeParenthesesSubpattern:
        case PatternTerm::TypeParentheticalAssertion:
            ASSERT_NOT_REACHED();
        case PatternTerm::TypeBackReference:
            m_shouldFallBack = true;
            break;
        }
    }
    void backtrackTerm(size_t opIndex)
    {
        YarrOp& op = m_ops[opIndex];
        PatternTerm* term = op.m_term;

        switch (term->type) {
        case PatternTerm::TypePatternCharacter:
            switch (term->quantityType) {
            case QuantifierFixedCount:
                if (term->quantityCount == 1)
                    backtrackPatternCharacterOnce(opIndex);
                else
                    backtrackPatternCharacterFixed(opIndex);
                break;
            case QuantifierGreedy:
                backtrackPatternCharacterGreedy(opIndex);
                break;
            case QuantifierNonGreedy:
                backtrackPatternCharacterNonGreedy(opIndex);
                break;
            }
            break;

        case PatternTerm::TypeCharacterClass:
            switch (term->quantityType) {
            case QuantifierFixedCount:
                if (term->quantityCount == 1)
                    backtrackCharacterClassOnce(opIndex);
                else
                    backtrackCharacterClassFixed(opIndex);
                break;
            case QuantifierGreedy:
                backtrackCharacterClassGreedy(opIndex);
                break;
            case QuantifierNonGreedy:
                backtrackCharacterClassNonGreedy(opIndex);
                break;
            }
            break;

        case PatternTerm::TypeAssertionBOL:
            backtrackAssertionBOL(opIndex);
            break;

        case PatternTerm::TypeAssertionEOL:
            backtrackAssertionEOL(opIndex);
            break;

        case PatternTerm::TypeAssertionWordBoundary:
            backtrackAssertionWordBoundary(opIndex);
            break;

        case PatternTerm::TypeForwardReference:
            break;

        case PatternTerm::TypeParenthesesSubpattern:
        case PatternTerm::TypeParentheticalAssertion:
            ASSERT_NOT_REACHED();
        case PatternTerm::TypeBackReference:
            m_shouldFallBack = true;
            break;
        }
    }

    void generate()
    {
        // Forwards generate the matching code.
        ASSERT(m_ops.size());
        size_t opIndex = 0;

        do {
            YarrOp& op = m_ops[opIndex];
            switch (op.m_op) {

            case OpTerm:
                generateTerm(opIndex);
                break;

            // OpBodyAlternativeBegin/Next/End
            //
            // These nodes wrap the set of alternatives in the body of the regular expression.
            // There may be either one or two chains of OpBodyAlternative nodes, one representing
            // the 'once through' sequence of alternatives (if any exist), and one representing
            // the repeating alternatives (again, if any exist).
            //
            // Upon normal entry to the Begin alternative, we will check that input is available.
            // Reentry to the Begin alternative will take place after the check has taken place,
            // and will assume that the input position has already been progressed as appropriate.
            //
            // Entry to subsequent Next/End alternatives occurs when the prior alternative has
            // successfully completed a match - return a success state from JIT code.
            //
            // Next alternatives allow for reentry optimized to suit backtracking from its
            // preceding alternative. It expects the input position to still be set to a position
            // appropriate to its predecessor, and it will only perform an input check if the
            // predecessor had a minimum size less than its own.
            //
            // In the case 'once through' expressions, the End node will also have a reentry
            // point to jump to when the last alternative fails. Again, this expects the input
            // position to still reflect that expected by the prior alternative.
            case OpBodyAlternativeBegin: {
                PatternAlternative* alternative = op.m_alternative;

                // Upon entry at the head of the set of alternatives, check if input is available
                // to run the first alternative. (This progresses the input position).
                op.m_jumps.append(jumpIfNoAvailableInput(alternative->m_minimumSize));
                // We will reenter after the check, and assume the input position to have been
                // set as appropriate to this alternative.
                op.m_reentry = label();

                m_checked += alternative->m_minimumSize;
                break;
            }
            case OpBodyAlternativeNext:
            case OpBodyAlternativeEnd: {
                PatternAlternative* priorAlternative = m_ops[op.m_previousOp].m_alternative;
                PatternAlternative* alternative = op.m_alternative;

                // If we get here, the prior alternative matched - return success.
                
                // Adjust the stack pointer to remove the pattern's frame.
                if (m_pattern.m_body->m_callFrameSize)
                    addPtr(Imm32(m_pattern.m_body->m_callFrameSize * sizeof(void*)), stackPointerRegister);

                // Load appropriate values into the return register and the first output
                // slot, and return. In the case of pattern with a fixed size, we will
                // not have yet set the value in the first 
                ASSERT(index != returnRegister);
                if (m_pattern.m_body->m_hasFixedSize) {
                    move(index, returnRegister);
                    if (priorAlternative->m_minimumSize)
                        sub32(Imm32(priorAlternative->m_minimumSize), returnRegister);
                    store32(returnRegister, output);
                } else
                    load32(Address(output), returnRegister);
                store32(index, Address(output, 4));
                generateReturn();

                // This is the divide between the tail of the prior alternative, above, and
                // the head of the subsequent alternative, below.

                if (op.m_op == OpBodyAlternativeNext) {
                    // This is the reentry point for the Next alternative. We expect any code
                    // that jumps here to do so with the input position matching that of the
                    // PRIOR alteranative, and we will only check input availability if we
                    // need to progress it forwards.
                    op.m_reentry = label();
                    if (int delta = alternative->m_minimumSize - priorAlternative->m_minimumSize) {
                        add32(Imm32(delta), index);
                        if (delta > 0)
                            op.m_jumps.append(jumpIfNoAvailableInput());
                    }
                } else if (op.m_nextOp == notFound) {
                    // This is the reentry point for the End of 'once through' alternatives,
                    // jumped to when the las alternative fails to match.
                    op.m_reentry = label();
                    sub32(Imm32(priorAlternative->m_minimumSize), index);
                }

                if (op.m_op == OpBodyAlternativeNext)
                    m_checked += alternative->m_minimumSize;
                m_checked -= priorAlternative->m_minimumSize;
                break;
            }

            // OpSimpleNestedAlternativeBegin/Next/End
            // OpNestedAlternativeBegin/Next/End
            //
            // These nodes are used to handle sets of alternatives that are nested within
            // subpatterns and parenthetical assertions. The 'simple' forms are used where
            // we do not need to be able to backtrack back into any alternative other than
            // the last, the normal forms allow backtracking into any alternative.
            //
            // Each Begin/Next node is responsible for planting an input check to ensure
            // sufficient input is available on entry. Next nodes additionally need to
            // jump to the end - Next nodes use the End node's m_jumps list to hold this
            // set of jumps.
            //
            // In the non-simple forms, successful alternative matches must store a
            // 'return address' using a DataLabelPtr, used to store the address to jump
            // to when backtracking, to get to the code for the appropriate alternative.
            case OpSimpleNestedAlternativeBegin:
            case OpNestedAlternativeBegin: {
                PatternTerm* term = op.m_term;
                PatternAlternative* alternative = op.m_alternative;
                PatternDisjunction* disjunction = term->parentheses.disjunction;

                // Calculate how much input we need to check for, and if non-zero check.
                op.m_checkAdjust = alternative->m_minimumSize;
                if ((term->quantityType == QuantifierFixedCount) && (term->type != PatternTerm::TypeParentheticalAssertion))
                    op.m_checkAdjust -= disjunction->m_minimumSize;
                if (op.m_checkAdjust)
                    op.m_jumps.append(jumpIfNoAvailableInput(op.m_checkAdjust));
 
                m_checked += op.m_checkAdjust;
                break;
            }
            case OpSimpleNestedAlternativeNext:
            case OpNestedAlternativeNext: {
                PatternTerm* term = op.m_term;
                PatternAlternative* alternative = op.m_alternative;
                PatternDisjunction* disjunction = term->parentheses.disjunction;

                // In the non-simple case, store a 'return address' so we can backtrack correctly.
                if (op.m_op == OpNestedAlternativeNext) {
                    unsigned parenthesesFrameLocation = term->frameLocation;
                    unsigned alternativeFrameLocation = parenthesesFrameLocation;
                    if (term->quantityType != QuantifierFixedCount)
                        alternativeFrameLocation += YarrStackSpaceForBackTrackInfoParenthesesOnce;
                    op.m_returnAddress = storeToFrameWithPatch(alternativeFrameLocation);
                }

                // If we reach here then the last alternative has matched - jump to the
                // End node, to skip over any further alternatives.
                //
                // FIXME: this is logically O(N^2) (though N can be expected to be very
                // small). We could avoid this either by adding an extra jump to the JIT
                // data structures, or by making backtracking code that jumps to Next
                // alternatives are responsible for checking that input is available (if
                // we didn't need to plant the input checks, then m_jumps would be free).
                YarrOp* endOp = &m_ops[op.m_nextOp];
                while (endOp->m_nextOp != notFound) {
                    ASSERT(endOp->m_op == OpSimpleNestedAlternativeNext || endOp->m_op == OpNestedAlternativeNext);
                    endOp = &m_ops[endOp->m_nextOp];
                }
                ASSERT(endOp->m_op == OpSimpleNestedAlternativeEnd || endOp->m_op == OpNestedAlternativeEnd);
                endOp->m_jumps.append(jump());

                // This is the entry point for the next alternative.
                op.m_reentry = label();

                // Calculate how much input we need to check for, and if non-zero check.
                op.m_checkAdjust = alternative->m_minimumSize;
                if ((term->quantityType == QuantifierFixedCount) && (term->type != PatternTerm::TypeParentheticalAssertion))
                    op.m_checkAdjust -= disjunction->m_minimumSize;
                if (op.m_checkAdjust)
                    op.m_jumps.append(jumpIfNoAvailableInput(op.m_checkAdjust));

                YarrOp& lastOp = m_ops[op.m_previousOp];
                m_checked -= lastOp.m_checkAdjust;
                m_checked += op.m_checkAdjust;
                break;
            }
            case OpSimpleNestedAlternativeEnd:
            case OpNestedAlternativeEnd: {
                PatternTerm* term = op.m_term;

                // In the non-simple case, store a 'return address' so we can backtrack correctly.
                if (op.m_op == OpNestedAlternativeEnd) {
                    unsigned parenthesesFrameLocation = term->frameLocation;
                    unsigned alternativeFrameLocation = parenthesesFrameLocation;
                    if (term->quantityType != QuantifierFixedCount)
                        alternativeFrameLocation += YarrStackSpaceForBackTrackInfoParenthesesOnce;
                    op.m_returnAddress = storeToFrameWithPatch(alternativeFrameLocation);
                }

                // If this set of alternatives contains more than one alternative,
                // then the Next nodes will have planted jumps to the End, and added
                // them to this node's m_jumps list.
                op.m_jumps.link(this);
                op.m_jumps.clear();

                YarrOp& lastOp = m_ops[op.m_previousOp];
                m_checked -= lastOp.m_checkAdjust;
                break;
            }

            // OpParenthesesSubpatternOnceBegin/End
            //
            // These nodes support (optionally) capturing subpatterns, that have a
            // quantity count of 1 (this covers fixed once, and ?/?? quantifiers). 
            case OpParenthesesSubpatternOnceBegin: {
                PatternTerm* term = op.m_term;
                unsigned parenthesesFrameLocation = term->frameLocation;
                const RegisterID indexTemporary = regT0;
                ASSERT(term->quantityCount == 1);

                // Upon entry to a Greedy quantified set of parenthese store the index.
                // We'll use this for two purposes:
                //  - To indicate which iteration we are on of mathing the remainder of
                //    the expression after the parentheses - the first, including the
                //    match within the parentheses, or the second having skipped over them.
                //  - To check for empty matches, which must be rejected.
                //
                // At the head of a NonGreedy set of parentheses we'll immediately set the
                // value on the stack to -1 (indicating a match skipping the subpattern),
                // and plant a jump to the end. We'll also plant a label to backtrack to
                // to reenter the subpattern later, with a store to set up index on the
                // second iteration.
                //
                // FIXME: for capturing parens, could use the index in the capture array?
                if (term->quantityType == QuantifierGreedy)
                    storeToFrame(index, parenthesesFrameLocation);
                else if (term->quantityType == QuantifierNonGreedy) {
                    storeToFrame(TrustedImm32(-1), parenthesesFrameLocation);
                    op.m_jumps.append(jump());
                    op.m_reentry = label();
                    storeToFrame(index, parenthesesFrameLocation);
                }

                // If the parenthese are capturing, store the starting index value to the
                // captures array, offsetting as necessary.
                //
                // FIXME: could avoid offsetting this value in JIT code, apply
                // offsets only afterwards, at the point the results array is
                // being accessed.
                if (term->capture()) {
                    int offsetId = term->parentheses.subpatternId << 1;
                    int inputOffset = term->inputPosition - m_checked;
                    if (term->quantityType == QuantifierFixedCount)
                        inputOffset -= term->parentheses.disjunction->m_minimumSize;
                    if (inputOffset) {
                        move(index, indexTemporary);
                        add32(Imm32(inputOffset), indexTemporary);
                        store32(indexTemporary, Address(output, offsetId * sizeof(int)));
                    } else
                        store32(index, Address(output, offsetId * sizeof(int)));
                }
                break;
            }
            case OpParenthesesSubpatternOnceEnd: {
                PatternTerm* term = op.m_term;
                unsigned parenthesesFrameLocation = term->frameLocation;
                const RegisterID indexTemporary = regT0;
                ASSERT(term->quantityCount == 1);

                // For Greedy/NonGreedy quantified parentheses, we must reject zero length
                // matches. If the minimum size is know to be non-zero we need not check.
                if (term->quantityType != QuantifierFixedCount && !term->parentheses.disjunction->m_minimumSize)
                    op.m_jumps.append(branch32(Equal, index, Address(stackPointerRegister, parenthesesFrameLocation * sizeof(void*))));

                // If the parenthese are capturing, store the ending index value to the
                // captures array, offsetting as necessary.
                //
                // FIXME: could avoid offsetting this value in JIT code, apply
                // offsets only afterwards, at the point the results array is
                // being accessed.
                if (term->capture()) {
                    int offsetId = (term->parentheses.subpatternId << 1) + 1;
                    int inputOffset = term->inputPosition - m_checked;
                    if (inputOffset) {
                        move(index, indexTemporary);
                        add32(Imm32(inputOffset), indexTemporary);
                        store32(indexTemporary, Address(output, offsetId * sizeof(int)));
                    } else
                        store32(index, Address(output, offsetId * sizeof(int)));
                }

                // If the parentheses are quantified Greedy then add a label to jump back
                // to if get a failed match from after the parentheses. For NonGreedy
                // parentheses, link the jump from before the subpattern to here.
                if (term->quantityType == QuantifierGreedy)
                    op.m_reentry = label();
                else if (term->quantityType == QuantifierNonGreedy) {
                    YarrOp& beginOp = m_ops[op.m_previousOp];
                    beginOp.m_jumps.link(this);
                }
                break;
            }

            // OpParenthesesSubpatternTerminalBegin/End
            case OpParenthesesSubpatternTerminalBegin: {
                PatternTerm* term = op.m_term;
                ASSERT(term->quantityType == QuantifierGreedy);
                ASSERT(term->quantityCount == quantifyInfinite);
                ASSERT(!term->capture());

                // Upon entry set a label to loop back to.
                op.m_reentry = label();

                // Store the start index of the current match; we need to reject zero
                // length matches.
                storeToFrame(index, term->frameLocation);
                break;
            }
            case OpParenthesesSubpatternTerminalEnd: {
                PatternTerm* term = op.m_term;

                // Check for zero length matches - if the match is non-zero, then we
                // can accept it & loop back up to the head of the subpattern.
                YarrOp& beginOp = m_ops[op.m_previousOp];
                branch32(NotEqual, index, Address(stackPointerRegister, term->frameLocation * sizeof(void*)), beginOp.m_reentry);

                // Reject the match - backtrack back into the subpattern.
                op.m_jumps.append(jump());

                // This is the entry point to jump to when we stop matching - we will
                // do so once the subpattern cannot match any more.
                op.m_reentry = label();
                break;
            }

            // OpParentheticalAssertionBegin/End
            case OpParentheticalAssertionBegin: {
                PatternTerm* term = op.m_term;

                // Store the current index - assertions should not update index, so
                // we will need to restore it upon a successful match.
                unsigned parenthesesFrameLocation = term->frameLocation;
                storeToFrame(index, parenthesesFrameLocation);

                // Check 
                op.m_checkAdjust = m_checked - term->inputPosition;
                if (op.m_checkAdjust)
                    sub32(Imm32(op.m_checkAdjust), index);

                m_checked -= op.m_checkAdjust;
                break;
            }
            case OpParentheticalAssertionEnd: {
                PatternTerm* term = op.m_term;

                // Restore the input index value.
                unsigned parenthesesFrameLocation = term->frameLocation;
                loadFromFrame(parenthesesFrameLocation, index);

                // If inverted, a successful match of the assertion must be treated
                // as a failure, so jump to backtracking.
                if (term->invert()) {
                    op.m_jumps.append(jump());
                    op.m_reentry = label();
                }

                YarrOp& lastOp = m_ops[op.m_previousOp];
                m_checked += lastOp.m_checkAdjust;
                break;
            }

            case OpMatchFailed:
                if (m_pattern.m_body->m_callFrameSize)
                    addPtr(Imm32(m_pattern.m_body->m_callFrameSize * sizeof(void*)), stackPointerRegister);
                move(TrustedImm32(-1), returnRegister);
                generateReturn();
                break;
            }

            ++opIndex;
        } while (opIndex < m_ops.size());
    }

    void backtrack()
    {
        // Backwards generate the backtracking code.
        size_t opIndex = m_ops.size();
        ASSERT(opIndex);

        do {
            --opIndex;
            YarrOp& op = m_ops[opIndex];
            switch (op.m_op) {

            case OpTerm:
                backtrackTerm(opIndex);
                break;

            // OpBodyAlternativeBegin/Next/End
            //
            // For each Begin/Next node representing an alternative, we need to decide what to do
            // in two circumstances:
            //  - If we backtrack back into this node, from within the alternative.
            //  - If the input check at the head of the alternative fails (if this exists).
            //
            // We treat these two cases differently since in the former case we have slightly
            // more information - since we are backtracking out of a prior alternative we know
            // that at least enough input was available to run it. For example, given the regular
            // expression /a|b/, if we backtrack out of the first alternative (a failed pattern
            // character match of 'a'), then we need not perform an additional input availability
            // check before running the second alternative.
            //
            // Backtracking required differs for the last alternative, which in the case of the
            // repeating set of alternatives must loop. The code generated for the last alternative
            // will also be used to handle all input check failures from any prior alternatives -
            // these require similar functionality, in seeking the next available alternative for
            // which there is sufficient input.
            //
            // Since backtracking of all other alternatives simply requires us to link backtracks
            // to the reentry point for the subsequent alternative, we will only be generating any
            // code when backtracking the last alternative.
            case OpBodyAlternativeBegin:
            case OpBodyAlternativeNext: {
                PatternAlternative* alternative = op.m_alternative;

                if (op.m_op == OpBodyAlternativeNext) {
                    PatternAlternative* priorAlternative = m_ops[op.m_previousOp].m_alternative;
                    m_checked += priorAlternative->m_minimumSize;
                }
                m_checked -= alternative->m_minimumSize;

                // Is this the last alternative? If not, then if we backtrack to this point we just
                // need to jump to try to match the next alternative.
                if (m_ops[op.m_nextOp].m_op != OpBodyAlternativeEnd) {
                    m_backtrackingState.linkTo(m_ops[op.m_nextOp].m_reentry, this);
                    break;
                }
                YarrOp& endOp = m_ops[op.m_nextOp];

                YarrOp* beginOp = &op;
                while (beginOp->m_op != OpBodyAlternativeBegin) {
                    ASSERT(beginOp->m_op == OpBodyAlternativeNext);
                    beginOp = &m_ops[beginOp->m_previousOp];
                }

                bool onceThrough = endOp.m_nextOp == notFound;

                // First, generate code to handle cases where we backtrack out of an attempted match
                // of the last alternative. If this is a 'once through' set of alternatives then we
                // have nothing to do - link this straight through to the End.
                if (onceThrough)
                    m_backtrackingState.linkTo(endOp.m_reentry, this);
                else {
                    // Okay, we're going to need to loop. Calculate the delta between where the input
                    // position was, and where we want it to be allowing for the fact that we need to
                    // increment by 1. E.g. for the regexp /a|x/ we need to increment the position by
                    // 1 between loop iterations, but for /abcd|xyz/ we need to increment by two when
                    // looping from the last alternative to the first, for /a|xyz/ we need to decrement
                    // by 1, and for /a|xy/ we don't need to move the input position at all.
                    int deltaLastAlternativeToFirstAlternativePlusOne = (beginOp->m_alternative->m_minimumSize - alternative->m_minimumSize) + 1;

                    // If we don't need to move the input poistion, and the pattern has a fixed size
                    // (in which case we omit the store of the start index until the pattern has matched)
                    // then we can just link the backtrack out of the last alternative straight to the
                    // head of the first alternative.
                    if (!deltaLastAlternativeToFirstAlternativePlusOne && m_pattern.m_body->m_hasFixedSize)
                        m_backtrackingState.linkTo(beginOp->m_reentry, this);
                    else {
                        // We need to generate a trampoline of code to execute before looping back
                        // around to the first alternative.
                        m_backtrackingState.link(this);

                        // If the pattern size is not fixed, then store the start index, for use if we match.
                        if (!m_pattern.m_body->m_hasFixedSize) {
                            if (alternative->m_minimumSize == 1)
                                store32(index, Address(output));
                            else {
                                move(index, regT0);
                                if (alternative->m_minimumSize)
                                    sub32(Imm32(alternative->m_minimumSize - 1), regT0);
                                else
                                    add32(Imm32(1), regT0);
                                store32(regT0, Address(output));
                            }
                        }

                        if (deltaLastAlternativeToFirstAlternativePlusOne)
                            add32(Imm32(deltaLastAlternativeToFirstAlternativePlusOne), index);

                        // Loop. Since this code is only reached when we backtrack out of the last
                        // alternative (and NOT linked to from the input check upon entry to the
                        // last alternative) we know that there must be at least enough input as
                        // required by the last alternative. As such, we only need to check if the
                        // first will require more to run - if the same or less is required we can
                        // unconditionally jump.
                        if (deltaLastAlternativeToFirstAlternativePlusOne > 0)
                            checkInput().linkTo(beginOp->m_reentry, this);
                        else
                            jump(beginOp->m_reentry);
                    }
                }

                // We can reach this point in the code in two ways:
                //  - Fallthrough from the code above (a repeating alternative backtracked out of its
                //    last alternative, and did not have sufficent input to run the first).
                //  - We will loop back up to the following label when a releating alternative loops,
                //    following a failed input check.
                //
                // Either way, we have just failed the input check for the first alternative.
                Label firstInputCheckFailed(this);

                // Generate code to handle input check failures from alternatives except the last.
                // prevOp is the alternative we're handling a bail out from (initially Begin), and
                // nextOp is the alternative we will be attempting to reenter into.
                // 
                // We will link input check failures from the forwards matching path back to the code
                // that can handle them.
                YarrOp* prevOp = beginOp;
                YarrOp* nextOp = &m_ops[beginOp->m_nextOp];
                while (nextOp->m_op != OpBodyAlternativeEnd) {
                    prevOp->m_jumps.link(this);

                    int delta = nextOp->m_alternative->m_minimumSize - prevOp->m_alternative->m_minimumSize;
                    if (delta)
                        add32(Imm32(delta), index);

                    // We only get here if an input check fails, it is only worth checking again
                    // if the next alternative has a minimum size less than the last.
                    if (delta < 0) {
                        // FIXME: if we added an extra label to YarrOp, we could avoid needing to
                        // subtract delta back out, and reduce this code. Should performance test
                        // the benefit of this.
                        Jump fail = jumpIfNoAvailableInput();
                        sub32(Imm32(delta), index);
                        jump(nextOp->m_reentry);
                        fail.link(this);
                    }
                    prevOp = nextOp;
                    nextOp = &m_ops[nextOp->m_nextOp];
                }

                // We fall through to here if there is insufficient input to run the last alternative.

                // If there is insufficient input to run the last alternative, then for 'once through'
                // alternatives we are done - just jump back up into the forwards matching path at the End.
                if (onceThrough) {
                    op.m_jumps.linkTo(endOp.m_reentry, this);
                    jump(endOp.m_reentry);
                    break;
                }

                // For repeating alternatives, link any input check failure from the last alternative to
                // this point.
                op.m_jumps.link(this);

                bool needsToUpdateMatchStart = !m_pattern.m_body->m_hasFixedSize;

                // Check for cases where input position is already incremented by 1 for the last
                // alternative (this is particularly useful where the minimum size of the body
                // disjunction is 0, e.g. /a*|b/).
                if (needsToUpdateMatchStart && alternative->m_minimumSize == 1) {
                    // index is already incremented by 1, so just store it now!
                    store32(index, Address(output));
                    needsToUpdateMatchStart = false;
                }

                // Check whether there is sufficient input to loop. Increment the input position by
                // one, and check. Also add in the minimum disjunction size before checking - there
                // is no point in looping if we're just going to fail all the input checks around
                // the next iteration.
                int deltaLastAlternativeToBodyMinimumPlusOne = (m_pattern.m_body->m_minimumSize + 1) - alternative->m_minimumSize;
                if (deltaLastAlternativeToBodyMinimumPlusOne)
                    add32(Imm32(deltaLastAlternativeToBodyMinimumPlusOne), index);
                Jump matchFailed = jumpIfNoAvailableInput();

                if (needsToUpdateMatchStart) {
                    if (!m_pattern.m_body->m_minimumSize)
                        store32(index, Address(output));
                    else {
                        move(index, regT0);
                        sub32(Imm32(m_pattern.m_body->m_minimumSize), regT0);
                        store32(regT0, Address(output));
                    }
                }

                // Calculate how much more input the first alternative requires than the minimum
                // for the body as a whole. If no more is needed then we dont need an additional
                // input check here - jump straight back up to the start of the first alternative.
                int deltaBodyMinimumToFirstAlternative = beginOp->m_alternative->m_minimumSize - m_pattern.m_body->m_minimumSize;
                if (!deltaBodyMinimumToFirstAlternative)
                    jump(beginOp->m_reentry);
                else {
                    add32(Imm32(deltaBodyMinimumToFirstAlternative), index);
                    checkInput().linkTo(beginOp->m_reentry, this);
                    jump(firstInputCheckFailed);
                }

                // We jump to here if we iterate to the point that there is insufficient input to
                // run any matches, and need to return a failure state from JIT code.
                matchFailed.link(this);

                if (m_pattern.m_body->m_callFrameSize)
                    addPtr(Imm32(m_pattern.m_body->m_callFrameSize * sizeof(void*)), stackPointerRegister);
                move(TrustedImm32(-1), returnRegister);
                generateReturn();
                break;
            }
            case OpBodyAlternativeEnd: {
                // We should never backtrack back into a body disjunction.
                ASSERT(m_backtrackingState.isEmpty());

                PatternAlternative* priorAlternative = m_ops[op.m_previousOp].m_alternative;
                m_checked += priorAlternative->m_minimumSize;
                break;
            }

            // OpSimpleNestedAlternativeBegin/Next/End
            // OpNestedAlternativeBegin/Next/End
            //
            // Generate code for when we backtrack back out of an alternative into
            // a Begin or Next node, or when the entry input count check fails. If
            // there are more alternatives we need to jump to the next alternative,
            // if not we backtrack back out of the current set of parentheses.
            //
            // In the case of non-simple nested assertions we need to also link the
            // 'return address' appropriately to backtrack back out into the correct
            // alternative.
            case OpSimpleNestedAlternativeBegin:
            case OpSimpleNestedAlternativeNext:
            case OpNestedAlternativeBegin:
            case OpNestedAlternativeNext: {
                YarrOp& nextOp = m_ops[op.m_nextOp];
                bool isBegin = op.m_previousOp == notFound;
                bool isLastAlternative = nextOp.m_nextOp == notFound;
                ASSERT(isBegin == (op.m_op == OpSimpleNestedAlternativeBegin || op.m_op == OpNestedAlternativeBegin));
                ASSERT(isLastAlternative == (nextOp.m_op == OpSimpleNestedAlternativeEnd || nextOp.m_op == OpNestedAlternativeEnd));

                // Treat an input check failure the same as a failed match.
                m_backtrackingState.append(op.m_jumps);

                // Set the backtracks to jump to the appropriate place. We may need
                // to link the backtracks in one of three different way depending on
                // the type of alternative we are dealing with:
                //  - A single alternative, with no simplings.
                //  - The last alternative of a set of two or more.
                //  - An alternative other than the last of a set of two or more.
                //
                // In the case of a single alternative on its own, we don't need to
                // jump anywhere - if the alternative fails to match we can just
                // continue to backtrack out of the parentheses without jumping.
                //
                // In the case of the last alternative in a set of more than one, we
                // need to jump to return back out to the beginning. We'll do so by
                // adding a jump to the End node's m_jumps list, and linking this
                // when we come to generate the Begin node. For alternatives other
                // than the last, we need to jump to the next alternative.
                //
                // If the alternative had adjusted the input position we must link
                // backtracking to here, correct, and then jump on. If not we can
                // link the backtracks directly to their destination.
                if (op.m_checkAdjust) {
                    // Handle the cases where we need to link the backtracks here.
                    m_backtrackingState.link(this);
                    sub32(Imm32(op.m_checkAdjust), index);
                    if (!isLastAlternative) {
                        // An alternative that is not the last should jump to its successor.
                        jump(nextOp.m_reentry);
                    } else if (!isBegin) {
                        // The last of more than one alternatives must jump back to the begnning.
                        nextOp.m_jumps.append(jump());
                    } else {
                        // A single alternative on its own can fall through.
                        m_backtrackingState.fallthrough();
                    }
                } else {
                    // Handle the cases where we can link the backtracks directly to their destinations.
                    if (!isLastAlternative) {
                        // An alternative that is not the last should jump to its successor.
                        m_backtrackingState.linkTo(nextOp.m_reentry, this);
                    } else if (!isBegin) {
                        // The last of more than one alternatives must jump back to the begnning.
                        m_backtrackingState.takeBacktracksToJumpList(nextOp.m_jumps, this);
                    }
                    // In the case of a single alternative on its own do nothing - it can fall through.
                }

                // At this point we've handled the backtracking back into this node.
                // Now link any backtracks that need to jump to here.

                // For non-simple alternatives, link the alternative's 'return address'
                // so that we backtrack back out into the previous alternative.
                if (op.m_op == OpNestedAlternativeNext)
                    m_backtrackingState.append(op.m_returnAddress);

                // If there is more than one alternative, then the last alternative will
                // have planted a jump to be linked to the end. This jump was added to the
                // End node's m_jumps list. If we are back at the beginning, link it here.
                if (isBegin) {
                    YarrOp* endOp = &m_ops[op.m_nextOp];
                    while (endOp->m_nextOp != notFound) {
                        ASSERT(endOp->m_op == OpSimpleNestedAlternativeNext || endOp->m_op == OpNestedAlternativeNext);
                        endOp = &m_ops[endOp->m_nextOp];
                    }
                    ASSERT(endOp->m_op == OpSimpleNestedAlternativeEnd || endOp->m_op == OpNestedAlternativeEnd);
                    m_backtrackingState.append(endOp->m_jumps);
                }

                if (!isBegin) {
                    YarrOp& lastOp = m_ops[op.m_previousOp];
                    m_checked += lastOp.m_checkAdjust;
                }
                m_checked -= op.m_checkAdjust;
                break;
            }
            case OpSimpleNestedAlternativeEnd:
            case OpNestedAlternativeEnd: {
                PatternTerm* term = op.m_term;

                // If we backtrack into the end of a simple subpattern do nothing;
                // just continue through into the last alternative. If we backtrack
                // into the end of a non-simple set of alterntives we need to jump
                // to the backtracking return address set up during generation.
                if (op.m_op == OpNestedAlternativeEnd) {
                    m_backtrackingState.link(this);

                    // Plant a jump to the return address.
                    unsigned parenthesesFrameLocation = term->frameLocation;
                    unsigned alternativeFrameLocation = parenthesesFrameLocation;
                    if (term->quantityType != QuantifierFixedCount)
                        alternativeFrameLocation += YarrStackSpaceForBackTrackInfoParenthesesOnce;
                    loadFromFrameAndJump(alternativeFrameLocation);

                    // Link the DataLabelPtr associated with the end of the last
                    // alternative to this point.
                    m_backtrackingState.append(op.m_returnAddress);
                }

                YarrOp& lastOp = m_ops[op.m_previousOp];
                m_checked += lastOp.m_checkAdjust;
                break;
            }

            // OpParenthesesSubpatternOnceBegin/End
            //
            // When we are backtracking back out of a capturing subpattern we need
            // to clear the start index in the matches output array, to record that
            // this subpattern has not been captured.
            //
            // When backtracking back out of a Greedy quantified subpattern we need
            // to catch this, and try running the remainder of the alternative after
            // the subpattern again, skipping the parentheses.
            //
            // Upon backtracking back into a quantified set of parentheses we need to
            // check whether we were currently skipping the subpattern. If not, we
            // can backtrack into them, if we were we need to either backtrack back
            // out of the start of the parentheses, or jump back to the forwards
            // matching start, depending of whether the match is Greedy or NonGreedy.
            case OpParenthesesSubpatternOnceBegin: {
                PatternTerm* term = op.m_term;
                ASSERT(term->quantityCount == 1);

                // We only need to backtrack to thispoint if capturing or greedy.
                if (term->capture() || term->quantityType == QuantifierGreedy) {
                    m_backtrackingState.link(this);

                    // If capturing, clear the capture (we only need to reset start).
                    if (term->capture())
                        store32(TrustedImm32(-1), Address(output, (term->parentheses.subpatternId << 1) * sizeof(int)));

                    // If Greedy, jump to the end.
                    if (term->quantityType == QuantifierGreedy) {
                        // Clear the flag in the stackframe indicating we ran through the subpattern.
                        unsigned parenthesesFrameLocation = term->frameLocation;
                        storeToFrame(TrustedImm32(-1), parenthesesFrameLocation);
                        // Jump to after the parentheses, skipping the subpattern.
                        jump(m_ops[op.m_nextOp].m_reentry);
                        // A backtrack from after the parentheses, when skipping the subpattern,
                        // will jump back to here.
                        op.m_jumps.link(this);
                    }

                    m_backtrackingState.fallthrough();
                }
                break;
            }
            case OpParenthesesSubpatternOnceEnd: {
                PatternTerm* term = op.m_term;

                if (term->quantityType != QuantifierFixedCount) {
                    m_backtrackingState.link(this);

                    // Check whether we should backtrack back into the parentheses, or if we
                    // are currently in a state where we had skipped over the subpattern
                    // (in which case the flag value on the stack will be -1).
                    unsigned parenthesesFrameLocation = term->frameLocation;
                    Jump hadSkipped = branch32(Equal, Address(stackPointerRegister, parenthesesFrameLocation * sizeof(void*)), TrustedImm32(-1));

                    if (term->quantityType == QuantifierGreedy) {
                        // For Greedy parentheses, we skip after having already tried going
                        // through the subpattern, so if we get here we're done.
                        YarrOp& beginOp = m_ops[op.m_previousOp];
                        beginOp.m_jumps.append(hadSkipped);
                    } else {
                        // For NonGreedy parentheses, we try skipping the subpattern first,
                        // so if we get here we need to try running through the subpattern
                        // next. Jump back to the start of the parentheses in the forwards
                        // matching path.
                        ASSERT(term->quantityType == QuantifierNonGreedy);
                        YarrOp& beginOp = m_ops[op.m_previousOp];
                        hadSkipped.linkTo(beginOp.m_reentry, this);
                    }

                    m_backtrackingState.fallthrough();
                }

                m_backtrackingState.append(op.m_jumps);
                break;
            }

            // OpParenthesesSubpatternTerminalBegin/End
            //
            // Terminal subpatterns will always match - there is nothing after them to
            // force a backtrack, and they have a minimum count of 0, and as such will
            // always produce an acceptable result.
            case OpParenthesesSubpatternTerminalBegin: {
                // We will backtrack to this point once the subpattern cannot match any
                // more. Since no match is accepted as a successful match (we are Greedy
                // quantified with a minimum of zero) jump back to the forwards matching
                // path at the end.
                YarrOp& endOp = m_ops[op.m_nextOp];
                m_backtrackingState.linkTo(endOp.m_reentry, this);
                break;
            }
            case OpParenthesesSubpatternTerminalEnd:
                // We should never be backtracking to here (hence the 'terminal' in the name).
                ASSERT(m_backtrackingState.isEmpty());
                m_backtrackingState.append(op.m_jumps);
                break;

            // OpParentheticalAssertionBegin/End
            case OpParentheticalAssertionBegin: {
                PatternTerm* term = op.m_term;
                YarrOp& endOp = m_ops[op.m_nextOp];

                // We need to handle the backtracks upon backtracking back out
                // of a parenthetical assertion if either we need to correct
                // the input index, or the assertion was inverted.
                if (op.m_checkAdjust || term->invert()) {
                     m_backtrackingState.link(this);

                    if (op.m_checkAdjust)
                        add32(Imm32(op.m_checkAdjust), index);

                    // In an inverted assertion failure to match the subpattern
                    // is treated as a successful match - jump to the end of the
                    // subpattern. We already have adjusted the input position
                    // back to that before the assertion, which is correct.
                    if (term->invert())
                        jump(endOp.m_reentry);

                    m_backtrackingState.fallthrough();
                }

                // The End node's jump list will contain any backtracks into
                // the end of the assertion. Also, if inverted, we will have
                // added the failure caused by a successful match to this.
                m_backtrackingState.append(endOp.m_jumps);

                m_checked += op.m_checkAdjust;
                break;
            }
            case OpParentheticalAssertionEnd: {
                // FIXME: We should really be clearing any nested subpattern
                // matches on bailing out from after the pattern. Firefox has
                // this bug too (presumably because they use YARR!)

                // Never backtrack into an assertion; later failures bail to before the begin.
                m_backtrackingState.takeBacktracksToJumpList(op.m_jumps, this);

                YarrOp& lastOp = m_ops[op.m_previousOp];
                m_checked -= lastOp.m_checkAdjust;
                break;
            }

            case OpMatchFailed:
                break;
            }

        } while (opIndex);
    }

    // Compilation methods:
    // ====================

    // opCompileParenthesesSubpattern
    // Emits ops for a subpattern (set of parentheses). These consist
    // of a set of alternatives wrapped in an outer set of nodes for
    // the parentheses.
    // Supported types of parentheses are 'Once' (quantityCount == 1)
    // and 'Terminal' (non-capturing parentheses quantified as greedy
    // and infinite).
    // Alternatives will use the 'Simple' set of ops if either the
    // subpattern is terminal (in which case we will never need to
    // backtrack), or if the subpattern only contains one alternative.
    void opCompileParenthesesSubpattern(PatternTerm* term)
    {
        YarrOpCode parenthesesBeginOpCode;
        YarrOpCode parenthesesEndOpCode;
        YarrOpCode alternativeBeginOpCode = OpSimpleNestedAlternativeBegin;
        YarrOpCode alternativeNextOpCode = OpSimpleNestedAlternativeNext;
        YarrOpCode alternativeEndOpCode = OpSimpleNestedAlternativeEnd;

        // We can currently only compile quantity 1 subpatterns that are
        // not copies. We generate a copy in the case of a range quantifier,
        // e.g. /(?:x){3,9}/, or /(?:x)+/ (These are effectively expanded to
        // /(?:x){3,3}(?:x){0,6}/ and /(?:x)(?:x)*/ repectively). The problem
        // comes where the subpattern is capturing, in which case we would
        // need to restore the capture from the first subpattern upon a
        // failure in the second.
        if (term->quantityCount == 1 && !term->parentheses.isCopy) {
            // Select the 'Once' nodes.
            parenthesesBeginOpCode = OpParenthesesSubpatternOnceBegin;
            parenthesesEndOpCode = OpParenthesesSubpatternOnceEnd;

            // If there is more than one alternative we cannot use the 'simple' nodes.
            if (term->parentheses.disjunction->m_alternatives.size() != 1) {
                alternativeBeginOpCode = OpNestedAlternativeBegin;
                alternativeNextOpCode = OpNestedAlternativeNext;
                alternativeEndOpCode = OpNestedAlternativeEnd;
            }
        } else if (term->parentheses.isTerminal) {
            // Select the 'Terminal' nodes.
            parenthesesBeginOpCode = OpParenthesesSubpatternTerminalBegin;
            parenthesesEndOpCode = OpParenthesesSubpatternTerminalEnd;
        } else {
            // This subpattern is not supported by the JIT.
            m_shouldFallBack = true;
            return;
        }

        size_t parenBegin = m_ops.size();
        m_ops.append(parenthesesBeginOpCode);

        m_ops.append(alternativeBeginOpCode);
        m_ops.last().m_previousOp = notFound;
        m_ops.last().m_term = term;
        Vector<PatternAlternative*>& alternatives =  term->parentheses.disjunction->m_alternatives;
        for (unsigned i = 0; i < alternatives.size(); ++i) {
            size_t lastOpIndex = m_ops.size() - 1;

            PatternAlternative* nestedAlternative = alternatives[i];
            opCompileAlternative(nestedAlternative);

            size_t thisOpIndex = m_ops.size();
            m_ops.append(YarrOp(alternativeNextOpCode));

            YarrOp& lastOp = m_ops[lastOpIndex];
            YarrOp& thisOp = m_ops[thisOpIndex];

            lastOp.m_alternative = nestedAlternative;
            lastOp.m_nextOp = thisOpIndex;
            thisOp.m_previousOp = lastOpIndex;
            thisOp.m_term = term;
        }
        YarrOp& lastOp = m_ops.last();
        ASSERT(lastOp.m_op == alternativeNextOpCode);
        lastOp.m_op = alternativeEndOpCode;
        lastOp.m_alternative = 0;
        lastOp.m_nextOp = notFound;

        size_t parenEnd = m_ops.size();
        m_ops.append(parenthesesEndOpCode);

        m_ops[parenBegin].m_term = term;
        m_ops[parenBegin].m_previousOp = notFound;
        m_ops[parenBegin].m_nextOp = parenEnd;
        m_ops[parenEnd].m_term = term;
        m_ops[parenEnd].m_previousOp = parenBegin;
        m_ops[parenEnd].m_nextOp = notFound;
    }

    // opCompileParentheticalAssertion
    // Emits ops for a parenthetical assertion. These consist of an
    // OpSimpleNestedAlternativeBegin/Next/End set of nodes wrapping
    // the alternatives, with these wrapped by an outer pair of
    // OpParentheticalAssertionBegin/End nodes.
    // We can always use the OpSimpleNestedAlternative nodes in the
    // case of parenthetical assertions since these only ever match
    // once, and will never backtrack back into the assertion.
    void opCompileParentheticalAssertion(PatternTerm* term)
    {
        size_t parenBegin = m_ops.size();
        m_ops.append(OpParentheticalAssertionBegin);

        m_ops.append(OpSimpleNestedAlternativeBegin);
        m_ops.last().m_previousOp = notFound;
        m_ops.last().m_term = term;
        Vector<PatternAlternative*>& alternatives =  term->parentheses.disjunction->m_alternatives;
        for (unsigned i = 0; i < alternatives.size(); ++i) {
            size_t lastOpIndex = m_ops.size() - 1;

            PatternAlternative* nestedAlternative = alternatives[i];
            opCompileAlternative(nestedAlternative);

            size_t thisOpIndex = m_ops.size();
            m_ops.append(YarrOp(OpSimpleNestedAlternativeNext));

            YarrOp& lastOp = m_ops[lastOpIndex];
            YarrOp& thisOp = m_ops[thisOpIndex];

            lastOp.m_alternative = nestedAlternative;
            lastOp.m_nextOp = thisOpIndex;
            thisOp.m_previousOp = lastOpIndex;
            thisOp.m_term = term;
        }
        YarrOp& lastOp = m_ops.last();
        ASSERT(lastOp.m_op == OpSimpleNestedAlternativeNext);
        lastOp.m_op = OpSimpleNestedAlternativeEnd;
        lastOp.m_alternative = 0;
        lastOp.m_nextOp = notFound;

        size_t parenEnd = m_ops.size();
        m_ops.append(OpParentheticalAssertionEnd);

        m_ops[parenBegin].m_term = term;
        m_ops[parenBegin].m_previousOp = notFound;
        m_ops[parenBegin].m_nextOp = parenEnd;
        m_ops[parenEnd].m_term = term;
        m_ops[parenEnd].m_previousOp = parenBegin;
        m_ops[parenEnd].m_nextOp = notFound;
    }

    // opCompileAlternative
    // Called to emit nodes for all terms in an alternative.
    void opCompileAlternative(PatternAlternative* alternative)
    {
        optimizeAlternative(alternative);

        for (unsigned i = 0; i < alternative->m_terms.size(); ++i) {
            PatternTerm* term = &alternative->m_terms[i];

            switch (term->type) {
            case PatternTerm::TypeParenthesesSubpattern:
                opCompileParenthesesSubpattern(term);
                break;

            case PatternTerm::TypeParentheticalAssertion:
                opCompileParentheticalAssertion(term);
                break;

            default:
                m_ops.append(term);
            }
        }
    }

    // opCompileBody
    // This method compiles the body disjunction of the regular expression.
    // The body consists of two sets of alternatives - zero or more 'once
    // through' (BOL anchored) alternatives, followed by zero or more
    // repeated alternatives.
    // For each of these two sets of alteratives, if not empty they will be
    // wrapped in a set of OpBodyAlternativeBegin/Next/End nodes (with the
    // 'begin' node referencing the first alternative, and 'next' nodes
    // referencing any further alternatives. The begin/next/end nodes are
    // linked together in a doubly linked list. In the case of repeating
    // alternatives, the end node is also linked back to the beginning.
    // If no repeating alternatives exist, then a OpMatchFailed node exists
    // to return the failing result.
    void opCompileBody(PatternDisjunction* disjunction)
    {
        Vector<PatternAlternative*>& alternatives =  disjunction->m_alternatives;
        size_t currentAlternativeIndex = 0;

        // Emit the 'once through' alternatives.
        if (alternatives.size() && alternatives[0]->onceThrough()) {
            m_ops.append(YarrOp(OpBodyAlternativeBegin));
            m_ops.last().m_previousOp = notFound;

            do {
                size_t lastOpIndex = m_ops.size() - 1;
                PatternAlternative* alternative = alternatives[currentAlternativeIndex];
                opCompileAlternative(alternative);

                size_t thisOpIndex = m_ops.size();
                m_ops.append(YarrOp(OpBodyAlternativeNext));

                YarrOp& lastOp = m_ops[lastOpIndex];
                YarrOp& thisOp = m_ops[thisOpIndex];

                lastOp.m_alternative = alternative;
                lastOp.m_nextOp = thisOpIndex;
                thisOp.m_previousOp = lastOpIndex;
                
                ++currentAlternativeIndex;
            } while (currentAlternativeIndex < alternatives.size() && alternatives[currentAlternativeIndex]->onceThrough());

            YarrOp& lastOp = m_ops.last();

            ASSERT(lastOp.m_op == OpBodyAlternativeNext);
            lastOp.m_op = OpBodyAlternativeEnd;
            lastOp.m_alternative = 0;
            lastOp.m_nextOp = notFound;
        }

        if (currentAlternativeIndex == alternatives.size()) {
            m_ops.append(YarrOp(OpMatchFailed));
            return;
        }

        // Emit the repeated alternatives.
        size_t repeatLoop = m_ops.size();
        m_ops.append(YarrOp(OpBodyAlternativeBegin));
        m_ops.last().m_previousOp = notFound;
        do {
            size_t lastOpIndex = m_ops.size() - 1;
            PatternAlternative* alternative = alternatives[currentAlternativeIndex];
            ASSERT(!alternative->onceThrough());
            opCompileAlternative(alternative);

            size_t thisOpIndex = m_ops.size();
            m_ops.append(YarrOp(OpBodyAlternativeNext));

            YarrOp& lastOp = m_ops[lastOpIndex];
            YarrOp& thisOp = m_ops[thisOpIndex];

            lastOp.m_alternative = alternative;
            lastOp.m_nextOp = thisOpIndex;
            thisOp.m_previousOp = lastOpIndex;
            
            ++currentAlternativeIndex;
        } while (currentAlternativeIndex < alternatives.size());
        YarrOp& lastOp = m_ops.last();
        ASSERT(lastOp.m_op == OpBodyAlternativeNext);
        lastOp.m_op = OpBodyAlternativeEnd;
        lastOp.m_alternative = 0;
        lastOp.m_nextOp = repeatLoop;
    }

    void generateEnter()
    {
#if CPU(X86_64)
        push(X86Registers::ebp);
        move(stackPointerRegister, X86Registers::ebp);
        push(X86Registers::ebx);
#elif CPU(X86)
        push(X86Registers::ebp);
        move(stackPointerRegister, X86Registers::ebp);
        // TODO: do we need spill registers to fill the output pointer if there are no sub captures?
        push(X86Registers::ebx);
        push(X86Registers::edi);
        push(X86Registers::esi);
        // load output into edi (2 = saved ebp + return address).
    #if COMPILER(MSVC)
        loadPtr(Address(X86Registers::ebp, 2 * sizeof(void*)), input);
        loadPtr(Address(X86Registers::ebp, 3 * sizeof(void*)), index);
        loadPtr(Address(X86Registers::ebp, 4 * sizeof(void*)), length);
        loadPtr(Address(X86Registers::ebp, 5 * sizeof(void*)), output);
    #else
        loadPtr(Address(X86Registers::ebp, 2 * sizeof(void*)), output);
    #endif
#elif CPU(ARM)
        push(ARMRegisters::r4);
        push(ARMRegisters::r5);
        push(ARMRegisters::r6);
#if CPU(ARM_TRADITIONAL)
        push(ARMRegisters::r8); // scratch register
#endif
        move(ARMRegisters::r3, output);
#elif CPU(SH4)
        push(SH4Registers::r11);
        push(SH4Registers::r13);
#elif CPU(MIPS)
        // Do nothing.
#endif
    }

    void generateReturn()
    {
#if CPU(X86_64)
        pop(X86Registers::ebx);
        pop(X86Registers::ebp);
#elif CPU(X86)
        pop(X86Registers::esi);
        pop(X86Registers::edi);
        pop(X86Registers::ebx);
        pop(X86Registers::ebp);
#elif CPU(ARM)
#if CPU(ARM_TRADITIONAL)
        pop(ARMRegisters::r8); // scratch register
#endif
        pop(ARMRegisters::r6);
        pop(ARMRegisters::r5);
        pop(ARMRegisters::r4);
#elif CPU(SH4)
        pop(SH4Registers::r13);
        pop(SH4Registers::r11);
#elif CPU(MIPS)
        // Do nothing
#endif
        ret();
    }

public:
    YarrGenerator(YarrPattern& pattern)
        : m_pattern(pattern)
        , m_shouldFallBack(false)
        , m_checked(0)
    {
    }

    void compile(JSGlobalData* globalData, YarrCodeBlock& jitObject)
    {
        generateEnter();

        if (!m_pattern.m_body->m_hasFixedSize)
            store32(index, Address(output));

        if (m_pattern.m_body->m_callFrameSize)
            subPtr(Imm32(m_pattern.m_body->m_callFrameSize * sizeof(void*)), stackPointerRegister);

        // Compile the pattern to the internal 'YarrOp' representation.
        opCompileBody(m_pattern.m_body);

        // If we encountered anything we can't handle in the JIT code
        // (e.g. backreferences) then return early.
        if (m_shouldFallBack) {
            jitObject.setFallBack(true);
            return;
        }

        generate();
        backtrack();

        // Link & finalize the code.
        LinkBuffer linkBuffer(this, globalData->regexAllocator);
        m_backtrackingState.linkDataLabels(linkBuffer);
        jitObject.set(linkBuffer.finalizeCode());
        jitObject.setFallBack(m_shouldFallBack);
    }

private:
    YarrPattern& m_pattern;

    // Used to detect regular expression constructs that are not currently
    // supported in the JIT; fall back to the interpreter when this is detected.
    bool m_shouldFallBack;

    // The regular expression expressed as a linear sequence of operations.
    Vector<YarrOp, 128> m_ops;

    // This records the current input offset being applied due to the current
    // set of alternatives we are nested within. E.g. when matching the
    // character 'b' within the regular expression /abc/, we will know that
    // the minimum size for the alternative is 3, checked upon entry to the
    // alternative, and that 'b' is at offset 1 from the start, and as such
    // when matching 'b' we need to apply an offset of -2 to the load.
    //
    // FIXME: This should go away. Rather than tracking this value throughout
    // code generation, we should gather this information up front & store it
    // on the YarrOp structure.
    int m_checked;

    // This class records state whilst generating the backtracking path of code.
    BacktrackingState m_backtrackingState;
};

void jitCompile(YarrPattern& pattern, JSGlobalData* globalData, YarrCodeBlock& jitObject)
{
    YarrGenerator(pattern).compile(globalData, jitObject);
}

int execute(YarrCodeBlock& jitObject, const UChar* input, unsigned start, unsigned length, int* output)
{
    return jitObject.execute(input, start, length, output);
}

}}

#endif
