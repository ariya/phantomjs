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

#include "config.h"

#if ENABLE(INSPECTOR)

#include "NetworkResourcesData.h"

#include "CachedResource.h"
#include "DOMImplementation.h"
#include "ResourceResponse.h"
#include "SharedBuffer.h"
#include "TextResourceDecoder.h"

namespace {
// 100MB
static size_t maximumResourcesContentSize = 100 * 1000 * 1000;

// 10MB
static size_t maximumSingleResourceContentSize = 10 * 1000 * 1000;
}

namespace WebCore {


PassRefPtr<XHRReplayData> XHRReplayData::create(const String &method, const KURL& url, bool async, PassRefPtr<FormData> formData, bool includeCredentials)
{
    return adoptRef(new XHRReplayData(method, url, async, formData, includeCredentials));
}

void XHRReplayData::addHeader(const AtomicString& key, const String& value)
{
    m_headers.set(key, value);
}

XHRReplayData::XHRReplayData(const String &method, const KURL& url, bool async, PassRefPtr<FormData> formData, bool includeCredentials)
    : m_method(method)
    , m_url(url)
    , m_async(async)
    , m_formData(formData)
    , m_includeCredentials(includeCredentials)
{
}

// ResourceData
NetworkResourcesData::ResourceData::ResourceData(const String& requestId, const String& loaderId)
    : m_requestId(requestId)
    , m_loaderId(loaderId)
    , m_base64Encoded(false)
    , m_isContentEvicted(false)
    , m_type(InspectorPageAgent::OtherResource)
    , m_cachedResource(0)
{
}

void NetworkResourcesData::ResourceData::setContent(const String& content, bool base64Encoded)
{
    ASSERT(!hasData());
    ASSERT(!hasContent());
    m_content = content;
    m_base64Encoded = base64Encoded;
}

static size_t contentSizeInBytes(const String& content)
{
    return content.isNull() ? 0 : content.impl()->sizeInBytes();
}

unsigned NetworkResourcesData::ResourceData::removeContent()
{
    unsigned result = 0;
    if (hasData()) {
        ASSERT(!hasContent());
        result = m_dataBuffer->size();
        m_dataBuffer = nullptr;
    }

    if (hasContent()) {
        ASSERT(!hasData());
        result = contentSizeInBytes(m_content);
        m_content = String();
    }
    return result;
}

unsigned NetworkResourcesData::ResourceData::evictContent()
{
    m_isContentEvicted = true;
    return removeContent();
}

size_t NetworkResourcesData::ResourceData::dataLength() const
{
    return m_dataBuffer ? m_dataBuffer->size() : 0;
}

void NetworkResourcesData::ResourceData::appendData(const char* data, size_t dataLength)
{
    ASSERT(!hasContent());
    if (!m_dataBuffer)
        m_dataBuffer = SharedBuffer::create(data, dataLength);
    else
        m_dataBuffer->append(data, dataLength);
}

size_t NetworkResourcesData::ResourceData::decodeDataToContent()
{
    ASSERT(!hasContent());
    size_t dataLength = m_dataBuffer->size();
    m_content = m_decoder->decode(m_dataBuffer->data(), m_dataBuffer->size());
    m_content.append(m_decoder->flush());
    m_dataBuffer = nullptr;
    return contentSizeInBytes(m_content) - dataLength;
}

// NetworkResourcesData
NetworkResourcesData::NetworkResourcesData()
    : m_contentSize(0)
    , m_maximumResourcesContentSize(maximumResourcesContentSize)
    , m_maximumSingleResourceContentSize(maximumSingleResourceContentSize)
{
}

NetworkResourcesData::~NetworkResourcesData()
{
    clear();
}

void NetworkResourcesData::resourceCreated(const String& requestId, const String& loaderId)
{
    ensureNoDataForRequestId(requestId);
    m_requestIdToResourceDataMap.set(requestId, new ResourceData(requestId, loaderId));
}

static PassRefPtr<TextResourceDecoder> createOtherResourceTextDecoder(const String& mimeType, const String& textEncodingName)
{
    RefPtr<TextResourceDecoder> decoder;
    if (!textEncodingName.isEmpty())
        decoder = TextResourceDecoder::create("text/plain", textEncodingName);
    else if (DOMImplementation::isXMLMIMEType(mimeType.lower())) {
        decoder = TextResourceDecoder::create("application/xml");
        decoder->useLenientXMLDecoding();
    } else if (equalIgnoringCase(mimeType, "text/html"))
        decoder = TextResourceDecoder::create("text/html", "UTF-8");
    else if (mimeType == "text/plain")
        decoder = TextResourceDecoder::create("text/plain", "ISO-8859-1");
    return decoder;
}

void NetworkResourcesData::responseReceived(const String& requestId, const String& frameId, const ResourceResponse& response)
{
    ResourceData* resourceData = resourceDataForRequestId(requestId);
    if (!resourceData)
        return;
    resourceData->setFrameId(frameId);
    resourceData->setUrl(response.url());
    resourceData->setDecoder(createOtherResourceTextDecoder(response.mimeType(), response.textEncodingName()));
    resourceData->setHTTPStatusCode(response.httpStatusCode());
}

void NetworkResourcesData::setResourceType(const String& requestId, InspectorPageAgent::ResourceType type)
{
    ResourceData* resourceData = resourceDataForRequestId(requestId);
    if (!resourceData)
        return;
    resourceData->setType(type);
}

InspectorPageAgent::ResourceType NetworkResourcesData::resourceType(const String& requestId)
{
    ResourceData* resourceData = resourceDataForRequestId(requestId);
    if (!resourceData)
        return InspectorPageAgent::OtherResource;
    return resourceData->type();
}

void NetworkResourcesData::setResourceContent(const String& requestId, const String& content, bool base64Encoded)
{
    ResourceData* resourceData = resourceDataForRequestId(requestId);
    if (!resourceData)
        return;
    size_t dataLength = contentSizeInBytes(content);
    if (dataLength > m_maximumSingleResourceContentSize)
        return;
    if (resourceData->isContentEvicted())
        return;
    if (ensureFreeSpace(dataLength) && !resourceData->isContentEvicted()) {
        // We can not be sure that we didn't try to save this request data while it was loading, so remove it, if any.
        if (resourceData->hasContent())
            m_contentSize -= resourceData->removeContent();
        m_requestIdsDeque.append(requestId);
        resourceData->setContent(content, base64Encoded);
        m_contentSize += dataLength;
    }
}

void NetworkResourcesData::maybeAddResourceData(const String& requestId, const char* data, size_t dataLength)
{
    ResourceData* resourceData = resourceDataForRequestId(requestId);
    if (!resourceData)
        return;
    if (!resourceData->decoder())
        return;
    if (resourceData->dataLength() + dataLength > m_maximumSingleResourceContentSize)
        m_contentSize -= resourceData->evictContent();
    if (resourceData->isContentEvicted())
        return;
    if (ensureFreeSpace(dataLength) && !resourceData->isContentEvicted()) {
        m_requestIdsDeque.append(requestId);
        resourceData->appendData(data, dataLength);
        m_contentSize += dataLength;
    }
}

void NetworkResourcesData::maybeDecodeDataToContent(const String& requestId)
{
    ResourceData* resourceData = resourceDataForRequestId(requestId);
    if (!resourceData)
        return;
    if (!resourceData->hasData())
        return;
    m_contentSize += resourceData->decodeDataToContent();
    size_t dataLength = contentSizeInBytes(resourceData->content());
    if (dataLength > m_maximumSingleResourceContentSize)
        m_contentSize -= resourceData->evictContent();
}

void NetworkResourcesData::addCachedResource(const String& requestId, CachedResource* cachedResource)
{
    ResourceData* resourceData = resourceDataForRequestId(requestId);
    if (!resourceData)
        return;
    resourceData->setCachedResource(cachedResource);
}

void NetworkResourcesData::addResourceSharedBuffer(const String& requestId, PassRefPtr<SharedBuffer> buffer, const String& textEncodingName)
{
    ResourceData* resourceData = resourceDataForRequestId(requestId);
    if (!resourceData)
        return;
    resourceData->setBuffer(buffer);
    resourceData->setTextEncodingName(textEncodingName);
}

NetworkResourcesData::ResourceData const* NetworkResourcesData::data(const String& requestId)
{
    return resourceDataForRequestId(requestId);
}

XHRReplayData* NetworkResourcesData::xhrReplayData(const String& requestId)
{
    if (m_reusedXHRReplayDataRequestIds.contains(requestId))
        return xhrReplayData(m_reusedXHRReplayDataRequestIds.get(requestId));

    ResourceData* resourceData = resourceDataForRequestId(requestId);
    if (!resourceData)
        return 0;
    return resourceData->xhrReplayData();
}

void NetworkResourcesData::setXHRReplayData(const String& requestId, XHRReplayData* xhrReplayData)
{
    ResourceData* resourceData = resourceDataForRequestId(requestId);
    if (!resourceData) {
        Vector<String> result;
        ReusedRequestIds::iterator it;
        ReusedRequestIds::iterator end = m_reusedXHRReplayDataRequestIds.end();
        for (it = m_reusedXHRReplayDataRequestIds.begin(); it != end; ++it) {
            if (it->value == requestId)
                setXHRReplayData(it->key, xhrReplayData);
        }
        return;
    }

    resourceData->setXHRReplayData(xhrReplayData);
}

void NetworkResourcesData::reuseXHRReplayData(const String& requestId, const String& reusedRequestId)
{
    ResourceData* reusedResourceData = resourceDataForRequestId(reusedRequestId);
    ResourceData* resourceData = resourceDataForRequestId(requestId);
    if (!reusedResourceData || !resourceData) {
        m_reusedXHRReplayDataRequestIds.set(requestId, reusedRequestId);
        return;
    }

    resourceData->setXHRReplayData(reusedResourceData->xhrReplayData());
}

Vector<String> NetworkResourcesData::removeCachedResource(CachedResource* cachedResource)
{
    Vector<String> result;
    ResourceDataMap::iterator it;
    ResourceDataMap::iterator end = m_requestIdToResourceDataMap.end();
    for (it = m_requestIdToResourceDataMap.begin(); it != end; ++it) {
        ResourceData* resourceData = it->value;
        if (resourceData->cachedResource() == cachedResource) {
            resourceData->setCachedResource(0);
            result.append(it->key);
        }
    }

    return result;
}

void NetworkResourcesData::clear(const String& preservedLoaderId)
{
    m_requestIdsDeque.clear();
    m_contentSize = 0;

    ResourceDataMap preservedMap;

    ResourceDataMap::iterator it;
    ResourceDataMap::iterator end = m_requestIdToResourceDataMap.end();
    for (it = m_requestIdToResourceDataMap.begin(); it != end; ++it) {
        ResourceData* resourceData = it->value;
        if (!preservedLoaderId.isNull() && resourceData->loaderId() == preservedLoaderId)
            preservedMap.set(it->key, it->value);
        else
            delete resourceData;
    }
    m_requestIdToResourceDataMap.swap(preservedMap);

    m_reusedXHRReplayDataRequestIds.clear();
}

void NetworkResourcesData::setResourcesDataSizeLimits(size_t maximumResourcesContentSize, size_t maximumSingleResourceContentSize)
{
    clear();
    m_maximumResourcesContentSize = maximumResourcesContentSize;
    m_maximumSingleResourceContentSize = maximumSingleResourceContentSize;
}

NetworkResourcesData::ResourceData* NetworkResourcesData::resourceDataForRequestId(const String& requestId)
{
    if (requestId.isNull())
        return 0;
    return m_requestIdToResourceDataMap.get(requestId);
}

void NetworkResourcesData::ensureNoDataForRequestId(const String& requestId)
{
    ResourceData* resourceData = resourceDataForRequestId(requestId);
    if (!resourceData)
        return;
    if (resourceData->hasContent() || resourceData->hasData())
        m_contentSize -= resourceData->evictContent();
    delete resourceData;
    m_requestIdToResourceDataMap.remove(requestId);
}

bool NetworkResourcesData::ensureFreeSpace(size_t size)
{
    if (size > m_maximumResourcesContentSize)
        return false;

    while (size > m_maximumResourcesContentSize - m_contentSize) {
        String requestId = m_requestIdsDeque.takeFirst();
        ResourceData* resourceData = resourceDataForRequestId(requestId);
        if (resourceData)
            m_contentSize -= resourceData->evictContent();
    }
    return true;
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
