/*
 * Copyright (C) 2002, 2003 The Karbon Developers
 * Copyright (C) 2006 Alexander Kellett <lypanov@kde.org>
 * Copyright (C) 2006, 2007 Rob Buis <buis@kde.org>
 * Copyright (C) 2007, 2009 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"

#if ENABLE(SVG)
#include "SVGParserUtilities.h"

#include "Document.h"
#include "FloatPoint.h"
#include "SVGPointList.h"

#include <limits>
#include <wtf/ASCIICType.h>

namespace WebCore {

template <typename FloatType> static inline bool isValidRange(const FloatType& x)
{
    static const FloatType max = std::numeric_limits<FloatType>::max();
    return x >= -max && x <= max;
}

// We use this generic parseNumber function to allow the Path parsing code to work 
// at a higher precision internally, without any unnecessary runtime cost or code
// complexity.
template <typename FloatType> static bool genericParseNumber(const UChar*& ptr, const UChar* end, FloatType& number, bool skip)
{
    FloatType integer, decimal, frac, exponent;
    int sign, expsign;
    const UChar* start = ptr;

    exponent = 0;
    integer = 0;
    frac = 1;
    decimal = 0;
    sign = 1;
    expsign = 1;

    // read the sign
    if (ptr < end && *ptr == '+')
        ptr++;
    else if (ptr < end && *ptr == '-') {
        ptr++;
        sign = -1;
    } 
    
    if (ptr == end || ((*ptr < '0' || *ptr > '9') && *ptr != '.'))
        // The first character of a number must be one of [0-9+-.]
        return false;

    // read the integer part, build right-to-left
    const UChar* ptrStartIntPart = ptr;
    while (ptr < end && *ptr >= '0' && *ptr <= '9')
        ++ptr; // Advance to first non-digit.

    if (ptr != ptrStartIntPart) {
        const UChar* ptrScanIntPart = ptr - 1;
        FloatType multiplier = 1;
        while (ptrScanIntPart >= ptrStartIntPart) {
            integer += multiplier * static_cast<FloatType>(*(ptrScanIntPart--) - '0');
            multiplier *= 10;
        }
        // Bail out early if this overflows.
        if (!isValidRange(integer))
            return false;
    }

    if (ptr < end && *ptr == '.') { // read the decimals
        ptr++;
        
        // There must be a least one digit following the .
        if (ptr >= end || *ptr < '0' || *ptr > '9')
            return false;
        
        while (ptr < end && *ptr >= '0' && *ptr <= '9')
            decimal += (*(ptr++) - '0') * (frac *= static_cast<FloatType>(0.1));
    }

    // read the exponent part
    if (ptr != start && ptr + 1 < end && (*ptr == 'e' || *ptr == 'E') 
        && (ptr[1] != 'x' && ptr[1] != 'm')) { 
        ptr++;

        // read the sign of the exponent
        if (*ptr == '+')
            ptr++;
        else if (*ptr == '-') {
            ptr++;
            expsign = -1;
        }
        
        // There must be an exponent
        if (ptr >= end || *ptr < '0' || *ptr > '9')
            return false;

        while (ptr < end && *ptr >= '0' && *ptr <= '9') {
            exponent *= static_cast<FloatType>(10);
            exponent += *ptr - '0';
            ptr++;
        }
        // Make sure exponent is valid.
        if (!isValidRange(exponent) || exponent > std::numeric_limits<FloatType>::max_exponent)
            return false;
    }

    number = integer + decimal;
    number *= sign;

    if (exponent)
        number *= static_cast<FloatType>(pow(10.0, expsign * static_cast<int>(exponent)));

    // Don't return Infinity() or NaN().
    if (!isValidRange(number))
        return false;

    if (start == ptr)
        return false;

    if (skip)
        skipOptionalSpacesOrDelimiter(ptr, end);

    return true;
}

bool parseNumber(const UChar*& ptr, const UChar* end, float& number, bool skip) 
{
    return genericParseNumber(ptr, end, number, skip);
}

// only used to parse largeArcFlag and sweepFlag which must be a "0" or "1"
// and might not have any whitespace/comma after it
bool parseArcFlag(const UChar*& ptr, const UChar* end, bool& flag)
{
    const UChar flagChar = *ptr++;
    if (flagChar == '0')
        flag = false;
    else if (flagChar == '1')
        flag = true;
    else
        return false;
    
    skipOptionalSpacesOrDelimiter(ptr, end);
    
    return true;
}

bool parseNumberOptionalNumber(const String& s, float& x, float& y)
{
    if (s.isEmpty())
        return false;
    const UChar* cur = s.characters();
    const UChar* end = cur + s.length();

    if (!parseNumber(cur, end, x))
        return false;

    if (cur == end)
        y = x;
    else if (!parseNumber(cur, end, y, false))
        return false;

    return cur == end;
}

bool pointsListFromSVGData(SVGPointList& pointsList, const String& points)
{
    if (points.isEmpty())
        return true;
    const UChar* cur = points.characters();
    const UChar* end = cur + points.length();

    skipOptionalSpaces(cur, end);

    bool delimParsed = false;
    while (cur < end) {
        delimParsed = false;
        float xPos = 0.0f;
        if (!parseNumber(cur, end, xPos))
           return false;

        float yPos = 0.0f;
        if (!parseNumber(cur, end, yPos, false))
            return false;

        skipOptionalSpaces(cur, end);

        if (cur < end && *cur == ',') {
            delimParsed = true;
            cur++;
        }
        skipOptionalSpaces(cur, end);

        pointsList.append(FloatPoint(xPos, yPos));
    }
    return cur == end && !delimParsed;
}

bool parseGlyphName(const String& input, HashSet<String>& values)
{
    // FIXME: Parsing error detection is missing.
    values.clear();

    const UChar* ptr = input.characters();
    const UChar* end = ptr + input.length();
    skipOptionalSpaces(ptr, end);

    while (ptr < end) {
        // Leading and trailing white space, and white space before and after separators, will be ignored.
        const UChar* inputStart = ptr;
        while (ptr < end && *ptr != ',')
            ++ptr;

        if (ptr == inputStart)
            break;

        // walk backwards from the ; to ignore any whitespace
        const UChar* inputEnd = ptr - 1;
        while (inputStart < inputEnd && isWhitespace(*inputEnd))
            --inputEnd;

        values.add(String(inputStart, inputEnd - inputStart + 1));
        skipOptionalSpacesOrDelimiter(ptr, end, ',');
    }

    return true;
}

static bool parseUnicodeRange(const UChar* characters, unsigned length, UnicodeRange& range)
{
    if (length < 2 || characters[0] != 'U' || characters[1] != '+')
        return false;
    
    // Parse the starting hex number (or its prefix).
    unsigned startRange = 0;
    unsigned startLength = 0;

    const UChar* ptr = characters + 2;
    const UChar* end = characters + length;
    while (ptr < end) {
        if (!isASCIIHexDigit(*ptr))
            break;
        ++startLength;
        if (startLength > 6)
            return false;
        startRange = (startRange << 4) | toASCIIHexValue(*ptr);
        ++ptr;
    }
    
    // Handle the case of ranges separated by "-" sign.
    if (2 + startLength < length && *ptr == '-') {
        if (!startLength)
            return false;
        
        // Parse the ending hex number (or its prefix).
        unsigned endRange = 0;
        unsigned endLength = 0;
        ++ptr;
        while (ptr < end) {
            if (!isASCIIHexDigit(*ptr))
                break;
            ++endLength;
            if (endLength > 6)
                return false;
            endRange = (endRange << 4) | toASCIIHexValue(*ptr);
            ++ptr;
        }
        
        if (!endLength)
            return false;
        
        range.first = startRange;
        range.second = endRange;
        return true;
    }
    
    // Handle the case of a number with some optional trailing question marks.
    unsigned endRange = startRange;
    while (ptr < end) {
        if (*ptr != '?')
            break;
        ++startLength;
        if (startLength > 6)
            return false;
        startRange <<= 4;
        endRange = (endRange << 4) | 0xF;
        ++ptr;
    }
    
    if (!startLength)
        return false;
    
    range.first = startRange;
    range.second = endRange;
    return true;
}

bool parseKerningUnicodeString(const String& input, UnicodeRanges& rangeList, HashSet<String>& stringList)
{
    // FIXME: Parsing error detection is missing.
    const UChar* ptr = input.characters();
    const UChar* end = ptr + input.length();

    while (ptr < end) {
        const UChar* inputStart = ptr;
        while (ptr < end && *ptr != ',')
            ++ptr;

        if (ptr == inputStart)
            break;

        // Try to parse unicode range first
        UnicodeRange range;
        if (parseUnicodeRange(inputStart, ptr - inputStart, range))
            rangeList.append(range);
        else
            stringList.add(String(inputStart, ptr - inputStart));
        ++ptr;
    }

    return true;
}

Vector<String> parseDelimitedString(const String& input, const char seperator)
{
    Vector<String> values;

    const UChar* ptr = input.characters();
    const UChar* end = ptr + input.length();
    skipOptionalSpaces(ptr, end);

    while (ptr < end) {
        // Leading and trailing white space, and white space before and after semicolon separators, will be ignored.
        const UChar* inputStart = ptr;
        while (ptr < end && *ptr != seperator) // careful not to ignore whitespace inside inputs
            ptr++;

        if (ptr == inputStart)
            break;

        // walk backwards from the ; to ignore any whitespace
        const UChar* inputEnd = ptr - 1;
        while (inputStart < inputEnd && isWhitespace(*inputEnd))
            inputEnd--;

        values.append(String(inputStart, inputEnd - inputStart + 1));
        skipOptionalSpacesOrDelimiter(ptr, end, seperator);
    }

    return values;
}

}

#endif // ENABLE(SVG)
