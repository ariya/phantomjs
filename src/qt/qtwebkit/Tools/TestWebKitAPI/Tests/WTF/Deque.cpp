/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
#include <wtf/Deque.h>

namespace TestWebKitAPI {

TEST(WTF, DequeIterator)
{
    Deque<int> deque;
    deque.append(11);
    deque.prepend(10);
    deque.append(12);
    deque.append(13);

    Deque<int>::iterator it = deque.begin();
    Deque<int>::iterator end = deque.end();
    EXPECT_TRUE(end != it);

    EXPECT_EQ(10, *it);
    ++it;
    EXPECT_EQ(11, *it);
    ++it;
    EXPECT_EQ(12, *it);
    ++it;
    EXPECT_EQ(13, *it);
    ++it;

    EXPECT_TRUE(end == it);
}

TEST(WTF, DequeReverseIterator)
{
    Deque<int> deque;
    deque.append(11);
    deque.prepend(10);
    deque.append(12);
    deque.append(13);

    Deque<int>::reverse_iterator it = deque.rbegin();
    Deque<int>::reverse_iterator end = deque.rend();
    EXPECT_TRUE(end != it);

    EXPECT_EQ(13, *it);
    ++it;
    EXPECT_EQ(12, *it);
    ++it;
    EXPECT_EQ(11, *it);
    ++it;
    EXPECT_EQ(10, *it);
    ++it;

    EXPECT_TRUE(end == it);
}

TEST(WTF, DequeRemove)
{
    Deque<int> deque;
    deque.append(11);
    deque.prepend(10);
    deque.append(12);
    deque.append(13);

    EXPECT_EQ(10, deque.first());
    EXPECT_EQ(13, deque.last());

    deque.removeLast();
    EXPECT_EQ(10, deque.first());
    EXPECT_EQ(12, deque.last());

    deque.removeFirst();
    EXPECT_EQ(11, deque.first());
    EXPECT_EQ(12, deque.last());

    deque.removeFirst();
    EXPECT_EQ(12, deque.first());
    EXPECT_EQ(12, deque.last());

    deque.removeLast();
    EXPECT_TRUE(deque.isEmpty());
}

} // namespace TestWebKitAPI
