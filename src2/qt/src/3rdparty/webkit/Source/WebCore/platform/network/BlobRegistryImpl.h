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

#ifndef BlobRegistryImpl_h
#define BlobRegistryImpl_h

#include "BlobData.h"
#include "BlobRegistry.h"
#include "BlobStorageData.h"
#include "PlatformString.h"
#include <wtf/HashMap.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringHash.h>

namespace WebCore {

class KURL;
class ResourceError;
class ResourceHandle;
class ResourceHandleClient;
class ResourceRequest;
class ResourceResponse;

// BlobRegistryImpl is not thread-safe. It should only be called from main thread.
class BlobRegistryImpl : public BlobRegistry {
public:
    virtual ~BlobRegistryImpl() { }

    virtual void registerBlobURL(const KURL&, PassOwnPtr<BlobData>);
    virtual void registerBlobURL(const KURL&, const KURL& srcURL);
    virtual void unregisterBlobURL(const KURL&);
    virtual PassRefPtr<ResourceHandle> createResourceHandle(const ResourceRequest&, ResourceHandleClient*);
    virtual bool loadResourceSynchronously(const ResourceRequest&, ResourceError&, ResourceResponse&, Vector<char>& data);

    PassRefPtr<BlobStorageData> getBlobDataFromURL(const KURL&) const;

private:
    bool shouldLoadResource(const ResourceRequest& request) const;
    void appendStorageItems(BlobStorageData*, const BlobDataItemList&);
    void appendStorageItems(BlobStorageData*, const BlobDataItemList&, long long offset, long long length);

    HashMap<String, RefPtr<BlobStorageData> > m_blobs;
};

} // namespace WebCore

#endif // BlobRegistryImpl_h
