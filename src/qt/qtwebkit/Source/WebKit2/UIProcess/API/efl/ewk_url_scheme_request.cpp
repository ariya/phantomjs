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
#include "ewk_url_scheme_request.h"

#include "WKData.h"
#include "WKString.h"
#include "WKURL.h"
#include "ewk_url_scheme_request_private.h"

using namespace WebKit;

EwkUrlSchemeRequest::EwkUrlSchemeRequest(WKSoupRequestManagerRef manager, WKURLRef url, uint64_t requestID)
    : m_wkRequestManager(manager)
    , m_url(url)
    , m_requestID(requestID)
    , m_scheme(AdoptWK, WKURLCopyScheme(url))
    , m_path(AdoptWK, WKURLCopyPath(url))
{
}

uint64_t EwkUrlSchemeRequest::id() const
{
    return m_requestID;
}

const char* EwkUrlSchemeRequest::url() const
{
    return m_url;
}

const char* EwkUrlSchemeRequest::scheme() const
{
    return m_scheme;
}

const char* EwkUrlSchemeRequest::path() const
{
    return m_path;
}

void EwkUrlSchemeRequest::finish(const void* contentData, uint64_t contentLength, const char* mimeType)
{
    WKRetainPtr<WKDataRef> wkData(AdoptWK, WKDataCreate(contentLength ? reinterpret_cast<const unsigned char*>(contentData) : 0, contentLength));
    WKRetainPtr<WKStringRef> wkMimeType = mimeType ? adoptWK(WKStringCreateWithUTF8CString(mimeType)) : 0;

    // In case of empty reply an empty WKDataRef is sent to the WebProcess.
    WKSoupRequestManagerDidHandleURIRequest(m_wkRequestManager.get(), wkData.get(), contentLength, wkMimeType.get(), m_requestID);
}

const char* ewk_url_scheme_request_scheme_get(const Ewk_Url_Scheme_Request* request)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkUrlSchemeRequest, request, impl, 0);

    return impl->scheme();
}

const char* ewk_url_scheme_request_url_get(const Ewk_Url_Scheme_Request* request)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkUrlSchemeRequest, request, impl, 0);

    return impl->url();
}

const char* ewk_url_scheme_request_path_get(const Ewk_Url_Scheme_Request* request)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkUrlSchemeRequest, request, impl, 0);

    return impl->path();
}

Eina_Bool ewk_url_scheme_request_finish(Ewk_Url_Scheme_Request* request, const void* contentData, uint64_t contentLength, const char* mimeType)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(EwkUrlSchemeRequest, request, impl, false);

    impl->finish(contentData, contentLength, mimeType);

    return true;
}
