/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2006, 2007, 2008, 2009, 2011, 2012, 2013 Apple Inc. All Rights Reserved.
 *  Copyright (C) 2007 Cameron Zwarich (cwzwarich@uwaterloo.ca)
 *  Copyright (C) 2010 Zoltan Herczeg (zherczeg@inf.u-szeged.hu)
 *  Copyright (C) 2012 Mathias Bynens (mathias@qiwi.be)
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
#include <wtf/dtoa.h>
#include <ctype.h>
#include <limits.h>
#include <string.h>
#include <wtf/Assertions.h>

using namespace WTF;
using namespace Unicode;

#include "KeywordLookup.h"
#include "Lexer.lut.h"
#include "Parser.h"

namespace JSC {

Keywords::Keywords(VM* vm)
    : m_vm(vm)
    , m_keywordTable(JSC::mainTable)
{
}

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

// 256 Latin-1 codes
static const unsigned short typesOfLatin1Characters[256] = {
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
/* 128 - Cc category        */ CharacterInvalid,
/* 129 - Cc category        */ CharacterInvalid,
/* 130 - Cc category        */ CharacterInvalid,
/* 131 - Cc category        */ CharacterInvalid,
/* 132 - Cc category        */ CharacterInvalid,
/* 133 - Cc category        */ CharacterInvalid,
/* 134 - Cc category        */ CharacterInvalid,
/* 135 - Cc category        */ CharacterInvalid,
/* 136 - Cc category        */ CharacterInvalid,
/* 137 - Cc category        */ CharacterInvalid,
/* 138 - Cc category        */ CharacterInvalid,
/* 139 - Cc category        */ CharacterInvalid,
/* 140 - Cc category        */ CharacterInvalid,
/* 141 - Cc category        */ CharacterInvalid,
/* 142 - Cc category        */ CharacterInvalid,
/* 143 - Cc category        */ CharacterInvalid,
/* 144 - Cc category        */ CharacterInvalid,
/* 145 - Cc category        */ CharacterInvalid,
/* 146 - Cc category        */ CharacterInvalid,
/* 147 - Cc category        */ CharacterInvalid,
/* 148 - Cc category        */ CharacterInvalid,
/* 149 - Cc category        */ CharacterInvalid,
/* 150 - Cc category        */ CharacterInvalid,
/* 151 - Cc category        */ CharacterInvalid,
/* 152 - Cc category        */ CharacterInvalid,
/* 153 - Cc category        */ CharacterInvalid,
/* 154 - Cc category        */ CharacterInvalid,
/* 155 - Cc category        */ CharacterInvalid,
/* 156 - Cc category        */ CharacterInvalid,
/* 157 - Cc category        */ CharacterInvalid,
/* 158 - Cc category        */ CharacterInvalid,
/* 159 - Cc category        */ CharacterInvalid,
/* 160 - Zs category (nbsp) */ CharacterWhiteSpace,
/* 161 - Po category        */ CharacterInvalid,
/* 162 - Sc category        */ CharacterInvalid,
/* 163 - Sc category        */ CharacterInvalid,
/* 164 - Sc category        */ CharacterInvalid,
/* 165 - Sc category        */ CharacterInvalid,
/* 166 - So category        */ CharacterInvalid,
/* 167 - So category        */ CharacterInvalid,
/* 168 - Sk category        */ CharacterInvalid,
/* 169 - So category        */ CharacterInvalid,
/* 170 - Ll category        */ CharacterIdentifierStart,
/* 171 - Pi category        */ CharacterInvalid,
/* 172 - Sm category        */ CharacterInvalid,
/* 173 - Cf category        */ CharacterInvalid,
/* 174 - So category        */ CharacterInvalid,
/* 175 - Sk category        */ CharacterInvalid,
/* 176 - So category        */ CharacterInvalid,
/* 177 - Sm category        */ CharacterInvalid,
/* 178 - No category        */ CharacterInvalid,
/* 179 - No category        */ CharacterInvalid,
/* 180 - Sk category        */ CharacterInvalid,
/* 181 - Ll category        */ CharacterIdentifierStart,
/* 182 - So category        */ CharacterInvalid,
/* 183 - Po category        */ CharacterInvalid,
/* 184 - Sk category        */ CharacterInvalid,
/* 185 - No category        */ CharacterInvalid,
/* 186 - Ll category        */ CharacterIdentifierStart,
/* 187 - Pf category        */ CharacterInvalid,
/* 188 - No category        */ CharacterInvalid,
/* 189 - No category        */ CharacterInvalid,
/* 190 - No category        */ CharacterInvalid,
/* 191 - Po category        */ CharacterInvalid,
/* 192 - Lu category        */ CharacterIdentifierStart,
/* 193 - Lu category        */ CharacterIdentifierStart,
/* 194 - Lu category        */ CharacterIdentifierStart,
/* 195 - Lu category        */ CharacterIdentifierStart,
/* 196 - Lu category        */ CharacterIdentifierStart,
/* 197 - Lu category        */ CharacterIdentifierStart,
/* 198 - Lu category        */ CharacterIdentifierStart,
/* 199 - Lu category        */ CharacterIdentifierStart,
/* 200 - Lu category        */ CharacterIdentifierStart,
/* 201 - Lu category        */ CharacterIdentifierStart,
/* 202 - Lu category        */ CharacterIdentifierStart,
/* 203 - Lu category        */ CharacterIdentifierStart,
/* 204 - Lu category        */ CharacterIdentifierStart,
/* 205 - Lu category        */ CharacterIdentifierStart,
/* 206 - Lu category        */ CharacterIdentifierStart,
/* 207 - Lu category        */ CharacterIdentifierStart,
/* 208 - Lu category        */ CharacterIdentifierStart,
/* 209 - Lu category        */ CharacterIdentifierStart,
/* 210 - Lu category        */ CharacterIdentifierStart,
/* 211 - Lu category        */ CharacterIdentifierStart,
/* 212 - Lu category        */ CharacterIdentifierStart,
/* 213 - Lu category        */ CharacterIdentifierStart,
/* 214 - Lu category        */ CharacterIdentifierStart,
/* 215 - Sm category        */ CharacterInvalid,
/* 216 - Lu category        */ CharacterIdentifierStart,
/* 217 - Lu category        */ CharacterIdentifierStart,
/* 218 - Lu category        */ CharacterIdentifierStart,
/* 219 - Lu category        */ CharacterIdentifierStart,
/* 220 - Lu category        */ CharacterIdentifierStart,
/* 221 - Lu category        */ CharacterIdentifierStart,
/* 222 - Lu category        */ CharacterIdentifierStart,
/* 223 - Ll category        */ CharacterIdentifierStart,
/* 224 - Ll category        */ CharacterIdentifierStart,
/* 225 - Ll category        */ CharacterIdentifierStart,
/* 226 - Ll category        */ CharacterIdentifierStart,
/* 227 - Ll category        */ CharacterIdentifierStart,
/* 228 - Ll category        */ CharacterIdentifierStart,
/* 229 - Ll category        */ CharacterIdentifierStart,
/* 230 - Ll category        */ CharacterIdentifierStart,
/* 231 - Ll category        */ CharacterIdentifierStart,
/* 232 - Ll category        */ CharacterIdentifierStart,
/* 233 - Ll category        */ CharacterIdentifierStart,
/* 234 - Ll category        */ CharacterIdentifierStart,
/* 235 - Ll category        */ CharacterIdentifierStart,
/* 236 - Ll category        */ CharacterIdentifierStart,
/* 237 - Ll category        */ CharacterIdentifierStart,
/* 238 - Ll category        */ CharacterIdentifierStart,
/* 239 - Ll category        */ CharacterIdentifierStart,
/* 240 - Ll category        */ CharacterIdentifierStart,
/* 241 - Ll category        */ CharacterIdentifierStart,
/* 242 - Ll category        */ CharacterIdentifierStart,
/* 243 - Ll category        */ CharacterIdentifierStart,
/* 244 - Ll category        */ CharacterIdentifierStart,
/* 245 - Ll category        */ CharacterIdentifierStart,
/* 246 - Ll category        */ CharacterIdentifierStart,
/* 247 - Sm category        */ CharacterInvalid,
/* 248 - Ll category        */ CharacterIdentifierStart,
/* 249 - Ll category        */ CharacterIdentifierStart,
/* 250 - Ll category        */ CharacterIdentifierStart,
/* 251 - Ll category        */ CharacterIdentifierStart,
/* 252 - Ll category        */ CharacterIdentifierStart,
/* 253 - Ll category        */ CharacterIdentifierStart,
/* 254 - Ll category        */ CharacterIdentifierStart,
/* 255 - Ll category        */ CharacterIdentifierStart
};

// This table provides the character that results from \X where X is the index in the table beginning
// with SPACE. A table value of 0 means that more processing needs to be done.
static const LChar singleCharacterEscapeValuesForASCII[128] = {
/*   0 - Null               */ 0,
/*   1 - Start of Heading   */ 0,
/*   2 - Start of Text      */ 0,
/*   3 - End of Text        */ 0,
/*   4 - End of Transm.     */ 0,
/*   5 - Enquiry            */ 0,
/*   6 - Acknowledgment     */ 0,
/*   7 - Bell               */ 0,
/*   8 - Back Space         */ 0,
/*   9 - Horizontal Tab     */ 0,
/*  10 - Line Feed          */ 0,
/*  11 - Vertical Tab       */ 0,
/*  12 - Form Feed          */ 0,
/*  13 - Carriage Return    */ 0,
/*  14 - Shift Out          */ 0,
/*  15 - Shift In           */ 0,
/*  16 - Data Line Escape   */ 0,
/*  17 - Device Control 1   */ 0,
/*  18 - Device Control 2   */ 0,
/*  19 - Device Control 3   */ 0,
/*  20 - Device Control 4   */ 0,
/*  21 - Negative Ack.      */ 0,
/*  22 - Synchronous Idle   */ 0,
/*  23 - End of Transmit    */ 0,
/*  24 - Cancel             */ 0,
/*  25 - End of Medium      */ 0,
/*  26 - Substitute         */ 0,
/*  27 - Escape             */ 0,
/*  28 - File Separator     */ 0,
/*  29 - Group Separator    */ 0,
/*  30 - Record Separator   */ 0,
/*  31 - Unit Separator     */ 0,
/*  32 - Space              */ ' ',
/*  33 - !                  */ '!',
/*  34 - "                  */ '"',
/*  35 - #                  */ '#',
/*  36 - $                  */ '$',
/*  37 - %                  */ '%',
/*  38 - &                  */ '&',
/*  39 - '                  */ '\'',
/*  40 - (                  */ '(',
/*  41 - )                  */ ')',
/*  42 - *                  */ '*',
/*  43 - +                  */ '+',
/*  44 - ,                  */ ',',
/*  45 - -                  */ '-',
/*  46 - .                  */ '.',
/*  47 - /                  */ '/',
/*  48 - 0                  */ 0,
/*  49 - 1                  */ 0,
/*  50 - 2                  */ 0,
/*  51 - 3                  */ 0,
/*  52 - 4                  */ 0,
/*  53 - 5                  */ 0,
/*  54 - 6                  */ 0,
/*  55 - 7                  */ 0,
/*  56 - 8                  */ 0,
/*  57 - 9                  */ 0,
/*  58 - :                  */ ':',
/*  59 - ;                  */ ';',
/*  60 - <                  */ '<',
/*  61 - =                  */ '=',
/*  62 - >                  */ '>',
/*  63 - ?                  */ '?',
/*  64 - @                  */ '@',
/*  65 - A                  */ 'A',
/*  66 - B                  */ 'B',
/*  67 - C                  */ 'C',
/*  68 - D                  */ 'D',
/*  69 - E                  */ 'E',
/*  70 - F                  */ 'F',
/*  71 - G                  */ 'G',
/*  72 - H                  */ 'H',
/*  73 - I                  */ 'I',
/*  74 - J                  */ 'J',
/*  75 - K                  */ 'K',
/*  76 - L                  */ 'L',
/*  77 - M                  */ 'M',
/*  78 - N                  */ 'N',
/*  79 - O                  */ 'O',
/*  80 - P                  */ 'P',
/*  81 - Q                  */ 'Q',
/*  82 - R                  */ 'R',
/*  83 - S                  */ 'S',
/*  84 - T                  */ 'T',
/*  85 - U                  */ 'U',
/*  86 - V                  */ 'V',
/*  87 - W                  */ 'W',
/*  88 - X                  */ 'X',
/*  89 - Y                  */ 'Y',
/*  90 - Z                  */ 'Z',
/*  91 - [                  */ '[',
/*  92 - \                  */ '\\',
/*  93 - ]                  */ ']',
/*  94 - ^                  */ '^',
/*  95 - _                  */ '_',
/*  96 - `                  */ '`',
/*  97 - a                  */ 'a',
/*  98 - b                  */ 0x08,
/*  99 - c                  */ 'c',
/* 100 - d                  */ 'd',
/* 101 - e                  */ 'e',
/* 102 - f                  */ 0x0C,
/* 103 - g                  */ 'g',
/* 104 - h                  */ 'h',
/* 105 - i                  */ 'i',
/* 106 - j                  */ 'j',
/* 107 - k                  */ 'k',
/* 108 - l                  */ 'l',
/* 109 - m                  */ 'm',
/* 110 - n                  */ 0x0A,
/* 111 - o                  */ 'o',
/* 112 - p                  */ 'p',
/* 113 - q                  */ 'q',
/* 114 - r                  */ 0x0D,
/* 115 - s                  */ 's',
/* 116 - t                  */ 0x09,
/* 117 - u                  */ 0,
/* 118 - v                  */ 0x0B,
/* 119 - w                  */ 'w',
/* 120 - x                  */ 0,
/* 121 - y                  */ 'y',
/* 122 - z                  */ 'z',
/* 123 - {                  */ '{',
/* 124 - |                  */ '|',
/* 125 - }                  */ '}',
/* 126 - ~                  */ '~',
/* 127 - Delete             */ 0
};

template <typename T>
Lexer<T>::Lexer(VM* vm)
    : m_isReparsing(false)
    , m_vm(vm)
{
}

template <typename T>
Lexer<T>::~Lexer()
{
}

template <typename T>
String Lexer<T>::invalidCharacterMessage() const
{
    switch (m_current) {
    case 0:
        return "Invalid character: '\\0'";
    case 10:
        return "Invalid character: '\\n'";
    case 11:
        return "Invalid character: '\\v'";
    case 13:
        return "Invalid character: '\\r'";
    case 35:
        return "Invalid character: '#'";
    case 64:
        return "Invalid character: '@'";
    case 96:
        return "Invalid character: '`'";
    default:
        return String::format("Invalid character '\\u%04u'", static_cast<unsigned>(m_current)).impl();
    }
}

template <typename T>
ALWAYS_INLINE const T* Lexer<T>::currentSourcePtr() const
{
    ASSERT(m_code <= m_codeEnd);
    return m_code;
}

template <typename T>
void Lexer<T>::setCode(const SourceCode& source, ParserArena* arena)
{
    m_arena = &arena->identifierArena();
    
    m_lineNumber = source.firstLine();
    m_lastToken = -1;
    
    const String& sourceString = source.provider()->source();

    if (!sourceString.isNull())
        setCodeStart(sourceString.impl());
    else
        m_codeStart = 0;

    m_source = &source;
    m_sourceOffset = source.startOffset();
    m_codeStartPlusOffset = m_codeStart + source.startOffset();
    m_code = m_codeStartPlusOffset;
    m_codeEnd = m_codeStart + source.endOffset();
    m_error = false;
    m_atLineStart = true;
    m_lineStart = m_code;
    m_lexErrorMessage = String();
    
    m_buffer8.reserveInitialCapacity(initialReadBufferCapacity);
    m_buffer16.reserveInitialCapacity((m_codeEnd - m_code) / 2);
    
    if (LIKELY(m_code < m_codeEnd))
        m_current = *m_code;
    else
        m_current = 0;
    ASSERT(currentOffset() == source.startOffset());
}

template <typename T>
template <int shiftAmount> ALWAYS_INLINE void Lexer<T>::internalShift()
{
    m_code += shiftAmount;
    ASSERT(currentOffset() >= currentLineStartOffset());
    m_current = *m_code;
}

template <typename T>
ALWAYS_INLINE void Lexer<T>::shift()
{
    // At one point timing showed that setting m_current to 0 unconditionally was faster than an if-else sequence.
    m_current = 0;
    ++m_code;
    if (LIKELY(m_code < m_codeEnd))
        m_current = *m_code;
}

template <typename T>
ALWAYS_INLINE bool Lexer<T>::atEnd() const
{
    ASSERT(!m_current || m_code < m_codeEnd);
    return UNLIKELY(UNLIKELY(!m_current) && m_code == m_codeEnd);
}

template <typename T>
ALWAYS_INLINE T Lexer<T>::peek(int offset) const
{
    ASSERT(offset > 0 && offset < 5);
    const T* code = m_code + offset;
    return (code < m_codeEnd) ? *code : 0;
}

template <typename T>
typename Lexer<T>::UnicodeHexValue Lexer<T>::parseFourDigitUnicodeHex()
{
    T char1 = peek(1);
    T char2 = peek(2);
    T char3 = peek(3);

    if (UNLIKELY(!isASCIIHexDigit(m_current) || !isASCIIHexDigit(char1) || !isASCIIHexDigit(char2) || !isASCIIHexDigit(char3)))
        return UnicodeHexValue((m_code + 4) >= m_codeEnd ? UnicodeHexValue::IncompleteHex : UnicodeHexValue::InvalidHex);

    int result = convertUnicode(m_current, char1, char2, char3);
    shift();
    shift();
    shift();
    shift();
    return UnicodeHexValue(result);
}

template <typename T>
void Lexer<T>::shiftLineTerminator()
{
    ASSERT(isLineTerminator(m_current));

    T prev = m_current;
    shift();

    // Allow both CRLF and LFCR.
    if (prev + m_current == '\n' + '\r')
        shift();

    ++m_lineNumber;
}

template <typename T>
ALWAYS_INLINE bool Lexer<T>::lastTokenWasRestrKeyword() const
{
    return m_lastToken == CONTINUE || m_lastToken == BREAK || m_lastToken == RETURN || m_lastToken == THROW;
}

static NEVER_INLINE bool isNonLatin1IdentStart(int c)
{
    return category(c) & (Letter_Uppercase | Letter_Lowercase | Letter_Titlecase | Letter_Modifier | Letter_Other);
}

static ALWAYS_INLINE bool isLatin1(LChar)
{
    return true;
}

static ALWAYS_INLINE bool isLatin1(UChar c)
{
    return c < 256;
}

static inline bool isIdentStart(LChar c)
{
    return typesOfLatin1Characters[c] == CharacterIdentifierStart;
}

static inline bool isIdentStart(UChar c)
{
    return isLatin1(c) ? isIdentStart(static_cast<LChar>(c)) : isNonLatin1IdentStart(c);
}

static NEVER_INLINE bool isNonLatin1IdentPart(int c)
{
    return (category(c) & (Letter_Uppercase | Letter_Lowercase | Letter_Titlecase | Letter_Modifier | Letter_Other
        | Mark_NonSpacing | Mark_SpacingCombining | Number_DecimalDigit | Punctuation_Connector)) || c == 0x200C || c == 0x200D;
}

static ALWAYS_INLINE bool isIdentPart(LChar c)
{
    // Character types are divided into two groups depending on whether they can be part of an
    // identifier or not. Those whose type value is less or equal than CharacterNumber can be
    // part of an identifier. (See the CharacterType definition for more details.)
    return typesOfLatin1Characters[c] <= CharacterNumber;
}

static ALWAYS_INLINE bool isIdentPart(UChar c)
{
    return isLatin1(c) ? isIdentPart(static_cast<LChar>(c)) : isNonLatin1IdentPart(c);
}

static inline LChar singleEscape(int c)
{
    if (c < 128) {
        ASSERT(static_cast<size_t>(c) < ARRAY_SIZE(singleCharacterEscapeValuesForASCII));
        return singleCharacterEscapeValuesForASCII[c];
    }
    return 0;
}

template <typename T>
inline void Lexer<T>::record8(int c)
{
    ASSERT(c >= 0);
    ASSERT(c <= 0xFF);
    m_buffer8.append(static_cast<LChar>(c));
}

template <typename T>
inline void assertCharIsIn8BitRange(T c)
{
    UNUSED_PARAM(c);
    ASSERT(c >= 0);
    ASSERT(c <= 0xFF);
}

template <>
inline void assertCharIsIn8BitRange(UChar c)
{
    UNUSED_PARAM(c);
    ASSERT(c <= 0xFF);
}

template <>
inline void assertCharIsIn8BitRange(LChar)
{
}

template <typename T>
inline void Lexer<T>::append8(const T* p, size_t length)
{
    size_t currentSize = m_buffer8.size();
    m_buffer8.grow(currentSize + length);
    LChar* rawBuffer = m_buffer8.data() + currentSize;

    for (size_t i = 0; i < length; i++) {
        T c = p[i];
        assertCharIsIn8BitRange(c);
        rawBuffer[i] = c;
    }
}

template <typename T>
inline void Lexer<T>::append16(const LChar* p, size_t length)
{
    size_t currentSize = m_buffer16.size();
    m_buffer16.grow(currentSize + length);
    UChar* rawBuffer = m_buffer16.data() + currentSize;

    for (size_t i = 0; i < length; i++)
        rawBuffer[i] = p[i];
}

template <typename T>
inline void Lexer<T>::record16(T c)
{
    m_buffer16.append(c);
}

template <typename T>
inline void Lexer<T>::record16(int c)
{
    ASSERT(c >= 0);
    ASSERT(c <= static_cast<int>(USHRT_MAX));
    m_buffer16.append(static_cast<UChar>(c));
}

template <>
template <bool shouldCreateIdentifier> ALWAYS_INLINE JSTokenType Lexer<LChar>::parseIdentifier(JSTokenData* tokenData, unsigned lexerFlags, bool strictMode)
{
    const ptrdiff_t remaining = m_codeEnd - m_code;
    if ((remaining >= maxTokenLength) && !(lexerFlags & LexerFlagsIgnoreReservedWords)) {
        JSTokenType keyword = parseKeyword<shouldCreateIdentifier>(tokenData);
        if (keyword != IDENT) {
            ASSERT((!shouldCreateIdentifier) || tokenData->ident);
            return keyword == RESERVED_IF_STRICT && !strictMode ? IDENT : keyword;
        }
    }

    const LChar* identifierStart = currentSourcePtr();
    unsigned identifierLineStart = currentLineStartOffset();
    
    while (isIdentPart(m_current))
        shift();
    
    if (UNLIKELY(m_current == '\\')) {
        setOffsetFromSourcePtr(identifierStart, identifierLineStart);
        return parseIdentifierSlowCase<shouldCreateIdentifier>(tokenData, lexerFlags, strictMode);
    }

    const Identifier* ident = 0;
    
    if (shouldCreateIdentifier) {
        int identifierLength = currentSourcePtr() - identifierStart;
        ident = makeIdentifier(identifierStart, identifierLength);

        tokenData->ident = ident;
    } else
        tokenData->ident = 0;

    if (UNLIKELY((remaining < maxTokenLength) && !(lexerFlags & LexerFlagsIgnoreReservedWords))) {
        ASSERT(shouldCreateIdentifier);
        if (remaining < maxTokenLength) {
            const HashEntry* entry = m_vm->keywords->getKeyword(*ident);
            ASSERT((remaining < maxTokenLength) || !entry);
            if (!entry)
                return IDENT;
            JSTokenType token = static_cast<JSTokenType>(entry->lexerValue());
            return (token != RESERVED_IF_STRICT) || strictMode ? token : IDENT;
        }
        return IDENT;
    }

    return IDENT;
}

template <>
template <bool shouldCreateIdentifier> ALWAYS_INLINE JSTokenType Lexer<UChar>::parseIdentifier(JSTokenData* tokenData, unsigned lexerFlags, bool strictMode)
{
    const ptrdiff_t remaining = m_codeEnd - m_code;
    if ((remaining >= maxTokenLength) && !(lexerFlags & LexerFlagsIgnoreReservedWords)) {
        JSTokenType keyword = parseKeyword<shouldCreateIdentifier>(tokenData);
        if (keyword != IDENT) {
            ASSERT((!shouldCreateIdentifier) || tokenData->ident);
            return keyword == RESERVED_IF_STRICT && !strictMode ? IDENT : keyword;
        }
    }

    const UChar* identifierStart = currentSourcePtr();
    int identifierLineStart = currentLineStartOffset();

    UChar orAllChars = 0;
    
    while (isIdentPart(m_current)) {
        orAllChars |= m_current;
        shift();
    }
    
    if (UNLIKELY(m_current == '\\')) {
        setOffsetFromSourcePtr(identifierStart, identifierLineStart);
        return parseIdentifierSlowCase<shouldCreateIdentifier>(tokenData, lexerFlags, strictMode);
    }

    bool isAll8Bit = false;

    if (!(orAllChars & ~0xff))
        isAll8Bit = true;

    const Identifier* ident = 0;
    
    if (shouldCreateIdentifier) {
        int identifierLength = currentSourcePtr() - identifierStart;
        if (isAll8Bit)
            ident = makeIdentifierLCharFromUChar(identifierStart, identifierLength);
        else
            ident = makeIdentifier(identifierStart, identifierLength);
        
        tokenData->ident = ident;
    } else
        tokenData->ident = 0;
    
    if (UNLIKELY((remaining < maxTokenLength) && !(lexerFlags & LexerFlagsIgnoreReservedWords))) {
        ASSERT(shouldCreateIdentifier);
        if (remaining < maxTokenLength) {
            const HashEntry* entry = m_vm->keywords->getKeyword(*ident);
            ASSERT((remaining < maxTokenLength) || !entry);
            if (!entry)
                return IDENT;
            JSTokenType token = static_cast<JSTokenType>(entry->lexerValue());
            return (token != RESERVED_IF_STRICT) || strictMode ? token : IDENT;
        }
        return IDENT;
    }

    return IDENT;
}

template <typename T>
template <bool shouldCreateIdentifier> JSTokenType Lexer<T>::parseIdentifierSlowCase(JSTokenData* tokenData, unsigned lexerFlags, bool strictMode)
{
    const ptrdiff_t remaining = m_codeEnd - m_code;
    const T* identifierStart = currentSourcePtr();
    bool bufferRequired = false;

    while (true) {
        if (LIKELY(isIdentPart(m_current))) {
            shift();
            continue;
        }
        if (LIKELY(m_current != '\\'))
            break;

        // \uXXXX unicode characters.
        bufferRequired = true;
        if (identifierStart != currentSourcePtr())
            m_buffer16.append(identifierStart, currentSourcePtr() - identifierStart);
        shift();
        if (UNLIKELY(m_current != 'u'))
            return atEnd() ? UNTERMINATED_IDENTIFIER_ESCAPE_ERRORTOK : INVALID_IDENTIFIER_ESCAPE_ERRORTOK;
        shift();
        UnicodeHexValue character = parseFourDigitUnicodeHex();
        if (UNLIKELY(!character.isValid()))
            return character.valueType() == UnicodeHexValue::IncompleteHex ? UNTERMINATED_IDENTIFIER_UNICODE_ESCAPE_ERRORTOK : INVALID_IDENTIFIER_UNICODE_ESCAPE_ERRORTOK;
        UChar ucharacter = static_cast<UChar>(character.value());
        if (UNLIKELY(m_buffer16.size() ? !isIdentPart(ucharacter) : !isIdentStart(ucharacter)))
            return INVALID_IDENTIFIER_UNICODE_ESCAPE_ERRORTOK;
        if (shouldCreateIdentifier)
            record16(ucharacter);
        identifierStart = currentSourcePtr();
    }

    int identifierLength;
    const Identifier* ident = 0;
    if (shouldCreateIdentifier) {
        if (!bufferRequired) {
            identifierLength = currentSourcePtr() - identifierStart;
            ident = makeIdentifier(identifierStart, identifierLength);
        } else {
            if (identifierStart != currentSourcePtr())
                m_buffer16.append(identifierStart, currentSourcePtr() - identifierStart);
            ident = makeIdentifier(m_buffer16.data(), m_buffer16.size());
        }

        tokenData->ident = ident;
    } else
        tokenData->ident = 0;

    if (LIKELY(!bufferRequired && !(lexerFlags & LexerFlagsIgnoreReservedWords))) {
        ASSERT(shouldCreateIdentifier);
        // Keywords must not be recognized if there was an \uXXXX in the identifier.
        if (remaining < maxTokenLength) {
            const HashEntry* entry = m_vm->keywords->getKeyword(*ident);
            ASSERT((remaining < maxTokenLength) || !entry);
            if (!entry)
                return IDENT;
            JSTokenType token = static_cast<JSTokenType>(entry->lexerValue());
            return (token != RESERVED_IF_STRICT) || strictMode ? token : IDENT;
        }
        return IDENT;
    }

    m_buffer16.resize(0);
    return IDENT;
}

static ALWAYS_INLINE bool characterRequiresParseStringSlowCase(LChar character)
{
    return character < 0xE;
}

static ALWAYS_INLINE bool characterRequiresParseStringSlowCase(UChar character)
{
    return character < 0xE || character > 0xFF;
}

template <typename T>
template <bool shouldBuildStrings> ALWAYS_INLINE typename Lexer<T>::StringParseResult Lexer<T>::parseString(JSTokenData* tokenData, bool strictMode)
{
    int startingOffset = currentOffset();
    int startingLineStartOffset = currentLineStartOffset();
    int startingLineNumber = lineNumber();
    T stringQuoteCharacter = m_current;
    shift();

    const T* stringStart = currentSourcePtr();

    while (m_current != stringQuoteCharacter) {
        if (UNLIKELY(m_current == '\\')) {
            if (stringStart != currentSourcePtr() && shouldBuildStrings)
                append8(stringStart, currentSourcePtr() - stringStart);
            shift();

            LChar escape = singleEscape(m_current);

            // Most common escape sequences first
            if (escape) {
                if (shouldBuildStrings)
                    record8(escape);
                shift();
            } else if (UNLIKELY(isLineTerminator(m_current)))
                shiftLineTerminator();
            else if (m_current == 'x') {
                shift();
                if (!isASCIIHexDigit(m_current) || !isASCIIHexDigit(peek(1))) {
                    m_lexErrorMessage = "\\x can only be followed by a hex character sequence";
                    return (atEnd() || (isASCIIHexDigit(m_current) && (m_code + 1 == m_codeEnd))) ? StringUnterminated : StringCannotBeParsed;
                }
                T prev = m_current;
                shift();
                if (shouldBuildStrings)
                    record8(convertHex(prev, m_current));
                shift();
            } else {
                setOffset(startingOffset, startingLineStartOffset);
                setLineNumber(startingLineNumber);
                m_buffer8.resize(0);
                return parseStringSlowCase<shouldBuildStrings>(tokenData, strictMode);
            }
            stringStart = currentSourcePtr();
            continue;
        }

        if (UNLIKELY(characterRequiresParseStringSlowCase(m_current))) {
            setOffset(startingOffset, startingLineStartOffset);
            setLineNumber(startingLineNumber);
            m_buffer8.resize(0);
            return parseStringSlowCase<shouldBuildStrings>(tokenData, strictMode);
        }

        shift();
    }

    if (currentSourcePtr() != stringStart && shouldBuildStrings)
        append8(stringStart, currentSourcePtr() - stringStart);
    if (shouldBuildStrings) {
        tokenData->ident = makeIdentifier(m_buffer8.data(), m_buffer8.size());
        m_buffer8.resize(0);
    } else
        tokenData->ident = 0;

    return StringParsedSuccessfully;
}

template <typename T>
template <bool shouldBuildStrings> typename Lexer<T>::StringParseResult Lexer<T>::parseStringSlowCase(JSTokenData* tokenData, bool strictMode)
{
    T stringQuoteCharacter = m_current;
    shift();

    const T* stringStart = currentSourcePtr();

    while (m_current != stringQuoteCharacter) {
        if (UNLIKELY(m_current == '\\')) {
            if (stringStart != currentSourcePtr() && shouldBuildStrings)
                append16(stringStart, currentSourcePtr() - stringStart);
            shift();

            LChar escape = singleEscape(m_current);

            // Most common escape sequences first
            if (escape) {
                if (shouldBuildStrings)
                    record16(escape);
                shift();
            } else if (UNLIKELY(isLineTerminator(m_current)))
                shiftLineTerminator();
            else if (m_current == 'x') {
                shift();
                if (!isASCIIHexDigit(m_current) || !isASCIIHexDigit(peek(1))) {
                    m_lexErrorMessage = "\\x can only be followed by a hex character sequence";
                    return StringCannotBeParsed;
                }
                T prev = m_current;
                shift();
                if (shouldBuildStrings)
                    record16(convertHex(prev, m_current));
                shift();
            } else if (m_current == 'u') {
                shift();
                UnicodeHexValue character = parseFourDigitUnicodeHex();
                if (character.isValid()) {
                    if (shouldBuildStrings)
                        record16(character.value());
                } else if (m_current == stringQuoteCharacter) {
                    if (shouldBuildStrings)
                        record16('u');
                } else {
                    m_lexErrorMessage = "\\u can only be followed by a Unicode character sequence";
                    return character.valueType() == UnicodeHexValue::IncompleteHex ? StringUnterminated : StringCannotBeParsed;
                }
            } else if (strictMode && isASCIIDigit(m_current)) {
                // The only valid numeric escape in strict mode is '\0', and this must not be followed by a decimal digit.
                int character1 = m_current;
                shift();
                if (character1 != '0' || isASCIIDigit(m_current)) {
                    m_lexErrorMessage = "The only valid numeric escape in strict mode is '\\0'";
                    return StringCannotBeParsed;
                }
                if (shouldBuildStrings)
                    record16(0);
            } else if (!strictMode && isASCIIOctalDigit(m_current)) {
                // Octal character sequences
                T character1 = m_current;
                shift();
                if (isASCIIOctalDigit(m_current)) {
                    // Two octal characters
                    T character2 = m_current;
                    shift();
                    if (character1 >= '0' && character1 <= '3' && isASCIIOctalDigit(m_current)) {
                        if (shouldBuildStrings)
                            record16((character1 - '0') * 64 + (character2 - '0') * 8 + m_current - '0');
                        shift();
                    } else {
                        if (shouldBuildStrings)
                            record16((character1 - '0') * 8 + character2 - '0');
                    }
                } else {
                    if (shouldBuildStrings)
                        record16(character1 - '0');
                }
            } else if (!atEnd()) {
                if (shouldBuildStrings)
                    record16(m_current);
                shift();
            } else {
                m_lexErrorMessage = "Unterminated string constant";
                return StringUnterminated;
            }

            stringStart = currentSourcePtr();
            continue;
        }
        // Fast check for characters that require special handling.
        // Catches 0, \n, \r, 0x2028, and 0x2029 as efficiently
        // as possible, and lets through all common ASCII characters.
        if (UNLIKELY(((static_cast<unsigned>(m_current) - 0xE) & 0x2000))) {
            // New-line or end of input is not allowed
            if (atEnd() || isLineTerminator(m_current)) {
                m_lexErrorMessage = "Unexpected EOF";
                return atEnd() ? StringUnterminated : StringCannotBeParsed;
            }
            // Anything else is just a normal character
        }
        shift();
    }

    if (currentSourcePtr() != stringStart && shouldBuildStrings)
        append16(stringStart, currentSourcePtr() - stringStart);
    if (shouldBuildStrings)
        tokenData->ident = makeIdentifier(m_buffer16.data(), m_buffer16.size());
    else
        tokenData->ident = 0;

    m_buffer16.resize(0);
    return StringParsedSuccessfully;
}

template <typename T>
ALWAYS_INLINE void Lexer<T>::parseHex(double& returnValue)
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

template <typename T>
ALWAYS_INLINE bool Lexer<T>::parseOctal(double& returnValue)
{
    // Optimization: most octal values fit into 4 bytes.
    uint32_t octalValue = 0;
    int maximumDigits = 9;
    // Temporary buffer for the digits. Makes easier
    // to reconstruct the input characters when needed.
    LChar digits[10];

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

template <typename T>
ALWAYS_INLINE bool Lexer<T>::parseDecimal(double& returnValue)
{
    // Optimization: most decimal values fit into 4 bytes.
    uint32_t decimalValue = 0;

    // Since parseOctal may be executed before parseDecimal,
    // the m_buffer8 may hold ascii digits.
    if (!m_buffer8.size()) {
        int maximumDigits = 9;
        // Temporary buffer for the digits. Makes easier
        // to reconstruct the input characters when needed.
        LChar digits[10];

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

template <typename T>
ALWAYS_INLINE void Lexer<T>::parseNumberAfterDecimalPoint()
{
    record8('.');
    while (isASCIIDigit(m_current)) {
        record8(m_current);
        shift();
    }
}

template <typename T>
ALWAYS_INLINE bool Lexer<T>::parseNumberAfterExponentIndicator()
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

template <typename T>
ALWAYS_INLINE bool Lexer<T>::parseMultilineComment()
{
    while (true) {
        while (UNLIKELY(m_current == '*')) {
            shift();
            if (m_current == '/') {
                shift();
                return true;
            }
        }

        if (atEnd())
            return false;

        if (isLineTerminator(m_current)) {
            shiftLineTerminator();
            m_terminator = true;
        } else
            shift();
    }
}

template <typename T>
bool Lexer<T>::nextTokenIsColon()
{
    const T* code = m_code;
    while (code < m_codeEnd && (isWhiteSpace(*code) || isLineTerminator(*code)))
        code++;
    
    return code < m_codeEnd && *code == ':';
}

template <typename T>
JSTokenType Lexer<T>::lex(JSToken* tokenRecord, unsigned lexerFlags, bool strictMode)
{
    JSTokenData* tokenData = &tokenRecord->m_data;
    JSTokenLocation* tokenLocation = &tokenRecord->m_location;
    ASSERT(!m_error);
    ASSERT(m_buffer8.isEmpty());
    ASSERT(m_buffer16.isEmpty());

    JSTokenType token = ERRORTOK;
    m_terminator = false;

start:
    while (isWhiteSpace(m_current))
        shift();

    if (atEnd())
        return EOFTOK;
    
    tokenLocation->startOffset = currentOffset();
    ASSERT(currentOffset() >= currentLineStartOffset());
    tokenRecord->m_startPosition = currentPosition();

    CharacterType type;
    if (LIKELY(isLatin1(m_current)))
        type = static_cast<CharacterType>(typesOfLatin1Characters[m_current]);
    else if (isNonLatin1IdentStart(m_current))
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
            m_lexErrorMessage = "Multiline comment was not closed properly";
            token = UNTERMINATED_MULTILINE_COMMENT_ERRORTOK;
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
        shift();
        token = SEMICOLON;
        break;
    case CharacterOpenBrace:
        tokenData->line = lineNumber();
        tokenData->offset = currentOffset();
        tokenData->lineStartOffset = currentLineStartOffset();
        ASSERT(tokenData->offset >= tokenData->lineStartOffset);
        shift();
        token = OPENBRACE;
        break;
    case CharacterCloseBrace:
        tokenData->line = lineNumber();
        tokenData->offset = currentOffset();
        tokenData->lineStartOffset = currentLineStartOffset();
        ASSERT(tokenData->offset >= tokenData->lineStartOffset);
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
            parseHex(tokenData->doubleValue);
            token = NUMBER;
        } else {
            record8('0');
            if (isASCIIOctalDigit(m_current)) {
                if (parseOctal(tokenData->doubleValue)) {
                    if (strictMode) {
                        m_lexErrorMessage = "Octal escapes are forbidden in strict mode";
                        token = INVALID_OCTAL_NUMBER_ERRORTOK;
                        goto returnError;
                    }
                    token = NUMBER;
                }
            }
        }
        // Fall through into CharacterNumber
    case CharacterNumber:
        if (LIKELY(token != NUMBER)) {
            if (!parseDecimal(tokenData->doubleValue)) {
                if (m_current == '.') {
                    shift();
inNumberAfterDecimalPoint:
                    parseNumberAfterDecimalPoint();
                }
                if ((m_current | 0x20) == 'e') {
                    if (!parseNumberAfterExponentIndicator()) {
                        m_lexErrorMessage = "Non-number found after exponent indicator";
                        token = atEnd() ? UNTERMINATED_NUMERIC_LITERAL_ERRORTOK : INVALID_NUMERIC_LITERAL_ERRORTOK;
                        goto returnError;
                    }
                }
                size_t parsedLength;
                tokenData->doubleValue = parseDouble(m_buffer8.data(), m_buffer8.size(), parsedLength);
            }
            token = NUMBER;
        }

        // No identifiers allowed directly after numeric literal, e.g. "3in" is bad.
        if (UNLIKELY(isIdentStart(m_current))) {
            m_lexErrorMessage = "At least one digit must occur after a decimal point";
            token = atEnd() ? UNTERMINATED_NUMERIC_LITERAL_ERRORTOK : INVALID_NUMERIC_LITERAL_ERRORTOK;
            goto returnError;
        }
        m_buffer8.resize(0);
        break;
    case CharacterQuote:
        if (lexerFlags & LexerFlagsDontBuildStrings) {
            StringParseResult result = parseString<false>(tokenData, strictMode);
            if (UNLIKELY(result != StringParsedSuccessfully)) {
                token = result == StringUnterminated ? UNTERMINATED_STRING_LITERAL_ERRORTOK : INVALID_STRING_LITERAL_ERRORTOK;
                goto returnError;
            }
        } else {
            StringParseResult result = parseString<true>(tokenData, strictMode);
            if (UNLIKELY(result != StringParsedSuccessfully)) {
                token = result == StringUnterminated ? UNTERMINATED_STRING_LITERAL_ERRORTOK : INVALID_STRING_LITERAL_ERRORTOK;
                goto returnError;
            }
        }
        shift();
        token = STRING;
        break;
    case CharacterIdentifierStart:
        ASSERT(isIdentStart(m_current));
        // Fall through into CharacterBackSlash.
    case CharacterBackSlash:
        if (lexerFlags & LexexFlagsDontBuildKeywords)
            token = parseIdentifier<false>(tokenData, lexerFlags, strictMode);
        else
            token = parseIdentifier<true>(tokenData, lexerFlags, strictMode);
        break;
    case CharacterLineTerminator:
        ASSERT(isLineTerminator(m_current));
        shiftLineTerminator();
        m_atLineStart = true;
        m_terminator = true;
        m_lineStart = m_code;
        goto start;
    case CharacterInvalid:
        m_lexErrorMessage = invalidCharacterMessage();
        token = ERRORTOK;
        goto returnError;
    default:
        RELEASE_ASSERT_NOT_REACHED();
        m_lexErrorMessage = "Internal Error";
        token = ERRORTOK;
        goto returnError;
    }

    m_atLineStart = false;
    goto returnToken;

inSingleLineComment:
    while (!isLineTerminator(m_current)) {
        if (atEnd())
            return EOFTOK;
        shift();
    }
    shiftLineTerminator();
    m_atLineStart = true;
    m_terminator = true;
    m_lineStart = m_code;
    if (!lastTokenWasRestrKeyword())
        goto start;

    token = SEMICOLON;
    // Fall through into returnToken.

returnToken:
    tokenLocation->line = m_lineNumber;
    tokenLocation->endOffset = currentOffset();
    tokenLocation->lineStartOffset = currentLineStartOffset();
    ASSERT(tokenLocation->endOffset >= tokenLocation->lineStartOffset);
    tokenRecord->m_endPosition = currentPosition();
    m_lastToken = token;
    return token;

returnError:
    m_error = true;
    tokenLocation->line = m_lineNumber;
    tokenLocation->endOffset = currentOffset();
    tokenLocation->lineStartOffset = currentLineStartOffset();
    ASSERT(tokenLocation->endOffset >= tokenLocation->lineStartOffset);
    tokenRecord->m_endPosition = currentPosition();
    RELEASE_ASSERT(token & ErrorTokenFlag);
    return token;
}

template <typename T>
static inline void orCharacter(UChar&, UChar);

template <>
inline void orCharacter<LChar>(UChar&, UChar) { }

template <>
inline void orCharacter<UChar>(UChar& orAccumulator, UChar character)
{
    orAccumulator |= character;
}

template <typename T>
bool Lexer<T>::scanRegExp(const Identifier*& pattern, const Identifier*& flags, UChar patternPrefix)
{
    ASSERT(m_buffer16.isEmpty());

    bool lastWasEscape = false;
    bool inBrackets = false;
    UChar charactersOredTogether = 0;

    if (patternPrefix) {
        ASSERT(!isLineTerminator(patternPrefix));
        ASSERT(patternPrefix != '/');
        ASSERT(patternPrefix != '[');
        record16(patternPrefix);
    }

    while (true) {
        if (isLineTerminator(m_current) || atEnd()) {
            m_buffer16.resize(0);
            return false;
        }

        T prev = m_current;
        
        shift();

        if (prev == '/' && !lastWasEscape && !inBrackets)
            break;

        record16(prev);
        orCharacter<T>(charactersOredTogether, prev);

        if (lastWasEscape) {
            lastWasEscape = false;
            continue;
        }

        switch (prev) {
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

    pattern = makeRightSizedIdentifier(m_buffer16.data(), m_buffer16.size(), charactersOredTogether);

    m_buffer16.resize(0);
    charactersOredTogether = 0;

    while (isIdentPart(m_current)) {
        record16(m_current);
        orCharacter<T>(charactersOredTogether, m_current);
        shift();
    }

    flags = makeRightSizedIdentifier(m_buffer16.data(), m_buffer16.size(), charactersOredTogether);
    m_buffer16.resize(0);

    return true;
}

template <typename T>
bool Lexer<T>::skipRegExp()
{
    bool lastWasEscape = false;
    bool inBrackets = false;

    while (true) {
        if (isLineTerminator(m_current) || atEnd())
            return false;

        T prev = m_current;
        
        shift();

        if (prev == '/' && !lastWasEscape && !inBrackets)
            break;

        if (lastWasEscape) {
            lastWasEscape = false;
            continue;
        }

        switch (prev) {
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

template <typename T>
void Lexer<T>::clear()
{
    m_arena = 0;

    Vector<LChar> newBuffer8;
    m_buffer8.swap(newBuffer8);

    Vector<UChar> newBuffer16;
    m_buffer16.swap(newBuffer16);

    m_isReparsing = false;
}

template <typename T>
SourceCode Lexer<T>::sourceCode(int openBrace, int closeBrace, int firstLine, unsigned startColumn)
{
    ASSERT(m_source->provider()->source()[openBrace] == '{');
    ASSERT(m_source->provider()->source()[closeBrace] == '}');
    return SourceCode(m_source->provider(), openBrace, closeBrace + 1, firstLine, startColumn);
}

// Instantiate the two flavors of Lexer we need instead of putting most of this file in Lexer.h
template class Lexer<LChar>;
template class Lexer<UChar>;

} // namespace JSC
