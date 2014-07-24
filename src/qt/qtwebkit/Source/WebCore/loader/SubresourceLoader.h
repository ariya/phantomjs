/*
 * Copyright (C) 2005, 2006, 2009 Apple Inc. All rights reserved.
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

#ifndef SubresourceLoader_h
#define SubresourceLoader_h

#include "FrameLoaderTypes.h"
#include "ResourceLoader.h"

#include <wtf/text/WTFString.h>
 
namespace WebCore {

class CachedResource;
class CachedResourceLoader;
class Document;
class PageActivityAssertionToken;
class ResourceRequest;

class SubresourceLoader : public ResourceLoader {
public:
    static PassRefPtr<SubresourceLoader> create(Frame*, CachedResource*, const ResourceRequest&, const ResourceLoaderOptions&);

    virtual ~SubresourceLoader();

    void cancelIfNotFinishing();
    virtual bool isSubresourceLoader();
    CachedResource* cachedResource();

private:
    SubresourceLoader(Frame*, CachedResource*, const ResourceLoaderOptions&);

    virtual bool init(const ResourceRequest&) OVERRIDE;

    virtual void willSendRequest(ResourceRequest&, const ResourceResponse& redirectResponse) OVERRIDE;
    virtual void didSendData(unsigned long long bytesSent, unsigned long long totalBytesToBeSent) OVERRIDE;
    virtual void didReceiveResponse(const ResourceResponse&) OVERRIDE;
    virtual void didReceiveData(const char*, int, long long encodedDataLength, DataPayloadType) OVERRIDE;
    virtual void didReceiveBuffer(PassRefPtr<SharedBuffer>, long long encodedDataLength, DataPayloadType) OVERRIDE;
    virtual void didFinishLoading(double finishTime) OVERRIDE;
    virtual void didFail(const ResourceError&) OVERRIDE;
    virtual void willCancel(const ResourceError&) OVERRIDE;
    virtual void didCancel(const ResourceError&) OVERRIDE;

#if USE(NETWORK_CFDATA_ARRAY_CALLBACK)
    virtual bool supportsDataArray() OVERRIDE { return true; }
    virtual void didReceiveDataArray(CFArrayRef) OVERRIDE;
#endif
    virtual void releaseResources() OVERRIDE;

#if USE(SOUP)
    virtual char* getOrCreateReadBuffer(size_t requestedSize, size_t& actualSize) OVERRIDE;
#endif

    bool checkForHTTPStatusCodeError();

    void didReceiveDataOrBuffer(const char*, int, PassRefPtr<SharedBuffer>, long long encodedDataLength, DataPayloadType);

    void notifyDone();

    enum SubresourceLoaderState {
        Uninitialized,
        Initialized,
        Finishing
    };

    class RequestCountTracker {
    public:
        RequestCountTracker(CachedResourceLoader*, CachedResource*);
        ~RequestCountTracker();
    private:
        CachedResourceLoader* m_cachedResourceLoader;
        CachedResource* m_resource;
    };

    CachedResource* m_resource;
    bool m_loadingMultipartContent;
    SubresourceLoaderState m_state;
    OwnPtr<RequestCountTracker> m_requestCountTracker;
    OwnPtr<PageActivityAssertionToken> m_activityAssertion;
};

}

#endif // SubresourceLoader_h
