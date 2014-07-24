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
#include "WebKitContextMenu.h"

#include "ImmutableArray.h"
#include "WebContextMenuItem.h"
#include "WebKitContextMenuItemPrivate.h"
#include "WebKitContextMenuPrivate.h"

using namespace WebKit;
using namespace WebCore;

/**
 * SECTION: WebKitContextMenu
 * @Short_description: Represents the context menu in a #WebKitWebView
 * @Title: WebKitContextMenu
 *
 * #WebKitContextMenu represents a context menu containing
 * #WebKitContextMenuItem<!-- -->s in a #WebKitWebView.
 *
 * When a #WebKitWebView is about to display the context menu, it
 * emits the #WebKitWebView::context-menu signal, which has the
 * #WebKitContextMenu as an argument. You can modify it, adding new
 * submenus that you can create with webkit_context_menu_new(), adding
 * new #WebKitContextMenuItem<!-- -->s with
 * webkit_context_menu_prepend(), webkit_context_menu_append() or
 * webkit_context_menu_insert(), maybe after having removed the
 * existing ones with webkit_context_menu_remove_all().
 *
 */

struct _WebKitContextMenuPrivate {
    GList* items;
    WebKitContextMenuItem* parentItem;
};

WEBKIT_DEFINE_TYPE(WebKitContextMenu, webkit_context_menu, G_TYPE_OBJECT)

static void webkitContextMenuDispose(GObject* object)
{
    webkit_context_menu_remove_all(WEBKIT_CONTEXT_MENU(object));
    G_OBJECT_CLASS(webkit_context_menu_parent_class)->dispose(object);
}

static void webkit_context_menu_class_init(WebKitContextMenuClass* listClass)
{
    GObjectClass* gObjectClass = G_OBJECT_CLASS(listClass);
    gObjectClass->dispose = webkitContextMenuDispose;
}

void webkitContextMenuPopulate(WebKitContextMenu* menu, Vector<ContextMenuItem>& contextMenuItems)
{
    for (GList* item = menu->priv->items; item; item = g_list_next(item)) {
        WebKitContextMenuItem* menuItem = WEBKIT_CONTEXT_MENU_ITEM(item->data);
        contextMenuItems.append(ContextMenuItem(webkitContextMenuItemRelease(menuItem)));
    }
}

WebKitContextMenu* webkitContextMenuCreate(ImmutableArray* items)
{
    WebKitContextMenu* menu = webkit_context_menu_new();
    for (size_t i = 0; i < items->size(); ++i) {
        WebContextMenuItem* item = static_cast<WebContextMenuItem*>(items->at(i));
        webkit_context_menu_prepend(menu, webkitContextMenuItemCreate(item));
    }
    menu->priv->items = g_list_reverse(menu->priv->items);

    return menu;
}

void webkitContextMenuSetParentItem(WebKitContextMenu* menu, WebKitContextMenuItem* item)
{
    menu->priv->parentItem = item;
}

WebKitContextMenuItem* webkitContextMenuGetParentItem(WebKitContextMenu* menu)
{
    return menu->priv->parentItem;
}

/**
 * webkit_context_menu_new:
 *
 * Creates a new #WebKitContextMenu object to be used as a submenu of an existing
 * #WebKitContextMenu. The context menu of a #WebKitWebView is created by the view
 * and passed as an argument of #WebKitWebView::context-menu signal.
 * To add items to the menu use webkit_context_menu_prepend(),
 * webkit_context_menu_append() or webkit_context_menu_insert().
 * See also webkit_context_menu_new_with_items() to create a #WebKitContextMenu with
 * a list of initial items.
 *
 * Returns: The newly created #WebKitContextMenu object
 */
WebKitContextMenu* webkit_context_menu_new()
{
    return WEBKIT_CONTEXT_MENU(g_object_new(WEBKIT_TYPE_CONTEXT_MENU, NULL));
}

/**
 * webkit_context_menu_new_with_items:
 * @items: (element-type WebKitContextMenuItem): a #GList of #WebKitContextMenuItem
 *
 * Creates a new #WebKitContextMenu object to be used as a submenu of an existing
 * #WebKitContextMenu with the given initial items.
 * See also webkit_context_menu_new()
 *
 * Returns: The newly created #WebKitContextMenu object
 */
WebKitContextMenu* webkit_context_menu_new_with_items(GList* items)
{
    WebKitContextMenu* menu = webkit_context_menu_new();
    g_list_foreach(items, reinterpret_cast<GFunc>(g_object_ref_sink), 0);
    menu->priv->items = g_list_copy(items);

    return menu;
}

/**
 * webkit_context_menu_prepend:
 * @menu: a #WebKitContextMenu
 * @item: the #WebKitContextMenuItem to add
 *
 * Adds @item at the beginning of the @menu.
 */
void webkit_context_menu_prepend(WebKitContextMenu* menu, WebKitContextMenuItem* item)
{
    webkit_context_menu_insert(menu, item, 0);
}

/**
 * webkit_context_menu_append:
 * @menu: a #WebKitContextMenu
 * @item: the #WebKitContextMenuItem to add
 *
 * Adds @item at the end of the @menu.
 */
void webkit_context_menu_append(WebKitContextMenu* menu, WebKitContextMenuItem* item)
{
    webkit_context_menu_insert(menu, item, -1);
}

/**
 * webkit_context_menu_insert:
 * @menu: a #WebKitContextMenu
 * @item: the #WebKitContextMenuItem to add
 * @position: the position to insert the item
 *
 * Inserts @item into the @menu at the given position.
 * If @position is negative, or is larger than the number of items
 * in the #WebKitContextMenu, the item is added on to the end of
 * the @menu. The first position is 0.
 */
void webkit_context_menu_insert(WebKitContextMenu* menu, WebKitContextMenuItem* item, int position)
{
    g_return_if_fail(WEBKIT_IS_CONTEXT_MENU(menu));
    g_return_if_fail(WEBKIT_IS_CONTEXT_MENU_ITEM(item));

    g_object_ref_sink(item);
    menu->priv->items = g_list_insert(menu->priv->items, item, position);
}

/**
 * webkit_context_menu_move_item:
 * @menu: a #WebKitContextMenu
 * @item: the #WebKitContextMenuItem to add
 * @position: the new position to move the item
 *
 * Moves @item to the given position in the @menu.
 * If @position is negative, or is larger than the number of items
 * in the #WebKitContextMenu, the item is added on to the end of
 * the @menu.
 * The first position is 0.
 */
void webkit_context_menu_move_item(WebKitContextMenu* menu, WebKitContextMenuItem* item, int position)
{
    g_return_if_fail(WEBKIT_IS_CONTEXT_MENU(menu));
    g_return_if_fail(WEBKIT_IS_CONTEXT_MENU_ITEM(item));

    if (!g_list_find(menu->priv->items, item))
        return;

    menu->priv->items = g_list_remove(menu->priv->items, item);
    menu->priv->items = g_list_insert(menu->priv->items, item, position);
}

/**
 * webkit_context_menu_get_items:
 * @menu: a #WebKitContextMenu
 *
 * Returns the item list of @menu.
 *
 * Returns: (element-type WebKitContextMenuItem) (transfer none): a #GList of
 *    #WebKitContextMenuItem<!-- -->s
 */
GList* webkit_context_menu_get_items(WebKitContextMenu* menu)
{
    g_return_val_if_fail(WEBKIT_IS_CONTEXT_MENU(menu), 0);

    return menu->priv->items;
}

/**
 * webkit_context_menu_get_n_items:
 * @menu: a #WebKitContextMenu
 *
 * Gets the length of the @menu.
 *
 * Returns: the number of #WebKitContextMenuItem<!-- -->s in @menu
 */
guint webkit_context_menu_get_n_items(WebKitContextMenu* menu)
{
    g_return_val_if_fail(WEBKIT_IS_CONTEXT_MENU(menu), 0);

    return g_list_length(menu->priv->items);
}

/**
 * webkit_context_menu_first:
 * @menu: a #WebKitContextMenu
 *
 * Gets the first item in the @menu.
 *
 * Returns: (transfer none): the first #WebKitContextMenuItem of @menu,
 *    or %NULL if the #WebKitContextMenu is empty.
 */
WebKitContextMenuItem* webkit_context_menu_first(WebKitContextMenu* menu)
{
    g_return_val_if_fail(WEBKIT_IS_CONTEXT_MENU(menu), 0);

    return menu->priv->items ? WEBKIT_CONTEXT_MENU_ITEM(menu->priv->items->data) : 0;
}

/**
 * webkit_context_menu_last:
 * @menu: a #WebKitContextMenu
 *
 * Gets the last item in the @menu.
 *
 * Returns: (transfer none): the last #WebKitContextMenuItem of @menu,
 *    or %NULL if the #WebKitContextMenu is empty.
 */
WebKitContextMenuItem* webkit_context_menu_last(WebKitContextMenu* menu)
{
    g_return_val_if_fail(WEBKIT_IS_CONTEXT_MENU(menu), 0);

    GList* last = g_list_last(menu->priv->items);
    return last ? WEBKIT_CONTEXT_MENU_ITEM(last->data) : 0;
}

/**
 * webkit_context_menu_get_item_at_position:
 * @menu: a #WebKitContextMenu
 * @position: the position of the item, counting from 0
 *
 * Gets the item at the given position in the @menu.
 *
 * Returns: (transfer none): the #WebKitContextMenuItem at position @position in @menu,
 *    or %NULL if the position is off the end of the @menu.
 */
WebKitContextMenuItem* webkit_context_menu_get_item_at_position(WebKitContextMenu* menu, unsigned position)
{
    g_return_val_if_fail(WEBKIT_IS_CONTEXT_MENU(menu), 0);

    gpointer item = g_list_nth_data(menu->priv->items, position);
    return item ? WEBKIT_CONTEXT_MENU_ITEM(item) : 0;
}

/**
 * webkit_context_menu_remove:
 * @menu: a #WebKitContextMenu
 * @item: the #WebKitContextMenuItem to remove
 *
 * Removes @item from the @menu.
 * See also webkit_context_menu_remove_all() to remove all items.
 */
void webkit_context_menu_remove(WebKitContextMenu* menu, WebKitContextMenuItem* item)
{
    g_return_if_fail(WEBKIT_IS_CONTEXT_MENU(menu));
    g_return_if_fail(WEBKIT_IS_CONTEXT_MENU_ITEM(item));

    if (!g_list_find(menu->priv->items, item))
        return;

    menu->priv->items = g_list_remove(menu->priv->items, item);
    g_object_unref(item);
}

/**
 * webkit_context_menu_remove_all:
 * @menu: a #WebKitContextMenu
 *
 * Removes all items of the @menu.
 */
void webkit_context_menu_remove_all(WebKitContextMenu* menu)
{
    g_return_if_fail(WEBKIT_IS_CONTEXT_MENU(menu));

    g_list_free_full(menu->priv->items, reinterpret_cast<GDestroyNotify>(g_object_unref));
    menu->priv->items = 0;
}
