/*
 * Copyright (C) 2003, 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2006 Samuel Weinig <sam.weinig@gmail.com>
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

#ifndef ResourceRequest_h
#define ResourceRequest_h

#include "ResourceRequestBase.h"
#include <wtf/RetainPtr.h>

OBJC_CLASS NSURLRequest;

#if PLATFORM(MAC) || USE(CFNETWORK)
typedef const struct _CFURLRequest* CFURLRequestRef;
typedef const struct __CFURLStorageSession* CFURLStorageSessionRef;
#endif

namespace WebCore {

    class ResourceRequest : public ResourceRequestBase {
    public:
        ResourceRequest(const String& url) 
            : ResourceRequestBase(KURL(ParsedURLString, url), UseProtocolCachePolicy)
        {
        }

        ResourceRequest(const KURL& url) 
            : ResourceRequestBase(url, UseProtocolCachePolicy)
        {
        }

        ResourceRequest(const KURL& url, const String& referrer, ResourceRequestCachePolicy policy = UseProtocolCachePolicy) 
            : ResourceRequestBase(url, policy)
        {
            setHTTPReferrer(referrer);
        }
        
        ResourceRequest()
            : ResourceRequestBase(KURL(), UseProtocolCachePolicy)
        {
        }
        
#if USE(CFNETWORK)
#if PLATFORM(MAC)
        ResourceRequest(NSURLRequest *);
        void updateNSURLRequest();
#endif

        ResourceRequest(CFURLRequestRef cfRequest)
            : ResourceRequestBase()
            , m_cfRequest(cfRequest)
        {
#if PLATFORM(MAC)
            updateNSURLRequest();
#endif
        }
#else
        ResourceRequest(NSURLRequest *nsRequest)
            : ResourceRequestBase()
            , m_nsRequest(nsRequest)
        {
        }

        void updateFromDelegatePreservingOldHTTPBody(const ResourceRequest&);
#endif

#if PLATFORM(MAC)
        void applyWebArchiveHackForMail();
        NSURLRequest *nsURLRequest(HTTPBodyUpdatePolicy) const;
#endif

#if ENABLE(CACHE_PARTITIONING)
        static String partitionName(const String& domain);
        const String& cachePartition() const { return m_cachePartition.isNull() ? emptyString() : m_cachePartition; }
        void setCachePartition(const String& cachePartition) { m_cachePartition = partitionName(cachePartition); }
#endif

#if PLATFORM(MAC) || USE(CFNETWORK)
        CFURLRequestRef cfURLRequest(HTTPBodyUpdatePolicy) const;
        void setStorageSession(CFURLStorageSessionRef);
#endif

        static bool httpPipeliningEnabled();
        static void setHTTPPipeliningEnabled(bool);

#if PLATFORM(MAC)
        static bool useQuickLookResourceCachingQuirks();
#endif

    private:
        friend class ResourceRequestBase;

        void doUpdatePlatformRequest();
        void doUpdateResourceRequest();
        void doUpdatePlatformHTTPBody();
        void doUpdateResourceHTTPBody();

        PassOwnPtr<CrossThreadResourceRequestData> doPlatformCopyData(PassOwnPtr<CrossThreadResourceRequestData>) const;
        void doPlatformAdopt(PassOwnPtr<CrossThreadResourceRequestData>);

#if USE(CFNETWORK)
        RetainPtr<CFURLRequestRef> m_cfRequest;
#endif
#if PLATFORM(MAC)
        RetainPtr<NSURLRequest> m_nsRequest;
#endif
#if ENABLE(CACHE_PARTITIONING)
        String m_cachePartition;
#endif

        static bool s_httpPipeliningEnabled;
    };

    struct CrossThreadResourceRequestData : public CrossThreadResourceRequestDataBase {
#if ENABLE(CACHE_PARTITIONING)
        String m_cachePartition;
#endif
    };

} // namespace WebCore

#endif // ResourceRequest_h
