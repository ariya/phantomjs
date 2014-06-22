/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include <limits>
#include <wtf/MathExtras.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

namespace TestWebKitAPI {

TEST(WTF, StringCreationFromLiteral)
{
    String stringFromLiteral(ASCIILiteral("Explicit construction syntax"));
    ASSERT_EQ(strlen("Explicit construction syntax"), stringFromLiteral.length());
    ASSERT_TRUE(stringFromLiteral == "Explicit construction syntax");
    ASSERT_TRUE(stringFromLiteral.is8Bit());
    ASSERT_TRUE(stringFromLiteral.impl()->hasTerminatingNullCharacter());
    ASSERT_TRUE(String("Explicit construction syntax") == stringFromLiteral);

    String stringWithTemplate("Template Literal", String::ConstructFromLiteral);
    ASSERT_EQ(strlen("Template Literal"), stringWithTemplate.length());
    ASSERT_TRUE(stringWithTemplate == "Template Literal");
    ASSERT_TRUE(stringWithTemplate.is8Bit());
    ASSERT_TRUE(stringWithTemplate.impl()->hasTerminatingNullCharacter());
    ASSERT_TRUE(String("Template Literal") == stringWithTemplate);
}

TEST(WTF, StringASCII)
{
    CString output;

    // Null String.
    output = String().ascii();
    ASSERT_STREQ("", output.data());

    // Empty String.
    output = emptyString().ascii();
    ASSERT_STREQ("", output.data());

    // Regular String.
    output = String(ASCIILiteral("foobar")).ascii();
    ASSERT_STREQ("foobar", output.data());
}

static void testNumberToStringECMAScript(double number, const char* reference)
{
    CString numberString = String::numberToStringECMAScript(number).latin1();
    ASSERT_STREQ(reference, numberString.data());
}

TEST(WTF, StringNumberToStringECMAScriptBoundaries)
{
    typedef std::numeric_limits<double> Limits;

    // Infinity.
    testNumberToStringECMAScript(Limits::infinity(), "Infinity");
    testNumberToStringECMAScript(-Limits::infinity(), "-Infinity");

    // NaN.
    testNumberToStringECMAScript(-Limits::quiet_NaN(), "NaN");

    // Zeros.
    testNumberToStringECMAScript(0, "0");
    testNumberToStringECMAScript(-0, "0");

    // Min-Max.
    testNumberToStringECMAScript(Limits::min(), "2.2250738585072014e-308");
    testNumberToStringECMAScript(Limits::max(), "1.7976931348623157e+308");
}

TEST(WTF, StringNumberToStringECMAScriptRegularNumbers)
{
    // Pi.
    testNumberToStringECMAScript(piDouble, "3.141592653589793");
    testNumberToStringECMAScript(piFloat, "3.1415927410125732");
    testNumberToStringECMAScript(piOverTwoDouble, "1.5707963267948966");
    testNumberToStringECMAScript(piOverTwoFloat, "1.5707963705062866");
    testNumberToStringECMAScript(piOverFourDouble, "0.7853981633974483");
    testNumberToStringECMAScript(piOverFourFloat, "0.7853981852531433");

    // e.
    const double e = 2.71828182845904523536028747135266249775724709369995;
    testNumberToStringECMAScript(e, "2.718281828459045");

    // c, speed of light in m/s.
    const double c = 299792458;
    testNumberToStringECMAScript(c, "299792458");

    // Golen ratio.
    const double phi = 1.6180339887498948482;
    testNumberToStringECMAScript(phi, "1.618033988749895");
}

TEST(WTF, StringReplaceWithLiteral)
{
    // Cases for 8Bit source.
    String testString = "1224";
    ASSERT_TRUE(testString.is8Bit());
    testString.replaceWithLiteral('2', "");
    ASSERT_STREQ("14", testString.utf8().data());

    testString = "1224";
    ASSERT_TRUE(testString.is8Bit());
    testString.replaceWithLiteral('2', "3");
    ASSERT_STREQ("1334", testString.utf8().data());

    testString = "1224";
    ASSERT_TRUE(testString.is8Bit());
    testString.replaceWithLiteral('2', "555");
    ASSERT_STREQ("15555554", testString.utf8().data());

    testString = "1224";
    ASSERT_TRUE(testString.is8Bit());
    testString.replaceWithLiteral('3', "NotFound");
    ASSERT_STREQ("1224", testString.utf8().data());

    // Cases for 16Bit source.
    testString = String::fromUTF8("résumé");
    ASSERT_FALSE(testString.is8Bit());
    testString.replaceWithLiteral(UChar(0x00E9 /*U+00E9 is 'é'*/), "e");
    ASSERT_STREQ("resume", testString.utf8().data());

    testString = String::fromUTF8("résumé");
    ASSERT_FALSE(testString.is8Bit());
    testString.replaceWithLiteral(UChar(0x00E9 /*U+00E9 is 'é'*/), "");
    ASSERT_STREQ("rsum", testString.utf8().data());

    testString = String::fromUTF8("résumé");
    ASSERT_FALSE(testString.is8Bit());
    testString.replaceWithLiteral('3', "NotFound");
    ASSERT_STREQ("résumé", testString.utf8().data());
}


} // namespace TestWebKitAPI
