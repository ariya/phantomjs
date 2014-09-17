/*
 *  Copyright (C) 1999-2002 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
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
#include "JSGlobalObject.h"
#include "JSString.h"
#include "JSStringBuilder.h"
#include "Lexer.h"
#include "LiteralParser.h"
#include "Nodes.h"
#include "Parser.h"
#include "UStringBuilder.h"
#include "dtoa.h"
#include <stdio.h>
#include <stdlib.h>
#include <wtf/ASCIICType.h>
#include <wtf/Assertions.h>
#include <wtf/MathExtras.h>
#include <wtf/StringExtras.h>
#include <wtf/unicode/UTF8.h>

using namespace WTF;
using namespace Unicode;

namespace JSC {

static JSValue encode(ExecState* exec, const char* doNotEscape)
{
    UString str = exec->argument(0).toString(exec);
    CString cstr = str.utf8(true);
    if (!cstr.data())
        return throwError(exec, createURIError(exec, "String contained an illegal UTF-16 sequence."));

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

static JSValue decode(ExecState* exec, const char* doNotUnescape, bool strict)
{
    JSStringBuilder builder;
    UString str = exec->argument(0).toString(exec);
    int k = 0;
    int len = str.length();
    const UChar* d = str.characters();
    UChar u = 0;
    while (k < len) {
        const UChar* p = d + k;
        UChar c = *p;
        if (c == '%') {
            int charLen = 0;
            if (k <= len - 3 && isASCIIHexDigit(p[1]) && isASCIIHexDigit(p[2])) {
                const char b0 = Lexer::convertHex(p[1], p[2]);
                const int sequenceLen = UTF8SequenceLength(b0);
                if (sequenceLen != 0 && k <= len - sequenceLen * 3) {
                    charLen = sequenceLen * 3;
                    char sequence[5];
                    sequence[0] = b0;
                    for (int i = 1; i < sequenceLen; ++i) {
                        const UChar* q = p + i * 3;
                        if (q[0] == '%' && isASCIIHexDigit(q[1]) && isASCIIHexDigit(q[2]))
                            sequence[i] = Lexer::convertHex(q[1], q[2]);
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
                    return throwError(exec, createURIError(exec, "URI error"));
                // The only case where we don't use "strict" mode is the "unescape" function.
                // For that, it's good to support the wonky "%u" syntax for compatibility with WinIE.
                if (k <= len - 6 && p[1] == 'u'
                        && isASCIIHexDigit(p[2]) && isASCIIHexDigit(p[3])
                        && isASCIIHexDigit(p[4]) && isASCIIHexDigit(p[5])) {
                    charLen = 6;
                    u = Lexer::convertUnicode(p[2], p[3], p[4], p[5]);
                }
            }
            if (charLen && (u == 0 || u >= 128 || !strchr(doNotUnescape, u))) {
                c = u;
                k += charLen - 1;
            }
        }
        k++;
        builder.append(c);
    }
    return builder.build(exec);
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

double parseIntOverflow(const char* s, int length, int radix)
{
    double number = 0.0;
    double radixMultiplier = 1.0;

    for (const char* p = s + length - 1; p >= s; p--) {
        if (radixMultiplier == Inf) {
            if (*p != '0') {
                number = Inf;
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
        if (radixMultiplier == Inf) {
            if (*p != '0') {
                number = Inf;
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

static double parseInt(const UString& s, int radix)
{
    int length = s.length();
    const UChar* data = s.characters();
    int p = 0;

    while (p < length && isStrWhiteSpace(data[p]))
        ++p;

    double sign = 1;
    if (p < length) {
        if (data[p] == '+')
            ++p;
        else if (data[p] == '-') {
            sign = -1;
            ++p;
        }
    }

    if ((radix == 0 || radix == 16) && length - p >= 2 && data[p] == '0' && (data[p + 1] == 'x' || data[p + 1] == 'X')) {
        radix = 16;
        p += 2;
    } else if (radix == 0) {
        if (p < length && data[p] == '0')
            radix = 8;
        else
            radix = 10;
    }

    if (radix < 2 || radix > 36)
        return NaN;

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

    if (number >= mantissaOverflowLowerBound) {
        if (radix == 10)
            number = WTF::strtod(s.substringSharingImpl(firstDigitPosition, p - firstDigitPosition).utf8().data(), 0);
        else if (radix == 2 || radix == 4 || radix == 8 || radix == 16 || radix == 32)
            number = parseIntOverflow(s.substringSharingImpl(firstDigitPosition, p - firstDigitPosition).utf8().data(), p - firstDigitPosition, radix);
    }

    if (!sawDigit)
        return NaN;

    return sign * number;
}

static const int SizeOfInfinity = 8;

static bool isInfinity(const UChar* data, const UChar* end)
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
static double jsHexIntegerLiteral(const UChar*& data, const UChar* end)
{
    // Hex number.
    data += 2;
    const UChar* firstDigitPosition = data;
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
static double jsStrDecimalLiteral(const UChar*& data, const UChar* end)
{
    ASSERT(data < end);

    // Copy the sting into a null-terminated byte buffer, and call strtod.
    Vector<char, 32> byteBuffer;
    for (const UChar* characters = data; characters < end; ++characters) {
        UChar character = *characters;
        byteBuffer.append(isASCII(character) ? character : 0);
    }
    byteBuffer.append(0);
    char* endOfNumber;
    double number = WTF::strtod(byteBuffer.data(), &endOfNumber);

    // Check if strtod found a number; if so return it.
    ptrdiff_t consumed = endOfNumber - byteBuffer.data();
    if (consumed) {
        data += consumed;
        return number;
    }

    // Check for [+-]?Infinity
    switch (*data) {
    case 'I':
        if (isInfinity(data, end)) {
            data += SizeOfInfinity;
            return Inf;
        }
        break;

    case '+':
        if (isInfinity(data + 1, end)) {
            data += SizeOfInfinity + 1;
            return Inf;
        }
        break;

    case '-':
        if (isInfinity(data + 1, end)) {
            data += SizeOfInfinity + 1;
            return -Inf;
        }
        break;
    }

    // Not a number.
    return NaN;
}

// See ecma-262 9.3.1
double jsToNumber(const UString& s)
{
    unsigned size = s.length();

    if (size == 1) {
        UChar c = s.characters()[0];
        if (isASCIIDigit(c))
            return c - '0';
        if (isStrWhiteSpace(c))
            return 0;
        return NaN;
    }

    const UChar* data = s.characters();
    const UChar* end = data + size;

    // Skip leading white space.
    for (; data < end; ++data) {
        if (!isStrWhiteSpace(*data))
            break;
    }

    // Empty string.
    if (data == end)
        return 0.0;

    double number;
    if (data[0] == '0' && data + 2 < end && (data[1] | 0x20) == 'x' && isASCIIHexDigit(data[2]))
        number = jsHexIntegerLiteral(data, end);
    else
        number = jsStrDecimalLiteral(data, end);

    // Allow trailing white space.
    for (; data < end; ++data) {
        if (!isStrWhiteSpace(*data))
            break;
    }
    if (data != end)
        return NaN;

    return number;
}

static double parseFloat(const UString& s)
{
    unsigned size = s.length();

    if (size == 1) {
        UChar c = s.characters()[0];
        if (isASCIIDigit(c))
            return c - '0';
        return NaN;
    }

    const UChar* data = s.characters();
    const UChar* end = data + size;

    // Skip leading white space.
    for (; data < end; ++data) {
        if (!isStrWhiteSpace(*data))
            break;
    }

    // Empty string.
    if (data == end)
        return NaN;

    return jsStrDecimalLiteral(data, end);
}

EncodedJSValue JSC_HOST_CALL globalFuncEval(ExecState* exec)
{
    JSObject* thisObject = exec->hostThisValue().toThisObject(exec);
    JSObject* unwrappedObject = thisObject->unwrappedObject();
    if (!unwrappedObject->isGlobalObject() || static_cast<JSGlobalObject*>(unwrappedObject)->evalFunction() != exec->callee())
        return throwVMError(exec, createEvalError(exec, "The \"this\" value passed to eval must be the global object from which eval originated"));

    JSValue x = exec->argument(0);
    if (!x.isString())
        return JSValue::encode(x);

    UString s = x.toString(exec);

    LiteralParser preparser(exec, s, LiteralParser::NonStrictJSON);
    if (JSValue parsedObject = preparser.tryLiteralParse())
        return JSValue::encode(parsedObject);

    EvalExecutable* eval = EvalExecutable::create(exec, makeSource(s), false);
    JSObject* error = eval->compile(exec, static_cast<JSGlobalObject*>(unwrappedObject)->globalScopeChain());
    if (error)
        return throwVMError(exec, error);

    return JSValue::encode(exec->interpreter()->execute(eval, exec, thisObject, static_cast<JSGlobalObject*>(unwrappedObject)->globalScopeChain()));
}

EncodedJSValue JSC_HOST_CALL globalFuncParseInt(ExecState* exec)
{
    JSValue value = exec->argument(0);
    int32_t radix = exec->argument(1).toInt32(exec);

    if (radix != 0 && radix != 10)
        return JSValue::encode(jsNumber(parseInt(value.toString(exec), radix)));

    if (value.isInt32())
        return JSValue::encode(value);

    if (value.isDouble()) {
        double d = value.asDouble();
        if (isfinite(d))
            return JSValue::encode(jsNumber((d > 0) ? floor(d) : ceil(d)));
        if (isnan(d) || isinf(d))
            return JSValue::encode(jsNaN());
        return JSValue::encode(jsNumber(0));
    }

    return JSValue::encode(jsNumber(parseInt(value.toString(exec), radix)));
}

EncodedJSValue JSC_HOST_CALL globalFuncParseFloat(ExecState* exec)
{
    return JSValue::encode(jsNumber(parseFloat(exec->argument(0).toString(exec))));
}

EncodedJSValue JSC_HOST_CALL globalFuncIsNaN(ExecState* exec)
{
    return JSValue::encode(jsBoolean(isnan(exec->argument(0).toNumber(exec))));
}

EncodedJSValue JSC_HOST_CALL globalFuncIsFinite(ExecState* exec)
{
    double n = exec->argument(0).toNumber(exec);
    return JSValue::encode(jsBoolean(!isnan(n) && !isinf(n)));
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
    UString str = exec->argument(0).toString(exec);
    const UChar* c = str.characters();
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
    UStringBuilder builder;
    UString str = exec->argument(0).toString(exec);
    int k = 0;
    int len = str.length();
    while (k < len) {
        const UChar* c = str.characters() + k;
        UChar u;
        if (c[0] == '%' && k <= len - 6 && c[1] == 'u') {
            if (isASCIIHexDigit(c[2]) && isASCIIHexDigit(c[3]) && isASCIIHexDigit(c[4]) && isASCIIHexDigit(c[5])) {
                u = Lexer::convertUnicode(c[2], c[3], c[4], c[5]);
                c = &u;
                k += 5;
            }
        } else if (c[0] == '%' && k <= len - 3 && isASCIIHexDigit(c[1]) && isASCIIHexDigit(c[2])) {
            u = UChar(Lexer::convertHex(c[1], c[2]));
            c = &u;
            k += 2;
        }
        k++;
        builder.append(*c);
    }

    return JSValue::encode(jsString(exec, builder.toUString()));
}

#ifndef NDEBUG
EncodedJSValue JSC_HOST_CALL globalFuncJSCPrint(ExecState* exec)
{
    CString string = exec->argument(0).toString(exec).utf8();
    puts(string.data());
    return JSValue::encode(jsUndefined());
}
#endif

} // namespace JSC
