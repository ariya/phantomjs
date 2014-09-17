/*
 *  Copyright (C) 2006 George Staikos <staikos@kde.org>
 *  Copyright (C) 2006 Alexey Proskuryakov <ap@nypop.com>
 *  Copyright (C) 2007 Apple Computer, Inc. All rights reserved.
 *  Copyright (C) 2007-2009 Torch Mobile, Inc.
 *  Copyright (C) 2010 Company 100, Inc.
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

#ifndef UnicodeBrew_h
#define UnicodeBrew_h

#include "UnicodeFromICU.h"
#include "UnicodeMacrosFromICU.h"

namespace WTF {
namespace Unicode {

enum Direction {
    LeftToRight = ICU::U_LEFT_TO_RIGHT,
    RightToLeft = ICU::U_RIGHT_TO_LEFT,
    EuropeanNumber = ICU::U_EUROPEAN_NUMBER,
    EuropeanNumberSeparator = ICU::U_EUROPEAN_NUMBER_SEPARATOR,
    EuropeanNumberTerminator = ICU::U_EUROPEAN_NUMBER_TERMINATOR,
    ArabicNumber = ICU::U_ARABIC_NUMBER,
    CommonNumberSeparator = ICU::U_COMMON_NUMBER_SEPARATOR,
    BlockSeparator = ICU::U_BLOCK_SEPARATOR,
    SegmentSeparator = ICU::U_SEGMENT_SEPARATOR,
    WhiteSpaceNeutral = ICU::U_WHITE_SPACE_NEUTRAL,
    OtherNeutral = ICU::U_OTHER_NEUTRAL,
    LeftToRightEmbedding = ICU::U_LEFT_TO_RIGHT_EMBEDDING,
    LeftToRightOverride = ICU::U_LEFT_TO_RIGHT_OVERRIDE,
    RightToLeftArabic = ICU::U_RIGHT_TO_LEFT_ARABIC,
    RightToLeftEmbedding = ICU::U_RIGHT_TO_LEFT_EMBEDDING,
    RightToLeftOverride = ICU::U_RIGHT_TO_LEFT_OVERRIDE,
    PopDirectionalFormat = ICU::U_POP_DIRECTIONAL_FORMAT,
    NonSpacingMark = ICU::U_DIR_NON_SPACING_MARK,
    BoundaryNeutral = ICU::U_BOUNDARY_NEUTRAL
};

enum DecompositionType {
    DecompositionNone = ICU::U_DT_NONE,
    DecompositionCanonical = ICU::U_DT_CANONICAL,
    DecompositionCompat = ICU::U_DT_COMPAT,
    DecompositionCircle = ICU::U_DT_CIRCLE,
    DecompositionFinal = ICU::U_DT_FINAL,
    DecompositionFont = ICU::U_DT_FONT,
    DecompositionFraction = ICU::U_DT_FRACTION,
    DecompositionInitial = ICU::U_DT_INITIAL,
    DecompositionIsolated = ICU::U_DT_ISOLATED,
    DecompositionMedial = ICU::U_DT_MEDIAL,
    DecompositionNarrow = ICU::U_DT_NARROW,
    DecompositionNoBreak = ICU::U_DT_NOBREAK,
    DecompositionSmall = ICU::U_DT_SMALL,
    DecompositionSquare = ICU::U_DT_SQUARE,
    DecompositionSub = ICU::U_DT_SUB,
    DecompositionSuper = ICU::U_DT_SUPER,
    DecompositionVertical = ICU::U_DT_VERTICAL,
    DecompositionWide = ICU::U_DT_WIDE,
};

enum CharCategory {
    NoCategory =  0,
    Other_NotAssigned = TO_MASK(ICU::U_GENERAL_OTHER_TYPES),
    Letter_Uppercase = TO_MASK(ICU::U_UPPERCASE_LETTER),
    Letter_Lowercase = TO_MASK(ICU::U_LOWERCASE_LETTER),
    Letter_Titlecase = TO_MASK(ICU::U_TITLECASE_LETTER),
    Letter_Modifier = TO_MASK(ICU::U_MODIFIER_LETTER),
    Letter_Other = TO_MASK(ICU::U_OTHER_LETTER),

    Mark_NonSpacing = TO_MASK(ICU::U_NON_SPACING_MARK),
    Mark_Enclosing = TO_MASK(ICU::U_ENCLOSING_MARK),
    Mark_SpacingCombining = TO_MASK(ICU::U_COMBINING_SPACING_MARK),

    Number_DecimalDigit = TO_MASK(ICU::U_DECIMAL_DIGIT_NUMBER),
    Number_Letter = TO_MASK(ICU::U_LETTER_NUMBER),
    Number_Other = TO_MASK(ICU::U_OTHER_NUMBER),

    Separator_Space = TO_MASK(ICU::U_SPACE_SEPARATOR),
    Separator_Line = TO_MASK(ICU::U_LINE_SEPARATOR),
    Separator_Paragraph = TO_MASK(ICU::U_PARAGRAPH_SEPARATOR),

    Other_Control = TO_MASK(ICU::U_CONTROL_CHAR),
    Other_Format = TO_MASK(ICU::U_FORMAT_CHAR),
    Other_PrivateUse = TO_MASK(ICU::U_PRIVATE_USE_CHAR),
    Other_Surrogate = TO_MASK(ICU::U_SURROGATE),

    Punctuation_Dash = TO_MASK(ICU::U_DASH_PUNCTUATION),
    Punctuation_Open = TO_MASK(ICU::U_START_PUNCTUATION),
    Punctuation_Close = TO_MASK(ICU::U_END_PUNCTUATION),
    Punctuation_Connector = TO_MASK(ICU::U_CONNECTOR_PUNCTUATION),
    Punctuation_Other = TO_MASK(ICU::U_OTHER_PUNCTUATION),

    Symbol_Math = TO_MASK(ICU::U_MATH_SYMBOL),
    Symbol_Currency = TO_MASK(ICU::U_CURRENCY_SYMBOL),
    Symbol_Modifier = TO_MASK(ICU::U_MODIFIER_SYMBOL),
    Symbol_Other = TO_MASK(ICU::U_OTHER_SYMBOL),

    Punctuation_InitialQuote = TO_MASK(ICU::U_INITIAL_PUNCTUATION),
    Punctuation_FinalQuote = TO_MASK(ICU::U_FINAL_PUNCTUATION)
};

UChar foldCase(UChar);

int foldCase(UChar* result, int resultLength, const UChar* source, int sourceLength, bool* isError);

int toLower(UChar* result, int resultLength, const UChar* source, int sourceLength, bool* isError);

UChar toUpper(UChar);
UChar toLower(UChar);

bool isUpper(UChar);

int toUpper(UChar* result, int resultLength, const UChar* source, int sourceLength, bool* isError);

UChar toTitleCase(UChar);

inline bool isArabicChar(UChar32 c)
{
    return c >= 0x0600 && c <= 0x06FF;
}

bool isAlphanumeric(UChar);

CharCategory category(unsigned int);

inline bool isSeparatorSpace(UChar c)
{
    return category(c) == Separator_Space;
}

bool isPrintableChar(UChar);

bool isDigit(UChar);

bool isPunct(UChar);

inline bool hasLineBreakingPropertyComplexContext(UChar32)
{
    // FIXME: implement!
    return false;
}

inline bool hasLineBreakingPropertyComplexContextOrIdeographic(UChar32 c)
{
    // FIXME
    return false;
}

UChar mirroredChar(UChar32);

Direction direction(UChar32);

bool isLower(UChar);

int digitValue(UChar);

unsigned char combiningClass(UChar32);

DecompositionType decompositionType(UChar32);

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

bool isSpace(UChar);
bool isLetter(UChar);

} // namespace Unicode
} // namespace WTF

#endif
