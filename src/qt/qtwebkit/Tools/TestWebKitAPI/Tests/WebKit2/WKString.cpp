/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

namespace TestWebKitAPI {

TEST(WebKit2, WKString)
{
    WKStringRef string = WKStringCreateWithUTF8CString("hello");
    EXPECT_TRUE(!WKStringIsEmpty(string));
    EXPECT_TRUE(WKStringIsEqual(string, string));
    EXPECT_TRUE(WKStringIsEqualToUTF8CString(string, "hello"));
    EXPECT_EQ(16u, WKStringGetMaximumUTF8CStringSize(string));

    size_t maxSize = WKStringGetMaximumUTF8CStringSize(string);
    char* buffer = new char[maxSize];

    size_t actualSize = WKStringGetUTF8CString(string, buffer, maxSize);
    EXPECT_EQ(6u, actualSize);
    EXPECT_STREQ("hello", buffer);

    delete[] buffer;
    
    maxSize = WKStringGetLength(string);
    EXPECT_EQ(5u, maxSize);

    // Allocate a buffer one character larger than we need.
    WKChar* uniBuffer = new WKChar[maxSize+1];
    actualSize = WKStringGetCharacters(string, uniBuffer, maxSize);
    EXPECT_EQ(5u, actualSize);
    
    WKChar helloBuffer[] = { 'h', 'e', 'l', 'l', 'o' };
    EXPECT_TRUE(!memcmp(uniBuffer, helloBuffer, 10));
    
    // Test passing a buffer length < the string length.
    actualSize = WKStringGetCharacters(string, uniBuffer, maxSize - 1);
    EXPECT_EQ(4u, actualSize);
    
    // Test passing a buffer length > the string length.
    actualSize = WKStringGetCharacters(string, uniBuffer, maxSize + 1);
    EXPECT_EQ(5u, actualSize);
    
    delete[] uniBuffer;
    
    WKRelease(string);
}

} // namespace TestWebKitAPI
