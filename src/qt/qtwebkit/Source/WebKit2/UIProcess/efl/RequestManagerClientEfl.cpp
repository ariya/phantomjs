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
#include "RequestManagerClientEfl.h"

#include "WKContextSoup.h"
#include "WKSoupRequestManager.h"
#include "ewk_context_private.h"
#include "ewk_url_scheme_request_private.h"

namespace WebKit {

struct EwkUrlSchemeHandler {
    Ewk_Url_Scheme_Request_Cb callback;
    void* userData;

    EwkUrlSchemeHandler()
        : callback(0)
        , userData(0)
    { }

    EwkUrlSchemeHandler(Ewk_Url_Scheme_Request_Cb callback, void* userData)
        : callback(callback)
        , userData(userData)
    { }
};

static inline RequestManagerClientEfl* toRequestManagerClientEfl(const void* clientInfo)
{
    return static_cast<RequestManagerClientEfl*>(const_cast<void*>(clientInfo));
}

void RequestManagerClientEfl::didReceiveURIRequest(WKSoupRequestManagerRef soupRequestManagerRef, WKURLRef urlRef, WKPageRef, uint64_t requestID, const void* clientInfo)
{
    RequestManagerClientEfl* requestManager = toRequestManagerClientEfl(clientInfo);

    RefPtr<EwkUrlSchemeRequest> schemeRequest = EwkUrlSchemeRequest::create(soupRequestManagerRef, urlRef, requestID);
    EwkUrlSchemeHandler handler = requestManager->m_urlSchemeHandlers.get(schemeRequest->scheme());
    if (!handler.callback)
        return;

    handler.callback(schemeRequest.get(), handler.userData);
}

RequestManagerClientEfl::RequestManagerClientEfl(WKContextRef context)
    : m_soupRequestManager(WKContextGetSoupRequestManager(context))
{
    ASSERT(m_soupRequestManager);

    WKSoupRequestManagerClient wkRequestManagerClient;
    memset(&wkRequestManagerClient, 0, sizeof(WKSoupRequestManagerClient));

    wkRequestManagerClient.version = kWKSoupRequestManagerClientCurrentVersion;
    wkRequestManagerClient.clientInfo = this;
    wkRequestManagerClient.didReceiveURIRequest = didReceiveURIRequest;

    WKSoupRequestManagerSetClient(m_soupRequestManager.get(), &wkRequestManagerClient);
}

RequestManagerClientEfl::~RequestManagerClientEfl()
{
}

void RequestManagerClientEfl::registerURLSchemeHandler(const String& scheme, Ewk_Url_Scheme_Request_Cb callback, void* userData)
{
    ASSERT(callback);

    m_urlSchemeHandlers.set(scheme, EwkUrlSchemeHandler(callback, userData));
    WKSoupRequestManagerRegisterURIScheme(m_soupRequestManager.get(), adoptWK(toCopiedAPI(scheme)).get());
}

} // namespace WebKit
