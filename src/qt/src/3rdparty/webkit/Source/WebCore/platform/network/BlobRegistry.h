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

#ifndef BlobRegistry_h
#define BlobRegistry_h

#include <wtf/PassOwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

class BlobData;
class BlobRegistry;
class KURL;
class ResourceError;
class ResourceHandle;
class ResourceHandleClient;
class ResourceRequest;
class ResourceResponse;

// Returns a single instance of BlobRegistry.
BlobRegistry& blobRegistry(); 

// BlobRegistry is not thread-safe. It should only be called from main thread.
class BlobRegistry {
public:
    // Registers a blob URL referring to the specified blob data.
    virtual void registerBlobURL(const KURL&, PassOwnPtr<BlobData>) = 0;
    
    // Registers a blob URL referring to the blob data identified by the specified srcURL.
    virtual void registerBlobURL(const KURL&, const KURL& srcURL) = 0;

    virtual void unregisterBlobURL(const KURL&) = 0;
    virtual PassRefPtr<ResourceHandle> createResourceHandle(const ResourceRequest&, ResourceHandleClient*) = 0;
    virtual bool loadResourceSynchronously(const ResourceRequest&, ResourceError&, ResourceResponse&, Vector<char>& data) = 0;

protected:
    virtual ~BlobRegistry() { }
};

} // namespace WebCore

#endif // BlobRegistry_h
