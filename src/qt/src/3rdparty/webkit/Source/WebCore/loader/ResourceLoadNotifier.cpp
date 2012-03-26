/*
 * Copyright (C) 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ResourceLoadNotifier.h"

#include "DocumentLoader.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "FrameLoaderClient.h"
#include "InspectorInstrumentation.h"
#include "Page.h"
#include "ProgressTracker.h"
#include "ResourceLoader.h"

namespace WebCore {

ResourceLoadNotifier::ResourceLoadNotifier(Frame* frame)
    : m_frame(frame)
{
}

void ResourceLoadNotifier::didReceiveAuthenticationChallenge(ResourceLoader* loader, const AuthenticationChallenge& currentWebChallenge)
{
    m_frame->loader()->client()->dispatchDidReceiveAuthenticationChallenge(loader->documentLoader(), loader->identifier(), currentWebChallenge);
}

void ResourceLoadNotifier::didCancelAuthenticationChallenge(ResourceLoader* loader, const AuthenticationChallenge& currentWebChallenge)
{
    m_frame->loader()->client()->dispatchDidCancelAuthenticationChallenge(loader->documentLoader(), loader->identifier(), currentWebChallenge);
}

void ResourceLoadNotifier::willSendRequest(ResourceLoader* loader, ResourceRequest& clientRequest, const ResourceResponse& redirectResponse)
{
    m_frame->loader()->applyUserAgent(clientRequest);

    dispatchWillSendRequest(loader->documentLoader(), loader->identifier(), clientRequest, redirectResponse);
}

void ResourceLoadNotifier::didReceiveResponse(ResourceLoader* loader, const ResourceResponse& r)
{
    loader->documentLoader()->addResponse(r);

    if (Page* page = m_frame->page())
        page->progress()->incrementProgress(loader->identifier(), r);

    dispatchDidReceiveResponse(loader->documentLoader(), loader->identifier(), r);
}

void ResourceLoadNotifier::didReceiveData(ResourceLoader* loader, const char* data, int dataLength, int encodedDataLength)
{
    if (Page* page = m_frame->page())
        page->progress()->incrementProgress(loader->identifier(), data, dataLength);

    dispatchDidReceiveContentLength(loader->documentLoader(), loader->identifier(), dataLength, encodedDataLength);
}

void ResourceLoadNotifier::didFinishLoad(ResourceLoader* loader, double finishTime)
{    
    if (Page* page = m_frame->page())
        page->progress()->completeProgress(loader->identifier());
    dispatchDidFinishLoading(loader->documentLoader(), loader->identifier(), finishTime);
}

void ResourceLoadNotifier::didFailToLoad(ResourceLoader* loader, const ResourceError& error)
{
    if (Page* page = m_frame->page())
        page->progress()->completeProgress(loader->identifier());

    if (!error.isNull())
        m_frame->loader()->client()->dispatchDidFailLoading(loader->documentLoader(), loader->identifier(), error);

    InspectorInstrumentation::didFailLoading(m_frame, loader->identifier(), error);
}

void ResourceLoadNotifier::assignIdentifierToInitialRequest(unsigned long identifier, DocumentLoader* loader, const ResourceRequest& request)
{
    m_frame->loader()->client()->assignIdentifierToInitialRequest(identifier, loader, request);
}

void ResourceLoadNotifier::dispatchWillSendRequest(DocumentLoader* loader, unsigned long identifier, ResourceRequest& request, const ResourceResponse& redirectResponse)
{
    StringImpl* oldRequestURL = request.url().string().impl();
    m_frame->loader()->documentLoader()->didTellClientAboutLoad(request.url());

    m_frame->loader()->client()->dispatchWillSendRequest(loader, identifier, request, redirectResponse);

    // If the URL changed, then we want to put that new URL in the "did tell client" set too.
    if (!request.isNull() && oldRequestURL != request.url().string().impl())
        m_frame->loader()->documentLoader()->didTellClientAboutLoad(request.url());

    InspectorInstrumentation::willSendRequest(m_frame, identifier, loader, request, redirectResponse);

    // Report WebTiming for all frames.
    if (loader && !request.isNull() && request.url() == loader->requestURL())
        request.setReportLoadTiming(true);
}

void ResourceLoadNotifier::dispatchDidReceiveResponse(DocumentLoader* loader, unsigned long identifier, const ResourceResponse& r)
{
    InspectorInstrumentationCookie cookie = InspectorInstrumentation::willReceiveResourceResponse(m_frame, identifier, r);
    m_frame->loader()->client()->dispatchDidReceiveResponse(loader, identifier, r);
    InspectorInstrumentation::didReceiveResourceResponse(cookie, identifier, loader, r);
}

void ResourceLoadNotifier::dispatchDidReceiveContentLength(DocumentLoader* loader, unsigned long identifier, int dataLength, int encodedDataLength)
{
    m_frame->loader()->client()->dispatchDidReceiveContentLength(loader, identifier, dataLength);

    InspectorInstrumentation::didReceiveContentLength(m_frame, identifier, dataLength, encodedDataLength);
}

void ResourceLoadNotifier::dispatchDidFinishLoading(DocumentLoader* loader, unsigned long identifier, double finishTime)
{
    m_frame->loader()->client()->dispatchDidFinishLoading(loader, identifier);

    InspectorInstrumentation::didFinishLoading(m_frame, identifier, finishTime);
}

void ResourceLoadNotifier::dispatchTransferLoadingResourceFromPage(unsigned long identifier, DocumentLoader* loader, const ResourceRequest& request, Page* oldPage)
{
    ASSERT(oldPage != m_frame->page());
    m_frame->loader()->client()->transferLoadingResourceFromPage(identifier, loader, request, oldPage);

    oldPage->progress()->completeProgress(identifier);
}

void ResourceLoadNotifier::sendRemainingDelegateMessages(DocumentLoader* loader, unsigned long identifier, const ResourceResponse& response, int dataLength, int encodedDataLength, const ResourceError& error)
{
    if (!response.isNull())
        dispatchDidReceiveResponse(loader, identifier, response);

    if (dataLength > 0)
        dispatchDidReceiveContentLength(loader, identifier, dataLength, encodedDataLength);

    if (error.isNull())
        dispatchDidFinishLoading(loader, identifier, 0);
    else
        m_frame->loader()->client()->dispatchDidFailLoading(loader, identifier, error);
}

} // namespace WebCore
