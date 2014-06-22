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
#include "WebArchiveResource.h"

#if PLATFORM(MAC)

#include "WebData.h"
#include <WebCore/ArchiveResource.h>
#include <WebCore/KURL.h>
#include <wtf/RetainPtr.h>

using namespace WebCore;

namespace WebKit {

PassRefPtr<WebArchiveResource> WebArchiveResource::create(WebData* data, const String& URL, const String& MIMEType, const String& textEncoding)
{
    return adoptRef(new WebArchiveResource(data, URL, MIMEType, textEncoding));
}

PassRefPtr<WebArchiveResource> WebArchiveResource::create(PassRefPtr<ArchiveResource> archiveResource)
{
    return adoptRef(new WebArchiveResource(archiveResource));
}

WebArchiveResource::WebArchiveResource(WebData* data, const String& URL, const String& MIMEType, const String& textEncoding)
    : m_archiveResource(ArchiveResource::create(SharedBuffer::create(data->bytes(), data->size()), KURL(KURL(), URL), MIMEType, textEncoding, String()))
{
}

WebArchiveResource::WebArchiveResource(PassRefPtr<ArchiveResource> archiveResource)
    : m_archiveResource(archiveResource)
{
}

WebArchiveResource::~WebArchiveResource()
{
}

static void releaseCFData(unsigned char*, const void* data)
{
    // Balanced by CFRetain in WebArchiveResource::data().
    CFRelease(data);
}

PassRefPtr<WebData> WebArchiveResource::data()
{
    RetainPtr<CFDataRef> cfData = adoptCF(m_archiveResource->data()->createCFData());

    // Balanced by CFRelease in releaseCFData.
    CFRetain(cfData.get());

    return WebData::createWithoutCopying(CFDataGetBytePtr(cfData.get()), CFDataGetLength(cfData.get()), releaseCFData, cfData.get());
}

String WebArchiveResource::URL()
{
    return m_archiveResource->url().string();
}

String WebArchiveResource::MIMEType()
{
    return m_archiveResource->mimeType();
}

String WebArchiveResource::textEncoding()
{
    return m_archiveResource->textEncoding();
}

ArchiveResource* WebArchiveResource::coreArchiveResource()
{
    return m_archiveResource.get();
}

} // namespace WebKit

#endif // PLATFORM(MAC)
