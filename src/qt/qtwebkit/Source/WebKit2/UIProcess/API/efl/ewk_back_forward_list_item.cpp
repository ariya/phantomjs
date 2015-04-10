/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this item of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this item of conditions and the following disclaimer in the
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
#include "ewk_back_forward_list_item.h"

#include "WKAPICast.h"
#include "WKBackForwardListItem.h"
#include "ewk_back_forward_list_item_private.h"

using namespace WebKit;

EwkBackForwardListItem::EwkBackForwardListItem(WKBackForwardListItemRef itemRef)
    : m_wkItem(itemRef)
{ }

const char* EwkBackForwardListItem::url() const
{
    m_url = WKEinaSharedString(AdoptWK, WKBackForwardListItemCopyURL(m_wkItem.get()));

    return m_url;
}

const char* EwkBackForwardListItem::title() const
{
    m_title = WKEinaSharedString(AdoptWK, WKBackForwardListItemCopyTitle(m_wkItem.get()));

    return m_title;
}

const char* EwkBackForwardListItem::originalURL() const
{
    m_originalURL = WKEinaSharedString(AdoptWK, WKBackForwardListItemCopyOriginalURL(m_wkItem.get()));

    return m_originalURL;
}

const char* ewk_back_forward_list_item_url_get(const Ewk_Back_Forward_List_Item* item)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkBackForwardListItem, item, impl, 0);

    return impl->url();
}

const char* ewk_back_forward_list_item_title_get(const Ewk_Back_Forward_List_Item* item)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkBackForwardListItem, item, impl, 0);

    return impl->title();
}

const char* ewk_back_forward_list_item_original_url_get(const Ewk_Back_Forward_List_Item* item)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkBackForwardListItem, item, impl, 0);

    return impl->originalURL();
}
