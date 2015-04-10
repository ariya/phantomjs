/*
 * Copyright (C) 2011 Igalia S.L.
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
#include "WebKitBackForwardList.h"

#include "WebKitBackForwardListPrivate.h"
#include "WebKitMarshal.h"
#include "WebKitPrivate.h"
#include <wtf/gobject/GRefPtr.h>

/**
 * SECTION: WebKitBackForwardList
 * @Short_description: List of visited pages
 * @Title: WebKitBackForwardList
 * @See_also: #WebKitWebView, #WebKitBackForwardListItem
 *
 * WebKitBackForwardList maintains a list of visited pages used to
 * navigate to recent pages. Items are inserted in the list in the
 * order they are visited.
 *
 * WebKitBackForwardList also maintains the notion of the current item
 * (which is always at index 0), the preceding item (which is at index -1),
 * and the following item (which is at index 1).
 * Methods webkit_web_view_go_back() and webkit_web_view_go_forward() move
 * the current item backward or forward by one. Method
 * webkit_web_view_go_to_back_forward_list_item() sets the current item to the
 * specified item. All other methods returning #WebKitBackForwardListItem<!-- -->s
 * do not change the value of the current item, they just return the requested
 * item or items.
 */

using namespace WebKit;

enum {
    CHANGED,

    LAST_SIGNAL
};

typedef HashMap<WebBackForwardListItem*, GRefPtr<WebKitBackForwardListItem> > BackForwardListItemsMap;

struct _WebKitBackForwardListPrivate {
    WebBackForwardList* backForwardItems;
    BackForwardListItemsMap itemsMap;
};

static guint signals[LAST_SIGNAL] = { 0, };

WEBKIT_DEFINE_TYPE(WebKitBackForwardList, webkit_back_forward_list, G_TYPE_OBJECT)

static void webkit_back_forward_list_class_init(WebKitBackForwardListClass* listClass)
{
    /**
     * WebKitBackForwardList::changed:
     * @back_forward_list: the #WebKitBackForwardList on which the signal was emitted
     * @item_added: (allow-none): the #WebKitBackForwardListItem added or %NULL
     * @items_removed: a #GList of #WebKitBackForwardListItem<!-- -->s
     *
     * This signal is emitted when @back_forward_list changes. This happens
     * when the current item is updated, a new item is added or one or more
     * items are removed. Note that both @item_added and @items_removed can
     * %NULL when only the current item is updated. Items are only removed
     * when the list is cleared or the maximum items limit is reached.
     */
    signals[CHANGED] =
        g_signal_new("changed",
                     G_TYPE_FROM_CLASS(listClass),
                     G_SIGNAL_RUN_LAST,
                     0, 0, 0,
                     webkit_marshal_VOID__OBJECT_POINTER,
                     G_TYPE_NONE, 2,
                     WEBKIT_TYPE_BACK_FORWARD_LIST_ITEM,
                     G_TYPE_POINTER);
}

static WebKitBackForwardListItem* webkitBackForwardListGetOrCreateItem(WebKitBackForwardList* list, WebBackForwardListItem* webListItem)
{
    if (!webListItem)
        return 0;

    WebKitBackForwardListPrivate* priv = list->priv;
    GRefPtr<WebKitBackForwardListItem> listItem = priv->itemsMap.get(webListItem);
    if (listItem)
        return listItem.get();

    listItem = webkitBackForwardListItemGetOrCreate(webListItem);
    priv->itemsMap.set(webListItem, listItem);

    return listItem.get();
}

static GList* webkitBackForwardListCreateList(WebKitBackForwardList* list, ImmutableArray* backForwardItems)
{
    if (!backForwardItems)
        return 0;

    GList* returnValue = 0;
    for (size_t i = 0; i < backForwardItems->size(); ++i) {
        WebBackForwardListItem* webItem = static_cast<WebBackForwardListItem*>(backForwardItems->at(i));
        returnValue = g_list_prepend(returnValue, webkitBackForwardListGetOrCreateItem(list, webItem));
    }

    return returnValue;
}

WebKitBackForwardList* webkitBackForwardListCreate(WebBackForwardList* backForwardItems)
{
    WebKitBackForwardList* list = WEBKIT_BACK_FORWARD_LIST(g_object_new(WEBKIT_TYPE_BACK_FORWARD_LIST, NULL));
    list->priv->backForwardItems = backForwardItems;

    return list;
}

void webkitBackForwardListChanged(WebKitBackForwardList* backForwardList, WebBackForwardListItem* webAddedItem, ImmutableArray* webRemovedItems)
{
    WebKitBackForwardListItem* addedItem = webkitBackForwardListGetOrCreateItem(backForwardList, webAddedItem);
    GList* removedItems = 0;

    size_t removedItemsSize = webRemovedItems ? webRemovedItems->size() : 0;
    WebKitBackForwardListPrivate* priv = backForwardList->priv;
    for (size_t i = 0; i < removedItemsSize; ++i) {
        WebBackForwardListItem* webItem = static_cast<WebBackForwardListItem*>(webRemovedItems->at(i));
        removedItems = g_list_prepend(removedItems, g_object_ref(G_OBJECT(priv->itemsMap.get(webItem).get())));
        priv->itemsMap.remove(webItem);
    }

    g_signal_emit(backForwardList, signals[CHANGED], 0, addedItem, removedItems, NULL);
    g_list_free_full(removedItems, static_cast<GDestroyNotify>(g_object_unref));
}

/**
 * webkit_back_forward_list_get_current_item:
 * @back_forward_list: a #WebKitBackForwardList
 *
 * Returns the current item in @back_forward_list.
 *
 * Returns: (transfer none): a #WebKitBackForwardListItem
 *    or %NULL if @back_forward_list is empty.
 */
WebKitBackForwardListItem* webkit_back_forward_list_get_current_item(WebKitBackForwardList* backForwardList)
{
    g_return_val_if_fail(WEBKIT_IS_BACK_FORWARD_LIST(backForwardList), 0);

    return webkitBackForwardListGetOrCreateItem(backForwardList, backForwardList->priv->backForwardItems->currentItem());
}

/**
 * webkit_back_forward_list_get_back_item:
 * @back_forward_list: a #WebKitBackForwardList
 *
 * Returns the item that precedes the current item.
 *
 * Returns: (transfer none): the #WebKitBackForwardListItem
 *    preceding the current item or %NULL.
 */
WebKitBackForwardListItem* webkit_back_forward_list_get_back_item(WebKitBackForwardList* backForwardList)
{
    g_return_val_if_fail(WEBKIT_IS_BACK_FORWARD_LIST(backForwardList), 0);

    return webkitBackForwardListGetOrCreateItem(backForwardList, backForwardList->priv->backForwardItems->backItem());
}

/**
 * webkit_back_forward_list_get_forward_item:
 * @back_forward_list: a #WebKitBackForwardList
 *
 * Returns the item that follows the current item.
 *
 * Returns: (transfer none): the #WebKitBackForwardListItem
 *    following the current item or %NULL.
 */
WebKitBackForwardListItem* webkit_back_forward_list_get_forward_item(WebKitBackForwardList* backForwardList)
{
    g_return_val_if_fail(WEBKIT_IS_BACK_FORWARD_LIST(backForwardList), 0);

    return webkitBackForwardListGetOrCreateItem(backForwardList, backForwardList->priv->backForwardItems->forwardItem());
}

/**
 * webkit_back_forward_list_get_nth_item:
 * @back_forward_list: a #WebKitBackForwardList
 * @index: the index of the item
 *
 * Returns the item at a given index relative to the current item.
 *
 * Returns: (transfer none): the #WebKitBackForwardListItem
 *    located at the specified index relative to the current item.
 */
WebKitBackForwardListItem* webkit_back_forward_list_get_nth_item(WebKitBackForwardList* backForwardList, gint index)
{
    g_return_val_if_fail(WEBKIT_IS_BACK_FORWARD_LIST(backForwardList), 0);

    return webkitBackForwardListGetOrCreateItem(backForwardList, backForwardList->priv->backForwardItems->itemAtIndex(index));
}

/**
 * webkit_back_forward_list_get_length:
 * @back_forward_list: a #WebKitBackForwardList
 *
 * Returns: the length of @back_forward_list.
 */
guint webkit_back_forward_list_get_length(WebKitBackForwardList* backForwardList)
{
    g_return_val_if_fail(WEBKIT_IS_BACK_FORWARD_LIST(backForwardList), 0);

    WebKitBackForwardListPrivate* priv = backForwardList->priv;
    guint currentItem = webkit_back_forward_list_get_current_item(backForwardList) ? 1 : 0;
    return priv->backForwardItems->backListCount() + priv->backForwardItems->forwardListCount() + currentItem;
}

/**
 * webkit_back_forward_list_get_back_list:
 * @back_forward_list: a #WebKitBackForwardList
 *
 * Returns: (element-type WebKit2.BackForwardListItem) (transfer container): a #GList of
 *    items preceding the current item.
 */
GList* webkit_back_forward_list_get_back_list(WebKitBackForwardList* backForwardList)
{
    g_return_val_if_fail(WEBKIT_IS_BACK_FORWARD_LIST(backForwardList), 0);

    return webkit_back_forward_list_get_back_list_with_limit(backForwardList, backForwardList->priv->backForwardItems->backListCount());
}

/**
 * webkit_back_forward_list_get_back_list_with_limit:
 * @back_forward_list: a #WebKitBackForwardList
 * @limit: the number of items to retrieve
 *
 * Returns: (element-type WebKit2.BackForwardListItem) (transfer container): a #GList of
 *    items preceding the current item limited by @limit.
 */
GList* webkit_back_forward_list_get_back_list_with_limit(WebKitBackForwardList* backForwardList, guint limit)
{
    g_return_val_if_fail(WEBKIT_IS_BACK_FORWARD_LIST(backForwardList), 0);

    WebKitBackForwardListPrivate* priv = backForwardList->priv;
    RefPtr<ImmutableArray> immutableArray = priv->backForwardItems->backListAsImmutableArrayWithLimit(limit);
    return webkitBackForwardListCreateList(backForwardList, immutableArray.get());
}

/**
 * webkit_back_forward_list_get_forward_list:
 * @back_forward_list: a #WebKitBackForwardList
 *
 * Returns: (element-type WebKit2.BackForwardListItem) (transfer container): a #GList of
 *    items following the current item.
 */
GList* webkit_back_forward_list_get_forward_list(WebKitBackForwardList* backForwardList)
{
    g_return_val_if_fail(WEBKIT_IS_BACK_FORWARD_LIST(backForwardList), 0);

    return webkit_back_forward_list_get_forward_list_with_limit(backForwardList, backForwardList->priv->backForwardItems->forwardListCount());
}

/**
 * webkit_back_forward_list_get_forward_list_with_limit:
 * @back_forward_list: a #WebKitBackForwardList
 * @limit: the number of items to retrieve
 *
 * Returns: (element-type WebKit2.BackForwardListItem) (transfer container): a #GList of
 *    items following the current item limited by @limit.
 */
GList* webkit_back_forward_list_get_forward_list_with_limit(WebKitBackForwardList* backForwardList, guint limit)
{
    g_return_val_if_fail(WEBKIT_IS_BACK_FORWARD_LIST(backForwardList), 0);

    WebKitBackForwardListPrivate* priv = backForwardList->priv;
    RefPtr<ImmutableArray> immutableArray = priv->backForwardItems->forwardListAsImmutableArrayWithLimit(limit);
    return webkitBackForwardListCreateList(backForwardList, immutableArray.get());
}
