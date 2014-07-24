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

#ifndef WebArchive_h
#define WebArchive_h

#if PLATFORM(MAC)

#include "APIObject.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore {
class LegacyWebArchive;
class Range;
}

namespace WebKit {

class ImmutableArray;
class WebArchiveResource;
class WebData;

class WebArchive : public TypedAPIObject<APIObject::TypeWebArchive> {
public:
    virtual ~WebArchive();

    static PassRefPtr<WebArchive> create(WebArchiveResource* mainResource, ImmutableArray* subresources, ImmutableArray* subframeArchives);
    static PassRefPtr<WebArchive> create(WebData*);
    static PassRefPtr<WebArchive> create(PassRefPtr<WebCore::LegacyWebArchive>);
    static PassRefPtr<WebArchive> create(WebCore::Range*);

    WebArchiveResource* mainResource();
    ImmutableArray* subresources();
    ImmutableArray* subframeArchives();

    PassRefPtr<WebData> data();

    WebCore::LegacyWebArchive* coreLegacyWebArchive();

private:
    WebArchive(WebArchiveResource* mainResource, ImmutableArray* subresources, ImmutableArray* subframeArchives);
    WebArchive(WebData*);
    WebArchive(PassRefPtr<WebCore::LegacyWebArchive>);

    RefPtr<WebCore::LegacyWebArchive> m_legacyWebArchive;
    RefPtr<WebArchiveResource> m_cachedMainResource;
    RefPtr<ImmutableArray> m_cachedSubresources;
    RefPtr<ImmutableArray> m_cachedSubframeArchives;
};

} // namespace WebKit

#endif // PLATFORM(MAC)

#endif // WebArchive_h
