/*
 * Copyright (C) 2011 Google Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "CachedRawResource.h"

#include "CachedRawResourceClient.h"
#include "CachedResourceClientWalker.h"
#include "CachedResourceLoader.h"
#include "ResourceBuffer.h"
#include "SubresourceLoader.h"
#include <wtf/PassRefPtr.h>

namespace WebCore {

CachedRawResource::CachedRawResource(ResourceRequest& resourceRequest, Type type)
    : CachedResource(resourceRequest, type)
    , m_identifier(0)
{
}

const char* CachedRawResource::calculateIncrementalDataChunk(ResourceBuffer* data, unsigned& incrementalDataLength)
{
    incrementalDataLength = 0;
    if (!data)
        return 0;

    unsigned previousDataLength = encodedSize();
    ASSERT(data->size() >= previousDataLength);
    incrementalDataLength = data->size() - previousDataLength;
    return data->data() + previousDataLength;
}

void CachedRawResource::addDataBuffer(ResourceBuffer* data)
{
    CachedResourceHandle<CachedRawResource> protect(this);
    ASSERT(m_options.dataBufferingPolicy == BufferData);
    m_data = data;

    unsigned incrementalDataLength;
    const char* incrementalData = calculateIncrementalDataChunk(data, incrementalDataLength);
    if (data)
        setEncodedSize(data->size());
    notifyClientsDataWasReceived(incrementalData, incrementalDataLength);
    if (m_options.dataBufferingPolicy == DoNotBufferData) {
        if (m_loader)
            m_loader->setDataBufferingPolicy(DoNotBufferData);
        clear();
    }
}

void CachedRawResource::addData(const char* data, unsigned length)
{
    ASSERT(m_options.dataBufferingPolicy == DoNotBufferData);
    notifyClientsDataWasReceived(data, length);
}

void CachedRawResource::finishLoading(ResourceBuffer* data)
{
    CachedResourceHandle<CachedRawResource> protect(this);
    DataBufferingPolicy dataBufferingPolicy = m_options.dataBufferingPolicy;
    if (dataBufferingPolicy == BufferData) {
        m_data = data;

        unsigned incrementalDataLength;
        const char* incrementalData = calculateIncrementalDataChunk(data, incrementalDataLength);
        if (data)
            setEncodedSize(data->size());
        notifyClientsDataWasReceived(incrementalData, incrementalDataLength);
    }

    CachedResource::finishLoading(data);
    if (dataBufferingPolicy == BufferData && m_options.dataBufferingPolicy == DoNotBufferData) {
        if (m_loader)
            m_loader->setDataBufferingPolicy(DoNotBufferData);
        clear();
    }
}

void CachedRawResource::notifyClientsDataWasReceived(const char* data, unsigned length)
{
    if (!length)
        return;

    CachedResourceHandle<CachedRawResource> protect(this);
    CachedResourceClientWalker<CachedRawResourceClient> w(m_clients);
    while (CachedRawResourceClient* c = w.next())
        c->dataReceived(this, data, length);
}

void CachedRawResource::didAddClient(CachedResourceClient* c)
{
    if (!hasClient(c))
        return;
    // The calls to the client can result in events running, potentially causing
    // this resource to be evicted from the cache and all clients to be removed,
    // so a protector is necessary.
    CachedResourceHandle<CachedRawResource> protect(this);
    CachedRawResourceClient* client = static_cast<CachedRawResourceClient*>(c);
    size_t redirectCount = m_redirectChain.size();
    for (size_t i = 0; i < redirectCount; i++) {
        RedirectPair redirect = m_redirectChain[i];
        ResourceRequest request(redirect.m_request);
        client->redirectReceived(this, request, redirect.m_redirectResponse);
        if (!hasClient(c))
            return;
    }
    ASSERT(redirectCount == m_redirectChain.size());

    if (!m_response.isNull())
        client->responseReceived(this, m_response);
    if (!hasClient(c))
        return;
    if (m_data)
        client->dataReceived(this, m_data->data(), m_data->size());
    if (!hasClient(c))
       return;
    CachedResource::didAddClient(client);
}

void CachedRawResource::allClientsRemoved()
{
    if (m_loader)
        m_loader->cancelIfNotFinishing();
}

void CachedRawResource::willSendRequest(ResourceRequest& request, const ResourceResponse& response)
{
    CachedResourceHandle<CachedRawResource> protect(this);
    if (!response.isNull()) {
        CachedResourceClientWalker<CachedRawResourceClient> w(m_clients);
        while (CachedRawResourceClient* c = w.next())
            c->redirectReceived(this, request, response);
        m_redirectChain.append(RedirectPair(request, response));
    }
    CachedResource::willSendRequest(request, response);
}

void CachedRawResource::responseReceived(const ResourceResponse& response)
{
    CachedResourceHandle<CachedRawResource> protect(this);
    if (!m_identifier)
        m_identifier = m_loader->identifier();
    CachedResource::responseReceived(response);
    CachedResourceClientWalker<CachedRawResourceClient> w(m_clients);
    while (CachedRawResourceClient* c = w.next())
        c->responseReceived(this, m_response);
}

void CachedRawResource::didSendData(unsigned long long bytesSent, unsigned long long totalBytesToBeSent)
{
    CachedResourceClientWalker<CachedRawResourceClient> w(m_clients);
    while (CachedRawResourceClient* c = w.next())
        c->dataSent(this, bytesSent, totalBytesToBeSent);
}

void CachedRawResource::switchClientsToRevalidatedResource()
{
    ASSERT(m_loader);
    // If we're in the middle of a successful revalidation, responseReceived() hasn't been called, so we haven't set m_identifier.
    ASSERT(!m_identifier);
    static_cast<CachedRawResource*>(resourceToRevalidate())->m_identifier = m_loader->identifier();
    CachedResource::switchClientsToRevalidatedResource();
}

void CachedRawResource::setDefersLoading(bool defers)
{
    if (m_loader)
        m_loader->setDefersLoading(defers);
}

void CachedRawResource::setDataBufferingPolicy(DataBufferingPolicy dataBufferingPolicy)
{
    m_options.dataBufferingPolicy = dataBufferingPolicy;
}

static bool shouldIgnoreHeaderForCacheReuse(AtomicString headerName)
{
    // FIXME: This list of headers that don't affect cache policy almost certainly isn't complete.
    DEFINE_STATIC_LOCAL(HashSet<AtomicString>, m_headers, ());
    if (m_headers.isEmpty()) {
        m_headers.add("Accept");
        m_headers.add("Cache-Control");
        m_headers.add("If-Modified-Since");
        m_headers.add("If-None-Match");
        m_headers.add("Origin");
        m_headers.add("Pragma");
        m_headers.add("Purpose");
        m_headers.add("Referer");
        m_headers.add("User-Agent");
    }
    return m_headers.contains(headerName);
}

bool CachedRawResource::canReuse(const ResourceRequest& newRequest) const
{
    if (m_options.dataBufferingPolicy == DoNotBufferData)
        return false;

    if (m_resourceRequest.httpMethod() != newRequest.httpMethod())
        return false;

    if (m_resourceRequest.httpBody() != newRequest.httpBody())
        return false;

    if (m_resourceRequest.allowCookies() != newRequest.allowCookies())
        return false;

    // Ensure most headers match the existing headers before continuing.
    // Note that the list of ignored headers includes some headers explicitly related to caching.
    // A more detailed check of caching policy will be performed later, this is simply a list of
    // headers that we might permit to be different and still reuse the existing CachedResource.
    const HTTPHeaderMap& newHeaders = newRequest.httpHeaderFields();
    const HTTPHeaderMap& oldHeaders = m_resourceRequest.httpHeaderFields();

    HTTPHeaderMap::const_iterator end = newHeaders.end();
    for (HTTPHeaderMap::const_iterator i = newHeaders.begin(); i != end; ++i) {
        AtomicString headerName = i->key;
        if (!shouldIgnoreHeaderForCacheReuse(headerName) && i->value != oldHeaders.get(headerName))
            return false;
    }

    end = oldHeaders.end();
    for (HTTPHeaderMap::const_iterator i = oldHeaders.begin(); i != end; ++i) {
        AtomicString headerName = i->key;
        if (!shouldIgnoreHeaderForCacheReuse(headerName) && i->value != newHeaders.get(headerName))
            return false;
    }

    for (size_t i = 0; i < m_redirectChain.size(); i++) {
        if (m_redirectChain[i].m_redirectResponse.cacheControlContainsNoStore())
            return false;
    }

    return true;
}

void CachedRawResource::clear()
{
    m_data.clear();
    setEncodedSize(0);
    if (m_loader)
        m_loader->clearResourceData();
}

} // namespace WebCore
