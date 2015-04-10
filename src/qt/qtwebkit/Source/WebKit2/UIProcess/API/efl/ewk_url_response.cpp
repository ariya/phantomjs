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
#include "ewk_url_response.h"

#include "ewk_url_response_private.h"
#include <wtf/text/CString.h>

using namespace WebKit;

EwkUrlResponse::EwkUrlResponse(WKURLResponseRef response)
    : m_response(response)
    , m_url(AdoptWK, WKURLResponseCopyURL(response))
    , m_mimeType(AdoptWK, WKURLResponseCopyMIMEType(response))
{ }

int EwkUrlResponse::httpStatusCode() const
{
    return WKURLResponseHTTPStatusCode(m_response.get());
}

const char* EwkUrlResponse::url() const
{
    return m_url;
}

const char* EwkUrlResponse::mimeType() const
{
    return m_mimeType;
}

unsigned long EwkUrlResponse::contentLength() const
{
    return WKURLResponseGetExpectedContentLength(m_response.get());
}

const char* ewk_url_response_url_get(const Ewk_Url_Response* response)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkUrlResponse, response, impl, 0);

    return impl->url();
}

int ewk_url_response_status_code_get(const Ewk_Url_Response* response)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkUrlResponse, response, impl, 0);

    return impl->httpStatusCode();
}

const char* ewk_url_response_mime_type_get(const Ewk_Url_Response* response)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkUrlResponse, response, impl, 0);

    return impl->mimeType();
}

unsigned long ewk_url_response_content_length_get(const Ewk_Url_Response* response)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkUrlResponse, response, impl, 0);

    return impl->contentLength();
}
