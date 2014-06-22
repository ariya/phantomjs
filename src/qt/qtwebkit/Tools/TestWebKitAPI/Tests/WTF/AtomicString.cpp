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

#include <wtf/text/AtomicString.h>

namespace TestWebKitAPI {

TEST(WTF, AtomicStringCreationFromLiteral)
{
    AtomicString stringWithTemplate("Template Literal", AtomicString::ConstructFromLiteral);
    ASSERT_EQ(strlen("Template Literal"), stringWithTemplate.length());
    ASSERT_TRUE(stringWithTemplate == "Template Literal");
    ASSERT_TRUE(stringWithTemplate.string().is8Bit());
    ASSERT_TRUE(stringWithTemplate.impl()->hasTerminatingNullCharacter());

    const char* programmaticStringData = "Explicit Size Literal";
    AtomicString programmaticString(programmaticStringData, strlen(programmaticStringData), AtomicString::ConstructFromLiteral);
    ASSERT_EQ(strlen(programmaticStringData), programmaticString.length());
    ASSERT_TRUE(programmaticStringData == programmaticStringData);
    ASSERT_TRUE(programmaticString.string().is8Bit());
    ASSERT_TRUE(programmaticString.impl()->hasTerminatingNullCharacter());
    ASSERT_EQ(programmaticStringData, reinterpret_cast<const char*>(programmaticString.string().characters8()));
}

TEST(WTF, AtomicStringCreationFromLiteralUniqueness)
{
    AtomicString string1("Template Literal", AtomicString::ConstructFromLiteral);
    AtomicString string2("Template Literal", AtomicString::ConstructFromLiteral);
    ASSERT_EQ(string1.impl(), string2.impl());

    AtomicString string3("Template Literal");
    ASSERT_EQ(string1.impl(), string3.impl());
}

} // namespace TestWebKitAPI
