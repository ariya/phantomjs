/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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
#include "ContextHistoryClientEfl.h"

#include "EwkView.h"
#include "WKAPICast.h"
#include "WKContext.h"
#include "WKEinaSharedString.h"
#include "WKString.h"
#include "ewk_context_private.h"
#include "ewk_navigation_data.h"
#include "ewk_navigation_data_private.h"
#include "ewk_url_response.h"
#include "ewk_url_response_private.h"

namespace WebKit {

static inline const ContextHistoryClientEfl* toContextHistoryClientEfl(const void* clientInfo)
{
    ASSERT(clientInfo);
    return static_cast<const ContextHistoryClientEfl*>(clientInfo);
}

void ContextHistoryClientEfl::didNavigateWithNavigationData(WKContextRef, WKPageRef page, WKNavigationDataRef navigationData, WKFrameRef, const void* clientInfo)
{
    const ContextHistoryClientEfl* historyClient = toContextHistoryClientEfl(clientInfo);

    if (!historyClient->m_navigate)
        return;

    RefPtr<EwkNavigationData> navigationDataEwk = EwkNavigationData::create(navigationData);
    historyClient->m_navigate(EwkView::toEvasObject(page), navigationDataEwk.get(), historyClient->m_userData);
}

void ContextHistoryClientEfl::didPerformClientRedirect(WKContextRef, WKPageRef page, WKURLRef sourceURL, WKURLRef destinationURL, WKFrameRef, const void* clientInfo)
{
    const ContextHistoryClientEfl* historyClient = toContextHistoryClientEfl(clientInfo);

    if (!historyClient->m_clientRedirect)
        return;

    WKEinaSharedString sourceURLString(sourceURL);
    WKEinaSharedString destinationURLString(destinationURL);

    historyClient->m_clientRedirect(EwkView::toEvasObject(page), sourceURLString, destinationURLString, historyClient->m_userData);
}

void ContextHistoryClientEfl::didPerformServerRedirect(WKContextRef, WKPageRef page, WKURLRef sourceURL, WKURLRef destinationURL, WKFrameRef, const void* clientInfo)
{
    const ContextHistoryClientEfl* historyClient = toContextHistoryClientEfl(clientInfo);

    if (!historyClient->m_serverRedirect)
        return;

    WKEinaSharedString sourceURLString(sourceURL);
    WKEinaSharedString destinationURLString(destinationURL);

    historyClient->m_serverRedirect(EwkView::toEvasObject(page), sourceURLString, destinationURLString, historyClient->m_userData);
}

void ContextHistoryClientEfl::didUpdateHistoryTitle(WKContextRef, WKPageRef page, WKStringRef title, WKURLRef URL, WKFrameRef, const void* clientInfo)
{
    const ContextHistoryClientEfl* historyClient = toContextHistoryClientEfl(clientInfo);

    if (!historyClient->m_titleUpdated)
        return;

    WKEinaSharedString titleString(title);
    WKEinaSharedString stringURL(URL);

    historyClient->m_titleUpdated(EwkView::toEvasObject(page), titleString, stringURL, historyClient->m_userData);
}

void ContextHistoryClientEfl::populateVisitedLinks(WKContextRef, const void* clientInfo)
{
    const ContextHistoryClientEfl* historyClient = toContextHistoryClientEfl(clientInfo);

    if (!historyClient->m_populateVisitedLinks)
        return;

    historyClient->m_populateVisitedLinks(historyClient->m_userData);
}

ContextHistoryClientEfl::ContextHistoryClientEfl(WKContextRef context)
    : m_context(context)
    , m_userData(0)
    , m_navigate(0)
    , m_clientRedirect(0)
    , m_serverRedirect(0)
    , m_titleUpdated(0)
    , m_populateVisitedLinks(0)
{
    ASSERT(m_context);

    WKContextHistoryClient wkHistoryClient;
    memset(&wkHistoryClient, 0, sizeof(WKContextHistoryClient));

    wkHistoryClient.version = kWKContextHistoryClientCurrentVersion;
    wkHistoryClient.clientInfo = this;

    wkHistoryClient.didNavigateWithNavigationData = didNavigateWithNavigationData;
    wkHistoryClient.didPerformClientRedirect = didPerformClientRedirect;
    wkHistoryClient.didPerformServerRedirect = didPerformServerRedirect;
    wkHistoryClient.didUpdateHistoryTitle = didUpdateHistoryTitle;
    wkHistoryClient.populateVisitedLinks = populateVisitedLinks;

    WKContextSetHistoryClient(m_context.get(), &wkHistoryClient);
}

ContextHistoryClientEfl::~ContextHistoryClientEfl()
{
    WKContextSetHistoryClient(m_context.get(), 0);
}

void ContextHistoryClientEfl::setCallbacks(Ewk_History_Navigation_Cb navigate, Ewk_History_Client_Redirection_Cb clientRedirect, Ewk_History_Server_Redirection_Cb serverRedirect, Ewk_History_Title_Update_Cb titleUpdate, Ewk_History_Populate_Visited_Links_Cb populateVisitedLinks, void* data)
{
    m_navigate = navigate;
    m_clientRedirect = clientRedirect;
    m_serverRedirect = serverRedirect;
    m_titleUpdated = titleUpdate;
    m_populateVisitedLinks = populateVisitedLinks;
    m_userData = data;
}

} // namespace WebKit
