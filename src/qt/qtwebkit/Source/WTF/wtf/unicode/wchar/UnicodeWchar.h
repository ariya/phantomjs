/*
 * Copyright (C) 2012 Patrick Gansterer <paroga@paroga.com>
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
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WTF_UnicodeWchar_h
#define WTF_UnicodeWchar_h

#include <stdint.h>
#include <wchar.h>
#include <wtf/unicode/ScriptCodesFromICU.h>
#include <wtf/unicode/UnicodeMacrosFromICU.h>

typedef wchar_t UChar;
typedef uint32_t UChar32;

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
    DecompositionWide
};

enum CharCategory {
    NoCategory = 0,
    Other_NotAssigned = U_MASK(0),
    Letter_Uppercase = U_MASK(1),
    Letter_Lowercase = U_MASK(2),
    Letter_Titlecase = U_MASK(3),
    Letter_Modifier = U_MASK(4),
    Letter_Other = U_MASK(5),

    Mark_NonSpacing = U_MASK(6),
    Mark_Enclosing = U_MASK(7),
    Mark_SpacingCombining = U_MASK(8),

    Number_DecimalDigit = U_MASK(9),
    Number_Letter = U_MASK(10),
    Number_Other = U_MASK(11),

    Separator_Space = U_MASK(12),
    Separator_Line = U_MASK(13),
    Separator_Paragraph = U_MASK(14),

    Other_Control = U_MASK(15),
    Other_Format = U_MASK(16),
    Other_PrivateUse = U_MASK(17),
    Other_Surrogate = U_MASK(18),

    Punctuation_Dash = U_MASK(19),
    Punctuation_Open = U_MASK(20),
    Punctuation_Close = U_MASK(21),
    Punctuation_Connector = U_MASK(22),
    Punctuation_Other = U_MASK(23),

    Symbol_Math = U_MASK(24),
    Symbol_Currency = U_MASK(25),
    Symbol_Modifier = U_MASK(26),
    Symbol_Other = U_MASK(27),

    Punctuation_InitialQuote = U_MASK(28),
    Punctuation_FinalQuote = U_MASK(29)
};


WTF_EXPORT_PRIVATE CharCategory category(UChar32);
WTF_EXPORT_PRIVATE unsigned char combiningClass(UChar32);
WTF_EXPORT_PRIVATE Direction direction(UChar32);
WTF_EXPORT_PRIVATE DecompositionType decompositionType(UChar32);
WTF_EXPORT_PRIVATE bool hasLineBreakingPropertyComplexContext(UChar32);
WTF_EXPORT_PRIVATE UChar32 mirroredChar(UChar32);

inline bool isAlphanumeric(UChar c) { return !!iswalnum(c); }
inline bool isDigit(UChar c) { return !!iswdigit(c); }
inline bool isLetter(UChar c) { return !!iswalpha(c); }
inline bool isLower(UChar c) { return !!iswlower(c); }
inline bool isPrintableChar(UChar c) { return !!iswprint(c); }
inline bool isPunct(UChar c) { return !!iswpunct(c); }
inline bool isSpace(UChar c) { return !!iswspace(c); }
inline bool isUpper(UChar c) { return !!iswupper(c); }

inline bool isArabicChar(UChar32 c) { return c >= 0x0600 && c <= 0x06ff; }
inline bool isSeparatorSpace(UChar32 c) { return category(c) == Separator_Space; }

inline UChar foldCase(UChar c) { return towlower(c); }
inline UChar toLower(UChar c) { return towlower(c); }
inline UChar toUpper(UChar c) { return towupper(c); }
inline UChar toTitleCase(UChar c) { return towupper(c); }

WTF_EXPORT_PRIVATE int foldCase(UChar* result, int resultLength, const UChar* source, int sourceLength, bool* isError);
WTF_EXPORT_PRIVATE int toLower(UChar* result, int resultLength, const UChar* source, int sourceLength, bool* isError);
WTF_EXPORT_PRIVATE int toUpper(UChar* result, int resultLength, const UChar* source, int sourceLength, bool* isError);

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

} // namespace Unicode
} // namespace WTF

#endif // WTF_UnicodeWchar_h
