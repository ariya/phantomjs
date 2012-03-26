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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef UStringConcatenate_h
#define UStringConcatenate_h

#include "UString.h"
#include <wtf/text/StringConcatenate.h>

namespace WTF {

template<>
class StringTypeAdapter<JSC::UString> {
public:
    StringTypeAdapter<JSC::UString>(JSC::UString& string)
        : m_data(string.characters())
        , m_length(string.length())
    {
    }

    unsigned length() { return m_length; }

    void writeTo(UChar* destination)
    {
        for (unsigned i = 0; i < m_length; ++i)
            destination[i] = m_data[i];
    }

private:
    const UChar* m_data;
    unsigned m_length;
};

}; // namespace WTF

namespace JSC {

template<typename StringType1, typename StringType2>
UString makeUString(StringType1 string1, StringType2 string2)
{
    PassRefPtr<StringImpl> resultImpl = WTF::tryMakeString(string1, string2);
    if (!resultImpl)
        CRASH();
    return resultImpl;
}

template<typename StringType1, typename StringType2, typename StringType3>
UString makeUString(StringType1 string1, StringType2 string2, StringType3 string3)
{
    PassRefPtr<StringImpl> resultImpl = WTF::tryMakeString(string1, string2, string3);
    if (!resultImpl)
        CRASH();
    return resultImpl;
}

template<typename StringType1, typename StringType2, typename StringType3, typename StringType4>
UString makeUString(StringType1 string1, StringType2 string2, StringType3 string3, StringType4 string4)
{
    PassRefPtr<StringImpl> resultImpl = WTF::tryMakeString(string1, string2, string3, string4);
    if (!resultImpl)
        CRASH();
    return resultImpl;
}

template<typename StringType1, typename StringType2, typename StringType3, typename StringType4, typename StringType5>
UString makeUString(StringType1 string1, StringType2 string2, StringType3 string3, StringType4 string4, StringType5 string5)
{
    PassRefPtr<StringImpl> resultImpl = WTF::tryMakeString(string1, string2, string3, string4, string5);
    if (!resultImpl)
        CRASH();
    return resultImpl;
}

template<typename StringType1, typename StringType2, typename StringType3, typename StringType4, typename StringType5, typename StringType6>
UString makeUString(StringType1 string1, StringType2 string2, StringType3 string3, StringType4 string4, StringType5 string5, StringType6 string6)
{
    PassRefPtr<StringImpl> resultImpl = WTF::tryMakeString(string1, string2, string3, string4, string5, string6);
    if (!resultImpl)
        CRASH();
    return resultImpl;
}

template<typename StringType1, typename StringType2, typename StringType3, typename StringType4, typename StringType5, typename StringType6, typename StringType7>
UString makeUString(StringType1 string1, StringType2 string2, StringType3 string3, StringType4 string4, StringType5 string5, StringType6 string6, StringType7 string7)
{
    PassRefPtr<StringImpl> resultImpl = WTF::tryMakeString(string1, string2, string3, string4, string5, string6, string7);
    if (!resultImpl)
        CRASH();
    return resultImpl;
}

template<typename StringType1, typename StringType2, typename StringType3, typename StringType4, typename StringType5, typename StringType6, typename StringType7, typename StringType8>
UString makeUString(StringType1 string1, StringType2 string2, StringType3 string3, StringType4 string4, StringType5 string5, StringType6 string6, StringType7 string7, StringType8 string8)
{
    PassRefPtr<StringImpl> resultImpl = WTF::tryMakeString(string1, string2, string3, string4, string5, string6, string7, string8);
    if (!resultImpl)
        CRASH();
    return resultImpl;
}

} // namespace JSC

#endif
