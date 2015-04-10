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

#ifndef TieredMMapArray_h
#define TieredMMapArray_h

#include <wtf/OSAllocator.h>

namespace JSC {

// This class implements a simple array class that can be grown by appending items to the end.
// This class is implemented purely in terms of system allocations, with no malloc/free, so that
// it can safely be used from a secondary thread whilst the main thrad is paused (potentially
// holding the fast malloc heap lock).
template<typename T>
class TieredMMapArray {
    static const size_t entriesPerBlock = 4096;

public:
    TieredMMapArray()
        : m_directoryCount(4096)
        , m_directory(static_cast<T**>(OSAllocator::reserveAndCommit(m_directoryCount * sizeof(T*))))
        , m_size(0)
    {
        for (size_t block = 0; block < m_directoryCount; ++block)
            m_directory[block] = 0;
    }

    ~TieredMMapArray()
    {
        size_t usedCount = (m_size + (entriesPerBlock - 1)) / entriesPerBlock;
        ASSERT(usedCount == m_directoryCount || !m_directory[usedCount]);

        for (size_t block = 0; block < usedCount; ++block) {
            ASSERT(m_directory[block]);
            OSAllocator::decommitAndRelease(m_directory[block], entriesPerBlock * sizeof(T));
        }

        OSAllocator::decommitAndRelease(m_directory, m_directoryCount * sizeof(T*));
    }

    T& operator[](size_t index)
    {
        ASSERT(index < m_size);
        size_t block = index / entriesPerBlock;
        size_t offset = index % entriesPerBlock;

        ASSERT(m_directory[block]);
        return m_directory[block][offset];
    }

    void append(const T& value)
    {
        // Check if the array is completely full, if so create more capacity in the directory.
        if (m_size == m_directoryCount * entriesPerBlock) {
            // Reallocate the directory.
            size_t oldDirectorySize = m_directoryCount * sizeof(T*);
            size_t newDirectorySize = oldDirectorySize * 2;
            RELEASE_ASSERT(newDirectorySize < oldDirectorySize);
            m_directory = OSAllocator::reallocateCommitted(m_directory, oldDirectorySize, newDirectorySize);

            // 
            size_t newDirectoryCount = m_directoryCount * 2;
            for (size_t block = m_directoryCount; block < newDirectoryCount; ++block)
                m_directory[block] = 0;
            m_directoryCount = newDirectoryCount;
        }

        size_t index = m_size;
        size_t block = index / entriesPerBlock;
        size_t offset = index % entriesPerBlock;

        if (!offset) {
            ASSERT(!m_directory[block]);
            m_directory[block] = static_cast<T*>(OSAllocator::reserveAndCommit(entriesPerBlock * sizeof(T)));
        }

        ASSERT(m_directory[block]);
        ++m_size;
        m_directory[block][offset] = value;
    }

    size_t size() const { return m_size; }

private:
    size_t m_directoryCount;
    T** m_directory;
    size_t m_size;
};

}

#endif // TieredMMapArray_h

