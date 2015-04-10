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
#include "WebKitBackForwardListItem.h"

#include "WebKitBackForwardListPrivate.h"
#include "WebKitPrivate.h"
#include <wtf/HashMap.h>
#include <wtf/gobject/GRefPtr.h>
#include <wtf/text/CString.h>

using namespace WebKit;

/**
 * SECTION: WebKitBackForwardListItem
 * @Short_description: One item of the #WebKitBackForwardList
 * @Title: WebKitBackForwardListItem
 * @See_also: #WebKitBackForwardList
 *
 * A history item is part of the #WebKitBackForwardList and consists
 * out of a title and a URI.
 *
 */

struct _WebKitBackForwardListItemPrivate {
    RefPtr<WebBackForwardListItem> webListItem;
    CString uri;
    CString title;
    CString originalURI;
};

WEBKIT_DEFINE_TYPE(WebKitBackForwardListItem, webkit_back_forward_list_item, G_TYPE_INITIALLY_UNOWNED)

static void webkit_back_forward_list_item_class_init(WebKitBackForwardListItemClass* listItemClass)
{
}

typedef HashMap<WebBackForwardListItem*, WebKitBackForwardListItem*> HistoryItemsMap;

static HistoryItemsMap& historyItemsMap()
{
    DEFINE_STATIC_LOCAL(HistoryItemsMap, itemsMap, ());
    return itemsMap;
}

static void webkitBackForwardListItemFinalized(gpointer webListItem, GObject* finalizedListItem)
{
    ASSERT(G_OBJECT(historyItemsMap().get(static_cast<WebBackForwardListItem*>(webListItem))) == finalizedListItem);
    historyItemsMap().remove(static_cast<WebBackForwardListItem*>(webListItem));
}

WebKitBackForwardListItem* webkitBackForwardListItemGetOrCreate(WebBackForwardListItem* webListItem)
{
    if (!webListItem)
        return 0;

    WebKitBackForwardListItem* listItem = historyItemsMap().get(webListItem);
    if (listItem)
        return listItem;

    listItem = WEBKIT_BACK_FORWARD_LIST_ITEM(g_object_new(WEBKIT_TYPE_BACK_FORWARD_LIST_ITEM, NULL));
    listItem->priv->webListItem = webListItem;

    g_object_weak_ref(G_OBJECT(listItem), webkitBackForwardListItemFinalized, webListItem);
    historyItemsMap().set(webListItem, listItem);

    return listItem;
}

WebBackForwardListItem* webkitBackForwardListItemGetItem(WebKitBackForwardListItem* listItem)
{
    return listItem->priv->webListItem.get();
}

/**
 * webkit_back_forward_list_item_get_uri:
 * @list_item: a #WebKitBackForwardListItem
 *
 * This URI may differ from the original URI if the page was,
 * for example, redirected to a new location.
 * See also webkit_back_forward_list_item_get_original_uri().
 *
 * Returns: the URI of @list_item or %NULL
 *    when the URI is empty.
 */
const gchar* webkit_back_forward_list_item_get_uri(WebKitBackForwardListItem* listItem)
{
    g_return_val_if_fail(WEBKIT_IS_BACK_FORWARD_LIST_ITEM(listItem), 0);

    WebKitBackForwardListItemPrivate* priv = listItem->priv;
    String url = priv->webListItem->url();
    if (url.isEmpty())
        return 0;

    priv->uri = url.utf8();
    return priv->uri.data();
}

/**
 * webkit_back_forward_list_item_get_title:
 * @list_item: a #WebKitBackForwardListItem
 *
 * Returns: the page title of @list_item or %NULL
 *    when the title is empty.
 */
const gchar* webkit_back_forward_list_item_get_title(WebKitBackForwardListItem* listItem)
{
    g_return_val_if_fail(WEBKIT_IS_BACK_FORWARD_LIST_ITEM(listItem), 0);

    WebKitBackForwardListItemPrivate* priv = listItem->priv;
    String title = priv->webListItem->title();
    if (title.isEmpty())
        return 0;

    priv->title = title.utf8();
    return priv->title.data();
}

/**
 * webkit_back_forward_list_item_get_original_uri:
 * @list_item: a #WebKitBackForwardListItem
 *
 * See also webkit_back_forward_list_item_get_uri().
 *
 * Returns: the original URI of @list_item or %NULL
 *    when the original URI is empty.
 */
const gchar* webkit_back_forward_list_item_get_original_uri(WebKitBackForwardListItem* listItem)
{
    g_return_val_if_fail(WEBKIT_IS_BACK_FORWARD_LIST_ITEM(listItem), 0);

    WebKitBackForwardListItemPrivate* priv = listItem->priv;
    String originalURL = priv->webListItem->originalURL();
    if (originalURL.isEmpty())
        return 0;

    priv->originalURI = originalURL.utf8();
    return priv->originalURI.data();
}
