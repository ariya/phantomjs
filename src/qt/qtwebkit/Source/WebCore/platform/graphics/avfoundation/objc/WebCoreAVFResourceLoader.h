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

#ifndef WebCoreAVFResourceLoader_h
#define WebCoreAVFResourceLoader_h

#if ENABLE(VIDEO) && USE(AVFOUNDATION) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090

#include "CachedRawResourceClient.h"
#include "CachedResourceHandle.h"
#include <wtf/Noncopyable.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/RetainPtr.h>

OBJC_CLASS AVAssetResourceLoadingRequest;

namespace WebCore {

class CachedRawResource;
class CachedResourceLoader;
class MediaPlayerPrivateAVFoundationObjC;

class WebCoreAVFResourceLoader : public CachedRawResourceClient {
    WTF_MAKE_NONCOPYABLE(WebCoreAVFResourceLoader); WTF_MAKE_FAST_ALLOCATED;
public:
    static PassOwnPtr<WebCoreAVFResourceLoader> create(MediaPlayerPrivateAVFoundationObjC* parent, AVAssetResourceLoadingRequest*);
    virtual ~WebCoreAVFResourceLoader();

    void startLoading();
    void stopLoading();

    CachedRawResource* resource();

private:
    // CachedResourceClient
    virtual void responseReceived(CachedResource*, const ResourceResponse&) OVERRIDE;
    virtual void dataReceived(CachedResource*, const char*, int) OVERRIDE;
    virtual void notifyFinished(CachedResource*) OVERRIDE;

    void fulfillRequestWithResource(CachedResource*);

    WebCoreAVFResourceLoader(MediaPlayerPrivateAVFoundationObjC* parent, AVAssetResourceLoadingRequest*);
    MediaPlayerPrivateAVFoundationObjC* m_parent;
    RetainPtr<AVAssetResourceLoadingRequest> m_avRequest;
    CachedResourceHandle<CachedRawResource> m_resource;
};

}

#endif // ENABLE(VIDEO) && USE(AVFOUNDATION) 

#endif // WebCoreAVFResourceLoader_h
