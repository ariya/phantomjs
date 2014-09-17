/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CachedMetadata_h
#define CachedMetadata_h

#include <wtf/RefCounted.h>
#include <wtf/Vector.h>

namespace WebCore {

// Metadata retrieved from the embedding application's cache.
//
// Serialized data is NOT portable across architectures. However, reading the
// data type ID will reject data generated with a different byte-order.
class CachedMetadata : public RefCounted<CachedMetadata> {
public:
    static PassRefPtr<CachedMetadata> create(unsigned dataTypeID, const char* data, size_t size)
    {
        return adoptRef(new CachedMetadata(dataTypeID, data, size));
    }

    static PassRefPtr<CachedMetadata> deserialize(const char* data, size_t size)
    {
        return adoptRef(new CachedMetadata(data, size));
    }

    const Vector<char>& serialize() const
    {
        return m_serializedData;
    }

    ~CachedMetadata() { }

    unsigned dataTypeID() const
    {
        return readUnsigned(dataTypeIDStart);
    }

    const char* data() const
    {
        if (m_serializedData.size() < dataStart)
            return 0;
        return m_serializedData.data() + dataStart;
    }

    size_t size() const
    {
        if (m_serializedData.size() < dataStart)
            return 0;
        return m_serializedData.size() - dataStart;
    }

private:
    // Reads an unsigned value at position. Returns 0 on error.
    unsigned readUnsigned(size_t position) const
    {
        if (m_serializedData.size() < position + sizeof(unsigned))
            return 0;
        return *reinterpret_cast_ptr<unsigned*>(const_cast<char*>(m_serializedData.data() + position));
    }

    // Appends an unsigned value to the end of the serialized data.
    void appendUnsigned(unsigned value)
    {
        m_serializedData.append(reinterpret_cast<const char*>(&value), sizeof(unsigned));
    }

    CachedMetadata(const char* data, size_t size)
    {
        // Serialized metadata should have non-empty data.
        ASSERT(size > dataStart);

        m_serializedData.append(data, size);
    }

    CachedMetadata(unsigned dataTypeID, const char* data, size_t size)
    {
        // Don't allow an ID of 0, it is used internally to indicate errors.
        ASSERT(dataTypeID);
        ASSERT(data);

        appendUnsigned(dataTypeID);
        m_serializedData.append(data, size);
    }

    // Serialization offsets. Format: [DATA_TYPE_ID][DATA].
    static const size_t dataTypeIDStart = 0;
    static const size_t dataStart = sizeof(unsigned);

    // Since the serialization format supports random access, storing it in
    // serialized form avoids need for a copy during serialization.
    Vector<char> m_serializedData;
};

}

#endif
