/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 * Copyright (C) 2010, 2011 Apple Inc. All rights reserved.
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
#include "WebSearchPopupMenu.h"

#include "WebPage.h"
#include "WebPageProxyMessages.h"
#include "WebProcess.h"
#include <wtf/text/AtomicString.h>

using namespace WebCore;

namespace WebKit {

PassRefPtr<WebSearchPopupMenu> WebSearchPopupMenu::create(WebPage* page, PopupMenuClient* client)
{
    return adoptRef(new WebSearchPopupMenu(page, client));
}

WebSearchPopupMenu::WebSearchPopupMenu(WebPage* page, PopupMenuClient* client)
    : m_popup(WebPopupMenu::create(page, client))
{
}

PopupMenu* WebSearchPopupMenu::popupMenu()
{
    return m_popup.get();
}

void WebSearchPopupMenu::saveRecentSearches(const AtomicString& name, const Vector<String>& searchItems)
{
    if (name.isEmpty())
        return;

    WebPage* page = m_popup->page();
    if (!page)
        return;

    WebProcess::shared().parentProcessConnection()->send(Messages::WebPageProxy::SaveRecentSearches(name, searchItems), page->pageID());
}

void WebSearchPopupMenu::loadRecentSearches(const AtomicString& name, Vector<String>& resultItems)
{
    if (name.isEmpty())
        return;

    WebPage* page = m_popup->page();
    if (!page)
        return;

    WebProcess::shared().parentProcessConnection()->sendSync(Messages::WebPageProxy::LoadRecentSearches(name), Messages::WebPageProxy::LoadRecentSearches::Reply(resultItems), page->pageID());
}

bool WebSearchPopupMenu::enabled()
{
    return true;
}

} // namespace WebKit
