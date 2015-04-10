/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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

#if ENABLE(PURGEABLE_MEMORY)

#include "PurgeableBuffer.h"

#include <mach/mach.h>
#include <wtf/Assertions.h>
#include <wtf/VMTags.h>

namespace WebCore {

// Purgeable buffers are allocated in multiples of the page size (4KB in common CPUs) so
// it does not make sense for very small buffers. Set our minimum size to 16KB.
static const size_t minPurgeableBufferSize = 4 * 4096;

PurgeableBuffer::PurgeableBuffer(char* data, size_t size)
    : m_data(data)
    , m_size(size)
    , m_purgePriority(PurgeDefault)
    , m_state(NonVolatile)
{
}

PurgeableBuffer::~PurgeableBuffer()
{
    vm_deallocate(mach_task_self(), reinterpret_cast<vm_address_t>(m_data), m_size);
}

PassOwnPtr<PurgeableBuffer> PurgeableBuffer::create(const char* data, size_t size)
{
    if (size < minPurgeableBufferSize)
        return nullptr;

    vm_address_t buffer = 0;
    kern_return_t ret = vm_allocate(mach_task_self(), &buffer, size, VM_FLAGS_PURGABLE | VM_FLAGS_ANYWHERE | VM_TAG_FOR_WEBCORE_PURGEABLE_MEMORY);

    ASSERT(ret == KERN_SUCCESS);
    if (ret != KERN_SUCCESS)
        return nullptr;

    memcpy(reinterpret_cast<char*>(buffer), data, size);

    return adoptPtr(new PurgeableBuffer(reinterpret_cast<char*>(buffer), size));
}

bool PurgeableBuffer::makePurgeable(bool purgeable)
{
    if (purgeable) {
        if (m_state != NonVolatile)
            return true;

        int volatileGroup;
        if (m_purgePriority == PurgeFirst)
            volatileGroup = VM_VOLATILE_GROUP_0;
        else if (m_purgePriority == PurgeMiddle)
            volatileGroup = VM_VOLATILE_GROUP_4;
        else
            volatileGroup = VM_VOLATILE_GROUP_7;
        
        int state = VM_PURGABLE_VOLATILE | volatileGroup;
        // So apparently "purgeable" is the correct spelling and the API here is misspelled.
        kern_return_t ret = vm_purgable_control(mach_task_self(), reinterpret_cast<vm_address_t>(m_data), VM_PURGABLE_SET_STATE, &state);
        
        if (ret != KERN_SUCCESS) {
            // If that failed we have no clue what state we are in so assume purged.
            m_state = Purged;
            return true;
        }
        
        m_state = Volatile;
        return true;
    }

    if (m_state == NonVolatile)
        return true;
    if (m_state == Purged)
        return false;
    
    int state = VM_PURGABLE_NONVOLATILE;
    kern_return_t ret = vm_purgable_control(mach_task_self(), reinterpret_cast<vm_address_t>(m_data), VM_PURGABLE_SET_STATE, &state);

    if (ret != KERN_SUCCESS) {
        // If that failed we have no clue what state we are in so assume purged.
        m_state = Purged;
        return false;
    }

    m_state = state & VM_PURGABLE_EMPTY ? Purged : NonVolatile;
    return m_state == NonVolatile;
}
    
bool PurgeableBuffer::wasPurged() const
{
    if (m_state == NonVolatile)
        return false;
    if (m_state == Purged)
        return true;

    int state;
    kern_return_t ret = vm_purgable_control(mach_task_self(), reinterpret_cast<vm_address_t>(m_data), VM_PURGABLE_GET_STATE, &state);

    if (ret != KERN_SUCCESS) {
        // If that failed we have no clue what state we are in so assume purged.
        m_state = Purged;
        return true;        
    }

    if (state & VM_PURGABLE_EMPTY) {
        m_state = Purged;
        return true;
    }
        
    return false;
}

const char* PurgeableBuffer::data() const
{
    ASSERT(m_state == NonVolatile);
    return m_data;
}
    
}

#endif
