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
#include "ewk_url_request.h"

#include "ewk_url_request_private.h"

using namespace WebKit;

EwkUrlRequest::EwkUrlRequest(WKURLRequestRef requestRef)
    : m_url(AdoptWK, WKURLRequestCopyURL(requestRef))
    , m_firstParty(AdoptWK, WKURLRequestCopyFirstPartyForCookies(requestRef))
    , m_httpMethod(AdoptWK, WKURLRequestCopyHTTPMethod(requestRef))
{ }

const char* EwkUrlRequest::url() const
{
    return m_url;
}

const char* EwkUrlRequest::firstParty() const
{
    return m_firstParty;
}

const char* EwkUrlRequest::httpMethod() const
{
    return m_httpMethod;
}

const char* ewk_url_request_url_get(const Ewk_Url_Request* request)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkUrlRequest, request, impl, 0);

    return impl->url();
}

const char* ewk_request_cookies_first_party_get(const Ewk_Url_Request* request)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkUrlRequest, request, impl, 0);

    return impl->firstParty();
}

const char* ewk_url_request_http_method_get(const Ewk_Url_Request* request)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkUrlRequest, request, impl, 0);

    return impl->httpMethod();
}
