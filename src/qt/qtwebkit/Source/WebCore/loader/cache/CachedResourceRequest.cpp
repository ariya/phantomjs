/*
 * Copyright (C) 2012 Google, Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. ``AS IS'' AND ANY
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

#include "config.h"
#include "CachedResourceRequest.h"

#include "CachedResourceLoader.h"
#include "Document.h"
#include "Element.h"

namespace WebCore {

CachedResourceRequest::CachedResourceRequest(const ResourceRequest& resourceRequest, const String& charset, ResourceLoadPriority priority)
    : m_resourceRequest(resourceRequest)
    , m_charset(charset)
    , m_options(CachedResourceLoader::defaultCachedResourceOptions())
    , m_priority(priority)
    , m_forPreload(false)
    , m_defer(NoDefer)
{
}

CachedResourceRequest::CachedResourceRequest(const ResourceRequest& resourceRequest, const ResourceLoaderOptions& options)
    : m_resourceRequest(resourceRequest)
    , m_options(options)
    , m_priority(ResourceLoadPriorityUnresolved)
    , m_forPreload(false)
    , m_defer(NoDefer)
{
}

CachedResourceRequest::CachedResourceRequest(const ResourceRequest& resourceRequest, ResourceLoadPriority priority)
    : m_resourceRequest(resourceRequest)
    , m_options(CachedResourceLoader::defaultCachedResourceOptions())
    , m_priority(priority)
    , m_forPreload(false)
    , m_defer(NoDefer)
{
}

CachedResourceRequest::~CachedResourceRequest()
{
}

void CachedResourceRequest::setInitiator(PassRefPtr<Element> element)
{
    ASSERT(!m_initiatorElement && m_initiatorName.isEmpty());
    m_initiatorElement = element;
}

void CachedResourceRequest::setInitiator(const AtomicString& name)
{
    ASSERT(!m_initiatorElement && m_initiatorName.isEmpty());
    m_initiatorName = name;
}

const AtomicString& CachedResourceRequest::initiatorName() const
{
    if (m_initiatorElement)
        return m_initiatorElement->localName();
    if (!m_initiatorName.isEmpty())
        return m_initiatorName;

    DEFINE_STATIC_LOCAL(AtomicString, defaultName, ("resource", AtomicString::ConstructFromLiteral));
    return defaultName;
}

} // namespace WebCore
