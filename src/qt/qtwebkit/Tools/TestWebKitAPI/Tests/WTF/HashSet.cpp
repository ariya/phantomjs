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

#include <wtf/HashSet.h>

namespace TestWebKitAPI {

template<int initialCapacity>
    struct InitialCapacityTestHashTraits : public WTF::UnsignedWithZeroKeyHashTraits<int> {
    static const int minimumTableSize = initialCapacity;
};

template<unsigned size>
void testInitialCapacity()
{
    const unsigned initialCapacity = WTF::HashTableCapacityForSize<size>::value;
    HashSet<int, DefaultHash<int>::Hash, InitialCapacityTestHashTraits<initialCapacity> > testSet;

    // Initial capacity is null.
    ASSERT_EQ(0, testSet.capacity());

    // Adding items up to size should never change the capacity.
    for (size_t i = 0; i < size; ++i) {
        testSet.add(i);
        ASSERT_EQ(initialCapacity, static_cast<unsigned>(testSet.capacity()));
    }

    // Adding items up to less than half the capacity should not change the capacity.
    unsigned capacityLimit = initialCapacity / 2 - 1;
    for (size_t i = size; i < capacityLimit; ++i) {
        testSet.add(i);
        ASSERT_EQ(initialCapacity, static_cast<unsigned>(testSet.capacity()));
    }

    // Adding one more item increase the capacity.
    testSet.add(initialCapacity);
    EXPECT_GT(static_cast<unsigned>(testSet.capacity()), initialCapacity);
}

template<unsigned size> void generateTestCapacityUpToSize();
template<> void generateTestCapacityUpToSize<0>()
{
}
template<unsigned size> void generateTestCapacityUpToSize()
{
    generateTestCapacityUpToSize<size - 1>();
    testInitialCapacity<size>();
}

TEST(WTF, HashSetInitialCapacity)
{
    generateTestCapacityUpToSize<128>();
}

} // namespace TestWebKitAPI
