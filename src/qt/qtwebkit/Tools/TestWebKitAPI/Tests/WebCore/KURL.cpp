/*
 * Copyright (C) 2011, 2012 Apple Inc. All rights reserved.
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
#include "WTFStringUtilities.h"
#include <WebCore/KURL.h>
#include <wtf/MainThread.h>

using namespace WebCore;

namespace TestWebKitAPI {

class KURLTest : public testing::Test {
public:
    virtual void SetUp()
    {
        WTF::initializeMainThread();
    }
};

TEST_F(KURLTest, KURLConstructorDefault)
{
    KURL kurl;

    EXPECT_TRUE(kurl.isEmpty());
    EXPECT_TRUE(kurl.isNull());
    EXPECT_FALSE(kurl.isValid());
}

TEST_F(KURLTest, KURLConstructorConstChar)
{
    KURL kurl(ParsedURLString, "http://username:password@www.example.com:8080/index.html?var=val#fragment");

    EXPECT_FALSE(kurl.isEmpty());
    EXPECT_FALSE(kurl.isNull());
    EXPECT_TRUE(kurl.isValid());

    EXPECT_EQ(String("http"), kurl.protocol());
    EXPECT_EQ(String("www.example.com"), kurl.host());
    EXPECT_TRUE(kurl.hasPort());
    EXPECT_EQ(8080, kurl.port());
    EXPECT_EQ(String("username"), kurl.user());
    EXPECT_EQ(String("password"), kurl.pass());
    EXPECT_EQ(String("/index.html"), kurl.path());
    EXPECT_EQ(String("index.html"), kurl.lastPathComponent());
    EXPECT_EQ(String("var=val"), kurl.query());
    EXPECT_TRUE(kurl.hasFragmentIdentifier());
    EXPECT_EQ(String("fragment"), kurl.fragmentIdentifier());
}

TEST_F(KURLTest, KURLDataURIStringSharing)
{
    KURL baseURL(ParsedURLString, "http://www.webkit.org/");
    String threeApples = "data:text/plain;charset=utf-8;base64,76O/76O/76O/";

    KURL url(baseURL, threeApples);
    EXPECT_EQ(threeApples.impl(), url.string().impl());
}

} // namespace TestWebKitAPI
