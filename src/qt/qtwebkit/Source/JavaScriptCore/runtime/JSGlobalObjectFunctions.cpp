/*
 *  Copyright (C) 1999-2002 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2012 Apple Inc. All rights reserved.
 *  Copyright (C) 2007 Cameron Zwarich (cwzwarich@uwaterloo.ca)
 *  Copyright (C) 2007 Maks Orlovich
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
#include "JSGlobalObjectFunctions.h"

#include "CallFrame.h"
#include "Interpreter.h"
#include "JSFunction.h"
#include "JSGlobalObject.h"
#include "JSString.h"
#include "JSStringBuilder.h"
#include "Lexer.h"
#include "LiteralParser.h"
#include "Nodes.h"
#include "Operations.h"
#include "Parser.h"
#include <wtf/dtoa.h>
#include <stdio.h>
#include <stdlib.h>
#include <wtf/ASCIICType.h>
#include <wtf/Assertions.h>
#include <wtf/MathExtras.h>
#include <wtf/StringExtras.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/unicode/UTF8.h>

using namespace WTF;
using namespace Unicode;

namespace JSC {

static JSValue encode(ExecState* exec, const char* doNotEscape)
{
    CString cstr = exec->argument(0).toString(exec)->value(exec).utf8(String::StrictConversion);
    if (!cstr.data())
        return throwError(exec, createURIError(exec, ASCIILiteral("String contained an illegal UTF-16 sequence.")));

    JSStringBuilder builder;
    const char* p = cstr.data();
    for (size_t k = 0; k < cstr.length(); k++, p++) {
        char c = *p;
        if (c && strchr(doNotEscape, c))
            builder.append(c);
        else {
            char tmp[4];
            snprintf(tmp, sizeof(tmp), "%%%02X", static_cast<unsigned char>(c));
            builder.append(tmp);
        }
    }
    return builder.build(exec);
}

template <typename CharType>
ALWAYS_INLINE
static JSValue decode(ExecState* exec, const CharType* characters, int length, const char* doNotUnescape, bool strict)
{
    JSStringBuilder builder;
    int k = 0;
    UChar u = 0;
    while (k < length) {
        const CharType* p = characters + k;
        CharType c = *p;
        if (c == '%') {
            int charLen = 0;
            if (k <= length - 3 && isASCIIHexDigit(p[1]) && isASCIIHexDigit(p[2])) {
                const char b0 = Lexer<CharType>::convertHex(p[1], p[2]);
                const int sequenceLen = UTF8SequenceLength(b0);
                if (sequenceLen && k <= length - sequenceLen * 3) {
                    charLen = sequenceLen * 3;
                    char sequence[5];
                    sequence[0] = b0;
                    for (int i = 1; i < sequenceLen; ++i) {
                        const CharType* q = p + i * 3;
                        if (q[0] == '%' && isASCIIHexDigit(q[1]) && isASCIIHexDigit(q[2]))
                            sequence[i] = Lexer<CharType>::convertHex(q[1], q[2]);
                        else {
                            charLen = 0;
                            break;
                        }
                    }
                    if (charLen != 0) {
                        sequence[sequenceLen] = 0;
                        const int character = decodeUTF8Sequence(sequence);
                        if (character < 0 || character >= 0x110000)
                            charLen = 0;
                        else if (character >= 0x10000) {
                            // Convert to surrogate pair.
                            builder.append(static_cast<UChar>(0xD800 | ((character - 0x10000) >> 10)));
                            u = static_cast<UChar>(0xDC00 | ((character - 0x10000) & 0x3FF));
                        } else
                            u = static_cast<UChar>(character);
                    }
                }
            }
            if (charLen == 0) {
                if (strict)
                    return throwError(exec, createURIError(exec, ASCIILiteral("URI error")));
                // The only case where we don't use "strict" mode is the "unescape" function.
                // For that, it's good to support the wonky "%u" syntax for compatibility with WinIE.
                if (k <= length - 6 && p[1] == 'u'
                        && isASCIIHexDigit(p[2]) && isASCIIHexDigit(p[3])
                        && isASCIIHexDigit(p[4]) && isASCIIHexDigit(p[5])) {
                    charLen = 6;
                    u = Lexer<UChar>::convertUnicode(p[2], p[3], p[4], p[5]);
                }
            }
            if (charLen && (u == 0 || u >= 128 || !strchr(doNotUnescape, u))) {
                if (u < 256)
                    builder.append(static_cast<LChar>(u));
                else
                    builder.append(u);
                k += charLen;
                continue;
            }
        }
        k++;
        builder.append(c);
    }
    return builder.build(exec);
}

static JSValue decode(ExecState* exec, const char* doNotUnescape, bool strict)
{
    JSStringBuilder builder;
    String str = exec->argument(0).toString(exec)->value(exec);
    
    if (str.is8Bit())
        return decode(exec, str.characters8(), str.length(), doNotUnescape, strict);
    return decode(exec, str.characters16(), str.length(), doNotUnescape, strict);
}

bool isStrWhiteSpace(UChar c)
{
    switch (c) {
        // ECMA-262-5th 7.2 & 7.3
        case 0x0009:
        case 0x000A:
        case 0x000B:
        case 0x000C:
        case 0x000D:
        case 0x0020:
        case 0x00A0:
        case 0x2028:
        case 0x2029:
        case 0xFEFF:
            return true;
        default:
            return c > 0xff && isSeparatorSpace(c);
    }
}

static int parseDigit(unsigned short c, int radix)
{
    int digit = -1;

    if (c >= '0' && c <= '9')
        digit = c - '0';
    else if (c >= 'A' && c <= 'Z')
        digit = c - 'A' + 10;
    else if (c >= 'a' && c <= 'z')
        digit = c - 'a' + 10;

    if (digit >= radix)
        return -1;
    return digit;
}

double parseIntOverflow(const LChar* s, int length, int radix)
{
    double number = 0.0;
    double radixMultiplier = 1.0;

    for (const LChar* p = s + length - 1; p >= s; p--) {
        if (radixMultiplier == std::numeric_limits<double>::infinity()) {
            if (*p != '0') {
                number = std::numeric_limits<double>::infinity();
                break;
            }
        } else {
            int digit = parseDigit(*p, radix);
            number += digit * radixMultiplier;
        }

        radixMultiplier *= radix;
    }

    return number;
}

double parseIntOverflow(const UChar* s, int length, int radix)
{
    double number = 0.0;
    double radixMultiplier = 1.0;

    for (const UChar* p = s + length - 1; p >= s; p--) {
        if (radixMultiplier == std::numeric_limits<double>::infinity()) {
            if (*p != '0') {
                number = std::numeric_limits<double>::infinity();
                break;
            }
        } else {
            int digit = parseDigit(*p, radix);
            number += digit * radixMultiplier;
        }

        radixMultiplier *= radix;
    }

    return number;
}

// ES5.1 15.1.2.2
template <typename CharType>
ALWAYS_INLINE
static double parseInt(const String& s, const CharType* data, int radix)
{
    // 1. Let inputString be ToString(string).
    // 2. Let S be a newly created substring of inputString consisting of the first character that is not a
    //    StrWhiteSpaceChar and all characters following that character. (In other words, remove leading white
    //    space.) If inputString does not contain any such characters, let S be the empty string.
    int length = s.length();
    int p = 0;
    while (p < length && isStrWhiteSpace(data[p]))
        ++p;

    // 3. Let sign be 1.
    // 4. If S is not empty and the first character of S is a minus sign -, let sign be -1.
    // 5. If S is not empty and the first character of S is a plus sign + or a minus sign -, then remove the first character from S.
    double sign = 1;
    if (p < length) {
        if (data[p] == '+')
            ++p;
        else if (data[p] == '-') {
            sign = -1;
            ++p;
        }
    }

    // 6. Let R = ToInt32(radix).
    // 7. Let stripPrefix be true.
    // 8. If R != 0,then
    //   b. If R != 16, let stripPrefix be false.
    // 9. Else, R == 0
    //   a. LetR = 10.
    // 10. If stripPrefix is true, then
    //   a. If the length of S is at least 2 and the first two characters of S are either ―0x or ―0X,
    //      then remove the first two characters from S and let R = 16.
    // 11. If S contains any character that is not a radix-R digit, then let Z be the substring of S
    //     consisting of all characters before the first such character; otherwise, let Z be S.
    if ((radix == 0 || radix == 16) && length - p >= 2 && data[p] == '0' && (data[p + 1] == 'x' || data[p + 1] == 'X')) {
        radix = 16;
        p += 2;
    } else if (radix == 0)
        radix = 10;

    // 8.a If R < 2 or R > 36, then return NaN.
    if (radix < 2 || radix > 36)
        return QNaN;

    // 13. Let mathInt be the mathematical integer value that is represented by Z in radix-R notation, using the letters
    //     A-Z and a-z for digits with values 10 through 35. (However, if R is 10 and Z contains more than 20 significant
    //     digits, every significant digit after the 20th may be replaced by a 0 digit, at the option of the implementation;
    //     and if R is not 2, 4, 8, 10, 16, or 32, then mathInt may be an implementation-dependent approximation to the
    //     mathematical integer value that is represented by Z in radix-R notation.)
    // 14. Let number be the Number value for mathInt.
    int firstDigitPosition = p;
    bool sawDigit = false;
    double number = 0;
    while (p < length) {
        int digit = parseDigit(data[p], radix);
        if (digit == -1)
            break;
        sawDigit = true;
        number *= radix;
        number += digit;
        ++p;
    }

    // 12. If Z is empty, return NaN.
    if (!sawDigit)
        return QNaN;

    // Alternate code path for certain large numbers.
    if (number >= mantissaOverflowLowerBound) {
        if (radix == 10) {
            size_t parsedLength;
            number = parseDouble(s.characters() + firstDigitPosition, p - firstDigitPosition, parsedLength);
        } else if (radix == 2 || radix == 4 || radix == 8 || radix == 16 || radix == 32)
            number = parseIntOverflow(s.substringSharingImpl(firstDigitPosition, p - firstDigitPosition).utf8().data(), p - firstDigitPosition, radix);
    }

    // 15. Return sign x number.
    return sign * number;
}

static double parseInt(const String& s, int radix)
{
    if (s.is8Bit())
        return parseInt(s, s.characters8(), radix);
    return parseInt(s, s.characters16(), radix);
}

static const int SizeOfInfinity = 8;

template <typename CharType>
static bool isInfinity(const CharType* data, const CharType* end)
{
    return (end - data) >= SizeOfInfinity
        && data[0] == 'I'
        && data[1] == 'n'
        && data[2] == 'f'
        && data[3] == 'i'
        && data[4] == 'n'
        && data[5] == 'i'
        && data[6] == 't'
        && data[7] == 'y';
}

// See ecma-262 9.3.1
template <typename CharType>
static double jsHexIntegerLiteral(const CharType*& data, const CharType* end)
{
    // Hex number.
    data += 2;
    const CharType* firstDigitPosition = data;
    double number = 0;
    while (true) {
        number = number * 16 + toASCIIHexValue(*data);
        ++data;
        if (data == end)
            break;
        if (!isASCIIHexDigit(*data))
            break;
    }
    if (number >= mantissaOverflowLowerBound)
        number = parseIntOverflow(firstDigitPosition, data - firstDigitPosition, 16);

    return number;
}

// See ecma-262 9.3.1
template <typename CharType>
static double jsStrDecimalLiteral(const CharType*& data, const CharType* end)
{
    RELEASE_ASSERT(data < end);

    size_t parsedLength;
    double number = parseDouble(data, end - data, parsedLength);
    if (parsedLength) {
        data += parsedLength;
        return number;
    }

    // Check for [+-]?Infinity
    switch (*data) {
    case 'I':
        if (isInfinity(data, end)) {
            data += SizeOfInfinity;
            return std::numeric_limits<double>::infinity();
        }
        break;

    case '+':
        if (isInfinity(data + 1, end)) {
            data += SizeOfInfinity + 1;
            return std::numeric_limits<double>::infinity();
        }
        break;

    case '-':
        if (isInfinity(data + 1, end)) {
            data += SizeOfInfinity + 1;
            return -std::numeric_limits<double>::infinity();
        }
        break;
    }

    // Not a number.
    return QNaN;
}

template <typename CharType>
static double toDouble(const CharType* characters, unsigned size)
{
    const CharType* endCharacters = characters + size;

    // Skip leading white space.
    for (; characters < endCharacters; ++characters) {
        if (!isStrWhiteSpace(*characters))
            break;
    }
    
    // Empty string.
    if (characters == endCharacters)
        return 0.0;
    
    double number;
    if (characters[0] == '0' && characters + 2 < endCharacters && (characters[1] | 0x20) == 'x' && isASCIIHexDigit(characters[2]))
        number = jsHexIntegerLiteral(characters, endCharacters);
    else
        number = jsStrDecimalLiteral(characters, endCharacters);
    
    // Allow trailing white space.
    for (; characters < endCharacters; ++characters) {
        if (!isStrWhiteSpace(*characters))
            break;
    }
    if (characters != endCharacters)
        return QNaN;
    
    return number;
}

// See ecma-262 9.3.1
double jsToNumber(const String& s)
{
    unsigned size = s.length();

    if (size == 1) {
        UChar c = s[0];
        if (isASCIIDigit(c))
            return c - '0';
        if (isStrWhiteSpace(c))
            return 0;
        return QNaN;
    }

    if (s.is8Bit())
        return toDouble(s.characters8(), size);
    return toDouble(s.characters16(), size);
}

static double parseFloat(const String& s)
{
    unsigned size = s.length();

    if (size == 1) {
        UChar c = s[0];
        if (isASCIIDigit(c))
            return c - '0';
        return QNaN;
    }

    if (s.is8Bit()) {
        const LChar* data = s.characters8();
        const LChar* end = data + size;

        // Skip leading white space.
        for (; data < end; ++data) {
            if (!isStrWhiteSpace(*data))
                break;
        }

        // Empty string.
        if (data == end)
            return QNaN;

        return jsStrDecimalLiteral(data, end);
    }

    const UChar* data = s.characters16();
    const UChar* end = data + size;

    // Skip leading white space.
    for (; data < end; ++data) {
        if (!isStrWhiteSpace(*data))
            break;
    }

    // Empty string.
    if (data == end)
        return QNaN;

    return jsStrDecimalLiteral(data, end);
}

EncodedJSValue JSC_HOST_CALL globalFuncEval(ExecState* exec)
{
    JSValue x = exec->argument(0);
    if (!x.isString())
        return JSValue::encode(x);

    String s = x.toString(exec)->value(exec);

    if (s.is8Bit()) {
        LiteralParser<LChar> preparser(exec, s.characters8(), s.length(), NonStrictJSON);
        if (JSValue parsedObject = preparser.tryLiteralParse())
            return JSValue::encode(parsedObject);
    } else {
        LiteralParser<UChar> preparser(exec, s.characters16(), s.length(), NonStrictJSON);
        if (JSValue parsedObject = preparser.tryLiteralParse())
            return JSValue::encode(parsedObject);        
    }

    JSGlobalObject* calleeGlobalObject = exec->callee()->globalObject();
    EvalExecutable* eval = EvalExecutable::create(exec, exec->vm().codeCache(), makeSource(s), false);
    JSObject* error = eval->compile(exec, calleeGlobalObject);
    if (error)
        return throwVMError(exec, error);

    return JSValue::encode(exec->interpreter()->execute(eval, exec, calleeGlobalObject->globalThis(), calleeGlobalObject));
}

EncodedJSValue JSC_HOST_CALL globalFuncParseInt(ExecState* exec)
{
    JSValue value = exec->argument(0);
    JSValue radixValue = exec->argument(1);

    // Optimized handling for numbers:
    // If the argument is 0 or a number in range 10^-6 <= n < INT_MAX+1, then parseInt
    // results in a truncation to integer. In the case of -0, this is converted to 0.
    //
    // This is also a truncation for values in the range INT_MAX+1 <= n < 10^21,
    // however these values cannot be trivially truncated to int since 10^21 exceeds
    // even the int64_t range. Negative numbers are a little trickier, the case for
    // values in the range -10^21 < n <= -1 are similar to those for integer, but
    // values in the range -1 < n <= -10^-6 need to truncate to -0, not 0.
    static const double tenToTheMinus6 = 0.000001;
    static const double intMaxPlusOne = 2147483648.0;
    if (value.isNumber()) {
        double n = value.asNumber();
        if (((n < intMaxPlusOne && n >= tenToTheMinus6) || !n) && radixValue.isUndefinedOrNull())
            return JSValue::encode(jsNumber(static_cast<int32_t>(n)));
    }

    // If ToString throws, we shouldn't call ToInt32.
    String s = value.toString(exec)->value(exec);
    if (exec->hadException())
        return JSValue::encode(jsUndefined());

    return JSValue::encode(jsNumber(parseInt(s, radixValue.toInt32(exec))));
}

EncodedJSValue JSC_HOST_CALL globalFuncParseFloat(ExecState* exec)
{
    return JSValue::encode(jsNumber(parseFloat(exec->argument(0).toString(exec)->value(exec))));
}

EncodedJSValue JSC_HOST_CALL globalFuncIsNaN(ExecState* exec)
{
    return JSValue::encode(jsBoolean(std::isnan(exec->argument(0).toNumber(exec))));
}

EncodedJSValue JSC_HOST_CALL globalFuncIsFinite(ExecState* exec)
{
    double n = exec->argument(0).toNumber(exec);
    return JSValue::encode(jsBoolean(std::isfinite(n)));
}

EncodedJSValue JSC_HOST_CALL globalFuncDecodeURI(ExecState* exec)
{
    static const char do_not_unescape_when_decoding_URI[] =
        "#$&+,/:;=?@";

    return JSValue::encode(decode(exec, do_not_unescape_when_decoding_URI, true));
}

EncodedJSValue JSC_HOST_CALL globalFuncDecodeURIComponent(ExecState* exec)
{
    return JSValue::encode(decode(exec, "", true));
}

EncodedJSValue JSC_HOST_CALL globalFuncEncodeURI(ExecState* exec)
{
    static const char do_not_escape_when_encoding_URI[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789"
        "!#$&'()*+,-./:;=?@_~";

    return JSValue::encode(encode(exec, do_not_escape_when_encoding_URI));
}

EncodedJSValue JSC_HOST_CALL globalFuncEncodeURIComponent(ExecState* exec)
{
    static const char do_not_escape_when_encoding_URI_component[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789"
        "!'()*-._~";

    return JSValue::encode(encode(exec, do_not_escape_when_encoding_URI_component));
}

EncodedJSValue JSC_HOST_CALL globalFuncEscape(ExecState* exec)
{
    static const char do_not_escape[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789"
        "*+-./@_";

    JSStringBuilder builder;
    String str = exec->argument(0).toString(exec)->value(exec);
    if (str.is8Bit()) {
        const LChar* c = str.characters8();
        for (unsigned k = 0; k < str.length(); k++, c++) {
            int u = c[0];
            if (u && strchr(do_not_escape, static_cast<char>(u)))
                builder.append(c, 1);
            else {
                char tmp[4];
                snprintf(tmp, sizeof(tmp), "%%%02X", u);
                builder.append(tmp);
            }
        }

        return JSValue::encode(builder.build(exec));        
    }

    const UChar* c = str.characters16();
    for (unsigned k = 0; k < str.length(); k++, c++) {
        int u = c[0];
        if (u > 255) {
            char tmp[7];
            snprintf(tmp, sizeof(tmp), "%%u%04X", u);
            builder.append(tmp);
        } else if (u != 0 && strchr(do_not_escape, static_cast<char>(u)))
            builder.append(c, 1);
        else {
            char tmp[4];
            snprintf(tmp, sizeof(tmp), "%%%02X", u);
            builder.append(tmp);
        }
    }

    return JSValue::encode(builder.build(exec));
}

EncodedJSValue JSC_HOST_CALL globalFuncUnescape(ExecState* exec)
{
    StringBuilder builder;
    String str = exec->argument(0).toString(exec)->value(exec);
    int k = 0;
    int len = str.length();
    
    if (str.is8Bit()) {
        const LChar* characters = str.characters8();
        LChar convertedLChar;
        while (k < len) {
            const LChar* c = characters + k;
            if (c[0] == '%' && k <= len - 6 && c[1] == 'u') {
                if (isASCIIHexDigit(c[2]) && isASCIIHexDigit(c[3]) && isASCIIHexDigit(c[4]) && isASCIIHexDigit(c[5])) {
                    builder.append(Lexer<UChar>::convertUnicode(c[2], c[3], c[4], c[5]));
                    k += 6;
                    continue;
                }
            } else if (c[0] == '%' && k <= len - 3 && isASCIIHexDigit(c[1]) && isASCIIHexDigit(c[2])) {
                convertedLChar = LChar(Lexer<LChar>::convertHex(c[1], c[2]));
                c = &convertedLChar;
                k += 2;
            }
            builder.append(*c);
            k++;
        }        
    } else {
        const UChar* characters = str.characters16();

        while (k < len) {
            const UChar* c = characters + k;
            UChar convertedUChar;
            if (c[0] == '%' && k <= len - 6 && c[1] == 'u') {
                if (isASCIIHexDigit(c[2]) && isASCIIHexDigit(c[3]) && isASCIIHexDigit(c[4]) && isASCIIHexDigit(c[5])) {
                    convertedUChar = Lexer<UChar>::convertUnicode(c[2], c[3], c[4], c[5]);
                    c = &convertedUChar;
                    k += 5;
                }
            } else if (c[0] == '%' && k <= len - 3 && isASCIIHexDigit(c[1]) && isASCIIHexDigit(c[2])) {
                convertedUChar = UChar(Lexer<UChar>::convertHex(c[1], c[2]));
                c = &convertedUChar;
                k += 2;
            }
            k++;
            builder.append(*c);
        }
    }

    return JSValue::encode(jsString(exec, builder.toString()));
}

EncodedJSValue JSC_HOST_CALL globalFuncThrowTypeError(ExecState* exec)
{
    return throwVMTypeError(exec);
}

EncodedJSValue JSC_HOST_CALL globalFuncProtoGetter(ExecState* exec)
{
    if (!exec->thisValue().isObject())
        return JSValue::encode(exec->thisValue().synthesizePrototype(exec));

    JSObject* thisObject = asObject(exec->thisValue());
    if (!thisObject->allowsAccessFrom(exec->trueCallerFrame()))
        return JSValue::encode(jsUndefined());

    return JSValue::encode(thisObject->prototype());
}

EncodedJSValue JSC_HOST_CALL globalFuncProtoSetter(ExecState* exec)
{
    JSValue value = exec->argument(0);

    // Setting __proto__ of a primitive should have no effect.
    if (!exec->thisValue().isObject())
        return JSValue::encode(jsUndefined());

    JSObject* thisObject = asObject(exec->thisValue());
    if (!thisObject->allowsAccessFrom(exec->trueCallerFrame()))
        return JSValue::encode(jsUndefined());

    // Setting __proto__ to a non-object, non-null value is silently ignored to match Mozilla.
    if (!value.isObject() && !value.isNull())
        return JSValue::encode(jsUndefined());

    if (!thisObject->isExtensible())
        return throwVMError(exec, createTypeError(exec, StrictModeReadonlyPropertyWriteError));

    if (!thisObject->setPrototypeWithCycleCheck(exec->vm(), value))
        throwError(exec, createError(exec, "cyclic __proto__ value"));
    return JSValue::encode(jsUndefined());
}

} // namespace JSC
