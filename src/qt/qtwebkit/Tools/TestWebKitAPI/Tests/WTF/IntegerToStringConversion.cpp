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
#include <wtf/StringExtras.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

template<typename IntegerType> struct PrintfFormatTrait { static const char format[]; };

template<> struct PrintfFormatTrait<short> { static const char format[]; };
const char PrintfFormatTrait<short>::format[] = "%hd";

template<> struct PrintfFormatTrait<int> { static const char format[]; };
const char PrintfFormatTrait<int>::format[] = "%d";

template<> struct PrintfFormatTrait<long> { static const char format[]; };
const char PrintfFormatTrait<long>::format[] = "%ld";

template<> struct PrintfFormatTrait<long long> { static const char format[]; };
#if OS(WINDOWS) && !PLATFORM(QT)
const char PrintfFormatTrait<long long>::format[] = "%I64i";
#else
const char PrintfFormatTrait<long long>::format[] = "%lli";
#endif // OS(WINDOWS) && !PLATFORM(QT)

template<> struct PrintfFormatTrait<unsigned short> { static const char format[]; };
const char PrintfFormatTrait<unsigned short>::format[] = "%hu";

template<> struct PrintfFormatTrait<unsigned> { static const char format[]; };
const char PrintfFormatTrait<unsigned>::format[] = "%u";

template<> struct PrintfFormatTrait<unsigned long> { static const char format[]; };
const char PrintfFormatTrait<unsigned long>::format[] = "%lu";

template<> struct PrintfFormatTrait<unsigned long long> { static const char format[]; };
#if OS(WINDOWS) && !PLATFORM(QT)
const char PrintfFormatTrait<unsigned long long>::format[] = "%I64u";
#else
const char PrintfFormatTrait<unsigned long long>::format[] = "%llu";
#endif // OS(WINDOWS) && !PLATFORM(QT)


// FIXME: use snprintf from StringExtras.h
template<typename IntegerType>
void testBoundaries()
{
    const unsigned bufferSize = 256;
    Vector<char, bufferSize> buffer;
    buffer.resize(bufferSize);

    const IntegerType min = std::numeric_limits<IntegerType>::min();
    CString minStringData = String::number(min).latin1();
    snprintf(buffer.data(), bufferSize, PrintfFormatTrait<IntegerType>::format, min);
    ASSERT_STREQ(buffer.data(), minStringData.data());

    const IntegerType max = std::numeric_limits<IntegerType>::max();
    CString maxStringData = String::number(max).latin1();
    snprintf(buffer.data(), bufferSize, PrintfFormatTrait<IntegerType>::format, max);
    ASSERT_STREQ(buffer.data(), maxStringData.data());
}

template<typename IntegerType>
void testNumbers()
{
    const unsigned bufferSize = 256;
    Vector<char, bufferSize> buffer;
    buffer.resize(bufferSize);

    for (int i = -100; i < 100; ++i) {
        const IntegerType number = static_cast<IntegerType>(i);
        CString numberStringData = String::number(number).latin1();
        snprintf(buffer.data(), bufferSize, PrintfFormatTrait<IntegerType>::format, number);
        ASSERT_STREQ(buffer.data(), numberStringData.data());
    }
}

TEST(WTF, IntegerToStringConversionSignedIntegerBoundaries)
{
    testBoundaries<short>();
    testBoundaries<int>();
    testBoundaries<long>();
    testBoundaries<long long>();
}

TEST(WTF, IntegerToStringConversionSignedIntegerRegularNumbers)
{
    testNumbers<short>();
    testNumbers<int>();
    testNumbers<long>();
    testNumbers<long long>();
}

TEST(WTF, IntegerToStringConversionUnsignedIntegerBoundaries)
{
    testBoundaries<unsigned short>();
    testBoundaries<unsigned int>();
    testBoundaries<unsigned long>();
    testBoundaries<unsigned long long>();
}

TEST(WTF, IntegerToStringConversionUnsignedIntegerRegularNumbers)
{
    testNumbers<unsigned short>();
    testNumbers<unsigned int>();
    testNumbers<unsigned long>();
    testNumbers<unsigned long long>();
}
