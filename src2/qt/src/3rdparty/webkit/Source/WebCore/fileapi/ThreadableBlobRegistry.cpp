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

#include "config.h"

#include "ThreadableBlobRegistry.h"

#include "BlobData.h"
#include "BlobRegistry.h"
#include <wtf/MainThread.h>

namespace WebCore {

struct BlobRegistryContext {
    BlobRegistryContext(const KURL& url, PassOwnPtr<BlobData> blobData)
        : url(url.copy())
        , blobData(blobData)
    {
        this->blobData->detachFromCurrentThread();
    }

    BlobRegistryContext(const KURL& url, const KURL& srcURL)
        : url(url.copy())
        , srcURL(srcURL.copy())
    {
    }

    BlobRegistryContext(const KURL& url)
        : url(url.copy())
    {
    }

    KURL url;
    KURL srcURL;
    OwnPtr<BlobData> blobData;
};

#if ENABLE(BLOB)

static void registerBlobURLTask(void* context)
{
    OwnPtr<BlobRegistryContext> blobRegistryContext = adoptPtr(static_cast<BlobRegistryContext*>(context));
    blobRegistry().registerBlobURL(blobRegistryContext->url, blobRegistryContext->blobData.release());
}

void ThreadableBlobRegistry::registerBlobURL(const KURL& url, PassOwnPtr<BlobData> blobData)
{
    if (isMainThread())
        blobRegistry().registerBlobURL(url, blobData);
    else {
        OwnPtr<BlobRegistryContext> context = adoptPtr(new BlobRegistryContext(url, blobData));
        callOnMainThread(&registerBlobURLTask, context.leakPtr());
    }
}

static void registerBlobURLFromTask(void* context)
{
    OwnPtr<BlobRegistryContext> blobRegistryContext = adoptPtr(static_cast<BlobRegistryContext*>(context));
    blobRegistry().registerBlobURL(blobRegistryContext->url, blobRegistryContext->srcURL);
}

void ThreadableBlobRegistry::registerBlobURL(const KURL& url, const KURL& srcURL)
{
    if (isMainThread())
        blobRegistry().registerBlobURL(url, srcURL);
    else {
        OwnPtr<BlobRegistryContext> context = adoptPtr(new BlobRegistryContext(url, srcURL));
        callOnMainThread(&registerBlobURLFromTask, context.leakPtr());
    }
}

static void unregisterBlobURLTask(void* context)
{
    OwnPtr<BlobRegistryContext> blobRegistryContext = adoptPtr(static_cast<BlobRegistryContext*>(context));
    blobRegistry().unregisterBlobURL(blobRegistryContext->url);
}

void ThreadableBlobRegistry::unregisterBlobURL(const KURL& url)
{
    if (isMainThread())
        blobRegistry().unregisterBlobURL(url);
    else {
        OwnPtr<BlobRegistryContext> context = adoptPtr(new BlobRegistryContext(url));
        callOnMainThread(&unregisterBlobURLTask, context.leakPtr());
    }
}

#else

void ThreadableBlobRegistry::registerBlobURL(const KURL&, PassOwnPtr<BlobData>)
{
}

void ThreadableBlobRegistry::registerBlobURL(const KURL&, const KURL&)
{
}

void ThreadableBlobRegistry::unregisterBlobURL(const KURL&)
{
}
#endif // ENABL(BLOB)

} // namespace WebCore
