/*
    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller (mueller@kde.org)
    Copyright (C) 2002 Waldo Bastian (bastian@kde.org)
    Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
    Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "CachedResourceRequest.h"

#include "CachedImage.h"
#include "CachedResource.h"
#include "CachedResourceLoader.h"
#include "Document.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "Logging.h"
#include "MemoryCache.h"
#include "ResourceHandle.h"
#include "ResourceLoadScheduler.h"
#include "ResourceRequest.h"
#include "ResourceResponse.h"
#include "SharedBuffer.h"
#include <wtf/Assertions.h>
#include <wtf/Vector.h>
#include <wtf/text/CString.h>

namespace WebCore {
    
static ResourceRequest::TargetType cachedResourceTypeToTargetType(CachedResource::Type type, ResourceLoadPriority priority)
{
#if !ENABLE(LINK_PREFETCH)
    UNUSED_PARAM(priority);
#endif
    switch (type) {
    case CachedResource::CSSStyleSheet:
#if ENABLE(XSLT)
    case CachedResource::XSLStyleSheet:
#endif
        return ResourceRequest::TargetIsStyleSheet;
    case CachedResource::Script: 
        return ResourceRequest::TargetIsScript;
    case CachedResource::FontResource:
        return ResourceRequest::TargetIsFontResource;
    case CachedResource::ImageResource:
        return ResourceRequest::TargetIsImage;
#if ENABLE(LINK_PREFETCH)
    case CachedResource::LinkResource:
        if (priority == ResourceLoadPriorityLowest)
            return ResourceRequest::TargetIsPrefetch;
        return ResourceRequest::TargetIsSubresource;
#endif
    }
    ASSERT_NOT_REACHED();
    return ResourceRequest::TargetIsSubresource;
}

CachedResourceRequest::CachedResourceRequest(CachedResourceLoader* cachedResourceLoader, CachedResource* resource, bool incremental)
    : m_cachedResourceLoader(cachedResourceLoader)
    , m_resource(resource)
    , m_incremental(incremental)
    , m_multipart(false)
    , m_finishing(false)
{
    m_resource->setRequest(this);
}

CachedResourceRequest::~CachedResourceRequest()
{
    m_resource->setRequest(0);
}

PassRefPtr<CachedResourceRequest> CachedResourceRequest::load(CachedResourceLoader* cachedResourceLoader, CachedResource* resource, bool incremental, SecurityCheckPolicy securityCheck, bool sendResourceLoadCallbacks)
{
    RefPtr<CachedResourceRequest> request = adoptRef(new CachedResourceRequest(cachedResourceLoader, resource, incremental));

    ResourceRequest resourceRequest(resource->url());
    resourceRequest.setTargetType(cachedResourceTypeToTargetType(resource->type(), resource->loadPriority()));

    if (!resource->accept().isEmpty())
        resourceRequest.setHTTPAccept(resource->accept());

    if (resource->isCacheValidator()) {
        CachedResource* resourceToRevalidate = resource->resourceToRevalidate();
        ASSERT(resourceToRevalidate->canUseCacheValidator());
        ASSERT(resourceToRevalidate->isLoaded());
        const String& lastModified = resourceToRevalidate->response().httpHeaderField("Last-Modified");
        const String& eTag = resourceToRevalidate->response().httpHeaderField("ETag");
        if (!lastModified.isEmpty() || !eTag.isEmpty()) {
            ASSERT(cachedResourceLoader->cachePolicy() != CachePolicyReload);
            if (cachedResourceLoader->cachePolicy() == CachePolicyRevalidate)
                resourceRequest.setHTTPHeaderField("Cache-Control", "max-age=0");
            if (!lastModified.isEmpty())
                resourceRequest.setHTTPHeaderField("If-Modified-Since", lastModified);
            if (!eTag.isEmpty())
                resourceRequest.setHTTPHeaderField("If-None-Match", eTag);
        }
    }
    
#if ENABLE(LINK_PREFETCH)
    if (resource->type() == CachedResource::LinkResource)
        resourceRequest.setHTTPHeaderField("Purpose", "prefetch");
#endif

    ResourceLoadPriority priority = resource->loadPriority();
    resourceRequest.setPriority(priority);

    RefPtr<SubresourceLoader> loader = resourceLoadScheduler()->scheduleSubresourceLoad(cachedResourceLoader->document()->frame(),
        request.get(), resourceRequest, priority, securityCheck, sendResourceLoadCallbacks);
    if (!loader || loader->reachedTerminalState()) {
        // FIXME: What if resources in other frames were waiting for this revalidation?
        LOG(ResourceLoading, "Cannot start loading '%s'", resource->url().latin1().data());
        cachedResourceLoader->decrementRequestCount(resource);
        cachedResourceLoader->loadFinishing();
        if (resource->resourceToRevalidate()) 
            memoryCache()->revalidationFailed(resource); 
        resource->error(CachedResource::LoadError);
        cachedResourceLoader->loadDone(0);
        return 0;
    }
    request->m_loader = loader;
    return request.release();
}

void CachedResourceRequest::willSendRequest(SubresourceLoader*, ResourceRequest&, const ResourceResponse&)
{
    m_resource->setRequestedFromNetworkingLayer();
}

void CachedResourceRequest::didFinishLoading(SubresourceLoader* loader, double)
{
    if (m_finishing)
        return;

    ASSERT(loader == m_loader.get());
    ASSERT(!m_resource->resourceToRevalidate());
    LOG(ResourceLoading, "Received '%s'.", m_resource->url().latin1().data());

    // Prevent the document from being destroyed before we are done with
    // the cachedResourceLoader that it will delete when the document gets deleted.
    RefPtr<Document> protector(m_cachedResourceLoader->document());
    if (!m_multipart)
        m_cachedResourceLoader->decrementRequestCount(m_resource);
    m_finishing = true;

    // If we got a 4xx response, we're pretending to have received a network
    // error, so we can't send the successful data() and finish() callbacks.
    if (!m_resource->errorOccurred()) {
        m_cachedResourceLoader->loadFinishing();
        m_resource->data(loader->resourceData(), true);
        if (!m_resource->errorOccurred())
            m_resource->finish();
    }
    m_cachedResourceLoader->loadDone(this);
}

void CachedResourceRequest::didFail(SubresourceLoader*, const ResourceError&)
{
    if (!m_loader)
        return;
    didFail();
}

void CachedResourceRequest::didFail(bool cancelled)
{
    if (m_finishing)
        return;

    LOG(ResourceLoading, "Failed to load '%s' (cancelled=%d).\n", m_resource->url().latin1().data(), cancelled);

    // Prevent the document from being destroyed before we are done with
    // the cachedResourceLoader that it will delete when the document gets deleted.
    RefPtr<Document> protector(m_cachedResourceLoader->document());
    if (!m_multipart)
        m_cachedResourceLoader->decrementRequestCount(m_resource);
    m_finishing = true;
    m_loader->clearClient();

    if (m_resource->resourceToRevalidate())
        memoryCache()->revalidationFailed(m_resource);

    if (!cancelled) {
        m_cachedResourceLoader->loadFinishing();
        m_resource->error(CachedResource::LoadError);
    }

    if (cancelled || !m_resource->isPreloaded())
        memoryCache()->remove(m_resource);
    
    m_cachedResourceLoader->loadDone(this);
}

void CachedResourceRequest::didReceiveResponse(SubresourceLoader* loader, const ResourceResponse& response)
{
    ASSERT(loader == m_loader.get());
    if (m_resource->isCacheValidator()) {
        if (response.httpStatusCode() == 304) {
            // 304 Not modified / Use local copy
            loader->clearClient();
            RefPtr<Document> protector(m_cachedResourceLoader->document());
            m_cachedResourceLoader->decrementRequestCount(m_resource);
            m_finishing = true;

            // Existing resource is ok, just use it updating the expiration time.
            memoryCache()->revalidationSucceeded(m_resource, response);
            
            if (m_cachedResourceLoader->frame())
                m_cachedResourceLoader->frame()->loader()->checkCompleted();

            m_cachedResourceLoader->loadDone(this);
            return;
        } 
        // Did not get 304 response, continue as a regular resource load.
        memoryCache()->revalidationFailed(m_resource);
    }

    m_resource->setResponse(response);

    String encoding = response.textEncodingName();
    if (!encoding.isNull())
        m_resource->setEncoding(encoding);
    
    if (m_multipart) {
        ASSERT(m_resource->isImage());
        static_cast<CachedImage*>(m_resource)->clear();
        if (m_cachedResourceLoader->frame())
            m_cachedResourceLoader->frame()->loader()->checkCompleted();
    } else if (response.isMultipart()) {
        m_multipart = true;
        
        // We don't count multiParts in a CachedResourceLoader's request count
        m_cachedResourceLoader->decrementRequestCount(m_resource);

        // If we get a multipart response, we must have a handle
        ASSERT(loader->handle());
        if (!m_resource->isImage())
            loader->handle()->cancel();
    }
}

void CachedResourceRequest::didReceiveData(SubresourceLoader* loader, const char* data, int size)
{
    ASSERT(loader == m_loader.get());
    ASSERT(!m_resource->isCacheValidator());
    
    if (m_resource->errorOccurred())
        return;

    if (m_resource->response().httpStatusCode() >= 400) {
        if (!m_resource->shouldIgnoreHTTPStatusCodeErrors())
            m_resource->error(CachedResource::LoadError);
        return;
    }

    // Set the data.
    if (m_multipart) {
        // The loader delivers the data in a multipart section all at once, send eof.
        // The resource data will change as the next part is loaded, so we need to make a copy.
        RefPtr<SharedBuffer> copiedData = SharedBuffer::create(data, size);
        m_resource->data(copiedData.release(), true);
    } else if (m_incremental)
        m_resource->data(loader->resourceData(), false);
}

void CachedResourceRequest::didReceiveCachedMetadata(SubresourceLoader*, const char* data, int size)
{
    ASSERT(!m_resource->isCacheValidator());
    m_resource->setSerializedCachedMetadata(data, size);
}

} //namespace WebCore
