/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "WebPopupMenu.h"

#include "PlatformPopupMenuData.h"
#include "WebCoreArgumentCoders.h"
#include "WebPage.h"
#include "WebPageProxyMessages.h"
#include "WebProcess.h"
#include <WebCore/FrameView.h>
#include <WebCore/PopupMenuClient.h>

using namespace WebCore;

namespace WebKit {

PassRefPtr<WebPopupMenu> WebPopupMenu::create(WebPage* page, PopupMenuClient* client)
{
    return adoptRef(new WebPopupMenu(page, client));
}

WebPopupMenu::WebPopupMenu(WebPage* page, PopupMenuClient* client)
    : m_popupClient(client)
    , m_page(page)
{
}

WebPopupMenu::~WebPopupMenu()
{
}

void WebPopupMenu::disconnectClient()
{
    m_popupClient = 0;
}

void WebPopupMenu::didChangeSelectedIndex(int newIndex)
{
    if (!m_popupClient)
        return;

#if PLATFORM(QT)
    if (newIndex >= 0)
        m_popupClient->listBoxSelectItem(newIndex, m_popupClient->multiple(), false);
#else
    m_popupClient->popupDidHide();
    if (newIndex >= 0)
        m_popupClient->valueChanged(newIndex);
#endif
}

void WebPopupMenu::setTextForIndex(int index)
{
    if (!m_popupClient)
        return;

    m_popupClient->setTextFromItem(index);
}

Vector<WebPopupItem> WebPopupMenu::populateItems()
{
    size_t size = m_popupClient->listSize();

    Vector<WebPopupItem> items;
    items.reserveInitialCapacity(size);
    
    for (size_t i = 0; i < size; ++i) {
        if (m_popupClient->itemIsSeparator(i))
            items.append(WebPopupItem(WebPopupItem::Separator));
        else {
            // FIXME: Add support for styling the font.
            // FIXME: Add support for styling the foreground and background colors.
            // FIXME: Find a way to customize text color when an item is highlighted.
            PopupMenuStyle itemStyle = m_popupClient->itemStyle(i);
            items.append(WebPopupItem(WebPopupItem::Item, m_popupClient->itemText(i), itemStyle.textDirection(), itemStyle.hasTextDirectionOverride(), m_popupClient->itemToolTip(i), m_popupClient->itemAccessibilityText(i), m_popupClient->itemIsEnabled(i), m_popupClient->itemIsLabel(i), m_popupClient->itemIsSelected(i)));
        }
    }

    return items;
}

void WebPopupMenu::show(const IntRect& rect, FrameView* view, int index)
{
    // FIXME: We should probably inform the client to also close the menu.
    Vector<WebPopupItem> items = populateItems();

    if (items.isEmpty() || !m_page) {
        m_popupClient->popupDidHide();
        return;
    }

    m_page->setActivePopupMenu(this);

    // Move to page coordinates
    IntRect pageCoordinates(view->contentsToWindow(rect.location()), rect.size());

    PlatformPopupMenuData platformData;
    setUpPlatformData(pageCoordinates, platformData);

    WebProcess::shared().parentProcessConnection()->send(Messages::WebPageProxy::ShowPopupMenu(pageCoordinates, m_popupClient->menuStyle().textDirection(), items, index, platformData), m_page->pageID());
}

void WebPopupMenu::hide()
{
    if (!m_page || !m_popupClient)
        return;

    WebProcess::shared().parentProcessConnection()->send(Messages::WebPageProxy::HidePopupMenu(), m_page->pageID());
    m_page->setActivePopupMenu(0);
}

void WebPopupMenu::updateFromElement()
{
}

} // namespace WebKit
