/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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

#define _USE_MATH_DEFINES 1
#include "config.h"

#include <wtf/MediaTime.h>

using namespace std;

#if COMPILER(MSVC)
// Work around Visual Studio 2008's lack of an INFINITY or NAN definition.
#include <limits>
#if !defined(INFINITY)
#define INFINITY (numeric_limits<double>::infinity())
#endif
#if !defined(NAN)
#define NAN (numeric_limits<double>::quiet_NaN())
#endif
#endif

namespace WTF {

std::ostream& operator<<(std::ostream& out, const MediaTime& val)
{
    out << "{ ";
    if (val.isInvalid())
        out << "invalid";
    else if (val.isPositiveInfinite())
        out << "+infinite";
    else if (val.isNegativeInfinite())
        out << "-infinite";
    else
        out << "value: " << val.timeValue() << ", scale: " << val.timeScale();
    return out << " }";
}

}

namespace TestWebKitAPI {

TEST(WTF, MediaTime)
{
    // Comparison Operators
    EXPECT_EQ(MediaTime::positiveInfiniteTime() > MediaTime::negativeInfiniteTime(), true);
    EXPECT_EQ(MediaTime::negativeInfiniteTime() < MediaTime::positiveInfiniteTime(), true);
    EXPECT_EQ(MediaTime::negativeInfiniteTime() == MediaTime::negativeInfiniteTime(), true);
    EXPECT_EQ(MediaTime::positiveInfiniteTime() == MediaTime::positiveInfiniteTime(), true);
    EXPECT_EQ(MediaTime::invalidTime() == MediaTime::invalidTime(), true);
    EXPECT_EQ(MediaTime::invalidTime() > MediaTime::negativeInfiniteTime(), true);
    EXPECT_EQ(MediaTime::invalidTime() > MediaTime::positiveInfiniteTime(), true);
    EXPECT_EQ(MediaTime::negativeInfiniteTime() < MediaTime::invalidTime(), true);
    EXPECT_EQ(MediaTime::positiveInfiniteTime() < MediaTime::invalidTime(), true);
    EXPECT_EQ(MediaTime::indefiniteTime() == MediaTime::indefiniteTime(), true);
    EXPECT_EQ(MediaTime::indefiniteTime() > MediaTime::negativeInfiniteTime(), true);
    EXPECT_EQ(MediaTime::indefiniteTime() < MediaTime::positiveInfiniteTime(), true);
    EXPECT_EQ(MediaTime::negativeInfiniteTime() < MediaTime::indefiniteTime(), true);
    EXPECT_EQ(MediaTime::positiveInfiniteTime() > MediaTime::indefiniteTime(), true);
    EXPECT_EQ(MediaTime(1, 1) < MediaTime::indefiniteTime(), true);
    EXPECT_EQ(MediaTime::indefiniteTime() > MediaTime(1, 1), true);
    EXPECT_EQ(MediaTime(1, 1) < MediaTime(2, 1), true);
    EXPECT_EQ(MediaTime(2, 1) > MediaTime(1, 1), true);
    EXPECT_EQ(MediaTime(2, 1) == MediaTime(2, 1), true);
    EXPECT_EQ(MediaTime(2, 1) == MediaTime(4, 2), true);

    // Addition Operators
    EXPECT_EQ(MediaTime::positiveInfiniteTime() + MediaTime::positiveInfiniteTime(), MediaTime::positiveInfiniteTime());
    EXPECT_EQ(MediaTime::negativeInfiniteTime() + MediaTime::negativeInfiniteTime(), MediaTime::negativeInfiniteTime());
    EXPECT_EQ(MediaTime::positiveInfiniteTime() + MediaTime::negativeInfiniteTime(), MediaTime::invalidTime());
    EXPECT_EQ(MediaTime::negativeInfiniteTime() + MediaTime::positiveInfiniteTime(), MediaTime::invalidTime());
    EXPECT_EQ(MediaTime::invalidTime() + MediaTime::positiveInfiniteTime(), MediaTime::invalidTime());
    EXPECT_EQ(MediaTime::invalidTime() + MediaTime::negativeInfiniteTime(), MediaTime::invalidTime());
    EXPECT_EQ(MediaTime::invalidTime() + MediaTime::invalidTime(), MediaTime::invalidTime());
    EXPECT_EQ(MediaTime::invalidTime() + MediaTime(1, 1), MediaTime::invalidTime());
    EXPECT_EQ(MediaTime::indefiniteTime() + MediaTime::positiveInfiniteTime(), MediaTime::indefiniteTime());
    EXPECT_EQ(MediaTime::indefiniteTime() + MediaTime::negativeInfiniteTime(), MediaTime::indefiniteTime());
    EXPECT_EQ(MediaTime::indefiniteTime() + MediaTime::indefiniteTime(), MediaTime::indefiniteTime());
    EXPECT_EQ(MediaTime::indefiniteTime() + MediaTime(1, 1), MediaTime::indefiniteTime());
    EXPECT_EQ(MediaTime(1, 1) + MediaTime(1, 1), MediaTime(2, 1));
    EXPECT_EQ(MediaTime(1, 2) + MediaTime(1, 3), MediaTime(5, 6));
    EXPECT_EQ(MediaTime(1, numeric_limits<int32_t>::max()-1) + MediaTime(1, numeric_limits<int32_t>::max()-2), MediaTime(2, numeric_limits<int32_t>::max()));

    // Subtraction Operators
    EXPECT_EQ(MediaTime::positiveInfiniteTime() - MediaTime::positiveInfiniteTime(), MediaTime::invalidTime());
    EXPECT_EQ(MediaTime::negativeInfiniteTime() - MediaTime::negativeInfiniteTime(), MediaTime::invalidTime());
    EXPECT_EQ(MediaTime::positiveInfiniteTime() - MediaTime::negativeInfiniteTime(), MediaTime::positiveInfiniteTime());
    EXPECT_EQ(MediaTime::negativeInfiniteTime() - MediaTime::positiveInfiniteTime(), MediaTime::negativeInfiniteTime());
    EXPECT_EQ(MediaTime::invalidTime() - MediaTime::positiveInfiniteTime(), MediaTime::invalidTime());
    EXPECT_EQ(MediaTime::invalidTime() - MediaTime::negativeInfiniteTime(), MediaTime::invalidTime());
    EXPECT_EQ(MediaTime::invalidTime() - MediaTime::invalidTime(), MediaTime::invalidTime());
    EXPECT_EQ(MediaTime::invalidTime() - MediaTime(1, 1), MediaTime::invalidTime());
    EXPECT_EQ(MediaTime::indefiniteTime() - MediaTime::positiveInfiniteTime(), MediaTime::indefiniteTime());
    EXPECT_EQ(MediaTime::indefiniteTime() - MediaTime::negativeInfiniteTime(), MediaTime::indefiniteTime());
    EXPECT_EQ(MediaTime::indefiniteTime() - MediaTime::indefiniteTime(), MediaTime::indefiniteTime());
    EXPECT_EQ(MediaTime::indefiniteTime() - MediaTime(1, 1), MediaTime::indefiniteTime());
    EXPECT_EQ(MediaTime(3, 1) - MediaTime(2, 1), MediaTime(1, 1));
    EXPECT_EQ(MediaTime(1, 2) - MediaTime(1, 3), MediaTime(1, 6));
    EXPECT_EQ(MediaTime(2, numeric_limits<int32_t>::max()-1) - MediaTime(1, numeric_limits<int32_t>::max()-2), MediaTime(1, numeric_limits<int32_t>::max()));

    // Constants
    EXPECT_EQ(MediaTime::zeroTime(), MediaTime(0, 1));
    EXPECT_EQ(MediaTime::invalidTime(), MediaTime(-1, 1, 0));
    EXPECT_EQ(MediaTime::positiveInfiniteTime(), MediaTime(0, 1, MediaTime::PositiveInfinite));
    EXPECT_EQ(MediaTime::negativeInfiniteTime(), MediaTime(0, 1, MediaTime::NegativeInfinite));
    EXPECT_EQ(MediaTime::indefiniteTime(), MediaTime(0, 1, MediaTime::Indefinite));

    // Absolute Functions
    EXPECT_EQ(abs(MediaTime::positiveInfiniteTime()), MediaTime::positiveInfiniteTime());
    EXPECT_EQ(abs(MediaTime::negativeInfiniteTime()), MediaTime::positiveInfiniteTime());
    EXPECT_EQ(abs(MediaTime::invalidTime()), MediaTime::invalidTime());
    EXPECT_EQ(abs(MediaTime(1, 1)), MediaTime(1, 1));
    EXPECT_EQ(abs(MediaTime(-1, 1)), MediaTime(1, 1));
    EXPECT_EQ(abs(MediaTime(-1, -1)), MediaTime(-1, -1));
    EXPECT_EQ(abs(MediaTime(1, -1)), MediaTime(-1, -1));

    // Floating Point Functions
    EXPECT_EQ(MediaTime::createWithFloat(1.0f), MediaTime(1, 1));
    EXPECT_EQ(MediaTime::createWithFloat(1.5f), MediaTime(3, 2));
    EXPECT_EQ(MediaTime::createWithDouble(1.0), MediaTime(1, 1));
    EXPECT_EQ(MediaTime::createWithDouble(1.5), MediaTime(3, 2));
    EXPECT_EQ(MediaTime(1, 1).toFloat(), 1.0f);
    EXPECT_EQ(MediaTime(3, 2).toFloat(), 1.5f);
    EXPECT_EQ(MediaTime(1, 1).toDouble(), 1.0);
    EXPECT_EQ(MediaTime(3, 2).toDouble(), 1.5);
    EXPECT_EQ(MediaTime(1, 1 << 16).toFloat(), 1 / pow(2.0f, 16.0f));
    EXPECT_EQ(MediaTime(1, 1 << 30).toDouble(), 1 / pow(2.0, 30.0));
    EXPECT_EQ(MediaTime::createWithDouble(M_PI, 1 << 30), MediaTime(3373259426U, 1 << 30));
    EXPECT_EQ(MediaTime::createWithFloat(INFINITY), MediaTime::positiveInfiniteTime());
    EXPECT_EQ(MediaTime::createWithFloat(-INFINITY), MediaTime::negativeInfiniteTime());
    EXPECT_EQ(MediaTime::createWithFloat(NAN), MediaTime::invalidTime());
    EXPECT_EQ(MediaTime::createWithDouble(INFINITY), MediaTime::positiveInfiniteTime());
    EXPECT_EQ(MediaTime::createWithDouble(-INFINITY), MediaTime::negativeInfiniteTime());
    EXPECT_EQ(MediaTime::createWithDouble(NAN), MediaTime::invalidTime());

    // Overflow Behavior
    EXPECT_EQ(MediaTime::createWithFloat(pow(2.0f, 64.0f)), MediaTime::positiveInfiniteTime());
    EXPECT_EQ(MediaTime::createWithFloat(-pow(2.0f, 64.0f)), MediaTime::negativeInfiniteTime());
    EXPECT_EQ(MediaTime::createWithFloat(pow(2.0f, 63.0f), 2).timeScale(), 1);
    EXPECT_EQ(MediaTime::createWithFloat(pow(2.0f, 63.0f), 3).timeScale(), 1);
    EXPECT_EQ(MediaTime::createWithDouble(pow(2.0, 64.0)), MediaTime::positiveInfiniteTime());
    EXPECT_EQ(MediaTime::createWithDouble(-pow(2.0, 64.0)), MediaTime::negativeInfiniteTime());
    EXPECT_EQ(MediaTime::createWithDouble(pow(2.0, 63.0), 2).timeScale(), 1);
    EXPECT_EQ(MediaTime::createWithDouble(pow(2.0, 63.0), 3).timeScale(), 1);
    EXPECT_EQ((MediaTime(numeric_limits<int64_t>::max(), 2) + MediaTime(numeric_limits<int64_t>::max(), 2)).timeScale(), 1);
    EXPECT_EQ((MediaTime(numeric_limits<int64_t>::min(), 2) - MediaTime(numeric_limits<int64_t>::max(), 2)).timeScale(), 1);
    EXPECT_EQ(MediaTime(numeric_limits<int64_t>::max(), 1) + MediaTime(numeric_limits<int64_t>::max(), 1), MediaTime::positiveInfiniteTime());
    EXPECT_EQ(MediaTime(numeric_limits<int64_t>::min(), 1) + MediaTime(numeric_limits<int64_t>::min(), 1), MediaTime::negativeInfiniteTime());
    EXPECT_EQ(MediaTime(numeric_limits<int64_t>::min(), 1) - MediaTime(numeric_limits<int64_t>::max(), 1), MediaTime::negativeInfiniteTime());
    EXPECT_EQ(MediaTime(numeric_limits<int64_t>::max(), 1) - MediaTime(numeric_limits<int64_t>::min(), 1), MediaTime::positiveInfiniteTime());
}

}

