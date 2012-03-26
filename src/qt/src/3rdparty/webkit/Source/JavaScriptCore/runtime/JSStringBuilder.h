/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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

#ifndef JSStringBuilder_h
#define JSStringBuilder_h

#include "ExceptionHelpers.h"
#include "JSString.h"
#include "UStringConcatenate.h"
#include "Vector.h"

namespace JSC {

class JSStringBuilder {
public:
    JSStringBuilder()
        : m_okay(true)
    {
    }

    void append(const UChar u)
    {
        m_okay &= buffer.tryAppend(&u, 1);
    }

    void append(const char* str)
    {
        append(str, strlen(str));
    }

    void append(const char* str, size_t len)
    {
        m_okay &= buffer.tryReserveCapacity(buffer.size() + len);
        for (size_t i = 0; i < len; i++) {
            UChar u = static_cast<unsigned char>(str[i]);
            m_okay &= buffer.tryAppend(&u, 1);
        }
    }

    void append(const UChar* str, size_t len)
    {
        m_okay &= buffer.tryAppend(str, len);
    }

    void append(const UString& str)
    {
        m_okay &= buffer.tryAppend(str.characters(), str.length());
    }

    JSValue build(ExecState* exec)
    {
        if (!m_okay)
            return throwOutOfMemoryError(exec);
        buffer.shrinkToFit();
        if (!buffer.data())
            return throwOutOfMemoryError(exec);
        return jsString(exec, UString::adopt(buffer));
    }

protected:
    Vector<UChar, 64> buffer;
    bool m_okay;
};

template<typename StringType1, typename StringType2>
inline JSValue jsMakeNontrivialString(ExecState* exec, StringType1 string1, StringType2 string2)
{
    PassRefPtr<StringImpl> result = WTF::tryMakeString(string1, string2);
    if (!result)
        return throwOutOfMemoryError(exec);
    return jsNontrivialString(exec, result);
}

template<typename StringType1, typename StringType2, typename StringType3>
inline JSValue jsMakeNontrivialString(ExecState* exec, StringType1 string1, StringType2 string2, StringType3 string3)
{
    PassRefPtr<StringImpl> result = WTF::tryMakeString(string1, string2, string3);
    if (!result)
        return throwOutOfMemoryError(exec);
    return jsNontrivialString(exec, result);
}

template<typename StringType1, typename StringType2, typename StringType3, typename StringType4>
inline JSValue jsMakeNontrivialString(ExecState* exec, StringType1 string1, StringType2 string2, StringType3 string3, StringType4 string4)
{
    PassRefPtr<StringImpl> result = WTF::tryMakeString(string1, string2, string3, string4);
    if (!result)
        return throwOutOfMemoryError(exec);
    return jsNontrivialString(exec, result);
}

template<typename StringType1, typename StringType2, typename StringType3, typename StringType4, typename StringType5>
inline JSValue jsMakeNontrivialString(ExecState* exec, StringType1 string1, StringType2 string2, StringType3 string3, StringType4 string4, StringType5 string5)
{
    PassRefPtr<StringImpl> result = WTF::tryMakeString(string1, string2, string3, string4, string5);
    if (!result)
        return throwOutOfMemoryError(exec);
    return jsNontrivialString(exec, result);
}

template<typename StringType1, typename StringType2, typename StringType3, typename StringType4, typename StringType5, typename StringType6>
inline JSValue jsMakeNontrivialString(ExecState* exec, StringType1 string1, StringType2 string2, StringType3 string3, StringType4 string4, StringType5 string5, StringType6 string6)
{
    PassRefPtr<StringImpl> result = WTF::tryMakeString(string1, string2, string3, string4, string5, string6);
    if (!result)
        return throwOutOfMemoryError(exec);
    return jsNontrivialString(exec, result);
}

}

#endif
