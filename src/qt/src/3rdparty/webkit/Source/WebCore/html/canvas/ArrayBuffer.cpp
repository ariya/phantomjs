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
#include "ArrayBuffer.h"

#include <wtf/RefPtr.h>

namespace WebCore {

PassRefPtr<ArrayBuffer> ArrayBuffer::create(unsigned numElements, unsigned elementByteSize)
{
    void* data = tryAllocate(numElements, elementByteSize);
    if (!data)
        return 0;
    return adoptRef(new ArrayBuffer(data, numElements * elementByteSize));
}

PassRefPtr<ArrayBuffer> ArrayBuffer::create(ArrayBuffer* other)
{
    return ArrayBuffer::create(other->data(), other->byteLength());
}

PassRefPtr<ArrayBuffer> ArrayBuffer::create(void* source, unsigned byteLength)
{
    void* data = tryAllocate(byteLength, 1);
    if (!data)
        return 0;
    RefPtr<ArrayBuffer> buffer = adoptRef(new ArrayBuffer(data, byteLength));
    memcpy(buffer->data(), source, byteLength);
    return buffer.release();
}

ArrayBuffer::ArrayBuffer(void* data, unsigned sizeInBytes)
    : m_sizeInBytes(sizeInBytes)
    , m_data(data)
{
}

void* ArrayBuffer::data()
{
    return m_data;
}

const void* ArrayBuffer::data() const
{
    return m_data;
}

unsigned ArrayBuffer::byteLength() const
{
    return m_sizeInBytes;
}

ArrayBuffer::~ArrayBuffer()
{
    WTF::fastFree(m_data);
}

void* ArrayBuffer::tryAllocate(unsigned numElements, unsigned elementByteSize)
{
    void* result;
    // Do not allow 32-bit overflow of the total size.
    // FIXME: Why not? The tryFastCalloc function already checks its arguments,
    // and will fail if there is any overflow, so why should we include a
    // redudant unnecessarily restrictive check here?
    if (numElements) {
        unsigned totalSize = numElements * elementByteSize;
        if (totalSize / numElements != elementByteSize)
            return 0;
    }
    if (WTF::tryFastCalloc(numElements, elementByteSize).getValue(result))
        return result;
    return 0;
}

}
