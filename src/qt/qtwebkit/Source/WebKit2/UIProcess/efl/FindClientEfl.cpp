/*
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
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
#include "FindClientEfl.h"

#include "EwkView.h"
#include "WKPage.h"

using namespace EwkViewCallbacks;

namespace WebKit {

static inline FindClientEfl* toFindClientEfl(const void* clientInfo)
{
    return static_cast<FindClientEfl*>(const_cast<void*>(clientInfo));
}

void FindClientEfl::didFindString(WKPageRef, WKStringRef, unsigned matchCount, const void* clientInfo)
{
    FindClientEfl* findClient = toFindClientEfl(clientInfo);
    findClient->m_view->smartCallback<TextFound>().call(&matchCount);
}

void FindClientEfl::didFailToFindString(WKPageRef, WKStringRef, const void* clientInfo)
{
    FindClientEfl* findClient = toFindClientEfl(clientInfo);
    unsigned matchCount = 0;
    findClient->m_view->smartCallback<TextFound>().call(&matchCount);
}

FindClientEfl::FindClientEfl(EwkView* viewImpl)
    : m_view(viewImpl)
{
    WKPageRef pageRef = m_view->wkPage();
    ASSERT(pageRef);

    WKPageFindClient findClient;
    memset(&findClient, 0, sizeof(WKPageFindClient));
    findClient.version = kWKPageFindClientCurrentVersion;
    findClient.clientInfo = this;
    findClient.didFindString = didFindString;
    findClient.didFailToFindString = didFailToFindString;
    findClient.didCountStringMatches = didFindString;
    WKPageSetPageFindClient(pageRef, &findClient);
}

} // namespace WebKit
