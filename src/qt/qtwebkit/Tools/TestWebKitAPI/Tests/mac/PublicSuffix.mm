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

#if USE(PUBLIC_SUFFIX_LIST)

#include "WTFStringUtilities.h"
#include <WebCore/PublicSuffix.h>
#include <wtf/MainThread.h>

using namespace WebCore;

namespace TestWebKitAPI {

class PublicSuffix: public testing::Test {
public:
    virtual void SetUp()
    {
        WTF::initializeMainThread();
        [WebView initialize];
    }
};

TEST_F(PublicSuffix, IsPublicSuffix)
{
    EXPECT_TRUE(isPublicSuffix("com"));
    EXPECT_FALSE(isPublicSuffix("test.com"));
    EXPECT_FALSE(isPublicSuffix("com.com"));
    EXPECT_TRUE(isPublicSuffix("net"));
    EXPECT_TRUE(isPublicSuffix("org"));
    EXPECT_TRUE(isPublicSuffix("co.uk"));
    EXPECT_FALSE(isPublicSuffix("bl.uk"));
    EXPECT_FALSE(isPublicSuffix("test.co.uk"));
    EXPECT_TRUE(isPublicSuffix("xn--zf0ao64a.tw"));
}

TEST_F(PublicSuffix, TopPrivatelyControlledDomain)
{
    EXPECT_EQ(String("test.com"), topPrivatelyControlledDomain("test.com"));
    EXPECT_EQ(String("test.com"), topPrivatelyControlledDomain("com.test.com"));
    EXPECT_EQ(String("test.com"), topPrivatelyControlledDomain("subdomain.test.com"));
    EXPECT_EQ(String("com.com"), topPrivatelyControlledDomain("www.com.com"));
    EXPECT_EQ(String("test.co.uk"), topPrivatelyControlledDomain("test.co.uk"));
    EXPECT_EQ(String("test.co.uk"), topPrivatelyControlledDomain("subdomain.test.co.uk"));
    EXPECT_EQ(String("bl.uk"), topPrivatelyControlledDomain("bl.uk"));
    EXPECT_EQ(String("bl.uk"), topPrivatelyControlledDomain("subdomain.bl.uk"));
    EXPECT_EQ(String("test.xn--zf0ao64a.tw"), topPrivatelyControlledDomain("test.xn--zf0ao64a.tw"));
    EXPECT_EQ(String("test.xn--zf0ao64a.tw"), topPrivatelyControlledDomain("www.test.xn--zf0ao64a.tw"));
    EXPECT_EQ(String("127.0.0.1"), topPrivatelyControlledDomain("127.0.0.1"));
    EXPECT_EQ(String(), topPrivatelyControlledDomain("1"));
    EXPECT_EQ(String(), topPrivatelyControlledDomain("com"));
}

}

#endif
