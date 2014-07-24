/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
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
 
#include "config.h"
#include "IconLoader.h"

#include "CachedRawResource.h"
#include "CachedResourceLoader.h"
#include "CachedResourceRequest.h"
#include "CachedResourceRequestInitiators.h"
#include "Document.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "IconController.h"
#include "IconDatabase.h"
#include "Logging.h"
#include "ResourceBuffer.h"
#include "ResourceRequest.h"
#include <wtf/text/CString.h>

namespace WebCore {

IconLoader::IconLoader(Frame* frame)
    : m_frame(frame)
{
}

PassOwnPtr<IconLoader> IconLoader::create(Frame* frame)
{
    return adoptPtr(new IconLoader(frame));
}

IconLoader::~IconLoader()
{
}

void IconLoader::startLoading()
{
    if (m_resource || !m_frame->document())
        return;

    CachedResourceRequest request(ResourceRequest(m_frame->loader()->icon()->url()), ResourceLoaderOptions(SendCallbacks, SniffContent, BufferData, DoNotAllowStoredCredentials, DoNotAskClientForAnyCredentials, DoSecurityCheck, UseDefaultOriginRestrictionsForType));

#if PLATFORM(BLACKBERRY)
    request.mutableResourceRequest().setTargetType(ResourceRequest::TargetIsFavicon);
#endif
    request.mutableResourceRequest().setPriority(ResourceLoadPriorityLow);
    request.setInitiator(cachedResourceRequestInitiators().icon);

    m_resource = m_frame->document()->cachedResourceLoader()->requestRawResource(request);
    if (m_resource)
        m_resource->addClient(this);
    else
        LOG_ERROR("Failed to start load for icon at url %s", m_frame->loader()->icon()->url().string().ascii().data());
}

void IconLoader::stopLoading()
{
    if (m_resource) {
        m_resource->removeClient(this);
        m_resource = 0;
    }
}

void IconLoader::notifyFinished(CachedResource* resource)
{
    ASSERT(resource == m_resource);

    // If we got a status code indicating an invalid response, then lets
    // ignore the data and not try to decode the error page as an icon.
    RefPtr<ResourceBuffer> data = resource->resourceBuffer();
    int status = resource->response().httpStatusCode();
    if (status && (status < 200 || status > 299))
        data = 0;

    static const char pdfMagicNumber[] = "%PDF";
    static unsigned pdfMagicNumberLength = sizeof(pdfMagicNumber) - 1;
    if (data && data->size() >= pdfMagicNumberLength && !memcmp(data->data(), pdfMagicNumber, pdfMagicNumberLength)) {
        LOG(IconDatabase, "IconLoader::finishLoading() - Ignoring icon at %s because it appears to be a PDF", resource->url().string().ascii().data());
        data = 0;
    }

    LOG(IconDatabase, "IconLoader::finishLoading() - Committing iconURL %s to database", resource->url().string().ascii().data());
    m_frame->loader()->icon()->commitToDatabase(resource->url());
    // Setting the icon data only after committing to the database ensures that the data is
    // kept in memory (so it does not have to be read from the database asynchronously), since
    // there is a page URL referencing it.
    iconDatabase().setIconDataForIconURL(data ? data->sharedBuffer() : 0, resource->url().string());
    m_frame->loader()->client()->dispatchDidReceiveIcon();
    stopLoading();
}

}
