/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#include "config.h"

#include <wtf/HashSet.h>
#include <wtf/HashMap.h>
#include <wtf/RetainPtr.h>

namespace TestWebKitAPI {

TEST(RetainPtrHashing, HashSet)
{
    HashSet<RetainPtr<CFStringRef> > set;

    RetainPtr<CFStringRef> foo = adoptCF(CFStringCreateWithCString(kCFAllocatorDefault, "foo", kCFStringEncodingUTF8));

    EXPECT_FALSE(set.contains(foo));
    set.add(foo);
    EXPECT_TRUE(set.contains(foo));

    RetainPtr<CFStringRef> foo2 = adoptCF(CFStringCreateWithCString(kCFAllocatorDefault, "foo", kCFStringEncodingUTF8));
    EXPECT_FALSE(set.contains(foo2));
    set.add(foo2);
    EXPECT_TRUE(set.contains(foo2));

    set.remove(foo);
    EXPECT_FALSE(set.contains(foo));
}

TEST(RetainPtrHashing, HashMapKey)
{
    HashMap<RetainPtr<CFStringRef>, int> map;

    RetainPtr<CFStringRef> foo = adoptCF(CFStringCreateWithCString(kCFAllocatorDefault, "foo", kCFStringEncodingUTF8));

    EXPECT_FALSE(map.contains(foo));
    map.add(foo, 1);
    EXPECT_EQ(1, map.get(foo));

    RetainPtr<CFStringRef> foo2 = adoptCF(CFStringCreateWithCString(kCFAllocatorDefault, "foo", kCFStringEncodingUTF8));
    EXPECT_FALSE(map.contains(foo2));
    map.add(foo2, 2);
    EXPECT_EQ(2, map.get(foo2));

    map.remove(foo);
    EXPECT_FALSE(map.contains(foo));
}

TEST(RetainPtrHashing, HashMapValue)
{
    HashMap<int, RetainPtr<CFStringRef> > map;

    RetainPtr<CFStringRef> foo = adoptCF(CFStringCreateWithCString(kCFAllocatorDefault, "foo", kCFStringEncodingUTF8));

    EXPECT_FALSE(map.contains(1));
    map.add(1, foo);
    EXPECT_EQ(foo, map.get(1));

    RetainPtr<CFStringRef> foo2 = adoptCF(CFStringCreateWithCString(kCFAllocatorDefault, "foo", kCFStringEncodingUTF8));
    EXPECT_FALSE(map.contains(2));
    map.add(2, foo2);
    EXPECT_EQ(foo2, map.get(2));
}

} // namespace TestWebKitAPI
