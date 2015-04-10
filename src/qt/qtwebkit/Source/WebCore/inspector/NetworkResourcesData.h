/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. AND ITS CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL GOOGLE INC.
 * OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef NetworkResourcesData_h
#define NetworkResourcesData_h

#include "HTTPHeaderMap.h"
#include "InspectorPageAgent.h"
#include "TextResourceDecoder.h"
#include <wtf/Deque.h>
#include <wtf/HashMap.h>
#include <wtf/RefCounted.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/text/WTFString.h>

#if ENABLE(INSPECTOR)

namespace WebCore {

class CachedResource;
class FormData;
class SharedBuffer;
class TextResourceDecoder;

class XHRReplayData : public RefCounted<XHRReplayData> {
public:
    static PassRefPtr<XHRReplayData> create(const String &method, const KURL&, bool async, PassRefPtr<FormData>, bool includeCredentials);

    void addHeader(const AtomicString& key, const String& value);
    const String& method() const { return m_method; }
    const KURL& url() const { return m_url; }
    bool async() const { return m_async; }
    PassRefPtr<FormData> formData() const { return m_formData; }
    const HTTPHeaderMap& headers() const { return m_headers; }
    bool includeCredentials() const { return m_includeCredentials; }
private:
    XHRReplayData(const String &method, const KURL&, bool async, PassRefPtr<FormData>, bool includeCredentials);

    String m_method;
    KURL m_url;
    bool m_async;
    RefPtr<FormData> m_formData;
    HTTPHeaderMap m_headers;
    bool m_includeCredentials;
};

class NetworkResourcesData {
    WTF_MAKE_FAST_ALLOCATED;
public:
    class ResourceData {
        WTF_MAKE_FAST_ALLOCATED;
        friend class NetworkResourcesData;
    public:
        ResourceData(const String& requestId, const String& loaderId);

        String requestId() const { return m_requestId; }
        String loaderId() const { return m_loaderId; }

        String frameId() const { return m_frameId; }
        void setFrameId(const String& frameId) { m_frameId = frameId; }

        String url() const { return m_url; }
        void setUrl(const String& url) { m_url = url; }

        bool hasContent() const { return !m_content.isNull(); }
        String content() const { return m_content; }
        void setContent(const String&, bool base64Encoded);

        bool base64Encoded() const { return m_base64Encoded; }

        unsigned removeContent();
        bool isContentEvicted() const { return m_isContentEvicted; }
        unsigned evictContent();

        InspectorPageAgent::ResourceType type() const { return m_type; }
        void setType(InspectorPageAgent::ResourceType type) { m_type = type; }

        int httpStatusCode() const { return m_httpStatusCode; }
        void setHTTPStatusCode(int httpStatusCode) { m_httpStatusCode = httpStatusCode; }

        String textEncodingName() const { return m_textEncodingName; }
        void setTextEncodingName(const String& textEncodingName) { m_textEncodingName = textEncodingName; }

        PassRefPtr<TextResourceDecoder> decoder() const { return m_decoder; }
        void setDecoder(PassRefPtr<TextResourceDecoder> decoder) { m_decoder = decoder; }

        PassRefPtr<SharedBuffer> buffer() const { return m_buffer; }
        void setBuffer(PassRefPtr<SharedBuffer> buffer) { m_buffer = buffer; }

        CachedResource* cachedResource() const { return m_cachedResource; }
        void setCachedResource(CachedResource* cachedResource) { m_cachedResource = cachedResource; }

        XHRReplayData* xhrReplayData() const { return m_xhrReplayData.get(); }
        void setXHRReplayData(XHRReplayData* xhrReplayData) { m_xhrReplayData = xhrReplayData; }

    private:
        bool hasData() const { return m_dataBuffer; }
        size_t dataLength() const;
        void appendData(const char* data, size_t dataLength);
        size_t decodeDataToContent();

        String m_requestId;
        String m_loaderId;
        String m_frameId;
        String m_url;
        String m_content;
        RefPtr<XHRReplayData> m_xhrReplayData;
        bool m_base64Encoded;
        RefPtr<SharedBuffer> m_dataBuffer;
        bool m_isContentEvicted;
        InspectorPageAgent::ResourceType m_type;
        int m_httpStatusCode;

        String m_textEncodingName;
        RefPtr<TextResourceDecoder> m_decoder;

        RefPtr<SharedBuffer> m_buffer;
        CachedResource* m_cachedResource;
    };

    NetworkResourcesData();

    ~NetworkResourcesData();

    void resourceCreated(const String& requestId, const String& loaderId);
    void responseReceived(const String& requestId, const String& frameId, const ResourceResponse&);
    void setResourceType(const String& requestId, InspectorPageAgent::ResourceType);
    InspectorPageAgent::ResourceType resourceType(const String& requestId);
    void setResourceContent(const String& requestId, const String& content, bool base64Encoded = false);
    void maybeAddResourceData(const String& requestId, const char* data, size_t dataLength);
    void maybeDecodeDataToContent(const String& requestId);
    void addCachedResource(const String& requestId, CachedResource*);
    void addResourceSharedBuffer(const String& requestId, PassRefPtr<SharedBuffer>, const String& textEncodingName);
    ResourceData const* data(const String& requestId);
    Vector<String> removeCachedResource(CachedResource*);
    void clear(const String& preservedLoaderId = String());

    void setResourcesDataSizeLimits(size_t maximumResourcesContentSize, size_t maximumSingleResourceContentSize);
    void setXHRReplayData(const String& requestId, XHRReplayData*);
    void reuseXHRReplayData(const String& requestId, const String& reusedRequestId);
    XHRReplayData* xhrReplayData(const String& requestId);

private:
    ResourceData* resourceDataForRequestId(const String& requestId);
    void ensureNoDataForRequestId(const String& requestId);
    bool ensureFreeSpace(size_t);

    Deque<String> m_requestIdsDeque;

    typedef HashMap<String, String> ReusedRequestIds;
    ReusedRequestIds m_reusedXHRReplayDataRequestIds;
    typedef HashMap<String, ResourceData*> ResourceDataMap;
    ResourceDataMap m_requestIdToResourceDataMap;
    size_t m_contentSize;
    size_t m_maximumResourcesContentSize;
    size_t m_maximumSingleResourceContentSize;
};

} // namespace WebCore

#endif // ENABLE(INSPECTOR)

#endif // !defined(NetworkResourcesData_h)
