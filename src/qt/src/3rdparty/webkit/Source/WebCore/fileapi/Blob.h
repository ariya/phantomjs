/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef Blob_h
#define Blob_h

#include "BlobData.h"
#include "KURL.h"
#include "PlatformString.h"
#include <wtf/PassOwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>

namespace WebCore {

class Blob : public RefCounted<Blob> {
public:
    static PassRefPtr<Blob> create(PassOwnPtr<BlobData> blobData, long long size)
    {
        return adoptRef(new Blob(blobData, size));
    }

    // For deserialization.
    static PassRefPtr<Blob> create(const KURL& srcURL, const String& type, long long size)
    {
        return adoptRef(new Blob(srcURL, type, size));
    }

    virtual ~Blob();

    const KURL& url() const { return m_internalURL; }
    const String& type() const { return m_type; }

    virtual unsigned long long size() const { return static_cast<unsigned long long>(m_size); }
    virtual bool isFile() const { return false; }

#if ENABLE(BLOB)
    PassRefPtr<Blob> webkitSlice(long long start, long long end = std::numeric_limits<long long>::max(), const String& contentType = String()) const;
#endif

protected:
    Blob(PassOwnPtr<BlobData>, long long size);

    // For deserialization.
    Blob(const KURL& srcURL, const String& type, long long size);

    // This is an internal URL referring to the blob data associated with this object. It serves
    // as an identifier for this blob. The internal URL is never used to source the blob's content
    // into an HTML or for FileRead'ing, public blob URLs must be used for those purposes.
    KURL m_internalURL;
    
    String m_type;
    long long m_size;
};

} // namespace WebCore

#endif // Blob_h
