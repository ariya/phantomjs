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

#ifndef FastBitVector_h
#define FastBitVector_h

#include <wtf/FastMalloc.h>
#include <wtf/OwnArrayPtr.h>
#include <wtf/PassOwnArrayPtr.h>
#include <wtf/StdLibExtras.h>

namespace WTF {

class FastBitVector {
public:
    FastBitVector()
        : m_array(0)
        , m_numBits(0)
    {
    }
    
    FastBitVector(const FastBitVector& other)
        : m_array(0)
        , m_numBits(0)
    {
        *this = other;
    }
    
    ~FastBitVector()
    {
        if (m_array)
            fastFree(m_array);
    }
    
    FastBitVector& operator=(const FastBitVector& other)
    {
        size_t length = other.arrayLength();
        uint32_t* newArray = static_cast<uint32_t*>(fastCalloc(length, 4));
        memcpy(newArray, other.m_array, length * 4);
        if (m_array)
            fastFree(m_array);
        m_array = newArray;
        m_numBits = other.m_numBits;
        return *this;
    }
    
    size_t numBits() const { return m_numBits; }
    
    void resize(size_t numBits)
    {
        // Use fastCalloc instead of fastRealloc because we expect the common
        // use case for this method to be initializing the size of the bitvector.
        
        size_t newLength = (numBits + 31) >> 5;
        uint32_t* newArray = static_cast<uint32_t*>(fastCalloc(newLength, 4));
        memcpy(newArray, m_array, arrayLength() * 4);
        if (m_array)
            fastFree(m_array);
        m_array = newArray;
        m_numBits = numBits;
    }
    
    void setAll()
    {
        memset(m_array, 255, arrayLength() * 4);
    }
    
    void clearAll()
    {
        memset(m_array, 0, arrayLength() * 4);
    }
    
    void set(const FastBitVector& other)
    {
        ASSERT(m_numBits == other.m_numBits);
        memcpy(m_array, other.m_array, arrayLength() * 4);
    }
    
    bool setAndCheck(const FastBitVector& other)
    {
        bool changed = false;
        ASSERT(m_numBits == other.m_numBits);
        for (unsigned i = arrayLength(); i--;) {
            if (m_array[i] == other.m_array[i])
                continue;
            m_array[i] = other.m_array[i];
            changed = true;
        }
        return changed;
    }
    
    bool equals(const FastBitVector& other) const
    {
        ASSERT(m_numBits == other.m_numBits);
        // Use my own comparison loop because memcmp does more than what I want
        // and bcmp is not as standard.
        for (unsigned i = arrayLength(); i--;) {
            if (m_array[i] != other.m_array[i])
                return false;
        }
        return true;
    }
    
    void merge(const FastBitVector& other)
    {
        ASSERT(m_numBits == other.m_numBits);
        for (unsigned i = arrayLength(); i--;)
            m_array[i] |= other.m_array[i];
    }
    
    void filter(const FastBitVector& other)
    {
        ASSERT(m_numBits == other.m_numBits);
        for (unsigned i = arrayLength(); i--;)
            m_array[i] &= other.m_array[i];
    }
    
    void exclude(const FastBitVector& other)
    {
        ASSERT(m_numBits == other.m_numBits);
        for (unsigned i = arrayLength(); i--;)
            m_array[i] &= ~other.m_array[i];
    }
    
    void set(size_t i)
    {
        ASSERT_WITH_SECURITY_IMPLICATION(i < m_numBits);
        m_array[i >> 5] |= (1 << (i & 31));
    }
    
    void clear(size_t i)
    {
        ASSERT_WITH_SECURITY_IMPLICATION(i < m_numBits);
        m_array[i >> 5] &= ~(1 << (i & 31));
    }
    
    void set(size_t i, bool value)
    {
        if (value)
            set(i);
        else
            clear(i);
    }
    
    bool get(size_t i) const
    {
        ASSERT_WITH_SECURITY_IMPLICATION(i < m_numBits);
        return !!(m_array[i >> 5] & (1 << (i & 31)));
    }
private:
    size_t arrayLength() const { return (m_numBits + 31) >> 5; }
    
    uint32_t* m_array; // No, this can't be an OwnArrayPtr.
    size_t m_numBits;
};

} // namespace WTF

using WTF::FastBitVector;

#endif // FastBitVector_h

