/*
 * Copyright (C) 2009, 2010 Apple Inc. All rights reserved.
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

#ifndef StringBuilder_h
#define StringBuilder_h

#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WTF {

class StringBuilder {
public:
    StringBuilder()
        : m_length(0)
    {
    }

    void append(const UChar*, unsigned);
    void append(const char*, unsigned);

    void append(const String& string)
    {
        // If we're appending to an empty string, and there is not buffer
        // (in case reserveCapacity has been called) then just retain the
        // string.
        if (!m_length && !m_buffer) {
            m_string = string;
            m_length = string.length();
            return;
        }
        append(string.characters(), string.length());
    }

    void append(const char* characters)
    {
        if (characters)
            append(characters, strlen(characters));
    }

    void append(UChar c)
    {
        if (m_buffer && m_length < m_buffer->length() && m_string.isNull())
            m_bufferCharacters[m_length++] = c;
        else
            append(&c, 1);
    }

    void append(char c)
    {
        if (m_buffer && m_length < m_buffer->length() && m_string.isNull())
            m_bufferCharacters[m_length++] = (unsigned char)c;
        else
            append(&c, 1);
    }

    String toString()
    {
        if (m_string.isNull()) {
            shrinkToFit();
            reifyString();
        }
        return m_string;
    }

    String toStringPreserveCapacity()
    {
        if (m_string.isNull())
            reifyString();
        return m_string;
    }

    unsigned length() const
    {
        return m_length;
    }

    bool isEmpty() const { return !length(); }

    void reserveCapacity(unsigned newCapacity);

    void resize(unsigned newSize);

    void shrinkToFit();

    UChar operator[](unsigned i) const
    {
        ASSERT(i < m_length);
        if (!m_string.isNull())
            return m_string[i];
        ASSERT(m_buffer);
        return m_buffer->characters()[i];
    }

    void clear()
    {
        m_length = 0;
        m_string = String();
        m_buffer = 0;
    }

private:
    void allocateBuffer(const UChar* currentCharacters, unsigned requiredLength);
    UChar* appendUninitialized(unsigned length);
    void reifyString();

    unsigned m_length;
    String m_string;
    RefPtr<StringImpl> m_buffer;
    UChar* m_bufferCharacters;
};

} // namespace WTF

using WTF::StringBuilder;

#endif // StringBuilder_h
