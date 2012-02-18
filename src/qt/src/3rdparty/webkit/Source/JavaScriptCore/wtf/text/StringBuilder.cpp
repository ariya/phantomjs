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

#include "config.h"
#include "StringBuilder.h"

#include "WTFString.h"

namespace WTF {

void StringBuilder::reifyString()
{
    // Check if the string already exists.
    if (!m_string.isNull()) {
        ASSERT(m_string.length() == m_length);
        return;
    }

    // Check for empty.
    if (!m_length) {
        m_string = StringImpl::empty();
        return;
    }

    // Must be valid in the buffer, take a substring (unless string fills the buffer).
    ASSERT(m_buffer && m_length <= m_buffer->length());
    m_string = (m_length == m_buffer->length())
        ? m_buffer.get()
        : StringImpl::create(m_buffer, 0, m_length);
}

void StringBuilder::resize(unsigned newSize)
{
    // Check newSize < m_length, hence m_length > 0.
    ASSERT(newSize <= m_length);
    if (newSize == m_length)
        return;
    ASSERT(m_length);

    // If there is a buffer, we only need to duplicate it if it has more than one ref.
    if (m_buffer) {
        if (!m_buffer->hasOneRef())
            allocateBuffer(m_buffer->characters(), m_buffer->length());
        m_length = newSize;
        m_string = String();
        return;
    }

    // Since m_length && !m_buffer, the string must be valid in m_string, and m_string.length() > 0.
    ASSERT(!m_string.isEmpty());
    ASSERT(m_length == m_string.length());
    ASSERT(newSize < m_string.length());
    m_length = newSize;
    m_string = StringImpl::create(m_string.impl(), 0, newSize);
}

void StringBuilder::reserveCapacity(unsigned newCapacity)
{
    if (m_buffer) {
        // If there is already a buffer, then grow if necessary.
        if (newCapacity > m_buffer->length())
            allocateBuffer(m_buffer->characters(), newCapacity);
    } else {
        // Grow the string, if necessary.
        if (newCapacity > m_length)
            allocateBuffer(m_string.characters(), newCapacity);
    }
}

// Allocate a new buffer, copying in currentCharacters (these may come from either m_string
// or m_buffer,  neither will be reassigned until the copy has completed).
void StringBuilder::allocateBuffer(const UChar* currentCharacters, unsigned requiredLength)
{
    // Copy the existing data into a new buffer, set result to point to the end of the existing data.
    RefPtr<StringImpl> buffer = StringImpl::createUninitialized(requiredLength, m_bufferCharacters);
    memcpy(m_bufferCharacters, currentCharacters, static_cast<size_t>(m_length) * sizeof(UChar)); // This can't overflow.

    // Update the builder state.
    m_buffer = buffer.release();
    m_string = String();
}

// Make 'length' additional capacity be available in m_buffer, update m_string & m_length,
// return a pointer to the newly allocated storage.
UChar* StringBuilder::appendUninitialized(unsigned length)
{
    ASSERT(length);

    // Calcuate the new size of the builder after appending.
    unsigned requiredLength = length + m_length;
    if (requiredLength < length)
        CRASH();

    if (m_buffer) {
        // If the buffer is valid it must be at least as long as the current builder contents!
        ASSERT(m_buffer->length() >= m_length);

        // Check if the buffer already has sufficient capacity.
        if (requiredLength <= m_buffer->length()) {
            unsigned currentLength = m_length;
            m_string = String();
            m_length = requiredLength;
            return m_bufferCharacters + currentLength;
        }

        // We need to realloc the buffer.
        allocateBuffer(m_buffer->characters(), std::max(requiredLength, m_buffer->length() * 2));
    } else {
        ASSERT(m_string.length() == m_length);
        allocateBuffer(m_string.characters(), std::max(requiredLength, requiredLength * 2));
    }

    UChar* result = m_bufferCharacters + m_length;
    m_length = requiredLength;
    return result;
}

void StringBuilder::append(const UChar* characters, unsigned length)
{
    if (!length)
        return;
    ASSERT(characters);

    memcpy(appendUninitialized(length), characters, static_cast<size_t>(length) * 2);
}

void StringBuilder::append(const char* characters, unsigned length)
{
    if (!length)
        return;
    ASSERT(characters);

    UChar* dest = appendUninitialized(length);
    const char* end = characters + length;
    while (characters < end)
        *(dest++) = *(const unsigned char*)(characters++);
}

void StringBuilder::shrinkToFit()
{
    // If the buffer is at least 80% full, don't bother copying. Need to tune this heuristic!
    if (m_buffer && m_buffer->length() > (m_length + (m_length >> 2))) {
        UChar* result;
        m_string = StringImpl::createUninitialized(m_length, result);
        memcpy(result, m_buffer->characters(), static_cast<size_t>(m_length) * 2); // This can't overflow.
        m_buffer = 0;
    }
}

} // namespace WTF
