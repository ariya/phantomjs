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

#import "config.h"
#import "InstanceMethodSwizzler.h"

#import <wtf/RetainPtr.h>

@interface SimpleObject : NSObject
- (void)setValue:(int*)value;
@end

@implementation SimpleObject
- (void)setValue:(int*)value
{
    *value = 1;
}
@end

namespace TestWebKitAPI {

static void setValue2(id self, SEL _cmd, int* value)
{
    *value = 2;
}

static void setValue3(id self, SEL _cmd, int* value)
{
    *value = 3;
}

TEST(TestWebKitAPI, InstanceMethodSwizzler)
{
    RetainPtr<SimpleObject> object = adoptNS([[SimpleObject alloc] init]);

    int value = 0;

    [object.get() setValue:&value];
    EXPECT_EQ(value, 1);

    {
        InstanceMethodSwizzler swizzle([object.get() class], @selector(setValue:), reinterpret_cast<IMP>(setValue2));

        [object.get() setValue:&value];
        EXPECT_EQ(value, 2);

        {
            InstanceMethodSwizzler swizzle([object.get() class], @selector(setValue:), reinterpret_cast<IMP>(setValue3));

            [object.get() setValue:&value];
            EXPECT_EQ(value, 3);
        }

        [object.get() setValue:&value];
        EXPECT_EQ(value, 2);
    }

    [object.get() setValue:&value];
    EXPECT_EQ(value, 1);
}

} // namespace TestWebKitAPI
