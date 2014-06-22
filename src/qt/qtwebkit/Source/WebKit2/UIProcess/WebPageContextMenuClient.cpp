/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#if ENABLE(CONTEXT_MENUS)

#include "WebPageContextMenuClient.h"

#include "Logging.h"
#include "MutableArray.h"
#include "WebContextMenuItem.h"
#include "WKAPICast.h"
#include "WKSharedAPICast.h"

namespace WebKit {

bool WebPageContextMenuClient::getContextMenuFromProposedMenu(WebPageProxy* page, const Vector<WebContextMenuItemData>& proposedMenuVector, Vector<WebContextMenuItemData>& customMenu, const WebHitTestResult::Data& hitTestResultData, APIObject* userData)
{
    if (!m_client.getContextMenuFromProposedMenu && !m_client.getContextMenuFromProposedMenu_deprecatedForUseWithV0)
        return false;

    if (m_client.version >= 2 && !m_client.getContextMenuFromProposedMenu)
        return false;

    unsigned size = proposedMenuVector.size();
    RefPtr<MutableArray> proposedMenu = MutableArray::create();
    proposedMenu->reserveCapacity(size);
    for (unsigned i = 0; i < size; ++i)
        proposedMenu->append(WebContextMenuItem::create(proposedMenuVector[i]).get());
        
    WKArrayRef newMenu = 0;
    if (m_client.version >= 2) {
        RefPtr<WebHitTestResult> webHitTestResult = WebHitTestResult::create(hitTestResultData);
        m_client.getContextMenuFromProposedMenu(toAPI(page), toAPI(proposedMenu.get()), &newMenu, toAPI(webHitTestResult.get()), toAPI(userData), m_client.clientInfo);
    } else
        m_client.getContextMenuFromProposedMenu_deprecatedForUseWithV0(toAPI(page), toAPI(proposedMenu.get()), &newMenu, toAPI(userData), m_client.clientInfo);

    RefPtr<ImmutableArray> array = adoptRef(toImpl(newMenu));
    
    customMenu.clear();
    
    size_t newSize = array ? array->size() : 0;
    for (size_t i = 0; i < newSize; ++i) {
        WebContextMenuItem* item = array->at<WebContextMenuItem>(i);
        if (!item) {
            LOG(ContextMenu, "New menu entry at index %i is not a WebContextMenuItem", (int)i);
            continue;
        }
        
        customMenu.append(*item->data());
    }
    
    return true;
}

void WebPageContextMenuClient::customContextMenuItemSelected(WebPageProxy* page, const WebContextMenuItemData& itemData)
{
    if (!m_client.customContextMenuItemSelected)
        return;

    RefPtr<WebContextMenuItem> item = WebContextMenuItem::create(itemData);
    m_client.customContextMenuItemSelected(toAPI(page), toAPI(item.get()), m_client.clientInfo);
}

void WebPageContextMenuClient::contextMenuDismissed(WebPageProxy* page)
{
    if (!m_client.contextMenuDismissed)
        return;
    
    m_client.contextMenuDismissed(toAPI(page), m_client.clientInfo);
}

bool WebPageContextMenuClient::showContextMenu(WebPageProxy* page, const WebCore::IntPoint& menuLocation, const Vector<WebContextMenuItemData>& menuItemsVector)
{
    if (!m_client.showContextMenu)
        return false;

    unsigned size = menuItemsVector.size();

    Vector<RefPtr<APIObject> > menuItems;
    menuItems.reserveCapacity(size);

    for (unsigned i = 0; i < size; ++i)
        menuItems.uncheckedAppend(WebContextMenuItem::create(menuItemsVector[i]).get());

    m_client.showContextMenu(toAPI(page), toAPI(menuLocation), toAPI(ImmutableArray::adopt(menuItems).get()), m_client.clientInfo);

    return true;
}

bool WebPageContextMenuClient::hideContextMenu(WebPageProxy* page)
{
    if (!m_client.hideContextMenu)
        return false;

    m_client.hideContextMenu(toAPI(page), m_client.clientInfo);

    return true;
}

} // namespace WebKit
#endif // ENABLE(CONTEXT_MENUS)
