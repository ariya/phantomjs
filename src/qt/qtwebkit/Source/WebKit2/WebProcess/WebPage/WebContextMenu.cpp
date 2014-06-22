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

#if ENABLE(CONTEXT_MENUS)

#include "WebContextMenu.h"

#include "InjectedBundleHitTestResult.h"
#include "InjectedBundleUserMessageCoders.h"
#include "WebCoreArgumentCoders.h"
#include "WebHitTestResult.h"
#include "WebPage.h"
#include "WebPageProxyMessages.h"
#include <WebCore/ContextMenu.h>
#include <WebCore/ContextMenuController.h>
#include <WebCore/Frame.h>
#include <WebCore/FrameView.h>
#include <WebCore/Page.h>

using namespace WebCore;

namespace WebKit {

WebContextMenu::WebContextMenu(WebPage* page)
    : m_page(page)
{
}

WebContextMenu::~WebContextMenu()
{
}

void WebContextMenu::show()
{
    ContextMenuController* controller = m_page->corePage()->contextMenuController();
    if (!controller)
        return;
    ContextMenu* menu = controller->contextMenu();
    if (!menu)
        return;
    Frame* frame = controller->hitTestResult().innerNodeFrame();
    if (!frame)
        return;
    FrameView* view = frame->view();
    if (!view)
        return;

    Vector<WebContextMenuItemData> menuItems;
    RefPtr<APIObject> userData;
    menuItemsWithUserData(menuItems, userData);
    WebHitTestResult::Data webHitTestResultData(controller->hitTestResult());

    // Mark the WebPage has having a shown context menu then notify the UIProcess.
    m_page->contextMenuShowing();
    m_page->send(Messages::WebPageProxy::ShowContextMenu(view->contentsToWindow(controller->hitTestResult().roundedPointInInnerNodeFrame()), webHitTestResultData, menuItems, InjectedBundleUserMessageEncoder(userData.get())));
}

void WebContextMenu::itemSelected(const WebContextMenuItemData& item)
{
    ContextMenuItem coreItem(ActionType, static_cast<ContextMenuAction>(item.action()), item.title());
    m_page->corePage()->contextMenuController()->contextMenuItemSelected(&coreItem);
}

void WebContextMenu::menuItemsWithUserData(Vector<WebContextMenuItemData> &menuItems, RefPtr<APIObject>& userData) const
{
    ContextMenuController* controller = m_page->corePage()->contextMenuController();
    if (!controller)
        return;

    ContextMenu* menu = controller->contextMenu();
    if (!menu)
        return;

    // Give the bundle client a chance to process the menu.
#if USE(CROSS_PLATFORM_CONTEXT_MENUS)
    const Vector<ContextMenuItem>& coreItems = menu->items();
#else
    Vector<ContextMenuItem> coreItems = contextMenuItemVector(menu->platformDescription());
#endif
    Vector<WebContextMenuItemData> proposedMenu = kitItems(coreItems, menu);
    Vector<WebContextMenuItemData> newMenu;
    RefPtr<InjectedBundleHitTestResult> hitTestResult = InjectedBundleHitTestResult::create(controller->hitTestResult());
    if (m_page->injectedBundleContextMenuClient().getCustomMenuFromDefaultItems(m_page, hitTestResult.get(), proposedMenu, newMenu, userData))
        proposedMenu = newMenu;
    menuItems = proposedMenu;
}

Vector<WebContextMenuItemData> WebContextMenu::items() const
{
    Vector<WebContextMenuItemData> menuItems;
    RefPtr<APIObject> userData;
    menuItemsWithUserData(menuItems, userData);
    return menuItems;
}

} // namespace WebKit

#endif // ENABLE(CONTEXT_MENUS)
