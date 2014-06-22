/*
 * Copyright (C) 2009, 2012 Google Inc. All rights reserved.
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

#ifndef DocumentThreadableLoader_h
#define DocumentThreadableLoader_h

#include "CachedRawResourceClient.h"
#include "CachedResourceHandle.h"
#include "FrameLoaderTypes.h"
#include "ThreadableLoader.h"
#include <wtf/Forward.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {
    class CachedRawResource;
    class Document;
    class KURL;
    class ResourceRequest;
    class SecurityOrigin;
    class ThreadableLoaderClient;

    class DocumentThreadableLoader : public RefCounted<DocumentThreadableLoader>, public ThreadableLoader, private CachedRawResourceClient  {
        WTF_MAKE_FAST_ALLOCATED;
    public:
        static void loadResourceSynchronously(Document*, const ResourceRequest&, ThreadableLoaderClient&, const ThreadableLoaderOptions&);
        static PassRefPtr<DocumentThreadableLoader> create(Document*, ThreadableLoaderClient*, const ResourceRequest&, const ThreadableLoaderOptions&);
        virtual ~DocumentThreadableLoader();

        virtual void cancel();
        virtual void setDefersLoading(bool);

        using RefCounted<DocumentThreadableLoader>::ref;
        using RefCounted<DocumentThreadableLoader>::deref;

    protected:
        virtual void refThreadableLoader() { ref(); }
        virtual void derefThreadableLoader() { deref(); }

    private:
        enum BlockingBehavior {
            LoadSynchronously,
            LoadAsynchronously
        };

        DocumentThreadableLoader(Document*, ThreadableLoaderClient*, BlockingBehavior, const ResourceRequest&, const ThreadableLoaderOptions&);

        void clearResource();

        // CachedRawResourceClient
        virtual void dataSent(CachedResource*, unsigned long long bytesSent, unsigned long long totalBytesToBeSent);
        virtual void responseReceived(CachedResource*, const ResourceResponse&);
        virtual void dataReceived(CachedResource*, const char* data, int dataLength);
        virtual void redirectReceived(CachedResource*, ResourceRequest&, const ResourceResponse&);
        virtual void notifyFinished(CachedResource*);

        void didReceiveResponse(unsigned long identifier, const ResourceResponse&);
        void didReceiveData(unsigned long identifier, const char* data, int dataLength);
        void didFinishLoading(unsigned long identifier, double finishTime);
        void didFail(unsigned long identifier, const ResourceError&);
        void makeCrossOriginAccessRequest(const ResourceRequest&);
        void makeSimpleCrossOriginAccessRequest(const ResourceRequest& request);
        void makeCrossOriginAccessRequestWithPreflight(const ResourceRequest& request);
        void preflightSuccess();
        void preflightFailure(unsigned long identifier, const String& url, const String& errorDescription);

        void loadRequest(const ResourceRequest&, SecurityCheckPolicy);
        bool isAllowedRedirect(const KURL&);

        SecurityOrigin* securityOrigin() const;

        CachedResourceHandle<CachedRawResource> m_resource;
        ThreadableLoaderClient* m_client;
        Document* m_document;
        ThreadableLoaderOptions m_options;
        bool m_sameOriginRequest;
        bool m_simpleRequest;
        bool m_async;
        OwnPtr<ResourceRequest> m_actualRequest;  // non-null during Access Control preflight checks
    };

} // namespace WebCore

#endif // DocumentThreadableLoader_h
