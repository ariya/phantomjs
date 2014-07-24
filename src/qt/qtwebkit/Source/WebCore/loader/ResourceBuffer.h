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

#ifndef ResourceBuffer_h
#define ResourceBuffer_h

#include "SharedBuffer.h"

#include <wtf/PassOwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

#if PLATFORM(MAC)
OBJC_CLASS NSData;
#endif

namespace WebCore {

class PurgeableBuffer;
class SharedBuffer;

class ResourceBuffer : public RefCounted<ResourceBuffer> {
public:

    static PassRefPtr<ResourceBuffer> create() { return adoptRef(new ResourceBuffer); }
    static PassRefPtr<ResourceBuffer> create(const char* data, int size) { return adoptRef(new ResourceBuffer(data, size)); }
    static PassRefPtr<ResourceBuffer> adoptSharedBuffer(PassRefPtr<SharedBuffer> shared) { return shared ? adoptRef(new ResourceBuffer(shared)) : 0; }

    virtual ~ResourceBuffer();

    virtual const char* data() const;
    virtual unsigned size() const;
    virtual bool isEmpty() const;

    void append(const char*, unsigned);
    void append(SharedBuffer*);
#if USE(NETWORK_CFDATA_ARRAY_CALLBACK)
    void append(CFDataRef);
#endif
    void clear();
    
    unsigned getSomeData(const char*& data, unsigned position = 0) const;
    
    SharedBuffer* sharedBuffer() const;
#if PLATFORM(MAC)
    void tryReplaceSharedBufferContents(SharedBuffer*);
#endif
    PassRefPtr<ResourceBuffer> copy() const;

    bool hasPurgeableBuffer() const;
    void createPurgeableBuffer() const;
    
    // Ensure this buffer has no other clients before calling this.
    PassOwnPtr<PurgeableBuffer> releasePurgeableBuffer();

#if PLATFORM(MAC)
    NSData *createNSData();
#endif
#if USE(CF)
    CFDataRef createCFData();
#endif

protected:
    ResourceBuffer();

private:
    ResourceBuffer(const char*, int);
    ResourceBuffer(PassRefPtr<SharedBuffer>);

    RefPtr<SharedBuffer> m_sharedBuffer;
};

} // namespace WebCore

#endif // ResourceBuffer_h
