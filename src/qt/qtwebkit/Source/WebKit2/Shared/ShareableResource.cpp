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
#include "ShareableResource.h"

#include "ArgumentCoders.h"
#include <WebCore/SharedBuffer.h>

using namespace WebCore;

namespace WebKit {

ShareableResource::Handle::Handle()
{
}

void ShareableResource::Handle::encode(CoreIPC::ArgumentEncoder& encoder) const
{
    encoder << m_handle;
    encoder << m_offset;
    encoder << m_size;
}

bool ShareableResource::Handle::decode(CoreIPC::ArgumentDecoder& decoder, Handle& handle)
{
    if (!decoder.decode(handle.m_handle))
        return false;
    if (!decoder.decode(handle.m_offset))
        return false;
    if (!decoder.decode(handle.m_size))
        return false;
    return true;
}

static void shareableResourceDeallocate(void *ptr, void *info)
{
    (static_cast<ShareableResource*>(info))->deref(); // Balanced by ref() in createShareableResourceDeallocator()
}
    
static CFAllocatorRef createShareableResourceDeallocator(ShareableResource* resource)
{
    resource->ref(); // Balanced by deref in shareableResourceDeallocate()

    CFAllocatorContext context = { 0,
        resource,
        NULL, // retain
        NULL, // release
        NULL, // copyDescription
        NULL, // allocate
        NULL, // reallocate
        shareableResourceDeallocate,
        NULL, // preferredSize
    };

    return CFAllocatorCreate(kCFAllocatorDefault, &context);
}

PassRefPtr<SharedBuffer> ShareableResource::Handle::tryWrapInSharedBuffer() const
{
    RefPtr<ShareableResource> resource = ShareableResource::create(*this);
    if (!resource) {
        LOG_ERROR("Failed to recreate ShareableResource from handle.");
        return 0;
    }

    RetainPtr<CFAllocatorRef> deallocator = adoptCF(createShareableResourceDeallocator(resource.get()));
    RetainPtr<CFDataRef> data = adoptCF(CFDataCreateWithBytesNoCopy(kCFAllocatorDefault, reinterpret_cast<const UInt8*>(resource->data()), static_cast<CFIndex>(resource->size()), deallocator.get()));

    return SharedBuffer::wrapCFData(data.get());
}
    
PassRefPtr<ShareableResource> ShareableResource::create(PassRefPtr<SharedMemory> sharedMemory, unsigned offset, unsigned size)
{
    return adoptRef(new ShareableResource(sharedMemory, offset, size));
}

PassRefPtr<ShareableResource> ShareableResource::create(const Handle& handle)
{
    RefPtr<SharedMemory> sharedMemory = SharedMemory::create(handle.m_handle, SharedMemory::ReadOnly);
    if (!sharedMemory)
        return 0;

    return create(sharedMemory.release(), handle.m_offset, handle.m_size);
}

ShareableResource::ShareableResource(PassRefPtr<SharedMemory> sharedMemory, unsigned offset, unsigned size)
    : m_sharedMemory(sharedMemory)
    , m_offset(offset)
    , m_size(size)
{
    ASSERT(m_sharedMemory);
    ASSERT(m_offset + m_size <= m_sharedMemory->size());
    
    // FIXME (NetworkProcess): This data was received from another process.  If it is bogus, should we assume that process is compromised and we should kill it?
}

ShareableResource::~ShareableResource()
{
}

bool ShareableResource::createHandle(Handle& handle)
{
    if (!m_sharedMemory->createHandle(handle.m_handle, SharedMemory::ReadOnly))
        return false;

    handle.m_offset = m_offset;
    handle.m_size = m_size;

    return true;
}

const char* ShareableResource::data() const
{
    return static_cast<const char*>(m_sharedMemory->data()) + m_offset;
}

unsigned ShareableResource::size() const
{
    return m_size;
}
    
} // namespace WebKit
