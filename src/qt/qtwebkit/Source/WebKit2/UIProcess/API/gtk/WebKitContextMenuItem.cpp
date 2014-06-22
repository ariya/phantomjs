/*
 * Copyright (C) 2012 Igalia S.L.
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
 */

#include "config.h"
#include "WebKitContextMenuItem.h"

#include "MutableArray.h"
#include "WebContextMenuItem.h"
#include "WebContextMenuItemData.h"
#include "WebKitContextMenuActionsPrivate.h"
#include "WebKitContextMenuItemPrivate.h"
#include "WebKitContextMenuPrivate.h"
#include <WebCore/ContextMenu.h>
#include <WebCore/ContextMenuItem.h>
#include <gtk/gtk.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/gobject/GOwnPtr.h>
#include <wtf/gobject/GRefPtr.h>

using namespace WebKit;
using namespace WebCore;

/**
 * SECTION: WebKitContextMenuItem
 * @Short_description: One item of the #WebKitContextMenu
 * @Title: WebKitContextMenuItem
 *
 * The #WebKitContextMenu is composed of #WebKitContextMenuItem<!--
 * -->s. These items can be created from a #GtkAction, from a
 * #WebKitContextMenuAction or from a #WebKitContextMenuAction and a
 * label. These #WebKitContextMenuAction<!-- -->s denote stock actions
 * for the items. You can also create separators and submenus.
 *
 */

struct _WebKitContextMenuItemPrivate {
    ~_WebKitContextMenuItemPrivate()
    {
        if (subMenu)
            webkitContextMenuSetParentItem(subMenu.get(), 0);
    }

    OwnPtr<ContextMenuItem> menuItem;
    GRefPtr<WebKitContextMenu> subMenu;
};

WEBKIT_DEFINE_TYPE(WebKitContextMenuItem, webkit_context_menu_item, G_TYPE_INITIALLY_UNOWNED)

static void webkit_context_menu_item_class_init(WebKitContextMenuItemClass* itemClass)
{
}

static bool checkAndWarnIfMenuHasParentItem(WebKitContextMenu* menu)
{
    if (menu && webkitContextMenuGetParentItem(menu)) {
        g_warning("Attempting to set a WebKitContextMenu as submenu of "
                  "a WebKitContextMenuItem, but the menu is already "
                  "a submenu of a WebKitContextMenuItem");
        return true;
    }

    return false;
}

static void webkitContextMenuItemSetSubMenu(WebKitContextMenuItem* item, GRefPtr<WebKitContextMenu> subMenu)
{
    if (checkAndWarnIfMenuHasParentItem(subMenu.get()))
        return;

    if (item->priv->subMenu)
        webkitContextMenuSetParentItem(item->priv->subMenu.get(), 0);
    item->priv->subMenu = subMenu;
    if (subMenu)
        webkitContextMenuSetParentItem(subMenu.get(), item);
}

WebKitContextMenuItem* webkitContextMenuItemCreate(WebContextMenuItem* webItem)
{
    WebKitContextMenuItem* item = WEBKIT_CONTEXT_MENU_ITEM(g_object_new(WEBKIT_TYPE_CONTEXT_MENU_ITEM, NULL));
    WebContextMenuItemData* itemData = webItem->data();
    item->priv->menuItem = WTF::adoptPtr(new ContextMenuItem(itemData->type(), itemData->action(), itemData->title(), itemData->enabled(), itemData->checked()));
    const Vector<WebContextMenuItemData>& subMenu = itemData->submenu();
    if (!subMenu.size())
        return item;

    RefPtr<MutableArray> subMenuItems = MutableArray::create();
    subMenuItems->reserveCapacity(subMenu.size());
    for (size_t i = 0; i < subMenu.size(); ++i)
        subMenuItems->append(WebContextMenuItem::create(subMenu[i]).get());
    webkitContextMenuItemSetSubMenu(item, adoptGRef(webkitContextMenuCreate(subMenuItems.get())));

    return item;
}

static WebKitContextMenuItem* webkitContextMenuItemCreateForGtkItem(GtkMenuItem* menuItem)
{
    WebKitContextMenuItem* item = WEBKIT_CONTEXT_MENU_ITEM(g_object_new(WEBKIT_TYPE_CONTEXT_MENU_ITEM, NULL));
    item->priv->menuItem = WTF::adoptPtr(new ContextMenuItem(menuItem));
    webkitContextMenuItemSetSubMenuFromGtkMenu(item, GTK_MENU(gtk_menu_item_get_submenu(menuItem)));

    return item;
}

void webkitContextMenuItemSetSubMenuFromGtkMenu(WebKitContextMenuItem* item, GtkMenu* subMenu)
{
    if (!subMenu)
        return;

    GOwnPtr<GList> children(gtk_container_get_children(GTK_CONTAINER(subMenu)));
    if (!g_list_length(children.get()))
        return;

    webkitContextMenuItemSetSubMenu(item, adoptGRef(webkit_context_menu_new()));
    for (GList* listItem = children.get(); listItem; listItem = g_list_next(listItem)) {
        GRefPtr<GtkWidget> widget = GTK_WIDGET(listItem->data);
        if (!GTK_IS_MENU_ITEM(widget.get()))
            continue;

        gtk_container_remove(GTK_CONTAINER(subMenu), widget.get());
        GtkMenuItem* menuItem = GTK_MENU_ITEM(widget.leakRef());
        g_object_force_floating(G_OBJECT(menuItem));
        webkit_context_menu_append(item->priv->subMenu.get(), webkitContextMenuItemCreateForGtkItem(menuItem));
    }
}

GtkMenuItem* webkitContextMenuItemRelease(WebKitContextMenuItem* item)
{
    if (item->priv->subMenu) {
        Vector<ContextMenuItem> subMenuItems;
        webkitContextMenuPopulate(item->priv->subMenu.get(), subMenuItems);
        ContextMenu subMenu(platformMenuDescription(subMenuItems));
        item->priv->menuItem->setSubMenu(&subMenu);
    }

    return item->priv->menuItem->releasePlatformDescription();
}

/**
 * webkit_context_menu_item_new:
 * @action: a #GtkAction
 *
 * Creates a new #WebKitContextMenuItem for the given @action.
 *
 * Returns: the newly created #WebKitContextMenuItem object.
 */
WebKitContextMenuItem* webkit_context_menu_item_new(GtkAction* action)
{
    g_return_val_if_fail(GTK_IS_ACTION(action), 0);

    WebKitContextMenuItem* item = WEBKIT_CONTEXT_MENU_ITEM(g_object_new(WEBKIT_TYPE_CONTEXT_MENU_ITEM, NULL));
    item->priv->menuItem = WTF::adoptPtr(new ContextMenuItem(GTK_MENU_ITEM(gtk_action_create_menu_item(action))));
    item->priv->menuItem->setAction(ContextMenuItemBaseApplicationTag);

    return item;
}

/**
 * webkit_context_menu_item_new_from_stock_action:
 * @action: a #WebKitContextMenuAction stock action
 *
 * Creates a new #WebKitContextMenuItem for the given stock action.
 * Stock actions are handled automatically by WebKit so that, for example,
 * when a menu item created with a %WEBKIT_CONTEXT_MENU_ACTION_STOP is
 * activated the action associated will be handled by WebKit and the current
 * load operation will be stopped. You can get the #GtkAction of a
 * #WebKitContextMenuItem created with a #WebKitContextMenuAction with
 * webkit_context_menu_item_get_action() and connect to #GtkAction::activate signal
 * to be notified when the item is activated. But you can't prevent the asociated
 * action from being performed.
 *
 * Returns: the newly created #WebKitContextMenuItem object.
 */
WebKitContextMenuItem* webkit_context_menu_item_new_from_stock_action(WebKitContextMenuAction action)
{
    g_return_val_if_fail(action > WEBKIT_CONTEXT_MENU_ACTION_NO_ACTION && action < WEBKIT_CONTEXT_MENU_ACTION_CUSTOM, 0);

    WebKitContextMenuItem* item = WEBKIT_CONTEXT_MENU_ITEM(g_object_new(WEBKIT_TYPE_CONTEXT_MENU_ITEM, NULL));
    ContextMenuItemType type = webkitContextMenuActionIsCheckable(action) ? CheckableActionType : ActionType;
    item->priv->menuItem = WTF::adoptPtr(new ContextMenuItem(type, webkitContextMenuActionGetActionTag(action), webkitContextMenuActionGetLabel(action)));

    return item;
}

/**
 * webkit_context_menu_item_new_from_stock_action_with_label:
 * @action: a #WebKitContextMenuAction stock action
 * @label: a custom label text to use instead of the predefined one
 *
 * Creates a new #WebKitContextMenuItem for the given stock action using the given @label.
 * Stock actions have a predefined label, this method can be used to create a
 * #WebKitContextMenuItem for a #WebKitContextMenuAction but using a custom label.
 *
 * Returns: the newly created #WebKitContextMenuItem object.
 */
WebKitContextMenuItem* webkit_context_menu_item_new_from_stock_action_with_label(WebKitContextMenuAction action, const gchar* label)
{
    g_return_val_if_fail(action > WEBKIT_CONTEXT_MENU_ACTION_NO_ACTION && action < WEBKIT_CONTEXT_MENU_ACTION_CUSTOM, 0);

    WebKitContextMenuItem* item = WEBKIT_CONTEXT_MENU_ITEM(g_object_new(WEBKIT_TYPE_CONTEXT_MENU_ITEM, NULL));
    ContextMenuItemType type = webkitContextMenuActionIsCheckable(action) ? CheckableActionType : ActionType;
    item->priv->menuItem = WTF::adoptPtr(new ContextMenuItem(type, webkitContextMenuActionGetActionTag(action), String::fromUTF8(label)));

    return item;
}

/**
 * webkit_context_menu_item_new_with_submenu:
 * @label: the menu item label text
 * @submenu: a #WebKitContextMenu to set
 *
 * Creates a new #WebKitContextMenuItem using the given @label with a submenu.
 *
 * Returns: the newly created #WebKitContextMenuItem object.
 */
WebKitContextMenuItem* webkit_context_menu_item_new_with_submenu(const gchar* label, WebKitContextMenu* submenu)
{
    g_return_val_if_fail(label, 0);
    g_return_val_if_fail(WEBKIT_IS_CONTEXT_MENU(submenu), 0);

    if (checkAndWarnIfMenuHasParentItem(submenu))
        return 0;

    WebKitContextMenuItem* item = WEBKIT_CONTEXT_MENU_ITEM(g_object_new(WEBKIT_TYPE_CONTEXT_MENU_ITEM, NULL));
    item->priv->menuItem = WTF::adoptPtr(new ContextMenuItem(SubmenuType, ContextMenuItemBaseApplicationTag, String::fromUTF8(label)));
    item->priv->subMenu = submenu;
    webkitContextMenuSetParentItem(submenu, item);

    return item;
}

/**
 * webkit_context_menu_item_new_separator:
 *
 * Creates a new #WebKitContextMenuItem representing a separator.
 *
 * Returns: the newly created #WebKitContextMenuItem object.
 */
WebKitContextMenuItem* webkit_context_menu_item_new_separator(void)
{
    WebKitContextMenuItem* item = WEBKIT_CONTEXT_MENU_ITEM(g_object_new(WEBKIT_TYPE_CONTEXT_MENU_ITEM, NULL));
    item->priv->menuItem = WTF::adoptPtr(new ContextMenuItem(SeparatorType, ContextMenuItemTagNoAction, String()));

    return item;
}

/**
 * webkit_context_menu_item_get_action:
 * @item: a #WebKitContextMenuItem
 *
 * Gets the action associated to @item.
 *
 * Returns: (transfer none): the #GtkAction associated to the #WebKitContextMenuItem,
 *    or %NULL if @item is a separator.
 */
GtkAction* webkit_context_menu_item_get_action(WebKitContextMenuItem* item)
{
    g_return_val_if_fail(WEBKIT_IS_CONTEXT_MENU_ITEM(item), 0);

    return item->priv->menuItem->gtkAction();
}

/**
 * webkit_context_menu_item_get_stock_action:
 * @item: a #WebKitContextMenuItem
 *
 * Gets the #WebKitContextMenuAction of @item. If the #WebKitContextMenuItem was not
 * created for a stock action %WEBKIT_CONTEXT_MENU_ACTION_CUSTOM will be
 * returned. If the #WebKitContextMenuItem is a separator %WEBKIT_CONTEXT_MENU_ACTION_NO_ACTION
 * will be returned.
 *
 * Returns: the #WebKitContextMenuAction of @item
 */
WebKitContextMenuAction webkit_context_menu_item_get_stock_action(WebKitContextMenuItem* item)
{
    g_return_val_if_fail(WEBKIT_IS_CONTEXT_MENU_ITEM(item), WEBKIT_CONTEXT_MENU_ACTION_NO_ACTION);

    return webkitContextMenuActionGetForContextMenuItem(item->priv->menuItem.get());
}

/**
 * webkit_context_menu_item_is_separator:
 * @item: a #WebKitContextMenuItem
 *
 * Checks whether @item is a separator.
 *
 * Returns: %TRUE is @item is a separator or %FALSE otherwise
 */
gboolean webkit_context_menu_item_is_separator(WebKitContextMenuItem* item)
{
    g_return_val_if_fail(WEBKIT_IS_CONTEXT_MENU_ITEM(item), FALSE);

    return item->priv->menuItem->type() == SeparatorType;
}

/**
 * webkit_context_menu_item_set_submenu:
 * @item: a #WebKitContextMenuItem
 * @submenu: (allow-none): a #WebKitContextMenu
 *
 * Sets or replaces the @item submenu. If @submenu is %NULL the current
 * submenu of @item is removed.
 */
void webkit_context_menu_item_set_submenu(WebKitContextMenuItem* item, WebKitContextMenu* submenu)
{
    g_return_if_fail(WEBKIT_IS_CONTEXT_MENU_ITEM(item));

    if (item->priv->subMenu == submenu)
        return;

    webkitContextMenuItemSetSubMenu(item, submenu);
}

/**
 * webkit_context_menu_item_get_submenu:
 * @item: a #WebKitContextMenuItem
 *
 * Gets the submenu of @item.
 *
 * Returns: (transfer none): the #WebKitContextMenu representing the submenu of
 *    @item or %NULL if @item doesn't have a submenu.
 */
WebKitContextMenu* webkit_context_menu_item_get_submenu(WebKitContextMenuItem* item)
{
    g_return_val_if_fail(WEBKIT_IS_CONTEXT_MENU_ITEM(item), 0);

    return item->priv->subMenu.get();
}

