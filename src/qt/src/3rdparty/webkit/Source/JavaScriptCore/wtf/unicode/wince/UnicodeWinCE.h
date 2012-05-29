/*
 *  Copyright (C) 2006 George Staikos <staikos@kde.org>
 *  Copyright (C) 2006 Alexey Proskuryakov <ap@nypop.com>
 *  Copyright (C) 2007 Apple Computer, Inc. All rights reserved.
 *  Copyright (C) 2007-2009 Torch Mobile, Inc.
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

#ifndef WTF_UnicodeWinCE_h
#define WTF_UnicodeWinCE_h

#include "UnicodeMacrosFromICU.h"

#include "ce_unicode.h"

#define TO_MASK(x) (1 << (x))

namespace WTF {
namespace Unicode {

enum Direction {
    LeftToRight = UnicodeCE::U_LEFT_TO_RIGHT,
    RightToLeft = UnicodeCE::U_RIGHT_TO_LEFT,
    EuropeanNumber = UnicodeCE::U_EUROPEAN_NUMBER,
    EuropeanNumberSeparator = UnicodeCE::U_EUROPEAN_NUMBER_SEPARATOR,
    EuropeanNumberTerminator = UnicodeCE::U_EUROPEAN_NUMBER_TERMINATOR,
    ArabicNumber = UnicodeCE::U_ARABIC_NUMBER,
    CommonNumberSeparator = UnicodeCE::U_COMMON_NUMBER_SEPARATOR,
    BlockSeparator = UnicodeCE::U_BLOCK_SEPARATOR,
    SegmentSeparator = UnicodeCE::U_SEGMENT_SEPARATOR,
    WhiteSpaceNeutral = UnicodeCE::U_WHITE_SPACE_NEUTRAL,
    OtherNeutral = UnicodeCE::U_OTHER_NEUTRAL,
    LeftToRightEmbedding = UnicodeCE::U_LEFT_TO_RIGHT_EMBEDDING,
    LeftToRightOverride = UnicodeCE::U_LEFT_TO_RIGHT_OVERRIDE,
    RightToLeftArabic = UnicodeCE::U_RIGHT_TO_LEFT_ARABIC,
    RightToLeftEmbedding = UnicodeCE::U_RIGHT_TO_LEFT_EMBEDDING,
    RightToLeftOverride = UnicodeCE::U_RIGHT_TO_LEFT_OVERRIDE,
    PopDirectionalFormat = UnicodeCE::U_POP_DIRECTIONAL_FORMAT,
    NonSpacingMark = UnicodeCE::U_DIR_NON_SPACING_MARK,
    BoundaryNeutral = UnicodeCE::U_BOUNDARY_NEUTRAL
};

enum DecompositionType {
    DecompositionNone = UnicodeCE::U_DT_NONE,
    DecompositionCanonical = UnicodeCE::U_DT_CANONICAL,
    DecompositionCompat = UnicodeCE::U_DT_COMPAT,
    DecompositionCircle = UnicodeCE::U_DT_CIRCLE,
    DecompositionFinal = UnicodeCE::U_DT_FINAL,
    DecompositionFont = UnicodeCE::U_DT_FONT,
    DecompositionFraction = UnicodeCE::U_DT_FRACTION,
    DecompositionInitial = UnicodeCE::U_DT_INITIAL,
    DecompositionIsolated = UnicodeCE::U_DT_ISOLATED,
    DecompositionMedial = UnicodeCE::U_DT_MEDIAL,
    DecompositionNarrow = UnicodeCE::U_DT_NARROW,
    DecompositionNoBreak = UnicodeCE::U_DT_NOBREAK,
    DecompositionSmall = UnicodeCE::U_DT_SMALL,
    DecompositionSquare = UnicodeCE::U_DT_SQUARE,
    DecompositionSub = UnicodeCE::U_DT_SUB,
    DecompositionSuper = UnicodeCE::U_DT_SUPER,
    DecompositionVertical = UnicodeCE::U_DT_VERTICAL,
    DecompositionWide = UnicodeCE::U_DT_WIDE
};

enum CharCategory {
    NoCategory =  0,
    Other_NotAssigned = TO_MASK(UnicodeCE::U_GENERAL_OTHER_TYPES),
    Letter_Uppercase = TO_MASK(UnicodeCE::U_UPPERCASE_LETTER),
    Letter_Lowercase = TO_MASK(UnicodeCE::U_LOWERCASE_LETTER),
    Letter_Titlecase = TO_MASK(UnicodeCE::U_TITLECASE_LETTER),
    Letter_Modifier = TO_MASK(UnicodeCE::U_MODIFIER_LETTER),
    Letter_Other = TO_MASK(UnicodeCE::U_OTHER_LETTER),

    Mark_NonSpacing = TO_MASK(UnicodeCE::U_NON_SPACING_MARK),
    Mark_Enclosing = TO_MASK(UnicodeCE::U_ENCLOSING_MARK),
    Mark_SpacingCombining = TO_MASK(UnicodeCE::U_COMBINING_SPACING_MARK),

    Number_DecimalDigit = TO_MASK(UnicodeCE::U_DECIMAL_DIGIT_NUMBER),
    Number_Letter = TO_MASK(UnicodeCE::U_LETTER_NUMBER),
    Number_Other = TO_MASK(UnicodeCE::U_OTHER_NUMBER),

    Separator_Space = TO_MASK(UnicodeCE::U_SPACE_SEPARATOR),
    Separator_Line = TO_MASK(UnicodeCE::U_LINE_SEPARATOR),
    Separator_Paragraph = TO_MASK(UnicodeCE::U_PARAGRAPH_SEPARATOR),

    Other_Control = TO_MASK(UnicodeCE::U_CONTROL_CHAR),
    Other_Format = TO_MASK(UnicodeCE::U_FORMAT_CHAR),
    Other_PrivateUse = TO_MASK(UnicodeCE::U_PRIVATE_USE_CHAR),
    Other_Surrogate = TO_MASK(UnicodeCE::U_SURROGATE),

    Punctuation_Dash = TO_MASK(UnicodeCE::U_DASH_PUNCTUATION),
    Punctuation_Open = TO_MASK(UnicodeCE::U_START_PUNCTUATION),
    Punctuation_Close = TO_MASK(UnicodeCE::U_END_PUNCTUATION),
    Punctuation_Connector = TO_MASK(UnicodeCE::U_CONNECTOR_PUNCTUATION),
    Punctuation_Other = TO_MASK(UnicodeCE::U_OTHER_PUNCTUATION),

    Symbol_Math = TO_MASK(UnicodeCE::U_MATH_SYMBOL),
    Symbol_Currency = TO_MASK(UnicodeCE::U_CURRENCY_SYMBOL),
    Symbol_Modifier = TO_MASK(UnicodeCE::U_MODIFIER_SYMBOL),
    Symbol_Other = TO_MASK(UnicodeCE::U_OTHER_SYMBOL),

    Punctuation_InitialQuote = TO_MASK(UnicodeCE::U_INITIAL_PUNCTUATION),
    Punctuation_FinalQuote = TO_MASK(UnicodeCE::U_FINAL_PUNCTUATION)
};

CharCategory category(unsigned int);

bool isSpace(UChar);
bool isLetter(UChar);
bool isPrintableChar(UChar);
bool isUpper(UChar);
bool isLower(UChar);
bool isPunct(UChar);
bool isDigit(UChar);
bool isAlphanumeric(UChar);
inline bool isSeparatorSpace(UChar c) { return category(c) == Separator_Space; }
inline bool isHighSurrogate(UChar c) { return (c & 0xfc00) == 0xd800; }
inline bool isLowSurrogate(UChar c) { return (c & 0xfc00) == 0xdc00; }

UChar toLower(UChar);
UChar toUpper(UChar);
UChar foldCase(UChar);
UChar toTitleCase(UChar);
int toLower(UChar* result, int resultLength, const UChar* source, int sourceLength, bool* isError);
int toUpper(UChar* result, int resultLength, const UChar* source, int sourceLength, bool* isError);
int foldCase(UChar* result, int resultLength, const UChar* source, int sourceLength, bool* isError);

int digitValue(UChar);

UChar mirroredChar(UChar32);
unsigned char combiningClass(UChar32);
DecompositionType decompositionType(UChar32);
Direction direction(UChar32);
inline bool isArabicChar(UChar32 c)
{
    return c >= 0x0600 && c <= 0x06FF;
}

inline bool hasLineBreakingPropertyComplexContext(UChar32)
{
    return false; // FIXME: implement!
}

inline int umemcasecmp(const UChar* a, const UChar* b, int len)
{
    for (int i = 0; i < len; ++i) {
        UChar c1 = foldCase(a[i]);
        UChar c2 = foldCase(b[i]);
        if (c1 != c2)
            return c1 - c2;
    }
    return 0;
}

inline UChar32 surrogateToUcs4(UChar high, UChar low)
{
    return (UChar32(high) << 10) + low - 0x35fdc00;
}

} // namespace Unicode
} // namespace WTF

#endif // WTF_UnicodeWinCE_h
