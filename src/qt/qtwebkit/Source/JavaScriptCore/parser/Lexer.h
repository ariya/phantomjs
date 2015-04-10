/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2011, 2012, 2013 Apple Inc. All rights reserved.
 *  Copyright (C) 2010 Zoltan Herczeg (zherczeg@inf.u-szeged.hu)
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#ifndef Lexer_h
#define Lexer_h

#include "Lookup.h"
#include "ParserArena.h"
#include "ParserTokens.h"
#include "SourceCode.h"
#include <wtf/ASCIICType.h>
#include <wtf/SegmentedVector.h>
#include <wtf/Vector.h>
#include <wtf/unicode/Unicode.h>

namespace JSC {

class Keywords {
public:
    bool isKeyword(const Identifier& ident) const
    {
        return m_keywordTable.entry(m_vm, ident);
    }
    
    const HashEntry* getKeyword(const Identifier& ident) const
    {
        return m_keywordTable.entry(m_vm, ident);
    }
    
    ~Keywords()
    {
        m_keywordTable.deleteTable();
    }
    
private:
    friend class VM;
    
    Keywords(VM*);
    
    VM* m_vm;
    const HashTable m_keywordTable;
};

enum LexerFlags {
    LexerFlagsIgnoreReservedWords = 1, 
    LexerFlagsDontBuildStrings = 2,
    LexexFlagsDontBuildKeywords = 4
};

template <typename T>
class Lexer {
    WTF_MAKE_NONCOPYABLE(Lexer);
    WTF_MAKE_FAST_ALLOCATED;

public:
    Lexer(VM*);
    ~Lexer();

    // Character manipulation functions.
    static bool isWhiteSpace(T character);
    static bool isLineTerminator(T character);
    static unsigned char convertHex(int c1, int c2);
    static UChar convertUnicode(int c1, int c2, int c3, int c4);

    // Functions to set up parsing.
    void setCode(const SourceCode&, ParserArena*);
    void setIsReparsing() { m_isReparsing = true; }
    bool isReparsing() const { return m_isReparsing; }

    JSTokenType lex(JSToken*, unsigned, bool strictMode);
    bool nextTokenIsColon();
    int lineNumber() const { return m_lineNumber; }
    ALWAYS_INLINE int currentOffset() const { return offsetFromSourcePtr(m_code); }
    ALWAYS_INLINE int currentLineStartOffset() const { return offsetFromSourcePtr(m_lineStart); }
    ALWAYS_INLINE JSTextPosition currentPosition() const
    {
        return JSTextPosition(m_lineNumber, currentOffset(), currentLineStartOffset());
    }
    void setLastLineNumber(int lastLineNumber) { m_lastLineNumber = lastLineNumber; }
    int lastLineNumber() const { return m_lastLineNumber; }
    bool prevTerminator() const { return m_terminator; }
    SourceCode sourceCode(int openBrace, int closeBrace, int firstLine, unsigned startColumn);
    bool scanRegExp(const Identifier*& pattern, const Identifier*& flags, UChar patternPrefix = 0);
    bool skipRegExp();

    // Functions for use after parsing.
    bool sawError() const { return m_error; }
    String getErrorMessage() const { return m_lexErrorMessage; }
    void clear();
    void setOffset(int offset, int lineStartOffset)
    {
        m_error = 0;
        m_lexErrorMessage = String();

        m_code = sourcePtrFromOffset(offset);
        m_lineStart = sourcePtrFromOffset(lineStartOffset);
        ASSERT(currentOffset() >= currentLineStartOffset());

        m_buffer8.resize(0);
        m_buffer16.resize(0);
        if (LIKELY(m_code < m_codeEnd))
            m_current = *m_code;
        else
            m_current = 0;
    }
    void setLineNumber(int line)
    {
        m_lineNumber = line;
    }

    SourceProvider* sourceProvider() const { return m_source->provider(); }

    JSTokenType lexExpectIdentifier(JSToken*, unsigned, bool strictMode);

private:
    void record8(int);
    void append8(const T*, size_t);
    void record16(int);
    void record16(T);
    void append16(const LChar*, size_t);
    void append16(const UChar* characters, size_t length) { m_buffer16.append(characters, length); }

    ALWAYS_INLINE void shift();
    ALWAYS_INLINE bool atEnd() const;
    ALWAYS_INLINE T peek(int offset) const;
    struct UnicodeHexValue {
        
        enum ValueType { ValidHex, IncompleteHex, InvalidHex };
        
        explicit UnicodeHexValue(int value)
            : m_value(value)
        {
        }
        explicit UnicodeHexValue(ValueType type)
            : m_value(type == IncompleteHex ? -2 : -1)
        {
        }

        ValueType valueType() const
        {
            if (m_value >= 0)
                return ValidHex;
            return m_value == -2 ? IncompleteHex : InvalidHex;
        }
        bool isValid() const { return m_value >= 0; }
        int value() const
        {
            ASSERT(m_value >= 0);
            return m_value;
        }
        
    private:
        int m_value;
    };
    UnicodeHexValue parseFourDigitUnicodeHex();
    void shiftLineTerminator();

    ALWAYS_INLINE int offsetFromSourcePtr(const T* ptr) const { return ptr - m_codeStart; }
    ALWAYS_INLINE const T* sourcePtrFromOffset(int offset) const { return m_codeStart + offset; }

    String invalidCharacterMessage() const;
    ALWAYS_INLINE const T* currentSourcePtr() const;
    ALWAYS_INLINE void setOffsetFromSourcePtr(const T* sourcePtr, unsigned lineStartOffset) { setOffset(offsetFromSourcePtr(sourcePtr), lineStartOffset); }

    ALWAYS_INLINE void setCodeStart(const StringImpl*);

    ALWAYS_INLINE const Identifier* makeIdentifier(const LChar* characters, size_t length);
    ALWAYS_INLINE const Identifier* makeIdentifier(const UChar* characters, size_t length);
    ALWAYS_INLINE const Identifier* makeLCharIdentifier(const LChar* characters, size_t length);
    ALWAYS_INLINE const Identifier* makeLCharIdentifier(const UChar* characters, size_t length);
    ALWAYS_INLINE const Identifier* makeRightSizedIdentifier(const UChar* characters, size_t length, UChar orAllChars);
    ALWAYS_INLINE const Identifier* makeIdentifierLCharFromUChar(const UChar* characters, size_t length);

    ALWAYS_INLINE bool lastTokenWasRestrKeyword() const;

    template <int shiftAmount> void internalShift();
    template <bool shouldCreateIdentifier> ALWAYS_INLINE JSTokenType parseKeyword(JSTokenData*);
    template <bool shouldBuildIdentifiers> ALWAYS_INLINE JSTokenType parseIdentifier(JSTokenData*, unsigned lexerFlags, bool strictMode);
    template <bool shouldBuildIdentifiers> NEVER_INLINE JSTokenType parseIdentifierSlowCase(JSTokenData*, unsigned lexerFlags, bool strictMode);
    enum StringParseResult {
        StringParsedSuccessfully,
        StringUnterminated,
        StringCannotBeParsed
    };
    template <bool shouldBuildStrings> ALWAYS_INLINE StringParseResult parseString(JSTokenData*, bool strictMode);
    template <bool shouldBuildStrings> NEVER_INLINE StringParseResult parseStringSlowCase(JSTokenData*, bool strictMode);
    ALWAYS_INLINE void parseHex(double& returnValue);
    ALWAYS_INLINE bool parseOctal(double& returnValue);
    ALWAYS_INLINE bool parseDecimal(double& returnValue);
    ALWAYS_INLINE void parseNumberAfterDecimalPoint();
    ALWAYS_INLINE bool parseNumberAfterExponentIndicator();
    ALWAYS_INLINE bool parseMultilineComment();

    static const size_t initialReadBufferCapacity = 32;

    int m_lineNumber;
    int m_lastLineNumber;

    Vector<LChar> m_buffer8;
    Vector<UChar> m_buffer16;
    bool m_terminator;
    int m_lastToken;

    const SourceCode* m_source;
    unsigned m_sourceOffset;
    const T* m_code;
    const T* m_codeStart;
    const T* m_codeEnd;
    const T* m_codeStartPlusOffset;
    const T* m_lineStart;
    bool m_isReparsing;
    bool m_atLineStart;
    bool m_error;
    String m_lexErrorMessage;

    T m_current;

    IdentifierArena* m_arena;

    VM* m_vm;
};

template <>
ALWAYS_INLINE bool Lexer<LChar>::isWhiteSpace(LChar ch)
{
    return ch == ' ' || ch == '\t' || ch == 0xB || ch == 0xC || ch == 0xA0;
}

template <>
ALWAYS_INLINE bool Lexer<UChar>::isWhiteSpace(UChar ch)
{
    return (ch < 256) ? Lexer<LChar>::isWhiteSpace(static_cast<LChar>(ch)) : (WTF::Unicode::isSeparatorSpace(ch) || ch == 0xFEFF);
}

template <>
ALWAYS_INLINE bool Lexer<LChar>::isLineTerminator(LChar ch)
{
    return ch == '\r' || ch == '\n';
}

template <>
ALWAYS_INLINE bool Lexer<UChar>::isLineTerminator(UChar ch)
{
    return ch == '\r' || ch == '\n' || (ch & ~1) == 0x2028;
}

template <typename T>
inline unsigned char Lexer<T>::convertHex(int c1, int c2)
{
    return (toASCIIHexValue(c1) << 4) | toASCIIHexValue(c2);
}

template <typename T>
inline UChar Lexer<T>::convertUnicode(int c1, int c2, int c3, int c4)
{
    return (convertHex(c1, c2) << 8) | convertHex(c3, c4);
}

template <typename T>
ALWAYS_INLINE const Identifier* Lexer<T>::makeIdentifier(const LChar* characters, size_t length)
{
    return &m_arena->makeIdentifier(m_vm, characters, length);
}

template <typename T>
ALWAYS_INLINE const Identifier* Lexer<T>::makeIdentifier(const UChar* characters, size_t length)
{
    return &m_arena->makeIdentifier(m_vm, characters, length);
}

template <>
ALWAYS_INLINE const Identifier* Lexer<LChar>::makeRightSizedIdentifier(const UChar* characters, size_t length, UChar)
{
    return &m_arena->makeIdentifierLCharFromUChar(m_vm, characters, length);
}

template <>
ALWAYS_INLINE const Identifier* Lexer<UChar>::makeRightSizedIdentifier(const UChar* characters, size_t length, UChar orAllChars)
{
    if (!(orAllChars & ~0xff))
        return &m_arena->makeIdentifierLCharFromUChar(m_vm, characters, length);

    return &m_arena->makeIdentifier(m_vm, characters, length);
}

template <>
ALWAYS_INLINE void Lexer<LChar>::setCodeStart(const StringImpl* sourceString)
{
    ASSERT(sourceString->is8Bit());
    m_codeStart = sourceString->characters8();
}

template <>
ALWAYS_INLINE void Lexer<UChar>::setCodeStart(const StringImpl* sourceString)
{
    ASSERT(!sourceString->is8Bit());
    m_codeStart = sourceString->characters16();
}

template <typename T>
ALWAYS_INLINE const Identifier* Lexer<T>::makeIdentifierLCharFromUChar(const UChar* characters, size_t length)
{
    return &m_arena->makeIdentifierLCharFromUChar(m_vm, characters, length);
}

template <typename T>
ALWAYS_INLINE const Identifier* Lexer<T>::makeLCharIdentifier(const LChar* characters, size_t length)
{
    return &m_arena->makeIdentifier(m_vm, characters, length);
}

template <typename T>
ALWAYS_INLINE const Identifier* Lexer<T>::makeLCharIdentifier(const UChar* characters, size_t length)
{
    return &m_arena->makeIdentifierLCharFromUChar(m_vm, characters, length);
}

template <typename T>
ALWAYS_INLINE JSTokenType Lexer<T>::lexExpectIdentifier(JSToken* tokenRecord, unsigned lexerFlags, bool strictMode)
{
    JSTokenData* tokenData = &tokenRecord->m_data;
    JSTokenLocation* tokenLocation = &tokenRecord->m_location;
    ASSERT((lexerFlags & LexerFlagsIgnoreReservedWords));
    const T* start = m_code;
    const T* ptr = start;
    const T* end = m_codeEnd;
    JSTextPosition startPosition = currentPosition();
    if (ptr >= end) {
        ASSERT(ptr == end);
        goto slowCase;
    }
    if (!WTF::isASCIIAlpha(*ptr))
        goto slowCase;
    ++ptr;
    while (ptr < end) {
        if (!WTF::isASCIIAlphanumeric(*ptr))
            break;
        ++ptr;
    }

    // Here's the shift
    if (ptr < end) {
        if ((!WTF::isASCII(*ptr)) || (*ptr == '\\') || (*ptr == '_') || (*ptr == '$'))
            goto slowCase;
        m_current = *ptr;
    } else
        m_current = 0;

    m_code = ptr;
    ASSERT(currentOffset() >= currentLineStartOffset());

    // Create the identifier if needed
    if (lexerFlags & LexexFlagsDontBuildKeywords)
        tokenData->ident = 0;
    else
        tokenData->ident = makeLCharIdentifier(start, ptr - start);
    tokenLocation->line = m_lineNumber;
    tokenLocation->lineStartOffset = currentLineStartOffset();
    tokenLocation->startOffset = offsetFromSourcePtr(start);
    tokenLocation->endOffset = currentOffset();
    ASSERT(tokenLocation->startOffset >= tokenLocation->lineStartOffset);
    tokenRecord->m_startPosition = startPosition;
    tokenRecord->m_endPosition = currentPosition();
    m_lastToken = IDENT;
    return IDENT;
    
slowCase:
    return lex(tokenRecord, lexerFlags, strictMode);
}

} // namespace JSC

#endif // Lexer_h
