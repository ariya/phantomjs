/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
#include "WebHistoryClient.h"

#include "WKAPICast.h"
#include "WebNavigationData.h"
#include <wtf/RefPtr.h>

using namespace WebCore;

namespace WebKit {

void WebHistoryClient::didNavigateWithNavigationData(WebContext* context, WebPageProxy* page, const WebNavigationDataStore& navigationDataStore, WebFrameProxy* frame)
{
    if (!m_client.didNavigateWithNavigationData)
        return;

    RefPtr<WebNavigationData> navigationData = WebNavigationData::create(navigationDataStore); 
    m_client.didNavigateWithNavigationData(toAPI(context), toAPI(page), toAPI(navigationData.get()), toAPI(frame), m_client.clientInfo);
}

void WebHistoryClient::didPerformClientRedirect(WebContext* context, WebPageProxy* page, const String& sourceURL, const String& destinationURL, WebFrameProxy* frame)
{
    if (!m_client.didPerformClientRedirect)
        return;

    m_client.didPerformClientRedirect(toAPI(context), toAPI(page), toURLRef(sourceURL.impl()), toURLRef(destinationURL.impl()), toAPI(frame), m_client.clientInfo);
}

void WebHistoryClient::didPerformServerRedirect(WebContext* context, WebPageProxy* page, const String& sourceURL, const String& destinationURL, WebFrameProxy* frame)
{
    if (!m_client.didPerformServerRedirect)
        return;

    m_client.didPerformServerRedirect(toAPI(context), toAPI(page), toURLRef(sourceURL.impl()), toURLRef(destinationURL.impl()), toAPI(frame), m_client.clientInfo);
}

void WebHistoryClient::didUpdateHistoryTitle(WebContext* context, WebPageProxy* page, const String& title, const String& url, WebFrameProxy* frame)
{
    if (!m_client.didUpdateHistoryTitle)
        return;

    m_client.didUpdateHistoryTitle(toAPI(context), toAPI(page), toAPI(title.impl()), toURLRef(url.impl()), toAPI(frame), m_client.clientInfo);
}

void WebHistoryClient::populateVisitedLinks(WebContext* context)
{
    if (!m_client.populateVisitedLinks)
        return;

    m_client.populateVisitedLinks(toAPI(context), m_client.clientInfo);
}

} // namespace WebKit
