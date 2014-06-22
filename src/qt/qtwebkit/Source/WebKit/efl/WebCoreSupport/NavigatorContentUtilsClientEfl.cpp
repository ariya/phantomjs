/*
 * Copyright (C) 2012 Samsung Electronics
 *
 * All rights reserved.
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
#include "NavigatorContentUtilsClientEfl.h"

#if ENABLE(NAVIGATOR_CONTENT_UTILS)

#include "ewk_custom_handler_private.h"
#include <wtf/text/CString.h>

namespace WebCore {

static Ewk_Custom_Handler_Data* customHandlerDataCreate(Evas_Object* ewkView, const char* scheme, const char* baseURL, const char* url)
{
    Ewk_Custom_Handler_Data* data = new Ewk_Custom_Handler_Data;
    data->ewkView = ewkView;
    data->scheme = eina_stringshare_add(scheme);
    data->base_url = eina_stringshare_add(baseURL);
    data->url = eina_stringshare_add(url);
    return data;
}

static void customHandlerDataDelete(Ewk_Custom_Handler_Data* data)
{
    eina_stringshare_del(data->scheme);
    eina_stringshare_del(data->base_url);
    eina_stringshare_del(data->url);
    delete data;
}

PassOwnPtr<NavigatorContentUtilsClientEfl> NavigatorContentUtilsClientEfl::create(Evas_Object* view)
{
    return adoptPtr(new NavigatorContentUtilsClientEfl(view));
}

NavigatorContentUtilsClientEfl::NavigatorContentUtilsClientEfl(Evas_Object* view)
    : m_view(view)
{
}

void NavigatorContentUtilsClientEfl::registerProtocolHandler(const String& scheme, const String& baseURL, const String& url, const String& title)
{
    Ewk_Custom_Handler_Data* data = customHandlerDataCreate(m_view, scheme.utf8().data(), baseURL.utf8().data(), url.utf8().data());
    data->title = eina_stringshare_add(title.utf8().data());
    ewk_custom_handler_register_protocol_handler(data);
    eina_stringshare_del(data->title);
    customHandlerDataDelete(data);
}

#if ENABLE(CUSTOM_SCHEME_HANDLER)
NavigatorContentUtilsClient::CustomHandlersState NavigatorContentUtilsClientEfl::isProtocolHandlerRegistered(const String& scheme, const String& baseURL, const String& url)
{
    Ewk_Custom_Handler_Data* data = customHandlerDataCreate(m_view, scheme.utf8().data(), baseURL.utf8().data(), url.utf8().data());
    NavigatorContentUtilsClient::CustomHandlersState result = static_cast<CustomHandlersState>(ewk_custom_handler_register_protocol_handler(data));
    customHandlerDataDelete(data);

    return result;
}

void NavigatorContentUtilsClientEfl::unregisterProtocolHandler(const String& scheme, const String& baseURL, const String& url)
{
    Ewk_Custom_Handler_Data* data = customHandlerDataCreate(m_view, scheme.utf8().data(), baseURL.utf8().data(), url.utf8().data());
    ewk_custom_handler_register_protocol_handler(data);
    customHandlerDataDelete(data);
}
#endif

}

#endif // ENABLE(NAVIGATOR_CONTENT_UTILS)
