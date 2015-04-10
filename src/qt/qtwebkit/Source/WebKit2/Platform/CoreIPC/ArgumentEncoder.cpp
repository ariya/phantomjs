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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ArgumentEncoder.h"

#include "DataReference.h"
#include <algorithm>
#include <stdio.h>

#if OS(DARWIN)
#include <sys/mman.h>
#endif

namespace CoreIPC {

static inline void* allocBuffer(size_t size)
{
#if OS(DARWIN)
    return mmap(0, size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
#else
    return fastMalloc(size);
#endif
}

static inline void freeBuffer(void* addr, size_t size)
{
#if OS(DARWIN)
    munmap(addr, size);
#else
    UNUSED_PARAM(size);
    fastFree(addr);
#endif
}

PassOwnPtr<ArgumentEncoder> ArgumentEncoder::create()
{
    return adoptPtr(new ArgumentEncoder);
}

ArgumentEncoder::ArgumentEncoder()
    : m_buffer(m_inlineBuffer)
    , m_bufferPointer(m_inlineBuffer)
    , m_bufferSize(0)
    , m_bufferCapacity(sizeof(m_inlineBuffer))
{
}

ArgumentEncoder::~ArgumentEncoder()
{
    if (m_buffer != m_inlineBuffer)
        freeBuffer(m_buffer, m_bufferCapacity);

#if !USE(UNIX_DOMAIN_SOCKETS)
    // FIXME: We need to dispose of the attachments in cases of failure.
#else
    for (size_t i = 0; i < m_attachments.size(); ++i)
        m_attachments[i].dispose();
#endif
}

static inline size_t roundUpToAlignment(size_t value, unsigned alignment)
{
    return ((value + alignment - 1) / alignment) * alignment;
}

uint8_t* ArgumentEncoder::grow(unsigned alignment, size_t size)
{
    size_t alignedSize = roundUpToAlignment(m_bufferSize, alignment);
    
    if (alignedSize + size > m_bufferCapacity) {
        size_t newCapacity = roundUpToAlignment(m_bufferCapacity * 2, 4096);
        while (newCapacity < alignedSize + size)
            newCapacity *= 2;

        uint8_t* newBuffer = static_cast<uint8_t*>(allocBuffer(newCapacity));
        if (!newBuffer)
            CRASH();

        memcpy(newBuffer, m_buffer, m_bufferSize);

        if (m_buffer != m_inlineBuffer)
            freeBuffer(m_buffer, m_bufferCapacity);

        m_buffer = newBuffer;
        m_bufferCapacity = newCapacity;
    }

    m_bufferSize = alignedSize + size;
    m_bufferPointer = m_buffer + alignedSize + size;
    
    return m_buffer + alignedSize;
}

void ArgumentEncoder::encodeFixedLengthData(const uint8_t* data, size_t size, unsigned alignment)
{
    ASSERT(!(reinterpret_cast<uintptr_t>(data) % alignment));

    uint8_t* buffer = grow(alignment, size);
    memcpy(buffer, data, size);
}

void ArgumentEncoder::encodeVariableLengthByteArray(const DataReference& dataReference)
{
    encode(static_cast<uint64_t>(dataReference.size()));
    encodeFixedLengthData(dataReference.data(), dataReference.size(), 1);
}

void ArgumentEncoder::encode(bool n)
{
    uint8_t* buffer = grow(sizeof(n), sizeof(n));
    
    *reinterpret_cast<bool*>(buffer) = n;
}

void ArgumentEncoder::encode(uint8_t n)
{
    uint8_t* buffer = grow(sizeof(n), sizeof(n));

    *reinterpret_cast<uint8_t*>(buffer) = n;
}

void ArgumentEncoder::encode(uint16_t n)
{
    uint8_t* buffer = grow(sizeof(n), sizeof(n));

    *reinterpret_cast_ptr<uint16_t*>(buffer) = n;
}

void ArgumentEncoder::encode(uint32_t n)
{
    uint8_t* buffer = grow(sizeof(n), sizeof(n));
    
    *reinterpret_cast_ptr<uint32_t*>(buffer) = n;
}

void ArgumentEncoder::encode(uint64_t n)
{
    uint8_t* buffer = grow(sizeof(n), sizeof(n));
    
    *reinterpret_cast_ptr<uint64_t*>(buffer) = n;
}

void ArgumentEncoder::encode(int32_t n)
{
    uint8_t* buffer = grow(sizeof(n), sizeof(n));
    
    *reinterpret_cast_ptr<int32_t*>(buffer) = n;
}

void ArgumentEncoder::encode(int64_t n)
{
    uint8_t* buffer = grow(sizeof(n), sizeof(n));
    
    *reinterpret_cast_ptr<int64_t*>(buffer) = n;
}

void ArgumentEncoder::encode(float n)
{
    uint8_t* buffer = grow(sizeof(n), sizeof(n));

    *reinterpret_cast_ptr<float*>(buffer) = n;
}

void ArgumentEncoder::encode(double n)
{
    uint8_t* buffer = grow(sizeof(n), sizeof(n));

    *reinterpret_cast_ptr<double*>(buffer) = n;
}

void ArgumentEncoder::addAttachment(const Attachment& attachment)
{
    m_attachments.append(attachment);
}

Vector<Attachment> ArgumentEncoder::releaseAttachments()
{
    Vector<Attachment> newList;
    newList.swap(m_attachments);
    return newList;
}

} // namespace CoreIPC
