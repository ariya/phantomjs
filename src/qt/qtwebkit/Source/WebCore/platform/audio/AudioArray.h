/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef AudioArray_h
#define AudioArray_h

#include <string.h>
#include <wtf/FastMalloc.h>
#include <wtf/Vector.h>

namespace WebCore {

template<typename T>
class AudioArray {
public:
    AudioArray() : m_allocation(0), m_alignedData(0), m_size(0) { }
    explicit AudioArray(size_t n) : m_allocation(0), m_alignedData(0), m_size(0)
    {
        allocate(n);
    }

    ~AudioArray()
    {
        fastFree(m_allocation);
    }

    // It's OK to call allocate() multiple times, but data will *not* be copied from an initial allocation
    // if re-allocated. Allocations are zero-initialized.
    void allocate(size_t n)
    {
        // Although n is a size_t, its true limit is max unsigned because we use unsigned in zeroRange()
        // and copyToRange(). Also check for integer overflow.
        if (n > std::numeric_limits<unsigned>::max() / sizeof(T))
            CRASH();
      
        unsigned initialSize = sizeof(T) * n;

#if USE(WEBAUDIO_FFMPEG) || USE(WEBAUDIO_OPENMAX_DL_FFT)
        const size_t alignment = 32;
#else
        const size_t alignment = 16;
#endif

        if (m_allocation)
            fastFree(m_allocation);
        
        bool isAllocationGood = false;
        
        while (!isAllocationGood) {
            // Initially we try to allocate the exact size, but if it's not aligned
            // then we'll have to reallocate and from then on allocate extra.
            static size_t extraAllocationBytes = 0;

            // Again, check for integer overflow.
            if (initialSize + extraAllocationBytes < initialSize)
                CRASH();

            T* allocation = static_cast<T*>(fastMalloc(initialSize + extraAllocationBytes));
            if (!allocation)
                CRASH();
            T* alignedData = alignedAddress(allocation, alignment);

            if (alignedData == allocation || extraAllocationBytes == alignment) {
                m_allocation = allocation;
                m_alignedData = alignedData;
                m_size = n;
                isAllocationGood = true;
                zero();
            } else {
                extraAllocationBytes = alignment; // always allocate extra after the first alignment failure.
                fastFree(allocation);
            }
        }
    }

    T* data() { return m_alignedData; }
    const T* data() const { return m_alignedData; }
    size_t size() const { return m_size; }

    T& at(size_t i)
    {
        // Note that although it is a size_t, m_size is now guaranteed to be
        // no greater than max unsigned. This guarantee is enforced in allocate().
        ASSERT_WITH_SECURITY_IMPLICATION(i < size());
        return data()[i];
    }

    T& operator[](size_t i) { return at(i); }

    void zero()
    {
        // This multiplication is made safe by the check in allocate().
        memset(this->data(), 0, sizeof(T) * this->size());
    }

    void zeroRange(unsigned start, unsigned end)
    {
        bool isSafe = (start <= end) && (end <= this->size());
        ASSERT(isSafe);
        if (!isSafe)
            return;

        // This expression cannot overflow because end - start cannot be
        // greater than m_size, which is safe due to the check in allocate().
        memset(this->data() + start, 0, sizeof(T) * (end - start));
    }

    void copyToRange(const T* sourceData, unsigned start, unsigned end)
    {
        bool isSafe = (start <= end) && (end <= this->size());
        ASSERT(isSafe);
        if (!isSafe)
            return;

        // This expression cannot overflow because end - start cannot be
        // greater than m_size, which is safe due to the check in allocate().
        memcpy(this->data() + start, sourceData, sizeof(T) * (end - start));
    }

private:
    static T* alignedAddress(T* address, intptr_t alignment)
    {
        intptr_t value = reinterpret_cast<intptr_t>(address);
        return reinterpret_cast<T*>((value + alignment - 1) & ~(alignment - 1));
    }

    T* m_allocation;
    T* m_alignedData;
    size_t m_size;
};

typedef AudioArray<float> AudioFloatArray;
typedef AudioArray<double> AudioDoubleArray;

} // WebCore

#endif // AudioArray_h
