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
#include "VisitedLinkTable.h"

#include "SharedMemory.h"

using namespace WebCore;

namespace WebKit {

VisitedLinkTable::VisitedLinkTable()
    : m_tableSize(0)
    , m_table(0)
{
}

VisitedLinkTable::~VisitedLinkTable()
{
}

static inline bool isPowerOf2(unsigned v)
{
    // Taken from http://www.cs.utk.edu/~vose/c-stuff/bithacks.html
    
    return !(v & (v - 1)) && v;
}

void VisitedLinkTable::setSharedMemory(PassRefPtr<SharedMemory> sharedMemory)
{
    m_sharedMemory = sharedMemory;
    
    ASSERT(m_sharedMemory);
    ASSERT(!(m_sharedMemory->size() % sizeof(LinkHash)));

    m_table = static_cast<LinkHash*>(m_sharedMemory->data());
    m_tableSize = m_sharedMemory->size() / sizeof(LinkHash);
    ASSERT(isPowerOf2(m_tableSize));
    
    m_tableSizeMask = m_tableSize - 1;
}

static inline unsigned doubleHash(unsigned key)
{
    key = ~key + (key >> 23);
    key ^= (key << 12);
    key ^= (key >> 7);
    key ^= (key << 2);
    key ^= (key >> 20);
    return key;
}
    
bool VisitedLinkTable::addLinkHash(LinkHash linkHash)
{
    ASSERT(m_sharedMemory);

    int k = 0;
    LinkHash* table = m_table;
    int sizeMask = m_tableSizeMask;
    unsigned h = static_cast<unsigned>(linkHash);
    int i = h & sizeMask;
  
    LinkHash* entry;
    while (1) {
        entry = table + i;

        // Check if this bucket is empty.
        if (!*entry)
            break;

        // Check if the same link hash is in the table already.
        if (*entry == linkHash)
            return false;

        if (!k)
            k = 1 | doubleHash(h);
        i = (i + k) & sizeMask;
    }

    *entry = linkHash;
    return true;
}

bool VisitedLinkTable::isLinkVisited(LinkHash linkHash) const
{
    if (!m_sharedMemory)
        return false;

    int k = 0;
    LinkHash* table = m_table;
    int sizeMask = m_tableSizeMask;
    unsigned h = static_cast<unsigned>(linkHash);
    int i = h & sizeMask;
    
    LinkHash* entry;
    while (1) {
        entry = table + i;

        // Check if we've reached the end of the table.
        if (!*entry)
            break;
        
        if (*entry == linkHash)
            return true;
        
        if (!k)
            k = 1 | doubleHash(h);
        i = (i + k) & sizeMask;
    }

    return false;
}

} // namespace WebKit
