/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
#include "ResourceBuffer.h"

#include "PurgeableBuffer.h"

namespace WebCore {

ResourceBuffer::ResourceBuffer()
    : m_sharedBuffer(SharedBuffer::create())
{
}

ResourceBuffer::ResourceBuffer(const char* data, int size)
    : m_sharedBuffer(SharedBuffer::create(data, size))
{
}

ResourceBuffer::ResourceBuffer(PassRefPtr<SharedBuffer> sharedBuffer)
    : m_sharedBuffer(sharedBuffer)
{
    ASSERT(m_sharedBuffer);
}
    
ResourceBuffer::~ResourceBuffer()
{
}

const char* ResourceBuffer::data() const
{
    return m_sharedBuffer->data();
}

unsigned ResourceBuffer::size() const
{
    return m_sharedBuffer->size();
}

bool ResourceBuffer::isEmpty() const
{
    return m_sharedBuffer->isEmpty();
}

void ResourceBuffer::append(const char* data, unsigned size)
{
    m_sharedBuffer->append(data, size);
}

void ResourceBuffer::append(SharedBuffer* buffer)
{
    m_sharedBuffer->append(buffer);
}

#if USE(NETWORK_CFDATA_ARRAY_CALLBACK)
void ResourceBuffer::append(CFDataRef data)
{
    ASSERT(m_sharedBuffer);
    m_sharedBuffer->append(data);
}
#endif

void ResourceBuffer::clear()
{
    m_sharedBuffer->clear();
}

unsigned ResourceBuffer::getSomeData(const char*& data, unsigned position) const
{
    return m_sharedBuffer->getSomeData(data, position);
}

SharedBuffer* ResourceBuffer::sharedBuffer() const
{
    // Currently all ResourceBuffers are backed by SharedBuffers.
    // In the future we might have to create the SharedBuffer on demand here.
    // We should also phase out as much use of this accessor as possible and have clients
    // either use the ResourceBuffer directly or use getSomeData() when sensical.
    return m_sharedBuffer.get();
}

PassRefPtr<ResourceBuffer> ResourceBuffer::copy() const
{
    return ResourceBuffer::adoptSharedBuffer(m_sharedBuffer->copy());
}

bool ResourceBuffer::hasPurgeableBuffer() const
{
    return m_sharedBuffer->hasPurgeableBuffer();
}

void ResourceBuffer::createPurgeableBuffer() const
{
    ASSERT(m_sharedBuffer);
    if (!sharedBuffer()->hasOneRef())
        return;
    sharedBuffer()->createPurgeableBuffer();
}

PassOwnPtr<PurgeableBuffer> ResourceBuffer::releasePurgeableBuffer()
{
    return m_sharedBuffer->releasePurgeableBuffer();
}

#if USE(CF)
CFDataRef ResourceBuffer::createCFData()
{
    return m_sharedBuffer->createCFData();
}
#endif

} // namespace WebCore
