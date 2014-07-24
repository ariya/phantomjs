/*
 * Copyright (c) 2012, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "limits.h"
#include <wtf/SaturatedArithmetic.h>

namespace TestWebKitAPI {

TEST(WTF, SaturatedArithmeticAddition)
{
    ASSERT_EQ(saturatedAddition(0, 0), 0);
    ASSERT_EQ(saturatedAddition(0, 1), 1);
    ASSERT_EQ(saturatedAddition(0, 100), 100);
    ASSERT_EQ(saturatedAddition(100, 50), 150);

    ASSERT_EQ(saturatedAddition(0, -1), -1);
    ASSERT_EQ(saturatedAddition(1, -1), 0);
    ASSERT_EQ(saturatedAddition(100, -50), 50);
    ASSERT_EQ(saturatedAddition(50, -100), -50);

    ASSERT_EQ(saturatedAddition(INT_MAX - 1, 0), INT_MAX - 1);
    ASSERT_EQ(saturatedAddition(INT_MAX - 1, 1), INT_MAX);
    ASSERT_EQ(saturatedAddition(INT_MAX - 1, 2), INT_MAX);
    ASSERT_EQ(saturatedAddition(0, INT_MAX - 1), INT_MAX - 1);
    ASSERT_EQ(saturatedAddition(1, INT_MAX - 1), INT_MAX);
    ASSERT_EQ(saturatedAddition(2, INT_MAX - 1), INT_MAX);
    ASSERT_EQ(saturatedAddition(INT_MAX - 1, INT_MAX - 1), INT_MAX);
    ASSERT_EQ(saturatedAddition(INT_MAX, INT_MAX), INT_MAX);

    ASSERT_EQ(saturatedAddition(INT_MIN, 0), INT_MIN);
    ASSERT_EQ(saturatedAddition(INT_MIN + 1, 0), INT_MIN + 1);
    ASSERT_EQ(saturatedAddition(INT_MIN + 1, 1), INT_MIN + 2);
    ASSERT_EQ(saturatedAddition(INT_MIN + 1, 2), INT_MIN + 3);
    ASSERT_EQ(saturatedAddition(INT_MIN + 1, -1), INT_MIN);
    ASSERT_EQ(saturatedAddition(INT_MIN + 1, -2), INT_MIN);
    ASSERT_EQ(saturatedAddition(0, INT_MIN + 1), INT_MIN + 1);
    ASSERT_EQ(saturatedAddition(-1, INT_MIN + 1), INT_MIN);
    ASSERT_EQ(saturatedAddition(-2, INT_MIN + 1), INT_MIN);

    ASSERT_EQ(saturatedAddition(INT_MAX / 2, 10000), INT_MAX / 2 + 10000);
    ASSERT_EQ(saturatedAddition(INT_MAX / 2 + 1, INT_MAX / 2 + 1), INT_MAX);
    ASSERT_EQ(saturatedAddition(INT_MIN, INT_MAX), -1);
}

TEST(WTF, SaturatedArithmeticSubtraction)
{
    ASSERT_EQ(saturatedSubtraction(0, 0), 0);
    ASSERT_EQ(saturatedSubtraction(0, 1), -1);
    ASSERT_EQ(saturatedSubtraction(0, 100), -100);
    ASSERT_EQ(saturatedSubtraction(100, 50), 50);
    
    ASSERT_EQ(saturatedSubtraction(0, -1), 1);
    ASSERT_EQ(saturatedSubtraction(1, -1), 2);
    ASSERT_EQ(saturatedSubtraction(100, -50), 150);
    ASSERT_EQ(saturatedSubtraction(50, -100), 150);

    ASSERT_EQ(saturatedSubtraction(INT_MAX, 0), INT_MAX);
    ASSERT_EQ(saturatedSubtraction(INT_MAX, 1), INT_MAX - 1);
    ASSERT_EQ(saturatedSubtraction(INT_MAX - 1, 0), INT_MAX - 1);
    ASSERT_EQ(saturatedSubtraction(INT_MAX - 1, -1), INT_MAX);
    ASSERT_EQ(saturatedSubtraction(INT_MAX - 1, -2), INT_MAX);
    ASSERT_EQ(saturatedSubtraction(0, INT_MAX - 1), -INT_MAX + 1);
    ASSERT_EQ(saturatedSubtraction(-1, INT_MAX - 1), -INT_MAX);
    ASSERT_EQ(saturatedSubtraction(-2, INT_MAX - 1), -INT_MAX - 1);
    ASSERT_EQ(saturatedSubtraction(-3, INT_MAX - 1), -INT_MAX - 1);

    ASSERT_EQ(saturatedSubtraction(INT_MIN, 0), INT_MIN);
    ASSERT_EQ(saturatedSubtraction(INT_MIN + 1, 0), INT_MIN + 1);
    ASSERT_EQ(saturatedSubtraction(INT_MIN + 1, 1), INT_MIN);
    ASSERT_EQ(saturatedSubtraction(INT_MIN + 1, 2), INT_MIN);

    ASSERT_EQ(saturatedSubtraction(INT_MIN, INT_MIN), 0);
    ASSERT_EQ(saturatedSubtraction(INT_MAX, INT_MAX), 0);
    ASSERT_EQ(saturatedSubtraction(INT_MAX, INT_MIN), INT_MAX);
}

} // namespace TestWebKitAPI
