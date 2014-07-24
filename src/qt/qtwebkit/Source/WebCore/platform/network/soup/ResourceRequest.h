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
#include <libsoup/soup.h>

namespace WebCore {

    class ResourceRequest : public ResourceRequestBase {
    public:
        ResourceRequest(const String& url)
            : ResourceRequestBase(KURL(ParsedURLString, url), UseProtocolCachePolicy)
            , m_soupFlags(static_cast<SoupMessageFlags>(0))
        {
        }

        ResourceRequest(const KURL& url)
            : ResourceRequestBase(url, UseProtocolCachePolicy)
            , m_soupFlags(static_cast<SoupMessageFlags>(0))
        {
        }

        ResourceRequest(const KURL& url, const String& referrer, ResourceRequestCachePolicy policy = UseProtocolCachePolicy)
            : ResourceRequestBase(url, policy)
            , m_soupFlags(static_cast<SoupMessageFlags>(0))
        {
            setHTTPReferrer(referrer);
        }

        ResourceRequest()
            : ResourceRequestBase(KURL(), UseProtocolCachePolicy)
            , m_soupFlags(static_cast<SoupMessageFlags>(0))
        {
        }

        ResourceRequest(SoupMessage* soupMessage)
            : ResourceRequestBase(KURL(), UseProtocolCachePolicy)
            , m_soupFlags(static_cast<SoupMessageFlags>(0))
        {
            updateFromSoupMessage(soupMessage);
        }

        void updateFromDelegatePreservingOldHTTPBody(const ResourceRequest& delegateProvidedRequest) { *this = delegateProvidedRequest; }

        void updateSoupMessageHeaders(SoupMessageHeaders*) const;
        void updateFromSoupMessageHeaders(SoupMessageHeaders*);
        void updateSoupMessage(SoupMessage*) const;
        SoupMessage* toSoupMessage() const;
        void updateFromSoupMessage(SoupMessage*);

        SoupMessageFlags soupMessageFlags() const { return m_soupFlags; }
        void setSoupMessageFlags(SoupMessageFlags soupFlags) { m_soupFlags = soupFlags; }

        SoupURI* soupURI() const;

    private:
        friend class ResourceRequestBase;

        SoupMessageFlags m_soupFlags;

        void doUpdatePlatformRequest() { }
        void doUpdateResourceRequest() { }
        void doUpdatePlatformHTTPBody() { }
        void doUpdateResourceHTTPBody() { }

        PassOwnPtr<CrossThreadResourceRequestData> doPlatformCopyData(PassOwnPtr<CrossThreadResourceRequestData> data) const { return data; }
        void doPlatformAdopt(PassOwnPtr<CrossThreadResourceRequestData>) { }
    };

    struct CrossThreadResourceRequestData : public CrossThreadResourceRequestDataBase {
    };

#if SOUP_CHECK_VERSION(2, 43, 1)
inline SoupMessagePriority toSoupMessagePriority(ResourceLoadPriority priority)
{
    switch (priority) {
    case ResourceLoadPriorityUnresolved:
        return SOUP_MESSAGE_PRIORITY_NORMAL;
    case ResourceLoadPriorityVeryLow:
        return SOUP_MESSAGE_PRIORITY_VERY_LOW;
    case ResourceLoadPriorityLow:
        return SOUP_MESSAGE_PRIORITY_LOW;
    case ResourceLoadPriorityMedium:
        return SOUP_MESSAGE_PRIORITY_NORMAL;
    case ResourceLoadPriorityHigh:
        return SOUP_MESSAGE_PRIORITY_HIGH;
    case ResourceLoadPriorityVeryHigh:
        return SOUP_MESSAGE_PRIORITY_VERY_HIGH;
    }

    ASSERT_NOT_REACHED();
    return SOUP_MESSAGE_PRIORITY_VERY_LOW;
}
#endif

} // namespace WebCore

#endif // ResourceRequest_h
