/*
 *  Copyright (C) 2006 George Staikos <staikos@kde.org>
 *  Copyright (C) 2006 Alexey Proskuryakov <ap@nypop.com>
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
 */

#include "config.h"
#include "UnicodeBrew.h"

#include <wchar.h>
#include <wctype.h>

namespace WTF {
namespace Unicode {

UChar toLower(UChar c)
{
    return towlower(c);
}

UChar toUpper(UChar c)
{
    return towupper(c);
}

UChar foldCase(UChar c)
{
    return towlower(c);
}

bool isPrintableChar(UChar c)
{
    return !!iswprint(c);
}

bool isUpper(UChar c)
{
    return !!iswupper(c);
}

bool isLower(UChar c)
{
    return !!iswlower(c);
}

bool isDigit(UChar c)
{
    return !!iswdigit(c);
}

bool isPunct(UChar c)
{
    return !!iswpunct(c);
}

bool isAlphanumeric(UChar c)
{
    return !!iswalnum(c);
}

int toLower(UChar* result, int resultLength, const UChar* source, int sourceLength, bool* isError)
{
    const UChar* sourceIterator = source;
    const UChar* sourceEnd = source + sourceLength;
    UChar* resultIterator = result;
    UChar* resultEnd = result + resultLength;

    if (sourceLength <= resultLength) {
        while (sourceIterator < sourceEnd)
            *resultIterator++ = towlower(*sourceIterator++);
    } else {
        while (resultIterator < resultEnd)
            *resultIterator++ = towlower(*sourceIterator++);
    }

    int remainingCharacters = sourceIterator < sourceEnd ? sourceEnd - sourceIterator : 0;
    *isError = !!remainingCharacters;
    if (resultIterator < resultEnd)
        *resultIterator = 0;

    return (resultIterator - result) + remainingCharacters;
}

int toUpper(UChar* result, int resultLength, const UChar* source, int sourceLength, bool* isError)
{
    const UChar* sourceIterator = source;
    const UChar* sourceEnd = source + sourceLength;
    UChar* resultIterator = result;
    UChar* resultEnd = result + resultLength;

    if (sourceLength <= resultLength) {
        while (sourceIterator < sourceEnd)
            *resultIterator++ = towupper(*sourceIterator++);
    } else {
        while (resultIterator < resultEnd)
            *resultIterator++ = towupper(*sourceIterator++);
    }

    int remainingCharacters = sourceIterator < sourceEnd ? sourceEnd - sourceIterator : 0;
    *isError = !!remainingCharacters;
    if (resultIterator < resultEnd)
        *resultIterator = 0;

    return (resultIterator - result) + remainingCharacters;
}

int foldCase(UChar* result, int resultLength, const UChar* source, int sourceLength, bool* isError)
{
    *isError = false;
    if (resultLength < sourceLength) {
        *isError = true;
        return sourceLength;
    }
    for (int i = 0; i < sourceLength; ++i)
        result[i] = foldCase(source[i]);
    return sourceLength;
}

UChar toTitleCase(UChar c)
{
    return towupper(c);
}

Direction direction(UChar32 c)
{
    return static_cast<Direction>(ICU::direction(c));
}

CharCategory category(unsigned int c)
{
    return static_cast<CharCategory>(TO_MASK((int8_t) ICU::category(c)));
}

DecompositionType decompositionType(UChar32 c)
{
    return static_cast<DecompositionType>(ICU::decompositionType(c));
}

unsigned char combiningClass(UChar32 c)
{
    return ICU::combiningClass(c);
}

UChar mirroredChar(UChar32 c)
{
    return ICU::mirroredChar(c);
}

int digitValue(UChar c)
{
    return ICU::digitValue(c);
}

bool isSpace(UChar c)
{
    return !!iswspace(c);
}

bool isLetter(UChar c)
{
    return !!iswalpha(c);
}

} // namespace Unicode
} // namespace WTF
