/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2006, 2007, 2008, 2009 Apple Inc. All Rights Reserved.
 *  Copyright (C) 2007 Cameron Zwarich (cwzwarich@uwaterloo.ca)
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

#include "config.h"
#include "Lexer.h"

#include "JSFunction.h"

#include "JSGlobalObjectFunctions.h"
#include "Identifier.h"
#include "NodeInfo.h"
#include "Nodes.h"
#include "dtoa.h"
#include <ctype.h>
#include <limits.h>
#include <string.h>
#include <wtf/Assertions.h>

using namespace WTF;
using namespace Unicode;

#include "JSParser.h"
#include "Lookup.h"
#include "Lexer.lut.h"

namespace JSC {


enum CharacterType {
    // Types for the main switch

    // The first three types are fixed, and also used for identifying
    // ASCII alpha and alphanumeric characters (see isIdentStart and isIdentPart).
    CharacterIdentifierStart,
    CharacterZero,
    CharacterNumber,

    CharacterInvalid,
    CharacterLineTerminator,
    CharacterExclamationMark,
    CharacterOpenParen,
    CharacterCloseParen,
    CharacterOpenBracket,
    CharacterCloseBracket,
    CharacterComma,
    CharacterColon,
    CharacterQuestion,
    CharacterTilde,
    CharacterQuote,
    CharacterDot,
    CharacterSlash,
    CharacterBackSlash,
    CharacterSemicolon,
    CharacterOpenBrace,
    CharacterCloseBrace,

    CharacterAdd,
    CharacterSub,
    CharacterMultiply,
    CharacterModulo,
    CharacterAnd,
    CharacterXor,
    CharacterOr,
    CharacterLess,
    CharacterGreater,
    CharacterEqual,

    // Other types (only one so far)
    CharacterWhiteSpace,
};

// 128 ASCII codes
static const unsigned short typesOfASCIICharacters[128] = {
/*   0 - Null               */ CharacterInvalid,
/*   1 - Start of Heading   */ CharacterInvalid,
/*   2 - Start of Text      */ CharacterInvalid,
/*   3 - End of Text        */ CharacterInvalid,
/*   4 - End of Transm.     */ CharacterInvalid,
/*   5 - Enquiry            */ CharacterInvalid,
/*   6 - Acknowledgment     */ CharacterInvalid,
/*   7 - Bell               */ CharacterInvalid,
/*   8 - Back Space         */ CharacterInvalid,
/*   9 - Horizontal Tab     */ CharacterWhiteSpace,
/*  10 - Line Feed          */ CharacterLineTerminator,
/*  11 - Vertical Tab       */ CharacterWhiteSpace,
/*  12 - Form Feed          */ CharacterWhiteSpace,
/*  13 - Carriage Return    */ CharacterLineTerminator,
/*  14 - Shift Out          */ CharacterInvalid,
/*  15 - Shift In           */ CharacterInvalid,
/*  16 - Data Line Escape   */ CharacterInvalid,
/*  17 - Device Control 1   */ CharacterInvalid,
/*  18 - Device Control 2   */ CharacterInvalid,
/*  19 - Device Control 3   */ CharacterInvalid,
/*  20 - Device Control 4   */ CharacterInvalid,
/*  21 - Negative Ack.      */ CharacterInvalid,
/*  22 - Synchronous Idle   */ CharacterInvalid,
/*  23 - End of Transmit    */ CharacterInvalid,
/*  24 - Cancel             */ CharacterInvalid,
/*  25 - End of Medium      */ CharacterInvalid,
/*  26 - Substitute         */ CharacterInvalid,
/*  27 - Escape             */ CharacterInvalid,
/*  28 - File Separator     */ CharacterInvalid,
/*  29 - Group Separator    */ CharacterInvalid,
/*  30 - Record Separator   */ CharacterInvalid,
/*  31 - Unit Separator     */ CharacterInvalid,
/*  32 - Space              */ CharacterWhiteSpace,
/*  33 - !                  */ CharacterExclamationMark,
/*  34 - "                  */ CharacterQuote,
/*  35 - #                  */ CharacterInvalid,
/*  36 - $                  */ CharacterIdentifierStart,
/*  37 - %                  */ CharacterModulo,
/*  38 - &                  */ CharacterAnd,
/*  39 - '                  */ CharacterQuote,
/*  40 - (                  */ CharacterOpenParen,
/*  41 - )                  */ CharacterCloseParen,
/*  42 - *                  */ CharacterMultiply,
/*  43 - +                  */ CharacterAdd,
/*  44 - ,                  */ CharacterComma,
/*  45 - -                  */ CharacterSub,
/*  46 - .                  */ CharacterDot,
/*  47 - /                  */ CharacterSlash,
/*  48 - 0                  */ CharacterZero,
/*  49 - 1                  */ CharacterNumber,
/*  50 - 2                  */ CharacterNumber,
/*  51 - 3                  */ CharacterNumber,
/*  52 - 4                  */ CharacterNumber,
/*  53 - 5                  */ CharacterNumber,
/*  54 - 6                  */ CharacterNumber,
/*  55 - 7                  */ CharacterNumber,
/*  56 - 8                  */ CharacterNumber,
/*  57 - 9                  */ CharacterNumber,
/*  58 - :                  */ CharacterColon,
/*  59 - ;                  */ CharacterSemicolon,
/*  60 - <                  */ CharacterLess,
/*  61 - =                  */ CharacterEqual,
/*  62 - >                  */ CharacterGreater,
/*  63 - ?                  */ CharacterQuestion,
/*  64 - @                  */ CharacterInvalid,
/*  65 - A                  */ CharacterIdentifierStart,
/*  66 - B                  */ CharacterIdentifierStart,
/*  67 - C                  */ CharacterIdentifierStart,
/*  68 - D                  */ CharacterIdentifierStart,
/*  69 - E                  */ CharacterIdentifierStart,
/*  70 - F                  */ CharacterIdentifierStart,
/*  71 - G                  */ CharacterIdentifierStart,
/*  72 - H                  */ CharacterIdentifierStart,
/*  73 - I                  */ CharacterIdentifierStart,
/*  74 - J                  */ CharacterIdentifierStart,
/*  75 - K                  */ CharacterIdentifierStart,
/*  76 - L                  */ CharacterIdentifierStart,
/*  77 - M                  */ CharacterIdentifierStart,
/*  78 - N                  */ CharacterIdentifierStart,
/*  79 - O                  */ CharacterIdentifierStart,
/*  80 - P                  */ CharacterIdentifierStart,
/*  81 - Q                  */ CharacterIdentifierStart,
/*  82 - R                  */ CharacterIdentifierStart,
/*  83 - S                  */ CharacterIdentifierStart,
/*  84 - T                  */ CharacterIdentifierStart,
/*  85 - U                  */ CharacterIdentifierStart,
/*  86 - V                  */ CharacterIdentifierStart,
/*  87 - W                  */ CharacterIdentifierStart,
/*  88 - X                  */ CharacterIdentifierStart,
/*  89 - Y                  */ CharacterIdentifierStart,
/*  90 - Z                  */ CharacterIdentifierStart,
/*  91 - [                  */ CharacterOpenBracket,
/*  92 - \                  */ CharacterBackSlash,
/*  93 - ]                  */ CharacterCloseBracket,
/*  94 - ^                  */ CharacterXor,
/*  95 - _                  */ CharacterIdentifierStart,
/*  96 - `                  */ CharacterInvalid,
/*  97 - a                  */ CharacterIdentifierStart,
/*  98 - b                  */ CharacterIdentifierStart,
/*  99 - c                  */ CharacterIdentifierStart,
/* 100 - d                  */ CharacterIdentifierStart,
/* 101 - e                  */ CharacterIdentifierStart,
/* 102 - f                  */ CharacterIdentifierStart,
/* 103 - g                  */ CharacterIdentifierStart,
/* 104 - h                  */ CharacterIdentifierStart,
/* 105 - i                  */ CharacterIdentifierStart,
/* 106 - j                  */ CharacterIdentifierStart,
/* 107 - k                  */ CharacterIdentifierStart,
/* 108 - l                  */ CharacterIdentifierStart,
/* 109 - m                  */ CharacterIdentifierStart,
/* 110 - n                  */ CharacterIdentifierStart,
/* 111 - o                  */ CharacterIdentifierStart,
/* 112 - p                  */ CharacterIdentifierStart,
/* 113 - q                  */ CharacterIdentifierStart,
/* 114 - r                  */ CharacterIdentifierStart,
/* 115 - s                  */ CharacterIdentifierStart,
/* 116 - t                  */ CharacterIdentifierStart,
/* 117 - u                  */ CharacterIdentifierStart,
/* 118 - v                  */ CharacterIdentifierStart,
/* 119 - w                  */ CharacterIdentifierStart,
/* 120 - x                  */ CharacterIdentifierStart,
/* 121 - y                  */ CharacterIdentifierStart,
/* 122 - z                  */ CharacterIdentifierStart,
/* 123 - {                  */ CharacterOpenBrace,
/* 124 - |                  */ CharacterOr,
/* 125 - }                  */ CharacterCloseBrace,
/* 126 - ~                  */ CharacterTilde,
/* 127 - Delete             */ CharacterInvalid,
};

Lexer::Lexer(JSGlobalData* globalData)
    : m_isReparsing(false)
    , m_globalData(globalData)
    , m_keywordTable(JSC::mainTable)
{
}

Lexer::~Lexer()
{
    m_keywordTable.deleteTable();
}

ALWAYS_INLINE const UChar* Lexer::currentCharacter() const
{
    ASSERT(m_code <= m_codeEnd);
    return m_code;
}

ALWAYS_INLINE int Lexer::currentOffset() const
{
    return currentCharacter() - m_codeStart;
}

void Lexer::setCode(const SourceCode& source, ParserArena& arena)
{
    m_arena = &arena.identifierArena();

    m_lineNumber = source.firstLine();
    m_delimited = false;
    m_lastToken = -1;

    const UChar* data = source.provider()->data();

    m_source = &source;
    m_codeStart = data;
    m_code = data + source.startOffset();
    m_codeEnd = data + source.endOffset();
    m_error = false;
    m_atLineStart = true;

    m_buffer8.reserveInitialCapacity(initialReadBufferCapacity);
    m_buffer16.reserveInitialCapacity((m_codeEnd - m_code) / 2);

    if (LIKELY(m_code < m_codeEnd))
        m_current = *m_code;
    else
        m_current = -1;
    ASSERT(currentOffset() == source.startOffset());
}

ALWAYS_INLINE void Lexer::shift()
{
    // Faster than an if-else sequence
    ASSERT(m_current != -1);
    m_current = -1;
    ++m_code;
    if (LIKELY(m_code < m_codeEnd))
        m_current = *m_code;
}

ALWAYS_INLINE int Lexer::peek(int offset)
{
    // Only use if necessary
    ASSERT(offset > 0 && offset < 5);
    const UChar* code = m_code + offset;
    return (code < m_codeEnd) ? *code : -1;
}

int Lexer::getUnicodeCharacter()
{
    int char1 = peek(1);
    int char2 = peek(2);
    int char3 = peek(3);

    if (UNLIKELY(!isASCIIHexDigit(m_current) || !isASCIIHexDigit(char1) || !isASCIIHexDigit(char2) || !isASCIIHexDigit(char3)))
        return -1;

    int result = convertUnicode(m_current, char1, char2, char3);
    shift();
    shift();
    shift();
    shift();
    return result;
}

void Lexer::shiftLineTerminator()
{
    ASSERT(isLineTerminator(m_current));

    int m_prev = m_current;
    shift();

    // Allow both CRLF and LFCR.
    if (m_prev + m_current == '\n' + '\r')
        shift();

    ++m_lineNumber;
}

ALWAYS_INLINE const Identifier* Lexer::makeIdentifier(const UChar* characters, size_t length)
{
    return &m_arena->makeIdentifier(m_globalData, characters, length);
}

ALWAYS_INLINE bool Lexer::lastTokenWasRestrKeyword() const
{
    return m_lastToken == CONTINUE || m_lastToken == BREAK || m_lastToken == RETURN || m_lastToken == THROW;
}

static NEVER_INLINE bool isNonASCIIIdentStart(int c)
{
    return category(c) & (Letter_Uppercase | Letter_Lowercase | Letter_Titlecase | Letter_Modifier | Letter_Other);
}

static inline bool isIdentStart(int c)
{
    return isASCII(c) ? typesOfASCIICharacters[c] == CharacterIdentifierStart : isNonASCIIIdentStart(c);
}

static NEVER_INLINE bool isNonASCIIIdentPart(int c)
{
    return category(c) & (Letter_Uppercase | Letter_Lowercase | Letter_Titlecase | Letter_Modifier | Letter_Other
        | Mark_NonSpacing | Mark_SpacingCombining | Number_DecimalDigit | Punctuation_Connector);
}

static inline bool isIdentPart(int c)
{
    // Character types are divided into two groups depending on whether they can be part of an
    // identifier or not. Those whose type value is less or equal than CharacterNumber can be
    // part of an identifier. (See the CharacterType definition for more details.)
    return isASCII(c) ? typesOfASCIICharacters[c] <= CharacterNumber : isNonASCIIIdentPart(c);
}

static inline int singleEscape(int c)
{
    switch (c) {
    case 'b':
        return 0x08;
    case 't':
        return 0x09;
    case 'n':
        return 0x0A;
    case 'v':
        return 0x0B;
    case 'f':
        return 0x0C;
    case 'r':
        return 0x0D;
    case '\\':
        return '\\';
    case '\'':
        return '\'';
    case '"':
        return '"';
    default:
        return 0;
    }
}

inline void Lexer::record8(int c)
{
    ASSERT(c >= 0);
    ASSERT(c <= 0xFF);
    m_buffer8.append(static_cast<char>(c));
}

inline void Lexer::record16(UChar c)
{
    m_buffer16.append(c);
}

inline void Lexer::record16(int c)
{
    ASSERT(c >= 0);
    ASSERT(c <= USHRT_MAX);
    record16(UChar(static_cast<unsigned short>(c)));
}

ALWAYS_INLINE JSTokenType Lexer::parseIdentifier(JSTokenData* lvalp, LexType lexType)
{
    bool bufferRequired = false;
    const UChar* identifierStart = currentCharacter();
    int identifierLength;

    while (true) {
        if (LIKELY(isIdentPart(m_current))) {
            shift();
            continue;
        }
        if (LIKELY(m_current != '\\'))
            break;

        // \uXXXX unicode characters.
        bufferRequired = true;
        if (identifierStart != currentCharacter())
            m_buffer16.append(identifierStart, currentCharacter() - identifierStart);
        shift();
        if (UNLIKELY(m_current != 'u'))
            return ERRORTOK;
        shift();
        int character = getUnicodeCharacter();
        if (UNLIKELY(character == -1))
            return ERRORTOK;
        if (UNLIKELY(m_buffer16.size() ? !isIdentPart(character) : !isIdentStart(character)))
            return ERRORTOK;
        record16(character);
        identifierStart = currentCharacter();
    }

    if (!bufferRequired)
        identifierLength = currentCharacter() - identifierStart;
    else {
        if (identifierStart != currentCharacter())
            m_buffer16.append(identifierStart, currentCharacter() - identifierStart);
        identifierStart = m_buffer16.data();
        identifierLength = m_buffer16.size();
    }

    const Identifier* ident = makeIdentifier(identifierStart, identifierLength);
    lvalp->ident = ident;
    m_delimited = false;

    if (LIKELY(!bufferRequired && lexType == IdentifyReservedWords)) {
        // Keywords must not be recognized if there was an \uXXXX in the identifier.
        const HashEntry* entry = m_keywordTable.entry(m_globalData, *ident);
        return entry ? static_cast<JSTokenType>(entry->lexerValue()) : IDENT;
    }

    m_buffer16.resize(0);
    return IDENT;
}

ALWAYS_INLINE bool Lexer::parseString(JSTokenData* lvalp, bool strictMode)
{
    int stringQuoteCharacter = m_current;
    shift();

    const UChar* stringStart = currentCharacter();

    while (m_current != stringQuoteCharacter) {
        if (UNLIKELY(m_current == '\\')) {
            if (stringStart != currentCharacter())
                m_buffer16.append(stringStart, currentCharacter() - stringStart);
            shift();

            int escape = singleEscape(m_current);

            // Most common escape sequences first
            if (escape) {
                record16(escape);
                shift();
            } else if (UNLIKELY(isLineTerminator(m_current)))
                shiftLineTerminator();
            else if (m_current == 'x') {
                shift();
                if (isASCIIHexDigit(m_current) && isASCIIHexDigit(peek(1))) {
                    int prev = m_current;
                    shift();
                    record16(convertHex(prev, m_current));
                    shift();
                } else
                    record16('x');
            } else if (m_current == 'u') {
                shift();
                int character = getUnicodeCharacter();
                if (character != -1)
                    record16(character);
                else if (m_current == stringQuoteCharacter)
                    record16('u');
                else // Only stringQuoteCharacter allowed after \u
                    return false;
            } else if (strictMode && isASCIIDigit(m_current)) {
                // The only valid numeric escape in strict mode is '\0', and this must not be followed by a decimal digit.
                int character1 = m_current;
                shift();
                if (character1 != '0' || isASCIIDigit(m_current))
                    return false;
                record16(0);
            } else if (!strictMode && isASCIIOctalDigit(m_current)) {
                // Octal character sequences
                int character1 = m_current;
                shift();
                if (isASCIIOctalDigit(m_current)) {
                    // Two octal characters
                    int character2 = m_current;
                    shift();
                    if (character1 >= '0' && character1 <= '3' && isASCIIOctalDigit(m_current)) {
                        record16((character1 - '0') * 64 + (character2 - '0') * 8 + m_current - '0');
                        shift();
                    } else
                        record16((character1 - '0') * 8 + character2 - '0');
                } else
                    record16(character1 - '0');
            } else if (m_current != -1) {
                record16(m_current);
                shift();
            } else
                return false;

            stringStart = currentCharacter();
            continue;
        }
        // Fast check for characters that require special handling.
        // Catches -1, \n, \r, 0x2028, and 0x2029 as efficiently
        // as possible, and lets through all common ASCII characters.
        if (UNLIKELY(((static_cast<unsigned>(m_current) - 0xE) & 0x2000))) {
            // New-line or end of input is not allowed
            if (UNLIKELY(isLineTerminator(m_current)) || UNLIKELY(m_current == -1))
                return false;
            // Anything else is just a normal character
        }
        shift();
    }

    if (currentCharacter() != stringStart)
        m_buffer16.append(stringStart, currentCharacter() - stringStart);
    lvalp->ident = makeIdentifier(m_buffer16.data(), m_buffer16.size());
    m_buffer16.resize(0);
    return true;
}

ALWAYS_INLINE void Lexer::parseHex(double& returnValue)
{
    // Optimization: most hexadecimal values fit into 4 bytes.
    uint32_t hexValue = 0;
    int maximumDigits = 7;

    // Shift out the 'x' prefix.
    shift();

    do {
        hexValue = (hexValue << 4) + toASCIIHexValue(m_current);
        shift();
        --maximumDigits;
    } while (isASCIIHexDigit(m_current) && maximumDigits >= 0);

    if (maximumDigits >= 0) {
        returnValue = hexValue;
        return;
    }

    // No more place in the hexValue buffer.
    // The values are shifted out and placed into the m_buffer8 vector.
    for (int i = 0; i < 8; ++i) {
         int digit = hexValue >> 28;
         if (digit < 10)
             record8(digit + '0');
         else
             record8(digit - 10 + 'a');
         hexValue <<= 4;
    }

    while (isASCIIHexDigit(m_current)) {
        record8(m_current);
        shift();
    }

    returnValue = parseIntOverflow(m_buffer8.data(), m_buffer8.size(), 16);
}

ALWAYS_INLINE bool Lexer::parseOctal(double& returnValue)
{
    // Optimization: most octal values fit into 4 bytes.
    uint32_t octalValue = 0;
    int maximumDigits = 9;
    // Temporary buffer for the digits. Makes easier
    // to reconstruct the input characters when needed.
    char digits[10];

    do {
        octalValue = octalValue * 8 + (m_current - '0');
        digits[maximumDigits] = m_current;
        shift();
        --maximumDigits;
    } while (isASCIIOctalDigit(m_current) && maximumDigits >= 0);

    if (!isASCIIDigit(m_current) && maximumDigits >= 0) {
        returnValue = octalValue;
        return true;
    }

    for (int i = 9; i > maximumDigits; --i)
         record8(digits[i]);

    while (isASCIIOctalDigit(m_current)) {
        record8(m_current);
        shift();
    }

    if (isASCIIDigit(m_current))
        return false;

    returnValue = parseIntOverflow(m_buffer8.data(), m_buffer8.size(), 8);
    return true;
}

ALWAYS_INLINE bool Lexer::parseDecimal(double& returnValue)
{
    // Optimization: most decimal values fit into 4 bytes.
    uint32_t decimalValue = 0;

    // Since parseOctal may be executed before parseDecimal,
    // the m_buffer8 may hold ascii digits.
    if (!m_buffer8.size()) {
        int maximumDigits = 9;
        // Temporary buffer for the digits. Makes easier
        // to reconstruct the input characters when needed.
        char digits[10];

        do {
            decimalValue = decimalValue * 10 + (m_current - '0');
            digits[maximumDigits] = m_current;
            shift();
            --maximumDigits;
        } while (isASCIIDigit(m_current) && maximumDigits >= 0);

        if (maximumDigits >= 0 && m_current != '.' && (m_current | 0x20) != 'e') {
            returnValue = decimalValue;
            return true;
        }

        for (int i = 9; i > maximumDigits; --i)
            record8(digits[i]);
    }

    while (isASCIIDigit(m_current)) {
        record8(m_current);
        shift();
    }

    return false;
}

ALWAYS_INLINE void Lexer::parseNumberAfterDecimalPoint()
{
    record8('.');
    while (isASCIIDigit(m_current)) {
        record8(m_current);
        shift();
    }
}

ALWAYS_INLINE bool Lexer::parseNumberAfterExponentIndicator()
{
    record8('e');
    shift();
    if (m_current == '+' || m_current == '-') {
        record8(m_current);
        shift();
    }

    if (!isASCIIDigit(m_current))
        return false;

    do {
        record8(m_current);
        shift();
    } while (isASCIIDigit(m_current));
    return true;
}

ALWAYS_INLINE bool Lexer::parseMultilineComment()
{
    while (true) {
        while (UNLIKELY(m_current == '*')) {
            shift();
            if (m_current == '/') {
                shift();
                return true;
            }
        }

        if (UNLIKELY(m_current == -1))
            return false;

        if (isLineTerminator(m_current))
            shiftLineTerminator();
        else
            shift();
    }
}

bool Lexer::nextTokenIsColon()
{
    const UChar* code = m_code;
    while (code < m_codeEnd && (isWhiteSpace(*code) || isLineTerminator(*code)))
        code++;
        
    return code < m_codeEnd && *code == ':';
}

JSTokenType Lexer::lex(JSTokenData* lvalp, JSTokenInfo* llocp, LexType lexType, bool strictMode)
{
    ASSERT(!m_error);
    ASSERT(m_buffer8.isEmpty());
    ASSERT(m_buffer16.isEmpty());

    JSTokenType token = ERRORTOK;
    m_terminator = false;

start:
    while (isWhiteSpace(m_current))
        shift();

    int startOffset = currentOffset();

    if (UNLIKELY(m_current == -1))
        return EOFTOK;

    m_delimited = false;

    CharacterType type;
    if (LIKELY(isASCII(m_current)))
        type = static_cast<CharacterType>(typesOfASCIICharacters[m_current]);
    else if (isNonASCIIIdentStart(m_current))
        type = CharacterIdentifierStart;
    else if (isLineTerminator(m_current))
        type = CharacterLineTerminator;
    else
        type = CharacterInvalid;

    switch (type) {
    case CharacterGreater:
        shift();
        if (m_current == '>') {
            shift();
            if (m_current == '>') {
                shift();
                if (m_current == '=') {
                    shift();
                    token = URSHIFTEQUAL;
                    break;
                }
                token = URSHIFT;
                break;
            }
            if (m_current == '=') {
                shift();
                token = RSHIFTEQUAL;
                break;
            }
            token = RSHIFT;
            break;
        }
        if (m_current == '=') {
            shift();
            token = GE;
            break;
        }
        token = GT;
        break;
    case CharacterEqual:
        shift();
        if (m_current == '=') {
            shift();
            if (m_current == '=') {
                shift();
                token = STREQ;
                break;
            }
            token = EQEQ;
            break;
        }
        token = EQUAL;
        break;
    case CharacterLess:
        shift();
        if (m_current == '!' && peek(1) == '-' && peek(2) == '-') {
            // <!-- marks the beginning of a line comment (for www usage)
            goto inSingleLineComment;
        }
        if (m_current == '<') {
            shift();
            if (m_current == '=') {
                shift();
                token = LSHIFTEQUAL;
                break;
            }
            token = LSHIFT;
            break;
        }
        if (m_current == '=') {
            shift();
            token = LE;
            break;
        }
        token = LT;
        break;
    case CharacterExclamationMark:
        shift();
        if (m_current == '=') {
            shift();
            if (m_current == '=') {
                shift();
                token = STRNEQ;
                break;
            }
            token = NE;
            break;
        }
        token = EXCLAMATION;
        break;
    case CharacterAdd:
        shift();
        if (m_current == '+') {
            shift();
            token = (!m_terminator) ? PLUSPLUS : AUTOPLUSPLUS;
            break;
        }
        if (m_current == '=') {
            shift();
            token = PLUSEQUAL;
            break;
        }
        token = PLUS;
        break;
    case CharacterSub:
        shift();
        if (m_current == '-') {
            shift();
            if (m_atLineStart && m_current == '>') {
                shift();
                goto inSingleLineComment;
            }
            token = (!m_terminator) ? MINUSMINUS : AUTOMINUSMINUS;
            break;
        }
        if (m_current == '=') {
            shift();
            token = MINUSEQUAL;
            break;
        }
        token = MINUS;
        break;
    case CharacterMultiply:
        shift();
        if (m_current == '=') {
            shift();
            token = MULTEQUAL;
            break;
        }
        token = TIMES;
        break;
    case CharacterSlash:
        shift();
        if (m_current == '/') {
            shift();
            goto inSingleLineComment;
        }
        if (m_current == '*') {
            shift();
            if (parseMultilineComment())
                goto start;
            goto returnError;
        }
        if (m_current == '=') {
            shift();
            token = DIVEQUAL;
            break;
        }
        token = DIVIDE;
        break;
    case CharacterAnd:
        shift();
        if (m_current == '&') {
            shift();
            token = AND;
            break;
        }
        if (m_current == '=') {
            shift();
            token = ANDEQUAL;
            break;
        }
        token = BITAND;
        break;
    case CharacterXor:
        shift();
        if (m_current == '=') {
            shift();
            token = XOREQUAL;
            break;
        }
        token = BITXOR;
        break;
    case CharacterModulo:
        shift();
        if (m_current == '=') {
            shift();
            token = MODEQUAL;
            break;
        }
        token = MOD;
        break;
    case CharacterOr:
        shift();
        if (m_current == '=') {
            shift();
            token = OREQUAL;
            break;
        }
        if (m_current == '|') {
            shift();
            token = OR;
            break;
        }
        token = BITOR;
        break;
    case CharacterOpenParen:
        token = OPENPAREN;
        shift();
        break;
    case CharacterCloseParen:
        token = CLOSEPAREN;
        shift();
        break;
    case CharacterOpenBracket:
        token = OPENBRACKET;
        shift();
        break;
    case CharacterCloseBracket:
        token = CLOSEBRACKET;
        shift();
        break;
    case CharacterComma:
        token = COMMA;
        shift();
        break;
    case CharacterColon:
        token = COLON;
        shift();
        break;
    case CharacterQuestion:
        token = QUESTION;
        shift();
        break;
    case CharacterTilde:
        token = TILDE;
        shift();
        break;
    case CharacterSemicolon:
        m_delimited = true;
        shift();
        token = SEMICOLON;
        break;
    case CharacterOpenBrace:
        lvalp->intValue = currentOffset();
        shift();
        token = OPENBRACE;
        break;
    case CharacterCloseBrace:
        lvalp->intValue = currentOffset();
        m_delimited = true;
        shift();
        token = CLOSEBRACE;
        break;
    case CharacterDot:
        shift();
        if (!isASCIIDigit(m_current)) {
            token = DOT;
            break;
        }
        goto inNumberAfterDecimalPoint;
    case CharacterZero:
        shift();
        if ((m_current | 0x20) == 'x' && isASCIIHexDigit(peek(1))) {
            parseHex(lvalp->doubleValue);
            token = NUMBER;
        } else {
            record8('0');
            if (isASCIIOctalDigit(m_current)) {
                if (parseOctal(lvalp->doubleValue)) {
                    if (strictMode)
                        goto returnError;
                    token = NUMBER;
                }
            }
        }
        // Fall through into CharacterNumber
    case CharacterNumber:
        if (LIKELY(token != NUMBER)) {
            if (!parseDecimal(lvalp->doubleValue)) {
                if (m_current == '.') {
                    shift();
inNumberAfterDecimalPoint:
                    parseNumberAfterDecimalPoint();
                }
                if ((m_current | 0x20) == 'e')
                    if (!parseNumberAfterExponentIndicator())
                        goto returnError;
                // Null-terminate string for strtod.
                m_buffer8.append('\0');
                lvalp->doubleValue = WTF::strtod(m_buffer8.data(), 0);
            }
            token = NUMBER;
        }

        // No identifiers allowed directly after numeric literal, e.g. "3in" is bad.
        if (UNLIKELY(isIdentStart(m_current)))
            goto returnError;
        m_buffer8.resize(0);
        m_delimited = false;
        break;
    case CharacterQuote:
        if (UNLIKELY(!parseString(lvalp, strictMode)))
            goto returnError;
        shift();
        m_delimited = false;
        token = STRING;
        break;
    case CharacterIdentifierStart:
        ASSERT(isIdentStart(m_current));
        // Fall through into CharacterBackSlash.
    case CharacterBackSlash:
        token = parseIdentifier(lvalp, lexType);
        break;
    case CharacterLineTerminator:
        ASSERT(isLineTerminator(m_current));
        shiftLineTerminator();
        m_atLineStart = true;
        m_terminator = true;
        goto start;
    case CharacterInvalid:
        goto returnError;
    default:
        ASSERT_NOT_REACHED();
        goto returnError;
    }

    m_atLineStart = false;
    goto returnToken;

inSingleLineComment:
    while (!isLineTerminator(m_current)) {
        if (UNLIKELY(m_current == -1))
            return EOFTOK;
        shift();
    }
    shiftLineTerminator();
    m_atLineStart = true;
    m_terminator = true;
    if (!lastTokenWasRestrKeyword())
        goto start;

    token = SEMICOLON;
    m_delimited = true;
    // Fall through into returnToken.

returnToken:
    llocp->line = m_lineNumber;
    llocp->startOffset = startOffset;
    llocp->endOffset = currentOffset();
    m_lastToken = token;
    return token;

returnError:
    m_error = true;
    return ERRORTOK;
}

bool Lexer::scanRegExp(const Identifier*& pattern, const Identifier*& flags, UChar patternPrefix)
{
    ASSERT(m_buffer16.isEmpty());

    bool lastWasEscape = false;
    bool inBrackets = false;

    if (patternPrefix) {
        ASSERT(!isLineTerminator(patternPrefix));
        ASSERT(patternPrefix != '/');
        ASSERT(patternPrefix != '[');
        record16(patternPrefix);
    }

    while (true) {
        int current = m_current;

        if (isLineTerminator(current) || current == -1) {
            m_buffer16.resize(0);
            return false;
        }

        shift();

        if (current == '/' && !lastWasEscape && !inBrackets)
            break;

        record16(current);

        if (lastWasEscape) {
            lastWasEscape = false;
            continue;
        }

        switch (current) {
        case '[':
            inBrackets = true;
            break;
        case ']':
            inBrackets = false;
            break;
        case '\\':
            lastWasEscape = true;
            break;
        }
    }

    pattern = makeIdentifier(m_buffer16.data(), m_buffer16.size());
    m_buffer16.resize(0);

    while (isIdentPart(m_current)) {
        record16(m_current);
        shift();
    }

    flags = makeIdentifier(m_buffer16.data(), m_buffer16.size());
    m_buffer16.resize(0);

    return true;
}

bool Lexer::skipRegExp()
{
    bool lastWasEscape = false;
    bool inBrackets = false;

    while (true) {
        int current = m_current;

        if (isLineTerminator(current) || current == -1)
            return false;

        shift();

        if (current == '/' && !lastWasEscape && !inBrackets)
            break;

        if (lastWasEscape) {
            lastWasEscape = false;
            continue;
        }

        switch (current) {
        case '[':
            inBrackets = true;
            break;
        case ']':
            inBrackets = false;
            break;
        case '\\':
            lastWasEscape = true;
            break;
        }
    }

    while (isIdentPart(m_current))
        shift();

    return true;
}

void Lexer::clear()
{
    m_arena = 0;

    Vector<char> newBuffer8;
    m_buffer8.swap(newBuffer8);

    Vector<UChar> newBuffer16;
    m_buffer16.swap(newBuffer16);

    m_isReparsing = false;
}

SourceCode Lexer::sourceCode(int openBrace, int closeBrace, int firstLine)
{
    ASSERT(m_source->provider()->data()[openBrace] == '{');
    ASSERT(m_source->provider()->data()[closeBrace] == '}');
    return SourceCode(m_source->provider(), openBrace, closeBrace + 1, firstLine);
}

} // namespace JSC
