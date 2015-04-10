/*
 * Copyright (C) 2008, 2009 Jan Michael C. Alonzo
 * Copyright (C) 2009 Igalia S.L.
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
#include "webkitwebhistoryitem.h"

#include "HistoryItem.h"
#include "KURL.h"
#include "webkitglobalsprivate.h"
#include "webkitwebhistoryitemprivate.h"
#include <glib.h>
#include <glib/gi18n-lib.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

/**
 * SECTION:webkitwebhistoryitem
 * @short_description: One item of the #WebKitWebBackForwardList and or global history
 * @see_also: #WebKitWebBackForwardList
 *
 * A history item consists out of a title and a uri. It can be part of the
 * #WebKitWebBackForwardList and the global history. The global history is used
 * for coloring the links of visited sites.  #WebKitWebHistoryItem's constructed with
 * #webkit_web_history_item_new and #webkit_web_history_item_new_with_data are
 * automatically added to the global history.
 *
 * <informalexample><programlisting>
 * /<!-- -->* Inject a visited page into the global history *<!-- -->/
 * webkit_web_history_item_new_with_data("http://www.gnome.org/", "GNOME: The Free Software Desktop Project");
 * webkit_web_history_item_new_with_data("http://www.webkit.org/", "The WebKit Open Source Project");
 * </programlisting></informalexample>
 *
 */

using namespace WebKit;

struct _WebKitWebHistoryItemPrivate {
    WebCore::HistoryItem* historyItem;

    WTF::CString title;
    WTF::CString alternateTitle;
    WTF::CString uri;
    WTF::CString originalUri;

    gboolean disposed;
};

enum {
    PROP_0,

    PROP_TITLE,
    PROP_ALTERNATE_TITLE,
    PROP_URI,
    PROP_ORIGINAL_URI,
    PROP_LAST_VISITED_TIME
};

G_DEFINE_TYPE(WebKitWebHistoryItem, webkit_web_history_item, G_TYPE_OBJECT);

static void webkit_web_history_item_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec);

static void webkit_web_history_item_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec);

GHashTable* webkit_history_items()
{
    static GHashTable* historyItems = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, g_object_unref);
    return historyItems;
}

void webkit_history_item_add(WebKitWebHistoryItem* webHistoryItem, WebCore::HistoryItem* historyItem)
{
    g_return_if_fail(WEBKIT_IS_WEB_HISTORY_ITEM(webHistoryItem));

    GHashTable* table = webkit_history_items();
    g_hash_table_insert(table, historyItem, webHistoryItem);
}

static void webkit_web_history_item_dispose(GObject* object)
{
    WebKitWebHistoryItem* webHistoryItem = WEBKIT_WEB_HISTORY_ITEM(object);
    WebKitWebHistoryItemPrivate* priv = webHistoryItem->priv;

    if (!priv->disposed) {
        WebCore::HistoryItem* item = core(webHistoryItem);
        item->deref();
        priv->disposed = true;
    }

    G_OBJECT_CLASS(webkit_web_history_item_parent_class)->dispose(object);
}

static void webkit_web_history_item_finalize(GObject* object)
{
    WebKitWebHistoryItem* webHistoryItem = WEBKIT_WEB_HISTORY_ITEM(object);
    WebKitWebHistoryItemPrivate* priv = webHistoryItem->priv;

    priv->title = WTF::CString();
    priv->alternateTitle = WTF::CString();
    priv->uri = WTF::CString();
    priv->originalUri = WTF::CString();

    G_OBJECT_CLASS(webkit_web_history_item_parent_class)->finalize(object);
}

static void webkit_web_history_item_class_init(WebKitWebHistoryItemClass* klass)
{
    GObjectClass* gobject_class = G_OBJECT_CLASS(klass);

    gobject_class->dispose = webkit_web_history_item_dispose;
    gobject_class->finalize = webkit_web_history_item_finalize;
    gobject_class->set_property = webkit_web_history_item_set_property;
    gobject_class->get_property = webkit_web_history_item_get_property;

    webkitInit();

    /**
    * WebKitWebHistoryItem:title:
    *
    * The title of the history item.
    *
    * Since: 1.0.2
    */
    g_object_class_install_property(gobject_class,
                                    PROP_TITLE,
                                    g_param_spec_string(
                                    "title",
                                    _("Title"),
                                    _("The title of the history item"),
                                    NULL,
                                    WEBKIT_PARAM_READABLE));

    /**
    * WebKitWebHistoryItem:alternate-title:
    *
    * The alternate title of the history item.
    *
    * Since: 1.0.2
    */
    g_object_class_install_property(gobject_class,
                                    PROP_ALTERNATE_TITLE,
                                    g_param_spec_string(
                                    "alternate-title",
                                    _("Alternate Title"),
                                    _("The alternate title of the history item"),
                                    NULL,
                                    WEBKIT_PARAM_READWRITE));

    /**
    * WebKitWebHistoryItem:uri:
    *
    * The URI of the history item.
    *
    * Since: 1.0.2
    */
    g_object_class_install_property(gobject_class,
                                    PROP_URI,
                                    g_param_spec_string(
                                    "uri",
                                    _("URI"),
                                    _("The URI of the history item"),
                                    NULL,
                                    WEBKIT_PARAM_READABLE));

    /**
    * WebKitWebHistoryItem:original-uri:
    *
    * The original URI of the history item.
    *
    * Since: 1.0.2
    */
    g_object_class_install_property(gobject_class,
                                    PROP_ORIGINAL_URI,
                                    g_param_spec_string(
                                    "original-uri",
                                    _("Original URI"),
                                    _("The original URI of the history item"),
                                    NULL,
                                    WEBKIT_PARAM_READABLE));

   /**
    * WebKitWebHistoryItem:last-visited-time:
    *
    * The time at which the history item was last visited.
    *
    * Since: 1.0.2
    */
    g_object_class_install_property(gobject_class,
                                    PROP_LAST_VISITED_TIME,
                                    g_param_spec_double(
                                    "last-visited-time",
                                    _("Last visited Time"),
                                    _("The time at which the history item was last visited"),
                                    0, G_MAXDOUBLE, 0,
                                    WEBKIT_PARAM_READABLE));

    g_type_class_add_private(gobject_class, sizeof(WebKitWebHistoryItemPrivate));
}

static void webkit_web_history_item_init(WebKitWebHistoryItem* webHistoryItem)
{
    webHistoryItem->priv = G_TYPE_INSTANCE_GET_PRIVATE(webHistoryItem, WEBKIT_TYPE_WEB_HISTORY_ITEM, WebKitWebHistoryItemPrivate);
}

static void webkit_web_history_item_set_property(GObject* object, guint prop_id, const GValue* value, GParamSpec* pspec)
{
    WebKitWebHistoryItem* webHistoryItem = WEBKIT_WEB_HISTORY_ITEM(object);

    switch(prop_id) {
    case PROP_ALTERNATE_TITLE:
        webkit_web_history_item_set_alternate_title(webHistoryItem, g_value_get_string(value));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

static void webkit_web_history_item_get_property(GObject* object, guint prop_id, GValue* value, GParamSpec* pspec)
{
    WebKitWebHistoryItem* webHistoryItem = WEBKIT_WEB_HISTORY_ITEM(object);

    switch (prop_id) {
    case PROP_TITLE:
        g_value_set_string(value, webkit_web_history_item_get_title(webHistoryItem));
        break;
    case PROP_ALTERNATE_TITLE:
        g_value_set_string(value, webkit_web_history_item_get_alternate_title(webHistoryItem));
        break;
    case PROP_URI:
        g_value_set_string(value, webkit_web_history_item_get_uri(webHistoryItem));
        break;
    case PROP_ORIGINAL_URI:
        g_value_set_string(value, webkit_web_history_item_get_original_uri(webHistoryItem));
        break;
    case PROP_LAST_VISITED_TIME:
        g_value_set_double(value, webkit_web_history_item_get_last_visited_time(webHistoryItem));
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
        break;
    }
}

/* Helper function to create a new WebHistoryItem instance when needed */
WebKitWebHistoryItem* webkit_web_history_item_new_with_core_item(PassRefPtr<WebCore::HistoryItem> historyItem)
{
    return kit(historyItem);
}


/**
 * webkit_web_history_item_new:
 *
 * Creates a new #WebKitWebHistoryItem instance
 *
 * Return value: the new #WebKitWebHistoryItem
 */
WebKitWebHistoryItem* webkit_web_history_item_new()
{
    WebKitWebHistoryItem* webHistoryItem = WEBKIT_WEB_HISTORY_ITEM(g_object_new(WEBKIT_TYPE_WEB_HISTORY_ITEM, NULL));
    WebKitWebHistoryItemPrivate* priv = webHistoryItem->priv;

    RefPtr<WebCore::HistoryItem> item = WebCore::HistoryItem::create();
    priv->historyItem = item.release().leakRef();
    webkit_history_item_add(webHistoryItem, priv->historyItem);

    return webHistoryItem;
}

/**
 * webkit_web_history_item_new_with_data:
 * @uri: the URI of the page
 * @title: the title of the page
 *
 * Creates a new #WebKitWebHistoryItem with the given URI and title
 *
 * Return value: the new #WebKitWebHistoryItem
 */
WebKitWebHistoryItem* webkit_web_history_item_new_with_data(const gchar* uri, const gchar* title)
{
    WebKitWebHistoryItem* webHistoryItem = WEBKIT_WEB_HISTORY_ITEM(g_object_new(WEBKIT_TYPE_WEB_HISTORY_ITEM, NULL));
    WebKitWebHistoryItemPrivate* priv = webHistoryItem->priv;

    WebCore::KURL historyUri(WebCore::KURL(), uri);
    WTF::String historyTitle = WTF::String::fromUTF8(title);
    RefPtr<WebCore::HistoryItem> item = WebCore::HistoryItem::create(historyUri, historyTitle, 0);
    priv->historyItem = item.release().leakRef();
    webkit_history_item_add(webHistoryItem, priv->historyItem);

    return webHistoryItem;
}

/**
 * webkit_web_history_item_get_title:
 * @web_history_item: a #WebKitWebHistoryItem
 *
 * Returns: the page title of @web_history_item
 */
const gchar* webkit_web_history_item_get_title(WebKitWebHistoryItem* webHistoryItem)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_HISTORY_ITEM(webHistoryItem), NULL);

    WebCore::HistoryItem* item = core(webHistoryItem);

    g_return_val_if_fail(item, NULL);

    WebKitWebHistoryItemPrivate* priv = webHistoryItem->priv;
    priv->title = item->title().utf8();

    return priv->title.data();
}

/**
 * webkit_web_history_item_get_alternate_title:
 * @web_history_item: a #WebKitWebHistoryItem
 *
 * Returns the alternate title of @web_history_item
 *
 * Return value: the alternate title of @web_history_item
 */
const gchar* webkit_web_history_item_get_alternate_title(WebKitWebHistoryItem* webHistoryItem)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_HISTORY_ITEM(webHistoryItem), NULL);

    WebCore::HistoryItem* item = core(webHistoryItem);

    g_return_val_if_fail(item, NULL);

    WebKitWebHistoryItemPrivate* priv = webHistoryItem->priv;
    priv->alternateTitle = item->alternateTitle().utf8();

    return priv->alternateTitle.data();
}

/**
 * webkit_web_history_item_set_alternate_title:
 * @web_history_item: a #WebKitWebHistoryItem
 * @title: the alternate title for @this history item
 *
 * Sets an alternate title for @web_history_item
 */
void webkit_web_history_item_set_alternate_title(WebKitWebHistoryItem* webHistoryItem, const gchar* title)
{
    g_return_if_fail(WEBKIT_IS_WEB_HISTORY_ITEM(webHistoryItem));
    g_return_if_fail(title);

    WebCore::HistoryItem* item = core(webHistoryItem);

    item->setAlternateTitle(WTF::String::fromUTF8(title));
    g_object_notify(G_OBJECT(webHistoryItem), "alternate-title");
}

/**
 * webkit_web_history_item_get_uri:
 * @web_history_item: a #WebKitWebHistoryItem
 *
 * Returns the URI of @this
 *
 * Return value: the URI of @web_history_item
 */
const gchar* webkit_web_history_item_get_uri(WebKitWebHistoryItem* webHistoryItem)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_HISTORY_ITEM(webHistoryItem), NULL);

    WebCore::HistoryItem* item = core(WEBKIT_WEB_HISTORY_ITEM(webHistoryItem));

    g_return_val_if_fail(item, NULL);

    WebKitWebHistoryItemPrivate* priv = webHistoryItem->priv;
    priv->uri = item->urlString().utf8();

    return priv->uri.data();
}

/**
 * webkit_web_history_item_get_original_uri:
 * @web_history_item: a #WebKitWebHistoryItem
 *
 * Returns the original URI of @web_history_item.
 *
 * Return value: the original URI of @web_history_item
 */
const gchar* webkit_web_history_item_get_original_uri(WebKitWebHistoryItem* webHistoryItem)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_HISTORY_ITEM(webHistoryItem), NULL);

    WebCore::HistoryItem* item = core(WEBKIT_WEB_HISTORY_ITEM(webHistoryItem));

    g_return_val_if_fail(item, NULL);

    WebKitWebHistoryItemPrivate* priv = webHistoryItem->priv;
    priv->originalUri = item->originalURLString().utf8();

    return webHistoryItem->priv->originalUri.data();
}

/**
 * webkit_web_history_item_get_last_visisted_time :
 * @web_history_item: a #WebKitWebHistoryItem
 *
 * Returns the last time @web_history_item was visited
 *
 * Return value: the time in seconds this @web_history_item was last visited
 */
gdouble webkit_web_history_item_get_last_visited_time(WebKitWebHistoryItem* webHistoryItem)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_HISTORY_ITEM(webHistoryItem), 0);

    WebCore::HistoryItem* item = core(WEBKIT_WEB_HISTORY_ITEM(webHistoryItem));

    g_return_val_if_fail(item, 0);

    return item->lastVisitedTime();
}

/**
 * webkit_web_history_item_copy:
 * @web_history_item: a #WebKitWebHistoryItem
 *
 * Makes a copy of the item for use with other WebView objects.
 *
 * Since: 1.1.18
 *
 * Return value: (transfer full): the new #WebKitWebHistoryItem.
 */
WebKitWebHistoryItem* webkit_web_history_item_copy(WebKitWebHistoryItem* self)
{
    WebKitWebHistoryItemPrivate* selfPrivate = self->priv;

    WebKitWebHistoryItem* item = WEBKIT_WEB_HISTORY_ITEM(g_object_new(WEBKIT_TYPE_WEB_HISTORY_ITEM, 0));
    WebKitWebHistoryItemPrivate* priv = item->priv;

    priv->title = selfPrivate->title;
    priv->alternateTitle = selfPrivate->alternateTitle;
    priv->uri = selfPrivate->uri;
    priv->originalUri = selfPrivate->originalUri;

    priv->historyItem = selfPrivate->historyItem->copy().leakRef();

    return item;
}

/* private methods */

gchar* webkit_web_history_item_get_target(WebKitWebHistoryItem* webHistoryItem)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_HISTORY_ITEM(webHistoryItem), NULL);

    WebCore::HistoryItem* item = core(webHistoryItem);

    g_return_val_if_fail(item, NULL);

    WTF::CString t = item->target().utf8();
    return g_strdup(t.data());
}

gboolean webkit_web_history_item_is_target_item(WebKitWebHistoryItem* webHistoryItem)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_HISTORY_ITEM(webHistoryItem), false);

    WebCore::HistoryItem* item = core(webHistoryItem);

    g_return_val_if_fail(item, false);

    return item->isTargetItem();
}

GList* webkit_web_history_item_get_children(WebKitWebHistoryItem* webHistoryItem)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_HISTORY_ITEM(webHistoryItem), NULL);

    WebCore::HistoryItem* item = core(webHistoryItem);

    g_return_val_if_fail(item, NULL);

    const WebCore::HistoryItemVector& children = item->children();
    if (!children.size())
        return NULL;

    unsigned size = children.size();
    GList* kids = NULL;
    for (unsigned i = 0; i < size; ++i)
        kids = g_list_prepend(kids, kit(children[i].get()));

    return g_list_reverse(kids);
}

WebCore::HistoryItem* WebKit::core(WebKitWebHistoryItem* webHistoryItem)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_HISTORY_ITEM(webHistoryItem), NULL);

    return webHistoryItem->priv->historyItem;
}

WebKitWebHistoryItem* WebKit::kit(PassRefPtr<WebCore::HistoryItem> historyItem)
{
    g_return_val_if_fail(historyItem, NULL);

    RefPtr<WebCore::HistoryItem> item = historyItem;
    GHashTable* table = webkit_history_items();
    WebKitWebHistoryItem* webHistoryItem = (WebKitWebHistoryItem*) g_hash_table_lookup(table, item.get());

    if (!webHistoryItem) {
        webHistoryItem = WEBKIT_WEB_HISTORY_ITEM(g_object_new(WEBKIT_TYPE_WEB_HISTORY_ITEM, NULL));
        WebKitWebHistoryItemPrivate* priv = webHistoryItem->priv;

        priv->historyItem = item.release().leakRef();
        webkit_history_item_add(webHistoryItem, priv->historyItem);
    }

    return webHistoryItem;
}

