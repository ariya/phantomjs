/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef CopiedAllocator_h
#define CopiedAllocator_h

#include "CopiedBlock.h"
#include <wtf/CheckedBoolean.h>
#include <wtf/DataLog.h>

namespace JSC {

class CopiedAllocator {
public:
    CopiedAllocator();
    
    bool fastPathShouldSucceed(size_t bytes) const;
    CheckedBoolean tryAllocate(size_t bytes, void** outPtr);
    CheckedBoolean tryReallocate(void *oldPtr, size_t oldBytes, size_t newBytes);
    void* forceAllocate(size_t bytes);
    CopiedBlock* resetCurrentBlock();
    void setCurrentBlock(CopiedBlock*);
    size_t currentCapacity();
    
    bool isValid() { return !!m_currentBlock; }

    CopiedBlock* currentBlock() { return m_currentBlock; }

    // Yes, these are public. No, that doesn't mean you can play with them.
    // If I had made them private then I'd have to list off all of the JIT
    // classes and functions that are entitled to modify these directly, and
    // that would have been gross.
    size_t m_currentRemaining;
    char* m_currentPayloadEnd;
    CopiedBlock* m_currentBlock; 
};

inline CopiedAllocator::CopiedAllocator()
    : m_currentRemaining(0)
    , m_currentPayloadEnd(0)
    , m_currentBlock(0)
{
}

inline bool CopiedAllocator::fastPathShouldSucceed(size_t bytes) const
{
    ASSERT(is8ByteAligned(reinterpret_cast<void*>(bytes)));
    
    return bytes <= m_currentRemaining;
}

inline CheckedBoolean CopiedAllocator::tryAllocate(size_t bytes, void** outPtr)
{
    ASSERT(is8ByteAligned(reinterpret_cast<void*>(bytes)));
    
    // This code is written in a gratuitously low-level manner, in order to
    // serve as a kind of template for what the JIT would do. Note that the
    // way it's written it ought to only require one register, which doubles
    // as the result, provided that the compiler does a minimal amount of
    // control flow simplification and the bytes argument is a constant.
    
    size_t currentRemaining = m_currentRemaining;
    if (bytes > currentRemaining)
        return false;
    currentRemaining -= bytes;
    m_currentRemaining = currentRemaining;
    *outPtr = m_currentPayloadEnd - currentRemaining - bytes;

    ASSERT(is8ByteAligned(*outPtr));

    return true;
}

inline CheckedBoolean CopiedAllocator::tryReallocate(
    void* oldPtr, size_t oldBytes, size_t newBytes)
{
    ASSERT(is8ByteAligned(oldPtr));
    ASSERT(is8ByteAligned(reinterpret_cast<void*>(oldBytes)));
    ASSERT(is8ByteAligned(reinterpret_cast<void*>(newBytes)));
    
    ASSERT(newBytes > oldBytes);
    
    size_t additionalBytes = newBytes - oldBytes;
    
    size_t currentRemaining = m_currentRemaining;
    if (m_currentPayloadEnd - currentRemaining - oldBytes != static_cast<char*>(oldPtr))
        return false;
    
    if (additionalBytes > currentRemaining)
        return false;
    
    m_currentRemaining = currentRemaining - additionalBytes;
    
    return true;
}

inline void* CopiedAllocator::forceAllocate(size_t bytes)
{
    void* result = 0; // Needed because compilers don't realize this will always be assigned.
    CheckedBoolean didSucceed = tryAllocate(bytes, &result);
    ASSERT(didSucceed);
    return result;
}

inline CopiedBlock* CopiedAllocator::resetCurrentBlock()
{
    CopiedBlock* result = m_currentBlock;
    if (result) {
        result->m_remaining = m_currentRemaining;
        m_currentBlock = 0;
        m_currentRemaining = 0;
        m_currentPayloadEnd = 0;
    }
    return result;
}

inline void CopiedAllocator::setCurrentBlock(CopiedBlock* newBlock)
{
    ASSERT(!m_currentBlock);
    m_currentBlock = newBlock;
    ASSERT(newBlock);
    m_currentRemaining = newBlock->m_remaining;
    m_currentPayloadEnd = newBlock->payloadEnd();
}

inline size_t CopiedAllocator::currentCapacity()
{
    if (!m_currentBlock)
        return 0;
    return m_currentBlock->capacity();
}

} // namespace JSC

#endif
