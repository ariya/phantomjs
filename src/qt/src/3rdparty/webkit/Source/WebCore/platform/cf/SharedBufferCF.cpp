/*
 * Copyright (C) 2008, 2010 Apple Inc. All rights reserved.
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
#include "config.h"
#include "SharedBuffer.h"

#include "PurgeableBuffer.h"

namespace WebCore {

SharedBuffer::SharedBuffer(CFDataRef cfData)
    : m_size(0)
    , m_cfData(cfData)
{
}

// Mac is a CF platform but has an even more efficient version of this method,
// so only use this version for non-Mac
#if !PLATFORM(MAC) && !(PLATFORM(QT) && USE(QTKIT))
CFDataRef SharedBuffer::createCFData()
{
    if (m_cfData) {
        CFRetain(m_cfData.get());
        return m_cfData.get();
    }

    // Internal data in SharedBuffer can be segmented. We need to get the contiguous buffer.
    const Vector<char>& contiguousBuffer = buffer();
    return CFDataCreate(0, reinterpret_cast<const UInt8*>(contiguousBuffer.data()), contiguousBuffer.size());
}
#endif

PassRefPtr<SharedBuffer> SharedBuffer::wrapCFData(CFDataRef data)
{
    return adoptRef(new SharedBuffer(data));
}

bool SharedBuffer::hasPlatformData() const
{
    return m_cfData;
}

const char* SharedBuffer::platformData() const
{
    return (const char*)CFDataGetBytePtr(m_cfData.get());
}

unsigned SharedBuffer::platformDataSize() const
{
    return CFDataGetLength(m_cfData.get());
}

void SharedBuffer::maybeTransferPlatformData()
{
    if (!m_cfData)
        return;
    
    ASSERT(!m_size);
        
    append((const char*)CFDataGetBytePtr(m_cfData.get()), CFDataGetLength(m_cfData.get()));
        
    m_cfData = 0;
}

void SharedBuffer::clearPlatformData()
{
    m_cfData = 0;
}

#if HAVE(CFNETWORK_DATA_ARRAY_CALLBACK)
void SharedBuffer::append(CFDataRef data)
{
    ASSERT(data);
    m_dataArray.append(data);
    m_size += CFDataGetLength(data);
}

void SharedBuffer::copyDataArrayAndClear(char *destination, unsigned bytesToCopy) const
{
    if (m_dataArray.isEmpty())
        return;

    CFIndex bytesLeft = bytesToCopy;
    Vector<RetainPtr<CFDataRef> >::const_iterator end = m_dataArray.end();
    for (Vector<RetainPtr<CFDataRef> >::const_iterator it = m_dataArray.begin(); it != end; ++it) {
        CFIndex dataLen = CFDataGetLength(it->get());
        ASSERT(bytesLeft >= dataLen);
        memcpy(destination, CFDataGetBytePtr(it->get()), dataLen);
        destination += dataLen;
        bytesLeft -= dataLen;
    }
    m_dataArray.clear();
}
#endif

}
