/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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

#include "UnitTestUtils/EWK2UnitTestBase.h"
#include "WKEinaSharedString.h"
#include <WebKit2/WKString.h>
#include <WebKit2/WKURL.h>

using namespace EWK2UnitTest;

extern EWK2UnitTestEnvironment* environment;

// Use macro here instead of global variables in order not to have always the same pointers.
#define testString "I'm test string!"
#define anotherTestString "I'm another test string!"
#define testUrl "file:///path/somewhere"

class EWK2EinaSharedStringTest : public EWK2UnitTestBase {
protected:
    void checkString(const WKEinaSharedString& string, const char* pattern)
    {
        ASSERT_EQ(string.isNull(), pattern ? false : true);
        ASSERT_EQ(string.length(), pattern ? strlen(pattern) : 0); // Compare length.
        ASSERT_EQ(string, pattern); // Compare values. Check '==' operator with WKEinaSharedString and plain string.
        ASSERT_STREQ(string, pattern); // Compare values. Check 'const char*' operator.
    }
};

TEST_F(EWK2EinaSharedStringTest, constructEmpty)
{
    WKEinaSharedString emptyString;
    checkString(emptyString, 0);
}

TEST_F(EWK2EinaSharedStringTest, constructFromPlainString)
{
    WKEinaSharedString emptyString(testString);
    checkString(emptyString, testString);
}

TEST_F(EWK2EinaSharedStringTest, constructFromWKString)
{
    WKEinaSharedString string(AdoptWK, WKStringCreateWithUTF8CString(testString));
    checkString(string, testString);
}

TEST_F(EWK2EinaSharedStringTest, constructFromWKURL)
{
    WKEinaSharedString string(AdoptWK, WKURLCreateWithUTF8CString(testUrl));
    checkString(string, testUrl);
}

TEST_F(EWK2EinaSharedStringTest, constructFromEinaStringShare)
{
    WKEinaSharedString string(WKEinaSharedString::adopt(eina_stringshare_add(testString)));
    checkString(string, testString);

    string = WKEinaSharedString::adopt(eina_stringshare_add(anotherTestString));
    checkString(string, anotherTestString);

    string = string;
    checkString(string, anotherTestString);
}

TEST_F(EWK2EinaSharedStringTest, costructCopy)
{
    WKEinaSharedString string(testString);
    WKEinaSharedString copyString(string);
    checkString(string, testString);
    checkString(copyString, testString);
    ASSERT_EQ(string, copyString); // Check '==' operator with two instances of WKEinaSharedString.
}

TEST_F(EWK2EinaSharedStringTest, comparisonOperators)
{
    WKEinaSharedString string(testString);
    WKEinaSharedString sameString(testString);
    WKEinaSharedString anotherString(anotherTestString);

    ASSERT_EQ(string, sameString); // Check '==' operator with two instances of WKEinaSharedString.
    ASSERT_NE(string, anotherString); // Check '!=' operator with two instances of WKEinaSharedString.

    const char* explicitlySharedString = eina_stringshare_add(testString);
    ASSERT_EQ(static_cast<const char*>(string), explicitlySharedString); // Compare pointers.
    ASSERT_STREQ(string, explicitlySharedString); // Compare values.
    eina_stringshare_del(explicitlySharedString);

    ASSERT_EQ(string, string); // Self-comparison.
}

TEST_F(EWK2EinaSharedStringTest, assignmentOperators)
{
    WKEinaSharedString string;

    string = testString;
    checkString(string, testString);

    WKEinaSharedString anotherString(anotherTestString);
    string = anotherString;
    checkString(string, anotherTestString);
    ASSERT_EQ(string, anotherString);

    string = string; // Check that self-assignment does not break WKEinaSharedString internal data.
    checkString(string, anotherTestString);
}

TEST_F(EWK2EinaSharedStringTest, leakString)
{
    WKEinaSharedString string;

    string = testString;
    checkString(string, testString);

    Eina_Stringshare* leakedString = string.leakString();
    checkString(string, 0);
    ASSERT_STREQ(leakedString, testString);

    eina_stringshare_del(leakedString);
}
