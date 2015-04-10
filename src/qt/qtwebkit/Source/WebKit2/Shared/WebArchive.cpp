/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#include "config.h"
#include "WebArchive.h"

#if PLATFORM(MAC)

#include "ImmutableArray.h"
#include "WebArchiveResource.h"
#include "WebData.h"
#include <WebCore/LegacyWebArchive.h>
#include <wtf/RetainPtr.h>

using namespace WebCore;

namespace WebKit {

PassRefPtr<WebArchive> WebArchive::create(WebArchiveResource* mainResource, ImmutableArray* subresources, ImmutableArray* subframeArchives)
{
    return adoptRef(new WebArchive(mainResource, subresources, subframeArchives));
}

PassRefPtr<WebArchive> WebArchive::create(WebData* data)
{
    return adoptRef(new WebArchive(data));
}

PassRefPtr<WebArchive> WebArchive::create(PassRefPtr<LegacyWebArchive> legacyWebArchive)
{
    return adoptRef(new WebArchive(legacyWebArchive));
}

PassRefPtr<WebArchive> WebArchive::create(Range* range)
{
    return adoptRef(new WebArchive(LegacyWebArchive::create(range)));
}

WebArchive::WebArchive(WebArchiveResource* mainResource, ImmutableArray* subresources, ImmutableArray* subframeArchives)
    : m_cachedMainResource(mainResource)
    , m_cachedSubresources(subresources)
    , m_cachedSubframeArchives(subframeArchives)
{
    RefPtr<ArchiveResource> coreMainResource = m_cachedMainResource->coreArchiveResource();

    Vector<PassRefPtr<ArchiveResource>> coreArchiveResources;
    for (size_t i = 0; i < m_cachedSubresources->size(); ++i) {
        RefPtr<WebArchiveResource> resource = m_cachedSubresources->at<WebArchiveResource>(i);
        ASSERT(resource);
        coreArchiveResources.append(resource->coreArchiveResource());
    }

    Vector<PassRefPtr<LegacyWebArchive>> coreSubframeLegacyWebArchives;
    for (size_t i = 0; i < m_cachedSubframeArchives->size(); ++i) {
        RefPtr<WebArchive> subframeWebArchive = m_cachedSubframeArchives->at<WebArchive>(i);
        ASSERT(subframeWebArchive);
        coreSubframeLegacyWebArchives.append(subframeWebArchive->coreLegacyWebArchive());
    }

    m_legacyWebArchive = LegacyWebArchive::create(coreMainResource.release(), coreArchiveResources, coreSubframeLegacyWebArchives);
}

WebArchive::WebArchive(WebData* data)
{
    RefPtr<SharedBuffer> buffer = SharedBuffer::create(data->bytes(), data->size());
    m_legacyWebArchive = LegacyWebArchive::create(buffer.get());
}

WebArchive::WebArchive(PassRefPtr<LegacyWebArchive> legacyWebArchive)
    : m_legacyWebArchive(legacyWebArchive)
{
}

WebArchive::~WebArchive()
{
}

WebArchiveResource* WebArchive::mainResource()
{
    if (!m_cachedMainResource)
        m_cachedMainResource = WebArchiveResource::create(m_legacyWebArchive->mainResource());
    return m_cachedMainResource.get();
}

ImmutableArray* WebArchive::subresources()
{
    if (!m_cachedSubresources) {
        Vector<RefPtr<APIObject>> subresources;
        subresources.reserveCapacity(m_legacyWebArchive->subresources().size());
        for (unsigned i = 0; i < m_legacyWebArchive->subresources().size(); ++i)
            subresources.append(WebArchiveResource::create(m_legacyWebArchive->subresources()[i].get()));

        m_cachedSubresources = ImmutableArray::adopt(subresources);
    }

    return m_cachedSubresources.get();
}

ImmutableArray* WebArchive::subframeArchives()
{
    if (!m_cachedSubframeArchives) {
        Vector<RefPtr<APIObject>> subframeWebArchives;
        subframeWebArchives.reserveCapacity(m_legacyWebArchive->subframeArchives().size());
        for (unsigned i = 0; i < m_legacyWebArchive->subframeArchives().size(); ++i)
            subframeWebArchives.append(WebArchive::create(static_cast<LegacyWebArchive*>(m_legacyWebArchive->subframeArchives()[i].get())));

        m_cachedSubframeArchives = ImmutableArray::adopt(subframeWebArchives);
    }

    return m_cachedSubframeArchives.get();
}

static void releaseCFData(unsigned char*, const void* data)
{
    // Balanced by CFRetain in WebArchive::data().
    CFRelease(data);
}

PassRefPtr<WebData> WebArchive::data()
{
    RetainPtr<CFDataRef> rawDataRepresentation = m_legacyWebArchive->rawDataRepresentation();

    // Balanced by CFRelease in releaseCFData.
    CFRetain(rawDataRepresentation.get());

    return WebData::createWithoutCopying(CFDataGetBytePtr(rawDataRepresentation.get()), CFDataGetLength(rawDataRepresentation.get()), releaseCFData, rawDataRepresentation.get());
}

LegacyWebArchive* WebArchive::coreLegacyWebArchive()
{
    return m_legacyWebArchive.get();
}

} // namespace WebKit

#endif // PLATFORM(MAC)
