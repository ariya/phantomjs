// Copyright 2010, Google Inc. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef URLBuffer_h
#define URLBuffer_h

namespace WTF {

// Base class for the canonicalizer output, this maintains a buffer and
// supports simple resizing and append operations on it.
//
// It is VERY IMPORTANT that no virtual function calls be made on the common
// code path. We only have two virtual function calls, the destructor and a
// resize function that is called when the existing buffer is not big enough.
// The derived class is then in charge of setting up our buffer which we will
// manage.
template<typename CHAR>
class URLBuffer {
public:
    URLBuffer() : m_buffer(0), m_capacity(0), m_length(0) { }
    virtual ~URLBuffer() { }

    // Implemented to resize the buffer. This function should update the buffer
    // pointer to point to the new buffer, and any old data up to |m_length| in
    // the buffer must be copied over.
    //
    // The new size must be larger than m_capacity.
    virtual void resize(int) = 0;

    inline char at(int offset) const { return m_buffer[offset]; }
    inline void set(int offset, CHAR ch)
    {
        // FIXME: Add ASSERT(offset < length());
        m_buffer[offset] = ch;
    }

    // Returns the current capacity of the buffer. The length() is the number of
    // characters that have been declared to be written, but the capacity() is
    // the number that can be written without reallocation. If the caller must
    // write many characters at once, it can make sure there is enough capacity,
    // write the data, then use setLength() to declare the new length().
    int capacity() const { return m_capacity; }
    int length() const { return m_length; }

    // The output will NOT be 0-terminated. Call length() to get the length.
    const CHAR* data() const { return m_buffer; }
    CHAR* data() { return m_buffer; }

    // Shortens the URL to the new length. Used for "backing up" when processing
    // relative paths. This can also be used if an external function writes a lot
    // of data to the buffer (when using the "Raw" version below) beyond the end,
    // to declare the new length.
    void setLength(int length)
    {
        // FIXME: Add ASSERT(length < capacity());
        m_length = length;
    }

    // This is the most performance critical function, since it is called for
    // every character.
    void append(CHAR ch)
    {
        // In VC2005, putting this common case first speeds up execution
        // dramatically because this branch is predicted as taken.
        if (m_length < m_capacity) {
            m_buffer[m_length] = ch;
            ++m_length;
            return;
        }

        if (!grow(1))
            return;

        m_buffer[m_length] = ch;
        ++m_length;
    }

    void append(const CHAR* str, int strLength)
    {
        if (m_length + strLength > m_capacity) {
            if (!grow(m_length + strLength - m_capacity))
                return;
        }
        for (int i = 0; i < strLength; i++)
            m_buffer[m_length + i] = str[i];
        m_length += strLength;
    }

protected:
    // Returns true if the buffer could be resized, false on OOM.
    bool grow(int minimumAdditionalCapacity)
    {
        static const int minimumCapacity = 16;
        int newCapacity = m_capacity ? m_capacity : minimumCapacity;
        do {
            if (newCapacity >= (1 << 30)) // Prevent overflow below.
                return false;
            newCapacity *= 2;
        } while (newCapacity < m_capacity + minimumAdditionalCapacity);
        resize(newCapacity);
        return true;
    }

    CHAR* m_buffer;
    int m_capacity;
    int m_length; // Used characters in the buffer.
};

} // namespace WTF

#endif // URLBuffer_h
