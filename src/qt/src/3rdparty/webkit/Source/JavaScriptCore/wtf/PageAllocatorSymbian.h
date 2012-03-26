/*
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
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

#ifndef PageAllocatorSymbian_h
#define PageAllocatorSymbian_h

#include <e32std.h>
#include <wtf/Bitmap.h>

namespace WTF { 

size_t pageSize();

// Convenience wrapper around an RChunk
class SymbianChunk : public RChunk {
   
public:   
    SymbianChunk(TInt handle) 
    {
        SetHandle(handle);
        // prevent kernel calls by caching these
        m_base = reinterpret_cast<char*>(Base()); 
        m_maxSize = MaxSize();
    }
    
    ~SymbianChunk() 
    { 
        Decommit(0, m_maxSize);
        Close();
    }
     
    // checks if address is in chunk's virtual address space
    bool contains(const void* address) const 
    {
        return (static_cast<const char*>(address) >= m_base && static_cast<const char*>(address) < (m_base + m_maxSize));  
    }
    
    char* m_base; 
    size_t m_maxSize; 
    
};

// Size of the large up-front reservation
#if defined(__WINS__) 
// Emulator has limited virtual address space
const size_t largeReservationSize = 64*1024*1024;
#else
// HW has plenty of virtual addresses
const size_t largeReservationSize = 256*1024*1024;
#endif

class PageAllocatorSymbian { 

public:     
    PageAllocatorSymbian();
    ~PageAllocatorSymbian();
    
    void* reserve(size_t);
    void release(void*, size_t);
    bool commit(void*, size_t);
    bool decommit(void*, size_t);
    
    bool contains(const void*) const; 
    
private:     
    static const size_t m_pageSize = 4096; 
    SymbianChunk* m_chunk; 
    Bitmap<largeReservationSize / m_pageSize> m_map;
    
};

} // namespace WTF

#endif // PageAllocatorSymbian_h

