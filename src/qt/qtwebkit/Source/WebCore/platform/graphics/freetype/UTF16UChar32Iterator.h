/*
 * Copyright (C) 2013 Igalia S.L.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY IGALIA AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef UTF16UChar32Iterator_h
#define UTF16UChar32Iterator_h

namespace WebCore {

class UTF16UChar32Iterator {
    WTF_MAKE_NONCOPYABLE(UTF16UChar32Iterator);
public:
    UTF16UChar32Iterator(const UChar* buffer, unsigned bufferLength)
        : m_buffer(buffer)
        , m_bufferLength(bufferLength)
        , m_currentOffset(0)
    {
    }

    static UChar32 end()
    {
        return static_cast<UChar32>(-1);
    }

    UChar32 next()
    {
        if (m_currentOffset >= m_bufferLength)
            return end();

        const UChar32 character = m_buffer[m_currentOffset++];
        if (U16_IS_SURROGATE_LEAD(character)
            && m_currentOffset != m_bufferLength
            && U16_IS_TRAIL(m_buffer[m_currentOffset])) {
            return U16_GET_SUPPLEMENTARY(character, m_buffer[m_currentOffset++]);
        }

        return character;
    }

private:
    const UChar* m_buffer;
    unsigned m_bufferLength;
    unsigned m_currentOffset;
};

} // namespace WebCore


#endif // UTF16UChar32Iterator_h
