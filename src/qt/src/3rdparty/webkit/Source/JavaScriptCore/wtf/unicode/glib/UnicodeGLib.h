/*
 *  Copyright (C) 2006 George Staikos <staikos@kde.org>
 *  Copyright (C) 2006 Alexey Proskuryakov <ap@nypop.com>
 *  Copyright (C) 2007 Apple Computer, Inc. All rights reserved.
 *  Copyright (C) 2008 Jürg Billeter <j@bitron.ch>
 *  Copyright (C) 2008 Dominik Röttsches <dominik.roettsches@access-company.com>
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

#ifndef UnicodeGLib_h
#define UnicodeGLib_h

#include "UnicodeMacrosFromICU.h"
#include "GOwnPtr.h"

#include <glib.h>
#include <pango/pango.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef uint16_t UChar;
typedef int32_t UChar32;

namespace WTF {
namespace Unicode {

enum Direction {
    LeftToRight,
    RightToLeft,
    EuropeanNumber,
    EuropeanNumberSeparator,
    EuropeanNumberTerminator,
    ArabicNumber,
    CommonNumberSeparator,
    BlockSeparator,
    SegmentSeparator,
    WhiteSpaceNeutral,
    OtherNeutral,
    LeftToRightEmbedding,
    LeftToRightOverride,
    RightToLeftArabic,
    RightToLeftEmbedding,
    RightToLeftOverride,
    PopDirectionalFormat,
    NonSpacingMark,
    BoundaryNeutral
};

enum DecompositionType {
    DecompositionNone,
    DecompositionCanonical,
    DecompositionCompat,
    DecompositionCircle,
    DecompositionFinal,
    DecompositionFont,
    DecompositionFraction,
    DecompositionInitial,
    DecompositionIsolated,
    DecompositionMedial,
    DecompositionNarrow,
    DecompositionNoBreak,
    DecompositionSmall,
    DecompositionSquare,
    DecompositionSub,
    DecompositionSuper,
    DecompositionVertical,
    DecompositionWide,
};

enum CharCategory {
    NoCategory =  0,
    Other_NotAssigned = U_MASK(G_UNICODE_UNASSIGNED),
    Letter_Uppercase = U_MASK(G_UNICODE_UPPERCASE_LETTER),
    Letter_Lowercase = U_MASK(G_UNICODE_LOWERCASE_LETTER),
    Letter_Titlecase = U_MASK(G_UNICODE_TITLECASE_LETTER),
    Letter_Modifier = U_MASK(G_UNICODE_MODIFIER_LETTER),
    Letter_Other = U_MASK(G_UNICODE_OTHER_LETTER),

    Mark_NonSpacing = U_MASK(G_UNICODE_NON_SPACING_MARK),
    Mark_Enclosing = U_MASK(G_UNICODE_ENCLOSING_MARK),
    Mark_SpacingCombining = U_MASK(G_UNICODE_COMBINING_MARK),

    Number_DecimalDigit = U_MASK(G_UNICODE_DECIMAL_NUMBER),
    Number_Letter = U_MASK(G_UNICODE_LETTER_NUMBER),
    Number_Other = U_MASK(G_UNICODE_OTHER_NUMBER),

    Separator_Space = U_MASK(G_UNICODE_SPACE_SEPARATOR),
    Separator_Line = U_MASK(G_UNICODE_LINE_SEPARATOR),
    Separator_Paragraph = U_MASK(G_UNICODE_PARAGRAPH_SEPARATOR),

    Other_Control = U_MASK(G_UNICODE_CONTROL),
    Other_Format = U_MASK(G_UNICODE_FORMAT),
    Other_PrivateUse = U_MASK(G_UNICODE_PRIVATE_USE),
    Other_Surrogate = U_MASK(G_UNICODE_SURROGATE),

    Punctuation_Dash = U_MASK(G_UNICODE_DASH_PUNCTUATION),
    Punctuation_Open = U_MASK(G_UNICODE_OPEN_PUNCTUATION),
    Punctuation_Close = U_MASK(G_UNICODE_CLOSE_PUNCTUATION),
    Punctuation_Connector = U_MASK(G_UNICODE_CONNECT_PUNCTUATION),
    Punctuation_Other = U_MASK(G_UNICODE_OTHER_PUNCTUATION),

    Symbol_Math = U_MASK(G_UNICODE_MATH_SYMBOL),
    Symbol_Currency = U_MASK(G_UNICODE_CURRENCY_SYMBOL),
    Symbol_Modifier = U_MASK(G_UNICODE_MODIFIER_SYMBOL),
    Symbol_Other = U_MASK(G_UNICODE_OTHER_SYMBOL),

    Punctuation_InitialQuote = U_MASK(G_UNICODE_INITIAL_PUNCTUATION),
    Punctuation_FinalQuote = U_MASK(G_UNICODE_FINAL_PUNCTUATION)
};

UChar32 foldCase(UChar32);

int foldCase(UChar* result, int resultLength, const UChar* src, int srcLength, bool* error);

int toLower(UChar* result, int resultLength, const UChar* src, int srcLength, bool* error);

inline UChar32 toLower(UChar32 c)
{
    return g_unichar_tolower(c);
}

inline UChar32 toUpper(UChar32 c)
{
    return g_unichar_toupper(c);
}

int toUpper(UChar* result, int resultLength, const UChar* src, int srcLength, bool* error);

inline UChar32 toTitleCase(UChar32 c)
{
    return g_unichar_totitle(c);
}

inline bool isArabicChar(UChar32 c)
{
    return c >= 0x0600 && c <= 0x06FF;
}

inline bool isAlphanumeric(UChar32 c)
{
    return g_unichar_isalnum(c);
}

inline bool isFormatChar(UChar32 c)
{
    return g_unichar_type(c) == G_UNICODE_FORMAT;
}

inline bool isSeparatorSpace(UChar32 c)
{
    return g_unichar_type(c) == G_UNICODE_SPACE_SEPARATOR;
}

inline bool isPrintableChar(UChar32 c)
{
    return g_unichar_isprint(c);
}

inline bool isDigit(UChar32 c)
{
    return g_unichar_isdigit(c);
}

inline bool isPunct(UChar32 c)
{
    return g_unichar_ispunct(c);
}

inline bool hasLineBreakingPropertyComplexContext(UChar32 c)
{
    // FIXME
    return false;
}

inline bool hasLineBreakingPropertyComplexContextOrIdeographic(UChar32 c)
{
    // FIXME
    return false;
}

inline UChar32 mirroredChar(UChar32 c)
{
    gunichar mirror = 0;
    g_unichar_get_mirror_char(c, &mirror);
    return mirror;
}

inline CharCategory category(UChar32 c)
{
    if (c > 0xffff)
        return NoCategory;

    return (CharCategory) U_MASK(g_unichar_type(c));
}

Direction direction(UChar32);

inline bool isLower(UChar32 c)
{
    return g_unichar_islower(c);
}

inline int digitValue(UChar32 c)
{
    return g_unichar_digit_value(c);
}

inline uint8_t combiningClass(UChar32 c)
{
    // FIXME
    // return g_unichar_combining_class(c);
    return 0;
}

inline DecompositionType decompositionType(UChar32 c)
{
    // FIXME
    return DecompositionNone;
}

int umemcasecmp(const UChar*, const UChar*, int len);

}
}

#endif

