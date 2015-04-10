/*
 * Copyright (C) 2013 Samsung Electronics. All rights reserved.
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
 * THE IMPLIED WARRANTIES OF MERCHANTAwBILITY AND FITNESS FOR A PARTICULAR
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
#include "WebUIPopupMenuClient.h"

#include "ImmutableArray.h"
#include "WKAPICast.h"
#include "WebPopupItemEfl.h"
#include "WebPopupMenuListenerEfl.h"

using namespace WebCore;
using namespace WebKit;

void WebUIPopupMenuClient::showPopupMenu(WebPageProxy* pageProxy, WebPopupMenuProxy* popupMenuProxy, const IntRect& rect, TextDirection textDirection, double pageScaleFactor, const Vector<WebPopupItem>& items, int32_t selectedIndex)
{
    if (!m_client.showPopupMenu)
        return;

    Vector<RefPtr<APIObject> > webPopupItems;
    size_t size = items.size();
    for (size_t i = 0; i < size; ++i)
        webPopupItems.append(WebPopupItemEfl::create(items[i]));

    RefPtr<ImmutableArray> ItemsArray;
    if (!webPopupItems.isEmpty())
        ItemsArray = ImmutableArray::adopt(webPopupItems);

    m_client.showPopupMenu(toAPI(pageProxy), toAPI(static_cast<WebPopupMenuListenerEfl*>(popupMenuProxy)), toAPI(rect), toAPI(textDirection), pageScaleFactor, toAPI(ItemsArray.get()), selectedIndex, m_client.clientInfo);
}

void WebUIPopupMenuClient::hidePopupMenu(WebPageProxy* pageProxy)
{
    if (m_client.hidePopupMenu)
        m_client.hidePopupMenu(toAPI(pageProxy), m_client.clientInfo);
}
