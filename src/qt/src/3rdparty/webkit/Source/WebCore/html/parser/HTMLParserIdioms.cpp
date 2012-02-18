/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "HTMLParserIdioms.h"

#include <limits>
#include <wtf/MathExtras.h>
#include <wtf/dtoa.h>
#include <wtf/text/AtomicString.h>

namespace WebCore {

String stripLeadingAndTrailingHTMLSpaces(const String& string)
{
    const UChar* characters = string.characters();
    unsigned length = string.length();

    unsigned numLeadingSpaces;
    for (numLeadingSpaces = 0; numLeadingSpaces < length; ++numLeadingSpaces) {
        if (isNotHTMLSpace(characters[numLeadingSpaces]))
            break;
    }

    if (numLeadingSpaces == length)
        return string.isNull() ? string : emptyAtom.string();

    unsigned numTrailingSpaces;
    for (numTrailingSpaces = 0; numTrailingSpaces < length; ++numTrailingSpaces) {
        if (isNotHTMLSpace(characters[length - numTrailingSpaces - 1]))
            break;
    }

    ASSERT(numLeadingSpaces + numTrailingSpaces < length);

    return string.substring(numLeadingSpaces, length - (numLeadingSpaces + numTrailingSpaces));
}

String serializeForNumberType(double number)
{
    // According to HTML5, "the best representation of the number n as a floating
    // point number" is a string produced by applying ToString() to n.
    NumberToStringBuffer buffer;
    unsigned length = numberToString(number, buffer);
    return String(buffer, length);
}

bool parseToDoubleForNumberType(const String& string, double* result)
{
    // See HTML5 2.4.4.3 `Real numbers.'

    // String::toDouble() accepts leading + and whitespace characters, which are not valid here.
    UChar firstCharacter = string[0];
    if (firstCharacter != '-' && !isASCIIDigit(firstCharacter))
        return false;

    bool valid = false;
    double value = string.toDouble(&valid);
    if (!valid)
        return false;

    // NaN and infinity are considered valid by String::toDouble, but not valid here.
    if (!isfinite(value))
        return false;

    // Numbers are considered finite IEEE 754 single-precision floating point values.
    // See HTML5 2.4.4.3 `Real numbers.'
    if (-std::numeric_limits<float>::max() > value || value > std::numeric_limits<float>::max())
        return false;

    if (result) {
        // The following expression converts -0 to +0.
        *result = value ? value : 0;
    }

    return true;
}

bool parseToDoubleForNumberTypeWithDecimalPlaces(const String& string, double *result, unsigned *decimalPlaces)
{
    if (decimalPlaces)
        *decimalPlaces = 0;

    if (!parseToDoubleForNumberType(string, result))
        return false;

    if (!decimalPlaces)
        return true;

    size_t dotIndex = string.find('.');
    size_t eIndex = string.find('e');
    if (eIndex == notFound) 
        eIndex = string.find('E');

    unsigned baseDecimalPlaces = 0;
    if (dotIndex != notFound) {
        if (eIndex == notFound)
            baseDecimalPlaces = string.length() - dotIndex - 1;
        else
            baseDecimalPlaces = eIndex - dotIndex - 1;
    }

    int exponent = 0;
    if (eIndex != notFound) {
        unsigned cursor = eIndex + 1, cursorSaved;
        int digit, exponentSign;
        int32_t exponent32;
        size_t length = string.length();

        // Not using String.toInt() in order to perform the same computation as dtoa() does.
        exponentSign = 0;
        switch (digit = string[cursor]) {
        case '-':
            exponentSign = 1;
        case '+':
            digit = string[++cursor];
        }
        if (digit >= '0' && digit <= '9') {
            while (cursor < length && digit == '0')
                digit = string[++cursor];
            if (digit > '0' && digit <= '9') {
                exponent32 = digit - '0';
                cursorSaved = cursor;
                while (cursor < length && (digit = string[++cursor]) >= '0' && digit <= '9')
                    exponent32 = (10 * exponent32) + digit - '0';
                if (cursor - cursorSaved > 8 || exponent32 > 19999)
                    /* Avoid confusion from exponents
                     * so large that e might overflow.
                     */
                    exponent = 19999; /* safe for 16 bit ints */
                else
                    exponent = static_cast<int>(exponent32);
                if (exponentSign)
                    exponent = -exponent;
            } else
                exponent = 0;
        }
    }

    int intDecimalPlaces = baseDecimalPlaces - exponent;
    if (intDecimalPlaces < 0)
        *decimalPlaces = 0;
    else if (intDecimalPlaces > 19999)
        *decimalPlaces = 19999;
    else
        *decimalPlaces = static_cast<unsigned>(intDecimalPlaces);

    return true;
}

// http://www.whatwg.org/specs/web-apps/current-work/#rules-for-parsing-integers
bool parseHTMLInteger(const String& input, int& value)
{
    // Step 1
    // Step 2
    const UChar* position = input.characters();
    const UChar* end = position + input.length();

    // Step 3
    int sign = 1;

    // Step 4
    while (position < end) {
        if (!isHTMLSpace(*position))
            break;
        ++position;
    }

    // Step 5
    if (position == end)
        return false;
    ASSERT(position < end);

    // Step 6
    if (*position == '-') {
        sign = -1;
        ++position;
    } else if (*position == '+')
        ++position;
    if (position == end)
        return false;
    ASSERT(position < end);

    // Step 7
    if (!isASCIIDigit(*position))
        return false;

    // Step 8
    Vector<UChar, 16> digits;
    while (position < end) {
        if (!isASCIIDigit(*position))
            break;
        digits.append(*position++);
    }

    // Step 9
    value = sign * charactersToIntStrict(digits.data(), digits.size());
    return true;
}

}
