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
#include <wtf/Vector.h>

namespace JSC {

class JSStringBuilder {
public:
    JSStringBuilder()
        : m_okay(true)
        , m_is8Bit(true)
    {
    }

    void append(const UChar u)
    {
        if (m_is8Bit) {
            if (u < 0xff) {
                LChar c = u;
                m_okay &= buffer8.tryAppend(&c, 1);
                return;
            }
            upConvert();
        }
        m_okay &= buffer16.tryAppend(&u, 1);
    }

    void append(const char* str)
    {
        append(str, strlen(str));
    }

    void append(const char* str, size_t len)
    {
        if (m_is8Bit) {
            m_okay &= buffer8.tryAppend(reinterpret_cast<const LChar*>(str), len);
            return;
        }
        m_okay &= buffer8.tryReserveCapacity(buffer16.size() + len);
        for (size_t i = 0; i < len; i++) {
            UChar u = static_cast<unsigned char>(str[i]);
            m_okay &= buffer16.tryAppend(&u, 1);
        }
    }

    void append(const LChar* str, size_t len)
    {
        if (m_is8Bit) {
            m_okay &= buffer8.tryAppend(str, len);
            return;
        }
        m_okay &= buffer8.tryReserveCapacity(buffer16.size() + len);
        for (size_t i = 0; i < len; i++) {
            UChar u = str[i];
            m_okay &= buffer16.tryAppend(&u, 1);
        }
    }
    
    void append(const UChar* str, size_t len)
    {
        if (m_is8Bit)
            upConvert(); // FIXME: We could check character by character its size.
        m_okay &= buffer16.tryAppend(str, len);
    }

    void append(const String& str)
    {
        unsigned length = str.length();

        if (!length)
            return;

        if (m_is8Bit) {
            if (str.is8Bit()) {
                m_okay &= buffer8.tryAppend(str.characters8(), length);
                return;
            }
            upConvert();
        }
        m_okay &= buffer16.tryAppend(str.characters(), length);
    }

    void upConvert()
    {
        ASSERT(m_is8Bit);
        size_t len = buffer8.size();

        for (size_t i = 0; i < len; i++)
            buffer16.append(buffer8[i]);

        buffer8.clear();
        m_is8Bit = false;
    }

    JSValue build(ExecState* exec)
    {
        if (!m_okay)
            return throwOutOfMemoryError(exec);
        if (m_is8Bit) {
            buffer8.shrinkToFit();
            if (!buffer8.data())
                return throwOutOfMemoryError(exec);
            return jsString(exec, String::adopt(buffer8));
        }
        buffer16.shrinkToFit();
        if (!buffer16.data())
            return throwOutOfMemoryError(exec);
        return jsString(exec, String::adopt(buffer16));
    }

protected:
    Vector<LChar, 64, UnsafeVectorOverflow> buffer8;
    Vector<UChar, 64, UnsafeVectorOverflow> buffer16;
    bool m_okay;
    bool m_is8Bit;
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
