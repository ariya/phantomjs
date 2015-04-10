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
#include "ewk_navigation_data.h"

#include "ewk_navigation_data_private.h"

using namespace WebKit;

EwkNavigationData::EwkNavigationData(WKNavigationDataRef dataRef)
    : m_request(EwkUrlRequest::create(adoptWK(WKNavigationDataCopyOriginalRequest(dataRef)).get()))
    , m_title(AdoptWK, WKNavigationDataCopyTitle(dataRef))
    , m_url(AdoptWK, WKNavigationDataCopyURL(dataRef))
{ }

EwkUrlRequest* EwkNavigationData::originalRequest() const
{
    return m_request.get();
}

const char* EwkNavigationData::title() const
{
    return m_title;
}

const char* EwkNavigationData::url() const
{
    return m_url;
}

const char* ewk_navigation_data_title_get(const Ewk_Navigation_Data* data)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkNavigationData, data, impl, 0);

    return impl->title();
}

Ewk_Url_Request* ewk_navigation_data_original_request_get(const Ewk_Navigation_Data* data)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkNavigationData, data, impl, 0);

    return impl->originalRequest();
}

const char* ewk_navigation_data_url_get(const Ewk_Navigation_Data* data)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkNavigationData, data, impl, 0);

    return impl->url();
}
