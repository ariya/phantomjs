/*
 * Copyright (C) 2006, 2008 Apple Inc. All rights reserved.
 * Copyright (C) Research In Motion Limited 2009-2010. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "SharedBuffer.h"

#include "PurgeableBuffer.h"
#include <wtf/PassOwnPtr.h>
#include <wtf/unicode/UTF8.h>
#include <wtf/unicode/Unicode.h>

using namespace std;

namespace WebCore {

static const unsigned segmentSize = 0x1000;
static const unsigned segmentPositionMask = 0x0FFF;

static inline unsigned segmentIndex(unsigned position)
{
    return position / segmentSize;
}

static inline unsigned offsetInSegment(unsigned position)
{
    return position & segmentPositionMask;
}

static inline char* allocateSegment()
{
    return static_cast<char*>(fastMalloc(segmentSize));
}

static inline void freeSegment(char* p)
{
    fastFree(p);
}

SharedBuffer::SharedBuffer()
    : m_size(0)
{
}

SharedBuffer::SharedBuffer(size_t size)
    : m_size(size)
    , m_buffer(size)
{
}

SharedBuffer::SharedBuffer(const char* data, int size)
    : m_size(0)
{
    // FIXME: Use unsigned consistently, and check for invalid casts when calling into SharedBuffer from other code.
    if (size < 0)
        CRASH();

    append(data, size);
}

SharedBuffer::SharedBuffer(const unsigned char* data, int size)
    : m_size(0)
{
    // FIXME: Use unsigned consistently, and check for invalid casts when calling into SharedBuffer from other code.
    if (size < 0)
        CRASH();

    append(reinterpret_cast<const char*>(data), size);
}
    
SharedBuffer::~SharedBuffer()
{
    clear();
}

PassRefPtr<SharedBuffer> SharedBuffer::adoptVector(Vector<char>& vector)
{
    RefPtr<SharedBuffer> buffer = create();
    buffer->m_buffer.swap(vector);
    buffer->m_size = buffer->m_buffer.size();
    return buffer.release();
}

PassRefPtr<SharedBuffer> SharedBuffer::adoptPurgeableBuffer(PassOwnPtr<PurgeableBuffer> purgeableBuffer) 
{ 
    ASSERT(!purgeableBuffer->isPurgeable());
    RefPtr<SharedBuffer> buffer = create();
    buffer->m_purgeableBuffer = purgeableBuffer;
    return buffer.release();
}

unsigned SharedBuffer::size() const
{
    if (hasPlatformData())
        return platformDataSize();
    
    if (m_purgeableBuffer)
        return m_purgeableBuffer->size();
    
    return m_size;
}

void SharedBuffer::createPurgeableBuffer() const
{
    if (m_purgeableBuffer)
        return;

    if (hasPlatformData())
        return;

#if USE(NETWORK_CFDATA_ARRAY_CALLBACK)
    if (singleDataArrayBuffer())
        return;
#endif

    m_purgeableBuffer = PurgeableBuffer::create(buffer().data(), m_size);
}

const char* SharedBuffer::data() const
{
    if (hasPlatformData())
        return platformData();

#if USE(NETWORK_CFDATA_ARRAY_CALLBACK)
    if (const char* buffer = singleDataArrayBuffer())
        return buffer;
#endif
    
    if (m_purgeableBuffer)
        return m_purgeableBuffer->data();
    
    return this->buffer().data();
}

void SharedBuffer::append(SharedBuffer* data)
{
    const char* segment;
    size_t position = 0;
    while (size_t length = data->getSomeData(segment, position)) {
        append(segment, length);
        position += length;
    }
}

void SharedBuffer::append(const char* data, unsigned length)
{
    ASSERT(!m_purgeableBuffer);
    if (!length)
        return;

    maybeTransferPlatformData();
    
    unsigned positionInSegment = offsetInSegment(m_size - m_buffer.size());
    m_size += length;

    if (m_size <= segmentSize) {
        // No need to use segments for small resource data
        if (m_buffer.isEmpty())
            m_buffer.reserveInitialCapacity(length);
        m_buffer.append(data, length);
        return;
    }

    char* segment;
    if (!positionInSegment) {
        segment = allocateSegment();
        m_segments.append(segment);
    } else
        segment = m_segments.last() + positionInSegment;

    unsigned segmentFreeSpace = segmentSize - positionInSegment;
    unsigned bytesToCopy = min(length, segmentFreeSpace);

    for (;;) {
        memcpy(segment, data, bytesToCopy);
        if (static_cast<unsigned>(length) == bytesToCopy)
            break;

        length -= bytesToCopy;
        data += bytesToCopy;
        segment = allocateSegment();
        m_segments.append(segment);
        bytesToCopy = min(length, segmentSize);
    }
}

void SharedBuffer::append(const Vector<char>& data)
{
    append(data.data(), data.size());
}

void SharedBuffer::clear()
{
    clearPlatformData();
    
    for (unsigned i = 0; i < m_segments.size(); ++i)
        freeSegment(m_segments[i]);

    m_segments.clear();
    m_size = 0;

    m_buffer.clear();
    m_purgeableBuffer.clear();
#if USE(NETWORK_CFDATA_ARRAY_CALLBACK)
    m_dataArray.clear();
#endif
}

PassRefPtr<SharedBuffer> SharedBuffer::copy() const
{
    RefPtr<SharedBuffer> clone(adoptRef(new SharedBuffer));
    if (m_purgeableBuffer || hasPlatformData()) {
        clone->append(data(), size());
        return clone;
    }

    clone->m_size = m_size;
    clone->m_buffer.reserveCapacity(m_size);
    clone->m_buffer.append(m_buffer.data(), m_buffer.size());
    for (unsigned i = 0; i < m_segments.size(); ++i)
        clone->m_buffer.append(m_segments[i], segmentSize);
    return clone;
}

PassOwnPtr<PurgeableBuffer> SharedBuffer::releasePurgeableBuffer()
{ 
    ASSERT(hasOneRef()); 
    return m_purgeableBuffer.release(); 
}

const Vector<char>& SharedBuffer::buffer() const
{
    unsigned bufferSize = m_buffer.size();
    if (m_size > bufferSize) {
        m_buffer.resize(m_size);
        char* destination = m_buffer.data() + bufferSize;
        unsigned bytesLeft = m_size - bufferSize;
        for (unsigned i = 0; i < m_segments.size(); ++i) {
            unsigned bytesToCopy = min(bytesLeft, segmentSize);
            memcpy(destination, m_segments[i], bytesToCopy);
            destination += bytesToCopy;
            bytesLeft -= bytesToCopy;
            freeSegment(m_segments[i]);
        }
        m_segments.clear();
#if USE(NETWORK_CFDATA_ARRAY_CALLBACK)
        copyDataArrayAndClear(destination, bytesLeft);
#endif
    }
    return m_buffer;
}

unsigned SharedBuffer::getSomeData(const char*& someData, unsigned position) const
{
    unsigned totalSize = size();
    if (position >= totalSize) {
        someData = 0;
        return 0;
    }

    if (hasPlatformData() || m_purgeableBuffer) {
        ASSERT_WITH_SECURITY_IMPLICATION(position < size());
        someData = data() + position;
        return totalSize - position;
    }

    ASSERT_WITH_SECURITY_IMPLICATION(position < m_size);
    unsigned consecutiveSize = m_buffer.size();
    if (position < consecutiveSize) {
        someData = m_buffer.data() + position;
        return consecutiveSize - position;
    }
 
    position -= consecutiveSize;
    unsigned segments = m_segments.size();
    unsigned maxSegmentedSize = segments * segmentSize;
    unsigned segment = segmentIndex(position);
    if (segment < segments) {
        unsigned bytesLeft = totalSize - consecutiveSize;
        unsigned segmentedSize = min(maxSegmentedSize, bytesLeft);

        unsigned positionInSegment = offsetInSegment(position);
        someData = m_segments[segment] + positionInSegment;
        return segment == segments - 1 ? segmentedSize - position : segmentSize - positionInSegment;
    }
#if USE(NETWORK_CFDATA_ARRAY_CALLBACK)
    ASSERT(maxSegmentedSize <= position);
    position -= maxSegmentedSize;
    return copySomeDataFromDataArray(someData, position);
#else
    ASSERT_NOT_REACHED();
    return 0;
#endif
}

#if !USE(CF) || PLATFORM(QT)

inline void SharedBuffer::clearPlatformData()
{
}

inline void SharedBuffer::maybeTransferPlatformData()
{
}

inline bool SharedBuffer::hasPlatformData() const
{
    return false;
}

inline const char* SharedBuffer::platformData() const
{
    ASSERT_NOT_REACHED();

    return 0;
}

inline unsigned SharedBuffer::platformDataSize() const
{
    ASSERT_NOT_REACHED();
    
    return 0;
}

#endif

PassRefPtr<SharedBuffer> utf8Buffer(const String& string)
{
    // Allocate a buffer big enough to hold all the characters.
    const int length = string.length();
    Vector<char> buffer(length * 3);

    // Convert to runs of 8-bit characters.
    char* p = buffer.data();
    const UChar* d = string.characters();
    WTF::Unicode::ConversionResult result = WTF::Unicode::convertUTF16ToUTF8(&d, d + length, &p, p + buffer.size(), true);
    if (result != WTF::Unicode::conversionOK)
        return 0;

    buffer.shrink(p - buffer.data());
    return SharedBuffer::adoptVector(buffer);
}

} // namespace WebCore
